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

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include "luxrays/devices/oclintersectiondevice.h"

using namespace std;

namespace luxrays {

//------------------------------------------------------------------------------
// OpenCL IntersectionDevice
//------------------------------------------------------------------------------

OpenCLIntersectionDevice::OpenCLIntersectionDevice(
		const Context *context,
		OpenCLDeviceDescription *desc,
		const size_t devIndex) :
		Device(context, desc->GetType(), devIndex), OpenCLDevice(context, desc, devIndex),
		kernel(nullptr) {
}

OpenCLIntersectionDevice::~OpenCLIntersectionDevice() {
}

void OpenCLIntersectionDevice::SetDataSet(DataSet *newDataSet) {
	IntersectionDevice::SetDataSet(newDataSet);

	if (dataSet) {
		const AcceleratorType accelType = dataSet->GetAcceleratorType();
		if (accelType != ACCEL_AUTO) {
			accel = dataSet->GetAccelerator(accelType);
		} else {
			if (dataSet->RequiresInstanceSupport() || dataSet->RequiresMotionBlurSupport())
				accel = dataSet->GetAccelerator(ACCEL_MBVH);
			else
				accel = dataSet->GetAccelerator(ACCEL_BVH);
		}
	}
}

void OpenCLIntersectionDevice::Update() {
	kernel->Update(dataSet);
}

void OpenCLIntersectionDevice::Start() {
	OpenCLDevice::Start();

	// Compile required kernel
	kernel = accel->NewOpenCLKernel(this);
}

void OpenCLIntersectionDevice::Stop() {
	OpenCLDevice::Stop();

	delete kernel;
	kernel = nullptr;

}

//------------------------------------------------------------------------------
// OpenCL Device specific methods
//------------------------------------------------------------------------------

void OpenCLIntersectionDevice::AllocBufferRO(cl::Buffer **buff, void *src, const size_t size, const std::string &desc) {
	AllocBuffer(src ? (CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR) : CL_MEM_READ_ONLY, buff, src, size, desc);
}

void OpenCLIntersectionDevice::AllocBufferRW(cl::Buffer **buff, void *src, const size_t size, const std::string &desc) {
	AllocBuffer(src ? (CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR) : CL_MEM_READ_WRITE, buff, src, size, desc);
}

void OpenCLIntersectionDevice::FreeBuffer(cl::Buffer **buff) {
	if (*buff) {
		FreeMemory((*buff)->getInfo<CL_MEM_SIZE>());
		delete *buff;
		*buff = nullptr;
	}
}

}

#endif
