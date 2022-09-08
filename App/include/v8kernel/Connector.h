/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/NormalID.h"
#include "rbx/Debug.h"
#include "Util/Memory.h"
#include "Util/Math.h"


namespace RBX {

	class Point;
	class Body;
	class Kernel;

	class RBXBaseClass Connector
	{
		friend class KernelData;
		friend class Kernel;
	private:
		int humanoidIndex;
		int	realTimeIndex;
		int secondPassIndex;
		int	jointIndex;
		int	buoyancyIndex;
		int contactIndex;

	protected:
		// Used by kernel only. Only add types that KernelData.h can handle.
		typedef enum { 
			CONTACT,
			JOINT,
			HUMANOID,
			KERNEL_JOINT,
			BUOYANCY
		} KernelType;

		/*implement*/ virtual KernelType getConnectorKernelType() const = 0;

	public:

		int& getHumanoidIndex() {return humanoidIndex;}
		int& getRealTimeIndex() {return realTimeIndex;}
		int& getSecondPassIndex() {return secondPassIndex;}
		int& getJointIndex()	{return jointIndex;}
		int& getBuoyancyIndex()	{return buoyancyIndex;}
		int& getContactIndex()	{return contactIndex;}
		bool isHumanoid()		{return humanoidIndex >= 0;}
		bool isRealTime()		{return realTimeIndex >= 0;}
		bool isSecondPass()		{return secondPassIndex >= 0;}
		bool isJoint()			{return jointIndex >= 0;}
		bool isBuoyancy()		{return buoyancyIndex >= 0;}
		bool isContact()		{return contactIndex >= 0;}
		bool isInKernel()		{return isHumanoid() || isRealTime() || isSecondPass() || isJoint() || isBuoyancy() || isContact();}

		Connector() : humanoidIndex(-1), realTimeIndex(-1), secondPassIndex(-1),
					  jointIndex(-1), buoyancyIndex(-1), contactIndex(-1) {}
		virtual ~Connector() {}

		virtual bool computeCanThrottle();

		/////////// Called by kernel //////////////////////////////
		virtual void computeForce(bool throttling) = 0;
		virtual bool computeImpulse(float& residualVelocity) {return false;}
		virtual bool getBroken()				{return false;}

		typedef enum { body0, body1 } BodyIndex;
		virtual Body* getBody(BodyIndex id) = 0;

		// DEBUGGING
		virtual float potentialEnergy()			{return 0.0;}
	};

	class JointConnector
		: public Connector
	{
	protected:
		/*override*/ virtual KernelType getConnectorKernelType() const {return JOINT;}
	};

	//////////////////////////////////////////////////////////////////////////

	// Force = kForce * d_length
	// Torque = kTorque * d_angle
	// d_length = d_angle * L;
	// d_angle= d_length / L
	// This should produce an equivalent "force" at a length L from the center.
	// so, force at a distance of L is Force = L * torque;
	// F = kTorque * d_angle / L = kForce * d_L
	// kTorque = kForce * d_L * L * L / d_L = kForce * L * L;

	class RotateConnector 
		: public JointConnector
	{
	private:
		float	baseRotation;		// rotation when assembled

	protected:
		Body* b0;
		Body* b1;
		CoordinateFrame j0;
		CoordinateFrame j1;

		float	k;					// spring constant			
		// Integrator properties
		float	currentAngle;
		float	desiredAngle;
		float	increment;
		bool	zeroVelocity;

		float computeNormalRotation(Vector3& normal);

		float computeNormalRotationFromBase(Vector3& normal);
		float computeNormalRotationFromBaseFast(Vector3& normal);

		virtual void stepGoals();

		/*override*/ Body* getBody(BodyIndex id);

		/////////// Called by kernel //////////////////////////////
		/*override*/ virtual void computeForce(bool throttling);

	public:
		RotateConnector(
			Body*					_b0,
			Body*					_b1,
			const CoordinateFrame&	_j0,
			const CoordinateFrame&	_j1,
			float					_baseAngle,
			float					kValue,
			float					armLength);

		void reset();			// after networking receive - update to synch internal desiredRotation

		void setRotationalGoal(float rotationalGoal);

		void setVelocityGoal(float velocity);

		static float computeJointAngle(
			const CoordinateFrame& b0,
			const CoordinateFrame& b1,
			const CoordinateFrame& j0,
			const CoordinateFrame& j1,
			Vector3& normal);
	};

	//////////////////////////////////////////////////////////////////////////

	class PointToPointBreakConnector 
		: public JointConnector 
	{
	protected:
		Point*		point0;
		Point*		point1;
		float		k;		// spring constant			
		float		breakForce;

		// state variable
		bool		broken;		

		void forceToPoints(const G3D::Vector3& force);

		/*override*/ Body* getBody(BodyIndex id);

	public:
		// initialize 
		PointToPointBreakConnector(Point* point0, Point* point1, float k, float breakForce) :
			point0(point0),
			point1(point1),
			k(k),
			breakForce(breakForce),
			broken(false)
		{}
			
		/////////// Called by kernel //////////////////////////////
		/*override*/ virtual void computeForce(bool throttling);

		/*override*/ virtual bool getBroken() {	return broken;	}

		/* override */ virtual float potentialEnergy();

		///////// for breakage - one "Joint" may need to break all connectors
		inline void setBroken() {	broken = true;	}

		float getStiffness() const { return k; }
		void setStiffness( float value ) { k = value; }
	};



	//////////////////////////////////////////////////////////////////////////

	// TODO: update normal very infrequently....

	class NormalBreakConnector 
		: public PointToPointBreakConnector 
		, public Allocator<NormalBreakConnector>
	{
	private:
		NormalId	normalIdBody0;

	public:
		NormalBreakConnector(
			Point* point0,
			Point* point1,
			float k, 
			float breakForce, 
			NormalId normalIdBody0) 
			: PointToPointBreakConnector(point0, point1, k, breakForce) 
			, normalIdBody0(normalIdBody0)
		{}

		/////////// Called by kernel //////////////////////////////
		/*override*/ virtual void computeForce(bool throttling);
	};

} // namespace