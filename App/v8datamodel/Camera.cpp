#include "stdafx.h"

#include "V8DataModel/Camera.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/PartInstance.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/UserInputService.h"
#include "Tool/ToolsArrow.h" // only needed for STUDIO_CAMERA_CONTROL_SHORTCUTS
#include "Humanoid/Humanoid.h"
#include "V8World/ContactManager.h"
#include "V8World/World.h"
#include "V8World/Tolerance.h"
#include "V8Kernel/Constants.h"
#include "Util/Math.h"
#include "Util/CameraSubject.h"
#include "V8DataModel/ICharacterSubject.h"
#include "Util/NavKeys.h"
#include "Network/Players.h"
#include "FastLog.h"
#include "v8datamodel/UserController.h"
#include "Util/UserInputBase.h"
#include "v8datamodel/GameBasicSettings.h"
#include "FastLog.h"

#include <algorithm>

FASTFLAG(FlyCamOnRenderStep)
FASTFLAG(UserBetterInertialScrolling)
FASTFLAGVARIABLE(UserAllCamerasInLua, false)
FASTFLAGVARIABLE(CameraInterpolateMethodEnhancement, true)

FASTFLAGVARIABLE(CameraVR, true)

namespace RBX {

const char* const  sCamera = "Camera";

const char *const category_Camera = "Camera";

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<Camera, Camera::CameraType> desc_cameraType("CameraType", category_Camera, &Camera::getCameraType, &Camera::setCameraType);

static Reflection::BoundFuncDesc<Camera, RBX::RbxRay(float,float,float)> func_viewportToWorldRay(&Camera::worldRayViewportLua, "ViewportPointToRay", "x","y","depth",0, Security::None);
static Reflection::BoundFuncDesc<Camera, RBX::RbxRay(float,float,float)> func_screenToWorldRay(&Camera::worldRayLua, "ScreenPointToRay", "x","y","depth",0, Security::None);

static Reflection::BoundFuncDesc<Camera, shared_ptr<const Reflection::Tuple>(Vector3)> func_worldToViewportPoint(&Camera::projectViewportLua, "WorldToViewportPoint", "worldPoint", Security::None);
static Reflection::BoundFuncDesc<Camera, shared_ptr<const Reflection::Tuple>(Vector3)> func_worldToScreenPoint(&Camera::projectLua, "WorldToScreenPoint", "worldPoint", Security::None);

static Reflection::PropDescriptor<Camera, Vector2> desc_viewport("ViewportSize", category_Data, &Camera::getViewport, NULL);

static Reflection::PropDescriptor<Camera, CoordinateFrame> desc_CFrame("CFrame", category_Data, &Camera::getCameraCoordinateFrame, &Camera::setCameraCoordinateFrame);
static Reflection::PropDescriptor<Camera, CoordinateFrame> desc_CoordFrame("CoordinateFrame", category_Data, &Camera::getCameraCoordinateFrame, &Camera::setCameraCoordinateFrame, Reflection::PropertyDescriptor::Attributes::deprecated(desc_CFrame, Reflection::PropertyDescriptor::LEGACY_SCRIPTING));
static Reflection::PropDescriptor<Camera, CoordinateFrame> desc_Focus("Focus", category_Data, &Camera::getCameraFocus, &Camera::setCameraFocus);
static Reflection::PropDescriptor<Camera, CoordinateFrame> desc_focus("focus", category_Data, &Camera::getCameraFocus, &Camera::setCameraFocus, Reflection::PropertyDescriptor::Attributes::deprecated(desc_Focus));
static Reflection::PropDescriptor<Camera, float> desc_FieldOfView("FieldOfView", category_Data, &Camera::getFieldOfViewDegrees, &Camera::setFieldOfViewDegrees);

static Reflection::RefPropDescriptor<Camera, Instance> cameraSubjectProp("CameraSubject", category_Camera, &Camera::getCameraSubjectInstanceDangerous, &Camera::setCameraSubject);

static Reflection::BoundFuncDesc<Camera, void(float)> func_setroll(&Camera::setRoll, "SetRoll", "rollAngle", Security::None);
static Reflection::BoundFuncDesc<Camera, float(void)> func_getroll(&Camera::getRollSlow, "GetRoll", Security::None);

static Reflection::BoundFuncDesc<Camera, float(void)> func_getTiltSpeed(&Camera::getTiltSpeed, "GetTiltSpeed", Security::None);
static Reflection::BoundFuncDesc<Camera, float(void)> func_getPanSpeed(&Camera::getPanSpeed, "GetPanSpeed", Security::None);

static Reflection::BoundFuncDesc<Camera, void(Camera::CameraPanMode)> func_setCameraPanMode(&Camera::setCameraPanMode, "SetCameraPanMode", "mode", Camera::CAMERAPANMODE_CLASSIC, Security::None);
static Reflection::BoundFuncDesc<Camera, bool(float)> func_zoom(&Camera::zoom, "Zoom", "distance", Security::RobloxScript);

static Reflection::BoundFuncDesc<Camera, void(int)> func_panUnits(&Camera::panUnits, "PanUnits", "units", Security::None);
static Reflection::BoundFuncDesc<Camera, bool(int)> func_tiltUnits(&Camera::tiltUnits, "TiltUnits", "units", Security::None);

static Reflection::BoundFuncDesc<Camera, void(CoordinateFrame, CoordinateFrame, float)> func_interpolateCamera(&Camera::beginCameraInterpolation, "Interpolate", "endPos", "endFocus", "duration", Security::None);
static Reflection::EventDesc<Camera, void()> event_doneInterpolating(&Camera::interpolationFinishedSignal, "InterpolationFinished");

static Reflection::EventDesc<Camera, void(bool)> event_firstPersonTransition(&Camera::firstPersonTransitionSignal, "FirstPersonTransition", "entering", Security::RobloxPlace);

static Reflection::PropDescriptor<Camera, bool> desc_HeadLocked("HeadLocked", category_Data, &Camera::getHeadLocked, &Camera::setHeadLocked);
static Reflection::BoundFuncDesc<Camera, CoordinateFrame()> func_GetRenderCFrame(&Camera::getRenderingCoordinateFrameLua, "GetRenderCFrame", Security::None);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<Camera::CameraType>::EnumDesc()
	:EnumDescriptor("CameraType")
{
	addPair(Camera::FIXED_CAMERA, "Fixed");
	addPair(Camera::WATCH_CAMERA, "Watch");
	addPair(Camera::ATTACH_CAMERA, "Attach");
	addPair(Camera::TRACK_CAMERA, "Track");
	addPair(Camera::FOLLOW_CAMERA, "Follow");
	addPair(Camera::CUSTOM_CAMERA, "Custom");
	addPair(Camera::LOCKED_CAMERA, "Scriptable");
}

template<>
EnumDesc<Camera::CameraMode>::EnumDesc()
:EnumDescriptor("CameraMode")
{
	addPair(Camera::CAMERAMODE_CLASSIC,                  "Classic");
	addPair(Camera::CAMERAMODE_LOCKFIRSTPERSON,          "LockFirstPerson");
}

template<>
EnumDesc<RBX::Camera::CameraPanMode>::EnumDesc()
:EnumDescriptor("CameraPanMode")
{
	addPair(RBX::Camera::CAMERAPANMODE_CLASSIC,	"Classic");
	addPair(RBX::Camera::CAMERAPANMODE_EDGEBUMP, "EdgeBump");
}

template<>
RBX::Camera::CameraPanMode& Variant::convert<RBX::Camera::CameraPanMode>(void)
{
	return genericConvert<RBX::Camera::CameraPanMode>();
}
}//namespace Reflection

