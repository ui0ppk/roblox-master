#include "stdafx.h"

#include "Util/Units.h"

namespace RBX {



//////////////////////////////////////////////////////////////////////
//
// KMS to / from Rbx



Vector3 Units::kmsVelocityToRbx(const Vector3& kmsVelocity)
{
	return kmsVelocity * rbxPerM();
}

Vector3 Units::kmsAccelerationToRbx(const Vector3& kmsAccel)
{
	return kmsAccel * rbxPerM();
}

float Units::kmsAccelerationToRbx(float kmsAccel)
{
	return kmsAccel * rbxPerM();
}

Vector3 Units::kmsForceToRbx(const Vector3& kmsForce)
{
	return kmsForce * rbxPerM();
}

float Units::kmsForceToRbx(float kmsForce)
{
	return kmsForce * rbxPerM();
}

Vector3 Units::kmsTorqueToRbx(const Vector3& kmsTorque)
{
	return kmsTorque * rbxPerM() * rbxPerM();
}

Vector3 Units::kmsKRotToRbx(const Vector3& kmsKRot)
{
	return kmsKRot * rbxPerM() * rbxPerM();
}


Vector3 Units::kmsKRotDampToRbx(const Vector3& kmsKRotDamp)
{
	return kmsKRotDamp * rbxPerM() * rbxPerM();
}


float Units::kmsDensityToRbx(float kmsDensity)
{
	return kmsDensity * rbxPerM() * rbxPerM() * rbxPerM();
}



} // namespace