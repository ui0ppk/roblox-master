/**
 * HandleAdornment.cpp
 * Copyright (c) 2015 ROBLOX Corp. All Rights Reserved.
 * Created by Tyler Berg on 3/24/2015
 */
#include "stdafx.h"

#include "V8DataModel/HandleAdornment.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/ModelInstance.h"
#include "AppDraw/DrawAdorn.h"
#include "AppDraw/Draw.h"
#include "v8datamodel/PVInstance.h"
#include "v8datamodel/MouseCommand.h"
#include "V8DataModel/Workspace.h"
#include "G3D/CollisionDetection.h"

namespace RBX {

const char* const sHandleAdornment = "HandleAdornment";

REFLECTION_BEGIN();
Reflection::PropDescriptor<HandleAdornment, Vector3> HandleAdornment::prop_sizeRelativeOffset("SizeRelativeOffset", category_Data, &HandleAdornment::getOffset, &HandleAdornment::setOffset);
Reflection::PropDescriptor<HandleAdornment, CoordinateFrame> HandleAdornment::prop_adornCFrame("CFrame", category_Data, &HandleAdornment::getCFrame, &HandleAdornment::setCFrame);
Reflection::PropDescriptor<HandleAdornment, int> HandleAdornment::prop_adornZIndex("ZIndex", category_Data, &HandleAdornment::getZIndex, &HandleAdornment::setZIndex);
Reflection::PropDescriptor<HandleAdornment, bool> HandleAdornment::prop_alwaysOnTop("AlwaysOnTop", category_Data, &HandleAdornment::getAlwaysOnTop, &HandleAdornment::setAlwaysOnTop);

static Reflection::RemoteEventDesc<HandleAdornment, void()> event_MouseEnter(&HandleAdornment::mouseEnterSignal, "MouseEnter", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<HandleAdornment, void()> event_MouseLeave(&HandleAdornment::mouseLeaveSignal, "MouseLeave", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<HandleAdornment, void()> event_MouseDown(&HandleAdornment::mouseButton1DownSignal, "MouseButton1Down", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<HandleAdornment, void()> event_MouseUp(&HandleAdornment::mouseButton1UpSignal, "MouseButton1Up", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

HandleAdornment::HandleAdornment(const char* name)
	: DescribedNonCreatable<HandleAdornment, PVAdornment, sHandleAdornment>(name)
	, sizeRelativeOffset(0,0,0)
	, coordinateFrame(Vector3(0,0,0))
	, zIndex(-1)
	, alwaysOnTop(false)
	, mouseOver(false)
{}

CoordinateFrame HandleAdornment::getWorldCoordinateFrame() const
{
	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
	{
		if (shared_ptr<PartInstance> partInstance = RBX::Instance::fastSharedDynamicCast<PartInstance>(pvInstance))
		{
			CoordinateFrame pvCFrame = partInstance->getCoordinateFrame();

			CoordinateFrame worldSpaceCFrame = pvCFrame * coordinateFrame;
			Vector3 worldSpaceOffset = pvCFrame.pointToWorldSpace(sizeRelativeOffset * partInstance->getPartSizeUi() * 0.5f) - pvCFrame.translation;

			return CoordinateFrame(worldSpaceCFrame.rotation, worldSpaceCFrame.translation + worldSpaceOffset);
		}
		else if (shared_ptr<ModelInstance> modelInstance = RBX::Instance::fastSharedDynamicCast<ModelInstance>(pvInstance))
		{
			RBX::Part pvPart = modelInstance->computePart();

			CoordinateFrame pvCFrame = pvPart.coordinateFrame;

			CoordinateFrame worldSpaceCFrame = pvCFrame * coordinateFrame;
			Vector3 worldSpaceOffset = pvCFrame.pointToWorldSpace(sizeRelativeOffset * pvPart.gridSize * 0.5f) - pvCFrame.translation;

			return CoordinateFrame(worldSpaceCFrame.rotation, worldSpaceCFrame.translation + worldSpaceOffset);
		}
	}

	return CoordinateFrame();
}

GuiResponse HandleAdornment::process(const shared_ptr<InputObject>& event)
{
	if (visible && event->isMouseEvent())
	{
		switch(event->getUserInputType())
		{
		case InputObject::TYPE_MOUSEMOVEMENT:
			{
				if (isCollidingWithHandle(event))
				{
					if (!mouseOver)
					{
						mouseOver = true;
						mouseEnterSignal();
					}
				}
				else
				{
					if(mouseOver)
					{
						mouseOver = false;
						mouseLeaveSignal();
					}
				}
			}
		case InputObject::TYPE_MOUSEBUTTON1:
			{
				if (event->isLeftMouseDownEvent())
				{
					if (isCollidingWithHandle(event))
					{
						mouseButton1DownSignal();
						return GuiResponse::sunk();
					}
				}
				else if (event->isLeftMouseUpEvent())
				{
					if (isCollidingWithHandle(event))
						mouseButton1UpSignal();
				}
			}
        default:
            break;
		}
	}

	return GuiResponse::notSunk();
}

void HandleAdornment::setZIndex(int value)
{
	if (zIndex == value || value > Adorn::maximumZIndex)
		return;
	zIndex = value;
	raisePropertyChanged(prop_adornZIndex);
}

const char* const sBoxHandleAdornment = "BoxHandleAdornment";

static Reflection::PropDescriptor<BoxHandleAdornment, Vector3> prop_boxSize("Size", category_Data, &BoxHandleAdornment::getSize, &BoxHandleAdornment::setSize);

BoxHandleAdornment::BoxHandleAdornment() 
	: DescribedCreatable<BoxHandleAdornment, HandleAdornment, sBoxHandleAdornment>(sBoxHandleAdornment)
	, boxSize(1.0f, 1.0f, 1.0f)
{}

void BoxHandleAdornment::render3dAdorn(Adorn* adorn)
{
	if (!getVisible())
		return;

	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
		adorn->box(getWorldCoordinateFrame(), boxSize / 2, Color4(color, 1.0f - transparency), zIndex, alwaysOnTop);
}

bool BoxHandleAdornment::isCollidingWithHandle(const shared_ptr<InputObject>& inputObject)
{
	if (Workspace *workspace = Workspace::findWorkspace(this))
	{
		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject, workspace));

		CoordinateFrame worldFrame = getWorldCoordinateFrame();
		RbxRay localRay = worldFrame.toObjectSpace(gridRay);

		Vector3 tempHitPoint;
		float collision_time = G3D::CollisionDetection::collisionTimeForMovingPointFixedBox(
			localRay.origin(),
			localRay.direction().unit(),
			Box(boxSize * -0.5, boxSize * 0.5),
			tempHitPoint);

		return collision_time != G3D::inf();
	}

	return false;
}

const char* const sConeHandleAdornment = "ConeHandleAdornment";

static Reflection::PropDescriptor<ConeHandleAdornment, float> prop_coneRadius("Radius", category_Data, &ConeHandleAdornment::getRadius, &ConeHandleAdornment::setRadius);
static Reflection::PropDescriptor<ConeHandleAdornment, float> prop_coneHeight("Height", category_Data, &ConeHandleAdornment::getHeight, &ConeHandleAdornment::setHeight);

ConeHandleAdornment::ConeHandleAdornment()
	: DescribedCreatable<ConeHandleAdornment, HandleAdornment, sConeHandleAdornment>(sConeHandleAdornment)
	, radius(0.5f)
	, height(2.0f)
{}

CoordinateFrame ConeHandleAdornment::getWorldCoordinateFrame()
{
	CoordinateFrame worldFrame = HandleAdornment::getWorldCoordinateFrame();
	return worldFrame * G3D::Matrix3::fromEulerAnglesXYZ(0, 0.5 * 3.14159f, 0);
}

bool ConeHandleAdornment::isCollidingWithHandle(const shared_ptr<InputObject>& inputObject)
{
	if (Workspace *workspace = Workspace::findWorkspace(this))
	{
		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject, workspace));
		Vector3 cameraDirection = workspace->getCamera()->coordinateFrame().lookRay().direction();

		CoordinateFrame worldFrame = getWorldCoordinateFrame();

		Vector3 lookVector = worldFrame.pointToWorldSpace(Vector3(1, 0, 0)) - worldFrame.translation;
		Vector3 triNormal = cameraDirection - (cameraDirection.dot(lookVector) * lookVector);
		Vector3 baseTangent = triNormal.cross(lookVector).unit() * radius;

		Vector3 v1 = worldFrame.translation - baseTangent;
		Vector3 v2 = worldFrame.translation + baseTangent;
		Vector3 v3 = worldFrame.translation + (lookVector.unit() * height);

		float tri_collision_time = G3D::CollisionDetection::collisionTimeForMovingPointFixedTriangle(
			gridRay.origin(),
			gridRay.direction().unit(),
			Triangle(v1, v2, v3));

		Vector3 basePlaneNormal =  cameraDirection - (cameraDirection.dot(triNormal.unit()) * triNormal.unit());
		basePlaneNormal = basePlaneNormal.direction() - (basePlaneNormal.direction().dot(baseTangent.unit()) * baseTangent.unit());
		basePlaneNormal *= -1;

		Vector3 hitLocation;

		G3D::CollisionDetection::collisionTimeForMovingPointFixedPlane(
			gridRay.origin(),
			gridRay.direction().unit(),
			Plane(basePlaneNormal, worldFrame.translation),
			hitLocation);

		return tri_collision_time != G3D::inf() || (hitLocation != Vector3::inf() && (worldFrame.translation - hitLocation).magnitude() <= radius);
	}

	return false;
}

void ConeHandleAdornment::render3dAdorn(Adorn* adorn)
{
	if (!getVisible())
		return;

	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
		adorn->cone(getWorldCoordinateFrame(), radius, height, Color4(color, 1.0f - transparency), zIndex, alwaysOnTop);
}

const char* const sCylinderHandleAdornment = "CylinderHandleAdornment";

static Reflection::PropDescriptor<CylinderHandleAdornment, float> prop_cylinderRadius("Radius", category_Data, &CylinderHandleAdornment::getRadius, &CylinderHandleAdornment::setRadius);
static Reflection::PropDescriptor<CylinderHandleAdornment, float> prop_cylinderHeight("Height", category_Data, &CylinderHandleAdornment::getHeight, &CylinderHandleAdornment::setHeight);

CylinderHandleAdornment::CylinderHandleAdornment()
	: DescribedCreatable<CylinderHandleAdornment, HandleAdornment, sCylinderHandleAdornment>(sCylinderHandleAdornment)
	, radius(1.0f)
	, height(1.0f)
{}

CoordinateFrame CylinderHandleAdornment::getWorldCoordinateFrame()
{
	CoordinateFrame worldFrame = HandleAdornment::getWorldCoordinateFrame();
	return worldFrame * G3D::Matrix3::fromEulerAnglesXYZ(0, 0.5 * 3.14159f, 0);
}

bool CylinderHandleAdornment::isCollidingWithHandle(const shared_ptr<InputObject>& inputObject)
{
	if (Workspace *workspace = Workspace::findWorkspace(this))
	{
		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject, workspace));

