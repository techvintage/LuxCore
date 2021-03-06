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

#if defined(LUXRAYS_ENABLE_CUDA)

#include <boost/regex.hpp>

#include "luxrays/devices/cudadevice.h"
#include "luxrays/kernels/kernels.h"

using namespace std;
using namespace luxrays;

//------------------------------------------------------------------------------
// OpenCL Device Description
//------------------------------------------------------------------------------

CUDADeviceDescription::CUDADeviceDescription(CUdevice dev, const size_t devIndex) :
		DeviceDescription("CUDAInitializingDevice", DEVICE_TYPE_CUDA_GPU),
		deviceIndex(devIndex), cudaDevice(dev) {
	char buff[128];
    CHECK_CUDA_ERROR(cuDeviceGetName(buff, 128, cudaDevice));
	name = string(buff);
}

CUDADeviceDescription::~CUDADeviceDescription() {
	CHECK_CUDA_ERROR(cuDevicePrimaryCtxRelease(cudaDevice));
}

int CUDADeviceDescription::GetComputeUnits() const {
	int major;
	CHECK_CUDA_ERROR(cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, cudaDevice));
	int minor;
	CHECK_CUDA_ERROR(cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, cudaDevice));
	
	typedef struct {
		int SM;
		int Cores;
	} SM2Cores;

	static SM2Cores archCoresPerSM[] = {
		// 0xMm (hexidecimal notation) where M = SM Major version and m = SM minor version
		{0x30, 192},
		{0x32, 192},
		{0x35, 192},
		{0x37, 192},
		{0x50, 128},
		{0x52, 128},
		{0x53, 128},
		{0x60, 64},
		{0x61, 128},
		{0x62, 128},
		{0x70, 64},
		{0x72, 64},
		{0x75, 64},
		{-1, -1}
	};

	int index = 0;
	while (archCoresPerSM[index].SM != -1) {
		if (archCoresPerSM[index].SM == ((major << 4) + minor))
			return archCoresPerSM[index].Cores;

		index++;
	}

	return DeviceDescription::GetComputeUnits();
}

u_int CUDADeviceDescription::GetNativeVectorWidthFloat() const {
	return 1;
}

size_t CUDADeviceDescription::GetMaxMemory() const {
	size_t bytes;
	CHECK_CUDA_ERROR(cuDeviceTotalMem(&bytes, cudaDevice));
	
	return bytes;
}

size_t CUDADeviceDescription::GetMaxMemoryAllocSize() const {
	return GetMaxMemory();
}

void CUDADeviceDescription::AddDeviceDescs(vector<DeviceDescription *> &descriptions) {
	int devCount;
	CHECK_CUDA_ERROR(cuDeviceGetCount(&devCount));

	for (int i = 0; i < devCount; i++) {
		CUdevice device;
		CHECK_CUDA_ERROR(cuDeviceGet(&device, i));

		CUDADeviceDescription *desc = new CUDADeviceDescription(device, i);

		descriptions.push_back(desc);
	}
}


//------------------------------------------------------------------------------
// CUDADevice
//------------------------------------------------------------------------------

CUDADevice::CUDADevice(
		const Context *context,
		CUDADeviceDescription *desc,
		const size_t devIndex) :
		Device(context, desc->GetType(), devIndex),
		deviceDesc(desc) {
	deviceName = (desc->GetName() + " Intersect").c_str();
}

CUDADevice::~CUDADevice() {
}

void CUDADevice::Start() {
	HardwareDevice::Start();

	CHECK_CUDA_ERROR(cuCtxCreate(&cudaContext, CU_CTX_SCHED_YIELD, deviceDesc->GetCUDADevice()));
}

void CUDADevice::Stop() {
	// Free all loaded modules
	for (auto &m : loadedModules) {
		CHECK_CUDA_ERROR(cuModuleUnload(m));
	}
	loadedModules.clear();

	CHECK_CUDA_ERROR(cuCtxDestroy(cudaContext));
		
	HardwareDevice::Stop();
}

//------------------------------------------------------------------------------
// Kernels handling for hardware (aka GPU) only applications
//------------------------------------------------------------------------------