template<>
bool RBX::StringConverter<RBX::Camera::CameraPanMode>::convertToValue(const std::string& text, RBX::Camera::CameraPanMode& value)
{
	if(text.find("Classic") != std::string::npos){
		value = RBX::Camera::CAMERAPANMODE_CLASSIC;
		return true;
	}
	if(text.find("EdgeBump") != std::string::npos){
		value = RBX::Camera::CAMERAPANMODE_EDGEBUMP;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

static const float defaultFieldOfView = G3D::toRadians(70.0f);

float Camera::CameraKeyMoveFactor = 1.5f;
float Camera::CameraMouseWheelMoveFactor = 15.0f;
float Camera::CameraShiftKeyMoveFactor = .2f;

Camera::Camera() : 
	camInterpolation(CAM_INTERPOLATION_NONE),
	interpolationDuration(0.f),
	interpolationTime(-1.f),
	cameraType(Camera::FIXED_CAMERA),
	cameraFocus(Vector3(0.0f, 0.0f, -5.0f)),
	fieldOfView(defaultFieldOfView),
	roll(0.0f),
	panSpeed(0.0f),
	tiltSpeed(0.0f),
	cameraPanMode(Camera::CAMERAPANMODE_CLASSIC),
	imagePlaneDepth(1.0f / (2.0f * tanf(defaultFieldOfView / 2.0f))),
	cameraHistoryStack(),
	currentCameraHistoryPosition(-1),
	lastHistoryPushTime(0),
	hasFocalObject(false),
	viewport(Vector2(0,0)),
	headLocked(true)
{
	setName("Camera");
	CoordinateFrame defaultC(Vector3(0.0f, 20.0f, 20.0f));
	defaultC.lookAt(Vector3::zero());

	cameraCoord = defaultC;
}

bool Camera::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<Workspace>(instance)!=NULL;
}

// static
float Camera::getNewZoomDistance(float currentDistance, float in)
{
	static const float ZOOM_FACTOR = 0.25f;						// unitless
	float answer;

	if (in > 0.0f) {
		answer = std::max((currentDistance / (1.0f + ZOOM_FACTOR*in)), Camera::distanceMin());
	}
	else if (in < 0.0f) {
		answer = std::min((currentDistance * (1.0f - ZOOM_FACTOR*in)), Camera::distanceMax());
	}
	else {
		answer = currentDistance;
	}
	return answer;
}



bool Camera::isCharacterCamera() const
{
	return	(	
				(cameraSubject.get() != NULL)
				&&	((cameraType == Camera::FOLLOW_CAMERA) || (cameraType == Camera::ATTACH_CAMERA) || 
				     (cameraType == Camera::TRACK_CAMERA) || (cameraType == Camera::CUSTOM_CAMERA))
			);
}

bool Camera::isFirstPersonCamera() const
{
	if(ICharacterSubject* charSubject = dynamic_cast<ICharacterSubject*>(cameraSubject.get()))
		return charSubject->isFirstPerson();
	return false;
}

bool Camera::isPartVisibleFast(const PartInstance& part, const ContactManager& contactManager, const HitTestFilter* filter) const
{
	Vector3 hitPoint;
	std::vector<const Primitive*> ignorePrims;

	CoordinateFrame cframe = FFlag::CameraVR ? getRenderingCoordinateFrame() : cameraCoord;

	Vector3 direction = (part.getCoordinateFrame().translation - cframe.translation) * 2;
	RbxRay ray = RbxRay::fromOriginAndDirection(cframe.translation, direction);
	if (Primitive* hitPrim = contactManager.getHit(ray, &ignorePrims, filter, hitPoint) )
		return hitPrim == part.getConstPartPrimitive();

	return false;
}

bool Camera::isPartInFrustum(const PartInstance& part) const
{
	RBX::Frustum fr( frustum() );
	if(!part.containedByFrustum(fr))
		return false;

	return true;
}

bool Camera::isLockedToFirstPerson() const
{
	if(ICharacterSubject* charSubject = dynamic_cast<ICharacterSubject*>(cameraSubject.get()))
			return charSubject->getCameraMode() == CAMERAMODE_LOCKFIRSTPERSON;
	return false;
}

void Camera::onHeartbeat(const Heartbeat& event)
{
	if (interpolationTime >= interpolationDuration)
        signalInterpolationDone();
		
	CameraSubject* subject = getCameraSubject();
	if (subject != NULL && cameraType != Camera::LOCKED_CAMERA && cameraType != Camera::CUSTOM_CAMERA)
		subject->onCameraHeartbeat(cameraCoord.translation, cameraFocus.translation);
	else if(RBX::GameBasicSettings::singleton().inStudioMode()) // only do camera interpolation if we are currently using studio
    {
		if( (cameraCoordGoal != cameraCoord) && (cameraFocus != cameraFocusGoal) && camInterpolation == CAM_INTERPOLATION_CONSTANT_SPEED )
        {
			fixedSpeedInterpolateCamera(event.wallStep);
        }
        else if (FFlag::CameraInterpolateMethodEnhancement && camInterpolation != CAM_INTERPOLATION_CONSTANT_SPEED)
            signalInterpolationDone();
    }
}

void Camera::signalInterpolationDone()
{
    camInterpolation = CAM_INTERPOLATION_NONE;
    interpolationTime = -1.f;
    interpolationFinishedSignal();
}

ICameraOwner* Camera::getCameraOwner()
{
	Instance* parent = this;
	while ((parent = parent->getParent())) {
		if (ICameraOwner* owner = dynamic_cast<ICameraOwner*>(parent)) {
			return owner;
		}
	}
	return NULL;
}
// this method will push the current camera data onto a stack, so we can have a camera history! (this will only work in studio with [] keys)
void Camera::pushCameraHistoryStack() 
{
	if(RBX::Time::nowFastSec() - lastHistoryPushTime < 0.5f) // don't update history too fast
		return;

	std::pair<CoordinateFrame,CoordinateFrame> newPair(cameraCoord,cameraFocus);
	if(currentCameraHistoryPosition >= 0 && (unsigned)currentCameraHistoryPosition < cameraHistoryStack.size() &&
		cameraHistoryStack.at(currentCameraHistoryPosition) == newPair) // don't want to push the same thing in history twice
		return;

	lastHistoryPushTime = RBX::Time::nowFastSec();

	if(currentCameraHistoryPosition >= 0 )
	{
		currentCameraHistoryPosition++;
		cameraHistoryStack.insert(cameraHistoryStack.begin() + currentCameraHistoryPosition,newPair);
	}
	else
	{
		currentCameraHistoryPosition = 0;
		cameraHistoryStack.push_back(newPair);
	}


	if(cameraHistoryStack.size() > 50) // can't maintain all history forever, need to remove oldest
	{
		cameraHistoryStack.erase(cameraHistoryStack.begin() + cameraHistoryStack.size() - 1);
		currentCameraHistoryPosition--;
	}
}

std::pair<CoordinateFrame,CoordinateFrame> Camera::popCameraHistoryStack(bool backward)
{
	if(cameraHistoryStack.size() > 0)
	{
		std::pair<CoordinateFrame,CoordinateFrame> cameraFrameReturn;
		if(currentCameraHistoryPosition < 0)
		{
			cameraFrameReturn = cameraHistoryStack.back();
			currentCameraHistoryPosition = cameraHistoryStack.size() - 1;
		}
		else
		{
			if(backward && currentCameraHistoryPosition > 0)
				currentCameraHistoryPosition--;
			else if(!backward && (unsigned)currentCameraHistoryPosition < (cameraHistoryStack.size() - 1) )
				currentCameraHistoryPosition++;

			cameraFrameReturn = cameraHistoryStack.at(currentCameraHistoryPosition);
		}
		return cameraFrameReturn;
	}
	return std::pair<CoordinateFrame,CoordinateFrame>(CoordinateFrame(),CoordinateFrame());
}

void Camera::stepCameraHistoryForward()
{
	if(isCharacterCamera())
		return;

	std::pair<CoordinateFrame,CoordinateFrame> newerCameraData = popCameraHistoryStack(false);
	if(newerCameraData != std::pair<CoordinateFrame,CoordinateFrame>(CoordinateFrame(),CoordinateFrame()))
	{
		setCameraCoordinateFrame(newerCameraData.first);
		setCameraFocus(newerCameraData.second);
	}
}
	
void Camera::stepCameraHistoryBackward()
{
	if(isCharacterCamera())
		return;

	std::pair<CoordinateFrame,CoordinateFrame> olderCameraData = popCameraHistoryStack(true);
	if(olderCameraData != std::pair<CoordinateFrame,CoordinateFrame>(CoordinateFrame(),CoordinateFrame()))
	{
		setCameraCoordinateFrame(olderCameraData.first);
		setCameraFocus(olderCameraData.second);
	}
}


void Camera::updateFocus()
{
	if (cameraSubject.get())
	{
		setCameraFocusWithoutPropertyChange( getCameraSubject()->getRenderLocation() );
	}
}

// when camera goals are different than the actual camera, this function will attempt to
// move the camera smoothly thru the space

// TODO: Expose this somehow to lua
// moves the camera from one position to another at a constant rate, not over a constant time period
void Camera::fixedSpeedInterpolateCamera(double elapsedTime)
{
	RBXASSERT(camInterpolation == CAM_INTERPOLATION_CONSTANT_SPEED);

	double percentOfDist = elapsedTime * interpolationSpeed();

	if((cameraCoordGoal.translation - cameraCoord.translation).magnitude() > 0.1)
	{
		if(percentOfDist < 1.0f)
		{
			Vector3 cameraCoordDiff = cameraCoordGoal.translation - cameraCoord.translation;
			cameraCoord.translation += (cameraCoordDiff * percentOfDist);
		}
		else
			setCameraCoordinateFrame(cameraCoordGoal);
	}
	else if(cameraCoord != cameraCoordGoal)
		setCameraCoordinateFrame(cameraCoordGoal);

	if((cameraFocusGoal.translation - cameraFocus.translation).magnitude() > 0.1)
	{
		if(percentOfDist < 1.0f)
		{
			Vector3 cameraFocusDiff = cameraFocusGoal.translation - cameraFocus.translation;
			cameraFocus.translation += (cameraFocusDiff * percentOfDist);
		}
		else
			setCameraFocusOnly(cameraFocusGoal);
	}
	else if(cameraFocus != cameraFocusGoal)
		setCameraFocusOnly(cameraFocusGoal);
}

// begin moving the camera from the current coordinates to endPos and focused on endFocus while taking duration seconds to move there
void Camera::beginCameraInterpolation(CoordinateFrame endPos, CoordinateFrame endFocus, float duration)
{
    if (FFlag::CameraInterpolateMethodEnhancement)
    {
        RBXASSERT(duration >= 0.f);
        RBXASSERT(cameraType == Camera::LOCKED_CAMERA ||
                  GameBasicSettings::singleton().inStudioMode());	// camera must be scriptable, if not used from Studio
        
        if (duration < 0.f)
        {
            throw std::runtime_error("Interpolation time must be positive or 0.");
        }
        if (cameraType != Camera::LOCKED_CAMERA && !GameBasicSettings::singleton().inStudioMode())
        {
            throw std::runtime_error("Attempted to use interpolation with a camera mode other than scriptable.");
        }
        
        if (duration > 0.f)
        {
            camInterpolation = CAM_INTERPOLATION_CONSTANT_TIME;
            interpolationDuration = duration;
            interpolationTime = 0.f;
        }
        else
        {
            camInterpolation = CAM_INTERPOLATION_CONSTANT_SPEED;
        }
        
        if (endPos != cameraCoord)
        {
            cameraCoordGoal = endPos;
            cameraFocusGoal = endFocus;
            cameraCoordPrev = cameraCoord;
            cameraFocusPrev = cameraFocus;
            cameraUpDirPrev = cameraCoord.upVector();
        }
        else
        {
            lookAt(endFocus.translation, true);
        }
    }
    else
    {
        RBXASSERT(duration > 0.f);
        RBXASSERT(cameraType == Camera::LOCKED_CAMERA);	// camera must be scriptable
        
        if (duration <= 0.f)
        {
            throw std::runtime_error("Interpolation time must be positive.");
        }
        if (cameraType != Camera::LOCKED_CAMERA)
        {
            throw std::runtime_error("Attempted to use interpolation with a camera mode other than scriptable.");
        }
        
        camInterpolation = CAM_INTERPOLATION_CONSTANT_TIME;
        interpolationDuration = duration;
        interpolationTime = 0.f;
        
        cameraCoordGoal = endPos;
        cameraFocusGoal = endFocus;
        cameraCoordPrev = cameraCoord;
        cameraFocusPrev = cameraFocus;
        cameraUpDirPrev = cameraCoord.upVector();
    }

}

void Camera::step(double elapsedTime)
{
    if (FFlag::UserAllCamerasInLua && hasClientPlayer())
    {
        return;
    }

	switch (cameraType)
	{
	case Camera::LOCKED_CAMERA:
		{
			G3D::Vector3 lookDir = cameraCoord.lookVector();
			G3D::Vector3 upDir = cameraCoord.upVector();
			if (camInterpolation == CAM_INTERPOLATION_CONSTANT_TIME)
			{
				interpolationTime = interpolationTime + elapsedTime;
				float interpolationVal = std::min(interpolationTime / interpolationDuration, 1.f);
				cameraCoord.translation = cameraCoordPrev.translation * (1.f - interpolationVal) + cameraCoordGoal.translation * interpolationVal;
				G3D::Vector3 focusTranslation = cameraFocusPrev.translation * (1.f - interpolationVal) + cameraFocusGoal.translation * interpolationVal;
				G3D::Vector3 lookVec = focusTranslation - cameraCoord.translation;

				G3D::Vector3 upTranslation = cameraUpDirPrev * (1.f - interpolationVal) + cameraCoordGoal.upVector() * interpolationVal;
				upDir = upTranslation.direction();

				lookDir = lookVec.direction();

			}
			G3D::Vector3 focusCoord = cameraCoord.translation + 20.0f*lookDir;

			// We don't have DM write access here, so lets not trigger property changed
			setCameraFocusOnlyWithoutPropertyChange(focusCoord); // always look straight ahead!

			if (camInterpolation == CAM_INTERPOLATION_CONSTANT_TIME)
			{
				if (cameraCoord.translation == cameraFocus.translation)
				{
					//cameraCoord.rotation = cameraFocus.rotation; // scenario when we're panning / tilting (translation is the same but we've rotated)
					cameraCoord.lookAt(cameraCoord.lookVector(), upDir);
				}
				else // otherwise look towards our focal point
					cameraCoord.lookAt(cameraFocus.translation, upDir);
			}
			return;
		}
	case Camera::FIXED_CAMERA:
		{
			break;
		}
	case Camera::WATCH_CAMERA:
		{
			// ******** Now - update focus ************
			updateFocus();
			break;
		}
	case Camera::ATTACH_CAMERA:
		{
			Vector3 delta = cameraCoord.translation - cameraFocus.translation;
			float distance = delta.xz().length();

			// ******** Now - update focus ************
			updateFocus();

			Vector2 newDeltaXZ = -cameraFocus.lookVector().xz().direction() * distance;
			Vector3 newDelta(newDeltaXZ.x, delta.y, newDeltaXZ.y);

			setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation,cameraFocus.translation + newDelta));
			break;
		}
	case Camera::TRACK_CAMERA:	
		{
			Vector3 oldFocusPt = cameraFocus.translation;

			// ******** Now - update focus ************
			updateFocus();

			setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation,cameraCoord.translation + (cameraFocus.translation - oldFocusPt)) );
			break;
		}
	case Camera::FOLLOW_CAMERA:	
		{
			// Note - distance lags and follows, height follows immediately
			// Get the desired Y-plane distance
			Vector3 delta = cameraFocus.translation - cameraCoord.translation;
			float distance = delta.xz().length();

			// ******** Now - update focus ************
			updateFocus();

			// Get the new look vector
			const Vector2 newDxz = (cameraFocus.translation.xz() - cameraCoord.translation.xz()).direction() * distance;
			const Vector3 newDelta(newDxz.x, delta.y, newDxz.y);

			// Move towards/away from the cameraFocus
			setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation,cameraFocus.translation - newDelta) );
			break;
		}
	case Camera::CUSTOM_CAMERA:
        return;
    default:
        break;
	}

	if (panSpeed != 0.0f)
	{
		panRadians(panSpeed * elapsedTime);
	}
	if (tiltSpeed != 0.0f)
	{
		tiltRadians(tiltSpeed * elapsedTime);
	}

	//If cameraFocus == camera position, just look in the direction we're looking
	if (cameraCoord.translation == cameraFocus.translation)
	{
		//cameraCoord.rotation = cameraFocus.rotation; // scenario when we're panning / tilting (translation is the same but we've rotated)
		cameraCoord.lookAt(cameraCoord.lookVector());
	}
	else // otherwise look towards our focal point
		cameraCoord.lookAt(cameraFocus.translation);
}

