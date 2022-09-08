/**
  @file Vector3int16.h
  
  @maintainer Morgan McGuire, matrix@brown.edu

  @created 2003-04-07
  @edited  2003-06-24
  Copyright 2000-2004, Morgan McGuire.
  All rights reserved.
 */

#ifndef VECTOR3INT16_H
#define VECTOR3INT16_H

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/HashTrait.h"

#ifdef _MSC_VER
// Turn off "conditional expression is constant" warning; MSVC generates this
// for debug assertions in inlined methods.
#pragma warning( push )
#pragma warning (disable : 4127)
#endif


namespace G3D {

/**
 \class Vector3int16
 A Vector3 that packs its fields into uint16s.
 */
G3D_BEGIN_PACKED_CLASS(2)
class Vector3int16 {
private:
    // Hidden operators
    bool operator<(const Vector3int16&) const;
    bool operator>(const Vector3int16&) const;
    bool operator<=(const Vector3int16&) const;
    bool operator>=(const Vector3int16&) const;

public:
    G3D::int16              x;
    G3D::int16              y;
    G3D::int16              z;

    Vector3int16() : x(0), y(0), z(0) {}
    Vector3int16(G3D::int16 _x, G3D::int16 _y, G3D::int16 _z) : x(_x), y(_y), z(_z) {}
    explicit Vector3int16(const class Vector3& v);
    explicit Vector3int16(int V[3]) : x(V[0]), y(V[1]), z(V[2]) {}

    inline G3D::int16& operator[] (int i) {
        debugAssert(i <= 2);
        return ((G3D::int16*)this)[i];
    }

    inline const G3D::int16& operator[] (int i) const {
        debugAssert(i <= 2);
        return ((G3D::int16*)this)[i];
    }

    inline Vector3int16 operator+(const Vector3int16& other) const {
        return Vector3int16(x + other.x, y + other.y, z + other.z);
    }

    inline Vector3int16 operator-(const Vector3int16& other) const {
        return Vector3int16(x - other.x, y - other.y, z - other.z);
    }

    inline Vector3int16 operator*(const Vector3int16& other) const {
        return Vector3int16(x * other.x, y * other.y, z * other.z);
    }

    inline Vector3int16 operator*(const int s) const {
        return Vector3int16(int16(x * s), int16(y * s), int16(z * s));
    }

	inline Vector3int16 operator/(const Vector3int16& other) const {
		return Vector3int16(int16(x / other.x), int16(y / other.y), int16(z / other.z));
	}

	inline Vector3int16 operator/(const int s) const {
		return Vector3int16(int16(x / s), int16(y / s), int16(z / s));
	}

	inline Vector3int16 operator%(const Vector3int16& other) const {
		return Vector3int16(int16(x % other.x), int16(y % other.y), int16(z % other.z));
	}

	inline Vector3int16 operator&(const Vector3int16& other) const {
		return Vector3int16(x & other.x, y & other.y, z & other.z);
	}

	inline Vector3int16 operator>>(const unsigned int s) const {
		return Vector3int16(x >> s, y >> s, z >> s);
	}

	inline Vector3int16 operator>>(const Vector3int16& other) const {
		return Vector3int16(x >> other.x, y >> other.y, z >> other.z);
	}

	inline Vector3int16 operator<<(const unsigned int s) const {
		return Vector3int16(x << s, y << s, z << s);
	}

	inline Vector3int16 operator<<(const Vector3int16& other) const {
		return Vector3int16(x << other.x, y << other.y, z << other.z);
	}

    inline Vector3int16& operator+=(const Vector3int16& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline Vector3int16& operator-=(const Vector3int16& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline Vector3int16& operator*=(const Vector3int16& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

	inline Vector3int16& operator&=(const Vector3int16& other) {
		x &= other.x;
		y &= other.y;
		z &= other.z;
		return *this;
	}

	inline Vector3int16& operator>>=(const unsigned int s) {
		x >>= s;
		y >>= s;
		z >>= s;
		return *this;
	}

	inline Vector3int16& operator>>=(const Vector3int16& other) {
		x >>= other.x;
		y >>= other.y;
		z >>= other.z;
		return *this;
	}

	inline Vector3int16& operator<<=(const unsigned int s) {
		x <<= s;
		y <<= s;
		z <<= s;
		return *this;
	}

	inline Vector3int16& operator<<=(const Vector3int16& other) {
		x <<= other.x;
		y <<= other.y;
		z <<= other.z;
		return *this;
	}

    inline bool operator== (const Vector3int16& rkVector) const {
        return ( x == rkVector.x && y == rkVector.y && z == rkVector.z );
    }

    inline bool operator!= (const Vector3int16& rkVector) const {
        return ( x != rkVector.x || y != rkVector.y || z != rkVector.z );
    }

    Vector3int16 max(const Vector3int16& v) const {
        return Vector3int16(std::max(x, v.x), std::max(y, v.y), std::max(z, v.z));
    }

	int max() const {
		return std::max(std::max(x, y), z);
	}

    Vector3int16 min(const Vector3int16& v) const {
        return Vector3int16(std::min(x, v.x), std::min(y, v.y), std::min(z, v.z));
    }

	int min() const {
		return std::min(std::min(x, y), z);
	}

	bool isBetweenInclusive(const Vector3int16& minVal, const Vector3int16& maxVal) const {
		return minVal.x <= x && x <= maxVal.x &&
			minVal.y <= y && y <= maxVal.y &&
			minVal.z <= z && z <= maxVal.z;
	}

	int sum() const {
		return x + y + z;
	}

	int dot(const Vector3int16& v) const {
		return x*v.x + y*v.y + z *v.z;
	}

    std::string toString() const;

	static const Vector3int16& zero();
	static const Vector3int16& one();

	struct boost_compatible_hash_value {
		size_t operator()(const Vector3int16& key) const {
			return static_cast<size_t>(key.x + ((int)key.y << 5) + ((int)key.z << 10));
		}
	};

}
G3D_END_PACKED_CLASS(2)

std::ostream& operator<<(std::ostream& os, const G3D::Vector3int16& v);

}

template <> struct HashTrait<G3D::Vector3int16> {
    static size_t hashCode(const G3D::Vector3int16& key) { return static_cast<size_t>(key.x + ((int)key.y << 5) + ((int)key.z << 10)); }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
