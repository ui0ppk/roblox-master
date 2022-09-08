/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/IEquipable.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "rbx/Debug.h"


namespace RBX {

IEquipable::IEquipable()
{
}

IEquipable::~IEquipable()
{
	RBXASSERT(!weld);
}

void IEquipable::buildWeld(PartInstance* humanoidPart, 
						   PartInstance* gadgetPart,
						   const CoordinateFrame& humanoidCoord,
						   const CoordinateFrame& gadgetCoord,
						   const std::string& name)
{
	// Move the gadget to line up with the humanoid
	CoordinateFrame attachmentCoordWorld = humanoidPart->getCoordinateFrame() * humanoidCoord;
	CoordinateFrame gadgetCoordWorld = attachmentCoordWorld * gadgetCoord.inverse();

//	gadgetPart->setCoordinateFrame(gadgetCoordWorld);

	// Build the weld
	if (weld.get()) {
		RBXASSERT_FISHING(0);
		weld->setParent(NULL);
		weld.reset();
	}

	weld = Creatable<Instance>::create<Weld>();

	weld->setName(name);
	weld->setPart0(humanoidPart);
	weld->setPart1(gadgetPart);

	weld->setC0(humanoidCoord);
	weld->setC1(gadgetCoord);
	weld->setParent(humanoidPart);
}

} // namespace