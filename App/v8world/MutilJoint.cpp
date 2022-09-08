 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/MultiJoint.h"
#include "V8World/Primitive.h"
#include "V8World/Tolerance.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Connector.h"
#include "V8Kernel/Point.h"

namespace RBX {

void MultiJoint::init(int numBreaking)
{
	numBreakingConnectors = numBreaking;
	numConnector = 0;
	for (int j = 0; j < 4; j++) {
		point[j*2] = NULL;
		point[j*2+1] = NULL;
		connector[j] = NULL;
	}
}

MultiJoint::MultiJoint(int numBreaking)
{
	init(numBreaking);
}

MultiJoint::MultiJoint(
	Primitive* p0,
	Primitive* p1,
	const CoordinateFrame& jointCoord0,
	const CoordinateFrame& jointCoord1,
	int numBreaking)
	:	Joint(p0, p1, jointCoord0, jointCoord1)
{
	init(numBreaking);
}

MultiJoint::~MultiJoint() 
{
	RBXASSERT(connector[0] == NULL);
	RBXASSERT(numConnector == 0);
}

void MultiJoint::putInKernel(Kernel* _kernel)
{
	Super::putInKernel(_kernel);
}


float MultiJoint::getJointK()
{
	return G3D::min( getPrimitive(0)->getJointK(), getPrimitive(1)->getJointK() );
}

void MultiJoint::addToMultiJoint(Point* point0, Point* point1, Connector* _connector)
{
	RBXASSERT(numConnector < 4);
	
	point[numConnector*2] = point0;
	point[numConnector*2+1] = point1;
	connector[numConnector] = _connector;
	getKernel()->insertConnector(_connector);

	numConnector++;

	RBXASSERT_IF_VALIDATING(validateMultiJoint());
}

Point* MultiJoint::getPoint(int id)
{
	RBXASSERT(id < 8);
	RBXASSERT(point[id] != NULL);
	RBXASSERT(point[id]->getBody() == getPrimitive(id % 2)->getBody());
	return point[id];
}

Connector* MultiJoint::getConnector(int id)
{
	RBXASSERT(id < 4);
	RBXASSERT(connector[id] != NULL);
	return connector[id];
}

#ifdef __RBX_VALIDATE_ASSERT
bool MultiJoint::pointsAligned() const
{
	RBXASSERT(numBreakingConnectors <= numConnector);

		// THIS ASSERT WILL BLOW in the situation where "unaligned" joints
		// are present on save or sleep.  For now, just watching to see how often 
		// this occurs
		// If too often - will need to store joint information or implement wheel/holes
		//
	for (int i = 0; i < numBreakingConnectors; ++i) {
		if (Tolerance::pointsUnaligned(
							point[i*2]->getWorldPos(), 
							point[i*2+1]->getWorldPos())) {
			return false;
		}
	}
	return true;
}
#endif


bool MultiJoint::validateMultiJoint()
{
#ifdef _DEBUG
	for (int i = 0; i < numConnector; ++i) 
	for (int j = 0; j < 2; ++j)
	{{
		Point *p = point[i*2 + j];
		RBXASSERT(p->getBody() == this->getPrimitive(j)->getBody());
		
		Connector* c = connector[i];
		RBXASSERT(c->getBody((Connector::BodyIndex)j) == this->getPrimitive(j)->getBody());
	}}
#endif
	return true;
}


void MultiJoint::removeFromKernel()
{
	RBXASSERT(this->inKernel());

// TODO - unsuppress this - indicates something that will NOT rejoin!
//	RBXASSERT_IF_VALIDATING(pointsAligned());

	RBXASSERT_IF_VALIDATING(validateMultiJoint());

	for (int i = 0; i < numConnector; i++) {
		RBXASSERT(connector[i]);

		getKernel()->removeConnector(connector[i]);
		delete connector[i];
		getKernel()->deletePoint(point[i*2]);
		getKernel()->deletePoint(point[i*2+1]);

		point[i*2] = NULL;
		point[i*2+1] = NULL;
		connector[i] = NULL;
	}

	numConnector = 0;

	Super::removeFromKernel();

	RBXASSERT(!this->inKernel());
}

bool MultiJoint::isBroken() const 
{
///	int max = std::min(numConnector, numBreakingConnectors);
	RBXASSERT(numBreakingConnectors <= numConnector);
	RBXASSERT(this->inKernel());

//	ToDo:  Saving joints, otherwise a save in this configuration could cause broken joints.
//	RBXASSERT_IF_VALIDATING(pointsAligned());
	
	for (int i = 0; i < numBreakingConnectors; ++i) {
		RBXASSERT(connector[i]);
		if (connector[i]->getBroken()) {
			return true;
		}
	}
	return false;
}


} // namespace

// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag10 = 0;
    };
};
