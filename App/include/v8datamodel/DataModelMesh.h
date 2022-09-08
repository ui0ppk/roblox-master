/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/G3DCore.h"

namespace RBX
{
	extern const char *const sDataModelMesh;
		
	class DataModelMesh
		: public DescribedNonCreatable<DataModelMesh, Instance, sDataModelMesh>
	{
	public:
		// Do not change these integer values, they correlate to the enum on the RBXViewNew side
		enum LODType
		{
			LOW_LOD = 0,
			MEDIUM_LOD = 1,
			HIGH_LOD = 2
		};
		
	protected:
		Vector3 scale;
		Vector3 vertColor;
		Vector3 offset;

		LODType LODx;
		LODType LODy;

	public:
		DataModelMesh();

		LODType getLevelOfDetailX() const {return LODx; }
		void setLevelOfDetailX(LODType val);

		LODType getLevelOfDetailY() const {return LODy; }
		void setLevelOfDetailY(LODType val);

		const G3D::Vector3& getScale() const {return scale;}
		void setScale(const G3D::Vector3& value);

		const G3D::Vector3 &getVertColor() const {return vertColor;}
		void setVertColor(const G3D::Vector3& value);

		float getAlpha() const;

		const G3D::Vector3& getOffset() const {return offset;}
		void setOffset(const G3D::Vector3& value);

	protected:
		bool askSetParent(const Instance* instance) const;
	};
}