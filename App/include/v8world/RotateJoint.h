/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/MultiJoint.h"
#include "Util/NormalId.h"

namespace RBX {
	
	class RotateConnector;
    class ConstraintAlign2Axes;
    class ConstraintBallInSocket;
    class ConstraintAngularVelocity;
    class Constraint;

	class RotateJoint : public MultiJoint
	{
	private:
		typedef MultiJoint Super;
		static RotateJoint* surfaceTypeToJoint(
			SurfaceType surfaceType,
			Primitive* axlePrim,
			Primitive* holePrim,
			const CoordinateFrame& c0,
			const CoordinateFrame& c1);


        void update();

	protected:
		typedef enum {AXLE_ID = 0, HOLE_ID} AxleHoleId;

		// Edge
		/*override*/ void putInKernel(Kernel* kernel);
		/*override*/ void removeFromKernel();
        /*override*/ JointType getJointType() const	{return Joint::ROTATE_JOINT;}
	
		void getPrimitivesTorqueArmLength(float& axleArmLength, float& holeArmLength);

        ConstraintAlign2Axes* align2Axes;
        ConstraintBallInSocket* ballInSocket;

	public:
		RotateJoint();

		RotateJoint(
			Primitive* axlePrim,
			Primitive* holePrim,
			const CoordinateFrame& c0,
			const CoordinateFrame& c1);

		virtual ~RotateJoint();

		static RotateJoint* canBuildJoint(
			Primitive* p0, 
			Primitive* p1, 
			NormalId nId0, 
			NormalId nId1);

		Primitive* getAxlePrim() {return getPrimitive(AXLE_ID);}
		Primitive* getHolePrim() {return getPrimitive(HOLE_ID);}

		NormalId getAxleId() {return getNormalId(AXLE_ID);}
		NormalId getHoleId() {return getNormalId(HOLE_ID);}

		Vector3 getAxleWorldDirection();

		float getAxleVelocity();
	};

	class DynamicRotateJoint : public RotateJoint
	{
	private:
		typedef RotateJoint Super;
		/*override*/ bool canStepWorld() const {return true;}
		/*override*/ bool canStepUi() const {return true;}

		/*override*/ bool stepUi(double distributedGameTime);
		/*override*/ void setPhysics();			// occurs after networking read;


       	float getChannelValue(double distributedGameTime);

	protected:
        // Edge
        /*override*/ void putInKernel(Kernel* kernel);
        /*override*/ void removeFromKernel();

        float baseAngle;						// what is the initial assembled rotation angle
		RotateConnector* rotateConnector;		// here when in kernel
		float uiValue;

	public:
		DynamicRotateJoint() : uiValue(0.0f), rotateConnector(NULL)
		{}

		DynamicRotateJoint(
			Primitive* axlePrim,
			Primitive* holePrim,
			const CoordinateFrame& c0,
			const CoordinateFrame& c1,
			float baseAngle);

		~DynamicRotateJoint();

		float getBaseAngle() const {		
			return baseAngle;
		}

        float getTorqueArmLength();

		void setBaseAngle(float value);
	};

	class RotatePJoint : public DynamicRotateJoint
	{
	private:
		// Joint
		/*override*/ JointType getJointType() const	{return Joint::ROTATE_P_JOINT;}

		/*override*/ void stepWorld();

        /*override*/ void putInKernel(Kernel* kernel);
        /*override*/ void removeFromKernel();

        float currentAngle;
        ConstraintAlign2Axes* alignmentConstraint;
	public:
		RotatePJoint(): currentAngle( 0.0f ), alignmentConstraint(NULL)
		{}

		RotatePJoint(
			Primitive* axlePrim,
			Primitive* holePrim,
			const CoordinateFrame& c0,
			const CoordinateFrame& c1,
			float baseAngle)
			: DynamicRotateJoint(axlePrim, holePrim, c0, c1, baseAngle), currentAngle( 0.0f ), alignmentConstraint(NULL)
		{}

        ~RotatePJoint();
	};

	class RotateVJoint : public DynamicRotateJoint
	{
	private:
		// Joint
		/*override*/ JointType getJointType() const	{return Joint::ROTATE_V_JOINT;}

		/*override*/ void stepWorld();

        /*override*/ void putInKernel(Kernel* kernel);
        /*override*/ void removeFromKernel();

        ConstraintAngularVelocity* angularVelocityConstraint;

	public:
		RotateVJoint(): angularVelocityConstraint( NULL )
		{}

		RotateVJoint(
			Primitive* axlePrim,
			Primitive* holePrim,
			const CoordinateFrame& c0,
			const CoordinateFrame& c1,
			float baseAngle);

        ~RotateVJoint();
	};



} // namespace
