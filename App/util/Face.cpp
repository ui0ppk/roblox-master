#include "stdafx.h"

#include "Util/Face.h"
#include "Util/Extents.h"
#include "rbx/Debug.h"
#include "Util/Math.h"

namespace RBX {
	
	const Vector3& Face::operator[] (int i) const {
	    return ((Vector3*)this)[i];
	}

	Vector3& Face::operator[] (int i) {
		return ((Vector3*)this)[i];
	}

	void Face::snapToGrid(float grid)
	{
		for (int i = 0; i < 4; ++i) {
			(*this)[i] = Math::toGrid((*this)[i], grid);
		}
	}

	bool Face::overlapWithinPlanes(const Face& f0, const Face& f1, float tolerance)
	{
		for (int i = 0; i < 2; ++i) {
			const Face& _f0 = (i == 0) ? f0 : f1;
			const Face& _f1 = (i == 0) ? f1 : f0;

			Plane p0(_f0.c0, _f0.c1, _f0.c3);
			Vector3 normal;
			float distance;
			p0.getEquation(normal, distance);	// norm + distance = 0;

			Face overlapOn1 = _f1.projectOverlapOnMe(_f0);

			for (int j = 0; j < 4; ++j) {
				float delta = normal.dot(overlapOn1[j]) + distance;
				if (std::abs(delta) > tolerance) {
					return false;
				}
			}
		}
		return true;
	}


	bool Face::fuzzyContainsInExtrusion(const Vector3& point, float tolerance) const
	{
		RBXASSERT(tolerance >= 0.0);

		for (int i = 0; i < 4; ++i) {
			const Vector3& from = (*this)[i];
			const Vector3& to = (*this)[(i+1) % 4];
			Vector3 toPoint = point - from;
			Vector3 edge = to - from;
			if (edge.dot(toPoint) < -tolerance) {
				return false;
			}
		}
		return true;
	}


	void Face::minMax(const Vector3& point, const Vector3& normal, float& min, float& max) const
	{
		min = FLT_MAX;
		max = -FLT_MAX;
		for (int i = 0; i < 4; ++i) {
			float thisDistance = ((*this)[i] - point).dot(normal);
			min = std::min(min, thisDistance);
			max = std::max(max, thisDistance);
		}
	}

	bool Face::hasOverlap(const Face& f0, const Face& f1, float byAtLeast)
	{
		for (int i = 0; i < 2; ++i) {
			Vector3 axis = f0.getAxis(i);
			float f0min, f0max, f1min, f1max;
			f0.minMax(f0.c0, axis, f0min, f0max);
			f1.minMax(f0.c0, axis, f1min, f1max);
			if ((f1max - byAtLeast) < f0min) {
				return false;
			}
			if ((f1min + byAtLeast) > f0max) {
				return false;
			}
		}
		return true;
	}

	bool Face::cornersAligned(const Face& f0, const Face& f1, float tolerance)
	{
		RBXASSERT(tolerance > 0.0);
		float tolSqr = tolerance * tolerance;

		for (int i = 0; i < 4; ++i) {
			bool foundOther = false;
			for (int j = 0; j < 4; ++j) {
				if ((f0[i] - f1[j]).squaredMagnitude() < tolSqr) {
					foundOther = true;
					break;
				}
			}
			if (!foundOther) {
				return false;
			}
		}
		return true;
	}


	// use f0.c0 as the base point, and it's sides as normals
	Face Face::projectOverlapOnMe(const Face& other) const
	{
		const Face& f0 = (*this);
		const Face& f1 = other;

		//RBXASSERT(Face::hasOverlap(f0, f1, 1e-6f));
		Vector3 normal[2];
		float minL[2];
		float maxL[2];

		normal[0] = f0.getU();
		normal[1] = f0.getV();
		for (int i = 0; i < 2; ++i) {
			float f0min, f0max, f1min, f1max;
			f0.minMax(f0.c0, normal[i], f0min, f0max);
			f1.minMax(f0.c0, normal[i], f1min, f1max);
			minL[i] = std::max(f0min, f1min);
			maxL[i] = std::min(f0max, f1max);
		}
		return Face(	f0.c0 + (normal[0] * minL[0] + normal[1] * minL[1]),
						f0.c0 + (normal[0] * minL[0] + normal[1] * maxL[1]),
						f0.c0 + (normal[0] * maxL[0] + normal[1] * maxL[1]),
						f0.c0 + (normal[0] * maxL[0] + normal[1] * minL[1])		);
	}

	Face Face::fromExtentsSide(const Extents& e, NormalId faceId)
	{
		Face answer;
		e.getFaceCorners(faceId, answer[0], answer[1], answer[2], answer[3]);
		return answer;
	}

	Face Face::toWorldSpace(const CoordinateFrame& objectCoord) const
	{
		Face answer;
		for (int i = 0; i < 4; ++i) {
			answer[i] = objectCoord.pointToWorldSpace((*this)[i]);
		}
		return answer;
	}

	Face Face::toObjectSpace(const CoordinateFrame& objectCoord) const
	{
		Face answer;
		for (int i = 0; i < 4; ++i) {
			answer[i] = objectCoord.pointToObjectSpace((*this)[i]);
		}
		return answer;
	}


} // namespace