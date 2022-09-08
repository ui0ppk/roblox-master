/**
	Roblox
	This is not the G3D 8.0 GCamera, instead it is from the old G3D release modified by Roblox.
  @file RbxCamera.cpp

  @author Morgan McGuire, matrix@graphics3d.com
 
  @created 2001-04-15
  @edited  2006-01-11
*/

#include "RbxG3D/RbxCamera.h"
#include "G3D/platform.h"
#include "G3D/Ray.h"
#include "G3D/Matrix4.h"

namespace RBX {


RbxCamera::RbxCamera() {
    nearPlane   = 0.1f;
    farPlane    = (float)inf();
	setFieldOfView((float)G3D::toRadians(55.0f));
}


RbxCamera::~RbxCamera() {
}


CoordinateFrame RbxCamera::coordinateFrame() const {
	return cframe;
}


void RbxCamera::getCoordinateFrame(CoordinateFrame& c) const {
	c = cframe;
}


void RbxCamera::setCoordinateFrame(const CoordinateFrame& c) {
	cframe = c;
}


void RbxCamera::setFieldOfView(float angle) {
	debugAssert((angle < G3D::pi()) && (angle > 0));

	fieldOfView = angle;

	// Solve for the corresponding image plane depth, as if the extent
	// of the film was 1x1.
	imagePlaneDepth = 1.0f / (2.0f * tanf(angle / 2.0f));
}
 

void RbxCamera::setImagePlaneDepth(
    float                                   depth,
    const class G3D::Rect2D&                     viewport) {
	
    debugAssert(depth > 0);
	setFieldOfView(2.0f * atanf(viewport.height() / (2.0f * depth)));
}


float RbxCamera::getImagePlaneDepth(
    const class G3D::Rect2D&                     viewport) const {

    // The image plane depth has been pre-computed for 
    // a 1x1 image.  Now that the image is width x height, 
    // we need to scale appropriately. 

    return imagePlaneDepth * viewport.height();
}


float RbxCamera::getViewportWidth(const G3D::Rect2D& viewport) const {
    return getViewportHeight(viewport) * viewport.width() / viewport.height();
}


float RbxCamera::getViewportHeight(const G3D::Rect2D& viewport) const {
    (void)viewport;
    return nearPlane / imagePlaneDepth;
}


// ROBLOX
RBX::RbxRay RbxCamera::worldRay(
    float                                  x,
    float                                  y,
    const G3D::Rect2D&                           viewport) const {
    
	int screenWidth  = G3D::iFloor(viewport.width());
	int screenHeight = G3D::iFloor(viewport.height());

    Vector3 origin = cframe.translation;

    float cx = screenWidth  / 2.0f;
    float cy = screenHeight / 2.0f;
    
	Vector3 direction = Vector3( (x - cx),
                -(y - cy),
                  - (getImagePlaneDepth(viewport)));
	

    direction = cframe.vectorToWorldSpace(direction);

    // Normalize the direction (we didn't do it before)
    direction = direction.direction();

	return RBX::RbxRay::fromOriginAndDirection(origin, direction);
}
// ==================


Vector3 RbxCamera::project(
    const Vector3&                      point,
    const G3D::Rect2D&                       viewport) const {

    int screenWidth  = (int)viewport.width();
    int screenHeight = (int)viewport.height();

    Vector3 out = cframe.pointToObjectSpace(point);
    float w = out.z * (-1.0f);

    if (w <= 0) {
		// provide at least basic quadrant information.
		// (helps with clipping)
		return Vector3(((out.x < 0) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity()),
						((out.y > 0) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity()),
						std::numeric_limits<float>::infinity());
    }
    debugAssert(w > 0);

    // Find where it hits an image plane of these dimensions
    float zImagePlane = getImagePlaneDepth(viewport);

    // Recover the distance
    float rhw = zImagePlane / w;

    // Add the image center, flip the y axis
    out.x = screenWidth / 2.0f - (rhw * out.x * (-1.0f));
    out.y = screenHeight / 2.0f - (rhw * out.y);
    out.z = rhw;

    return out;
}


Vector3 RbxCamera::inverseProject(
    const Vector3&                      point,
    const G3D::Rect2D&                       viewport) const 
{

    int screenWidth  = (int)viewport.width();
    int screenHeight = (int)viewport.height();

	float rhw = point.z;
	Vector3 p;
	// back to [-1..1][-1..1] coords ?
	p.x = (screenWidth/2.0f  - point.x) / (rhw * (-1.0f) );
	p.y = (screenHeight/2.0f  - point.y) / (rhw);
	
	float zImagePlane = getImagePlaneDepth(viewport);
	float w = zImagePlane / rhw;
	p.z = w * (-1.0f);

	Vector3 out = cframe.pointToWorldSpace(p);

    return out;
}

Matrix4 RbxCamera::projectionMatrix(const class G3D::Rect2D& viewport) const
{
    double pixelAspect = viewport.width() / viewport.height();

    // Half extents of viewport
    double y = -nearPlaneZ() * tan(getFieldOfView() / 2.0);
    double x = y * pixelAspect;

    double r, l, t, b, n, f;
    n = -nearPlaneZ();
    f = -farPlaneZ();
    r = x;
    l = -x;
    t = y;
    b = -y;

    return Matrix4::perspectiveProjection(l, r, b, t, n, f);
}


float RbxCamera::worldToScreenSpaceArea(float area, float z, const G3D::Rect2D& viewport) const {

    if (z >= 0) {
        return (float)inf();
    }

    float zImagePlane = getImagePlaneDepth(viewport);

	return area * (float)G3D::square(zImagePlane / z);
}


/*
double RbxCamera::getZValue(
    double              x,
    double              y,
    const class G3D::Rect2D&                     viewport    int                 width,
    int                 height,
    double              lineOffset) const {

    double depth = renderDevice->getDepthBufferValue((int)x, (int)(height - y));

    double n = -nearPlane;
    double f = -farPlane;

    // Undo the hyperbolic scaling.
    // Derivation:
    //                  a = ((1/out) - (1/n)) / ((1/f) - (1/n))
    //              depth = (1-a) * lineOffset) + (a * 1)
    //
    //              depth = lineOffset + a * (-lineOffset + 1)
    //              depth = lineOffset + (((1/z) - (1/n)) / ((1/f) - (1/n))) * (1 - lineOffset)
    // depth - lineOffset = (((1/z) - (1/n)) / ((1/f) - (1/n))) * (1 - lineOffset)
    //
    //(depth - lineOffset) / (1 - lineOffset) = (((1/z) - (1/n)) / ((1/f) - (1/n)))
    //((1/f) - (1/n)) * (depth - lineOffset) / (1 - lineOffset) = ((1/z) - (1/n))  
    //(((1/f) - (1/n)) * (depth - lineOffset) / (1 - lineOffset)) + 1/n = (1/z) 
    //
    // z = 1/( (((1/f) - (1/n)) * (depth - lineOffset) / (1 - lineOffset)) + 1/n)

    if (f >= inf) {
        // Infinite far plane
        return  1 / (((-1/n) * (depth - lineOffset) / (1 - lineOffset)) + 1/n);
    } else {
        return  1 / ((((1/f) - (1/n)) * (depth - lineOffset) / (1 - lineOffset)) + 1/n);
    }
}
*/


void RbxCamera::getClipPlanes(
    const G3D::Rect2D&       viewport,
    Array<Plane>&       clip) const {

    Frustum fr;
    frustum(viewport, fr);
	clip.resize(fr.faceArray.size(), G3D::DONT_SHRINK_UNDERLYING_ARRAY);
    for (int f = 0; f < clip.size(); ++f) {
        clip[f] = fr.faceArray[f].plane;
    }

    /*
    clip.resize(0, DONT_SHRINK_UNDERLYING_ARRAY);

    double screenWidth  = viewport.width();
    double screenHeight = viewport.height();

	// First construct the planes.  Do this in the order of near, left,
    // right, bottom, top, far so that early out clipping tests are likely
    // to end quickly.

	double fovx = screenWidth * fieldOfView / screenHeight;

	// Near (recall that nearPlane, farPlane are positive numbers, so
	// we need to negate them to produce actual z values.)
	clip.append(Plane(Vector3(0,0,-1), Vector3(0,0,-nearPlane)));

    // Right
    clip.append(Plane(Vector3(-cos(fovx/2), 0, -sin(fovx/2)), Vector3::zero()));

	// Left
	clip.append(Plane(Vector3(-clip.last().normal().x, 0, clip.last().normal().z), Vector3::zero()));

    // Top
    clip.append(Plane(Vector3(0, -cos(fieldOfView/2), -sin(fieldOfView/2)), Vector3::zero()));

	// Bottom
	clip.append(Plane(Vector3(0, -clip.last().normal().y, clip.last().normal().z), Vector3::zero()));

    // Far
    if (farPlane < inf()) {
    	clip.append(Plane(Vector3(0, 0, 1), Vector3(0, 0, -farPlane)));
    }

	// Now transform the planes to world space
	for (int p = 0; p < clip.size(); ++p) {
		// Since there is no scale factor, we don't have to 
		// worry about the inverse transpose of the normal.
        Vector3 normal;
        float d;

        clip[p].getEquation(normal, d);
		
		Vector3 newNormal = cframe.rotation * normal;
	    
        if (isFinite(d)) {
    		d = (newNormal * -d + cframe.translation).dot(newNormal);
    		clip[p] = Plane(newNormal, newNormal * d);
        } else {
            // When d is infinite, we can't multiply 0's by it without
            // generating NaNs.
            clip[p] = Plane::fromEquation(newNormal.x, newNormal.y, newNormal.z, d);
        }
	}
    */
}


RbxCamera::Frustum RbxCamera::frustum(const G3D::Rect2D& viewport) const {
    Frustum f;
    frustum(viewport, f);
    return f;
}


void RbxCamera::frustum(const G3D::Rect2D& viewport, Frustum& fr) const {

	fr.vertexPos.fastClear();
	fr.faceArray.fastClear();
    // The volume is the convex hull of the vertices definining the view
    // frustum and the light source point at infinity.

    const float x               = getViewportWidth(viewport) / 2;
    const float y               = getViewportHeight(viewport) / 2;
    const float z               = nearPlaneZ();
    const float w               = z / farPlaneZ();
	const float fovx            = x * fieldOfView / y;

    // Near face (ccw from UR)
    fr.vertexPos.append(
        Vector4( x,  y, z, 1),
        Vector4(-x,  y, z, 1),
        Vector4(-x, -y, z, 1),
        Vector4( x, -y, z, 1));

    // Far face (ccw from UR, from origin)
    fr.vertexPos.append(
        Vector4( x,  y, z, w),
        Vector4(-x,  y, z, w),
        Vector4(-x, -y, z, w),
        Vector4( x, -y, z, w));

    Frustum::Face face;

    // Near plane (wind backwards so normal faces into frustum)
	// Recall that nearPlane, farPlane are positive numbers, so
	// we need to negate them to produce actual z values.
	face.plane = Plane(Vector3(0,0,-1), Vector3(0,0,-nearPlane));
    face.vertexIndex[0] = 3;
    face.vertexIndex[1] = 2;
    face.vertexIndex[2] = 1;
    face.vertexIndex[3] = 0;
    fr.faceArray.append(face);

    // Right plane
    face.plane = Plane(Vector3(-cosf(fovx/2), 0, -sinf(fovx/2)), Vector3::zero());
    face.vertexIndex[0] = 0;
    face.vertexIndex[1] = 4;
    face.vertexIndex[2] = 7;
    face.vertexIndex[3] = 3;
    fr.faceArray.append(face);

    // Left plane
	face.plane = Plane(Vector3(-fr.faceArray.last().plane.normal().x, 0, fr.faceArray.last().plane.normal().z), Vector3::zero());
    face.vertexIndex[0] = 5;
    face.vertexIndex[1] = 1;
    face.vertexIndex[2] = 2;
    face.vertexIndex[3] = 6;
    fr.faceArray.append(face);

    // Top plane
    face.plane = Plane(Vector3(0, -cosf(fieldOfView/2.0f), -sinf(fieldOfView/2.0f)), Vector3::zero());
    face.vertexIndex[0] = 1;
    face.vertexIndex[1] = 5;
    face.vertexIndex[2] = 4;
    face.vertexIndex[3] = 0;
    fr.faceArray.append(face);

    // Bottom plane
	face.plane = Plane(Vector3(0, -fr.faceArray.last().plane.normal().y, fr.faceArray.last().plane.normal().z), Vector3::zero());
    face.vertexIndex[0] = 2;
    face.vertexIndex[1] = 3;
    face.vertexIndex[2] = 7;
    face.vertexIndex[3] = 6;
    fr.faceArray.append(face);

    // Far plane
    if (farPlane < inf()) {
    	face.plane = Plane(Vector3(0, 0, 1), Vector3(0, 0, -farPlane));
        face.vertexIndex[0] = 4;
        face.vertexIndex[1] = 5;
        face.vertexIndex[2] = 6;
        face.vertexIndex[3] = 7;
        fr.faceArray.append(face);
    }

    // Transform vertices to world space
    for (int v = 0; v < fr.vertexPos.size(); ++v) {
        fr.vertexPos[v] = cframe.toWorldSpace(fr.vertexPos[v]);
    }

	// Transform planes to world space
	for (int p = 0; p < fr.faceArray.size(); ++p) {
		// Since there is no scale factor, we don't have to 
		// worry about the inverse transpose of the normal.
        Vector3 normal;
        float d;

        fr.faceArray[p].plane.getEquation(normal, d);
		
		Vector3 newNormal = cframe.rotation * normal;
	    
		if (G3D::isFinite(d)) {
    		d = (newNormal * -d + cframe.translation).dot(newNormal);
    		fr.faceArray[p].plane = Plane(newNormal, newNormal * d);
        } else {
            // When d is infinite, we can't multiply 0's by it without
            // generating NaNs.
            fr.faceArray[p].plane = Plane::fromEquation(newNormal.x, newNormal.y, newNormal.z, d);
        }
	}
}

bool RbxCamera::Frustum::containsPoint(const Vector3& point) const
{
	for (int i = 0; i < 6; ++i) {			// ignore front, back
		const G3D::Plane& plane = faceArray[i].plane;
		if (!plane.halfSpaceContains(point)) {
			return false;
		}
	}
	return true;
}

bool RbxCamera::Frustum::intersectsSphere(const Vector3& center, float radius) const
{
	for (int i = 0; i < 6; ++i) {			// ignore front, back
		const G3D::Plane& p = faceArray[i].plane;
		G3D::Plane offsetplane(p.normal(), p.distance()- radius);
		if (!offsetplane.halfSpaceContains(center)) {
			return false;
		}
	}
	return true;
}

void RbxCamera::get3DViewportCorners(
    const G3D::Rect2D& viewport,
    Vector3& outUR,
    Vector3& outUL,
    Vector3& outLL,
    Vector3& outLR) const {

    // Must be kept in sync with frustum()
    const float sign            = (-1.0f);
    const float w               = -sign * getViewportWidth(viewport) / 2.0f;
    const float h               = getViewportHeight(viewport) / 2.0f;
    const float z               = -sign * nearPlaneZ();

    // Compute the points
    outUR = Vector3( w,  h, z);
    outUL = Vector3(-w,  h, z);
    outLL = Vector3(-w, -h, z);
    outLR = Vector3( w, -h, z);

    // Take to world space
    outUR = cframe.pointToWorldSpace(outUR);
    outUL = cframe.pointToWorldSpace(outUL);
    outLR = cframe.pointToWorldSpace(outLR);
    outLL = cframe.pointToWorldSpace(outLL);
}


void RbxCamera::setPosition(const Vector3& t) { 
    cframe.translation = t;
}


void RbxCamera::lookAt(const Vector3& position, const Vector3& up) { 
    cframe.lookAt(position, up);
}

} // namespace
