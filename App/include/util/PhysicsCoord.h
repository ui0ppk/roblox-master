#pragma once

#include "Util/G3DCore.h"
#include "Util/Quaternion.h"

namespace RBX {

	class PhysicsCoord
	{
	public:
		Vector3				translation;
		Quaternion			rotation;

		bool operator==(const PhysicsCoord& other) const {
			return (translation == other.translation) && (rotation == other.rotation);
		}

		bool operator!=(const PhysicsCoord& other) const {
			return !(*this == other);
		}

		inline PhysicsCoord() : 
			translation(Vector3::zero()) {}

		PhysicsCoord(const CoordinateFrame& cframe) :
			translation(cframe.translation), rotation(cframe.rotation)
		{}

		PhysicsCoord(const Vector3& _translation) :
			translation(_translation) {}

		PhysicsCoord(const Vector3& _translation, const Quaternion& _rotation) :
			translation(_translation), rotation(_rotation) {}

		PhysicsCoord(const PhysicsCoord &other) :
			translation(other.translation), rotation(other.rotation) {}

		PhysicsCoord operator+ (const PhysicsCoord& rhs) const {
		    return PhysicsCoord(translation + rhs.translation, rotation + rhs.rotation);
		}

		PhysicsCoord operator- (const PhysicsCoord& rhs) const {
		    return PhysicsCoord(translation - rhs.translation, rotation - rhs.rotation);
		}

		float squaredMagnitude() const {
			return translation.squaredMagnitude() + rotation.magnitude();			// note for quaternion magnitude == squared values?....
		}

		PhysicsCoord& operator+= (const PhysicsCoord& other) {
			translation += other.translation;
			rotation += other.rotation;
			return *this;
		}

		PhysicsCoord operator*(float f) const {
			return PhysicsCoord(translation * f, rotation * f);
		}

		PhysicsCoord operator/(float f) const {
			float mul = 1.0f/f;
			return *this * mul;
		}

	};


} // namespace RBX