void Camera::stepSubject()
{
	if (CameraSubject* subject = getCameraSubject())
	{
		subject->stepRotationalVelocity(cameraCoord.translation, cameraFocus.translation);
	}
}

void Camera::zoomOut(CoordinateFrame& cameraPos, CoordinateFrame& cameraFocus, float currentFocusToCameraDistance)
{
	currentFocusToCameraDistance *= 2.0f;

	currentFocusToCameraDistance = std::max(Camera::distanceMin(), currentFocusToCameraDistance);
	currentFocusToCameraDistance = std::min(Camera::distanceMax(), currentFocusToCameraDistance);

	setDistanceFromTarget(currentFocusToCameraDistance, cameraPos, cameraFocus);
}


void Camera::lerpToExtents(const Extents& extents)
{
	// first, make sure camera lerp goals are stopped, lerp goals are changing
	stopInterpolation();

	// make camera look at the extents
	if (cameraType == Camera::FIXED_CAMERA)
	{
		Vector3 newFocus = extents.center();
		Vector3 delta = newFocus - cameraFocus.translation;

		cameraFocusGoal = CoordinateFrame(cameraFocus.rotation,cameraFocus.translation + delta);
		cameraCoordGoal = CoordinateFrame(cameraCoord.rotation,cameraCoord.translation + delta);
	}

	cameraCoordGoal.lookAt(cameraFocusGoal.translation);

	bool cameraInExtents = extents.contains(cameraCoordGoal.translation);

	const Vector3 initialCameraToFocus = (cameraFocusGoal.translation - cameraCoordGoal.translation);
	float goalFocusToCamera = initialCameraToFocus.magnitude();
	CoordinateFrame pos = cameraCoordGoal;
	CoordinateFrame focus = cameraFocusGoal;

	if ( RBX::ServiceProvider::findServiceProvider(this) != NULL &&
		 ( RBX::Network::Players::getGameMode(this) == RBX::Network::EDIT || 
		   RBX::Network::Players::getGameMode(this) == RBX::Network::DPHYS_GAME_SERVER ||
		   RBX::Network::Players::getGameMode(this) == RBX::Network::GAME_SERVER ) )
	{
		const Vector3 focusToCameraUnit = -initialCameraToFocus.unit();
		float distNeeded = (extents.longestSide()) - (pos.translation - extents.center()).magnitude();
		if(distNeeded > 0.0f)
		{
			distNeeded += 4.0f;
			pos.translation += focusToCameraUnit * distNeeded;
			focus.translation = extents.center();
		}
	}
	else
	{
		// make sure camera is not inside extents, if so, push out camera
		while(cameraInExtents && goalFocusToCamera > Camera::distanceMin() && goalFocusToCamera < Camera::distanceMax())
		{
            float oldGoal = goalFocusToCamera;
			zoomOut(pos, focus, goalFocusToCamera);

            // if we can no longer zoom out, quit
            if ( goalFocusToCamera == oldGoal )
                break;

			cameraInExtents = extents.contains(pos.translation);
		}
	}
	setCameraLerpGoals(pos,focus);
	hasFocalObject = true;

	camInterpolation = CAM_INTERPOLATION_CONSTANT_SPEED;
}

