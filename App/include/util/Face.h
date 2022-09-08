#pragma once

#include "Util/G3DCore.h"
#include "Util/NormalId.h"
#include "rbx/Debug.h"

namespace RBX {

	class Extents;

	class Face
	{
	private:
		Vector3			c0, c1, c2, c3;

		Face(const Vector3& c0, const Vector3& c1, const Vector3& c2, const Vector3& c3)
			: c0(c0), c1(c1), c2(c2), c3(c3)
		{}

		Vector3 getAxis(int i) const	{
			RBXASSERT((i == 0) || (i==1));
			return i == 0 ? getU() : getV();
		}

		void minMax(const Vector3& point, const Vector3& normal, float& min, float& max) const;

		Face operator* (float fScalar) const {
			return Face(fScalar*c0, fScalar*c1, fScalar*c2, fScalar*c3);
		}

		Face operator* (const Vector3& vector3) const {
			return Face(vector3*c0, vector3*c1, vector3*c2, vector3*c3);
		}

	public:
		Face() {}

		Face(const Face& other) 
			: c0(other.c0), c1(other.c1), c2(other.c2), c3(other.c3)
		{}

		static Face fromExtentsSide(const Extents& e, NormalId faceId);

		void snapToGrid(float grid);

		Vector3& operator[] (int i);

		const Vector3& operator[] (int i) const;

		Vector3 getU() const			{return (c1 - c0).direction();}

		Vector3 getV() const			{return (c3 - c0).direction();}

		Vector3 getNormal() const		{return getU().cross(getV()).direction();}

		Vector2 size() const		{return Vector2((c1-c0).magnitude(), (c3-c0).magnitude());}

		Vector3 center() const		{return 0.5 * (c0 + c2);}

		// Create new faces
		Face toWorldSpace(const CoordinateFrame& objectCoord) const;

		Face toObjectSpace(const CoordinateFrame& objectCoord) const;

		Face projectOverlapOnMe(const Face& other) const;

		// Tests
		bool fuzzyContainsInExtrusion(const Vector3& point, float tolerance) const;

		static bool cornersAligned(const Face& f0, const Face& f1, float tolerance);

		static bool hasOverlap(const Face& f0, const Face& f1, float byAtLeast);

		static bool overlapWithinPlanes(const Face& f0, const Face& f1, float tolerance);
	};
} // namespace RBX
