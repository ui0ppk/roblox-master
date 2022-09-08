#include "stdafx.h"

#include "v8world/Cylinder.h"

#include "v8world/MegaClusterPoly.h"

#include "Util/Units.h"
#include "Util/Math.h"

DYNAMIC_FASTFLAGVARIABLE(CylinderSurfaceNormalHitFix, false)


namespace RBX {

Cylinder::Cylinder()
    : realLength(0)
    , realWidth(0)
{
    bulletCollisionObject.reset(new btCollisionObject());
}

Cylinder::~Cylinder()
{
}

bool Cylinder::setUpBulletCollisionData()
{
	if (!bulletCollisionObject)
		updateBulletCollisionData();

	return true;
}

void Cylinder::updateBulletCollisionData()
{
	if (!bulletCollisionObject)
		bulletCollisionObject.reset(new btCollisionObject());

	bulletCylinderShape = BulletCylinderShapePool::getToken(Vector3(realLength, realWidth, realWidth));
    bulletCollisionObject->setCollisionShape(const_cast<btCylinderShape*>(bulletCylinderShape->getShape()));
}

Vector3 Cylinder::getCenterToCorner(const Matrix3& rotation) const
{
    // Approximate with a box
    Vector3 size(realLength, realWidth, realWidth);

    Vector3 newSize = Vector3(
                              fabs(rotation[0][0]) * size[0] + fabs(rotation[0][1]) * size[1] + fabs(rotation[0][2]) * size[2],
                              fabs(rotation[1][0]) * size[0] + fabs(rotation[1][1]) * size[1] + fabs(rotation[1][2]) * size[2],
                              fabs(rotation[2][0]) * size[0] + fabs(rotation[2][1]) * size[1] + fabs(rotation[2][2]) * size[2]);

    return newSize * 0.5f;
}

Matrix3 Cylinder::getMoment(float mass) const
{
    float radius2 = realWidth * realWidth * 0.25f;
    float length2 = realLength * realLength;

    float x = (1.f / 2.f) * mass * radius2;
    float yz = (1.f / 4.f) * mass * radius2 + (1.f / 12.f) * mass * length2;

    return Matrix3::fromDiagonal(Vector3(x, yz, yz));
}

float Cylinder::getVolume() const
{
    return Math::pif() * realWidth * realWidth * realLength * 0.25f;
}

float Cylinder::getRadius() const
{
    return (Vector3(realLength, realWidth, realWidth) * 0.5f).length();
}

int Cylinder::getNumSurfaces() const
{
    return 6;
}

bool Cylinder::findTouchingSurfacesConvex(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId) const
{
    return false;
}

bool Cylinder::FacesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const
{
    return false;
}

bool Cylinder::FaceVerticesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const
{
    return false;
}

bool Cylinder::FaceEdgesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const
{
    return false;
}

bool Cylinder::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal)
{
	if (DFFlag::CylinderSurfaceNormalHitFix)
	{
		Vector3 halfSize = getSize() * 0.5f;
		double radius = std::min<double>(halfSize.y, halfSize.z);

		if (rayInMe.origin().x < -halfSize.x)
		{
			if (rayInMe.direction().x > 0)
			{
				double distanceToPlane = (-halfSize.x - rayInMe.origin().x) / rayInMe.direction().x;
				Vector3 intersectionPoint = rayInMe.origin() + (rayInMe.direction() * distanceToPlane);

				double distance = (intersectionPoint - Vector3(-halfSize.x, 0, 0)).magnitude();

				if (distance <= radius)
				{
					surfaceNormal = Vector3(-1, 0, 0);
					localHitPoint = intersectionPoint;
					return true;
				}
			}
			else
			{
				return false;
			}
			
		}
		else if (rayInMe.origin().x > halfSize.x)
		{
			if (rayInMe.direction().x < 0)
			{
				double distanceToPlane = (halfSize.x - rayInMe.origin().x) / rayInMe.direction().x;
				Vector3 intersectionPoint = rayInMe.origin() + (rayInMe.direction() * distanceToPlane);

				double distance = (intersectionPoint - Vector3(halfSize.x, 0, 0)).magnitude();

				if (distance <= radius)
				{
					surfaceNormal = Vector3(1, 0, 0);
					localHitPoint = intersectionPoint;
					return true;
				}
			}
			else
			{
				return false;
			}
		}

		Vector3 projectedDirection = rayInMe.direction() * Vector3(0, 1, 1);
		Vector3 projectedOrigin = rayInMe.origin() * Vector3(0, 1, 1);

		double a = projectedDirection.dot(projectedDirection);
		double b = 2 * projectedOrigin.dot(projectedDirection);
		double c = projectedOrigin.dot(projectedOrigin) - (radius * radius);

		double discriminant = (b*b)-(4*a*c);

		if (discriminant < 0)
			return false;

		double distance = (-b - std::sqrt(discriminant)) / (2*a);

		if (distance > 0)
		{
			Vector3 intersectionPoint = rayInMe.origin() + (rayInMe.direction() * distance);
			if (intersectionPoint.x < halfSize.x && intersectionPoint.x > -halfSize.x)
			{
				surfaceNormal = (intersectionPoint * Vector3(0, 1, 1)).unit();
				localHitPoint = intersectionPoint;
				return true;
			}
		}

		return false;
	}
	else
	{
		if (!bulletCylinderShape)
			return false;

		const float maxDistance = MC_SEARCH_RAY_MAX;

		btTransform identityTransform;
		identityTransform.setIdentity();

		Vector3 fromVector3 = rayInMe.origin();
		btVector3 from(fromVector3.x, fromVector3.y, fromVector3.z);
		Vector3 toVector3 = rayInMe.origin() + maxDistance * rayInMe.direction();
		btVector3 to(toVector3.x, toVector3.y, toVector3.z);

		btCollisionWorld::ClosestRayResultCallback resultCallback(from, to);

		btCollisionObjectWrapper colObWrap(0, bulletCylinderShape->getShape(), getBulletCollisionObject(), identityTransform, -1, -1);

		btTransform rayFromTrans(identityTransform.getBasis(), from);
		btTransform rayToTrans(identityTransform.getBasis(), to);
		btCollisionWorld::rayTestSingleInternal(rayFromTrans, rayToTrans, &colObWrap, resultCallback);

		if (resultCallback.hasHit())
		{
			surfaceNormal = Vector3(resultCallback.m_hitNormalWorld.x(), resultCallback.m_hitNormalWorld.y(), resultCallback.m_hitNormalWorld.z());
			localHitPoint = Vector3(resultCallback.m_hitPointWorld.x(), resultCallback.m_hitPointWorld.y(), resultCallback.m_hitPointWorld.z());
			return true;
		}

		return false;
	}
}