void Camera::tryZoomExtents(const Extents& extents)
{
	Vector3 extentsSize = extents.size();
	float largestSize = extentsSize.x;
	if ( extentsSize.y > largestSize )
		largestSize = extentsSize.y;
	if( extentsSize.z > largestSize )
		largestSize = extentsSize.z;
    
    // we are looking at an infinitely small point in space
    // camera will display white if we don't give it some
    // volume (otherwise focus == camera position)
    if(largestSize <= 0)
        largestSize = 1;
    
    // do this before getting lookdir, otherwise if
    // focus.translation == coord.translation we get a nan vector
    setCameraFocus(CoordinateFrame(cameraFocus.rotation,extents.center()));

	Vector3 lookDir = (cameraCoord.translation - cameraFocus.translation).unit();
    setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation,extents.center() + (lookDir * largestSize)));
}
void Camera::zoomExtents(const ModelInstance* model, ZoomType zoomType)
{
	Extents extents = model->computeExtentsWorld();
	extents = extents.clampInsideOf(Tolerance::maxExtents());

	zoomExtents(extents, zoomType);
}
void Camera::zoomExtents(const Extents& extents, ZoomType zoomType)
{
	stopInterpolation();

	Vector3 delta = Vector3::zero();

	if (cameraType == Camera::FIXED_CAMERA)
	{
		Vector3 newFocus = extents.center();
		delta = newFocus - cameraFocus.translation;

		setCameraFocus(CoordinateFrame(cameraFocus.rotation,cameraFocus.translation + delta) );
		setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation,cameraCoord.translation + delta) );
	}

	float min = distanceMin();
	float max = distanceMax();

	float cameraToFocus = cameraToFocusDistance();

	if (zoomType == ZOOM_OUT_ONLY)
	{
		min = std::max(min, cameraToFocus);	
		max = std::max(max, cameraToFocus);
	}

	const float current = std::min(max, std::max(min, cameraToFocus));
	RBXASSERT(G3D::isFinite(current));

	if (G3D::isFinite(min) && G3D::isFinite(current) && G3D::isFinite(max))
	{
		tryZoomExtents(extents);
		hasFocalObject = true;
	}

	pushCameraHistoryStack();
}
bool Camera::zoomExtents()
{
	if (ICameraOwner* owner = getCameraOwner()) 
	{
		zoomExtents(owner->getCameraOwnerModel(), ZOOM_IN_OR_OUT);
		return true;
	}
	else
		return false;
}