void CUDADevice::CompileProgram(HardwareDeviceProgram **program,
		const std::string &programParameters, const std::string &programSource,	
		const std::string &programName) {
	LR_LOG(deviceContext, "[" << programName << "] Defined symbols: " << programParameters);
	LR_LOG(deviceContext, "[" << programName << "] Compiling kernels");

	const string cudaProgramParameters = "-D LUXRAYS_CUDA_DEVICE " +
		programParameters;

	const string cudaProgramSource =
		luxrays::ocl::KernelSource_cudadevice_oclemul_types +
		luxrays::ocl::KernelSource_cudadevice_math +
		luxrays::ocl::KernelSource_cudadevice_oclemul_funcs +
		programSource;
	nvrtcProgram prog;
	CHECK_NVRTC_ERROR(nvrtcCreateProgram(&prog, cudaProgramSource.c_str(), programName.c_str(), 0, nullptr, nullptr));

    #if defined (__APPLE__)
        boost::regex paramsRE("/\\-D\\s+\\w+\\s*=\\s*[+-\\w\\d]+|\\-D\\s+\\w+/");
    #else
        boost::regex paramsRE("\\-D\\s+\\w+\\s*=\\s*[+-\\w\\d]+|\\-D\\s+\\w+");
    #endif
    
	boost::sregex_token_iterator paramsIter(cudaProgramParameters.begin(), cudaProgramParameters.end(), paramsRE);
	boost::sregex_token_iterator paramsEnd;
	vector<string> cudaOptsStr;
	vector<const char *> cudaOpts;

	cudaOptsStr.push_back("--device-as-default-execution-space");
	cudaOpts.push_back(cudaOptsStr.back().c_str());
    
	while (paramsIter != paramsEnd) {
		cudaOptsStr.push_back(*paramsIter++);
		cudaOpts.push_back(cudaOptsStr.back().c_str());
	}

	const nvrtcResult compilationResult = nvrtcCompileProgram(prog,
			cudaOpts.size(),
			(cudaOpts.size() > 0) ? &cudaOpts[0] : nullptr);
	if (compilationResult != NVRTC_SUCCESS) {
		size_t logSize;
		CHECK_NVRTC_ERROR(nvrtcGetProgramLogSize(prog, &logSize));
		unique_ptr<char> log(new char[logSize]);
		CHECK_NVRTC_ERROR(nvrtcGetProgramLog(prog, log.get()));

		LR_LOG(deviceContext, "[" << programName << "] program compilation error" << endl << log.get());

		throw runtime_error(programName + " program compilation error");
	}

	if (!*program)
		*program = new CUDADeviceProgram();
	
	CUDADeviceProgram *cudaDeviceProgram = dynamic_cast<CUDADeviceProgram *>(*program);
	assert (cudaDeviceProgram);

	// Obtain PTX from the program.
	size_t ptxSize;
	CHECK_NVRTC_ERROR(nvrtcGetPTXSize(prog, &ptxSize));
	char *ptx = new char[ptxSize];
	CHECK_NVRTC_ERROR(nvrtcGetPTX(prog, ptx));

	CUmodule module;
	CHECK_CUDA_ERROR(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));

	cudaDeviceProgram->Set(prog, module);
	
	loadedModules.push_back(module);
}

void CUDADevice::GetKernel(HardwareDeviceProgram *program,
		HardwareDeviceKernel **kernel, const string &kernelName) {
	if (!*kernel)
		*kernel = new CUDADeviceKernel();

	CUDADeviceKernel *cudaDeviceKernel = dynamic_cast<CUDADeviceKernel *>(*kernel);
	assert (cudaDeviceKernel);

	CUDADeviceProgram *cudaDeviceProgram = dynamic_cast<CUDADeviceProgram *>(program);
	assert (cudaDeviceProgram);

	CUfunction function;
	CHECK_CUDA_ERROR(cuModuleGetFunction(&function, cudaDeviceProgram->GetModule(), kernelName.c_str()));
	
	cudaDeviceKernel->Set(function);
}

