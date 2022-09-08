#pragma once

#include "V8DataModel/Workspace.h"
#include "util/UDim.h"

namespace RBX {

class ViewportBillboarder
{
private:
    CoordinateFrame cframe;
    Rect2D viewport;
    bool visibleAndValid;
    Vector2 screenOffset2D;

    Vector2 getScreenOffset(const Rect2D& parentviewport, const RBX::Camera& camera, const CoordinateFrame& desiredModelView);

public:

    Vector3 partExtentRelativeOffset;
    Vector3 partStudsOffset;
    Vector2 billboardSizeRelativeOffset;
    UDim2 billboardSize;
    const Vector2* guiScreenSize;
    bool alwaysOnTop;

    ViewportBillboarder();
    ViewportBillboarder(const Vector3& partExtentRelativeOffset, 
        const Vector3& partStudsOffset, 
        const Vector2& billboardSizeRelativeOffset,
        const UDim2& billboardSize, //studs* x + pixels
        const Vector2* guiScreenSize //null for pixel-exact.
        );

    void update(const Rect2D& parentviewport, const Camera& camera, Vector3 partSize, CoordinateFrame partCFrame);

    bool hitTest(const Vector2int16& mousePosition, const Vector2int16& windowSize, 
        RBX::Workspace* workspace, Vector2& billboardMousePosition);

    const Vector2& getScreenOffset() const { return screenOffset2D; }

    bool isVisibleAndValid() const
    {
        return visibleAndValid;
    }

    const Rect2D& getViewport() const
    {
        return viewport;
    }

    const CoordinateFrame& getCoordinateFrame() const
    {
        return cframe;
    }
};

};
