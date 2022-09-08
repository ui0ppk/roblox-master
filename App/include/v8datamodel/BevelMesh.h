/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "DataModelMesh.h"

namespace RBX
{
	extern const char* const sBevelMesh;
	class BevelMesh
		: public DescribedNonCreatable<BevelMesh, DataModelMesh, sBevelMesh>
	{	
	protected :
		float bevel;
		float roundness;
		float bulge; // TODO : note, quick hack putting it in here.
	public:
		BevelMesh();

		const float getRoundness() const;
		void setRoundness(const float roundness);

		const float getBevel() const;
		void setBevel(const float bevel);

		const float getBulge() const;
		void setBulge(const float bulge);
	};
}