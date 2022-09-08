/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/Joint.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "V8World/Tolerance.h"
#include "reflection/EnumConverter.h"
#include "V8World/PrismPoly.h"
#include "V8World/PyramidPoly.h"
#include "V8World/ParallelRampPoly.h"
#include "V8World/RightAngleRampPoly.h"
#include "V8World/CornerWedgePoly.h"
#include "V8World/WedgePoly.h"

LOGGROUP(JointLifetime)
DYNAMIC_FASTFLAGVARIABLE(OrthonormalizeJointCoords, false)

namespace RBX {
namespace Reflection{
template<>
EnumDesc<Joint::JointType>::EnumDesc()
	:EnumDescriptor("JointType")
{
	addPair(Joint::NO_JOINT, "None");
	addPair(Joint::ROTATE_JOINT, "Rotate");
	addPair(Joint::ROTATE_P_JOINT, "RotateP");
	addPair(Joint::ROTATE_V_JOINT, "RotateV");
	addPair(Joint::GLUE_JOINT, "Glue");
	addPair(Joint::WELD_JOINT, "Weld");
	addPair(Joint::SNAP_JOINT, "Snap");
}
}//namespace Reflection


//TODO: for now, orthogonalizing the incoming coords, because in multiplayer
// they may not be exact in coming over the wire.
//
// The get normal ID expects them to be ortho
Joint::Joint(Primitive* prim0,
			 Primitive* prim1,
			 const CoordinateFrame& _jointCoord0,
			 const CoordinateFrame& _jointCoord1)
: jointOwner(NULL)
, Edge(prim0, prim1)
, jointCoord0(_jointCoord0)
, jointCoord1(_jointCoord1)
{
	FASTLOG3(FLog::JointLifetime, "Joint %p created, prim0: %p, prim1: %p", this, prim0, prim1);
}

Joint::Joint()
: jointOwner(NULL)
, Edge(NULL, NULL)
{
	FASTLOG1(FLog::JointLifetime, "Joint %p created, empty primitives", this);
	// confirm CoordinateFrame default constructor
	RBXASSERT(jointCoord0 == CoordinateFrame());
	RBXASSERT(jointCoord1 == CoordinateFrame());
}

Joint::~Joint() 
{
	RBXASSERT(!jointOwner);
	FASTLOG1(FLog::JointLifetime, "Joint %p destroyed", this);
}

CoordinateFrame Joint::getJointWorldCoord(int i) 
{
	RBXASSERT(getPrimitive(i));
	Primitive* p = getPrimitive(i);
	return p ? getPrimitive(i)->getCoordinateFrame() * getJointCoord(i) : CoordinateFrame();
}

void Joint::notifyMoved()
{
	Primitive* p0 = getPrimitive(0);
	Primitive* p1 = getPrimitive(1);
	if (p0 && p0->getOwner())
		p0->getOwner()->notifyMoved();
	if (p1 && p1->getOwner())
		p1->getOwner()->notifyMoved();
}

const Joint* Joint::findConstJoint(const Primitive* p, Joint::JointType jointType)
{
	int num = p->getNumJoints();
	for (int i = 0; i < num; ++i) 
	{
		const Joint * j = p->getConstJoint(i);
		if (j->getJointType() == jointType) {
			return j;
		}
	}
	return NULL;
}

const Joint* Joint::getConstJoint(const Primitive* p, Joint::JointType jointType)
{
	const Joint* answer = findConstJoint(p, jointType);
	RBXASSERT(answer);
	return answer;
}

Joint* Joint::getJoint(Primitive* p, Joint::JointType jointType)
{
	const Joint* answer = getConstJoint(p, jointType);
	return const_cast<Joint*>(answer);
}

void Joint::setJointOwner(IJointOwner* value)
{
	RBXASSERT((value == NULL) != (jointOwner == NULL));
	jointOwner = value;
}

IJointOwner* Joint::getJointOwner() const
{
	return jointOwner;
}

void Joint::setPrimitive(int i, Primitive* p)
{
	FASTLOG3(FLog::JointLifetime, "Joint %p, setting primitive %u to %p", this, i, p);

	if (p != getPrimitive(i))
	{
		World* world = findWorld();

		if (world && getPrimitive(i)) {
			world->onJointPrimitiveNulling(this, getPrimitive(i));
		}

		Super::setPrimitive(i, p);

		if (world && p) {
			world->onJointPrimitiveSet(this, p);
		}
	}
}


void Joint::setJointCoord(int i, const CoordinateFrame& c)
{
	if (c != getJointCoord(i)) {
		if (i == 0) {
			jointCoord0 = c;
			if (DFFlag::OrthonormalizeJointCoords)
				Math::orthonormalizeIfNecessary(jointCoord0.rotation);
			
		}
		else {
			jointCoord1 = c;
			if (DFFlag::OrthonormalizeJointCoords)
				Math::orthonormalizeIfNecessary(jointCoord1.rotation);
		}
		
		if (World* world = findWorld())	{
			world->jointCoordsChanged(this);
		}
	}
}

bool Joint::canBuildJoint(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1,
	float angleMax,
	float planarMax)
{		
	if( !p0->isGeometryOrthogonal() || !p1->isGeometryOrthogonal() )
	{
		size_t face0Id = p0->getGeometry()->getFaceFromLegacyNormalId(nId0);
		size_t face1Id = p1->getGeometry()->getFaceFromLegacyNormalId(nId1);

		// A poly may not have a face corresponding to a legacy NORM value
		// If so, return false.
		if( face0Id == -1 || face1Id == -1 )
			return false;

		// Angular alignment
		CoordinateFrame face0Coord = p0->getCoordinateFrame() * p0->getGeometry()->getSurfaceCoordInBody(face0Id);
		CoordinateFrame face1Coord = p1->getCoordinateFrame() * p1->getGeometry()->getSurfaceCoordInBody(face1Id);

		if (!Math::fuzzyAxisAligned(
			face0Coord.rotation, 
			face1Coord.rotation, 
			angleMax)) {
			return false;
		}

		if( !FacesOverlapped(p0, face0Id, p1, face1Id, 0.99f) )
			return false;

		//if distance b/t faces is outside tolerance
		CoordinateFrame p0Coord = p0->getCoordinateFrame() * p0->getGeometry()->getSurfaceCoordInBody(face0Id);
		CoordinateFrame p1Coord = p1->getCoordinateFrame() * p1->getGeometry()->getSurfaceCoordInBody(face1Id);
		//Vector3 face0OriginWorld = p0Coord.translation;
		Vector3 face1OriginWorld = p1Coord.translation;
		Vector3 face1OriginInface0Coord = p0Coord.pointToObjectSpace(face1OriginWorld);

		// if the z distance (in face0 coord system) of the face1 origin is more than the distance tolerance, return false.
		// Scale tolerance up by factor of two for special shapes due to less precise face alignment.
		if( fabs(face1OriginInface0Coord.z) > 2.0 * planarMax )
			return false;
	}
	else
	{
		// Angular Alignment
		if (!Math::fuzzyAxisAligned(
				p0->getCoordinateFrame().rotation, 
				p1->getCoordinateFrame().rotation, 
				angleMax)) {
			return false;
		}

		Face f0 = p0->getFaceInWorld(nId0);
		Face f1 = p1->getFaceInWorld(nId1);

		// Overlap
		if (!Face::hasOverlap(f0, f1, Tolerance::jointOverlapMin2())) 
			return false;

		// Coplanar
		if (!Face::overlapWithinPlanes(f0, f1, planarMax)) {
			return false;
		}
	}

	return true;
}



bool Joint::canBuildJointTight(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	return canBuildJoint(p0, p1, nId0, nId1, Tolerance::jointAngleMax(), Tolerance::jointPlanarMax());
}

bool Joint::canBuildJointLoose(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	return canBuildJoint(p0, p1, nId0, nId1, Tolerance::glueAngleMax(), Tolerance::gluePlanarMax());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int getPrimitiveSize2(const Primitive* p)
{
	return const_cast<Primitive*>(p)->getSortSize();
}

unsigned int getJointSize2(const Joint* j)
{
	unsigned int answer = getPrimitiveSize2(j->getConstPrimitive(0));
	if (j->getConstPrimitive(1)) {
		answer = std::max(answer, getPrimitiveSize2(j->getConstPrimitive(1)));
	}
	return answer;
}

static int biggerJointSize2(const Joint* j0, const Joint * j1)
{
	unsigned int s0 = getJointSize2(j0);
	unsigned int s1 = getJointSize2(j1);
	if (s0 > s1) {
		return 1;
	}
	else if (s1 > s0) {
		return -1;
	}
	else {
		return 0;
	}
}

static int biggerJointGuid2(const Joint* j0, const Joint* j1)
{
	const Guid* j00 = &j0->getConstPrimitive(0)->getGuid();
	const Guid* j10 = &j1->getConstPrimitive(0)->getGuid();

	const Guid* j01 = j0->getConstPrimitive(1) ? &j0->getConstPrimitive(1)->getGuid() : NULL;
	const Guid* j11 = j1->getConstPrimitive(1) ? &j1->getConstPrimitive(1)->getGuid() : NULL;

	int answer = Guid::compare(j00, j01, j10, j11);
	RBXASSERT(answer == Guid::compare(j01, j00, j10, j11));
	RBXASSERT(answer == Guid::compare(j00, j01, j11, j10));
	RBXASSERT(answer == -Guid::compare(j11, j10, j00, j01));
	RBXASSERT(answer == -Guid::compare(j10, j11, j01, j00));
	return answer;
}


bool Joint::isHeavierThan(const SpanningEdge* other) const
{
	const Joint* const j0 = this;
	const Joint* j1 = rbx_static_cast<const Joint*>(other);

	if (j0 == j1) {
		return false;
	}
	else {
		// 1.  Sort by type - anchor == heaviest, free is lightest
		Joint::JointType jt0 = j0->getJointType();
		Joint::JointType jt1 = j1->getJointType();
		if (jt0 != jt1) {
			return (jt0 < jt1);
		}
		else {
			// 2. Sort by size - bigger size == heavier joint
			int i = biggerJointSize2(j0, j1);
			if (i > 0) {	// j0 size > j1 size
				return true;
			}
			else if (i < 0) {
				return false;
			}
			else {
				// 3. sort by guid pair - bigger guid == heavier joint
				i = biggerJointGuid2(j0, j1);
				if (i > 0) {
					return true;
				}
				else if (i < 0) {
					return false;
				}
				else {
					// 4.  Fail, sort by pointers
					return (j0 < j1);
				}
			}
		}
	}
}


SpanningNode* Joint::otherNode(SpanningNode* n)
{
	Primitive* p = rbx_static_cast<Primitive*>(n);
	return otherPrimitive(p);
}


const SpanningNode* Joint::otherConstNode(const SpanningNode* n) const
{
	const Primitive* p = rbx_static_cast<const Primitive*>(n);
	return otherConstPrimitive(p);
}


SpanningNode* Joint::getNode(int i)
{
	return getPrimitive(i);
}

const SpanningNode* Joint::getConstNode(int i) const
{
	return getConstPrimitive(i);
}

bool Joint::FacesOverlapped( const Primitive* p0, size_t face0Id, const Primitive* p1, size_t face1Id, float adjustPartTolerance )
{
	// The input "adjustPartTolerance" is used to more liberally or conservatively detect overlap.
	// It has the effect of slightly shrinking or expanding a part to more/less readily get overlap.
	// It defaults to 1.0.

	if( Joint::FaceVerticesOverlapped(p0, face0Id, p1, face1Id, adjustPartTolerance) )
		return true;

	if (Joint::FaceEdgesOverlapped(p0, face0Id, p1, face1Id, adjustPartTolerance) )
		return true;

	// no overlap found.
	return false;
}

bool Joint::FaceVerticesOverlapped( const Primitive* p0, size_t face0Id, const Primitive* p1, size_t face1Id, float adjustPartTolerance )
{
	// The input "adjustPartTolerance" is used to more liberally or conservatively detect overlap.
	// It has the effect of slightly shrinking or expanding a part to more/less readily get overlap.
	int numVertsInface0 = p0->getConstGeometry()->getNumVertsInSurface(face0Id);
	for( int i = 0; i < numVertsInface0; ++i )
	{
		Vector3 p0VertInp0 = p0->getConstGeometry()->getSurfaceVertInBody(face0Id, i) * adjustPartTolerance;
		Vector3 p0VertInWorld = p0->getCoordinateFrame().pointToWorldSpace(p0VertInp0);
		Vector3 p0VertInp1 = p1->getCoordinateFrame().pointToObjectSpace(p0VertInWorld);

		if( p1->getConstGeometry()->vertOverlapsFace(p0VertInp1, face1Id) )
			return true;
	}

	// Now test snap verts in drag face
	int numVertsInface1 = p1->getConstGeometry()->getNumVertsInSurface(face1Id);
	for( int i = 0; i < numVertsInface1; ++i )
	{
		Vector3 p1VertInp1 = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, i) * adjustPartTolerance;
		Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1VertInp1);
		Vector3 p1VertInp0 = p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld);

