/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "Util/PV.h"
#include "Util/Quaternion.h"
#include "Util/Math.h"
#include "Util/Memory.h"
#include "rbx/threadsafe.h"
#include "v8kernel/Constants.h"
#include "Fastlog.h"

namespace RBX {

	class Body;
	class SimBody
		: public Allocator<SimBody>
	{
	private:
		Body*				body;
		float				dt;
		bool				dirty;
        boost::uint64_t     uid;

		PV					pv;
		Quaternion			qOrientation;		// master for simulation
		Vector3				angMomentum;		// master for simulation
		Vector3				moment;
		Vector3				momentRecip;
		Matrix3				momentRecipWorld;
		float				massRecip;
		float				constantForceY;

		// accumulators
		Vector3				force;				// at center of mass, in world coordinates
		Vector3				torque;				// in world coordinates
		Vector3				impulse;			// at center of mass, in world coordinates
		Vector3				rotationalImpulse;	// in world coordinates
		
		// Cache for impulse solver
		Vector3				impulseLast;

		int					freeFallBodyIndex;
		int					realTimeBodyIndex;
		int					jointBodyIndex;
		int					buoyancyBodyIndex;
		int					contactBodyIndex;

		int					numOfConnectors;		// how many connectors connect this SimBody (assembly)
		int					numOfHumanoidConnectors;
		int					numOfSecondPassConnectors;
		int					numOfRealTimeConnectors;
		int					numOfJointConnectors;
		int					numOfBuoyancyConnectors;
		int					numOfContactConnectors;

		// Symmetrical state detection
		bool				symmetricContact;
		bool				verticalContact;
		Vector3				penetrationTorque;		// aggregated from contact normals scaled by penetration depth
		Vector3				penetrationForce;		// aggregated from contact normals scaled by penetration depth

		inline void clearForceAccumulators() {
			force = getWorldGravityForce();
			torque = Vector3(0.0, 0.0, 0.0);
		}

		inline void clearImpulseAccumulators() {
			impulse = Vector3(0.0, 0.0, 0.0);
			rotationalImpulse = Vector3(0.0, 0.0, 0.0);
		}


		void update(); 

		// All debugging stuff;
		static float maxTorqueXX;
		static float maxForceXX;
		static float maxLinearImpulseXX;
		static float maxRotationalImpulseXX;
		static float maxDebugTorque();
		static float maxDebugForce();
		static float maxDebugLinearImpulse();
		static float maxDebugRotationalImpulse();

	public:
		SimBody(Body* body);
		~SimBody();

		Body* getBody() {return body;}
		const Body* getBodyConst() const {return body;}
		void setDt(float _dt)		{dt = _dt;}
		float getDt() const			{return dt;}
        void setUID( boost::uint64_t _uid ) { uid = _uid; }
        boost::uint64_t getUID() const { return uid; }
		inline void updateMomentRecipWorld();
		inline Vector3 computeRotationVelocityFromMomentum();
		inline Vector3 computeRotationVelocityFromMomentumFast();
		inline const Matrix3& getInverseInertiaInWorld() const {return momentRecipWorld;}

		void step();
		void stepVelocity();
		void stepPosition();
		void stepFreeFall();
		
		void applyImpulse(const Vector3& _impulse, const Vector3& worldPos);

		void clearVelocity();
		void updateAngMomentum();

        void updateFromSolver( const Vector3& newPosition, const Matrix3& newOrientation, const Vector3& newLinearVelocity, const Vector3& newAngularVelocity );

		inline void updateIfDirty() { // called before step. Assumes body cofm is clean
			if (dirty)
				update();
		}

		inline Vector3 getWorldGravityForce() const { return Vector3(0, constantForceY, 0); }
		inline void clearSymStateAndAccummulator() {
			symmetricContact = true;
			verticalContact = true;
			penetrationTorque = Vector3::zero();
			penetrationForce = Vector3::zero();
		}

		inline void makeDirty() {dirty = true;}

		bool getDirty() const {return dirty;}
	
		inline const PV& getPV() const {return pv;}

		PV getOwnerPV();

		static Vector3 computeTorqueFromOffsetForce(const Vector3& _force, const Vector3& cofm, const Vector3& forceLocationWorld) {
			Vector3 localPosWorld = forceLocationWorld - cofm;
			return localPosWorld.cross(_force);
		}

		//////////////////////////////////////////////////////////////////////////////
		//
		// Parallel physics will accumulate forces from different threads - need a mutex for each
		//
		//
		inline void accumulateForceCofm(const Vector3& _force) {
			updateIfDirty();
			force += _force;
			RBXASSERT_SLOW(force.isFinite());
			RBXASSERT_SLOW(Math::longestVector3Component(force) < maxDebugForce());
		}

		inline void accumulateForce(const Vector3& _force, const Vector3& worldPos) {
			RBXASSERT_SLOW(Math::longestVector3Component(_force) < maxDebugForce());
			updateIfDirty();
			force += _force;
			torque += computeTorqueFromOffsetForce(_force, pv.position.translation, worldPos);
			RBXASSERT_SLOW(force.isFinite());
			RBXASSERT_SLOW(torque.isFinite());
		}

		inline void accumulatePenetrationForce(const Vector3& _force, const Vector3& worldPos) {
			penetrationForce += _force;
			penetrationTorque += computeTorqueFromOffsetForce(_force, pv.position.translation, worldPos);
		}

		inline void accumulateTorque(const Vector3& _torque) {
			RBXASSERT_SLOW(Math::longestVector3Component(_torque) < maxDebugTorque());
			updateIfDirty();
			torque += _torque;
			RBXASSERT_SLOW(torque.isFinite());
		}

		inline void accumulateImpulse(const Vector3& _impulse, const Vector3& worldPos) {
			RBXASSERT_SLOW(Math::longestVector3Component(_impulse) < maxDebugLinearImpulse());
			updateIfDirty();
			impulse += _impulse;
			Vector3 localPosWorld = worldPos - pv.position.translation;
			rotationalImpulse += localPosWorld.cross(_impulse);
			RBXASSERT_SLOW(impulse.isFinite());
			RBXASSERT_SLOW(rotationalImpulse.isFinite());
		}

		inline void accumulateImpulseAtBranchCofm(const Vector3& _impulse) {
			RBXASSERT_SLOW(Math::longestVector3Component(_impulse) < maxDebugLinearImpulse());
			updateIfDirty();
			impulse += _impulse;
			RBXASSERT_SLOW(impulse.isFinite());
		}

		inline void accumulateRotationalImpulse(const Vector3& _rotationalImpulse) {
			RBXASSERT_SLOW(Math::longestVector3Component(_rotationalImpulse) < maxDebugRotationalImpulse());
			updateIfDirty();
			rotationalImpulse += _rotationalImpulse;
			RBXASSERT_SLOW(rotationalImpulse.isFinite());
		}
		// End of parallel section
		//
		//////////////////////////////////////////////////////////////////////////////////////////

		inline void resetImpulseAccumulators() {
			updateIfDirty();
			clearImpulseAccumulators();
		}

		inline void resetForceAccumulators() {
			updateIfDirty();
			clearForceAccumulators();
		}

		inline const Vector3& getForce() const		{return force;}

		inline const Vector3& getTorque() const	{return torque;}

		inline const Vector3& getImpulse() const	{return impulse;}

		inline const Vector3& getRotationallmpulse() const	{return rotationalImpulse;}

		inline const float& getMassRecip() const {return massRecip;}

		inline const Vector3& getImpulseLast() const {return impulseLast;}

		inline bool hasExternalForceOrImpulse() const {return force != getWorldGravityForce() || torque != Vector3::zero() ||
															  impulse != Vector3::zero() || rotationalImpulse != Vector3::zero();}

		inline bool updateSymmetricContactState() {
			symmetricContact = (penetrationTorque.squaredMagnitude() < Constants::impulseSolverSymStateTorqueBound());
			verticalContact = ( ( fabs(penetrationForce.x) < Constants::impulseSolverSymStateForceBound() ) &&
								( fabs(penetrationForce.z) < Constants::impulseSolverSymStateForceBound() ) );

			return symmetricContact;
		}

		inline bool isSymmetricContact() const {return symmetricContact;}
		inline bool isVerticalContact() const {return verticalContact;}
		inline void clearSymmetricContact() {symmetricContact = false;}
		inline int& getRealTimeBodyIndex()	{return realTimeBodyIndex;}
		inline int& getFreeFallBodyIndex()	{return freeFallBodyIndex;}
		inline int& getJointBodyIndex()		{return jointBodyIndex;}
		inline int& getBuoyancyBodyIndex()	{return buoyancyBodyIndex;}
		inline int& getContactBodyIndex()	{return contactBodyIndex;}

		inline bool isFreeFallBody()	const	{return freeFallBodyIndex >= 0;}
		inline bool isRealTimeBody()	const	{return realTimeBodyIndex >= 0;}
		inline bool	isJointBody()		const	{return jointBodyIndex >= 0; }
		inline bool	isBuoyancyBody()	const	{return buoyancyBodyIndex >= 0; }
		inline bool isContactBody()		const	{return contactBodyIndex >= 0;}
		inline bool isInKernel()		const	{return isFreeFallBody() || isRealTimeBody() || isJointBody() || isContactBody() || isBuoyancyBody();}
		inline bool validateBodyLists()	const	{return (freeFallBodyIndex >= 0) + (realTimeBodyIndex >= 0) +
														(jointBodyIndex >= 0) + (contactBodyIndex >= 0) <= 1;}
		inline const int& getHumanoidConnectorCount() const {return numOfHumanoidConnectors;}
		inline const int& getSecondPassConnectorCount() const {return numOfSecondPassConnectors;}
		inline const int& getRealTimeConnectorCount() const {return numOfRealTimeConnectors;}
		inline const int& getJointConnectorCount() const {return numOfJointConnectors;}
		inline const int& getBuoyancyConnectorCount() const {return numOfBuoyancyConnectors;}
		inline const int& getContactConnectorCount() const {return numOfContactConnectors;}
		inline const int& getConnectorCount() const {return numOfConnectors;}
		inline void incrementHumanoidConnectorCount() {++numOfHumanoidConnectors; ++numOfConnectors;}
		inline void decrementHumanoidConnectorCount() {--numOfHumanoidConnectors; --numOfConnectors;}
		inline void incrementSecondPassConnectorCount() {++numOfSecondPassConnectors; ++numOfConnectors;}
		inline void decrementSecondPassConnectorCount() {--numOfSecondPassConnectors; --numOfConnectors;}
		inline void incrementRealTimeConnectorCount() {++numOfRealTimeConnectors; ++numOfConnectors;}
		inline void decrementRealTimeConnectorCount() {--numOfRealTimeConnectors; --numOfConnectors;}
		inline void incrementJointConnetorCount() {++numOfJointConnectors; ++numOfConnectors;}
		inline void decrementJointConnetorCount() {--numOfJointConnectors; --numOfConnectors;}
		inline void incrementBuoyancyConnectorCount() {++numOfBuoyancyConnectors; ++numOfConnectors;}
		inline void decrementBuoyancyConnectorCount() {--numOfBuoyancyConnectors; --numOfConnectors;}
		inline void incrementContactConnectorCount() {++numOfContactConnectors; ++numOfConnectors;}
		inline void decrementContactConnectorCount() {--numOfContactConnectors; -- numOfConnectors;}
	};

} // namespace

