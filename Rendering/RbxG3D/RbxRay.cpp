/**
 @file RbxRay.cpp 
 
 @maintainer Morgan McGuire, matrix@graphics3d.com
 
 @created 2002-07-12
 @edited  2004-03-19
 // ROBLOX: from the old g3d (previous to upgrade to g3d 8.0). This version make our code happier.
 // It doesn't requires to have unit() vectors. 
 // We should fix this and remove de need of thise file in the future and use the G3D ones.
 */

//#include "G3D/platform.h"
#include "RbxG3D/RbxRay.h"
#include "G3D/CollisionDetection.h"

namespace RBX {


RbxRay RbxRay::refract(
    const Vector3&  newOrigin,
    const Vector3&  normal,
    float           iInside,
    float           iOutside) const {

    Vector3 D = m_direction.refractionDirection(normal, iInside, iOutside);
    return RbxRay::fromOriginAndDirection(
        newOrigin + (m_direction + normal * (float)sign(m_direction.dot(normal))) * 0.001f, D);
}


RbxRay RbxRay::reflect(
    const Vector3&  newOrigin,
    const Vector3&  normal) const {
    
    Vector3 D = m_direction.reflectionDirection(normal);
    return RbxRay::fromOriginAndDirection(newOrigin + (D + normal) * 0.001f, D);
}   
        


Vector3 RbxRay::intersectionPlane(const Plane& plane) const {
/*
    float d;
    Vector3 normal = plane.normal();
    plane.getEquation(normal, d);
    float rate = m_direction.dot(normal);
    
    if (rate >= 0.0f) {
        return Vector3::inf();
    } else {
        float t = -(d + m_origin.dot(normal)) / rate;

        return m_origin + m_direction * t;
    }
*/
    Vector3 planeNormal = plane.normal();
    float rate = m_direction.dot(planeNormal);

    if (fabs(rate) < 1e-6f) {       // parallel
        return Vector3::inf();
    }
    else {
        float distanceOnRay = (plane.distance() - m_origin.dot(planeNormal)) / rate;
        if (distanceOnRay >= 0.0f) {
            return m_origin + m_direction * distanceOnRay;
        }
        else {
            return Vector3::inf();          // other side of ray
        }
    }
}       
    
    
float RbxRay::intersectionTime(const class Sphere& sphere) const {
    Vector3 dummy;
	return G3D::CollisionDetection::collisionTimeForMovingPointFixedSphere(
            m_origin, m_direction, sphere, dummy);
}       
            
        
float RbxRay::intersectionTime(const class Plane& plane) const {
    Vector3 dummy;
	return G3D::CollisionDetection::collisionTimeForMovingPointFixedPlane(
            m_origin, m_direction, plane, dummy);
}
    
    
float RbxRay::intersectionTime(const class Box& box) const {
    Vector3 dummy;
	float time = G3D::CollisionDetection::collisionTimeForMovingPointFixedBox(
            m_origin, m_direction, box, dummy);

    if ((time == inf()) && (box.contains(m_origin))) {
       return 0.0f;
    } else {
        return time;
    }
}


float RbxRay::intersectionTime(const class AABox& box) const {
    Vector3 dummy;
    bool inside;
	float time = G3D::CollisionDetection::collisionTimeForMovingPointFixedAABox(
            m_origin, m_direction, box, dummy, inside);

    if ((time == inf()) && inside) {
        return 0.0f;
    } else {
        return time;
    }
}

}