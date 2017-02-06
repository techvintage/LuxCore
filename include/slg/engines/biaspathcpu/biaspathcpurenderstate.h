/***************************************************************************
 * Copyright 1998-2015 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
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

#ifndef _SLG_BIASPATHCPURENDERSTATE_H
#define	_SLG_BIASPATHCPURENDERSTATE_H

#include <vector>
#include <memory>
#include <typeinfo> 
#include <boost/serialization/version.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/vector.hpp>

#include "eos/portable_oarchive.hpp"
#include "eos/portable_iarchive.hpp"

#include "slg/slg.h"
#include "slg/renderstate.h"

namespace slg {


class BiasPathCPURenderState : public RenderState {
public:
	BiasPathCPURenderState(const u_int seed, TileRepository *tileRepository);
	virtual ~BiasPathCPURenderState();

	u_int bootStrapSeed;
	TileRepository *tileRepository;

	friend class boost::serialization::access;

private:
	// Used by serialization
	BiasPathCPURenderState() { }

	template<class Archive> void serialize(Archive &ar, const u_int version);
};

}

BOOST_CLASS_VERSION(slg::BiasPathCPURenderState, 1)

BOOST_CLASS_EXPORT_KEY(slg::BiasPathCPURenderState)

#endif	/* _SLG_BIASPATHCPURENDERSTATE_H */