		if( p0->getConstGeometry()->vertOverlapsFace(p1VertInp0, face0Id) )
			return true;
	}

	return false;

}

bool Joint::FaceEdgesOverlapped( const Primitive* p0, size_t face0Id, const Primitive* p1, size_t face1Id, float adjustPartTolerance )
{
	// The input "adjustPartTolerance" is used to more liberally or conservatively detect overlap.
	// It has the effect of slightly shrinking or expanding a part to more/less readily get overlap.
	const float distanceTolerance = 1e-5;
	const float largeDistance = 1e6;
	int numVertsInface0 = p0->getConstGeometry()->getNumVertsInSurface(face0Id);
	int numVertsInface1 = p1->getConstGeometry()->getNumVertsInSurface(face1Id);

	for( int i = 0; i < numVertsInface0-1; ++i )
	{	
		// line segment from A to B
		Vector3 face0VertInp0_A = p0->getConstGeometry()->getSurfaceVertInBody(face0Id, i);
		Vector3 face0VertInWorld_A = p0->getCoordinateFrame().pointToWorldSpace(face0VertInp0_A);
		Vector3 face0VertInp1_A = p1->getCoordinateFrame().pointToObjectSpace(face0VertInWorld_A);
		Vector3 face0VertInp0_B = p0->getConstGeometry()->getSurfaceVertInBody(face0Id, i+1);
		Vector3 face0VertInWorld_B = p0->getCoordinateFrame().pointToWorldSpace(face0VertInp0_B);
		Vector3 face0VertInp1_B = p1->getCoordinateFrame().pointToObjectSpace(face0VertInWorld_B);

		for( int j = 0; j < numVertsInface1-1; ++j )
		{	
			Vector3 face1VertInp1_A = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, j);
			Vector3 face1VertInp1_B = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, j+1);
			
			float distance = largeDistance;
			bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, adjustPartTolerance);

			if( crossing && distance < distanceTolerance )
				return true;
		}

		// Check last pair of p1-face1 verts
		Vector3 face1VertInp1_A = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, numVertsInface1-1);
		Vector3 face1VertInp1_B = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, 0);

		float distance = largeDistance;
		bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, adjustPartTolerance);

		if( crossing && distance < distanceTolerance )
			return true;
	}

	// Now check last pair of p0-face0 verts with all pairs of p1-face1 verts.
	Vector3 face0VertInp0_A = p0->getConstGeometry()->getSurfaceVertInBody(face0Id, numVertsInface0-1);
	Vector3 face0VertInWorld_A = p0->getCoordinateFrame().pointToWorldSpace(face0VertInp0_A);
	Vector3 face0VertInp1_A = p1->getCoordinateFrame().pointToObjectSpace(face0VertInWorld_A);
	Vector3 face0VertInp0_B = p0->getConstGeometry()->getSurfaceVertInBody(face0Id, 0);
	Vector3 face0VertInWorld_B = p0->getCoordinateFrame().pointToWorldSpace(face0VertInp0_B);
	Vector3 face0VertInp1_B = p1->getCoordinateFrame().pointToObjectSpace(face0VertInWorld_B);

	for( int j = 0; j < numVertsInface1-1; ++j )
	{	
		Vector3 face1VertInp1_A = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, j);
		Vector3 face1VertInp1_B = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, j+1);
		
		float distance = largeDistance;
		bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, adjustPartTolerance);

		if( crossing && distance < distanceTolerance )
			return true;
	}

	// Check last pair of p1-face1 verts
	Vector3 face1VertInp1_A = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, numVertsInface1-1);
	Vector3 face1VertInp1_B = p1->getConstGeometry()->getSurfaceVertInBody(face1Id, 0);

	float distance = largeDistance;
	bool crossing = Math::lineSegmentDistanceIfCrossing(face0VertInp1_A, face0VertInp1_B, face1VertInp1_A, face1VertInp1_B, distance, adjustPartTolerance);

	if( crossing && distance < distanceTolerance )
		return true;

	// no overlap found.
	return false;
}

