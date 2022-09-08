#pragma once

#include "Util/G3DCore.h"

namespace RBX
{
    class Extents;
}

namespace RBX
{
namespace Graphics
{

class RenderCamera
{
public:
	RenderCamera();

    void setViewCFrame(const CoordinateFrame& cframe, float roll = 0);
    void setViewMatrix(const Matrix4& value);

    void setProjectionPerspective(float fovY, float aspect, float znear, float zfar);
    void setProjectionPerspective(float fovUpTan, float fovDownTan, float fovLeftTan, float fovRightTan, float znear, float zfar);
    void setProjectionOrtho(float width, float height, float znear, float zfar, bool halfPixelOffset = false);
    void setProjectionMatrix(const Matrix4& value);
    
    void changeProjectionPerspectiveZ(float znear, float zfar);

	const Matrix4& getViewMatrix() const { return view; }
	const Matrix4& getProjectionMatrix() const { return projection; }
	const Matrix4& getViewProjectionMatrix() const { return viewProjection; }

	const Vector3& getPosition() const { return position; }
    const Vector3& getDirection() const { return direction; }

    bool isVisible(const Extents& extents) const;
    bool isVisible(const Sphere& sphere) const;
    bool isVisible(const Extents& extents, const CoordinateFrame& cframe) const;

	IntersectResult intersects(const Extents& extents) const;

private:
    Vector3 position;
    Vector3 direction;
    Matrix4 view;
    Matrix4 projection;

    Matrix4 viewProjection;

    struct FrustumPlane
	{
        Vector4 plane;
        Vector3 planeAbs;

        FrustumPlane()
		{
		}

        FrustumPlane(const Vector4& plane);
	};

    FrustumPlane frustumPlanes[6];

    void updateViewProjection();
};

}
}
