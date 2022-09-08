 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/GlueJoint.h"
#include "V8World/Primitive.h"
#include "V8World/Tolerance.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Connector.h"
#include "V8Kernel/Body.h"
#include "v8kernel/Point.h"
#include "solver/Constraint.h"

FASTFLAGVARIABLE(PGSGlueJoint, false)
DYNAMIC_FASTFLAG(OrthonormalizeJointCoords)

namespace RBX {

GlueJoint::GlueJoint()
	: MultiJoint(4)
{}

GlueJoint::GlueJoint(
	Primitive* p0,
	Primitive* p1,
	const CoordinateFrame& jointCoord0,
	const CoordinateFrame& jointCoord1,
	const Face& faceInJointSpace)
	: MultiJoint(p0, p1, jointCoord0, jointCoord1, 4)
	, faceInJointSpace(faceInJointSpace)
{}


GlueJoint* GlueJoint::canBuildJoint(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	// 1. Surfaces
	if (!compatibleSurfaces(p0, p1, nId0, nId1)) {
		return NULL;
	}

	if (!Joint::canBuildJointLoose(p0, p1, nId0, nId1)) {
		return NULL;
	}

	Face f0 = p0->getFaceInWorld(nId0);
	Face f1 = p1->getFaceInWorld(nId1);

	Face overlapOn0World = f0.projectOverlapOnMe(f1);
	Face overlapOn1World = f1.projectOverlapOnMe(f0);

	Face offsetInP0 = overlapOn0World.toObjectSpace(p0->getCoordinateFrame());
	Face offsetInP1 = overlapOn1World.toObjectSpace(p1->getCoordinateFrame());

	Face snappedOffsetInP0 = offsetInP0;
	Face snappedOffsetInP1 = offsetInP1;

	snappedOffsetInP0.snapToGrid(Tolerance::mainGrid());
	snappedOffsetInP1.snapToGrid(Tolerance::mainGrid());

	Face offset0World = snappedOffsetInP0.toWorldSpace(p0->getCoordinateFrame());
	Face offset1World = snappedOffsetInP1.toWorldSpace(p1->getCoordinateFrame());

	if (! Face::cornersAligned(offset0World, offset1World, Tolerance::jointMaxUnaligned()) ) {
		// try once more, without rounding
		offset0World = offsetInP0.toWorldSpace(p0->getCoordinateFrame());
		offset1World = offsetInP1.toWorldSpace(p1->getCoordinateFrame());

		if (! Face::cornersAligned(offset0World, offset1World, Tolerance::jointMaxUnaligned()) )
			return NULL;
	}
		
	CoordinateFrame jointCoord0 = p0->getFaceCoordInObject(nId0);
	if (DFFlag::OrthonormalizeJointCoords)
		Math::orthonormalizeIfNecessary(jointCoord0.rotation);
	CoordinateFrame jointCoord0InWorld = p0->getCoordinateFrame() * jointCoord0;
	CoordinateFrame	jc0InP1 = p1->getCoordinateFrame().toObjectSpace(jointCoord0InWorld);
	CoordinateFrame jointCoord1 = Math::snapToGrid(jc0InP1, Tolerance::mainGrid());
	if (DFFlag::OrthonormalizeJointCoords)
		Math::orthonormalizeIfNecessary(jointCoord1.rotation);

	Face faceInJointSpace = offsetInP0.toObjectSpace(jointCoord0);

	return new GlueJoint(p0, p1, jointCoord0, jointCoord1, faceInJointSpace);
}

bool GlueJoint::compatibleSurfaces(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	SurfaceType t0 = p0->getSurfaceType(nId0);
	SurfaceType t1 = p1->getSurfaceType(nId1);

	// Compatible surfaces
	bool answer = ((t0 == GLUE) || (t1 == GLUE));
	return answer;
}


float GlueJoint::getMaxForce()
{
	Vector2 overlap = faceInJointSpace.size();
	float maxKmsForce = Constants::getKmsMaxJointForce(overlap.x, overlap.y);		// max force is for one row thick...
	return Units::kmsForceToRbx(maxKmsForce);
}

void GlueJoint::putInKernel(Kernel* _kernel)
{
	Super::putInKernel(_kernel);

	Body* b0 = getPrimitive(0)->getBody();
	Body* b1 = getPrimitive(1)->getBody();

	NormalId nId0 = getNormalId(0);

	Face overlapInP0 = faceInJointSpace.toWorldSpace(jointCoord0);
	Face overlapInP1 = faceInJointSpace.toWorldSpace(jointCoord1);	// moving from Joint1 space to P1 space (object to world space)

    if( FFlag::PGSGlueJoint && _kernel->getUsingPGSSolver() )
    {
        for (int i = 0; i < 4; i++) 
        {
            ConstraintBallInSocket* ballInSocket = new ConstraintBallInSocket( b0, b1 );
            ballInSocket->setPivotA( overlapInP0[i] );
            ballInSocket->setPivotB( overlapInP1[i] );
            _kernel->pgsSolver.addConstraint(ballInSocket);
            constraints.push_back(ballInSocket);
        }
    }
    else
    {
        for (int i = 0; i < 4; i++) 
        {
            Point* point0 = getKernel()->newPointLocal(b0, overlapInP0[i]);
            Point* point1 = getKernel()->newPointLocal(b1, overlapInP1[i]);
            Connector* connector  = new NormalBreakConnector(point0, 
                point1,
                getJointK(),
                getMaxForce(), 
                nId0);
            addToMultiJoint(point0, point1, connector);
        }
    }
}

void GlueJoint::removeFromKernel()
{
    if( FFlag::PGSGlueJoint && getKernel()->getUsingPGSSolver() )
    {
        for(Constraint* c : constraints)
        {
            getKernel()->pgsSolver.removeConstraint(c);
            delete c;
        }
        constraints.clear();
    }

    Super::removeFromKernel();
}

bool GlueJoint::isBroken() const
{
    if( FFlag::PGSGlueJoint && inKernel() && getKernel()->getUsingPGSSolver() )
    {
        for(Constraint* c : constraints)
        {
            if( c->isBroken() )
            {
                return true;
            }
        }
        return false;
    }
    else
    {
        return Super::isBroken();
    }
}

void ManualGlueJoint::putInKernel(Kernel* kernel)
{
	computeIntersectingSurfacePoints();
	Super::putInKernel(kernel);
}

void ManualGlueJoint::computeIntersectingSurfacePoints(void)
{
	const Primitive* p0 = getPrimitive(0);
	const Primitive* p1 = getPrimitive(1);
	int s0 = getSurface0();
	int s1 = getSurface1();
	RBXASSERT(s0 != -1 && s1 != -1);

	// Must have valid parts and surfaces
	if(p0 && p1 && (s0 != -1) && (s1 != -1))
	{
		// Reset the joint coords
		CoordinateFrame c0 = p0->getConstGeometry()->getSurfaceCoordInBody(s0);
		CoordinateFrame worldC = p0->getCoordinateFrame() * c0;
		CoordinateFrame c1 = p1->getCoordinateFrame().toObjectSpace(worldC);
		setJointCoord(0, c0);
		setJointCoord(1, c1);

		// set the first 4 face points
		// TO DO: this must be generalized to handle n-polygon faces
		// currently the joint class that are inherited are limited to 4
		std::vector<Vector3> polygon1;
		for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
		{
			Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
			polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
		}
		
		std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

		int intersections = intersectPolyInP0.size();
		int numFacePoints = std::min(intersections, 4);
		const CoordinateFrame jCoord0 = getJointCoord(0);
		for( int i = 0; i < numFacePoints; i++ )
		{
			Vector3 facePointInC0 = jCoord0.pointToObjectSpace(intersectPolyInP0[i]);
			setFacePoint(i, facePointInC0);
		}
	}
}



} // namespace
