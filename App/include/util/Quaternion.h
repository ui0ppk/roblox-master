/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"

namespace RBX {

class Quaternion {
public:
	float x, y, z, w;

    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	Quaternion(const G3D::Vector3& v, float _w = 0) : x((float)v.x), y((float)v.y), z((float)v.z), w(_w) {}

	Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

    Quaternion(const G3D::Matrix3& rot);

    Quaternion& operator= (const Quaternion& other);

	inline const G3D::Vector3& imag() const {
		return *(reinterpret_cast<const G3D::Vector3*>(this));
    }

	inline G3D::Vector3& imag() {
		return *(reinterpret_cast<G3D::Vector3*>(this));
    }

    void toRotationMatrix(
		Matrix3& rot) const;

	inline float dot(const Quaternion& other) const {
	    return (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
	}

	inline float magnitude() const { 
	    return x*x + y*y + z*z + w*w; 
	}

	float maxComponent() const {
		return std::max(	std::max(fabs(x), fabs(y)),  std::max(fabs(z), fabs(w)));
	}

	// Return the angle of the axis-angle representation of the quaternion
	inline float getAngle() const {
		return 2.0f * acos(w);
	}

	// Return the axis of the axis-angle representation of the quaternion
	inline Vector3 getAxis() const {
		float sinSquared = 1.f - w * w;
		if (sinSquared < 1e-6) //Check for divide by zero
			return Vector3(1.0, 0.0, 0.0); // Arbitrary
		float sinRecip = 1.f/ sqrtf(sinSquared);
		return Vector3(x * sinRecip, y * sinRecip, z * sinRecip);
	}

	inline Quaternion conjugate() const {
		return Quaternion(-x, -y, -z, w);
	}

	inline float& operator[] (int i) const {
	    return ((float*)this)[i];
	}

	inline operator float* () {
		return (float*)this;
	}

	inline operator const float* () const {
		return (float*)this;
	}

	inline Quaternion operator*(const Quaternion& other) const {
		// Following Watt & Watt, page 360
		const Vector3& v1 = imag();
		const Vector3& v2 = other.imag();
		float         s1 = w;
		float         s2 = other.w;
		return Quaternion(s1*v2 + s2*v1 + v1.cross(v2), s1*s2 - v1.dot(v2));
	}

	inline Quaternion operator+ (const Quaternion& other) const {
		return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	inline Quaternion operator- (const Quaternion& other) const {
	    return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	inline Quaternion operator* (float s) const {
		return Quaternion(s*x, s*y, s*z, s*w);
	}

	// inline
	Quaternion& operator*=(float fScalar);

	// inline
	Quaternion& operator+=(const Quaternion& rkQuaternion);

	void normalize() {
		*this *= 1.0f / sqrtf(magnitude());
	}
};

} // namespace


#include "Quaternion.inl"

