/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/MaterialProperties.h"
#include "V8World/Contact.h"
#include "V8World/Ball.h"
#include "V8World/Block.h"
#include "V8World/Primitive.h"
#include "v8World/Geometry.h"
#include "v8World/World.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/ContactConnector.h"
#include "V8Kernel/Body.h"
#include "Util/StlExtra.h"

DYNAMIC_FASTFLAGVARIABLE(FixTouchEndedReporting, false)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

namespace RBX {

int BlockBlockContact::pairMatches = 0;
int BlockBlockContact::pairMisses = 0;
int BlockBlockContact::featureMatches = 0;
int BlockBlockContact::featureMisses = 0;


float BlockBlockContact::pairHitRatio()
{
	int denom = pairMatches + pairMisses;
	return (denom == 0) 
		? -1
		: (float)(pairMatches) / (float)(denom);
}

float BlockBlockContact::featureHitRatio()
{
	int denom = featureMatches + featureMisses;
	return (denom == 0) 
		? -1
		: (float)(featureMatches) / (float)(denom);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

Body* Contact::getBody(int i) 
{
	return getPrimitive(i)->getBody();
}

Contact::Contact(Primitive* p0, 
				Primitive* p1) 
	: Edge(p0, p1)
	, steppingIndex(-1)
	, lastUiContactStep(-2)
	, numTouchCycles(0)
{
    contactParams = NULL;
}

Contact::~Contact() 
{
	if (DFFlag::FixTouchEndedReporting && lastUiContactStep > 0)
		Primitive::onStopOverlap(getPrimitive(0), getPrimitive(1));

    if(contactParams)
        delete contactParams;
	setPrimitive(0, NULL);
	setPrimitive(1, NULL);
}


void Contact::primitiveMovedExternally() {
	for (int i = 0; i < numConnectors(); ++i) {
		getConnector(i)->reset();
	}
}

/*
For every contact pair - maximum of one notification for every UI step.
In addition, the contact must leave contact to create another event
in the next UI step

							In Contact

						true		false
Last == This			last = this	ignore
Last == this-1			last = this	last-> -1
Last == this-2			last = this last-> -1
Last == -1				notify		ignore
*/

bool Contact::step(int longStepId)
{
	RBXASSERT(longStepId >= 0);

	bool inContact = stepContact();

	if (inContact) {
		if (lastUiContactStep < 0) {
			Primitive::onNewOverlap(getPrimitive(0), getPrimitive(1));
			numTouchCycles++;
		}
		lastUiContactStep = longStepId;
	}
	else {
		if (lastUiContactStep < longStepId) 
		{
			if (lastUiContactStep != -1 && (!DFFlag::FixTouchEndedReporting || lastUiContactStep > 0))
				Primitive::onStopOverlap(getPrimitive(0), getPrimitive(1));

			lastUiContactStep = -1;

			// reset touch cycles
			G3D::Vector3 dir = getPrimitive(0)->getCoordinateFrameUnsafe().translation - getPrimitive(1)->getCoordinateFrameUnsafe().translation;

			if (fabsf(dir.squaredLength()) > 22.0f * 22.0f)	// check for a distance of 22
				numTouchCycles = 0;
		}
	}
	return inContact;
}

bool Contact::computeIsAdjacentUi(float spaceAllowed)
{
	bool isOverlapping = computeIsCollidingUi(spaceAllowed);
	if (isOverlapping) {
		return false;
	}
	else {
		bool isProximate = computeIsCollidingUi(-spaceAllowed);
		return isProximate;
	}
}

bool Contact::computeIsCollidingUi(float overlapIgnored)
{
	getPrimitive(0)->getFastFuzzyExtents();			// updates - outside of world loop
	getPrimitive(1)->getFastFuzzyExtents();
	return computeIsColliding(overlapIgnored);
}

float calculateFriction(float c0, float c1)
{
	c0 = G3D::clamp(c0, 0.0f, 2.0f);
	c1 = G3D::clamp(c1, 0.0f, 2.0f);
	if (	((c0 <= 1.0f) && (c1 <= 1.0f)) 
		||	((c0 >= 1.0f) && (c1 >= 1.0f))	)
	{
		return std::min(c0, c1);
	}
	else 
	{
		return (c0 + c1 - 1.0f);
	}
}

void Contact::onPrimitiveContactParametersChanged()
{
    if(!contactParams)
        generateDataForMovingAssemblyStage();

    Primitive* p0 = getPrimitive(0);
    Primitive* p1 = getPrimitive(1);
	World* world = p0->getWorld() ? p0->getWorld() : p1->getWorld();
	RBXASSERT(world);

	if (world->getUsingNewPhysicalProperties())
	{
		MaterialProperties::updateContactParamsPrims(*contactParams, p0, p1);
	}
	else
	{
		contactParams->kFriction	= calculateFriction(p0->getFriction(),	p1->getFriction() );
		contactParams->kElasticity	= std::min(			p0->getElasticity(),p1->getElasticity() );
		contactParams->kSpring		= std::min(			p0->getJointK(),	p1->getJointK() );
		contactParams->kNeg			= contactParams->kSpring * Constants::getElasticMultiplier(contactParams->kElasticity);
	}
}

void Contact::deleteConnector(ContactConnector* c)
{
	RBXASSERT_VERY_FAST(c);
	getKernel()->removeConnector(c);
	delete c;
}

void Contact::generateDataForMovingAssemblyStage(void)
{
    if(!contactParams)
        contactParams = new ContactParams();

    onPrimitiveContactParametersChanged();
}

void Contact::invalidateContactCache()
{
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

Ball* BallBallContact::ball(int i) 
{
	return rbx_static_cast<Ball*>(getPrimitive(i)->getGeometry());
}

ContactConnector* BallBallContact::getConnector(int i)
{
	return ballBallConnector;
}

void BallBallContact::deleteAllConnectors() {
	if (ballBallConnector) {
		deleteConnector(ballBallConnector);
		ballBallConnector = NULL;
	}
}


bool BallBallContact::computeIsColliding(float overlapIgnored)
{
	float r0 = ball(0)->getRadius();
	float r1 = ball(1)->getRadius();

	Vector3 delta = getBody(1)->getPos() - getBody(0)->getPos();
	float radSum = r0 + r1;

	float radSumOverlapIgnored = radSum - overlapIgnored;

	return radSumOverlapIgnored > 0 && delta.squaredMagnitude() < radSumOverlapIgnored * radSumOverlapIgnored;
}

bool BallBallContact::stepContact() 
{
    if(contactParams)
    {
	    if (BallBallContact::computeIsColliding(0.0)) {
		    if (inKernel()) {
			    if (!ballBallConnector) {
				    ballBallConnector = new BallBallConnector(getBody(0), getBody(1), *contactParams);
				    getKernel()->insertConnector(ballBallConnector);
			    }
			    ballBallConnector->setRadius(	ball(0)->getRadius(),
											    ball(1)->getRadius()); // only do this once...
				ballBallConnector->updateContactPoint();
		    }
		    return true;
	    }
	    else {
		    deleteAllConnectors();
		    return false;
	    }
    }
    else
        return false;
}	

void BallBallContact::generateDataForMovingAssemblyStage(void)
{
    Contact::generateDataForMovingAssemblyStage();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
Primitive* BallBlockContact::ballPrim() {return getPrimitive(0);}
Primitive* BallBlockContact::blockPrim() {return getPrimitive(1);}

Ball* BallBlockContact::ball() {
	return rbx_static_cast<Ball*>(ballPrim()->getGeometry());
}
Block* BallBlockContact::block() {
	return rbx_static_cast<Block*>(blockPrim()->getGeometry());
}

ContactConnector* BallBlockContact::getConnector(int i)
{
	return ballBlockConnector;
}


void BallBlockContact::deleteAllConnectors()
{
	if (ballBlockConnector) {
		deleteConnector(ballBlockConnector);
		ballBlockConnector = NULL;
	}
}


bool BallBlockContact::computeIsColliding(float overlapIgnored)	
{
	int onBorder;
	Vector3int16 clip;
	Vector3 projectionInBlock;
	return computeIsColliding(onBorder, clip, projectionInBlock, overlapIgnored);
}

bool BallBlockContact::computeIsColliding(int& onBorder, 
										  Vector3int16& clip, 
										  Vector3& projectionInBlock,
										  float overlapIgnored)	
{
	if (Primitive::aaBoxCollide(*ballPrim(), *blockPrim()))
	{
		Body* ballBody = ballPrim()->getBody();
		Body* blockBody = blockPrim()->getBody();

		const CoordinateFrame& ballCoord = ballBody->getCoordinateFrameFast();
		const CoordinateFrame& blockCoord = blockBody->getCoordinateFrameFast();

		Vector3 blockToBall = ballCoord.translation - blockCoord.translation;

		// to block coordinates
		projectionInBlock = blockCoord.rotation.transpose() * blockToBall;

		// projection is in block coords, on the face of block
		block()->projectToFace(projectionInBlock, clip, onBorder);

		Vector3 blockPtWorld = blockCoord.pointToWorldSpace(projectionInBlock);
			
		Vector3 depth = blockPtWorld - ballCoord.translation;

		return (depth.length() < (ball()->getRadius() - overlapIgnored));
	}
	else
	{
		return false;
	}
}
	

bool BallBlockContact::stepContact() 
{
    if(contactParams)
    {
	    int onBorder;
	    Vector3int16 clip;
	    Vector3 projectionInBlock;

	    if (BallBlockContact::computeIsColliding(onBorder, clip, projectionInBlock, 0.0)) 
	    {	
		    if (inKernel()) 
		    {
			    if (!ballBlockConnector) {
				    ballBlockConnector = new BallBlockConnector(ballPrim()->getBody(), blockPrim()->getBody(), *contactParams);
				    getKernel()->insertConnector(ballBlockConnector);
			    }

			    const Vector3* offset;
			    NormalId normalID;
			    GeoPairType pairType = onBorder ? 
				    block()->getBallBlockInfo(onBorder, clip, offset, normalID) :
				    block()->getBallInsideInfo(projectionInBlock, offset, normalID);
    			
			    ballBlockConnector->setBallBlock(ball()->getRadius(), offset, normalID, pairType);
				ballBlockConnector->updateContactPoint();
		    }
		    return true;
	    }
	    else {
		    deleteAllConnectors();
		    return false;
	    }
    }
    else
        return false;
}		

void BallBlockContact::generateDataForMovingAssemblyStage(void)
{
    Contact::generateDataForMovingAssemblyStage();
}

/////////////////////////////////////////////////////////////////
//
//	Match array - for every 8 steps (world step),
//
//	match new block contact pairs with ones from the previous eight steps
//

Block* BlockBlockContact::block(int i) {
	return rbx_static_cast<Block*>(getPrimitive(i)->getGeometry());
}

ContactConnector* BlockBlockContact::getConnector(int i)
{
    if( !myData )
        return NULL;
    else
		return myData->getConnector(i);
}

void BlockBlockContact::deleteAllConnectors()
{
    RBXASSERT( myData );
    if( myData ) 
	{
        for (size_t i = 0; i < (size_t)myData->numConnectors(); ++i) 
		{
		    deleteConnector(myData->getConnector(i));
	    }

	    myData->clearConnectors();
    }
}

GeoPairConnector* BlockBlockContact::findGeoPairConnector(	Body* b0, 
															Body* b1, 
															GeoPairType _pairType,
															int param0, 
															int param1) 
{
    RBXASSERT( myData && contactParams );
    if( myData && contactParams )
	    return myData->findGeoPairConnector(b0, b1, _pairType, param0, param1);
    else
        return NULL;
}

bool BlockBlockContact::computeIsColliding(float overlapIgnored)
{
	bool planeContact = false;
	return computeIsColliding(overlapIgnored, planeContact);
}


bool BlockBlockContact::computeIsColliding(float overlapIgnored, bool& planeContact)
{
    if( !myData )
        generateDataForMovingAssemblyStage();

	return (	Primitive::aaBoxCollide(*getPrimitive(0), *getPrimitive(1))
			&&	getBestPlaneEdge(overlapIgnored, planeContact)
			);
}


bool BlockBlockContact::stepContact() 
{
    RBXASSERT( myData );
    if( myData )
		return myData->stepContact();
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////////////

void BlockBlockContact::loadGeoPairEdgeEdge(
											int b0, 
											int b1, 
											int edge0, 
											int edge1) 
{
	NormalId norm0 = block(b0)->getEdgeNormal(edge0);
	NormalId norm1 = block(b1)->getEdgeNormal(edge1);

	GeoPairConnector* geoPair = findGeoPairConnector(	getBody(b0), 
														getBody(b1), 
														EDGE_EDGE_PAIR, 
														norm0,
														norm1	);

	if(!geoPair)
	{
	    BlockBlockContact::pairMisses++;
	    geoPair = new GeoPairConnector(getBody(b0), getBody(b1), *getContactParams());
		geoPair->setEdgeEdge(	block(b0)->getEdgeVertex(edge0),
								block(b1)->getEdgeVertex(edge1),
								norm0,
								norm1	);
		geoPair->updateContactPoint();
		if (geoPair->getContactPoint().length >= -ContactConnector::overlapGoal())
		{
			delete geoPair;
			return;
		}
	    getKernel()->insertConnector(geoPair);
	} else
	{
		geoPair->setEdgeEdge(	block(b0)->getEdgeVertex(edge0),
								block(b1)->getEdgeVertex(edge1),
								norm0,
								norm1	);
		geoPair->updateContactPoint();
		if (geoPair->getContactPoint().length >= -ContactConnector::overlapGoal())
		{
			deleteConnector(geoPair);
			return;
		}
	}
	RBXASSERT(geoPair->getContactPoint().length < 0);
	RBXASSERT(geoPair->getContactPoint().normal.magnitude() > 0.99f);

	myData->connectors[!myData->connectorsIndex].push_back(geoPair);
}

/////////////////////////////////////////////////////////////////////////////////////////


void BlockBlockContact::loadGeoPairPointPlane(	int pointBody,
												int planeBody, 
												int pointID, 
												NormalId pointFaceID, 
												NormalId planeFaceID) 
{
	GeoPairConnector* geoPair = NULL;
	if((geoPair = findGeoPairConnector(getBody(pointBody), getBody(planeBody), POINT_PLANE_PAIR, pointID, planeFaceID)))
	{
		if(geoFeaturesOverlap(pointBody, planeBody, pointID, pointFaceID, planeFaceID))
		{
			geoPair->setPointPlane(	block(pointBody)->getFaceVertex(pointFaceID, pointID),
									block(planeBody)->getFaceVertex(planeFaceID, 0),
									pointID,
									planeFaceID	);
			geoPair->updateContactPoint();
			RBXASSERT(geoPair->getContactPoint().length < 0);
			RBXASSERT(geoPair->getContactPoint().normal.magnitude() > 0.99f);

			myData->connectors[!myData->connectorsIndex].push_back(geoPair);
		}
		else
			deleteConnector(geoPair);
	}
	else
	{
		if(geoFeaturesOverlap(pointBody, planeBody, pointID, pointFaceID, planeFaceID))
		{
			BlockBlockContact::pairMisses++;
			geoPair = new GeoPairConnector(getBody(pointBody), getBody(planeBody), *getContactParams());
			geoPair->setPointPlane(	block(pointBody)->getFaceVertex(pointFaceID, pointID),
									block(planeBody)->getFaceVertex(planeFaceID, 0),
									pointID,
									planeFaceID	);
			geoPair->updateContactPoint();
			getKernel()->insertConnector(geoPair);
			RBXASSERT(geoPair->getContactPoint().length < -ContactConnector::overlapGoal() + 0.001);
			RBXASSERT(geoPair->getContactPoint().normal.magnitude() > 0.99f);

			myData->connectors[!myData->connectorsIndex].push_back(geoPair);
		}
	}
}

bool BlockBlockContact::geoFeaturesOverlap(	int pointBody,
											int planeBody, 
											int pointID, 
											NormalId pointFaceID, 
											NormalId planeFaceID) 
{
	Vector3 pVertexInWorld = getBody(pointBody)->getCoordinateFrameFast().pointToWorldSpace(*block(pointBody)->getFaceVertex(pointFaceID, pointID));
	Vector3 pVertexInPlaneBody = getBody(planeBody)->getCoordinateFrameFast().pointToObjectSpace(pVertexInWorld);
	Vector3 pPlaneVertexInPlanBody = *block(planeBody)->getFaceVertex(planeFaceID, 0);

	float overlap = normalIdToVector3(planeFaceID).dot(pVertexInPlaneBody - pPlaneVertexInPlanBody);

	return overlap < -ContactConnector::overlapGoal();
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


// returns true if in contact
bool BlockBlockContact::getBestPlaneEdge(float overlapIgnored, bool& planeContact)
{    
    if( myData )
        return myData->getBestPlaneEdge(overlapIgnored, planeContact);
    else
        return false;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

BlockBlockContact::BlockBlockContact(Primitive* p0, Primitive* p1) : Contact(p0, p1)
{
    myData = NULL;
}

BlockBlockContact::~BlockBlockContact( void )
{
    if( myData )
        delete myData;
}

int BlockBlockContact::numConnectors() const 
{
    if( !myData )
        return 0;
    else
		return myData->numConnectors();
}

void BlockBlockContact::generateDataForMovingAssemblyStage(void)
{
    Contact::generateDataForMovingAssemblyStage();
    if( !myData )
        myData = new BlockBlockContactData(this);
}

BlockBlockContactData::BlockBlockContactData(BlockBlockContact* owner)
{
    witnessId = 0;
    separatingAxisId = 0;
    myOwner = owner;
	feature[0] = -1;
	feature[1] = -1;
	connectorsIndex = 0;
}

ContactConnector* BlockBlockContactData::getConnector(int i)
{
    return connectors[connectorsIndex][i];
}

void BlockBlockContactData::clearConnectors()
{
	connectors[connectorsIndex].fastClear();
}

GeoPairConnector* BlockBlockContactData::findGeoPairConnector(	Body* b0, 
																Body* b1, 
																GeoPairType _pairType,
																int param0, 
																int param1) 
{
    ContactParams* cp = myOwner->getContactParams();
    RBXASSERT(cp);
    if(cp)
    {
	    for (size_t i = 0; i < connectors[connectorsIndex].size(); ++i) {
		    GeoPairConnector* found = connectors[connectorsIndex][i];
		    if (found->match(b0, b1, _pairType, param0, param1)) {
			    BlockBlockContact::pairMatches++;
				connectors[connectorsIndex].fastRemove(i);
			    found->setBody(0, b0);		// for now, need to do this because matching both ways
			    found->setBody(1, b1);
			    return found;
		    }
	    }
		return NULL;
    }
    else
        return NULL;
}

bool BlockBlockContactData::stepContact() 
{
	bool planeContact = false;
    if (myOwner->computeIsColliding(0.0f, planeContact)) {
	    if (myOwner->inKernel()) {

		    if (planeContact) {						// should return 2-8
			    computePlaneContact();
		    }
		    else {									// should return 1
			    myOwner->loadGeoPairEdgeEdge(0, 1, feature[0] - 6, feature[1] - 6);
		    }
			
		    myOwner->deleteAllConnectors();
			// switch the connectors index so we are pointing to the newly filled container
			connectorsIndex = !connectorsIndex;
	    }
	    return true;
    }
    else {
	    myOwner->deleteAllConnectors();
	    feature[0] = -1;
	    feature[1] = -1;	// should be in deleteAllPairs
	    return false;
    }
}

void BlockBlockContactData::loadGeoPairEdgeEdgePlane(
													int edgeBody, 
													int planeBody, 
													int edge0, 
													int edge1) 
{
	NormalId norm0 = myOwner->block(edgeBody)->getEdgeNormal(edge0);
	NormalId norm1 = myOwner->block(planeBody)->getEdgeNormal(edge1);

	GeoPairConnector* geoPair = findGeoPairConnector(	myOwner->getBody(edgeBody), 
														myOwner->getBody(planeBody), 
														EDGE_EDGE_PLANE_PAIR, 
														norm0,
														norm1 );

	if(!geoPair)
	{
	    BlockBlockContact::pairMisses++;
	    geoPair = new GeoPairConnector(myOwner->getBody(edgeBody), myOwner->getBody(planeBody), *myOwner->getContactParams());

		geoPair->setEdgeEdgePlane(	myOwner->block(edgeBody)->getEdgeVertex(edge0),
									myOwner->block(planeBody)->getEdgeVertex(edge1),
									norm0,
									norm1,
									planeID,
									myOwner->getPrimitive(edgeBody)->getSize()[norm0 % 3],
									myOwner->getPrimitive(planeBody)->getSize()[norm1 % 3]);
		geoPair->updateContactPoint();
		if (geoPair->getContactPoint().length >= -ContactConnector::overlapGoal())
		{
			delete geoPair;
			return;
		}
	    myOwner->getKernel()->insertConnector(geoPair);
	} else
	{
		geoPair->setEdgeEdgePlane(	myOwner->block(edgeBody)->getEdgeVertex(edge0),
									myOwner->block(planeBody)->getEdgeVertex(edge1),
									norm0,
									norm1,
									planeID,
									myOwner->getPrimitive(edgeBody)->getSize()[norm0 % 3],
									myOwner->getPrimitive(planeBody)->getSize()[norm1 % 3]);
		geoPair->updateContactPoint();
		if (geoPair->getContactPoint().length >= -ContactConnector::overlapGoal())
		{
			myOwner->deleteConnector(geoPair);
			return;
		}
	}
	RBXASSERT(geoPair->getContactPoint().length < 0);
	RBXASSERT(geoPair->getContactPoint().normal.magnitude() > 0.99f);
	connectors[!connectorsIndex].push_back(geoPair);
}

// returns true if in contact
bool BlockBlockContactData::getBestPlaneEdge(float overlapIgnored, bool& planeContact)
{
	RBXASSERT(planeContact == false);

    const float epsilon = 1e-03f;
	const float hysteresis = 1.01f;	// to switch planes, must be Nx as good...

	float bestPlaneLength = Math::inf();
	float bestEdgeLength = Math::inf();
	float lastPlaneLength = Math::inf();	// not last time through, but length of the plane

	// if not the first time through, and last time a plane was choosen...
	int lastFeature[2];
	lastFeature[0] = feature[0];
	lastFeature[1] = feature[1];
	bool checkLastFeature = (		((lastFeature[0] >= 0) && (lastFeature[0] < 6))
								||	((lastFeature[1] >= 0) && (lastFeature[1] < 6)));

    // Box0 normals;
	for (int i = witnessId; i < witnessId + 2; ++i)
	{
		int baseId = i % 2;
		int testId = (i + 1) % 2;
		const CoordinateFrame& cBase = myOwner->getBody(baseId)->getCoordinateFrameFast();
		const CoordinateFrame& cTest = myOwner->getBody(testId)->getCoordinateFrameFast();
		const Vector3& pTest = cTest.translation;
		const Matrix3& rBase = cBase.rotation;
		const Matrix3& rTest = cTest.rotation;
		const Vector3& eBase = myOwner->block(baseId)->getExtent();
		const Vector3& eTest = myOwner->block(testId)->getExtent();
		Vector3 pTestInBase = cBase.pointToObjectSpace(pTest);
		{
			for (int j = separatingAxisId; j < separatingAxisId + 3; ++j)
			{
				int axisId = j % 3;
				float projectedExtent, overlap;
				myOwner->boxProjection(rBase.column(axisId), rTest, eTest, projectedExtent);
				if (!myOwner->updateBestAxis(eBase[axisId], pTestInBase[axisId], projectedExtent, overlap, overlapIgnored)) {
					witnessId = baseId;
					separatingAxisId = axisId;
                    BlockBlockContact::featureMatches++;
					return false;
				}
				else {
					if (checkLastFeature) {							// set the value of the last feature
						if ((lastFeature[baseId] % 3) == axisId) {
							lastPlaneLength = overlap;
						}
					}
					if (overlap < bestPlaneLength) {
						bestPlaneLength = overlap;
						feature[baseId] = pTestInBase[axisId] > 0 ? axisId : axisId + 3;
						feature[testId] = -1;
						witnessId = baseId;
						separatingAxisId = axisId;
						planeContact = true;
					}
				}
			}
		}
	}

	BlockBlockContact::featureMisses++;
	// Do plane hysteresis here - new must be better than the old...
	if (checkLastFeature) {
		if ((feature[0] != lastFeature[0]) || (feature[1] != lastFeature[1])) {
			bool useNew = ((bestPlaneLength * hysteresis) < lastPlaneLength);
			if (!useNew) {
				bestPlaneLength = lastPlaneLength;
				feature[0] = lastFeature[0];
				feature[1] = lastFeature[1];
			}
		}
	}

	const CoordinateFrame& c0 = myOwner->getBody(0)->getCoordinateFrameFast();
	const CoordinateFrame& c1 = myOwner->getBody(1)->getCoordinateFrameFast();
	const Matrix3& R0 = c0.rotation;
	const Matrix3& R1 = c1.rotation;
	const Vector3& extent0 = myOwner->block(0)->getExtent();
	const Vector3& extent1 = myOwner->block(1)->getExtent();
	Vector3 p0p1 = c1.translation - c0.translation;

    // edges cross() edges
    for (int i0 = 0; i0 < 3; i0++) {
        for (int i1 = 0; i1 < 3; i1++) {
            Vector3 crossAxis = R0.column(i0).cross(R1.column(i1));
			
            // Since all axes are unit length (assumed), then can
            // just compare against a constant (not relative) epsilon
			// note - replaced with length instead of length squared, so epilon
			// change from 1e-6 to 1e-3;

            if ( crossAxis.unitize() <= epsilon )
            {
				return (planeContact);
            }

			float p0p1inCrossAxis = crossAxis.dot(p0p1);
			float proj0, proj1;
			myOwner->boxProjection(crossAxis, R0, extent0, proj0);
			myOwner->boxProjection(crossAxis, R1, extent1, proj1);
				
			float overlap;
			if (!myOwner->updateBestAxis(proj0, p0p1inCrossAxis, proj1, overlap, overlapIgnored)) {
		         return false;
			}
			else {
				if (overlap < bestEdgeLength) {
					bestEdgeLength = overlap;
					if (bestEdgeLength * 10.0 < bestPlaneLength) {
						NormalId n0 = static_cast<NormalId>(i0);
						NormalId n1 = static_cast<NormalId>(i1);
						if (p0p1inCrossAxis > 0) {
							feature[0] = 6 + myOwner->block(0)->getClosestEdge(R0, n0, crossAxis);
							feature[1] = 6 + myOwner->block(1)->getClosestEdge(R1, n1, -crossAxis);
						}
						else {
							feature[0] = 6 + myOwner->block(0)->getClosestEdge(R0, n0, -crossAxis);
							feature[1] = 6 + myOwner->block(1)->getClosestEdge(R1, n1, crossAxis);
						}
						planeContact = false;
					}
				}
			}
        }
    }
	// bestPlaneLenth is smallest value of plane overlap, corresponds to best plane
	// bestEdgeLength is smallest value of edge overlap, corrseponds to best edge pair
	// if bestPlane = 1.0 and bestEdge = 1.0, pick the plane
	// if bestPlane = 1.0 and bestEdge = 1.05, pick the edge
	// i.e. - lean towards picking a plane contact....
	// because the edge contact is always only ONE contact point
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
//
//	BlockBlock::computePlaneContact (& helper functions)
//

// returns number of contact points
int BlockBlockContactData::computePlaneContact(void) 
{
	if (feature[0] >= 0) {
		planeID = static_cast<NormalId>(feature[0]);
		bPlane = 0;
		bOther = 1;
	}
	else {
		planeID = static_cast<NormalId>(feature[1]);
		bPlane = 1;
		bOther = 0;
	}

	const CoordinateFrame& otherFrame = myOwner->getBody(bOther)->getCoordinateFrameFast();
	const CoordinateFrame& planeFrame = myOwner->getBody(bPlane)->getCoordinateFrameFast();
	CoordinateFrame otherToPlane = planeFrame.inverse() * otherFrame;

	Block& otherBlock = *(myOwner->block(bOther));
	Block& planeBlock = *(myOwner->block(bPlane));

	Vector3 planeNormal = Math::getWorldNormal(planeID, planeFrame);

	// what's the best plane on "other" - use negative planeNormal here...
	otherPlaneID = Math::getClosestObjectNormalId(-planeNormal, otherFrame.rotation);

	// the 0 vertex will be +,+ in a projection along the plane normal
	const Vector3* planeFaceVertex = planeBlock.getFaceVertex(planeID, 0);
	Vector2 planeRect = planeBlock.getProjectedVertex(*planeFaceVertex, planeID);

	Vector2 otherQuad[4];
	for (int i = 0; i < 4; i++) {
		const Vector3* otherFaceVertex = otherBlock.getFaceVertex(otherPlaneID, i);
		Vector3 otherVertexPlaneCoords = otherToPlane.pointToWorldSpace(*otherFaceVertex);
		otherQuad[i] = otherBlock.getProjectedVertex(otherVertexPlaneCoords, planeID);
	}

	return intersectRectQuad(planeRect, otherQuad);
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

int BlockBlockContactData::intersectRectQuad(Vector2& planeRect, Vector2 (&otherQuad)[4])
{

	// true if [rectPt][quadPt] has quad point to the left of the rectangle line
	// i.e. - point could be "IN"
	// points on lines are considered in;


	bool rectCrossQuad[4][4];	
	bool quadIn[4] = {true, true, true, true};
	int found = 0;
	int q;

	// for each quad 'q'
	for (q = 3; q >= 0; q--) {
		if (otherQuad[q].y <= planeRect.y) {rectCrossQuad[0][q] = true;}
		else {								rectCrossQuad[0][q] = false;	quadIn[q] = false;}

		if (otherQuad[q].x >= -planeRect.x) {rectCrossQuad[1][q] = true;}
		else {								rectCrossQuad[1][q] = false;	quadIn[q] = false;}

		if (otherQuad[q].y >= -planeRect.y) {rectCrossQuad[2][q] = true;}
		else {								rectCrossQuad[2][q] = false;	quadIn[q] = false;}

		if (otherQuad[q].x <= planeRect.x) {rectCrossQuad[3][q] = true;}
		else {								rectCrossQuad[3][q] = false;	quadIn[q] = false;}
	}
		
	// start with all quad points in Rect
	for (q = 3; q >= 0; q--) {
		if (quadIn[q]) {
			myOwner->loadGeoPairPointPlane( 
				bOther, 
				bPlane, 
				q, 
				otherPlaneID, 
				planeID);
			found++;
		}
	}
	if (found == 4) return found;		// all points of quad were in rect;

	Vector2 rect[4];
	rect[0] = Vector2(planeRect.x, planeRect.y);
	rect[1] = Vector2(-planeRect.x, planeRect.y);
	rect[2] = Vector2(-planeRect.x, -planeRect.y);
	rect[3] = Vector2(planeRect.x, -planeRect.y);
	bool quadCrossRect[4][4];
	bool rectIn[4] = {true, true, true, true};

	// note quad vectors are from pt 3 to pt 2, etc. - counter clockwise
	for (q = 3; q >= 0; q--) {
		Vector2 dQ(otherQuad[(q+3)%4] - otherQuad[q]);
		for (int r = 0; r < 4; r++) {
			Vector2 dR(rect[r] - otherQuad[q]);
			float cross = (dQ.x*dR.y - dQ.y*dR.x);
			if (cross >= 0.0) {				quadCrossRect[q][r] = true;}
			else {							quadCrossRect[q][r] = false;	rectIn[r] = false;}
		}
	}
		
	int noQuadPts = (found == 0);
	for (int r = 0; r < 4; r++) {
		if (rectIn[r]) {
			myOwner->loadGeoPairPointPlane(
				bPlane, 
				bOther, 
				r, 
				planeID, 
				otherPlaneID);
			found++;
		}
	}
	if (noQuadPts && (found == 4)) return found;	// all points of rect were in quad;

	// now load crossing lines
	for (int r = 0; r < 4; r++) {
		for (q = 3; q >= 0; q--) {
			if (	(rectCrossQuad[r][q] != rectCrossQuad[r][(q+3)%4])
				&&	(quadCrossRect[q][r] != quadCrossRect[q][(r+1)%4])	)
			{
				loadGeoPairEdgeEdgePlane(
					bOther, 
					bPlane, 
					myOwner->block(bOther)->faceVertexToEdge(otherPlaneID, (q+3)%4),				
					myOwner->block(bPlane)->faceVertexToEdge(planeID, r) );

				found++;
			}
		}
	}
	RBXASSERT(found <= 8);
	return found;
}

} // namespace
