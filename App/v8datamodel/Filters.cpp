/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Filters.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/PartInstance.h"
#include "v8datamodel/Accoutrement.h"
#include "V8World/Assembly.h"
#include "V8DataModel/Visit.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"
#include "V8World/Primitive.h"
#include "v8datamodel/PartCookie.h"
#include "V8DataModel/ClickDetector.h"

FASTFLAGVARIABLE(UseFixedTransparencyNonCollidableBehaviour, true)

namespace RBX {

///////////////////////////////////////////////////////////////////////////////////
//
//  AdornBillboarder raycasting filter

FilterInvisibleNonColliding::FilterInvisibleNonColliding()
{

}

HitTestFilter::Result FilterInvisibleNonColliding::filterResult(const RBX::Primitive *testMe) const
{
	const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);
	if (FFlag::UseFixedTransparencyNonCollidableBehaviour)
	{
		const ClickDetector *cd = (const_cast<PartInstance*>(testPart))->findFirstChildOfType<ClickDetector>();
		if (cd == NULL && (testPart->getTransparencyXml() > .95f) && (!testPart->getCanCollide()) )
			return HitTestFilter::IGNORE_PRIM;
	}
	else
	{
		if ((testPart->getTransparencyXml() > .95f) && (!testPart->getCanCollide()) )
			return HitTestFilter::IGNORE_PRIM;
	}
	return HitTestFilter::INCLUDE_PRIM;
}


///////////////////////////////////////////////////////////////////////////////////
//

bool Unlocked::unlocked(const Primitive* testMe) 
{
	return !PartInstance::fromConstPrimitive(testMe)->getPartLocked();
}

///////////////////////////////////////////////////////////////////////////////////
//
// Filter Character

PartByLocalCharacter::PartByLocalCharacter(Instance* root)
{
	character = shared_from(Network::Players::findLocalCharacter(root));
	head = shared_from(Humanoid::getHeadFromCharacter(character.get()));
}


HitTestFilter::Result PartByLocalCharacter::filterResult(const Primitive* testMe) const
{
	if ((character != NULL) && (head != NULL)) 
	{
		const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);

		//if (testPart->isAncestorOf(character.get()))
		if (testPart->isDescendantOf(character.get()))
		{
			return (head->getIsTransparent()) ? HitTestFilter::IGNORE_PRIM : HitTestFilter::STOP_TEST;
		}
	}
	return HitTestFilter::INCLUDE_PRIM;
}


HitTestFilter::Result UnlockedPartByLocalCharacter::filterResult(const Primitive* testMe) const
{
	HitTestFilter::Result result = PartByLocalCharacter::filterResult(testMe);
	
	if (result == HitTestFilter::INCLUDE_PRIM) {
		return Unlocked::unlocked(testMe) ? HitTestFilter::INCLUDE_PRIM : HitTestFilter::STOP_TEST;
	}
	else {
		return result;
	}
}


FilterDescendents::FilterDescendents(shared_ptr<Instance> i)
{
	inst = i;
}

HitTestFilter::Result FilterDescendents::filterResult(const RBX::Primitive *testMe) const
{
	if (inst) 
	{
		const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);
		if (testPart == inst.get() || testPart->isDescendantOf(inst.get()))
		{
			return HitTestFilter::IGNORE_PRIM;
		} 
	}
	return HitTestFilter::INCLUDE_PRIM;
}

FilterDescendentsList::FilterDescendentsList(const Instances* i)
{
	instances = i;
}

HitTestFilter::Result FilterDescendentsList::filterResult(const RBX::Primitive *testMe) const
{
	if (instances) 
	{
		const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);
		for (Instances::const_iterator ignoreIt = instances->begin(); ignoreIt != instances->end(); ++ignoreIt){
			Instance* inst = ignoreIt->get();
			if (testPart == inst || testPart->isDescendantOf(inst))
			{
				return HitTestFilter::IGNORE_PRIM;
			} 
		}
	}
	return HitTestFilter::INCLUDE_PRIM;
}

FilterCharacterOcclusion::FilterCharacterOcclusion(float headHeight)
: headHeight(headHeight)
{
}

HitTestFilter::Result FilterCharacterOcclusion::filterResult(const RBX::Primitive *testMe) const
{
	const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);

	// Don't occlude 
	// 1. "walkable" bricks
	// 2. Not colliding bricks
	// 3. Transparent bricks
	// 4. Bricks in another character
	// 5. Locked/dragging bricks

	if (	((testMe->getCoordinateFrame().translation.y < headHeight) && !testMe->hasAutoJoints() && testMe->getConstAssembly() && testMe->getConstAssembly()->getAssemblyIsMovingState())
		||	(!testPart->getCanCollide())
		||	(testPart->getTransparencyXml() > .95f)
		||  (Humanoid::constHumanoidFromBodyPart(testPart) != NULL)
		||  (testPart->getDragging())
		)
	{
		return HitTestFilter::IGNORE_PRIM;
	} 

	return HitTestFilter::INCLUDE_PRIM;
}

HitTestFilter::Result FilterHumanoidParts::filterResult(const RBX::Primitive* testMe) const
{
	const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);

	return Humanoid::constHumanoidFromBodyPart(testPart) != NULL ?
			HitTestFilter::IGNORE_PRIM : HitTestFilter::INCLUDE_PRIM;
}

FilterHumanoidNameOcclusion::FilterHumanoidNameOcclusion(shared_ptr<Humanoid> i)
{
	inst = i;
}

HitTestFilter::Result FilterHumanoidNameOcclusion::filterResult(const RBX::Primitive* testMe) const
{
	const PartInstance* testPart = PartInstance::fromConstPrimitive(testMe);
	const Humanoid *pHumanoid = Humanoid::constHumanoidFromDescendant(testPart);

	if (pHumanoid != NULL) 
	{
		if (pHumanoid == inst.get() && testPart == pHumanoid->getHeadSlow())
		{
			return HitTestFilter::INCLUDE_PRIM;
		}
		else
		{
			return HitTestFilter::IGNORE_PRIM;
		}
	} 
	else 
	{
		if (testPart->getTransparencyXml() > 0.99f)
		{
			return HitTestFilter::IGNORE_PRIM;
		}
		return HitTestFilter::INCLUDE_PRIM;
	}
}

MergedFilter::MergedFilter(const HitTestFilter *a, const HitTestFilter* b)
:aFilter(a)
,bFilter(b)
{}

HitTestFilter::Result MergedFilter::filterResult(const Primitive* testMe) const
{
	if(aFilter)
	{
		HitTestFilter::Result aResult = aFilter->filterResult(testMe);
		if(aResult != HitTestFilter::INCLUDE_PRIM)
			return aResult;
	}
	if(bFilter)
	{
		HitTestFilter::Result bResult = bFilter->filterResult(testMe);
		if(bResult != HitTestFilter::INCLUDE_PRIM)
			return bResult;
	}
	return HitTestFilter::INCLUDE_PRIM;
}

/// FILTER SAME ASSEMBLY
HitTestFilter::Result FilterSameAssembly::filterResult(const Primitive* testMe) const
{
	if (PartInstance* assemblyPrim = assemblyPart.get())
	{
		if (!testMe->getCanCollide() || (assemblyPrim->getPartPrimitive()->getAssembly()->getAssemblyPrimitive() == testMe->getConstAssembly()->getConstAssemblyPrimitive()))
		{
			return HitTestFilter::IGNORE_PRIM;
		}
	}
	return HitTestFilter::INCLUDE_PRIM;
}

} // namespace
