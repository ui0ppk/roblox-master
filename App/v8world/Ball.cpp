#include "stdafx.h"

#include "V8World/Ball.h"
#include "G3D/CollisionDetection.h"
#include "G3D/Sphere.h"
#include "Util/Units.h"
#include "Util/Math.h"


namespace RBX {

Matrix3 Ball::getMomentSolid(float mass) const
{
	float c = mass * (2.0f/5.0f) * realRadius * realRadius;

	return Math::fromDiagonal(Vector3(c, c, c));
}


float Ball::getVolume() const
{
	// sphere volume == 4/3 * pi * r^3
	return 1.33333333f * Math::pif() * realRadius * realRadius * realRadius;
}


bool Ball::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal)
{
	bool hit =  (G3D::CollisionDetection::collisionTimeForMovingPointFixedSphere(
							rayInMe.origin(),
							rayInMe.direction(),
							G3D::Sphere(Vector3::zero(), realRadius),
							localHitPoint,
                            surfaceNormal)
							
							!= G3D::inf()	);

	return hit;
}


void Ball::setSize(const G3D::Vector3& _size)
{
	Super::setSize(_size);
	RBXASSERT(getSize() == _size);

	realRadius = _size.x * 0.5f;	

	if (bulletCollisionObject)
		updateBulletCollisionData();
}

size_t Ball::closestSurfaceToPoint( const Vector3& pointInBody )  const
{
	float maxDotProd = pointInBody.x;
	size_t id = 0;

	if( pointInBody.y > maxDotProd )
	{
		maxDotProd = pointInBody.y;
		id = 1;
	}
	if( pointInBody.z > maxDotProd )
	{
		maxDotProd = pointInBody.z;
		id = 2;
	}
	if( -pointInBody.x > maxDotProd )
	{
		maxDotProd = -pointInBody.x;
		id = 3;
	}
	if( -pointInBody.y > maxDotProd )
	{
		maxDotProd = -pointInBody.y;
		id = 4;
	}
	if( -pointInBody.z > maxDotProd )
	{
		maxDotProd = -pointInBody.z;
		id = 5;
	}

	return id;
}

Plane Ball::getPlaneFromSurface( const size_t surfaceId ) const 
{

	switch(surfaceId)
	{
		case 0:
		default:
		{
			Vector3 normal(1.0f, 0.0f, 0.0f);
			Plane aPlane(normal, realRadius);
			return aPlane;
		}
		case 1:
		{
			Vector3 normal(0.0f, 1.0f, 0.0f);
			Plane aPlane(normal, realRadius);
			return aPlane;
		}
		case 2:
		{
			Vector3 normal(0.0f, 0.0f, 1.0f);
			Plane aPlane(normal, realRadius);
			return aPlane;
		}
		case 3:
		{
			Vector3 normal(-1.0f, 0.0f, 0.0f);
			Plane aPlane(normal, realRadius);
			return aPlane;
		}
		case 4:
		{
			Vector3 normal(0.0f, -1.0f, 0.0f);
			Plane aPlane(normal, realRadius);
			return aPlane;
		}
		case 5:
		{
			Vector3 normal(0.0f, 0.0f, -1.0f);
			Plane aPlane(normal, realRadius);
			return aPlane;
		}
	}
}

Vector3 Ball::getSurfaceNormalInBody( const size_t surfaceId ) const 
{
	switch(surfaceId)
	{
		case 0:
		default:
		{
			Vector3 normal(1.0f, 0.0f, 0.0f);
			return normal;
		}
		case 1:
		{
			Vector3 normal(0.0f, 1.0f, 0.0f);
			return normal;
		}
		case 2:
		{
			Vector3 normal(0.0f, 0.0f, 1.0f);
			return normal;
		}
		case 3:
		{
			Vector3 normal(-1.0f, 0.0f, 0.0f);
			return normal;
		}
		case 4:
		{
			Vector3 normal(0.0f, -1.0f, 0.0f);
			return normal;
		}
		case 5:
		{
			Vector3 normal(0.0f, 0.0f, -1.0f);
			return normal;
		}
	}
}

