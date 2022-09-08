#include "GfxBase/ViewportBillboarder.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/Camera.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"

namespace RBX {

ViewportBillboarder::ViewportBillboarder()
    :guiScreenSize(NULL)
    ,alwaysOnTop(false)
    ,screenOffset2D(Vector2::zero())
{}
ViewportBillboarder::ViewportBillboarder(
    const Vector3& partExtentRelativeOffset, 
    const Vector3& partStudsOffset, 
    const Vector2& billboardSizeRelativeOffset,
    const UDim2& billboardSize,
    const Vector2* guiScreenSize)
    :partExtentRelativeOffset(partExtentRelativeOffset)
    ,partStudsOffset(partStudsOffset)
    ,billboardSizeRelativeOffset(billboardSizeRelativeOffset)
    ,billboardSize(billboardSize)
    ,guiScreenSize(guiScreenSize)
    ,alwaysOnTop(false)
    ,screenOffset2D(Vector2::zero())
{}

Vector2 ViewportBillboarder::getScreenOffset(const Rect2D& parentviewport, const RBX::Camera& camera, const CoordinateFrame& desiredModelView)
{
    const CoordinateFrame& cameraFrame = camera.coordinateFrame();

    Vector3 projectedTranslation = camera.project((cameraFrame * desiredModelView).translation);

    return Math::roundVector2(projectedTranslation.xy());
}

void ViewportBillboarder::update(const Rect2D& parentviewport, const Camera& camera, Vector3 partSize, CoordinateFrame partCFrame)
{
    Vector3 halfSize = partSize/2;

    Extents cameraSpaceExtents;

    // calculate extents in camera space
    for(int a = 0; a < 8; a++) 
    {
        // go through all permutations (use bits of counter)
        Vector3 extent((a & 4) ? halfSize.x : -halfSize.x,
            (a & 2) ? halfSize.y : -halfSize.y,
            (a & 1) ? halfSize.z : -halfSize.z);
        Vector3 pextent = camera.coordinateFrame().pointToObjectSpace(partCFrame.pointToWorldSpace(extent));
        cameraSpaceExtents.expandToContain(pextent);
    }

    Vector3 relativeOffsetInCameraSpace = (partExtentRelativeOffset + Vector3::one()) * 0.5f * cameraSpaceExtents.size() + cameraSpaceExtents.min();
    Vector3 billboardCenterInCameraSpace = relativeOffsetInCameraSpace + partStudsOffset;

    Vector3 billboardCenterInWorldSpace = camera.coordinateFrame().pointToWorldSpace(billboardCenterInCameraSpace);
    Vector3 billboardCenterInProjSpace = camera.project(billboardCenterInWorldSpace);

    if(billboardCenterInProjSpace.z > 0 && billboardCenterInProjSpace.z < 1000)
    {
        visibleAndValid = true;
    }
    else
    {
        visibleAndValid = false;
        return;
    }

    float pixelsPerStud = billboardCenterInProjSpace.z;
    Vector2 studsPerPixel(1.f/ pixelsPerStud, 1.f/ pixelsPerStud);   // undo the perspective effect of finding extents in screenspace.
    //  -1 : to UI coordinates (0,0 upper left, )
    Vector2 billboardSizeInStuds = (billboardSize * (Vector2::one()*pixelsPerStud)) * studsPerPixel;
    if(guiScreenSize)
        viewport = Rect2D(*guiScreenSize);
    else
        viewport = Rect2D(billboardSizeInStuds * pixelsPerStud);

    Vector2 UIToCameraSpaceScaler(billboardSizeInStuds/ viewport.wh());

    billboardCenterInCameraSpace += Vector3(billboardSizeRelativeOffset * billboardSizeInStuds, 0);

    CoordinateFrame desiredModelView;
    desiredModelView.translation = billboardCenterInCameraSpace - Vector3(billboardSizeInStuds.x * 0.5f, billboardSizeInStuds.y * -0.5f, 0); 
    desiredModelView.rotation.set(UIToCameraSpaceScaler.x, 0,0,0,UIToCameraSpaceScaler.y, 0,0,0, 1);

	if (alwaysOnTop)
		screenOffset2D = getScreenOffset(parentviewport, camera, desiredModelView);

	cframe = camera.coordinateFrame() * desiredModelView;
}

bool ViewportBillboarder::hitTest(const Vector2int16& mousePosition, const Vector2int16& windowSize, 
                                    RBX::Workspace* workspace, 
                                    Vector2& billboardMousePosition)
{
    const RBX::Camera& camera = *(workspace->getConstCamera());

	Vector3 x0y0Screen, x1y1Screen;

	if (alwaysOnTop)
	{
		x0y0Screen = Vector3(screenOffset2D, 0);
		x1y1Screen = Vector3(screenOffset2D + viewport.wh(), 0);
	}
	else
	{
		Vector3 x0y0World = cframe.pointToWorldSpace(Vector3(viewport.x0(), -viewport.y0(),0));
		Vector3 x1y1World = cframe.pointToWorldSpace(Vector3(viewport.x1(), -viewport.y1(),0));

		x0y0Screen = camera.project(x0y0World);
		x1y1Screen = camera.project(x1y1World);
	}

    Extents extents;
    extents.expandToContain(Vector3(x0y0Screen.xy(),0));
    extents.expandToContain(Vector3(x1y1Screen.xy(),0));

    if(extents.contains(Vector3(mousePosition.x, mousePosition.y, 0)))
    {
        billboardMousePosition = Vector2((mousePosition.x - x0y0Screen.x)/(x1y1Screen.x - x0y0Screen.x)*viewport.width(), (mousePosition.y - x0y0Screen.y)/(x1y1Screen.y - x0y0Screen.y)*viewport.height());

        if (alwaysOnTop) {
            return true;
        }
        ContactManager& contactManager = *workspace->getWorld()->getContactManager();
        RbxRay unitRay = camera.worldRay(mousePosition.x, mousePosition.y);
        RbxRay searchRay = RbxRay::fromOriginAndDirection(unitRay.origin(), unitRay.direction() * 2048);
        Vector3 partHitPointWorld;
        FilterInvisibleNonColliding invisibleNonCollidingObjectsFilter; // invisible & non-colliding parts don't block mouse-clicks
        if(contactManager.getHit( searchRay, NULL, &invisibleNonCollidingObjectsFilter, partHitPointWorld) == NULL)
        {
            //We didn't hit anything
            return true;
        }

        Vector3 hitPointWorld = cframe.pointToWorldSpace(Vector3(billboardMousePosition.x, billboardMousePosition.y, 0));
        Vector3 hitPointScreen = camera.project(hitPointWorld);

        Vector3 partHitPointScreen = camera.project(partHitPointWorld);
        if(partHitPointScreen.z < hitPointScreen.z)
        {
            return true;				
        }
        return false;
    }

    return false;
}

}
