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

#ifndef _LUXRAYS_NATIVETHREADIDEVICE_H
#define	_LUXRAYS_NATIVETHREADIDEVICE_H

#include "luxrays/luxrays.h"
#include "luxrays/core/intersectiondevice.h"

namespace luxrays {

//------------------------------------------------------------------------------
// Native thread devices
//------------------------------------------------------------------------------

class NativeThreadIntersectionDevice : public HardwareIntersectionDevice {
public:
	NativeThreadIntersectionDevice(const Context *context, const size_t devIndex);
	virtual ~NativeThreadIntersectionDevice();

	virtual void SetDataSet(DataSet *newDataSet);

	friend class Context;
};

}

#endif	/* _LUXRAYS_NATIVETHREADIDEVICE_H */
