/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "reflection/reflection.h"

#ifdef _PRISM_PYRAMID_

namespace RBX {

extern const char* const sPrism;

class PrismInstance
	: public DescribedNonCreatable<PrismInstance, PartInstance, sPrism>
{
	private:
		typedef DescribedNonCreatable<PrismInstance, PartInstance, sPrism> Super;

	public:

		PrismInstance();
		~PrismInstance();

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

		static const Reflection::EnumPropDescriptor<PrismInstance, NumSidesEnum> prop_sidesXML;

		void SetNumSides(const PrismInstance::NumSidesEnum num);
		NumSidesEnum GetNumSides() const;

		void SetNumSlices(const int num);
		int GetNumSlices() const;
	 
		/*override*/ virtual PartType getPartType() const { return PRISM_PART; }
		/*override*/ virtual void setPartSizeXml(const Vector3& rbxSize);

	private:
		// These are redundant with data in the Poly

		// shape parameters
		//NumSidesEnum NumSides;
		//int NumSlices;


};

} // namespace

#endif // _PRISM_PYRAMID_