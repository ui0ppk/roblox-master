/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "reflection/reflection.h"

#ifdef _PRISM_PYRAMID_

namespace RBX {

extern const char* const sRightAngleRamp;

class RightAngleRampInstance
	: public DescribedNonCreatable<RightAngleRampInstance, PartInstance, sRightAngleRamp>
{
	public:

		RightAngleRampInstance();
		~RightAngleRampInstance();
	 
		/*override*/ virtual PartType getPartType() const { return RIGHTANGLERAMP_PART; }


};

} // namespace

#endif // _PRISM_PYRAMID_