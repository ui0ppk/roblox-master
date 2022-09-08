#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	class Velocity
	{
	public:
		Vector3				linear;
		Vector3				rotational;

		bool operator==(const Velocity& other) const {
			return (linear == other.linear) && (rotational == other.rotational);
		}

		bool operator!=(const Velocity& other) const {
			return !(*this == other);
		}

		inline Velocity() : 
			linear(Vector3::zero()), rotational(Vector3::zero()) {}

		Velocity(const Vector3& _linear) :
			linear(_linear), rotational(Vector3::zero()) {}

		Velocity(const Vector3& _linear, const Vector3& _rotational) :
			linear(_linear), rotational(_rotational) {}

		Velocity(const Velocity &other) :
			linear(other.linear), rotational(other.rotational) {}

		inline Velocity operator+ (const Velocity& rhs) const {
		    return Velocity(linear + rhs.linear, rotational + rhs.rotational);
		}

		inline Velocity operator- (const Velocity& rhs) const {
		    return Velocity(linear - rhs.linear, rotational - rhs.rotational);
		}

		inline Velocity operator*(float f) const {
			return Velocity(linear * f, rotational * f);
		}

		inline Velocity operator- () const {
			return Velocity(-linear, -rotational);
		}

		Velocity rotateBy(const Matrix3& m) const {
			return Velocity(m * linear, m * rotational);
		}

		static Velocity toObjectSpace(const Velocity& vWorld, const CoordinateFrame& c) {
			return vWorld.rotateBy(c.rotation.transpose());
	    }

		static Velocity toWorldSpace(const Velocity& vInObject, const CoordinateFrame& c) {
			return vInObject.rotateBy(c.rotation);
		}

		Vector3 linearVelocityAtOffset(const Vector3& offset) const {
			return linear + rotational.cross(offset);
		}

		Velocity velocityAtOffset(const Vector3& offset) const {
			return Velocity(linearVelocityAtOffset(offset), rotational);
		}

		static const Velocity& zero() {
			static Velocity v; return v;
		}

		Velocity lerp(const Velocity& other, float alpha) const {
			return Velocity(	linear.lerp(other.linear, alpha),
								rotational.lerp(other.rotational, alpha)	);
		}
	};


} // namespace RBX
