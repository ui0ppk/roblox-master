/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#ifndef _4136E845AFB04f0d835292E319F64778
#define _4136E845AFB04f0d835292E319F64778

#include "Util/G3DCore.h"

namespace RBX {

	class Units
	{
	public:
		static inline float mPerRbx() { return 0.05f; }
		static inline float rbxPerM() { return 20.0f; }
		static inline float rbxPerMM() { return rbxPerM() * 0.001f; }
		static Vector3 kmsVelocityToRbx(const Vector3& kmsVelocity);
		static float kmsAccelerationToRbx(float kmsAccel);
		static Vector3 kmsAccelerationToRbx(const Vector3& kmsAccel);
		static float kmsForceToRbx(float kmsForce);
		static Vector3 kmsForceToRbx(const Vector3& kmsForce);
		static Vector3 kmsTorqueToRbx(const Vector3& kmsTorque);
		static float kmsDensityToRbx(float kmsDensity);
		static Vector3 kmsKRotToRbx(const Vector3& kmsKRot);
		static Vector3 kmsKRotDampToRbx(const Vector3& kmsKRotDamp);
	};
}	// namespace

#endif