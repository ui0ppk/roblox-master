/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

// TODO - move to datamodel, out of UTIL

#pragma once

#include "Util/G3DCore.h"
#include "Util/IHasLocation.h"
#include <vector>

namespace RBX {

	class Primitive;
	class Camera;

	class RBXBaseClass CameraSubject : public virtual IHasLocation
	{
	public:
		CameraSubject() {}

		virtual ~CameraSubject() {}

		// Old Camera Subject Stuff
		/*implement*/ virtual void onCameraHeartbeat(const Vector3& cameraLocation, const Vector3& focusPoint) {}
		/*implement*/ virtual const CoordinateFrame getRenderLocation() = 0;		// goes to the rendering location, not the regular location
		/*implement*/ virtual const Vector3 getRenderSize() = 0;
		/*implement*/ virtual void onCameraNear(float distance) {}
		/*implement*/ virtual void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives) {}
		/*implement*/ virtual void getSelectionIgnorePrimitives(std::vector<const Primitive*>& primitives) {}
		/*implement*/ virtual void stepRotationalVelocity(Vector3& cameraLocation, Vector3& focusLocation) {}

	protected:
		class ContactManager* getContactManager();
	};
} // namespace