bool Joint::findTouchingSurfacesConvex( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// find the touching surfaces
	// for each pair of surfaces, check passing of these required conditions
	// 1. parallel
	// 2. overlapping
	// 3. within distance tolerance
	
	for( face0Id = 0; face0Id < (size_t)p0.getConstGeometry()->getNumSurfaces(); face0Id++ )
	{
		for( face1Id = 0; face1Id < (size_t)p1.getConstGeometry()->getNumSurfaces(); face1Id++ )
		{
			CoordinateFrame face0Coord = p0.getCoordinateFrame() * p0.getConstGeometry()->getSurfaceCoordInBody(face0Id);
			CoordinateFrame face1Coord = p1.getCoordinateFrame() * p1.getConstGeometry()->getSurfaceCoordInBody(face1Id);

			Vector3 p0ZInWorld = face0Coord.vectorToWorldSpace(Vector3::unitZ());
			Vector3 p0ZInP1 = face1Coord.vectorToObjectSpace(p0ZInWorld);
			if( fabs(1.0f + p0ZInP1.dot(Vector3::unitZ())) > Tolerance::jointAngleMax() )
				continue;

			if( !Joint::FacesOverlapped(&p0, face0Id, &p1, face1Id, 0.99f) )
				continue;

			//if distance b/t faces is outside tolerance
			//Vector3 face0OriginWorld = face0Coord.translation;
			Vector3 face1OriginWorld = face1Coord.translation;
			Vector3 face1OriginInface0Coord = face0Coord.pointToObjectSpace(face1OriginWorld);

			// if the z distance (in face0 coord system) of the face1 origin is more than the distance tolerance, return false.
			// Scale tolerance up by factor of two for special shapes due to less precise face alignment.
			if( fabs(face1OriginInface0Coord.z) > 2.0 * Tolerance::jointPlanarMax() )
				continue;
			
			// found a contacting pair	
			return true;
		}
	}

	// Nothing found
	return false;
}