bool Camera::canZoom(bool inwards) const
{
	if (cameraType == Camera::LOCKED_CAMERA)
		return false;
	
	if (!hasFocalObject) // If we're not focusing on a specific object, we should be able to zoom anywhere
		return true;

	return inwards
		? (cameraToFocusDistance() > Camera::distanceMin())
		: (cameraToFocusDistance() < Camera::distanceMax());
}

// only updates the goal
// 
bool Camera::setDistanceFromTarget(float newDistance)
{
	return setDistanceFromTarget(newDistance, cameraCoord, cameraFocus);
}

bool Camera::setDistanceFromTarget(float newDistance, CoordinateFrame& newCameraPos, const CoordinateFrame& newCameraFocus)
{
	const Vector3 coordToFocus = newCameraFocus.translation - newCameraPos.translation;
	const float currentDistance = coordToFocus.magnitude();
	const float min = Camera::distanceMin();
	const float max = Camera::distanceMax();

	if ((newDistance < min) && (currentDistance == min)) {
		return false;
	}

	if ((newDistance > max) && (currentDistance == max)) {
		return false;
	}

	newDistance = std::max(min, newDistance);
	newDistance = std::min(max, newDistance);

	newCameraPos = CoordinateFrame(newCameraPos.rotation, newCameraFocus.translation - (newDistance * (coordToFocus / currentDistance)) );
	
	return true;
}

void Camera::onMousePan(const Vector2& wrapMouseDelta)
{
	if (cameraType != Camera::LOCKED_CAMERA)
	{
		tiltRadians(Math::degreesToRadians(-0.3f * wrapMouseDelta.y));
		panRadians(Math::degreesToRadians(-0.4f * wrapMouseDelta.x));
	}
}

void Camera::onMouseTrack(const Vector2& wrapMouseDelta)
{
	if (cameraType != Camera::LOCKED_CAMERA)
	{
        // Track camera by 5% of total pixels moved by the mouse.
        const float kCameraTrackModifer = 0.05f;
    
        // Translate by the Camera y-direction.
        Vector3 vec = cameraCoord.rotation.column(1);
        vec.unitize();
        cameraCoord.translation += vec * wrapMouseDelta.y * kCameraTrackModifer;
        cameraFocus.translation += vec * wrapMouseDelta.y * kCameraTrackModifer;

        // Translate by the Camera x-direction.
        vec = cameraCoord.rotation.column(0);
        vec.unitize();
        cameraCoord.translation -= vec * wrapMouseDelta.x * kCameraTrackModifer;
        cameraFocus.translation -= vec * wrapMouseDelta.x * kCameraTrackModifer;
	}
}

	
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void Camera::setCameraType(Camera::CameraType value)
{
	if (cameraType != value)
	{
		stopInterpolation();
		cameraType = value;
		raisePropertyChanged(desc_cameraType);
	}
	if (cameraType != Camera::LOCKED_CAMERA)
		roll = 0; // no rolling for most camera types for now
}

void Camera::setCameraSubject(Instance* newSubject)
{
	if ( (newSubject != cameraSubject.get()) &&	dynamic_cast<CameraSubject*>(newSubject) )
	{
		if(ICharacterSubject* charSubject = dynamic_cast<ICharacterSubject*>(cameraSubject.get()))
			charSubject->tellCameraSubjectDidChange(cameraSubject,shared_from(newSubject));

		cameraSubject = shared_from(newSubject);

		// this is to help when people set the camera back to the character, instead of the humanoid (control schemes can't interface with camera otherwise)
		shared_ptr<RBX::Instance> potentialHuman = shared_from(cameraSubject->findFirstChildByName("Humanoid"));
		if(potentialHuman &&  Instance::fastDynamicCast<RBX::Humanoid>(potentialHuman.get()))
			cameraSubject = potentialHuman;

		raisePropertyChanged(cameraSubjectProp);
	}
}

Instance* Camera::getCameraSubjectInstanceDangerous() const	// for reflection
{
	return cameraSubject.get();
}

const CameraSubject* Camera::getConstCameraSubject() const
{ 
	const Instance* i = cameraSubject.get();
	if (i) {
		const CameraSubject* answer = dynamic_cast<const CameraSubject*>(i);
		RBXASSERT(answer);
		return answer;
	}
	else {
		return NULL;
	}
}

CameraSubject* Camera::getCameraSubject()
{
	return const_cast<CameraSubject*>(getConstCameraSubject());
}

// Would rather provide an overloaded function but the compiler can't disambiguate for our Reflection PropDescriptor
void Camera::setCameraFocusAndMaintainFocus(const CoordinateFrame& value, bool maintainFocusOnPoint)
{
	setCameraFocus(value);
	// If there is an object that we're focusing on and we want it to stay our focal point, assign here (thus allowing us to rotate around)
	hasFocalObject = maintainFocusOnPoint;
}

void Camera::setCameraFocusOnly(const CoordinateFrame& value)
{
	if (value != cameraFocus)
	{
		cameraFocus = value;
		raisePropertyChanged(desc_Focus);
	}
}

void Camera::setCameraFocusOnlyWithoutPropertyChange(const CoordinateFrame& value)
{
	if (value != cameraFocus)
	{
		cameraFocus = value;
	}
}

void Camera::setCameraFocus(const CoordinateFrame& value)
{
	if (value != cameraFocus)
	{
		interpolationDuration = 0.f;
		camInterpolation = CAM_INTERPOLATION_NONE;

		cameraFocus = value;
		cameraFocusGoal = value;
		raisePropertyChanged(desc_Focus);
	}
}

void Camera::setCameraFocusWithoutPropertyChange(const CoordinateFrame& value)
{
	if (value != cameraFocus)
	{
		interpolationDuration = 0.f;
		camInterpolation = CAM_INTERPOLATION_NONE;

		cameraFocus = value;
		cameraFocusGoal = value;
	}
}


