/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/G3DCore.h"
#include "Util/HeartbeatInstance.h"
#include <vector>
#include "RbxG3D/Frustum.h"

namespace RBX {

class ICameraOwner;
class CameraSubject;
class ContactManager;
class Primitive;
class NavKeys;
class Extents;
class ModelInstance;
class PartInstance;
class HitTestFilter;

extern const char* const sCamera;
class Camera	: public DescribedCreatable<Camera, Instance, sCamera, Reflection::ClassDescriptor::PERSISTENT_LOCAL>
				, public HeartbeatInstance
{
public:

	rbx::signal<void()> interpolationFinishedSignal;
	rbx::signal<void(bool)> firstPersonTransitionSignal;
	rbx::signal<void(CoordinateFrame)> cframeChangedSignal;

	// Warning - these enums are part of the XML - only append
	enum CameraType {	FIXED_CAMERA = 0,
								ATTACH_CAMERA = 1,		// maintain fixed position and rotation w.r.t target coordinate frame
								WATCH_CAMERA = 2,		// rotate to keep target in view
								TRACK_CAMERA = 3,		// translate to keep target in view
								FOLLOW_CAMERA = 4,		// rotate and translate to keep target in view
								CUSTOM_CAMERA = 5,
								LOCKED_CAMERA = 6,
								NUM_CAMERA_TYPE = 7};
	typedef enum {ZOOM_IN_OR_OUT, ZOOM_OUT_ONLY} ZoomType;
	enum CameraMode
    {
		CAMERAMODE_CLASSIC			= 0,
	    CAMERAMODE_LOCKFIRSTPERSON	= 1
    };
	enum CameraPanMode
	{
		CAMERAPANMODE_CLASSIC		= 0,
		CAMERAPANMODE_EDGEBUMP		= 1,
	};


	static float	distanceDefault()			{return 36.0f;}		// grids
	static float	distanceMin()				{return 0.5f;}		// down from 4.0
	static float	distanceMax()				{return 1000.0f;}	
	static float	distanceMaxCharacter()		{return 400.0f;}
	static float	distanceMinOcclude()		{return 4.0f;}

	static double	interpolationSpeed()		{return 5.0f;} // studs per second

	static bool legalCameraCoord(const CoordinateFrame& c);

	static float CameraKeyMoveFactor;
	static float CameraMouseWheelMoveFactor;
	static float CameraShiftKeyMoveFactor;

	Camera();
	~Camera() {}

	// Yuck - called by Window code
	void step(double elapsedTime);
	void stepSubject();

	void pushCameraHistoryStack();
	std::pair<CoordinateFrame,CoordinateFrame> popCameraHistoryStack(bool backward);

	void getHeadingElevationDistance(float& heading, float& elevation, float& distance);

	bool isCharacterCamera() const;
	bool isFirstPersonCamera() const;	// hack for now - this should be in CameraSubject?? discuss

	bool isPartInFrustum(const PartInstance& part) const;
	bool isPartVisibleFast(const PartInstance& part, const ContactManager& contactManager, const HitTestFilter* filter = NULL) const; // uses only one ray to do check (can have inaccurate results, but takes 1/4 the time)

	bool isLockedToFirstPerson() const;

	CameraSubject* getCameraSubject();
	const CameraSubject* getConstCameraSubject() const;
	Instance* getCameraSubjectInstanceDangerous() const;	// for reflection only
	void setCameraSubject(Instance* newSubject);

	void setCameraLerpGoals(const CoordinateFrame& cameraCoordValue, const CoordinateFrame& cameraFocusValue);

	const CoordinateFrame& getCameraFocus() const { return cameraFocus; }
	void setCameraFocus(const CoordinateFrame& value);	// sets camera focus and camera focus goal
	void setCameraFocusWithoutPropertyChange(const CoordinateFrame& value);
	void setCameraFocusOnly(const CoordinateFrame& value);		// sets camera focus
	void setCameraFocusOnlyWithoutPropertyChange(const CoordinateFrame& value);
	void setCameraFocusAndMaintainFocus(const CoordinateFrame& value, bool maintainFocusOnPoint);

	const CoordinateFrame& getCameraCoordinateFrame() const  {return cameraCoord;}
	void setCameraCoordinateFrame(const CoordinateFrame& value);

	CoordinateFrame getRenderingCoordinateFrame() const;
	CoordinateFrame getRenderingCoordinateFrameLua() { return getRenderingCoordinateFrame(); }

	bool getHeadLocked() const { return headLocked; }
	void setHeadLocked(bool value);

	CameraType getCameraType() const	{return cameraType;}
	void setCameraType(CameraType value);

	bool canZoom(bool inwards) const;
	bool canTilt(int up) const;

	// Cursor Control
	void onMousePan(const Vector2& wrapMouseDelta);
	void onMouseTrack(const Vector2& wrapMouseDelta);

	// Zoom
	bool zoom(float in);
	bool setDistanceFromTarget(float newDistance);
	bool setDistanceFromTarget(float newDistance, CoordinateFrame& newCameraPos, const CoordinateFrame& newCameraFocus);

	bool zoomExtents();	// of the world
	void zoomExtents(const ModelInstance* model, ZoomType zoomType);
	void zoomExtents(const Extents& extents, ZoomType zoomType);

	void lerpToExtents(const Extents& extents);

	// Pan
	void panRadians(float angle);
	void panUnits(int units);
	void panSpeedRadians(float tilt);
	float getPanSpeed(void) { return panSpeed; }

	// Tilt
	bool tiltRadians(float up);
	bool tiltUnits(int up);
	void tiltSpeedRadians(float tilt);
	float getTiltSpeed(void) { return tiltSpeed; }

	// 
	void setCameraPanMode(Camera::CameraPanMode mode);
	Camera::CameraPanMode getCameraPanMode() const { return cameraPanMode; }

	// LookAt
	void lookAt(const Vector3& point, bool lerpCamera);
	void setImageServerViewNoLerp(const CoordinateFrame& modelCoord);

	// Fly
	void doFly(const NavKeys& nav, int steps);

	// Camera History
	void stepCameraHistoryForward();
	void stepCameraHistoryBackward();

	// Util
	static float getNewZoomDistance(float currentDistance, float in);
    
    bool hasClientPlayer() const;
    
	// Clipping plane, *not* imaging plane. Returns a negative z-value.
	float nearPlaneZ() const;

	// Returns a negative z-value.
	inline float farPlaneZ() const {
		return -5e3f;
	}

	inline float getFieldOfView() const {
		return fieldOfView;
	}
	inline float getFieldOfViewDegrees() const {
		return G3D::toDegrees(fieldOfView);
	}
	void setFieldOfViewDegrees(float value);

	inline float getRoll() const{
		return roll;
	}
	void setRoll(float value);
	float getRollSlow();

	// Taken from RbxCamera, a derivative of the G3D Camera.
	// Returns the image plane depth, <I>s'</I>, given the current field
	// of view for film of dimensions width x height.  See
	// setImagePlaneDepth for a discussion of worldspace values width and height. 
	float getImagePlaneDepth() const;

	// Returns the Camera space width of the viewport.
	float getViewportWidth() const;

	// Returns the Camera space height of the viewport.
	float getViewportHeight() const;

	// Taken from RbxCamera, a derivative of the G3D Camera.
	// Projects a world space point onto a width x height screen.  The
	// returned coordinate uses pixmap addressing: x = right and y =
	// down.  The resulting z value is <I>rhw</I>.
	// If the point is behind the camera, Vector3::inf() is returned.
	Vector3 project(const Vector3& point) const;
	shared_ptr<const Reflection::Tuple> projectLua(Vector3 point);
	shared_ptr<const Reflection::Tuple> projectViewportLua(Vector3 point);

	// Taken from RbxCamera, a derivative of the G3D Camera.
	// Returns the world space ray passing through the center of pixel
	// (x, y) on the image plane.  The pixel x and y axes are opposite
	// the 3D object space axes: (0,0) is the upper left corner of the screen.
	// They are in viewport coordinates, not screen coordinates.
	// Integer (x, y) values correspond to
	// the upper left corners of pixels.  If you want to cast rays
	// through pixel centers, add 0.5 to x and y.        
	RbxRay worldRay(float x, float y, float depth = 0.0f) const;
	RbxRay worldRayLua(float x, float y, float depth);
	RbxRay worldRayViewportLua(float x, float y, float depth);

	// Taken from RbxCamera, a derivative of the G3D Camera.
	// Returns the world space view frustum, which is a truncated pyramid describing
	// the volume of space seen by this camera.
	void frustum(const float farPlaneZ, RBX::Frustum& fr) const;

	RBX::Frustum frustum() const;

	const CoordinateFrame& coordinateFrame() const;

	Vector2int16 getViewport() const { return viewport; }
	void setViewport(Vector2int16 newViewport);

	// Calculates the dot product of camera direction and point. Useful for 
	// half space test or getting angle between the 2
	float dot(const Vector3& point) const;

	void beginCameraInterpolation(CoordinateFrame endPos, CoordinateFrame endFocus, float duration);

	Vector4 projectPointToScreen(const Vector3& point) const;


private:
	typedef DescribedCreatable<Camera, Instance, sCamera, Reflection::ClassDescriptor::PERSISTENT_LOCAL> Super;

	enum CameraInterpolation
	{
		CAM_INTERPOLATION_CONSTANT_TIME,
		CAM_INTERPOLATION_CONSTANT_SPEED,
		CAM_INTERPOLATION_NONE,
	};

	void updateFocus();

	bool isEditMode() const;
	bool characterZoom(float in);
	bool nonCharacterZoom(float in);

	void tryZoomExtents(const Extents& extents);

	float cameraToFocusDistance() const {
		return (cameraCoord.translation - cameraFocus.translation).magnitude();
	}

	void setHeadingElevationDistance(float heading, float elevation, float distance);

	// Instance
	/*override*/ bool askSetParent(const Instance* instance) const;
	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
		Super::onServiceProvider(oldProvider, newProvider);
		onServiceProviderHeartbeatInstance(oldProvider, newProvider);		// hooks up heartbeat
	}