void CUDADevice::SetKernelArg(HardwareDeviceKernel *kernel,
		const u_int index, const size_t size, const void *arg) {
	assert (kernel);
	assert (!kernel->IsNull());

	CUDADeviceKernel *cudaDeviceKernel = dynamic_cast<CUDADeviceKernel *>(kernel);
	assert (cudaDeviceKernel);

	if (index >= cudaDeviceKernel->args.size())
		cudaDeviceKernel->args.resize(index + 1, nullptr);

	void *argCpy;
	if (arg) {
		// Copy the argument
		argCpy = new char[size];
		memcpy(argCpy, arg, size);
	} else {
		CUdeviceptr p = 0;
		argCpy = new char[sizeof(CUdeviceptr)];
		memcpy(argCpy, &p, sizeof(CUdeviceptr));
	}

	cudaDeviceKernel->args[index] = argCpy;
}

void CUDADevice::SetKernelArgBuffer(HardwareDeviceKernel *kernel,
		const u_int index, const HardwareDeviceBuffer *buff) {
	assert (kernel);
	assert (!kernel->IsNull());

	if (buff) {
		const CUDADeviceBuffer *cudaDeviceBuff = dynamic_cast<const CUDADeviceBuffer *>(buff);
		assert (cudaDeviceBuff);

		SetKernelArg(kernel, index, sizeof(CUdeviceptr), &cudaDeviceBuff->cudaBuff);
	} else
		SetKernelArg(kernel, index, sizeof(CUdeviceptr), nullptr);
}

static void ConvertHardwareRange(const u_int globalSize, const u_int workGroupSize,
		u_int &block, u_int &thread) {
	if (workGroupSize == 0) {
		block = globalSize / 32;
		thread = 32;
	} else {
		block = globalSize / workGroupSize;
		thread = workGroupSize;
	}
}

static void ConvertHardwareRange(const HardwareDeviceRange &globalSize,
		const HardwareDeviceRange &workGroupSize,
		u_int &blockX, u_int &blockY, u_int &blockZ,
		u_int &threadX, u_int &threadY, u_int &threadZ) {
	if (globalSize.dimensions == 1) {
		ConvertHardwareRange(globalSize.sizes[0], workGroupSize.sizes[0], blockX, threadX);
		blockY = 1;
		threadY = 1;
		blockZ = 1;
		threadZ = 1;
	} else if (globalSize.dimensions == 2) {
		ConvertHardwareRange(globalSize.sizes[0], workGroupSize.sizes[0], blockX, threadX);
		ConvertHardwareRange(globalSize.sizes[1], workGroupSize.sizes[1], blockY, threadY);
		blockZ = 1;
		threadZ = 1;
	} else {
		ConvertHardwareRange(globalSize.sizes[0], workGroupSize.sizes[0], blockX, threadX);
		ConvertHardwareRange(globalSize.sizes[1], workGroupSize.sizes[1], blockY, threadY);
		ConvertHardwareRange(globalSize.sizes[2], workGroupSize.sizes[2], blockZ, threadZ);
	}
}

void CUDADevice::EnqueueKernel(HardwareDeviceKernel *kernel,
			const HardwareDeviceRange &globalSize,
			const HardwareDeviceRange &workGroupSize) {
	assert (kernel);
	assert (!kernel->IsNull());

	CUDADeviceKernel *cudaDeviceKernel = dynamic_cast<CUDADeviceKernel *>(kernel);
	assert (cudaDeviceKernel);

	u_int blockX, blockY, blockZ;
	u_int threadX, threadY, threadZ;
	ConvertHardwareRange(globalSize, workGroupSize,
			blockX, blockY, blockZ,
			threadX, threadY, threadZ);

	CHECK_CUDA_ERROR(cuLaunchKernel(cudaDeviceKernel->cudaKernel,
			blockX, blockY, blockZ,  // blocks
			threadX, threadY, threadZ,  // threads
			0, 0,
			&cudaDeviceKernel->args[0],
			nullptr));
}

void CUDADevice::EnqueueReadBuffer(const HardwareDeviceBuffer *buff,
		const bool blocking, const size_t size, void *ptr) {
	assert (buff);
	assert (!buff->IsNull());

	const CUDADeviceBuffer *cudaDeviceBuff = dynamic_cast<const CUDADeviceBuffer *>(buff);
	assert (cudaDeviceBuff);

	if (blocking) {
		CHECK_CUDA_ERROR(cuMemcpyDtoH(ptr, cudaDeviceBuff->cudaBuff, size));
	} else {
		CHECK_CUDA_ERROR(cuMemcpyDtoHAsync(ptr, cudaDeviceBuff->cudaBuff, size, 0));
	}
}