void Camera::setFieldOfViewDegrees(float value)
{
	float clampedValue = G3D::clamp(value,1.0f,120.0f); // anything above or below this range is not compatible with the camera
	if(clampedValue != value)
		StandardOut::singleton()->printf(MESSAGE_WARNING, "FieldOfView set out of range, should be between %f and %f, setting to %f",1.0f,120.0f,clampedValue);

	clampedValue = G3D::toRadians(clampedValue);
	if(clampedValue != fieldOfView)
	{
		fieldOfView = clampedValue;
		imagePlaneDepth = 1.0f / (2.0f * tanf(fieldOfView / 2.0f));
		raisePropertyChanged(desc_FieldOfView);
	}
}

float Camera::getRollSlow()
{
	return roll;
}

void Camera::setCameraLerpGoals(const CoordinateFrame& cameraCoordValue, const CoordinateFrame& cameraFocusValue)
{
	cameraCoordGoal = cameraCoordValue;
	cameraFocusGoal = cameraFocusValue;
}

void Camera::stopInterpolation()
{
	camInterpolation = CAM_INTERPOLATION_NONE;
	setCameraLerpGoals(cameraCoord,cameraFocus);
	interpolationTime = -1.f;
}

void Camera::setRoll(float value)
{
	if (cameraType == Camera::LOCKED_CAMERA) 
		roll = value; 
	else
	{
		// print a warning and disallow for now
		StandardOut::singleton()->printf(MESSAGE_WARNING, "SetRoll can only be used on Camera objects with a CameraType of Scriptable");
		roll = 0;
	}
}

void Camera::setCameraCoordinateFrame(const CoordinateFrame& value)
{
	if ((cameraCoord != value) && legalCameraCoord(value))
	{
		cameraCoord = value;
		cameraCoordGoal = value;
		raisePropertyChanged(desc_CFrame);
		raisePropertyChanged(desc_CoordFrame);
		cframeChangedSignal(cameraCoord);
	}
	else if(cameraCoordGoal != value)
		cameraCoordGoal = value;
}

////////////////////////////////////////////////////////////////////////

bool Camera::zoom(float in)		// in zoom percentage
{
	if(isEditMode())
		return nonCharacterZoom(in);

	if (cameraType == Camera::CUSTOM_CAMERA)
	{
		return false;
	}

	return isCharacterCamera() 
				? characterZoom(in)
				: nonCharacterZoom(in);
}

bool Camera::nonCharacterZoom(float in)
{
	if (!hasClientPlayer())
	{
		const Vector3 lookVector = cameraCoord.lookVector();
		Vector3 zoomVector = lookVector;

		if ( ControllerService* service = ServiceProvider::create<ControllerService>(this) )
        {
            if ( const UserInputBase* hardwareDevice = service->getHardwareDevice() ) 
		    {
				if(DataModel* dm = DataModel::get(this))
				{
					NavKeys navKeys;
					hardwareDevice->getNavKeys(navKeys, dm->getSharedSuppressNavKeys());

					if ( navKeys.shiftKeyDown())
						zoomVector *= Camera::CameraShiftKeyMoveFactor;
					else
						zoomVector *= (FFlag::UserBetterInertialScrolling ? G3D::abs(in/CameraMouseWheelMoveFactor) : CameraMouseWheelMoveFactor);
				}
		    }
        }

		if(in <= 0.0f)
		{
			setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation, cameraCoord.translation - zoomVector));
			if(!hasFocalObject)
				setCameraFocus(CoordinateFrame(cameraFocus.rotation, cameraFocus.translation - zoomVector));
		}
		else
		{
			if(!hasFocalObject)
			{
				setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation, cameraCoord.translation + zoomVector));
				setCameraFocus(CoordinateFrame(cameraFocus.rotation, cameraFocus.translation + zoomVector));
			}
			else
			{
				// If our zoom distance is less than the distance to the part, zoom in 1 increment
				if(cameraToFocusDistance() > zoomVector.magnitude())
					setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation, cameraCoord.translation + zoomVector));
				// Otherwise zoom to the center of the object
				else
					setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation, cameraFocus.translation));
			
			}
		}
		pushCameraHistoryStack();
		return true;
	}
	
	const Vector3 lookVector = cameraFocus.translation - cameraCoord.translation;
	const float currentDistance = lookVector.magnitude();
	const float newDistance = getNewZoomDistance(currentDistance, in);

	if (newDistance == currentDistance)
		return false;
	else 
	{
		setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation, cameraCoord.translation - (lookVector * (newDistance / currentDistance - 1.0f))) );
		pushCameraHistoryStack();
		return true;
	}
}

bool Camera::isEditMode() const
{
	return RBX::ServiceProvider::findServiceProvider(this) != NULL && 
		(RBX::Network::Players::getGameMode(this) == RBX::Network::EDIT || RBX::Network::Players::isCloudEdit(this));
}

bool Camera::hasClientPlayer() const
{
	RBX::Network::Players* players = ServiceProvider::create<Network::Players>(this);
	return players && players->getLocalPlayer() && !Network::Players::isCloudEdit(this);
}

bool Camera::characterZoom(float in)		// in zoom percentage
{
	// This method is poorly named! This zoom method is used for camera zooming
	// on a known target, but is not used for zooming in on players, the
	// ICharacterSubject is instead used for zooming in on players.

	Vector3 focusToGoal = cameraCoord.translation - cameraFocus.translation;
	float currentDistance = focusToGoal.magnitude();
	float newDistance = std::min(	getNewZoomDistance(currentDistance, in)
								,	distanceMaxCharacter()	);

	
	if (newDistance == currentDistance) 
	{
		return false;
	}
	else 
	{
		// This is duplicated in ICharacterSubject. The code is not currently
		// shared because ICharacterSubject does occlusion adjustments, but the
		// camera should not account for occlusion when characterZoom is called.
		focusToGoal.y = 0.0f;
		focusToGoal.unitize();
		// don't allow the camera to get within pi/20 of directly overhead, 
		// the camera's orientation will be lost. tan(pi/20) ~= 1/6.
		// manToCamera was unitized so the magnitude of the x and z components is
		// 1, so make sure that the y value is less than or equal to 6. 
		focusToGoal.y = std::min(6.0f, 0.025f * newDistance);
		setCameraCoordinateFrame(CoordinateFrame(cameraFocus.rotation, cameraFocus.translation + (focusToGoal.unit() * newDistance)) );
		return true;
	}
}

void Camera::lookAt(const Vector3& point, bool lerpCamera)
{
	if(lerpCamera) // for lerping camera in studio
	{
		CoordinateFrame cameraCoordCopy = cameraCoord;
		cameraCoordCopy.lookAt(point);
		setCameraLerpGoals(cameraCoordCopy, CoordinateFrame(cameraFocus.rotation, point));
	}
	else
	{
		setCameraFocus(CoordinateFrame(cameraFocus.rotation, point));
		cameraCoord.lookAt(point);
		setCameraCoordinateFrame(cameraCoord);
	}
}

bool Camera::canTilt(int up) const
{
	if (cameraType == Camera::LOCKED_CAMERA)
		return false;

	const Vector3 look = cameraCoord.lookVector();

	if (look.y != look.y)    // guard against #INV
		return false;

	float angle = Math::elevationAngle(look);
	return 	(up < 0)	
				? (angle >= -Math::piHalf()) 
				: (angle <= Math::piHalf());
}

void Camera::getHeadingElevationDistance(float& heading, float& elevation, float& distance)
{
	Math::getHeadingElevation(cameraCoord, heading, elevation);
	distance = cameraToFocusDistance();
}


