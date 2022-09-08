/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ForceField.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "GfxBase/Adorn.h"
#include "AppDraw/Draw.h"

FASTFLAG(RenderNewParticles2Enable);

namespace RBX {

const char* const sForceField = "ForceField";

ForceField::ForceField() : cycle(0)
{
	setName("ForceField");

	if( !RBX::TaskScheduler::singleton().isCyclicExecutive() )
	{
		invertCycle = cycles()/2;
	}
	else
	{
		startTime = Time::now<Time::Fast>();
	}	
}

bool ForceField::askSetParent(const Instance* instance) const
{
	return (!Instance::fastDynamicCast<Workspace>(instance));
}

bool containsForceField(Instance* instance)
{
	for (size_t i = 0; i < instance->numChildren(); ++i)
	{
		Instance* child = instance->getChild(i);
		if (Instance::fastDynamicCast<ForceField>(child)) {
			return true;
		}
	}
	return false;
}

bool ancestorContainsForceField(Instance* instance)
{
	RBXASSERT(instance);
	if (containsForceField(instance)) {
		return true;
	}
	else {
		if (Instance* parent = instance->getParent()) {
			if (!Instance::fastDynamicCast<Workspace>(parent)) {		// bail out at workspace level to prevent going through all workspace children
				return ancestorContainsForceField(parent);
			}
		}
		return false;
	}
}

bool ForceField::partInForceField(PartInstance* part)
{
	return ancestorContainsForceField(part);
}

void renderForceField(shared_ptr<RBX::Instance> descendant, Adorn* adorn, int cycle, int invertCycle)
{
	PartInstance *part = Instance::fastDynamicCast<PartInstance>(descendant.get());
	if (part && part->getLocalTransparencyModifier() < 0.99f )  // don't render it for bricks too close to camera
	{
		int max = ForceField::cycles();
		int half = max / 2;

		int value = cycle < half ? cycle : max - cycle;
		int invertVal = invertCycle < half ? invertCycle : max - invertCycle;

		float percent =	static_cast<float>(value) / static_cast<float>(half);
		float invertPercent = static_cast<float>(invertVal) / static_cast<float>(max);

		Vector3 pos = part->calcRenderingCoordinateFrame().translation;
		adorn->setObjectToWorldMatrix(pos);
		adorn->sphere(Sphere(Vector3::zero(),part->getPartSizeXml().magnitude()),
			RBX::Color4(0,51.0f/255.0f,204.0f/255.0f,(percent * 0.6f)) * .8f);
		adorn->sphere(Sphere(Vector3::zero(),part->getPartSizeXml().magnitude()* largeSize),
			RBX::Color4(0,invertPercent,1.0f - invertPercent,((invertPercent) * 0.45f)) * .8f);
	}
}


void ForceField::render3dAdorn(Adorn* adorn) 
{
    if (FFlag::RenderNewParticles2Enable) 
    {{{ return; }}}

	Instance* parent = this->getParent();
	if(!torso.get())
		torso = shared_from(Instance::fastDynamicCast<PartInstance>(this->getParent()->findFirstChildByName2("Torso",true).get()));

	RBXASSERT(!Instance::fastDynamicCast<Workspace>(parent));

	if (!parent->fastDynamicCast<Workspace>() && torso.get())		// Second Hack
	{
		if( !RBX::TaskScheduler::singleton().isCyclicExecutive() )
		{
			cycle = (cycle + 1) % cycles();
			invertCycle = (invertCycle + 1) % cycles();
		}
		else
		{
			const Time now = Time::now<Time::Fast>();
			double offset = (now-startTime).seconds() * 60.0;
			offset -= ::floor( offset );

			int cycle = (int)(offset * cycles());
			int invertCycle = cycle + cycles()/2;
			if( invertCycle > cycles() )
			{
				invertCycle -= cycles();
			}
		}

		renderForceField(torso,adorn,cycle,invertCycle);
	}	
}


} // namespace