		Vector3 cameraDirection = workspace->getCamera()->coordinateFrame().lookRay().direction();
		CoordinateFrame worldFrame = getWorldCoordinateFrame();

		Vector3 lookVector = worldFrame.pointToWorldSpace(Vector3(1, 0, 0)) - worldFrame.translation;
		
		Vector3 normal = (cameraDirection - (cameraDirection.dot(lookVector) * lookVector)).unit() * -1;
		Vector3 tangent = normal.cross(lookVector).unit() * radius;

		Vector3 endpoint1 = worldFrame.translation + (lookVector.unit() * height * 0.5);
		Vector3 endpoint2 = worldFrame.translation - (lookVector.unit() * height * 0.5);
		
		Vector3 hitLocation;
		float collision_time_rect = G3D::CollisionDetection::collisionTimeForMovingPointFixedRectangle(
			gridRay.origin(),
			gridRay.direction().unit(),
			endpoint1 + tangent, 
			endpoint2 + tangent, 
			endpoint2 - tangent, 
			endpoint1 - tangent,
			hitLocation);


		Vector3 planeNormal =  gridRay.direction() - (gridRay.direction().dot(normal.unit()) * normal.unit());
		planeNormal = planeNormal.direction() - (planeNormal.direction().dot(tangent.unit()) * tangent.unit());
		planeNormal *= -1;

