/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "MechanismItem.h"
#include "Util/PV.h"
#include "Util/Math.h"
#include "rbx/Debug.h"

using namespace RBX;

MechanismItem::~MechanismItem()
{
	for (int i=0; i<buffer.size(); ++i)
		delete buffer[i];
}

void MechanismItem::reset(int numElements)
{
	networkHumanoidState = 0;
	hasVelocity = false;
	networkTime = 0;
	currentElements = numElements;
	while (buffer.size() < numElements)
		buffer.append(new AssemblyItem());
}

AssemblyItem& MechanismItem::appendAssembly()
{
	RBXASSERT(buffer.size() >= currentElements);

	if (buffer.size() == currentElements)
		buffer.append(new AssemblyItem());
	else
	{
		RBXASSERT(buffer[currentElements]!=NULL);
		buffer[currentElements]->reset();
	}

	AssemblyItem& answer = *buffer[currentElements];

	currentElements++;

	return answer;
}

/* 
	For now - no lerp on mechanisms (>1 assembly)
*/

bool MechanismItem::consistent(const MechanismItem* before, const MechanismItem* after)
{
	RBXASSERT(before && after);
	RBXASSERT(before->networkTime < after->networkTime);
	
	return (	(before->hasVelocity == after->hasVelocity)
			&&	(before->numAssemblies() == 1)
			&&	(after->numAssemblies() == 1)
			&&	(before->getAssemblyItem(0).motorAngles.size() == after->getAssemblyItem(0).motorAngles.size())	);
}


void MechanismItem::lerp(const MechanismItem* before, const MechanismItem* after, MechanismItem* out, float lerpAlpha)
{
	RBXASSERT(consistent(before, after));
	RBXASSERT(before->numAssemblies() == 1);

	out->reset(before->numAssemblies());	

	out->networkTime = 0;		// shouldn't be used
	out->networkHumanoidState = after->networkHumanoidState;
	out->hasVelocity = after->hasVelocity;

	AssemblyItem& beforeA = before->getAssemblyItem(0);
	AssemblyItem& afterA = after->getAssemblyItem(0);
	AssemblyItem& outA = out->getAssemblyItem(0);

	outA.rootPart = afterA.rootPart;

	outA.pv = beforeA.pv.lerp(afterA.pv, lerpAlpha);

	outA.motorAngles.resize(beforeA.motorAngles.size());		// should prevent reallocs
	for (int i = 0; i < beforeA.motorAngles.size(); ++i)
	{
		outA.motorAngles[i] = CompactCFrame(
			beforeA.motorAngles[i].translation.lerp(afterA.motorAngles[i].translation, lerpAlpha),
			beforeA.motorAngles[i].getAxisAngle().lerp(afterA.motorAngles[i].getAxisAngle(), lerpAlpha));
	}
}