void Camera::setHeadingElevationDistance(float heading, float elevation, float distance)
{
	Math::setHeadingElevation(cameraCoord, heading, elevation);
	if (distance == 0.0f) // Hack to allow us to tilt / pan when we're at our focal point
		distance = 1.0f;
	setCameraCoordinateFrame(CoordinateFrame(cameraCoord.rotation, cameraFocus.translation - distance * cameraCoord.lookVector()) );
}

void Camera::tiltSpeedRadians(float tilt)
{
	tiltSpeed = tilt;
}

void Camera::panSpeedRadians(float angle)
{
	panSpeed = angle;
}

bool Camera::tiltRadians(float tilt)
{
	if (tilt!=0.0f)
	{
		float heading, elevation, distance;

		getHeadingElevationDistance(heading, elevation, distance);

		static const float almost90Degrees = Math::pif() * (9.0f / 20.0f);
		float lookUpMax = almost90Degrees;
		if(ICharacterSubject* charSubject = dynamic_cast<ICharacterSubject*>(cameraSubject.get())) {
			if (charSubject->getCustomCameraMode() == GameBasicSettings::CAMERA_MODE_FOLLOW) {
				lookUpMax = 60 * Math::pif() / 180.0f;
			}
		}

		float newElevation = G3D::clamp(elevation + tilt, -almost90Degrees,	lookUpMax );

		if (newElevation != elevation)
		{
			if(RBX::GameBasicSettings::singleton().inHybridMode() && !isFirstPersonCamera() && isCharacterCamera()) // don't allow hybrid mode to tilt so much
				newElevation = G3D::clamp(newElevation,-0.44f,0.22f);
			setHeadingElevationDistance(heading, newElevation, distance);
			return true;
		}
	}
	return false;
}


void Camera::panRadians(float angle)
{
	RBXASSERT(angle > -100.0f);
	RBXASSERT(angle < 100.0f);	// catch weird numbers here

	if (angle != 0.0f)
	{
		float heading, elevation, distance;			// from the camera, looking away

		getHeadingElevationDistance(heading, elevation, distance);
		
		heading = static_cast<float>(Math::radWrap(heading + angle));

		FASTLOG3F(FLog::UserInputProfile, "Panning camera, heading: %f, elevation: %f, distance: %f", heading, elevation, distance);

		setHeadingElevationDistance(heading, elevation, distance);
	}
}

void Camera::setCameraPanMode(Camera::CameraPanMode mode)
{
	cameraPanMode = mode;
}

bool Camera::tiltUnits(int up)
{
	const Vector3 look = cameraCoord.lookVector();
	float angle = Math::elevationAngle(look);
	float angleD = Math::radiansToDegrees(angle);
	int angleId = Math::iRound(angleD * 0.1f);		// to 10 degree slots
	float newAngle = Math::degreesToRadians(10.0f * (angleId + up));
	return tiltRadians(newAngle - angle);
}


void Camera::panUnits(int units)
{
	const Vector3 look = cameraCoord.lookVector();
	float angle = atan2(-look.z, -look.x);
	float newAngle = Math::iRound(angle * 4.0f / Math::pif() + units) * Math::pif() / 4.0f;
	panRadians(angle - newAngle);
}

void Camera::setImageServerViewNoLerp(const CoordinateFrame& modelCoord)
{
	Vector3 look = modelCoord.lookVector();

	// 1.  Clip the look vector to the plane
	if (std::abs(look.y) > 0.95f) {
		look = -Vector3::unitZ();
	}
	else {
		look.y = 0.0f;
		look = look.direction();
	}

	CoordinateFrame lookCoord;
	lookCoord.lookAt(look);	// goal -z == look vector

	// 2. Now rotate the look vector to give us a near-"isometric" view
	lookCoord.rotation *= Matrix3::fromEulerAnglesZXY(45.0f*G3D::pif()/180.0f, 35.0f*G3D::pi()/180.0f, 0.0f);	// changed from 40.0, 30.0 on 3/22/07

	look = lookCoord.lookVector();		// ok, now clipped and rotated
	lookCoord.translation = modelCoord.translation + (10.0f * look);		// looking AT - reverse direction
	lookCoord.lookAt(modelCoord.translation);

	setCameraType(Camera::FIXED_CAMERA);
	setCameraFocus(modelCoord.translation);
	setCameraCoordinateFrame(lookCoord);
	zoomExtents();
}


void Camera::doFly(const NavKeys& nav, int steps)
{
	if (getCameraType() != Camera::FIXED_CAMERA)
		return;

	if (nav.navKeyDown())
	{
		bool shiftFly = nav.shiftKeyDown();
		float accelerationMultiplier = 1.0f;
		if (FFlag::FlyCamOnRenderStep)
		{
			float framesPerSecond = 60.0f;
			accelerationMultiplier = 0.5f;
			if (!shiftFly && steps > 2 * framesPerSecond)
				accelerationMultiplier =  std::min(15.0f, ((float)(steps)) / (4.0f * framesPerSecond));
		}
		else
		{
			if (!shiftFly && steps > 60)
				accelerationMultiplier =  std::min(30.0f, ((float)(steps))/60.0f);
		}

		// If we don't have an avatar, or mouselock mode is off, shift key modifies our camera
		bool isInMouseLockMode = hasClientPlayer() && GameBasicSettings::singleton().inMouseLockMode();
		if ( nav.shiftKeyDown() && !isInMouseLockMode )
			accelerationMultiplier *= CameraShiftKeyMoveFactor;

		CoordinateFrame current = this->getCameraCoordinateFrame();
		//Vector3 focus = this->getCameraFocus().translation;
		Vector3 look = current.lookVector();
		Vector3 right = current.rightVector();
		Vector3 up = current.upVector();
		Vector3 delta;

		accelerationMultiplier *= CameraKeyMoveFactor;

		if (nav.forward())
			delta += look * accelerationMultiplier;
		if (nav.backward())
			delta -= look * accelerationMultiplier;
		if (nav.right())
			delta += right * accelerationMultiplier;
		if (nav.left())
			delta -= right * accelerationMultiplier;
		if (nav.up())
			delta -= up * accelerationMultiplier;
		if (nav.down())
			delta += up * accelerationMultiplier;

	  	current.translation += delta;
		
		setCameraCoordinateFrame(current);

		if (isEditMode())
		{
			hasFocalObject = false;
			setCameraFocus(current.translation + look*2.0f);
		}
		else
		{
			// TODO: Hmmm... we're changing the camera type based on a key press
			setCameraType(Camera::FIXED_CAMERA);
			setCameraFocus(current.translation + 20.0f*look);
		}
	}
}

float Camera::nearPlaneZ() const 
{
	return -0.5f;
}

float Camera::getImagePlaneDepth() const 
{
	// The image plane depth has been pre-computed for 
	// a 1x1 image.  Now that the image is width x height, 
	// we need to scale appropriately.
	return imagePlaneDepth * viewport.y;
}

float Camera::getViewportWidth() const 
{
	return viewport.x;
}


float Camera::getViewportHeight() const
{
	return viewport.y;
}


void Camera::setViewport(Vector2int16 newViewport)
{
	if (newViewport != viewport)
	{
		viewport = newViewport;
		raisePropertyChanged(desc_viewport);
	}
}

