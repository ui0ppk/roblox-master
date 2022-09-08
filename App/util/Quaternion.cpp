#include "stdafx.h"

/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "Util/Quaternion.h"
#include "rbx/Debug.h"
#include "Util/Math.h"

namespace RBX {

	
Quaternion& Quaternion::operator= (const Quaternion& other) 
{
	w = other.w;
	x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}


Quaternion::Quaternion(const Matrix3& rot) 
{
    float tr = rot[0][0] + rot[1][1] + rot[2][2];
    if (tr > 0.0) {
        float s = sqrt(tr + 1.0f);
        w = s * 0.5f;
        s = 0.5f / s;
        x = (rot[2][1] - rot[1][2]) * s;
        y = (rot[0][2] - rot[2][0]) * s;
        z = (rot[1][0] - rot[0][1]) * s;
    } else if ((rot[0][0] > rot[1][1]) && (rot[0][0] > rot[2][2])) {
        float s = -sqrt( 1.0f + rot[0][0] - rot[1][1] - rot[2][2]);
        x = s * 0.5f;
		s = 0.5f / s;
        y = (rot[0][1] + rot[1][0] ) * s;		// i == 0, j == 1, k == 2
        z = (rot[0][2] + rot[2][0] ) * s;
        w = (rot[2][1] - rot[1][2] ) * s;		// kj - jk == 21 - 12
    } else if (rot[1][1] > rot[2][2]) {
        float s = -sqrt( 1.0f + rot[1][1] - rot[0][0] - rot[2][2]);
        y = s * 0.5f;
		s = 0.5f / s;
        x = (rot[0][1] + rot[1][0] ) * s;		// i == 1, j == 2, k == 0
        z = (rot[1][2] + rot[2][1] ) * s;
        w = (rot[0][2] - rot[2][0] ) * s;		// kj - jk == 02 - 20
    } else {
        float s = -sqrt( 1.0f + rot[2][2] - rot[0][0] - rot[1][1] );
        z = s * 0.5f;
		s = 0.5f / s;
        x = (rot[0][2] + rot[2][0] ) * s;		// i == 2, j == 0, k == 1
        y = (rot[1][2] + rot[2][1] ) * s;
        w = (rot[1][0] - rot[0][1] ) * s;		// kj - jk == 10 - 01
    }
}

void Quaternion::toRotationMatrix(
    Matrix3&            rot) const {

	const float c = 2.0f;

    float xc = x * c;
    float yc = y * c;
    float zc = z * c;

    float xx = x * xc;
    float xy = x * yc;
    float xz = x * zc;

    float wx = w * xc;
    float wy = w * yc;
    float wz = w * zc;

    float yy = y * yc;
    float yz = y * zc;
    float zz = z * zc;

	float* mat = &rot[0][0];
	mat[0] = 1.0f - (yy + zz);
	mat[1] = xy - wz;
	mat[2] = xz + wy;
	mat[3] = xy + wz;
	mat[4] = 1.0f - (xx + zz);
	mat[5] = yz - wx;
	mat[6] = xz - wy;
	mat[7] = yz + wx;
	mat[8] = 1.0f - (xx + yy);

	//RBXASSERT(Math::isOrthonormal(rot));
}

} // namespace


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag13 = 0;
    };
};
