/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PVInstance.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/Workspace.h"
#include "Util/Math.h"
#include "Util/Action.h"
#include "rbx/Debug.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Part.h"
#include "GfxBase/Adorn.h"



namespace RBX {

const char *const sPVInstance = "PVInstance";

///////////////////////////
// legacy stuff - old offset

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<PVInstance, CoordinateFrame> desc_CoordFrame("CoordinateFrame", category_Data, NULL, &PVInstance::setPVGridOffsetLegacy, Reflection::PropertyDescriptor::UI);
REFLECTION_END();

PVInstance::PVInstance(const char* name) :
	Reflection::Described<PVInstance, sPVInstance, Instance >(name)
{
}

void PVInstance::moveToPointNoUnjoinNoJoin(Vector3 point)
{
	if (Workspace* workspace = Workspace::findWorkspace(this)) {
		workspace->moveToPoint(this, point, DRAG::NO_UNJOIN_NO_JOIN);
	}
}


void PVInstance::moveToPointAndJoin(Vector3 point)
{
	if (Workspace* workspace = Workspace::findWorkspace(this)) {
		workspace->moveToPoint(this, point, DRAG::UNJOIN_JOIN);
	}
}

void PVInstance::moveToPointNoJoin(Vector3 point)
{
	if (Workspace* workspace = Workspace::findWorkspace(this)) {
		workspace->moveToPoint(this, point, DRAG::UNJOIN_NO_JOIN);
	}
}

void PVInstance::readProperty(const XmlElement* propertyElement, IReferenceBinder& binder)
{
	
	// If the element is an Feature and the Feature does not exist, create it
	if (propertyElement->getTag()=="Feature")
	{
		const RBX::Name* name = NULL;
		bool foundNameAttribute = propertyElement->findAttributeValue(name_name, name);
		RBXASSERT(foundNameAttribute);
		if (*name=="Part")
		{
			// Read legacy PartInstance data
			this->readProperties(propertyElement, binder);
			return;
		}
		else if (*name=="Item")
		{
			// Read legacy PVAttribute data
			this->readProperties(propertyElement, binder);
			return;
		}
	}

	Super::readProperty(propertyElement, binder);
}


PVInstance::~PVInstance()
{
}


PVInstance* PVInstance::getTopLevelPVParent()
{
	if (isTopLevelPVInstance()) {						// no parent, or parent==root
		return this;
	}
	else {
		return getTypedParent<PVInstance>()->getTopLevelPVParent();
	}
}

const PVInstance* PVInstance::getTopLevelPVParent() const
{
	if (isTopLevelPVInstance()) {						// no parent, or parent==root
		return this;
	}
	else {
		return getTypedParent<PVInstance>()->getTopLevelPVParent();
	}
}

void PVInstance::renderCoordinateFrame(Adorn* adorn) 
{
	adorn->setObjectToWorldMatrix( getLocation() );
	adorn->axes(G3D::Color3::red(), G3D::Color3::green(),
		G3D::Color3::blue(), 10.0);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


	// UI versions
void PVInstance::setPVGridOffsetLegacy(const CoordinateFrame& _offset) 
{
	throw RBX::runtime_error("CoordinateFrame is not a valid member of %s", getDescriptor().name.c_str());
}




} // namespace RBX