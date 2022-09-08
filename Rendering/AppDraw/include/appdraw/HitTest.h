 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"

namespace RBX {
	
	class Part;

	class HitTest {
	private:
		// hitTests
		static bool hitTestBox(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal);
		static bool hitTestBall(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal); 
		static bool hitTestCylinder(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal);

	public:
		static bool hitTest(
			const Part&		part,
			RbxRay&			rayInPartCoords, 
			Vector3&		hitPointInPartCoords,
			float			gridToReal);
	};

} // namespace

