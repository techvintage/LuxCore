/***************************************************************************
 * Copyright 1998-2020 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#include "slg/engines/pathcpu/pathcpu.h"
#include "slg/volumes/volume.h"
#include "slg/utils/varianceclamping.h"
#include "slg/samplers/metropolis.h"

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#include <Windows.h>
#endif

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// PathCPU RenderThread
//------------------------------------------------------------------------------

PathCPURenderThread::PathCPURenderThread(PathCPURenderEngine *engine,
		const u_int index, IntersectionDevice *device) :
		CPUNoTileRenderThread(engine, index, device) {
}

void PathCPURenderThread::RenderFunc() {
	//SLG_LOG("[PathCPURenderEngine::" << threadIndex << "] Rendering thread started");

	//--------------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------------

	PathCPURenderEngine *engine = (PathCPURenderEngine *)renderEngine;
	const PathTracer &pathTracer = engine->pathTracer;

	//Check to see if processor group support is present and then set thread affinity
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined (WIN64)
	bool hasProcessorGroupsSupport = (GetActiveProcessorGroupCount && GetActiveProcessorCount && SetThreadGroupAffinity); 
	if (hasProcessorGroupsSupport)
	{
		auto totalProcessors = 0U;
		int processorIndex = threadIndex % GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

		// Determine which processor group to bind the thread to.
		for (auto i = 0U; i < GetActiveProcessorGroupCount(); ++i)
		{
			totalProcessors += GetActiveProcessorCount(i);
			if (totalProcessors >= processorIndex)
			{
				auto mask = (1ULL << GetActiveProcessorCount(i)) - 1;
				GROUP_AFFINITY groupAffinity = { mask, static_cast<WORD>(i), { 0, 0, 0 } };
				SetThreadGroupAffinity(GetCurrentThread(), &groupAffinity, nullptr);
				break;
			}
		}
	}
#endif
	// (engine->seedBase + 1) seed is used for sharedRndGen
	RandomGenerator *rndGen = new RandomGenerator(engine->seedBase + 1 + threadIndex);

	// Setup the sampler(s)

	Sampler *eyeSampler = nullptr;
	Sampler *lightSampler = nullptr;

	eyeSampler = engine->renderConfig->AllocSampler(rndGen, engine->film,
			nullptr, engine->samplerSharedData, Properties());
	eyeSampler->SetThreadIndex(threadIndex);
	eyeSampler->RequestSamples(PIXEL_NORMALIZED_ONLY, pathTracer.eyeSampleSize);

	if (pathTracer.hybridBackForwardEnable) {
		// Light path sampler is always Metropolis
		Properties props;
		props <<
			Property("sampler.type")("METROPOLIS") <<
			// Disable image plane meaning for samples 0 and 1
			Property("sampler.imagesamples.enable")(false);

		lightSampler = Sampler::FromProperties(props, rndGen, engine->film, nullptr,
				engine->lightSamplerSharedData);
		
		lightSampler->SetThreadIndex(threadIndex);
		lightSampler->RequestSamples(SCREEN_NORMALIZED_ONLY, pathTracer.lightSampleSize);
	}

	// Setup variance clamping
	VarianceClamping varianceClamping(pathTracer.sqrtVarianceClampMaxValue);

	// Setup PathTracer thread state
	PathTracerThreadState pathTracerThreadState(device,
			eyeSampler, lightSampler,
			engine->renderConfig->scene, engine->film,
			&varianceClamping);

	//--------------------------------------------------------------------------
	// Trace paths
	//--------------------------------------------------------------------------

	for (u_int steps = 0; !boost::this_thread::interruption_requested(); ++steps) {
		// Check if we are in pause mode
		if (engine->pauseMode) {
			// Check every 100ms if I have to continue the rendering
			while (!boost::this_thread::interruption_requested() && engine->pauseMode)
				boost::this_thread::sleep(boost::posix_time::millisec(100));

			if (boost::this_thread::interruption_requested())
				break;
		}

		pathTracer.RenderSample(pathTracerThreadState);

#ifdef WIN32
		// Work around Windows bad scheduling
		renderThread->yield();
#endif

		// Check halt conditions
		if (engine->film->GetConvergence() == 1.f)
			break;
		
		if (engine->photonGICache) {
			try {
				const u_int spp = engine->film->GetTotalEyeSampleCount() / engine->film->GetPixelCount();
				engine->photonGICache->Update(threadIndex, spp);
			} catch (boost::thread_interrupted &ti) {
				// I have been interrupted, I must stop
				break;
			}
		}
	}

	delete eyeSampler;
	delete lightSampler;
	delete rndGen;

	threadDone = true;

	// This is done to interrupt thread pending on barrier wait
	// inside engine->photonGICache->Update(). This can happen when an
	// halt condition is satisfied.
	for (u_int i = 0; i < engine->renderThreads.size(); ++i)
		engine->renderThreads[i]->Interrupt();

	//SLG_LOG("[PathCPURenderEngine::" << threadIndex << "] Rendering thread halted");
}
