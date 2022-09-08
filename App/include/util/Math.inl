#pragma once

namespace RBX {
	namespace Math{

// = mat.transpose() * vec
Vector3 vectorToObjectSpace(const Vector3& _vec, const Matrix3& _mat) 
{
	const float* vec = &_vec[0];
	const float* mat = &_mat[0][0];

	return Vector3 (	mat[0]*vec[0] + mat[3]*vec[1] + mat[6]*vec[2],
						mat[1]*vec[0] + mat[4]*vec[1] + mat[7]*vec[2],
						mat[2]*vec[0] + mat[5]*vec[1] + mat[8]*vec[2]	);
}

	}	// namespace
}	// namespace