void CUDADevice::EnqueueWriteBuffer(const HardwareDeviceBuffer *buff,
		const bool blocking, const size_t size, const void *ptr) {
	assert (buff);
	assert (!buff->IsNull());

	const CUDADeviceBuffer *cudaDeviceBuff = dynamic_cast<const CUDADeviceBuffer *>(buff);
	assert (cudaDeviceBuff);

	if (blocking) {
		CHECK_CUDA_ERROR(cuMemcpyHtoD(cudaDeviceBuff->cudaBuff, ptr, size));
	} else {
		CHECK_CUDA_ERROR(cuMemcpyHtoDAsync(cudaDeviceBuff->cudaBuff, ptr, size, 0));
	}
}

void CUDADevice::FlushQueue() {
}

void CUDADevice::FinishQueue() {
	cuStreamSynchronize(0);
}

//------------------------------------------------------------------------------
// Memory management for hardware (aka GPU) only applications
//------------------------------------------------------------------------------

void CUDADevice::AllocBuffer(CUdeviceptr *buff,
		void *src, const size_t size, const std::string &desc) {
	// Handle the case of an empty buffer
	if (!size) {
		if (*buff) {
			// Free the buffer
			size_t cudaSize;
			CHECK_CUDA_ERROR(cuMemGetAddressRange(0, &cudaSize, *buff));
			FreeMemory(cudaSize);

			CHECK_CUDA_ERROR(cuMemFree(*buff));
		}

		*buff = 0;

		return;
	}

	if (*buff) {
		// Check the size of the already allocated buffer

		size_t cudaSize;
		CHECK_CUDA_ERROR(cuMemGetAddressRange(0, &cudaSize, *buff));

		if (size == cudaSize) {
			// I can reuse the buffer; just update the content

			//LR_LOG(deviceContext, "[Device " << GetName() << "] " << desc << " buffer updated for size: " << (size / 1024) << "Kbytes");
			if (src) {
				CHECK_CUDA_ERROR(cuMemcpyHtoDAsync(*buff, src, size, 0));
			}

			return;
		} else {
			// Free the buffer
			size_t cudaSize;
			CHECK_CUDA_ERROR(cuMemGetAddressRange(0, &cudaSize, *buff));
			FreeMemory(cudaSize);

			CHECK_CUDA_ERROR(cuMemFree(*buff));
			*buff = 0;
		}
	}

	if (desc != "")
		LR_LOG(deviceContext, "[Device " << GetName() << "] " << desc <<
				" buffer size: " << ToMemString(size));

	CHECK_CUDA_ERROR(cuMemAlloc(buff, size));
	if (src) {
		CHECK_CUDA_ERROR(cuMemcpyHtoDAsync(*buff, src, size, 0));
	}
	
	AllocMemory(size);
}

void CUDADevice::AllocBufferRO(HardwareDeviceBuffer **buff, void *src, const size_t size, const std::string &desc) {
	AllocBufferRW(buff, src, size, desc);
}

void CUDADevice::AllocBufferRW(HardwareDeviceBuffer **buff, void *src, const size_t size, const std::string &desc) {
	if (!*buff)
		*buff = new CUDADeviceBuffer();

	CUDADeviceBuffer *cudaDeviceBuff = dynamic_cast<CUDADeviceBuffer *>(*buff);
	assert (cudaDeviceBuff);

	AllocBuffer(&(cudaDeviceBuff->cudaBuff), src, size, desc);
}

void CUDADevice::FreeBuffer(HardwareDeviceBuffer **buff) {
	if (*buff && !(*buff)->IsNull()) {
		CUDADeviceBuffer *cudaDeviceBuff = dynamic_cast<CUDADeviceBuffer *>(*buff);
		assert (cudaDeviceBuff);

		size_t cudaSize;
		CHECK_CUDA_ERROR(cuMemGetAddressRange(0, &cudaSize, cudaDeviceBuff->cudaBuff));
		FreeMemory(cudaSize);

		CHECK_CUDA_ERROR(cuMemFree(cudaDeviceBuff->cudaBuff));

		delete *buff;
		*buff = nullptr;
	}
}

#endif
