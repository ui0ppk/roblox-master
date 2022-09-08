#pragma once

#include "Util/Velocity.h"

namespace RBX {

	class PV
	{
	public:
		CoordinateFrame		position;
		Velocity			velocity;

	private:
/*
		inline PV operator *(const PV& localPV) const {
			CoordinateFrame worldPos(position * localPV.position);
			Velocity otherVWorld = localPV.velocity.rotateBy(position.rotation);
			Vector3 linearVel = linearVelocityAtPoint(worldPos.translation) + otherVWorld.linear;
			Vector3 rotVel = velocity.rotational + otherVWorld.rotational;
			Velocity worldVel(linearVel, rotVel);
			return PV(worldPos, worldVel);
		}
*/

	public:

		bool operator==(const PV& other) const {
			return (position == other.position) && (velocity == other.velocity);
		}

		bool operator!=(const PV& other) const {
			return !(*this == other);
		}

		// CoordinateFrame and Velocity both initialize to identity/0
		inline PV() 
		{}

		PV(const CoordinateFrame& _position, const Velocity& _velocity) :
			position(_position), velocity(_velocity) {}

		PV(const PV &other) :
			position(other.position), velocity(other.velocity) {}

		inline ~PV() {}

		/**
		Computes the inverse of this PV.
		*/
/*
		inline PV inverse() const {
			PV out;
			out.position = position.inverse();
			out.velocity = -velocity.rotateBy(out.position.rotation);
			return out;
		}

		inline PV toObjectSpace(const PV& g) const {
			return this->inverse() * g;
		}

		inline PV toWorldSpace(const PV& localPV) const {
			return *this * localPV;
		}
*/

		// Generate Local Linear Velocities

		inline Vector3 linearVelocityAtPoint(const Vector3& worldPos) const {
			return velocity.linearVelocityAtOffset(worldPos - position.translation);
		}


		// Generate Local Velocities

		inline Velocity velocityAtPoint(const Vector3& worldPos) const {
			return velocity.velocityAtOffset(worldPos - position.translation);
		}

		inline Velocity velocityAtLocalOffset(const Vector3& localOffset) const {
			Vector3 worldPos = position.pointToWorldSpace(localOffset);
			return velocityAtPoint(worldPos);
		}

		// Generate Local PVs
		inline PV pvAtLocalOffset(const Vector3& localOffset) const {
			return pvAtLocalCoord(CoordinateFrame(localOffset));
		}

		static inline void pvAtLocalCoord(const PV& base, const CoordinateFrame& localCoord, PV& answer) {
			CoordinateFrame::mul(base.position, localCoord, answer.position);
			answer.velocity = base.velocityAtPoint(answer.position.translation);
		}

		inline PV pvAtLocalCoord(const CoordinateFrame& localCoord) const {
			PV answer;
			pvAtLocalCoord(*this, localCoord, answer);
			return answer;
		}

		inline PV lerp(const PV& other, float alpha) const {
			return PV(	position.lerp(other.position, alpha),
						velocity.lerp(other.velocity, alpha)	);
		}

	};

} // namespace RBX
