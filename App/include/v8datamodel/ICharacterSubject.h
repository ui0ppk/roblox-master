/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/CameraSubject.h"
#include "Util/Velocity.h"
#include "V8DataModel/Camera.h"

namespace RBX {

	class ContactManager;
	class Camera;

	class RBXInterface ICharacterSubject : public CameraSubject
	{
	public:
		ICharacterSubject();

		bool isFirstPerson() const;
		bool isDistanceFirstPerson(float distance) const;
		bool isTransitioning() const { return cameraTransitioning; }

		int getControlMode() const;
		int getCustomCameraMode() const;

		RBX::Camera::CameraMode getCameraMode() const { return cameraMode; };
		void setCameraMode (RBX::Camera::CameraMode value);

		float getMinDistance() const { return minDistance; } 
		void setMinDistance(float value);

		float getMaxDistance() const { return maxDistance; } 
		void setMaxDistance(float value);

		float firstPersonCutoff() const {return 4.5f;}

		/*implement*/ virtual void tellCameraSubjectDidChange(shared_ptr<Instance> oldSubject, shared_ptr<Instance> newSubject) const = 0;

		/*override*/ void stepRotationalVelocity(Vector3& cameraLocation, Vector3& focusLocation);

	protected:
		/*implement*/ virtual void tellCameraNear(float distance) const = 0;
		/*implement*/ virtual void tellCursorOver(float cursorOffset) const = 0;
		/*implement*/ virtual void setFirstPersonRotationalVelocity(const Vector3& desiredLook, bool firstPersonOn) = 0;
		/*implement*/ virtual float getYAxisRotationalVelocity() const = 0;
		/*implement*/ virtual Velocity calcDesiredWalkVelocity() const = 0; // used for camera control
		/*implement*/ virtual bool hasFocusCoord() const = 0;		// Super hacky - clean all this up

	private:
		float maxOutPerSecond() const {return 10.0f;}
		float maxZoomOutDistance() const {return 26.0f;} // not being used currently, much shorter zoom than current model

		////////////////////////////////////////////////////
		//
		// CameraSubject
	
		/*override*/ void onCameraHeartbeat(const Vector3& cameraLocation, const Vector3& focusPoint);

		// data members
		static const float maxMouseLockOffset;

		float requestedDistance;
		float mouseLockOffset;

		bool cameraTransitioning;

		RBX::Camera::CameraMode cameraMode;

		float minDistance;
		float maxDistance;
	};

}	// namespace RBX
