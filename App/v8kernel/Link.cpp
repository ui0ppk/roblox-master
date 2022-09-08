/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Kernel/Link.h"
#include "V8Kernel/Body.h"

namespace RBX {


Link::Link()
: body(NULL)
, stateIndex(Body::getNextStateIndex())
{
}

Link::~Link()
{
}

void Link::dirty()
{	
	if (body)
	{
		RBXASSERT(body->getLink() == this);
		body->makeStateDirty();
	}
}

const CoordinateFrame& Link::getChildInParent() 
{
	unsigned int parentState = body->getParent()->getStateIndex();

	if (stateIndex != parentState)
	{
		computeChildInParent(childInParent);
		stateIndex = parentState;
	}
	return childInParent;
}

void Link::reset(
	const CoordinateFrame& parentC,
	const CoordinateFrame& childC)
{
	parentCoord = parentC;
	childCoord = childC;
	childCoordInverse = childC.inverse();
}


void RevoluteLink::computeChildInParent(CoordinateFrame& answer) const
{
	CoordinateFrame rotatedParentCoord(	Math::rotateAboutZ(parentCoord.rotation, jointAngle),
										parentCoord.translation);
	
	CoordinateFrame::mul(rotatedParentCoord, childCoordInverse, answer);
}



void D6Link::computeChildInParent(CoordinateFrame& answer) const
{
	CoordinateFrame rotatedParentCoord = parentCoord * offsetCFrame;
	
	CoordinateFrame::mul(rotatedParentCoord, childCoordInverse, answer);
}


} // namespace