void Cylinder::setSize(const G3D::Vector3& _size)
{
    Super::setSize(_size);
    RBXASSERT(getSize() == _size);

    realLength = _size.x;
    realWidth = std::min(_size.y, _size.z);

    if (bulletCollisionObject)
        updateBulletCollisionData();
}

size_t Cylinder::closestSurfaceToPoint(const Vector3& pointInBody)  const
{
    Vector3 size(realLength, realWidth, realWidth);

    size_t surface = 0;
    
    for (size_t i = 1; i < 6; ++i)
    {
        // This is not very efficient but the function should only be used in dragger code so we don't care too much
        float di = getPlaneFromSurface(i).distance(pointInBody);
        float ds = getPlaneFromSurface(surface).distance(pointInBody);
        
        if (fabsf(di) > fabsf(ds))
            surface = i;
    }
    
    return surface;
}

Plane Cylinder::getPlaneFromSurface(const size_t surfaceId) const
{
    Vector3 normal = Math::getWorldNormal(static_cast<NormalId>(surfaceId), Matrix3::identity());
    float distance = dot(normal, Vector3(realLength, realWidth, realWidth) * 0.5f);

    return Plane(normal, fabsf(distance));
}

Vector3 Cylinder::getSurfaceVertInBody(const size_t surfaceId, const int vertId) const
{
    Vector3 size(realLength, realWidth, realWidth);
    Extents extents(-size * 0.5f, size * 0.5f);

    Vector3 surface[4];
    extents.getFaceCorners(static_cast<NormalId>(surfaceId), surface[0], surface[1], surface[2], surface[3]);

    return surface[vertId];
}

size_t Cylinder::getMostAlignedSurface(const Vector3& vecInWorld, const G3D::Matrix3& objectR) const
{
    return Math::getClosestObjectNormalId(vecInWorld, objectR);
}

int Cylinder::getNumVertsInSurface(const size_t surfaceId) const
{
    return 4;
}

bool Cylinder::vertOverlapsFace(const Vector3& pointInBody, const size_t surfaceId) const
{
    if (surfaceId == NORM_X || surfaceId == NORM_X_NEG)
        return pointInBody.yz().squaredLength() <= realWidth * realWidth * 0.25f;
    else if (surfaceId == NORM_Y || surfaceId == NORM_Y_NEG)
        return fabsf(pointInBody.x) <= realLength * 0.5f && fabsf(pointInBody.z) <= realWidth * 0.5f;
    else if (surfaceId == NORM_Z || surfaceId == NORM_Z_NEG)
        return fabsf(pointInBody.x) <= realLength * 0.5f && fabsf(pointInBody.y) <= realWidth * 0.5f;
    else
        return false;
}

CoordinateFrame Cylinder::getSurfaceCoordInBody(const size_t surfaceId) const
{
    float sign = (surfaceId >= NORM_X_NEG) ? -1 : 1;

    Vector3 faceCentroid;
    faceCentroid.x = (surfaceId == NORM_X || surfaceId == NORM_X_NEG) ? sign * realLength * 0.5f : 0;
    faceCentroid.y = (surfaceId == NORM_Y || surfaceId == NORM_Y_NEG) ? sign * realWidth * 0.5f : 0;
    faceCentroid.z = (surfaceId == NORM_Z || surfaceId == NORM_Z_NEG) ? sign * realWidth * 0.5f : 0;

    Matrix3 rotation = Math::getWellFormedRotForZVector(getSurfaceNormalInBody(surfaceId));

    return CoordinateFrame(rotation, faceCentroid);
}

Vector3 Cylinder::getSurfaceNormalInBody(const size_t surfaceId) const
{
    return Math::getWorldNormal(static_cast<NormalId>(surfaceId), Matrix3::identity());
}

} // namespace RBX
