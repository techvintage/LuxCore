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

#ifndef _LUXRAYS_INTERSECTIONDEVICE_H
#define	_LUXRAYS_INTERSECTIONDEVICE_H

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"

namespace luxrays {

class IntersectionDevice : public Device {
public:
	const Accelerator *GetAccelerator() const { return accel; }

	//--------------------------------------------------------------------------
	// Statistics
	//--------------------------------------------------------------------------

	virtual double GetTotalRaysCount() const { return statsTotalSerialRayCount + statsTotalDataParallelRayCount; }
	virtual double GetTotalPerformance() const {
		const double statsTotalRayTime = WallClockTime() - statsStartTime;
		return (statsTotalRayTime == 0.0) ?	1.0 : ((statsTotalSerialRayCount + statsTotalDataParallelRayCount) / statsTotalRayTime);
	}
	virtual double GetSerialPerformance() const {
		const double statsTotalRayTime = WallClockTime() - statsStartTime;
		return (statsTotalRayTime == 0.0) ?	1.0 : (statsTotalSerialRayCount / statsTotalRayTime);
	}
	virtual double GetDataParallelPerformance() const {
		const double statsTotalRayTime = WallClockTime() - statsStartTime;
		return (statsTotalRayTime == 0.0) ?	1.0 : (statsTotalDataParallelRayCount / statsTotalRayTime);
	}
	virtual void ResetPerformaceStats() {
		statsStartTime = WallClockTime();
		statsTotalSerialRayCount = 0.0;
		statsTotalDataParallelRayCount = 0.0;
	}

	//--------------------------------------------------------------------------
	// Serial interface: to trace a single ray (on the CPU)
	//--------------------------------------------------------------------------

	virtual bool TraceRay(const Ray *ray, RayHit *rayHit) {
		statsTotalSerialRayCount += 1.0;
		return accel->Intersect(ray, rayHit);
	}

	friend class Context;

protected:
	IntersectionDevice(const Context *context, const DeviceType type, const size_t index);
	virtual ~IntersectionDevice();

	virtual void SetDataSet(DataSet *newDataSet);
	virtual void Start();

	DataSet *dataSet;
	const Accelerator *accel;
	mutable double statsStartTime, statsTotalSerialRayCount, statsTotalDataParallelRayCount;
};

class HardwareIntersectionDevice : public IntersectionDevice {
protected:
	HardwareIntersectionDevice(const Context *context,
		const DeviceType type, const size_t index) :
		IntersectionDevice(context, type, index) { }
	virtual ~HardwareIntersectionDevice() { }
};

}

#endif	/* _LUXRAYS_INTERSECTIONDEVICE_H */
