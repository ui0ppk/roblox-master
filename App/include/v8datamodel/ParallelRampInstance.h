/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "reflection/reflection.h"

#ifdef _PRISM_PYRAMID_

namespace RBX {

extern const char* const sParallelRamp;

class ParallelRampInstance
	: public DescribedNonCreatable<ParallelRampInstance, PartInstance, sParallelRamp>
{
	public:

		ParallelRampInstance();
		~ParallelRampInstance();
	 
		/*override*/ virtual PartType getPartType() const { return PARALLELRAMP_PART; }


};

} // namespace

#endif // _PRISM_PYRAMID_