		Vector3 hitLocation1;
		float plane_collision_time1 = G3D::CollisionDetection::collisionTimeForMovingPointFixedPlane(
			gridRay.origin(),
			gridRay.direction().unit(),
			Plane(planeNormal, endpoint1),
			hitLocation1);

		Vector3 hitLocation2;
		float plane_collision_time2  = G3D::CollisionDetection::collisionTimeForMovingPointFixedPlane(
			gridRay.origin(),
			gridRay.direction().unit(),
			Plane(planeNormal, endpoint2),
			hitLocation2);

		return	collision_time_rect != G3D::inf() || 
				(plane_collision_time1 != G3D::inf() && (hitLocation1 - endpoint1).magnitude() <= radius) ||
				(plane_collision_time2 != G3D::inf() && (hitLocation2 - endpoint2).magnitude() <= radius);

	}

	return false;
}

void CylinderHandleAdornment::render3dAdorn(Adorn* adorn)
{
	if (!getVisible())
		return;

	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
		adorn->cylinder(getWorldCoordinateFrame(), radius, height, Color4(color, 1.0f - transparency), zIndex, alwaysOnTop);
}

const char* const sSphereHandleAdornment = "SphereHandleAdornment";

static Reflection::PropDescriptor<SphereHandleAdornment, float> prop_sphereRadius("Radius", category_Data, &SphereHandleAdornment::getRadius, &SphereHandleAdornment::setRadius);

