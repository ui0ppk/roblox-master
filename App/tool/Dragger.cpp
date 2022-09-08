/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/Dragger.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/FastLogSettings.h"
#include "V8World/Primitive.h"
#include "V8World/ContactManager.h"
#include "V8World/Tolerance.h"
#include "Tool/DragUtilities.h"
#include "V8World/Tolerance.h"
#include "V8World/MegaClusterPoly.h"
#include "V8World/Mesh.h"
#include "V8World/Ball.h"

DYNAMIC_FASTINTVARIABLE(DraggerMaxMovePercent, 100)
DYNAMIC_FASTINTVARIABLE(DraggerMaxMoveSteps, 10000)
FASTFLAGVARIABLE(DraggerInfiniteRecursionFix, false)

namespace RBX {

Extents Dragger::computeExtentsRelative(const std::vector<Primitive*>& primitives, CoordinateFrame& relativeFrame)
{
	RBXASSERT(primitives.size() > 0);

	Extents answer = Extents::negativeMaxExtents();
	for (size_t i = 0; i < primitives.size(); ++i) 
	{
		answer.unionWith(primitives[i]->getExtentsLocal().express(primitives[i]->getCoordinateFrame(), relativeFrame));
	}
	return answer;
}


Extents Dragger::computeExtentsRelative(const G3D::Array<Primitive*>& primitives, CoordinateFrame& relativeFrame)
{
	RBXASSERT(primitives.size() > 0);

	Extents answer = Extents::negativeMaxExtents();
	for (int i = 0; i < primitives.size(); ++i) 
	{
		answer.unionWith(primitives[i]->getExtentsLocal().express(primitives[i]->getCoordinateFrame(), relativeFrame));
	}
	return answer;
}

PartInstance* Dragger::computePrimaryPart(const std::vector<Primitive*>& primitives)
{
	RBXASSERT(primitives.size() > 0);

	float maxSize = 0.0f;
	int maxIndex = 0;
	for (size_t i = 0; i < primitives.size(); ++i) 
	{
		float area = primitives[i]->getExtentsWorld().areaXZ();
		if (area > maxSize)
		{
			maxIndex = i;
		}
	}
	return PartInstance::fromPrimitive(primitives[maxIndex]);
}

PartInstance* Dragger::computePrimaryPart(const G3D::Array<Primitive*>& primitives)
{
	RBXASSERT(primitives.size() > 0);

	float maxSize = 0.0f;
	int maxIndex = 0;
	for (int i = 0; i < primitives.size(); ++i) 
	{
		float area = primitives[i]->getExtentsWorld().areaXZ();
		if (area > maxSize)
		{
			maxIndex = i;
		}
	}
	return PartInstance::fromPrimitive(primitives[maxIndex]);
}


Extents Dragger::computeExtents(const std::vector<Primitive*>& primitives)
{
	RBXASSERT(primitives.size() > 0);

	Extents answer = Extents::negativeMaxExtents();
	for (size_t i = 0; i < primitives.size(); ++i) {
		answer.unionWith(primitives[i]->getExtentsWorld());
	}
	return answer;
}

Extents Dragger::computeExtents(const G3D::Array<Primitive*>& primitives)
{
	RBXASSERT(primitives.size() > 0);

	Extents answer = Extents::negativeMaxExtents();
	for (int i = 0; i < primitives.size(); ++i) {
		answer.unionWith(primitives[i]->getExtentsWorld());
	}
	return answer;
}

bool Dragger::intersectingWorldOrOthers(	PartInstance& partInstance,
											ContactManager& contactManager,
											const float tolerance,
											const float bottomPlaneHeight )
{
	G3D::Array<Primitive*> primitives;
	primitives.append(partInstance.getPartPrimitive());
	return intersectingWorldOrOthers(primitives, contactManager, tolerance, bottomPlaneHeight);
}


bool Dragger::intersectingWorldOrOthers(const G3D::Array<Primitive*>& primitives, 
										ContactManager& contactManager,
										const float bottomPlaneHeight )
{
	return intersectingWorldOrOthers(primitives, contactManager, Tolerance::maxOverlapOrGap(), bottomPlaneHeight);
}

bool Dragger::intersectingGroundPlane(const G3D::Array<Primitive*>& primitives, 
									  float yHeight)
{
	for (int i = 0; i < primitives.size(); ++i) {
		Primitive* p = primitives[i];
		if (	(p->getFastFuzzyExtents().min().y < yHeight)
			&&	(p->getConstGeometry()->collidesWithGroundPlane(p->getCoordinateFrame(), yHeight))
			)
		{
			return true;
		}
	}
	return false;
}


bool Dragger::intersectingWorldOrOthers(const G3D::Array<Primitive*>& primitives, 
										ContactManager& contactManager,
										const float tolerance,
										const float bottomPlaneHeight )
{
	RBXASSERT(primitives.size() > 0);

	return (	intersectingGroundPlane(primitives, bottomPlaneHeight)
			||	contactManager.intersectingOthers(primitives, tolerance)
			);

}




bool Dragger::movePrimitivesGoal(	const G3D::Array<Primitive*>& primitives,
									const Vector3& goal, 
									Vector3& movedSoFar,
                                    bool snapToWorld)
{
	Vector3 delta = goal - movedSoFar;
    
    if (snapToWorld)
    {
        delta = DragUtilities::toGrid(delta);
    }
    else
    {
        delta = DragUtilities::toLocalGrid(delta);
    }
    
	if (delta != Vector3::zero()) {
		movePrimitives(primitives, delta, snapToWorld);
		movedSoFar = goal;
        return true;
	}
    
    return false;
}


void Dragger::movePrimitivesDelta(	const G3D::Array<Primitive*>& primitives, 
									const Vector3& delta, 
									Vector3& movedSoFar)
{
	//RBXASSERT(delta == DragUtilities::toGrid(delta));
	RBXASSERT(delta != Vector3::zero());

	movePrimitives(primitives, delta);
	movedSoFar = DragUtilities::toGrid(movedSoFar + delta);
	//movedSoFar = movedSoFar + delta;
}


void Dragger::movePrimitives(const G3D::Array<Primitive*>& primitives, 
							 const Vector3& delta,
                             bool snapToWorld)
{
	RBXASSERT(primitives.size() > 0);

	if (delta != Vector3::zero()) {
		for (int i = 0; i < primitives.size(); ++i) {
			Primitive* p = primitives[i];
			PartInstance* part = PartInstance::fromPrimitive(p);
			DragUtilities::moveByDelta(part, delta, snapToWorld);
		}
	}
}

void Dragger::searchFine(const G3D::Array<Primitive*> primitives,
							Vector3& movedSoFar,
							ContactManager& contactManager,
							const float bottomPlaneHeight,
							Vector3 insidePosition,
							Vector3 outsidePosition)
{
	RBXASSERT(primitives.size() > 0);

	Vector3 currentPosition(0, 0, 0);
		
	for (int i = 0; i < DFInt::DraggerMaxMoveSteps; ++i)
	{
		Vector3 newPosition((insidePosition + outsidePosition) * 0.5f);

		if (Math::fuzzyEq(newPosition, currentPosition))
			break;

		movePrimitivesDelta(primitives, newPosition - currentPosition, movedSoFar);

		if (Math::isNanInfVector3(movedSoFar))
		{
			RBXASSERT(0);
			break;
		}

		currentPosition = newPosition;

		if (intersectingWorldOrOthers(primitives, contactManager, 0.0f, bottomPlaneHeight))
		{
			insidePosition = currentPosition;
		}
		else
		{
			outsidePosition = currentPosition;
		}
	}

	if (outsidePosition - currentPosition != Vector3::zero())
		movePrimitivesDelta(primitives, outsidePosition - currentPosition, movedSoFar);
}

/*
0-100	move by 1	100
-200 move by 2	50
-400 move by 4	50
-800 move by 8	50
-1600 move by 16
-3200 move by 32
-6400 move by 64
-12800 move by 128
-25600 stop by 256
*/

const float maxMove() {return (16000.0f/DFInt::DraggerMaxMovePercent)*100;}
/*
float findSafeDelta(float movedY)
{
	float answer = 1.2f;

	while ((movedY > 100.0f) && (answer < (maxMove() * 0.01f))) 
	{
		movedY *= 0.5f;
		answer *= 2.0f;
	}
	return answer;
}
*/


// Note - recursive
void Dragger::searchUpGross(	const G3D::Array<Primitive*>& primitives, 
								Vector3& movedSoFar,
								ContactManager& contactManager,
								const float bottomPlaneHeight)
{
	RBXASSERT(primitives.size() > 0);

	for (int i = 0; i < DFInt::DraggerMaxMoveSteps; ++i)
	{
		if (fabs(movedSoFar.y) > maxMove()) {
			RBXASSERT_FISHING(0);
			return;
		}

		//	float delta = findSafeDelta(movedSoFar.y);

		movePrimitivesDelta(primitives, Vector3(0.0f, 1.2f, 0.0f), movedSoFar);

		// 3/1/08 - hack to try to prevent infinite looping - never got a good stack
		if (Math::isNanInfVector3(movedSoFar)) {
			RBXASSERT(0);
			return;
		}

		if (!intersectingWorldOrOthers(primitives, contactManager, Tolerance::maxOverlapOrGap(), bottomPlaneHeight))
		{
			searchFine(primitives, movedSoFar, contactManager, bottomPlaneHeight, Vector3(0, -1.2f, 0), Vector3(0, 0, 0));
			return;
		}
	}

	
}


// Recursive
void Dragger::searchDownGross(const G3D::Array<Primitive*>& primitives, 
								Vector3& movedSoFar,
								ContactManager& contactManager,
								const float bottomPlaneHeight)
{
	RBXASSERT(primitives.size() > 0);
	RBXASSERT(!intersectingWorldOrOthers(primitives, contactManager, Tolerance::maxOverlapOrGap(), bottomPlaneHeight));

	for (int i = 0; i < DFInt::DraggerMaxMoveSteps; ++i)
	{
		movePrimitivesDelta(primitives, Vector3(0.0f,-1.2f, 0.0f), movedSoFar);

		// 3/1/08 - hack to try to prevent infinite looping - never got a good stack
		if (Math::isNanInfVector3(movedSoFar)) {
			RBXASSERT(0);
			return;
		}

		if (intersectingWorldOrOthers(primitives, contactManager, bottomPlaneHeight))
		{
			searchFine(primitives, movedSoFar, contactManager, bottomPlaneHeight, Vector3(0, 0, 0), Vector3(0, 1.2f, 0));
			return;
		}
	}
}


// 1.	Try moving to the end
// 2.	If this fails, split the difference

void Dragger::safePlaceAlongLine( const G3D::Array<Primitive*>& primitives, 
								  const Vector3& startMove, 
								  const Vector3& endMove,
								  Vector3& movedSoFar,
								  ContactManager& contactManager,
                                  bool snapToWorld)
{
	RBXASSERT(startMove != endMove);

	movePrimitivesGoal(primitives, endMove, movedSoFar, snapToWorld);

	static const float overlapTolerance = 0.001f;

	if (!intersectingWorldOrOthers(primitives, contactManager, overlapTolerance, maxDragDepth())) {		
		return;														// We're done - moved to the end - no intersection
	}
	else {
		// Moved to the end, intersection - must split
		Vector3 middleMove = endMove - startMove;	// will bias towards endMove if
        middleMove = middleMove * 0.51f;
        middleMove = startMove + middleMove;
        
        if (snapToWorld)
        {
            middleMove = DragUtilities::toGrid(middleMove);
        }
        else
        {
            middleMove =  DragUtilities::toLocalGrid(middleMove);
        }
        
		RBXASSERT(middleMove != startMove);

		movePrimitivesGoal(primitives, middleMove, movedSoFar, snapToWorld);

		bool intersecting = intersectingWorldOrOthers(primitives, contactManager, overlapTolerance, maxDragDepth());

		if ((middleMove - endMove).length() < 0.001f || (FFlag::DraggerInfiniteRecursionFix && (middleMove - startMove).length() < 0.001f)) {		// done - split down to a gap of 1 division
			if (intersecting) {
				movePrimitivesGoal(primitives, startMove, movedSoFar, snapToWorld);
			}
			else {
				return;
			}
		}
		else {
			if (intersecting) {
				safePlaceAlongLine(primitives, startMove, middleMove, movedSoFar, contactManager, snapToWorld);
			}
			else {
				safePlaceAlongLine(primitives, middleMove, endMove, movedSoFar, contactManager, snapToWorld);
			}
		}
	}
}



//////////////////////////////////////////////////////////////////////
//
// SafeMoveAlongLine: 
//

Vector3 Dragger::safeMoveAlongLine(	const G3D::Array<Primitive*>& primitives,
									const G3D::Vector3& tryMove,
									ContactManager& contactManager,
									const float customPlaneHeight,
                                    bool snapToWorld )
{
	if (primitives.size() == 0) {
		RBXASSERT(0);			// just wanted to see where this happens
		return Vector3::zero();
	}

	Vector3 movedSoFar = Vector3::zero();

	if (tryMove != Vector3::zero()) {
		safePlaceAlongLine(	primitives, 
							Vector3::zero(), 
							tryMove, 
							movedSoFar,
							contactManager,
                            snapToWorld);

		RBXASSERT(		(movedSoFar == Vector3::zero())
					||	!intersectingWorldOrOthers(primitives, contactManager, maxDragDepth())
					);
	}

	return movedSoFar;
}


//////////////////////////////////////////////////////////////////////
//
// SafeMoveYDrop:  If floating, move down until on something or 0 plane.
//				   If intersecting, move up

Vector3 Dragger::safeMoveYDrop(	const G3D::Array<Primitive*>& primitives,
								const G3D::Vector3& tryMove,
								ContactManager& contactManager,
								const float customPlaneHeight )
{
	if (primitives.size() == 0) {
		RBXASSERT(0);			// just wanted to see where this happens
		return Vector3::zero();
	}

	// if number of primitives are greater than 200 then only do optimized dragging
	if (primitives.size() > RBX::ClientAppSettings::singleton().GetValueMinPartsForOptDragging())
		return safeMoveYDrop_EXT(primitives, tryMove, contactManager, customPlaneHeight);

	Vector3 moved = Math::toGrid(tryMove, Tolerance::mainGrid());

	movePrimitives(primitives, moved);

	if (intersectingWorldOrOthers(primitives, contactManager, customPlaneHeight)) {
		searchUpGross(primitives, moved, contactManager, customPlaneHeight);
	}
	else {
		searchDownGross(primitives, moved, contactManager, customPlaneHeight);
	}
	
	return moved;
}


//////////////////////////////////////////////////////////////////////
//
// SafeMove:  Move up (if necessary) until not overlapping
Vector3 Dragger::safeMoveNoDrop(const G3D::Array<Primitive*>& primitives,
								const Vector3& tryMove,
								ContactManager& contactManager)
{
	Vector3 movedSoFar;
	
	if (primitives.size() == 0) {
		RBXASSERT(0);			// just wanted to see where this happens
		return Vector3::zero();
	}

	if (tryMove != Vector3::zero()) {
		movePrimitives(primitives, tryMove);
		movedSoFar += tryMove;
	}

	if (intersectingWorldOrOthers(primitives, contactManager, maxDragDepth())) {
		searchUpGross(primitives, movedSoFar, contactManager, maxDragDepth());
	}

	return movedSoFar;
}



//////////////////////////////////////////////////////////////////////
//

void Dragger::safeRotate(	const G3D::Array<Primitive*>& primitives,
							const Matrix3& rotate,
							ContactManager& contactManager)
{
	RBXASSERT(primitives.size());

	PartInstance* primaryPart = computePrimaryPart(primitives);
	CoordinateFrame primaryPartLocation = primaryPart->getCoordinateFrame();
	const Extents relativeExtents = computeExtentsRelative(primitives, primaryPartLocation);
	Vector3 center = primaryPartLocation.pointToWorldSpace(relativeExtents.center());

	const CoordinateFrame centerCoord(center);
	const CoordinateFrame rotationCoord(rotate,Vector3::zero());

	for (int i = 0; i < primitives.size(); ++i) 
    {
        Primitive* p = primitives[i];

        CoordinateFrame primInCenter = centerCoord.toObjectSpace(p->getCoordinateFrame());
		CoordinateFrame newPrimInCenter = rotationCoord * primInCenter;
		CoordinateFrame primInWorld = centerCoord * newPrimInCenter;
		
		primInWorld.rotation.orthonormalize();

        PartInstance* part = PartInstance::fromPrimitive(p);
        part->setCoordinateFrame(primInWorld);
	}

    // detect intersection and move to safe place
//	safeMoveNoDrop(primitives, Vector3::zero(), contactManager);
}

//////////////////////////////////////////////////////////////////////
//

void Dragger::safeRotate2(	const G3D::Array<Primitive*>& primitives,
							const Matrix3& rotate,
							ContactManager& contactManager)
{
	safeRotate(primitives, rotate, contactManager);
}

Vector3 Dragger::safeMoveYDrop_EXT(  const G3D::Array<Primitive*>& primitives,
											const Vector3& tryMove,
											ContactManager& contactManager,
											const float customPlaneHeight)
{
	if ((primitives.size() == 0) || (tryMove == Vector3::zero()))
		return Vector3::zero();

	Vector3 moved = Math::toGrid(tryMove, Tolerance::mainGrid());

	// populate data
	RBX::Extents tmpExtent;
	std::vector<RBX::Extents> primExtents;
	primExtents.reserve(primitives.size());

	// ContactManager requires unordered_set
	boost::unordered_set<const Primitive*> ignorePrimitives;
	
	for (G3D::Array<Primitive*>::const_iterator iter = primitives.begin(); iter != primitives.end(); ++iter) {
		// extents data
		tmpExtent = RBX::Extents(Math::toGrid((*iter)->getFastFuzzyExtents().min(), Tolerance::mainGrid()), 
								 Math::toGrid((*iter)->getFastFuzzyExtents().max(), Tolerance::mainGrid()));

		tmpExtent.expand(-Tolerance::maxOverlapOrGap());
		tmpExtent.shift(moved); //instead of again looping we can move the extents now

		primExtents.push_back(tmpExtent);
		// for contact manager
		ignorePrimitives.insert(*iter);
	}

	//sort prims based upon the y value 
	//TODO: do this only once when we select the prims?
	G3D::Array<Primitive*> tempPrimitives(primitives);
	Primitive* tempPrim = NULL;
	for (int ii=0; ii < (int)primExtents.size(); ++ii)
	{
		for (int jj=0; jj < (int)primExtents.size() - ii -1; ++jj)
		{
			if (primExtents[jj].min().y > primExtents[ii].min().y)
			{
				tmpExtent = primExtents[jj];
				primExtents[jj] = primExtents[ii];
				primExtents[ii] = tmpExtent;

				tempPrim = tempPrimitives[jj];
				tempPrimitives[jj] = tempPrimitives[ii];
				tempPrimitives[ii] = tempPrim;
			}
		}
	}

	//std::clock_t start = std::clock();

	if (intersectingWorldOrOthers_EXT(primExtents, tempPrimitives, ignorePrimitives, contactManager, customPlaneHeight, moved)) {
		searchUpGross_EXT(primExtents, tempPrimitives, ignorePrimitives, contactManager, customPlaneHeight, moved);
	}
	else {
		searchDownGross_EXT(primExtents, tempPrimitives, ignorePrimitives, contactManager, customPlaneHeight, moved);
	}

	//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "TryMove: %f - Moved: %f - Total Time: %lf", tryMove.y, moved.y, (std::clock() - start ) / (double) CLOCKS_PER_SEC);

	movePrimitives(primitives, moved);
	
	return moved;
}

void Dragger::searchUpGross_EXT(	std::vector<RBX::Extents> &primExtents,
									const G3D::Array<Primitive*>& primitives, 
									const boost::unordered_set<const Primitive*> &ignorePrimitives,
									ContactManager& contactManager,
									const float bottomPlaneHeight,
									Vector3& movedSoFar)
{
	if (fabs(movedSoFar.y) > maxMove()) {
		RBXASSERT_FISHING(0);
		return;
	}
	
	moveExtentsDelta(primExtents, Vector3(0.0f, 1.2f, 0.0f), movedSoFar);
	
	// hack to try to prevent infinite looping
	if (Math::isNanInfVector3(movedSoFar)) {
		RBXASSERT(0);
		return;
	}

	if (intersectingWorldOrOthers_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar)) {
		searchUpGross_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
	else {
		searchDownFine_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
}

void Dragger::searchDownGross_EXT(	std::vector<RBX::Extents> &primExtents,
									const G3D::Array<Primitive*>& primitives, 
									const boost::unordered_set<const Primitive*> &ignorePrimitives,
									ContactManager& contactManager,
									const float bottomPlaneHeight,
									Vector3& movedSoFar)
{
	if (fabs(movedSoFar.y) > maxMove()) {
		RBXASSERT_FISHING(0);
		return;
	}
	
	moveExtentsDelta(primExtents, Vector3(0.0f, -1.2f, 0.0f), movedSoFar);
	
	// hack to try to prevent infinite looping
	if (Math::isNanInfVector3(movedSoFar)) {
		RBXASSERT(0);
		return;
	}

	if (!intersectingWorldOrOthers_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar)) {
		searchDownGross_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
	else {
		searchUpFine_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
}

void Dragger::searchUpFine_EXT(	std::vector<RBX::Extents> &primExtents,
								const G3D::Array<Primitive*>& primitives, 
								const boost::unordered_set<const Primitive*> &ignorePrimitives,
								ContactManager& contactManager,
								const float bottomPlaneHeight,
								Vector3& movedSoFar)
{
	if (fabs(movedSoFar.y) > maxMove()) {
		RBXASSERT_FISHING(0);
		return;
	}
	
	moveExtentsDelta(primExtents, Vector3(0.0f, 0.1f, 0.0f), movedSoFar);
	
	// hack to try to prevent infinite looping
	if (Math::isNanInfVector3(movedSoFar)) {
		RBXASSERT(0);
		return;
	}

	if (intersectingWorldOrOthers_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar)) {
		searchUpFine_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
	else {
		//done
	}
}

void Dragger::searchDownFine_EXT(	std::vector<RBX::Extents> &primExtents,
									const G3D::Array<Primitive*>& primitives, 
									const boost::unordered_set<const Primitive*> &ignorePrimitives,
									ContactManager& contactManager,
									const float bottomPlaneHeight,
									Vector3& movedSoFar)
{
	if (fabs(movedSoFar.y) > maxMove()) {
		RBXASSERT_FISHING(0);
		return;
	}
	
	/*
	moveExtentsDelta(primExtents, Vector3(0.0f, -0.1f, 0.0f), movedSoFar);
	
	// hack to try to prevent infinite looping
	if (Math::isNanInfVector3(movedSoFar)) {
		RBXASSERT(0);
		return;
	}

	if (!intersectingWorldOrOthers_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar)) {
		searchDownFine_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
	else {
		searchUpFine_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
	}
	*/

	int min = 0;
	int max = 12;
	int mid = 0;
	while (max > min)
	{
		mid = (min+max)/2;
		moveExtentsDelta(primExtents, Vector3(0.0f, -(mid)/10.0f, 0.0f), movedSoFar);
		
		// hack to try to prevent infinite looping
		if (Math::isNanInfVector3(movedSoFar)) {
			RBXASSERT(0);
			return;
		}

		if (intersectingWorldOrOthers_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar)) {
			searchUpFine_EXT(primExtents, primitives, ignorePrimitives, contactManager, bottomPlaneHeight, movedSoFar);
			return;
		}
		else
		{
			max = mid;
		}
	}
}

bool Dragger::intersectingWorldOrOthers_EXT(std::vector<RBX::Extents> &primExtents,
											const G3D::Array<Primitive*>& primitives, 
											const boost::unordered_set<const Primitive*> &ignorePrimitives,
											ContactManager& contactManager,
											const float bottomPlaneHeight,  
											const Vector3& movedSoFar)
{
	//check if intersecting with ground
	if (intersectingGroundPlane_EXT(primExtents, primitives, bottomPlaneHeight, movedSoFar))
		return true;

	G3D::Array<Primitive*> foundPrims;
	for (int a = 0; a < (int)primExtents.size(); ++a) {
		//look for only 1 prim instead of getting a complete list
		foundPrims.fastClear();
		contactManager.getPrimitivesTouchingExtents(primExtents[a], ignorePrimitives, 1, foundPrims);
		if (foundPrims.size()) {
			//ideally we must get only one prim here!!!
			for (int zz = 0; zz < foundPrims.size(); ++zz) {
				RBX::Extents tmpExtent(Math::toGrid(foundPrims[zz]->getFastFuzzyExtents().min(), Tolerance::mainGrid()), 
									   Math::toGrid(foundPrims[zz]->getFastFuzzyExtents().max(), Tolerance::mainGrid()));
				tmpExtent.expand(-Tolerance::maxOverlapOrGap()+0.001);
				
				if (tmpExtent.overlapsOrTouches(primExtents[a]))
				{
					//if the found prim is a block and not rotated then return
					if( foundPrims[zz]->getConstGeometry()->getGeometryType() == Geometry::GEOMETRY_BLOCK && 
						Math::isAxisAligned(foundPrims[zz]->getCoordinateFrame().rotation) )
						return true;

					CoordinateFrame foundCFrame  = foundPrims[zz]->getCoordinateFrame();
					CoordinateFrame movingPrimCF(primitives[a]->getCoordinateFrame().rotation, primExtents[a].center());

					if (isIntersecting(foundPrims[zz], foundCFrame, primitives[a], movingPrimCF))
						return true;
				}
			}
		}

		//check if there are any terrain cells in the extent's region
		Region3 correctedRegion(primExtents[a].min(), primExtents[a].max());
		if (contactManager.terrainCellsInRegion3(correctedRegion))
			return true;
	}

	return false;
}

bool Dragger::intersectingGroundPlane_EXT(	const std::vector<RBX::Extents>& primExtents, 
											const G3D::Array<Primitive*>& primitives, 
											const float yHeight, 
											const Vector3& movedSoFar)
{
	for (int i = 0; i < primitives.size(); ++i) {
		if ((primExtents[i].min().y < yHeight)&& (primitives[i]->getConstGeometry()->collidesWithGroundPlane(primExtents[i].center(), yHeight)))
		{
			return true;
		}
	}

	return false;
}

bool Dragger::isIntersecting(const Primitive* prim1, const CoordinateFrame &cFrame1, const Primitive* prim2, const CoordinateFrame &cFrame2)
{
	Geometry::GeometryType prim1GeomType(prim1->getConstGeometry()->getGeometryType());
	Geometry::GeometryType prim2GeomType(prim2->getConstGeometry()->getGeometryType());

	switch (prim1GeomType)
	{
	case Geometry::GEOMETRY_BALL:
		{
			switch (prim2GeomType)
			{
			case Geometry::GEOMETRY_BALL:
				return checkBallBallIntersection(prim1, cFrame1, prim2, cFrame2);
			default:
				return checkBallPolyIntersection(prim1, cFrame1, prim2, cFrame2);
			}
		}

	default:
		{
			switch (prim2GeomType)
			{
			case Geometry::GEOMETRY_BALL:
				return checkBallPolyIntersection(prim2, cFrame2, prim1, cFrame1);
			default:
				return checkPolyPolyIntersection(prim1, cFrame1, prim2, cFrame2);
			}
		}
	}
}

bool Dragger::checkBallPolyIntersection(const Primitive* ballPrim, const CoordinateFrame &ballCFrame, const Primitive* polyPrim, const CoordinateFrame &polyCFrame)
{
	const Ball* ballGeom = dynamic_cast<const Ball*>(ballPrim->getConstGeometry());
	const Poly* polyGeom = dynamic_cast<const Poly*>(polyPrim->getConstGeometry());
	if (!ballGeom || !polyGeom)
		return true;

	const POLY::Mesh* polyMesh = polyGeom->getMesh();
	if (!polyMesh)
		return true;

	// check if polygon edge is lying inside of sphere
	Sphere ballSphere(Vector3::zero(), ballGeom->getRadius());
	for (unsigned int i = 0; i < polyMesh->numVertices(); i++)
	{
		if (ballSphere.contains(ballCFrame.pointToObjectSpace(polyCFrame.pointToWorldSpace(polyMesh->getVertex(i)->getOffset()))))
			return true;
	}

	Vector3 p1, p2, hitPoint, unusedSurfaceNormal;
	RbxRay edgeRay;

	// check if edge is intersecting with sphere
	for (unsigned int j = 0; j < polyMesh->numEdges(); j++)
	{
		p1      = ballCFrame.pointToObjectSpace(polyCFrame.pointToWorldSpace(polyMesh->getEdge(j)->getVertexOffset(NULL, 0)));
		p2      = ballCFrame.pointToObjectSpace(polyCFrame.pointToWorldSpace(polyMesh->getEdge(j)->getVertexOffset(NULL, 1)));
		edgeRay = RbxRay(p1, (p2 - p1).direction());

		if ((const_cast<Ball*>(ballGeom))->hitTest(edgeRay, hitPoint, unusedSurfaceNormal))
		{
			if (((hitPoint - p1).magnitude() <= (p2 - p1).magnitude()))
				return true;
		}
	}

	return false;
}

bool Dragger::checkBallBallIntersection(const Primitive* ballPrim1, const CoordinateFrame &ballCFrame1, const Primitive* ballPrim2, const CoordinateFrame &ballCFrame2)
{
	const Ball* ballGeom1 = dynamic_cast<const Ball*>(ballPrim1->getConstGeometry());
	const Ball* ballGeom2 = dynamic_cast<const Ball*>(ballPrim2->getConstGeometry());

	if (!ballGeom1 || !ballGeom2)
		return true;

	Vector3 delta = ballCFrame1.translation - ballCFrame2.translation;
	float radiusSum = ballGeom1->getRadius() + ballGeom2->getRadius();
	
	if (Math::longestVector3Component(delta) < radiusSum)
		return (delta.length() < (radiusSum - Tolerance::maxOverlapOrGap()));

	return false;
}

bool Dragger::checkPolyPolyIntersection(const Primitive* polyPrim1, const CoordinateFrame &polyCFrame1, const Primitive* polyPrim2, const CoordinateFrame &polyCFrame2)
{
	const Poly* polyGeom1 = dynamic_cast<const Poly*>(polyPrim1->getConstGeometry());
	const Poly* polyGeom2 = dynamic_cast<const Poly*>(polyPrim2->getConstGeometry());

	if (!polyGeom1 || !polyGeom2)
		return true;

	const POLY::Mesh* polyMesh1 = polyGeom1->getMesh();
	const POLY::Mesh* polyMesh2 = polyGeom2->getMesh();
	if (!polyMesh1 || !polyMesh2)
		return true;

	// Perform 4 checks for polyhedron intersection; return immediately if any are true

	// #1: Are any movingPrimPoly verts inside of the foundPoly
	for (unsigned int i = 0; i < polyMesh1->numVertices(); i++)
	{
		if (polyMesh2->pointInMesh(polyCFrame2.pointToObjectSpace(polyCFrame1.pointToWorldSpace(polyMesh1->getVertex(i)->getOffset()))))
			return true;
	}

	// #2: Are any foundPoly verts inside of the movingPrimPoly
	for (unsigned int i = 0; i < polyMesh2->numVertices(); i++)
	{
		if (polyMesh1->pointInMesh(polyCFrame1.pointToObjectSpace(polyCFrame2.pointToWorldSpace(polyMesh2->getVertex(i)->getOffset()))))
			return true;
	}

	return false;
}

void Dragger::moveExtents(std::vector<RBX::Extents> &primExtents, const Vector3& delta)
{
	std::vector<RBX::Extents>::iterator end_iter = primExtents.end();
	for (std::vector<RBX::Extents>::iterator iter = primExtents.begin(); iter != end_iter; ++iter) 
		(*iter).shift(delta);
}

void Dragger::moveExtentsDelta(std::vector<RBX::Extents> &primExtents, const Vector3& delta, Vector3& movedSoFar)
{
	moveExtents(primExtents, delta);
	//movedSoFar = DragUtilities::toGrid(movedSoFar + delta);
	movedSoFar = movedSoFar + delta;
}

} // namespace
