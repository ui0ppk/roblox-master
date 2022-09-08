 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "G3D/Vector3.h"
#include "G3D/Vector3int16.h"
#include "Util/Math.h"
#include "rbx/Debug.h"
#include <limits.h>

namespace RBX {

	class Vector3int32 {
	public:
		int						x;
		int						y;
		int						z;

		Vector3int32() : x(0), y(0), z(0) {}

		Vector3int32(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}

		explicit Vector3int32(const Vector3int16& v) : x(v.x), y(v.y), z(v.z) {}

		const int& operator[] (int i) const {
			return ((int*)this)[i];
		}

		int& operator[] (int i) {
			return ((int*)this)[i];
		}

		Vector3int32 operator- () const {
			return Vector3int32(-x, -y, -z);
		}

		Vector3int32 operator+ (const Vector3int32& v) const {
			return Vector3int32(x + v.x, y + v.y, z + v.z);
		}

		Vector3int32 operator- (const Vector3int32& v) const {
			return Vector3int32(x - v.x, y - v.y, z - v.z);
		}

		Vector3int32 operator* (const int v) const {
			return Vector3int32(x * v, y * v, z * v);
		}

		Vector3int32 operator* (const Vector3int16& v) const {
			return Vector3int32(x * v.x, y * v.y, z * v.z);
		}

		Vector3int32 operator* (const Vector3int32& v) const {
			return Vector3int32(x * v.x, y * v.y, z * v.z);
		}

		Vector3int32 operator>> (const Vector3int32& v) const {
			return Vector3int32(x >> v.x, y >> v.y, z >> v.z);
		}

		Vector3int32 operator>> (const Vector3int16& v) const {
			return Vector3int32(x >> v.x, y >> v.y, z >> v.z);
		}

		Vector3int32 operator>> (unsigned int shift) const {
			RBXASSERT_SLOW(shift < 32);
			return Vector3int32(x >> shift, y >> shift, z >> shift);
		}

		Vector3int32 operator<< (const Vector3int32& v) const {
			return Vector3int32(x << v.x, y << v.y, z << v.z);
		}

		Vector3int32 operator<< (const Vector3int16& v) const {
			return Vector3int32(x << v.x, y << v.y, z << v.z);
		}

		Vector3int32 operator<< (unsigned int shift) const {
			RBXASSERT_SLOW(shift < 32);
			return Vector3int32(x << shift, y << shift, z << shift);
		}

		Vector3int32 operator& (const Vector3int32& v) const {
			return Vector3int32(x & v.x, y & v.y, z & v.z);
		}

		Vector3int32 operator% (const Vector3int32& v) const {
			return Vector3int32(x % v.x, y % v.y, z % v.z);
		}

		void shiftRight(int shift) {
			RBXASSERT_SLOW(shift >= 0);
			RBXASSERT_SLOW(shift < 32);
			x >>= shift;	y >>= shift;	z >>= shift;
		}

		bool operator==(const Vector3int32& rkVector) const {
		    return ( x == rkVector.x && y == rkVector.y && z == rkVector.z );
		}

		bool operator!=(const Vector3int32& rkVector) const {
		    return ( x != rkVector.x || y != rkVector.y || z != rkVector.z );
		}
		bool operator<(const Vector3int32& rkVector) const {
			return x < rkVector.x || (x == rkVector.x && y < rkVector.y) || (x == rkVector.x && y == rkVector.y && z < rkVector.z);
		}

		inline float squaredMagnitude () const {
			return x*x + y*y + z*z;
		}

		static Vector3int32 floor(const G3D::Vector3& v) {
			return Vector3int32(Math::iFloor(v.x), Math::iFloor(v.y), Math::iFloor(v.z));
		}

		Vector3int32 min(const Vector3int32 &v) const {
			return Vector3int32(std::min(v.x, x), std::min(v.y, y), std::min(v.z, z));
		}

		Vector3int32 max(const Vector3int32 &v) const {
			return Vector3int32(std::max(v.x, x), std::max(v.y, y), std::max(v.z, z));
		}

		G3D::Vector3 toVector3() const {
			return Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
		}

		G3D::Vector3int16 toVector3int16() const {
			return Vector3int16(
				static_cast<G3D::int16>(x),
				static_cast<G3D::int16>(y),
				static_cast<G3D::int16>(z));
		}

		int sum() const {
			return x + y + z;	
		}

		inline static const Vector3int32& zero()     { static Vector3int32 v(0, 0, 0); return v; }
		inline static const Vector3int32& one()      { static Vector3int32 v(1, 1, 1); return v; }
		inline static const Vector3int32& maxInt()     { static Vector3int32 v(INT_MAX, INT_MAX, INT_MAX); return v; }
		inline static const Vector3int32& minInt()     { static Vector3int32 v(INT_MIN, INT_MIN, INT_MIN); return v; }
	};

	std::ostream& operator<<(std::ostream& os, const Vector3int32& v);
	::std::size_t hash_value(const RBX::Vector3int32& v);

	inline RBX::Vector3int32 fastFloorInt32(const RBX::Vector3& v)
	{
		return RBX::Vector3int32(fastFloorInt(v.x), fastFloorInt(v.y), fastFloorInt(v.z));
	}

}