SphereHandleAdornment::SphereHandleAdornment()
	: DescribedCreatable<SphereHandleAdornment, HandleAdornment, sSphereHandleAdornment>(sSphereHandleAdornment)
	, radius(1.0f)
{}

bool SphereHandleAdornment::isCollidingWithHandle(const shared_ptr<InputObject>& inputObject)
{
	if (Workspace *workspace = Workspace::findWorkspace(this))
	{
		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject, workspace));

		CoordinateFrame worldFrame = getWorldCoordinateFrame();

		Vector3 hitLocation;
		float collision_time = G3D::CollisionDetection::collisionTimeForMovingPointFixedSphere(
			gridRay.origin(),
			gridRay.direction().unit(),
			Sphere(worldFrame.translation, radius),
			hitLocation);

		return collision_time != G3D::inf();
	}

	return false;
}

void SphereHandleAdornment::render3dAdorn(Adorn* adorn)
{
	if (!getVisible())
		return;

	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
		adorn->sphere(getWorldCoordinateFrame(), radius, Color4(color, 1.0f - transparency), zIndex, alwaysOnTop);
}

const char* const sLineHandleAdornment = "LineHandleAdornment";

static Reflection::PropDescriptor<LineHandleAdornment, float> prop_lineLength("Length", category_Data, &LineHandleAdornment::getLength, &LineHandleAdornment::setLength);
static Reflection::PropDescriptor<LineHandleAdornment, float> prop_lineThickness("Thickness", category_Data, &LineHandleAdornment::getThickness, &LineHandleAdornment::setThickness);

LineHandleAdornment::LineHandleAdornment()
	: DescribedCreatable<LineHandleAdornment, HandleAdornment, sLineHandleAdornment>(sLineHandleAdornment)
	, length(5.0f)
	, thickness(1.0f)
{}

bool LineHandleAdornment::isCollidingWithHandle(const shared_ptr<InputObject>& inputObject)
{
	if (Workspace * workspace = Workspace::findWorkspace(this))
	{
		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject, workspace));

		RBX::Camera* camera = workspace->getCamera();

		Vector4 mousePos = camera->projectPointToScreen(gridRay.origin() + gridRay.direction());
		
		CoordinateFrame worldFrame = getWorldCoordinateFrame();

		Vector4 projectedA = camera->projectPointToScreen(worldFrame.translation);
		Vector4 projectedB = camera->projectPointToScreen(worldFrame.translation + (worldFrame.lookVector() * length));

		Vector3 pointA(projectedA.x, projectedA.y, 0);
		Vector3 pointB(projectedB.x, projectedB.y, 0);
		
        Vector3 change;
        
        if ((pointB.y - pointA.y) != 0.0f)
            change = Vector3(1, -(pointB.x - pointA.x) / (pointB.y - pointA.y), 0).unit() * thickness * 0.5;
        else
            change = Vector3(0, thickness * 0.5, 0);
				
		Vector3 v0 = pointA + change;
		Vector3 v1 = pointA - change;
		Vector3 v2 = pointB + change;
		Vector3 v3 = pointB - change;

		Vector3 mouseLocation(mousePos.x, mousePos.y, 1);

		Vector3 hitLocation;
		float collision_time1 = G3D::CollisionDetection::collisionTimeForMovingPointFixedRectangle(
			mouseLocation,
			Vector3(0,0,-1),
			v0, pointA.y < pointB.y ? v2 : v1, v3, pointA.y < pointB.y? v1 : v2,
			hitLocation);

		return collision_time1 != G3D::inf();

	}
	return false;
}