RBX::SurfaceType Joint::getSurfaceTypeFromNormal( const Primitive& primitive, const NormalId& normalId )
{
	size_t convertedFaceId = primitive.getConstGeometry()->getFaceFromLegacyNormalId( normalId );
	return ( primitive.getSurfaceType( (NormalId)convertedFaceId ) );
}

bool Joint::compatibleForHingeAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// To Do ************** Upgrade this so we are not using NormalId for determining surface type
	SurfaceType t0 = getSurfaceTypeFromNormal(p0, (NormalId)face0Id);
	SurfaceType t1 = getSurfaceTypeFromNormal(p1, (NormalId)face1Id);
	return ((t0 == ROTATE) || (t1 == ROTATE) || (t0 == ROTATE_P) || (t1 == ROTATE_P) || (t0 == ROTATE_V) || (t1 == ROTATE_V));
}

bool Joint::compatibleForGlueAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// To Do ************** Upgrade this so we are not using NormalId for determining surface type
	SurfaceType t0 = getSurfaceTypeFromNormal(p0, (NormalId)face0Id);
	SurfaceType t1 = getSurfaceTypeFromNormal(p1, (NormalId)face1Id);
	return ((t0 == GLUE) || (t1 == GLUE));
}

bool Joint::compatibleForWeldAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// To Do ************** Upgrade this so we are not using NormalId for determining surface type
	SurfaceType t0 = getSurfaceTypeFromNormal(p0, (NormalId)face0Id);
	SurfaceType t1 = getSurfaceTypeFromNormal(p1, (NormalId)face1Id);
	return ((t0 == WELD) || (t1 == WELD));
}

