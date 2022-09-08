/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "Util/NormalId.h"
#include "appdraw/HandleType.h"

namespace RBX {

	class Extents;

	class HandleHitTest
	{
	public:
		static bool hitTestHandleLocal(	const Extents& localExtents,
										const CoordinateFrame& location,
										HandleType handleType,
										const Ray& gridRay, 
										Vector3& hitPointWorld,
										NormalId& localNormalId,
										const int normalIdMask = NORM_ALL_MASK);

		static bool hitTestHandleWorld(	const Extents& worldExtents,
										HandleType handleType,
										const Ray& gridRay, 
										Vector3& hitPointWorld,
										NormalId& worldNormalId,
										const int normalIdMask = NORM_ALL_MASK);

		static bool hitTestMoveHandleWorld(const Extents& worldExtents,
										const RbxRay& gridRay, 
										Vector3& hitPointWorld,
										NormalId& worldNormalId,
										const int normalIdMask = NORM_ALL_MASK);
	};
}	// namespace RBX