/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	enum NormalIdMask
	{
		NORM_NONE_MASK  = 0x00,
		NORM_X_MASK     = 0x01,
		NORM_Y_MASK     = 0x02,
		NORM_Z_MASK     = 0x04,
		NORM_X_NEG_MASK = 0x08,
		NORM_Y_NEG_MASK = 0x10,
		NORM_Z_NEG_MASK = 0x20,
		NORM_ALL_MASK   = 0x3f
	};
	enum NormalId { NORM_X = 0, 
							NORM_Y, 
							NORM_Z, 
							NORM_X_NEG, 
							NORM_Y_NEG, 
							NORM_Z_NEG,
							NORM_UNDEFINED};

	
	bool validNormalId(NormalId normalId);

	NormalIdMask normalIdToMask(NormalId normal);
	
	NormalId normalIdOpposite(NormalId normalId);
	NormalId normalIdToU(NormalId normalId);
	NormalId normalIdToV(NormalId normalId);

	const Vector3& normalIdToVector3(NormalId normalId);		// Vector pointing along the normal direction
	const Matrix3& normalIdToMatrix3(NormalId normalId);		// Z axis is away from face

	NormalId Vector3ToNormalId(const Vector3& v);
	NormalId Matrix3ToNormalId(const Matrix3& m);
	NormalId intToNormalId(int i);

	Vector3 uvwToObject(const Vector3& uvwPt, NormalId faceId);
	Vector3 objectToUvw(const Vector3& objectPt, NormalId faceId);

	template<NormalId faceId>
	Vector3 uvwToObject(const Vector3& v);

	template<NormalId faceId>
	Vector3 objectToUvw(const Vector3& v);



	// LEGACY - Deprecated
	// old stuff - need to inspect and see if it is really objectToUvw or uvwToObject
	Vector3 mapToUvw_Legacy(const Vector3& ptInObject, NormalId normalId);

	template<NormalId normalId>
	Vector3 faceMap_Legacy(const Vector3& v) {
		return uvwToObject<normalId>(v);
	}

	template<NormalId normalId>
	Vector3 faceMap_Legacy(float x, float y, float z) {
		return faceMap_Legacy<normalId>(Vector3(x, y, z));
	}

} // namespace