Vector3 Ball::getSurfaceVertInBody( const size_t surfaceId, const int vertId ) const 
{
	float x, y, z;
	switch(surfaceId)
	{
		case 0:
		default:
		{
			switch(vertId)
			{
				case 0:
				default:
				{
					x = realRadius; y = -realRadius; z = realRadius;
					break;
				}
				case 1:
				{
					x = realRadius; y = -realRadius; z = -realRadius;
					break;
				}
				case 2:
				{
					x = realRadius; y = realRadius; z = realRadius;
					break;
				}
				case 3:
				{
					x = realRadius; y = realRadius; z = -realRadius;
					break;
				}
			}
			Vector3 virtualVertex(x, y, z);
			return virtualVertex;
		}
		case 1:
		{
			switch(vertId)
			{
				case 0:
				default:
				{
					x = realRadius; y = realRadius; z = -realRadius;
					break;
				}
				case 1:
				{
					x = -realRadius; y = realRadius; z = -realRadius;
					break;
				}
				case 2:
				{
					x = -realRadius; y = realRadius; z = realRadius;
					break;
				}
				case 3:
				{
					x = realRadius; y = realRadius; z = realRadius;
					break;
				}
			}
			Vector3 virtualVertex(x, y, z);
			return virtualVertex;
		}
		case 2:
		{
			switch(vertId)
			{
				case 0:
				default:
				{
					x = -realRadius; y = realRadius; z = realRadius;
					break;
				}
				case 1:
				{
					x = -realRadius; y = -realRadius; z = realRadius;
					break;
				}
				case 2:
				{
					x = realRadius; y = -realRadius; z = realRadius;
					break;
				}
				case 3:
				{
					x = realRadius; y = realRadius; z = realRadius;
					break;
				}
			}
			Vector3 virtualVertex(x, y, z);
			return virtualVertex;
		}
		case 3:
		{
			switch(vertId)
			{
				case 0:
				default:
				{
					x = -realRadius; y = realRadius; z = -realRadius;
					break;
				}
				case 1:
				{
					x = -realRadius; y = -realRadius; z = -realRadius;
					break;
				}
				case 2:
				{
					x = -realRadius; y = -realRadius; z = realRadius;
					break;
				}
				case 3:
				{
					x = -realRadius; y = realRadius; z = realRadius;
					break;
				}
			}
			Vector3 virtualVertex(x, y, z);
			return virtualVertex;
		}
		case 4:
		{
			switch(vertId)
			{
				case 0:
				default:
				{
					x = -realRadius; y = -realRadius; z = realRadius;
					break;
				}
				case 1:
				{
					x = -realRadius; y = -realRadius; z = -realRadius;
					break;
				}
				case 2:
				{
					x = realRadius; y = -realRadius; z = -realRadius;
					break;
				}
				case 3:
				{
					x = realRadius; y = -realRadius; z = realRadius;
					break;
				}
			}
			Vector3 virtualVertex(x, y, z);
			return virtualVertex;
		}
		case 5:
		{
			switch(vertId)
			{
				case 0:
				default:
				{
					x = realRadius; y = -realRadius; z = -realRadius;
					break;
				}
				case 1:
				{
					x = -realRadius; y = -realRadius; z = -realRadius;
					break;
				}
				case 2:
				{
					x = -realRadius; y = -realRadius; z = realRadius;
					break;
				}
				case 3:
				{
					x = realRadius; y = -realRadius; z = realRadius;
					break;
				}
			}
			Vector3 virtualVertex(x, y, z);
			return virtualVertex;
		}
	}
		
}


size_t Ball::getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const 
{
	size_t id = 0;
	Vector3 pointInBody = objectR.transpose() * vecInWorld;
	float maxDotProd = pointInBody.x;

	if( pointInBody.y > maxDotProd )
	{
		maxDotProd = pointInBody.y;
		id = 1;
	}
	if( pointInBody.z > maxDotProd )
	{
		maxDotProd = pointInBody.z;
		id = 2;
	}
	if( -pointInBody.x > maxDotProd )
	{
		maxDotProd = -pointInBody.x;
		id = 3;
	}
	if( -pointInBody.y > maxDotProd )
	{
		maxDotProd = -pointInBody.y;
		id = 4;
	}
	if( -pointInBody.z > maxDotProd )
	{
		maxDotProd = -pointInBody.z;
		id = 5;
	}

	return id;
}

int Ball::getNumVertsInSurface( const size_t surfaceId ) const
{
	return 4;
}

bool Ball::vertOverlapsFace( const Vector3& pointInBody, const size_t surfaceId ) const
{
	switch(surfaceId)
	{
		case 0:
		case 3:
		default:
		{
			if( fabs(pointInBody.y) < realRadius && fabs(pointInBody.z) < realRadius )
				return true;
		}
		case 1:
		case 4:
		{
			if( fabs(pointInBody.x) < realRadius && fabs(pointInBody.z) < realRadius )
				return true;
		}
		case 2:
		case 5:
		{
			if( fabs(pointInBody.x) < realRadius && fabs(pointInBody.y) < realRadius )
				return true;
		}
	}

	return false;
}

CoordinateFrame Ball::getSurfaceCoordInBody( const size_t surfaceId ) const
{
	// This computes the CS for the specified surface.  It is expressed in terms of the body, not world.
	// The surface centroid is the origin and the frame is aligned with the surface normal (for z).  The y axis of
	// this frame is the projection of either the body's y or z axis, depending on which one has a more predominant projection.
	CoordinateFrame aCS;

	// Compute and set centroid
	Vector3 faceCentroid(0.0f, 0.0f, 0.0f);

	switch(surfaceId)
	{
		case 0:
		default:
			faceCentroid.x = realRadius;
			break;
		case 1:
			faceCentroid.y = realRadius;
			break;
		case 2:
			faceCentroid.z = realRadius;
			break;
		case 3:
			faceCentroid.x = -realRadius;
			break;
		case 4:
			faceCentroid.y = -realRadius;
			break;
		case 5:
			faceCentroid.z = -realRadius;
			break;
	}

	aCS.translation = faceCentroid;
	aCS.rotation = Math::getWellFormedRotForZVector(getSurfaceNormalInBody(surfaceId));

	return aCS;
}

bool Ball::setUpBulletCollisionData(void)
{
	if (!bulletCollisionObject)
		updateBulletCollisionData();

	return true;
}

void Ball::updateBulletCollisionData()
{
	if (!bulletCollisionObject)
		bulletCollisionObject.reset(new btCollisionObject());

	bulletSphereShape = BulletSphereShapePool::getToken(getSize().x);
    bulletCollisionObject->setCollisionShape(const_cast<btSphereShape*>(bulletSphereShape->getShape()));
}

} // namespace RBX
