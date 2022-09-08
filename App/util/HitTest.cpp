/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/HitTest.h"
#include "Util/Math.h"
#include "Util/Extents.h"
#include "AppDraw/DrawAdorn.h"
#include "AppDraw/HitTest.h"
#include "G3D/CollisionDetection.h"
#include "G3D/Capsule.h"
#include "v8datamodel/FastLogSettings.h"

namespace RBX {

bool HandleHitTest::hitTestHandleWorld(	const Extents& worldExtents,
									    HandleType handleType,
										const RbxRay& gridRay, 
										Vector3& hitPointWorld,
										NormalId& worldNormalId,
										const int normalIdMask)
{
	CoordinateFrame location(worldExtents.center());
	Extents localExtents = Extents::fromCenterCorner(Vector3::zero(), worldExtents.size() * 0.5);

	return hitTestHandleLocal(	localExtents,
								location,
								handleType,
								gridRay,
								hitPointWorld,
								worldNormalId,
								normalIdMask);
}

bool HandleHitTest::hitTestHandleLocal(	const Extents& localExtents,
										const CoordinateFrame& location,
										HandleType handleType,
										const RbxRay& gridRay, 
										Vector3& hitPointWorld,
										NormalId& localNormalId,
										const int normalIdMask)
{
    for (int i = 0; i < 6; ++i)
    {
        if(normalIdMask & (1<<i))
        {
            if (handleType == HANDLE_MOVE)
            {
                const NormalId normalId = intToNormalId(i);
                const Vector3 handlePos = DrawAdorn::handlePosInObject(
                    gridRay.origin(),
                    localExtents,
                    HANDLE_MOVE,
                    normalId );
                const Vector3 handleInWorld = location.pointToWorldSpace(handlePos);
                const float handleRadius = DrawAdorn::scaleHandleRelativeToCamera(
                    gridRay.origin(),
                    HANDLE_MOVE,
                    handleInWorld );

                const Vector3 arrow = location.vectorToWorldSpace(normalIdToVector3(normalId) * handleRadius);
                float capsuleRadius = (RBX::ClientAppSettings::singleton().GetValueAxisAdornmentGrabSize()/100.0f) * handleRadius;
                
                Vector3 hitPoint;

                if ( G3D::CollisionDetection::collisionTimeForMovingPointFixedCapsule(
                        gridRay.origin(), 
                        gridRay.direction(), 
                        G3D::Capsule(handleInWorld,handleInWorld + arrow, capsuleRadius), 
                        hitPoint ) != G3D::inf() ) 
                {
                    localNormalId = normalId;
                    hitPointWorld = hitPoint;
                    return true;
                }
            }
            else
            {
                Vector3 localCenter = DrawAdorn::handlePosInObject(
                    gridRay.origin(),
                    localExtents, 
                    handleType, 
                    intToNormalId(i) );

                Vector3 worldCenter = location.pointToWorldSpace(localCenter);

                float handleRadius = DrawAdorn::scaleHandleRelativeToCamera(
                    gridRay.origin(),
                    handleType,
                    worldCenter);

                Vector3 tempHitPoint;

                float collision_time = G3D::CollisionDetection::collisionTimeForMovingPointFixedSphere(
                    gridRay.origin(),
                    gridRay.direction(),
                    Sphere(worldCenter, handleRadius),
                    tempHitPoint );

                if ( collision_time != G3D::inf() ) 
                {
                    localNormalId = NormalId(i);
                    hitPointWorld = worldCenter;
                    return true;
                }
            }
        }
    }
    return false;
}

bool HandleHitTest::hitTestMoveHandleWorld(const Extents& worldExtents,
										const RbxRay& gridRay, 
										Vector3& hitPointWorld,
										NormalId& worldNormalId,
										const int normalIdMask)
{
	const CoordinateFrame location(worldExtents.center());
    const Vector3 halfSize = worldExtents.size() / 2;
    const Extents localExtents(-halfSize,halfSize);

	for (int i = 0; i < 6; ++i) 
	{
		if(normalIdMask & (1<<i))
		{
			const NormalId normalId = intToNormalId(i);
            const Vector3 handlePos = DrawAdorn::handlePosInObject(
                gridRay.origin(),
                localExtents,
                HANDLE_MOVE,
                normalId );
            const Vector3 handleInWorld = location.pointToWorldSpace(handlePos);
            const float handleRadius = DrawAdorn::scaleHandleRelativeToCamera(
                gridRay.origin(),
                HANDLE_MOVE,
                handleInWorld );

            const Vector3 arrow = normalIdToVector3(normalId) * handleRadius;
			float capsuleRadius = (RBX::ClientAppSettings::singleton().GetValueAxisAdornmentGrabSize()/100.0f) * handleRadius;
            
            Vector3 hitPoint;

            if ( G3D::CollisionDetection::collisionTimeForMovingPointFixedCapsule(
                    gridRay.origin(), 
                    gridRay.direction(), 
                    G3D::Capsule(handleInWorld,handleInWorld + arrow, capsuleRadius), 
                    hitPoint ) != G3D::inf() ) 
            {
                worldNormalId = normalId;
                hitPointWorld = hitPoint;
                return true;
            }
		}
	}
	return false;
}

}	// namespace RBX