void LineHandleAdornment::render3dAdorn(Adorn* adorn)
{
	if (!getVisible())
		return;

	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
	{
		CoordinateFrame cFrame = getWorldCoordinateFrame();
		adorn->line3dAA(cFrame.translation, cFrame.translation + cFrame.lookVector() * length, Color4(color, 1.0f - transparency), thickness, zIndex, alwaysOnTop);
	}
}

const char* const sImageHandleAdornment = "ImageHandleAdornment";

static Reflection::PropDescriptor<ImageHandleAdornment, Vector2> prop_imageSize("Size", category_Data, &ImageHandleAdornment::getSize, &ImageHandleAdornment::setSize);
static Reflection::PropDescriptor<ImageHandleAdornment, TextureId> prop_imageLocation("Image", category_Data, &ImageHandleAdornment::getImage, &ImageHandleAdornment::setImage);

ImageHandleAdornment::ImageHandleAdornment()
	: DescribedCreatable<ImageHandleAdornment, HandleAdornment, sImageHandleAdornment>(sImageHandleAdornment)
	, size(Vector2(1.0f, 1.0f))
	, image("rbxasset://textures/SurfacesDefault.png")
{
	color = BrickColor::brickWhite().color3();
}

bool ImageHandleAdornment::isCollidingWithHandle(const shared_ptr<InputObject>& inputObject)
{
	if (Workspace *workspace = Workspace::findWorkspace(this))
	{
		RbxRay gridRay(MouseCommand::getUnitMouseRay(inputObject, workspace));

		Vector3 hitLocation;

		CoordinateFrame worldFrame = getWorldCoordinateFrame();

		Vector3 v0 = worldFrame.pointToWorldSpace(Vector3(size.x * 0.5, size.y * 0.5, 0));
		Vector3 v1 = worldFrame.pointToWorldSpace(Vector3(-size.x * 0.5, size.y * 0.5, 0));
		Vector3 v2 = worldFrame.pointToWorldSpace(Vector3(size.x * 0.5, -size.y * 0.5, 0));
		Vector3 v3 = worldFrame.pointToWorldSpace(Vector3(-size.x * 0.5, -size.y * 0.5, 0));

		float collision_time = G3D::CollisionDetection::collisionTimeForMovingPointFixedRectangle(
			gridRay.origin(),
			gridRay.direction().unit(),
			v1, v0, v2, v3,
			hitLocation);

		return collision_time != G3D::inf();
	}

	return false;
}

void ImageHandleAdornment::render3dAdorn(Adorn* adorn)
{
	if (!getVisible())
		return;

	if (shared_ptr<PVInstance> pvInstance = adornee.lock())
	{
		bool waiting;
		TextureProxyBaseRef textureProxy = adorn->createTextureProxy(image, waiting, false, getFullName() + ".Image");

		if (!waiting && textureProxy)
		{
			adorn->setTexture(0, textureProxy);
			
			CoordinateFrame worldFrame = getWorldCoordinateFrame();

			adorn->setObjectToWorldMatrix(worldFrame);

			Vector3 v0 = Vector3(size.x / 2, size.y / 2, 0);
			Vector3 v1 = Vector3(-size.x / 2, size.y / 2, 0);
			Vector3 v2 = Vector3(size.x / 2, -size.y / 2, 0);
			Vector3 v3 = Vector3(-size.x / 2, -size.y / 2, 0);

			adorn->quad(v0, v1, v2, v3, Color4(color, 1.0f - transparency), Vector2::zero(), Vector2::one(), zIndex, alwaysOnTop);
			
			adorn->setTexture(0, TextureProxyBaseRef());
		}
	}
}

}