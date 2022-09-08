 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/RotateJoint.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "V8World/Tolerance.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Point.h"
#include "V8Kernel/Connector.h"
#include "v8Kernel/Constants.h"
#include "util/Math.h"

#include "solver/Solver.h"

FASTFLAGVARIABLE( PGSSteppingMotorFix, false )
DYNAMIC_FASTFLAG(OrthonormalizeJointCoords)

namespace RBX {

RotateJoint::RotateJoint()
	: MultiJoint(2)
    , align2Axes( NULL )
    , ballInSocket( NULL )
{}

RotateJoint::RotateJoint(
	Primitive* axlePrim,
	Primitive* holePrim,
	const CoordinateFrame& c0,
	const CoordinateFrame& c1)
		: MultiJoint(axlePrim, holePrim, c0, c1, 2)
        , align2Axes( NULL )
        , ballInSocket( NULL )
{
}

RotateJoint::~RotateJoint()
{
    if( ballInSocket != NULL )
    {
        delete ballInSocket;
    }

    if( align2Axes != NULL )
    {
        delete align2Axes;
    }
}

void RotateJoint::update()
{
    if( align2Axes == NULL )
    {
        Body* b0 = getAxlePrim()->getBody();
        Body* b1 = getHolePrim()->getBody();
        align2Axes = new ConstraintAlign2Axes( b0, b1 );
        ballInSocket = new ConstraintBallInSocket( b0, b1 );
    }

    Vector3 axisA = jointCoord0.lookVector();
    Vector3 axisB = jointCoord1.lookVector();
    align2Axes->setAxisA( axisA );
    align2Axes->setAxisB( axisB );

    Vector3 pivot0 = jointCoord0.translation;
    Vector3 pivot1 = jointCoord1.translation;
    ballInSocket->setPivotA( pivot0 );
    ballInSocket->setPivotB( pivot1 );
}

Vector3 RotateJoint::getAxleWorldDirection()
{
	return getJointWorldCoord(0).rotation.column(2);		// axle points in Z direction
}

float RotateJoint::getAxleVelocity()
{
	Vector3 axleDirection = getAxleWorldDirection();

	Vector3 axleRotVelocity = getPrimitive(AXLE_ID)->getPV().velocity.rotational;
	Vector3 holeRotVelocity = getPrimitive(HOLE_ID)->getPV().velocity.rotational;

	return (axleRotVelocity - holeRotVelocity).dot(axleDirection);
}

RotateJoint* RotateJoint::surfaceTypeToJoint(
	SurfaceType surfaceType,
	Primitive* axlePrim,
	Primitive* holePrim,
	const CoordinateFrame& c0,
	const CoordinateFrame& c1)
{
	if (surfaceType == ROTATE)
	{
		return new RotateJoint(axlePrim, holePrim, c0, c1);
	}
	else
	{
		Vector3 temp;
		float baseAngle = RotateConnector::computeJointAngle(	axlePrim->getCoordinateFrame(),
																holePrim->getCoordinateFrame(),
																c0,
																c1,
																temp);
		if (surfaceType == ROTATE_P)
		{
			return new RotatePJoint(axlePrim, holePrim, c0, c1, baseAngle);
		}
		else if (surfaceType == ROTATE_V)
		{
			return new RotateVJoint(axlePrim, holePrim, c0, c1, baseAngle);
		}
		else
		{
			RBXASSERT(0);
			return NULL;
		}
	}
}

// helper function for canBuildJoint
static bool axleOverlapsHole(Vector3 &axlePtWorld, Vector3 &holePtWorld, Vector3 &n0, Vector3 &n1)
{
	// check the place where the axle and hole align
	if (Tolerance::pointsUnaligned(axlePtWorld, holePtWorld)) {
		return false;
	}

	for (int i = 0; i < 2; i++) {
		float polarity = (i == 0) ? 1.0f : -1.0f;
		Vector3 ref0 = axlePtWorld - (n0 * polarity);
		Vector3 ref1 = holePtWorld + (n1 * polarity);		// opposite side of the connector
		if (Tolerance::pointsUnaligned(ref0, ref1)) {
			return false;
		}
	}

	return true;
}

RotateJoint* RotateJoint::canBuildJoint(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	SurfaceType t0 = p0->getSurfaceType(nId0);
	SurfaceType t1 = p1->getSurfaceType(nId1);

	// 1. Joint Types
	if (!(IsRotate(t0) || IsRotate(t1))) {
		return NULL;
	}

	const CoordinateFrame& coord0 = p0->getCoordinateFrame();
	const CoordinateFrame& coord1 = p1->getCoordinateFrame();

	Vector3 n0 = Math::getWorldNormal(nId0, coord0);
	Vector3 n1 = Math::getWorldNormal(nId1, coord1);

	// 2. Normals must be aligned within tolerance
	if (Math::angle(n0, -n1) > Tolerance::rotateAngleMax()) {
		return NULL;
	}

	Face f0 = p0->getFaceInWorld(nId0);
	Face f1 = p1->getFaceInWorld(nId1);

	// 3. Overlap
	if (!Face::hasOverlap(f0, f1, Tolerance::jointOverlapMin())) {
		return NULL;
	}

	// 4. Coplanar
	if (!Face::overlapWithinPlanes(f0, f1, Tolerance::rotatePlanarMax())) {
		return NULL;
	}

	Vector3 center0 = f0.center();
	Vector3 center1 = f1.center();

	bool touchCenter0 = f1.fuzzyContainsInExtrusion(center0, Tolerance::jointMaxUnaligned());
	bool touchCenter1 = f0.fuzzyContainsInExtrusion(center1, Tolerance::jointMaxUnaligned());

	bool constraint0 = (IsRotate(t0) && touchCenter0);
	bool constraint1 = (IsRotate(t1) && touchCenter1);

	if (!(constraint0 || constraint1)) {
		return NULL;
	}

	if (	!constraint0 
		||	(constraint1 &&	(p1->getSize().sum() > p0->getSize().sum()))
		) 
	{
		std::swap(t0, t1);
		std::swap(p0, p1);
		std::swap(nId0, nId1);
		std::swap(center0, center1);
		std::swap(n0, n1);
	}

	CoordinateFrame axleInP0 = p0->getFaceCoordInObject(nId0);
	if (DFFlag::OrthonormalizeJointCoords)
		Math::orthonormalizeIfNecessary(axleInP0.rotation);
	Vector3 axlePtWorld = p0->getCoordinateFrame().pointToWorldSpace(axleInP0.translation);
	Vector3 axlePtInP1 = p1->getCoordinateFrame().pointToObjectSpace(axlePtWorld);
	NormalId intoP1 = normalIdOpposite(nId1);
	CoordinateFrame holeInP1(	normalIdToMatrix3(intoP1),
								Math::toGrid(axlePtInP1, Tolerance::mainGrid())
								);
	if (DFFlag::OrthonormalizeJointCoords)
		Math::orthonormalizeIfNecessary(holeInP1.rotation);
	Vector3 holePtWorld = p1->getCoordinateFrame().pointToWorldSpace(holeInP1.translation);
	
	if (! axleOverlapsHole( axlePtWorld, holePtWorld, n0, n1 ) )
	{
		// try checking without rounding
		holeInP1 = CoordinateFrame( normalIdToMatrix3(intoP1), axlePtInP1 );
		holePtWorld = p1->getCoordinateFrame().pointToWorldSpace(holeInP1.translation);
		if (! axleOverlapsHole( axlePtWorld, holePtWorld, n0, n1 ) )
			return NULL;
	}
	
	return surfaceTypeToJoint(t0, p0, p1, axleInP0, holeInP1);
}


void RotateJoint::removeFromKernel()
{
	RBXASSERT(getKernel());

	if( getKernel()->getUsingPGSSolver() )
    {
        if( align2Axes != NULL )
        {
            getKernel()->pgsSolver.removeConstraint( align2Axes );
            getKernel()->pgsSolver.removeConstraint( ballInSocket );
        }
    }

	Super::removeFromKernel();
}

void RotateJoint::getPrimitivesTorqueArmLength(float& axleArmLength, float& holeArmLength)
{
	Vector3 ownerSize = getAxlePrim()->getSize();
	Vector3 otherSize = getHolePrim()->getSize();
	NormalId nId0 = getAxleId();
	NormalId nId1 = getHoleId();

	int jOwner = (nId0 + 1) % 3;
	int kOwner = (nId0 + 2) % 3;
	int jOther = (nId1 + 1) % 3;
	int kOther = (nId1 + 2) % 3;

	axleArmLength = std::max(ownerSize[jOwner], ownerSize[kOwner]);
	holeArmLength = std::max(otherSize[jOther], otherSize[kOther]);
}


/*
	joint coordinates - the Z axis points in the direction of the axle
*/

void RotateJoint::putInKernel(Kernel* _kernel)
{
	Super::putInKernel(_kernel);

	Body* b0 = getAxlePrim()->getBody();
	Body* b1 = getHolePrim()->getBody();

	if (!getKernel()->getUsingPGSSolver())
	{
		for (int i = 0; i < 2; i++) 
		{
			float polarity = (i == 0) ? -1.0f : 1.0f;

			Vector3 local0 = jointCoord0.pointToWorldSpace(polarity * Vector3::unitZ());  // puts the z offset  into the joint coord space
			Vector3 local1 = jointCoord1.pointToWorldSpace(polarity * Vector3::unitZ());			

			Point* point0 = getKernel()->newPointLocal(b0, local0);		// axle
			Point* point1 = getKernel()->newPointLocal(b1, local1);		// hole
	  
			Connector* connector = new PointToPointBreakConnector(	point0, 
																	point1, 
																	getJointK(),
																	Math::inf() );
			addToMultiJoint(point0, point1, connector);
		}
	}
	else
    {
        if( ballInSocket != NULL && ( ballInSocket->getBodyA()->getUID() != b0->getUID() || ballInSocket->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete ballInSocket;
            delete align2Axes;
            ballInSocket = NULL;
            align2Axes = NULL;
        }
        update();
        _kernel->pgsSolver.addConstraint( ballInSocket );
        _kernel->pgsSolver.addConstraint( align2Axes );
    }
}

DynamicRotateJoint::DynamicRotateJoint(
	Primitive* axlePrim,
	Primitive* holePrim,
	const CoordinateFrame& c0,
	const CoordinateFrame& c1,
	float baseAngle)
	: RotateJoint(axlePrim, holePrim, c0, c1)
	, baseAngle(baseAngle)
	, uiValue(0.0f)
	, rotateConnector(NULL)
{}

DynamicRotateJoint::~DynamicRotateJoint()
{
	RBXASSERT(!rotateConnector);
}


void DynamicRotateJoint::setPhysics()			// occurs after networking read;
{
	if (rotateConnector) {
		rotateConnector->reset();
	}
}

void DynamicRotateJoint::putInKernel(Kernel* _kernel)
{
	Super::putInKernel(_kernel);

    if( !getKernel()->getUsingPGSSolver() )
    {
        RBXASSERT(!rotateConnector);

        rotateConnector = new RotateConnector(	getAxlePrim()->getBody(),
            getHolePrim()->getBody(), 
            jointCoord0,
            jointCoord1,
            baseAngle,
            getJointK(),
            getTorqueArmLength() );

        getKernel()->insertConnector(rotateConnector);
    }
//     else
//     {
//          Implemented in the derived classes
//     }
}


void DynamicRotateJoint::removeFromKernel()
{
	RBXASSERT(getKernel());

    if( !getKernel()->getUsingPGSSolver() )
    {
        RBXASSERT(rotateConnector);
        getKernel()->removeConnector(rotateConnector);
        delete rotateConnector;
        rotateConnector = NULL;
    }

	Super::removeFromKernel();
}

bool DynamicRotateJoint::stepUi(double distributedGameTime)
{
	uiValue = getChannelValue(distributedGameTime);
	if (uiValue != 0.0f) {
		World* world = this->findWorld();
		world->ticklePrimitive(getAxlePrim(), true);
		world->ticklePrimitive(getHolePrim(), true);
	}
	return false;
}

float DynamicRotateJoint::getTorqueArmLength()
{
	float ownerMax, otherMax;
	getPrimitivesTorqueArmLength(ownerMax, otherMax);

	// here, trying to keep everything as derived form one spring constant
	// the torqueArmPercent is a factor that gives the actual length of a torque arm
	// to use that is equivalent to a force using the same spring consant
	return std::min(ownerMax, otherMax) * 0.10f;
}




float DynamicRotateJoint::getChannelValue(double distributedGameTime) 
{
	RBXASSERT(getAxlePrim());

	NormalId normalId = getNormalId(0);		// surface of the axle Primitive
	const SurfaceData& surfaceData = getAxlePrim()->getSurfaceData(normalId);

	float paramA = surfaceData.paramA;
	float paramB = surfaceData.paramB;

	switch (surfaceData.inputType)
	{
	case LegacyController::CONSTANT_INPUT:		return paramB;
	case LegacyController::SIN_INPUT:			return paramA*sin(static_cast<float>(distributedGameTime)*paramB);
	case LegacyController::NO_INPUT:			
	default:									return 0.0;
	}
}


void DynamicRotateJoint::setBaseAngle(float value) {		// in joint space
    baseAngle = value;
}

//////////////////////////////////////////////////////////////////////

void RotatePJoint::stepWorld()
{
	World* world = this->findWorld();
	if( world &&  world->getUsingPGSSolver() )
    {
        if( alignmentConstraint != NULL )
        {
            float deltaRotation = Math::deltaRotationClose(uiValue + baseAngle, currentAngle);
            float increment = std::max( std::min( deltaRotation, 0.01f ), -0.01f );
            currentAngle += increment;

            Matrix3 rot = Matrix3::fromAxisAngle( jointCoord1.lookVector(), currentAngle );
            Vector3 axisB = rot * jointCoord1.upVector();
            alignmentConstraint->setAxisB(axisB);
        }
    }
    else
    {
        if (rotateConnector) {
            rotateConnector->setRotationalGoal(uiValue);
        }
    }
}


void RotateVJoint::stepWorld()
{
	World* world = this->findWorld();
    if( world &&  world->getUsingPGSSolver() )
    {
        if( angularVelocityConstraint != NULL )
		{
			float k = getJointK();
			float l = getTorqueArmLength();
			static const float maxForceAdjustmentFactor = 10.0f;
			static const float velocityAdjustmentFactor = -31.0f; //This factor maps desired angular velocity on legacy joints to the PGS version.
			angularVelocityConstraint->setMaxForce( ( 1.0f + maxForceAdjustmentFactor * std::abs( uiValue ) ) * k * l * l );
			angularVelocityConstraint->setDesiredAngularVelocity( uiValue * velocityAdjustmentFactor );
		}
	}
	else
	{
		if (rotateConnector) {
			rotateConnector->setVelocityGoal(uiValue);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

RotatePJoint::~RotatePJoint()
{
    if( alignmentConstraint != NULL )
    {
        delete alignmentConstraint;
    }
}

void RotatePJoint::putInKernel(Kernel* kernel)
{
    DynamicRotateJoint::putInKernel(kernel);

    if( getKernel()->getUsingPGSSolver() )
    {
        Body* b0 = getAxlePrim()->getBody();
        Body* b1 = getHolePrim()->getBody();
        if( alignmentConstraint != NULL && ( alignmentConstraint->getBodyA()->getUID() != b0->getUID() || alignmentConstraint->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete alignmentConstraint;
            alignmentConstraint = NULL;
        }

        // For caching we don't destroy this when removing from kernel
        if( alignmentConstraint == NULL )
        {
            alignmentConstraint = new ConstraintAlign2Axes( b0, b1 );
        }
        Vector3 axisA = jointCoord0.upVector();
        Vector3 axisB = jointCoord1.upVector();
        if( FFlag::PGSSteppingMotorFix )
        {
            b1->getRootSimBody()->updateIfDirty();
            b0->getRootSimBody()->updateIfDirty();
            Vector3 axisAInB = b1->getCoordinateFrame().vectorToObjectSpace( b0->getCoordinateFrame().vectorToWorldSpace( axisA ) );
            float currentAngleSin = jointCoord1.lookVector().dot( axisB.cross( axisAInB ) );
            float currentAngleCos = axisAInB.dot(axisB);
            currentAngle = atan2f(currentAngleSin, currentAngleCos);
            Matrix3 rot = Matrix3::fromAxisAngle( jointCoord1.lookVector(), currentAngle );
            axisB = rot * jointCoord1.upVector();
        }
        alignmentConstraint->setAxisA(axisA);
        alignmentConstraint->setAxisB(axisB);
        getKernel()->pgsSolver.addConstraint( alignmentConstraint );
    }
}

void RotatePJoint::removeFromKernel()
{
    if( getKernel()->getUsingPGSSolver() )
    {
		if( alignmentConstraint != NULL )
		{
			getKernel()->pgsSolver.removeConstraint( alignmentConstraint );
		}
    }

    DynamicRotateJoint::removeFromKernel();
}

RotateVJoint::RotateVJoint(
    Primitive* axlePrim,
    Primitive* holePrim,
    const CoordinateFrame& c0,
    const CoordinateFrame& c1,
    float baseAngle)
    : DynamicRotateJoint(axlePrim, holePrim, c0, c1, baseAngle), angularVelocityConstraint( NULL )
{
}

RotateVJoint::~RotateVJoint()
{
    if( angularVelocityConstraint != NULL )
    {
        delete angularVelocityConstraint;
    }
}

void RotateVJoint::putInKernel(Kernel* kernel)
{
    DynamicRotateJoint::putInKernel(kernel);

    if( getKernel()->getUsingPGSSolver() )
    {
        Body* b0 = getAxlePrim()->getBody();
        Body* b1 = getHolePrim()->getBody();

        if( angularVelocityConstraint != NULL && ( angularVelocityConstraint->getBodyA()->getUID() != b0->getUID() || angularVelocityConstraint->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete angularVelocityConstraint;
            angularVelocityConstraint = NULL;
        }

        // For caching we don't destroy this when removing from kernel
        if( angularVelocityConstraint == NULL )
        {
            angularVelocityConstraint = new ConstraintAngularVelocity( b0, b1 );
        }
        Vector3 axisA = jointCoord0.lookVector();
        Vector3 axisB = jointCoord1.lookVector();
        angularVelocityConstraint->setAxisA( axisA );
        angularVelocityConstraint->setAxisB( axisB );
        getKernel()->pgsSolver.addConstraint( angularVelocityConstraint );
    }
}

void RotateVJoint::removeFromKernel()
{
    if( getKernel()->getUsingPGSSolver() )
    {
		if( angularVelocityConstraint != NULL )
		{
			getKernel()->pgsSolver.removeConstraint( angularVelocityConstraint );
		}
    }

    DynamicRotateJoint::removeFromKernel();
}

} // namespace