Vector4 Camera::projectPointToScreen(const Vector3& point) const
{
	int screenWidth  = viewport.x;
	int screenHeight = viewport.y;

	// Find where it hits an image plane of these dimensions
	const float zImagePlane = getImagePlaneDepth();

	const Matrix4 projection = getProjectionPerspective();
	const Vector4 out4 = projection * Vector4(point, 1.0f);
	const Vector3 q = out4.xyz() / out4.w;
	
	return Vector4((screenWidth / 2.0) + ((screenWidth / 2.0) * q.x), (screenHeight / 2.0) - ((screenHeight / 2.0) * q.y), zImagePlane * 2 * q.z, out4.w);
}

shared_ptr<Reflection::Tuple> makeProjectionArgs(const Vector3& vectorIn2D, const Vector2& viewport)
{
	shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();
	args->values.push_back(vectorIn2D);

	const Vector2 clampedPosition = G3D::clamp(Vector2(vectorIn2D.x, vectorIn2D.y),Vector2(0,0), viewport); 
	bool isOnScreen = vectorIn2D.z > 0 && (clampedPosition == Vector2(vectorIn2D.x, vectorIn2D.y));
	args->values.push_back(isOnScreen);

	return args;
}
shared_ptr<const Reflection::Tuple> Camera::projectLua(Vector3 point)
{
	const Vector4 projection = projectPointToScreen(point);
	
	Vector3 offsetVector = Vector3(projection.x, projection.y, projection.w);
	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
	{
		Vector4 guiInset = guiService->getGlobalGuiInset();
		offsetVector = Vector3(offsetVector.x - guiInset.x, offsetVector.y - guiInset.y, offsetVector.z);
	}

	return makeProjectionArgs(offsetVector, viewport);
}

shared_ptr<const Reflection::Tuple> Camera::projectViewportLua(Vector3 point)
{
	const Vector4 projection = projectPointToScreen(point);
	return makeProjectionArgs(Vector3(projection.x, projection.y, projection.w), viewport);
}

Vector3 Camera::project(const Vector3& point) const
{
	const Vector4 projection = projectPointToScreen(point);
	if (projection.w <= 0.0f) 
	{
		// provide at least basic quadrant information.
		// (helps with clipping)
		return Vector3(((projection.x < 0.0f) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity()),
			((projection.y > 0.0f) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity()),
			std::numeric_limits<float>::infinity());
	}

	return Vector3(projection.x, projection.y, projection.z);
}

RbxRay Camera::worldRayLua(float x, float y, float depth)
{
	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
	{
		Vector4 guiInset = guiService->getGlobalGuiInset();
		return worldRayViewportLua(x + guiInset.x, y + guiInset.y, depth);
	}

	return worldRayViewportLua(x, y, depth);
}

RbxRay Camera::worldRayViewportLua(float x, float y, float depth)
{
	return worldRay(x, y, depth);
}

RBX::RbxRay Camera::worldRay(float x, float y, float depth) const
{
	int screenWidth  = viewport.x;
	int screenHeight = viewport.y;

	CoordinateFrame cameraFrame = getRenderingCoordinateFrame();
	Vector3 origin = cameraFrame.translation;

	float cx = screenWidth  / 2.0f;
	float cy = screenHeight / 2.0f;
 
	Vector3 point = Vector3((x / cx) - 1.0f, 1.0f - (y  / cy), imagePlaneDepth);
	Matrix4 projection = getProjectionPerspective();
	Vector4 projectedPoint = projection.inverse() * Vector4(point, 1.0f);
	Vector3 projectedPointAdjusted = projectedPoint.xyz() / projectedPoint.w;
	Vector3 direction = projectedPointAdjusted - origin;

	// Normalize the direction (we didn't do it before)
	direction = direction.direction();


	float theta = acos(std::min(1.0f, direction.dot(cameraFrame.lookVector())));
	float depthToNearClipPlane = imagePlaneDepth / sin((Math::pif() / 2) - theta);
        
	return RBX::RbxRay::fromOriginAndDirection(origin + (direction * depthToNearClipPlane) + (direction * depth), direction);
}

const CoordinateFrame& Camera::coordinateFrame() const {
	return getCameraCoordinateFrame();
}

float Camera::dot(const Vector3& point) const {
	Vector3 toPoint = point - cameraCoord.translation;
	return cameraCoord.lookVector().dot(toPoint);
}

RBX::Frustum Camera::frustum() const {
    RBX::Frustum f;
	frustum(farPlaneZ(), f);
    return f;
}

void Camera::frustum(const float farPlaneZ, RBX::Frustum& fr) const 
{
	fr.faceArray.fastClear();
	// The volume is the convex hull of the vertices defining the view
	// frustum and the light source point at infinity.
	const CoordinateFrame& cframe = getRenderingCoordinateFrame();
	if( !Math::hasNanOrInf(cframe) )
	{
		float fovx;
		fovx = 2 * atan(tan(fieldOfView * 0.5f) * viewport.x / viewport.y);
        
		fr = Frustum(cframe.translation, -cframe.rotation.column(2), cframe.rotation.column(1), -nearPlaneZ(), -farPlaneZ, fovx, fieldOfView);
	}
}

bool Camera::legalCameraCoord(const CoordinateFrame& c)
{
    if(Math::hasNanOrInf(c))
        return false;
    
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			float r = c.rotation[i][j];
			if (!((r > -1.2f) && (r < 1.2f))) {
				return false;
			}
		}
		float t = c.translation[i];
		if (!((t > -1e6f) && (t < 1e6f))) {
			return false;
		}
	}
	return true;
}


Matrix4 Camera::getProjectionPerspective() const 
{
	Matrix4 view;

	if (FFlag::CameraVR)
	{
		CoordinateFrame cframe = getRenderingCoordinateFrame();

		view = cframe.inverse().toMatrix4();
	}
	else
	{
		CoordinateFrame cframe = getCameraCoordinateFrame();

		view = Matrix4::rollDegrees(G3D::toDegrees(roll)) * cframe.inverse().toMatrix4();
	}

	int screenWidth  = viewport.x;
	int screenHeight = viewport.y;
	float aspect = (float) screenWidth / (float) screenHeight;

	float h = 1 / tanf(getFieldOfView() / 2);
    float w = h / aspect;
	float zfar = - nearPlaneZ();
	float znear = - farPlaneZ();

    // Note: this maps to [0..1] Z range
    float q = -zfar / (zfar - znear);
    float qn = znear * q;

    Matrix4 projection(
            w, 0, 0, 0,
            0, h, 0, 0,
            0, 0, q, qn,
            0, 0, -1, 0);

    Matrix4 viewProjection = projection * view;

	return viewProjection;
}

CoordinateFrame Camera::getRenderingCoordinateFrame() const
{
	CoordinateFrame result = cameraCoord;

	if (roll != 0)
		result.rotation *= Matrix3::fromAxisAngle(Vector3::unitZ(), -roll);

	UserInputService* uis = ServiceProvider::find<UserInputService>(this);

	if (uis && headLocked)
		result = result * uis->getUserHeadCFrame();

	return result;
}

void Camera::setHeadLocked(bool value)
{
	if (headLocked != value)
	{
		headLocked = value;
		raisePropertyChanged(desc_HeadLocked);
	}
}

} // namespace RBX

