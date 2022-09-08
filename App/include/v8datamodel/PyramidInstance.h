/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "reflection/reflection.h"

#ifdef _PRISM_PYRAMID_

namespace RBX {

extern const char* const sPyramid;

class PyramidInstance
	: public DescribedNonCreatable<PyramidInstance, PartInstance, sPyramid>
{
	private:
		typedef DescribedNonCreatable<PyramidInstance, PartInstance, sPyramid> Super;

	public:
		PyramidInstance();
		~PyramidInstance();
		
		enum NumSidesEnum
		{
			sides3 = 3, 
			sides4	 ,
			sides5	 ,
			sides6	 ,
			sides7	 ,
			sides8	 ,
			sides9	 ,
			sides10 ,
			sides11 ,
			sides12 ,
			sides13 ,
			sides14 ,
			sides15 ,
			sides16 ,
			sides17 ,
			sides18 ,
			sides19 ,
			sides20 ,
		};

		static const Reflection::EnumPropDescriptor<PyramidInstance, NumSidesEnum> prop_sidesXML;

		void SetNumSides(const PyramidInstance::NumSidesEnum num);
		NumSidesEnum GetNumSides(void) const;

		void SetNumSlices(const int num);
		int GetNumSlices(void) const;
	 
		/*override*/ virtual PartType getPartType() const { return PYRAMID_PART; }
		/*override*/ virtual void setPartSizeXml(const Vector3& rbxSize);

private:
	// Note - num sides, num slices - no redundant data - stored in the Poly just like size and other stuff

};

} // namespace

#endif //_PRISM_PYRAMID_