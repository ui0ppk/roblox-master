/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/HandlesBase.h"
#include "V8World/Primitive.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/MouseCommand.h"
#include "Network/Players.h"
#include "AppDraw/DrawAdorn.h"
#include "Util/HitTest.h"

namespace RBX {

const char* const  sHandlesBase = "HandlesBase";

HandlesBase::HandlesBase(const char* name)
	: DescribedNonCreatable<HandlesBase, PartAdornment, sHandlesBase>(name)
	, mouseOver(NORM_UNDEFINED)
	, serverGuiObject(false)
{
}

bool HandlesBase::findTargetHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& hitNormalId)
{
	if (PartInstance* part = adornee.lock().get()) {
		Workspace *workspace = Workspace::findWorkspace(this);
		if(!workspace) return false;

		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject,workspace));
		if (HandleHitTest::hitTestHandleLocal(	part->getPartPrimitive()->getExtentsLocal(),
												part->getLocation(),
												getHandleType(),
												gridRay, 
												hitPointWorld, 
												hitNormalId,
												getHandlesNormalIdMask()))
		{
			return true;
		}
	}
	return false;
}

bool HandlesBase::getDistanceFromHandle(const shared_ptr<InputObject>& inputObject, NormalId localNormalId, const Vector3& hitPointWorld, float& distance)
{
	if (PartInstance* part = adornee.lock().get()) {
		Workspace *workspace = Workspace::findWorkspace(this);
		if(!workspace) return false;

		Vector3 worldNormal = Math::getWorldNormal(localNormalId, part->getLocation());
		RbxRay axisRay = RBX::RbxRay::fromOriginAndDirection(hitPointWorld, worldNormal);
		RbxRay gridRay = MouseCommand::getUnitMouseRay(inputObject,workspace);
		Vector3 closePoint = RBX::Math::closestPointOnRay(axisRay, gridRay);
		Vector3 handleToCurrent = closePoint - hitPointWorld;
		distance = axisRay.direction().dot(handleToCurrent);
		return true;
	}

	return false;
}

bool HandlesBase::getFacePosFromHandle(const shared_ptr<InputObject>& inputObject, NormalId faceId, const Vector3& hitPointWorld, Vector2& relativePos, Vector2& absolutePos)
{
	if (PartInstance* part = adornee.lock().get()) {
		Workspace *workspace = Workspace::findWorkspace(this);
		if(!workspace) return false;

		Vector3 worldNormal = Math::getWorldNormal(faceId, part->getLocation());
		Plane facePlaneWorld(worldNormal, hitPointWorld);
		RbxRay gridRay = MouseCommand::getUnitMouseRay(inputObject,workspace);
		Vector3 intersectWorld = gridRay.intersectionPlane(facePlaneWorld);		// note - subtle change made to intersection with plane code 2/15/10 DB
		if(!intersectWorld.isFinite())
		{
			return false;
		}
		
		Vector3 intersectObject = part->getLocation().pointToObjectSpace(intersectWorld);
		Vector3 intersectFace = objectToUvw(intersectObject, faceId);

		Vector3 hitPointObject = part->getLocation().pointToObjectSpace(hitPointWorld);
		Vector3 hitPointFace = objectToUvw(hitPointObject, faceId);

		relativePos = (intersectFace - hitPointFace).xy();
		absolutePos = intersectFace.xy();

		return true;
	}

	return false;
}

bool HandlesBase::getAngleRadiusFromHandle(const shared_ptr<InputObject>& inputObject, NormalId faceId, const Vector3& hitPointWorld, float& angle, float& radius, float& absangle, float& absradius)
{
	Vector2 relpos;
	Vector2 abspos;
	if(getFacePosFromHandle(inputObject, faceId, hitPointWorld, relpos, abspos))
	{
		Vector2 origpos = abspos-relpos;	

		absradius = abspos.length();
		absangle = 0;
		if(std::abs(absradius) > 0.00001)
		{
			absangle = atan2(abspos.y, abspos.x);
		}

		float origradius = origpos.length();
		float origangle = 0;
		if(std::abs(origradius) > 0.00001)
		{
			origangle = atan2(origpos.y, origpos.x);
		}

		angle = absangle-origangle;
		radius = absradius-origradius;

		return true;
	}

	return false;
}
bool HandlesBase::canProcessMeAndDescendants() const 
{
	return (adornee.lock()) && (getHandlesNormalIdMask() != NORM_NONE_MASK) && getVisible();
}

void HandlesBase::render2d(Adorn* adorn)
{
	Workspace* workspace = Workspace::findWorkspace(this);
	shared_ptr<PartInstance> part = adornee.lock();

	if(part){
		DrawAdorn::handles2d(
			part->getPartSizeXml(),
			part->getCoordinateFrame(),
			*(workspace->getConstCamera()),
			adorn, 
			getHandleType(),
			color,
            false,
			getHandlesNormalIdMask());
	}
}

void HandlesBase::render3dAdorn(Adorn* adorn)
{
	Workspace* workspace = Workspace::findWorkspace(this);
	shared_ptr<PartInstance> part = adornee.lock();
    NormalId highlightedNormalId = mouseOver;
    
    if (mouseDownCaptureInfo &&
        mouseDownCaptureInfo->hitNormalId != NORM_UNDEFINED)
    {
        highlightedNormalId = mouseDownCaptureInfo->hitNormalId;
    }

	if(part){
		RBX::DrawAdorn::handles3d(
			part->getPartSizeXml(),
			part->getCoordinateFrame(),
			adorn, 
			getHandleType(),
			workspace->getConstCamera()->coordinateFrame().translation,
			color,
            getHandleType() == HANDLE_ROTATE,
			getHandlesNormalIdMask(),
            highlightedNormalId);
	}
}

void HandlesBase::setServerGuiObject() 
{	
	serverGuiObject = true;
}

void HandlesBase::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	if(!serverGuiObject && Network::Players::serverIsPresent(this,false)){
		setServerGuiObject();
	}
}

}
