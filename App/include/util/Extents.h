#pragma once

#include "Util/NormalId.h"
#include "Util/G3DCore.h"
#include "rbx/Debug.h"
#include "Util/Math.h"

namespace RBX {

	class Extents {
	private:
		Vector3		low;
		Vector3		high;

	public:
		Extents()				// initialize to negativeInfiniteExtents; 
			: low(Vector3::maxFinite())
			, high(-Vector3::maxFinite())
		{}		

		Extents(const Vector3& _min, const Vector3& _max)
			: low(_min)
			, high(_max)
		{
			RBXASSERT_SLOW(low == low.min(high));
			RBXASSERT_SLOW(high == high.max(low));
		}

		bool isNanInf() const
		{
			return (Math::isNanInfVector3(low) || Math::isNanInfVector3(high));
		}

		static Extents fromCenterCorner(const Vector3& center, const Vector3& corner) {
			return Extents(center-corner, center+corner);
		}

		static Extents fromCenterRadius(const Vector3& center, float radius) {
			return fromCenterCorner(center, Vector3(radius, radius, radius));
		}

		bool operator==(const Extents& other) const {
			return ((low == other.low) && (high == other.high));
		}

		bool operator!=(const Extents& other) const {
			return !(*this == other);
		}

		static Extents vv(const Vector3& v0, const Vector3& v1) {
			Extents e;
			e.low = v0.min(v1);
			e.high = v0.max(v1);
			return e;
		}

		const Vector3& min() const					{return low;}
		const Vector3& max() const					{return high;}

		Vector3int16 getCornerIndex(int i) const;
		Vector3 getCorner(int i) const;

		Vector3 size() const {
			return high - low;
		}

		Vector3 center() const {
			return 0.5 * (low + high);
		}

		Vector3 bottomCenter() const {
			Vector3 answer = center();
			answer.y = low.y;
			return answer;
		}

		Vector3 topCenter() const {
			Vector3 answer = center();
			answer.y = high.y;
			return answer;
		}

		float longestSide() const {
			Vector3 s = size();
			return G3D::max(G3D::max(s.x, s.y), s.z);
		}

		float volume() const {
			Vector3 s = size();
			return s.x * s.y * s.z;
		}

		float areaXZ() const {
			Vector3 s = size();
			return s.x * s.z;
		}

        bool isNull() const {
            return low.x > high.x || low.y > high.y || low.z > high.z;
		}

		Extents toWorldSpace(const CoordinateFrame& offset) const;

		Extents express(const CoordinateFrame& myFrame, const CoordinateFrame& expressInFrame) const;

		// faceId's are in order of x, y, z, -x, -y, -z
		Vector3 faceCenter(NormalId faceId) const;

		/**
		Returns the four corners of a face (0 <= f < 6).
		The corners are returned to form a counter clockwise quad facing outwards.
		*/
		void getFaceCorners(
			NormalId			faceId,
			Vector3&            v0,
			Vector3&            v1,
			Vector3&            v2,
			Vector3&            v3) const;

		Plane getPlane(NormalId normalId) const;

		// clip the vector to be inside the Extents
		Vector3 clip(const Vector3& clipVector) const {
			return clipVector.clamp(low, high);
		}


		float computeClosestSqDistanceToPoint(const Vector3& point) const;

		// minimum amount to move innerExtents and keep them within Extents
		Vector3 clamp(const Extents& innerExtents) const;

		NormalId closestFace(const Vector3& point); 

		void unionWith(const Extents& other) { //		Extents& operator&= (const Extents& other) {
			low = low.min(other.low);
			high = high.max(other.high);
		}

		Extents clampInsideOf(const Extents& other) const; //		Extents& operator&= (const Extents& other) {

		void shift(const Vector3& shiftVector) {// 	Extents& operator+= (const Vector3& shiftVector) {
			low += shiftVector;
			high += shiftVector;
		}

		void scale(float x) {		//	Extents& operator*= (float x) {
			low *= x;
			high *= x;
		}

		void expand(float x) {
			low -= Vector3(x,x,x);
			high += Vector3(x,x,x);
		}

        void expand(const Vector3& p) {
            low -= p;
            high += p;
        }

		void expandToContain(const Vector3& p) {
			low = low.min(p);
			high = high.max(p);
		}

		void expandToContain(const Extents& e) {
			low = low.min(e.low);
			high = high.max(e.high);
		}

		bool contains(const Vector3& point) const {
			return (	(point.x >= low.x) 
					&&	(point.y >= low.y) 
					&&	(point.z >= low.z) 
					&&	(point.x <= high.x) 
					&&	(point.y <= high.y) 
					&&	(point.z <= high.z)	);
		}

		bool fuzzyContains(const Vector3& point, float slop) const {
			return (	(point.x >= (low.x - slop)) 
					&&	(point.y >= (low.y - slop)) 
					&&	(point.z >= (low.z - slop)) 
					&&	(point.x <= (high.x + slop)) 
					&&	(point.y <= (high.y + slop)) 
					&&	(point.z <= (high.z + slop))	);
		}


		bool overlapsOrTouches(const Extents& other) const {		// true if sides are exactly equal (touching) 
			return (	(this->low.x > other.high.x) 
					||	(this->low.y > other.high.y) 
					||	(this->low.z > other.high.z)
					||	(this->high.y < other.low.y)
					||	(this->high.x < other.low.x)
					||	(this->high.z < other.low.z)) ? false : true;
		}

		static bool overlapsOrTouches(const Extents& e0, const Extents& e1) {return e0.overlapsOrTouches(e1);}

		bool clampToOverlap(const Extents& other) {
			if	(	(this->low.x >= other.high.x) 
				||	(this->low.y >= other.high.y) 
				||	(this->low.z >= other.high.z)
				||	(this->high.y <= other.low.y)
				||	(this->high.x <= other.low.x)
				||	(this->high.z <= other.low.z))
				return false;	// not overlap
			low = low.clamp(other.low, other.high);
			high = high.clamp(other.low, other.high);
			return true;
		}

		bool separatedByMoreThan(const Extents& other, float distance) const;


		static const Extents& zero() {
			static Extents e(Vector3::zero(), Vector3::zero());
			return e;
		}

		static const Extents& unit() {
			static Extents e(Vector3(-1,-1,-1), Vector3(1,1,1));
			return e;
		}

		static const Extents& negativeMaxExtents() {
			static Extents e;			// default constructor builds this;
			return e;
		}

		static const Extents& maxExtents() {
			static Extents e(-Vector3::maxFinite(), Vector3::maxFinite());
			return e;
		}

	};
} // namespace