bool Joint::compatibleForStudAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// To Do ************** Upgrade this so we are not using NormalId for determining surface type
	SurfaceType t0 = getSurfaceTypeFromNormal(p0, (NormalId)face0Id);
	SurfaceType t1 = getSurfaceTypeFromNormal(p1, (NormalId)face1Id);
	bool compatibleForSnap =  (	((t0 == STUDS) && (t1 == INLET))
		||	((t0 == INLET) && (t1 == STUDS))	
		||  ((t0 == UNIVERSAL) && (t1 == UNIVERSAL))
		||  ((t0 == UNIVERSAL) && (t1 == INLET || t1 == STUDS))
		||  ((t0 == INLET || t0 == STUDS) && (t1 == UNIVERSAL)));


	return compatibleForSnap;
}

bool Joint::inCompatibleForAnyJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// To Do ************** Upgrade this so we are not using NormalId for determining surface type
	SurfaceType t0 = getSurfaceTypeFromNormal(p0, (NormalId)face0Id);
	SurfaceType t1 = getSurfaceTypeFromNormal(p1, (NormalId)face1Id);
	bool inCompatible =	(	((t0 == STUDS) && (t1 != INLET)) 
		||	((t1 == STUDS) && (t0 != INLET))
		||	((t0 == UNIVERSAL) && ((t1 != STUDS) && (t1 != INLET)))
		||	((t1 == UNIVERSAL) && ((t0 != STUDS) && (t0 != INLET))));

	return inCompatible;
}

bool Joint::positionedForStudAutoJoint( const Primitive& p0, size_t& face0Id, const Primitive& p1, size_t& face1Id )
{
	// To Do  ************** For now this only checks for parallel stud/inlet/universal, but does not check on versus off grid
	CoordinateFrame p0SurfaceCoords = p0.getCoordinateFrame() * p0.getConstGeometry()->getSurfaceCoordInBody(face0Id);
	CoordinateFrame p1SurfaceCoords = p1.getCoordinateFrame() * p1.getConstGeometry()->getSurfaceCoordInBody(face1Id);

	bool aligned = Math::fuzzyAxisAligned(p0SurfaceCoords.rotation, p1SurfaceCoords.rotation, Tolerance::jointAngleMax());

	return aligned;
}


} // namespace
