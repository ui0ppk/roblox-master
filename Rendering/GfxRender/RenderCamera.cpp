#include "stdafx.h"
#include "RenderCamera.h"

namespace RBX
{
namespace Graphics
{

static Matrix4 projectionOrtho(float width, float height, float znear, float zfar, bool halfPixelOffset = false)
{
    // Note: this maps to [0..1] Z range
    float q = 1 / (zfar - znear);
    float qn = -znear * q;

    float wOffset = 0;
    float hOffset = 0;

    if (halfPixelOffset)
    {
        wOffset = 1.0f / width;
        hOffset = 1.0f / height;
    }

    return
		Matrix4(
            2 / width, 0, 0,  -1 - wOffset,
            0, 2 / height, 0, -1 + hOffset,
            0, 0, q, qn,
            0, 0, 0, 1);
}

static Matrix4 projectionPerspective(float fovY, float aspect, float znear, float zfar)
{
    float h = 1 / tanf(fovY / 2);
    float w = h / aspect;

    // Note: this maps to [0..1] Z range
    float q = -zfar / (zfar - znear);
    float qn = znear * q;

    return
		Matrix4(
            w, 0, 0, 0,
            0, h, 0, 0,
            0, 0, q, qn,
            0, 0, -1, 0);
}

static Matrix4 projectionPerspective(float fovUpTan, float fovDownTan, float fovLeftTan, float fovRightTan, float znear, float zfar)
{
	float sx = 2.f / (fovLeftTan + fovRightTan);
	float ox = (fovRightTan - fovLeftTan) / (fovLeftTan + fovRightTan);
	float sy = 2.f / (fovUpTan + fovDownTan);
	float oy = (fovUpTan - fovDownTan) / (fovUpTan + fovDownTan);

    // Note: this maps to [0..1] Z range
    float q = -zfar / (zfar - znear);
    float qn = znear * q;

    return
		Matrix4(
            sx, 0,  ox, 0,
            0,  sy, oy, 0,
            0,  0,  q, qn,
            0, 0,  -1, 0);
}

RenderCamera::FrustumPlane::FrustumPlane(const Vector4& plane)
	: plane(plane)
	, planeAbs(G3D::abs(plane.x), G3D::abs(plane.y), G3D::abs(plane.z))
{
}

RenderCamera::RenderCamera()
{
}

void RenderCamera::setViewCFrame(const CoordinateFrame& cframe, float roll)
{
	position = cframe.translation;
    direction = -cframe.rotation.column(2);
	view = Matrix4::rollDegrees(G3D::toDegrees(roll)) * cframe.inverse().toMatrix4();

	updateViewProjection();
}

void RenderCamera::setViewMatrix(const Matrix4& value)
{
	position = (value.inverse() * Vector4(0, 0, 0, 1)).xyz();
	direction = (value.inverse() * Vector4(0, 0, -1, 0)).xyz();
    view = value;

	updateViewProjection();
}

void RenderCamera::setProjectionPerspective(float fovY, float aspect, float znear, float zfar)
{
	projection = projectionPerspective(fovY, aspect, znear, zfar);

	updateViewProjection();
}

void RenderCamera::setProjectionPerspective(float fovUpTan, float fovDownTan, float fovLeftTan, float fovRightTan, float znear, float zfar)
{
	projection = projectionPerspective(fovUpTan, fovDownTan, fovLeftTan, fovRightTan, znear, zfar);

	updateViewProjection();
}

void RenderCamera::setProjectionOrtho(float width, float height, float znear, float zfar, bool halfPixelOffset /*= false*/)
{
	projection = projectionOrtho(width, height, znear, zfar, halfPixelOffset);

	updateViewProjection();
}

void RenderCamera::setProjectionMatrix(const Matrix4& value)
{
	projection = value;

	updateViewProjection();
}

void RenderCamera::changeProjectionPerspectiveZ(float znear, float zfar)
{
    // Note: this maps to [0..1] Z range
    float q = -zfar / (zfar - znear);
    float qn = znear * q;
    
    projection[2][2] = q;
    projection[2][3] = qn;
	projection[3][2] = -1;
	projection[3][3] = 0;
    
    updateViewProjection();
}

bool RenderCamera::isVisible(const Extents& extents) const
{
	Vector4 center = Vector4(extents.center(), 1);
	Vector3 extent = extents.size() * 0.5f;

    for (int i = 0; i < 6; ++i)
	{
		const FrustumPlane& p = frustumPlanes[i];

		float d = p.plane.dot(center);
		float r = p.planeAbs.dot(extent);

        // box is in negative half-space => outside the frustum
        if (d + r < 0)
			return false;
	}

	return true;
}

bool RenderCamera::isVisible(const Sphere& sphere) const
{
	Vector4 center = Vector4(sphere.center, 1);

	float r = sphere.radius;

    for (int i = 0; i < 6; ++i)
	{
		const FrustumPlane& p = frustumPlanes[i];

		float d = p.plane.dot(center);

        // sphere is in negative half-space => outside the frustum
        if (d + r < 0)
			return false;
	}

	return true;
}

bool RenderCamera::isVisible(const Extents& extents, const CoordinateFrame& cframe) const
{
    return isVisible(extents.toWorldSpace(cframe));
}

IntersectResult RenderCamera::intersects(const Extents& extents) const
{
	Vector4 center = Vector4(extents.center(), 1);
	Vector3 extent = extents.size() * 0.5f;

	IntersectResult result = irFull;

    for (int i = 0; i < 6; ++i)
	{
		const FrustumPlane& p = frustumPlanes[i];

		float d = p.plane.dot(center);
		float r = p.planeAbs.dot(extent);

        // box is in negative half-space => outside the frustum
        if (d + r < 0)
			return irNone;

        // box intersects the plane => not fully inside the frustum
		if (d - r < 0)
			result = irPartial;
	}

	return result;
}

static Vector4 normalize3(const Vector4& v)
{
	return v / v.xyz().length();
}

void RenderCamera::updateViewProjection()
{
    viewProjection = projection * view;

    // near: z >= 0
	frustumPlanes[0] = normalize3(viewProjection.row(2));
    // far: z <= w
	frustumPlanes[1] = normalize3(viewProjection.row(3) - viewProjection.row(2));
    // left: x >= -w
	frustumPlanes[2] = normalize3(viewProjection.row(3) + viewProjection.row(0));
    // right: x <= w
	frustumPlanes[3] = normalize3(viewProjection.row(3) - viewProjection.row(0));
    // bottom: y >= -w
	frustumPlanes[4] = normalize3(viewProjection.row(3) + viewProjection.row(1));
    // top: y <= w
	frustumPlanes[5] = normalize3(viewProjection.row(3) - viewProjection.row(1));
}

}
}
