/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "reflection/reflection.h"

#ifdef _PRISM_PYRAMID_

namespace RBX {

extern const char* const sCornerWedge;

class CornerWedgeInstance
	: public DescribedCreatable<CornerWedgeInstance, PartInstance, sCornerWedge>
{
	public:

		CornerWedgeInstance();
		~CornerWedgeInstance();
	 
		/*override*/ virtual PartType getPartType() const { return CORNERWEDGE_PART; }


};

} // namespace

#endif // _PRISM_PYRAMID_