	void fixedSpeedInterpolateCamera(double elapsedTime);

	void zoomOut(CoordinateFrame& cameraPos, CoordinateFrame& cameraFocus, float currentFocusToCameraDistance);

	Matrix4 getProjectionPerspective() const;

	void stopInterpolation();
    void signalInterpolationDone();

	// HeartbeatInstance
	/*override*/ void onHeartbeat(const Heartbeat& event);

	CameraInterpolation			camInterpolation;

	CoordinateFrame				cameraCoord;				// Where is the camera
	CoordinateFrame				cameraFocus;				// Looking at what?
	CoordinateFrame				cameraCoordGoal;			// Where we want to be (used for camera transitions)
	CoordinateFrame				cameraFocusGoal;			// Where we want to look (used for camera transitions)
	CoordinateFrame				cameraCoordPrev;			// Where we used to be (used for camera interpolation)
	CoordinateFrame				cameraFocusPrev;			// Where we used to look (used for camera interpolation)
	G3D::Vector3				cameraUpDirPrev;
	float						interpolationDuration;		// Time in seconds to interpolate position and focus; < 0 for constant speed interpolation
	mutable float				interpolationTime;			// Time in seconds that we have been doing the current interpolation

	bool						headLocked;

	CameraType					cameraType;	
	shared_ptr<Instance>		cameraSubject;				// Guaranteed to be a CameraSubject
	float						fieldOfView;
	float						roll;

	Vector2int16				viewport;

	float						panSpeed;
	float						tiltSpeed;

	CameraPanMode				cameraPanMode;

	// The image plane depth corresponding to a vertical field of 
	// view, where the film size is 1x1.  
	float						imagePlaneDepth;
	bool						hasFocalObject;

	ICameraOwner*				getCameraOwner();

	std::vector< std::pair<CoordinateFrame,CoordinateFrame> >	cameraHistoryStack;
	int	currentCameraHistoryPosition;
	double lastHistoryPushTime;
};

}	// namespace RBX
