#pragma once

#include "Util/Vector3int32.h"
#include "Util/Extents.h"
#include "rbx/Debug.h"

namespace RBX {

	class ExtentsInt32 {
	public:
		Vector3int32	low;
		Vector3int32	high;

		ExtentsInt32()				
			: low(Vector3int32::maxInt())
			, high(Vector3int32::minInt())
		{}		

		ExtentsInt32(const Vector3int32& _min, const Vector3int32& _max)
			: low(_min)
			, high(_max)
		{
			RBXASSERT_SLOW(low == low.min(high));
			RBXASSERT_SLOW(high == high.max(low));
		}

		bool operator==(const ExtentsInt32& other) const {
			return ((low == other.low) && (high == other.high));
		}

		bool operator!=(const ExtentsInt32& other) const {
			return !(*this == other);
		}

		ExtentsInt32& operator= (const ExtentsInt32& other) {
			low = other.low;
			high = other.high;
			return *this;
		}

		ExtentsInt32 shiftRight(int shift) const {
			RBXASSERT_SLOW(shift >= 0);
			RBXASSERT_SLOW(shift <= 32);
			return ExtentsInt32(low >> shift, high >> shift);
		}

		ExtentsInt32 shiftRight(const Vector3int32& shift) const {
			return ExtentsInt32(low >> shift, high >> shift);
		}

		ExtentsInt32 shiftLeft(int shift) const {
			RBXASSERT_SLOW(shift >= 0);
			RBXASSERT_SLOW(shift <= 32);
			return ExtentsInt32(low << shift, high << shift);
		}

		ExtentsInt32 shiftLeft(const Vector3int32& shift) const {
			return ExtentsInt32(low << shift, high << shift);
		}

		static ExtentsInt32 vv(const Vector3int32& v0, const Vector3int32& v1) {
			ExtentsInt32 e;
			e.low = v0.min(v1);
			e.high = v0.max(v1);
			return e;
		}

		const Vector3int32& min() const					{return low;}
		const Vector3int32& max() const					{return high;}

		Vector3int32 getCorner(int i) const;

		Vector3int32 size() const {
			return high - low;
		}

		Vector3int32 center() const {
			return ((low + high) >> 1);
		}

		Vector3int32 bottomCenter() const {
			Vector3int32 answer = center();
			answer.y = low.y;
			return answer;
		}

		Vector3int32 topCenter() const {
			Vector3int32 answer = center();
			answer.y = high.y;
			return answer;
		}

		int longestSide() const {
			Vector3int32 s = size();
			return std::max(std::max(s.x, s.y), s.z);
		}

		int volume() const {
			Vector3int32 s = size();
			long long int answer = s.x * s.y * s.z;
			RBXASSERT(answer < INT_MAX);
			return static_cast<int>(answer);
		}

		static ExtentsInt32 unionExtents(const ExtentsInt32& a, const ExtentsInt32& b) {
			return ExtentsInt32(a.low.min(b.low), a.high.max(b.high));
		}

		void shift(const Vector3int32& shiftVector) {
			low = low + shiftVector;
			high = high + shiftVector;
		}

		void expand(int x) {
			low = low - Vector3int32(x,x,x);
			high = high + Vector3int32(x,x,x);
		}

		const Vector3int32& operator[] (int i) const {
			return ((Vector3int32*)this)[i];
		}

		Vector3int32& operator[] (int i) {
			return ((Vector3int32*)this)[i];
		}

		operator Vector3int32* () {
			return (Vector3int32*)this;
		}
		operator const Vector3int32* () const {
			return (Vector3int32*)this;
		}

		bool contains(int x, int y, int z) const {
			return (	(x >= low.x) 
					&&	(y >= low.y) 
					&&	(z >= low.z) 
					&&	(x <= high.x) 
					&&	(y <= high.y) 
					&&	(z <= high.z)	);
		}

		bool contains(const Vector3int32& point) const {
			return contains(point.x, point.y, point.z);
		}

		bool overlapsOrTouches(const ExtentsInt32& other) const {		// true if sides are exactly equal (touching) 
			return (	(this->low.x > other.high.x) 
					||	(this->low.y > other.high.y) 
					||	(this->low.z > other.high.z)
					||	(this->high.y < other.low.y)
					||	(this->high.x < other.low.x)
					||	(this->high.z < other.low.z)) ? false : true;
		}

		static bool overlapsOrTouches(const ExtentsInt32& e0, const ExtentsInt32& e1) {return e0.overlapsOrTouches(e1);}

		Extents toExtents() const {
			return Extents(low.toVector3(), high.toVector3());
		}

		static const ExtentsInt32& zero() {
			static ExtentsInt32 e(Vector3int32::zero(), Vector3int32::zero());
			return e;
		}

		static const ExtentsInt32& empty() {
			static ExtentsInt32 e;  // constructor sets negatives
			return e;
		}

	};
} // namespace
