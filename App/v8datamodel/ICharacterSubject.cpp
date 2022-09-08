/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ICharacterSubject.h"

#include "boost/foreach.hpp"

#include "V8DataModel/Camera.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/TextButton.h"
#include "V8DataModel/ScreenGui.h"
#include "v8datamodel/UserInputService.h"
#include "V8World/ContactManager.h"
#include "V8World/World.h"
#include "Util/UserInputBase.h"

#include "Humanoid/Humanoid.h"

FASTFLAG(UserAllCamerasInLua)

namespace RBX {

const float ICharacterSubject::maxMouseLockOffset = 1.5f;

ICharacterSubject::ICharacterSubject()
: requestedDistance(10.0f)
, mouseLockOffset(maxMouseLockOffset)
, cameraTransitioning(false)
, cameraMode(RBX::Camera::CAMERAMODE_CLASSIC)
, minDistance(0.0f)
, maxDistance(RBX::Camera::distanceMaxCharacter())
{
}

int ICharacterSubject::getControlMode() const
{
	return RBX::GameBasicSettings::singleton().getControlMode();
}

int ICharacterSubject::getCustomCameraMode() const
{
	return RBX::GameBasicSettings::singleton().getCameraModeWithDefault();
}

bool ICharacterSubject::isFirstPerson() const	
{
	return requestedDistance < firstPersonCutoff(); 
}

bool ICharacterSubject::isDistanceFirstPerson(float distance) const
{
	return distance < firstPersonCutoff(); 
}

void ICharacterSubject::stepRotationalVelocity(Vector3& cameraLocation, Vector3& focusLocation)
{
	if (RBX::GameBasicSettings::singleton().getRotationType() == RBX::GameBasicSettings::ROTATION_TYPE_CAMERA_RELATIVE)
	{
		setFirstPersonRotationalVelocity(focusLocation - cameraLocation, true);
	}
}

void ICharacterSubject::onCameraHeartbeat(const Vector3& cameraLocation, const Vector3& focusPoint)
{
    if (FFlag::UserAllCamerasInLua)
    {
        return;
    }
	RBX::GameBasicSettings& settings(RBX::GameBasicSettings::singleton());

	tellCameraNear((focusPoint - cameraLocation).magnitude());

	if( settings.mouseLockedInMouseLockMode())
		tellCursorOver(mouseLockOffset/maxMouseLockOffset);

	// Ugly
	Instance* i = dynamic_cast<Instance*>(this);
	if (Workspace* workspace = ServiceProvider::find<Workspace>(i))
	{
		workspace->requestFirstPersonCamera(settings.mouseLockedInMouseLockMode() || isFirstPerson(),
			isTransitioning(), getControlMode());
	}

	if ((isFirstPerson() && !cameraTransitioning)
		|| settings.mouseLockedInMouseLockMode()
		|| settings.camLockedInCamLockMode())
	{
		// Rotate guy on camera pan
		setFirstPersonRotationalVelocity(focusPoint - cameraLocation, true);
	}
	else if (!isFirstPerson() || cameraTransitioning) 
	{
		// don't rotate guy
		setFirstPersonRotationalVelocity(Vector3::zero(), false);
	}
}

void ICharacterSubject::setCameraMode(RBX::Camera::CameraMode value)
{
	if(value != cameraMode)
		if(value == RBX::Camera::CAMERAMODE_CLASSIC)
			requestedDistance = 11.0f;

	cameraMode = value;
}

void ICharacterSubject::setMinDistance(float value)
{
	if (value != minDistance)
	{
		if (value > maxDistance)
			value = maxDistance;

		if (value < Camera::distanceMin())
			value = Camera::distanceMin();

		minDistance = value;
	}
}

void ICharacterSubject::setMaxDistance(float value)
{
	if (value != maxDistance)
	{
		if (value < minDistance)
			value = minDistance;

		if (value > Camera::distanceMaxCharacter())
			value = Camera::distanceMaxCharacter();

		maxDistance = value;
	}
}

} // namespace RBX
