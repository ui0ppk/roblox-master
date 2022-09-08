/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/NormalId.h"
#include "rbx/Debug.h"


namespace RBX {
	NormalIdMask normalIdToMask(NormalId normal)	
	{
		switch(normal){
		case NORM_X:     return NORM_X_MASK;    
		case NORM_Y:	 return NORM_Y_MASK;    
		case NORM_Z:	 return NORM_Z_MASK;
		case NORM_X_NEG: return NORM_X_NEG_MASK;
		case NORM_Y_NEG: return NORM_Y_NEG_MASK;
		case NORM_Z_NEG: return NORM_Z_NEG_MASK;
        default:         break;
		}
		return NORM_NONE_MASK;
	}
bool validNormalId(NormalId normalId)
{
	return ((normalId >= 0) && (normalId < 6)); 
}


NormalId intToNormalId(int i)
{
	return static_cast<NormalId>(i);
}

NormalId normalIdOpposite(NormalId normalId)
{
	return (NormalId)((normalId + 3) % 6);
}


NormalId normalIdToU(NormalId normalId)
{
	switch ( normalId )
    {
        case NORM_X:    
            return NORM_Z;
        case NORM_Y:
            return NORM_X;
        case NORM_Z:
            return NORM_Y;
        case NORM_X_NEG:
            return NORM_Z_NEG;
        case NORM_Y_NEG:
            return NORM_X_NEG;
        case NORM_Z_NEG:
            return NORM_Y_NEG;
        default:
            RBXASSERT(0);
            return NORM_Y;
    }
}


NormalId normalIdToV(NormalId normalId)
{
	switch ( normalId )
    {
        case NORM_X:    
            return NORM_Y;
        case NORM_Y:
            return NORM_Z;
        case NORM_Z:
            return NORM_X;
        case NORM_X_NEG:
            return NORM_Y_NEG;
        case NORM_Y_NEG:
            return NORM_Z_NEG;
        case NORM_Z_NEG:
            return NORM_X_NEG;
        default:
            RBXASSERT(0);
            return NORM_Y;
    }
}

////////////////////////////////////////////////////
//
// UVW to Object coords

template<>
Vector3 uvwToObject<NORM_X>(const Vector3& v) 
{
	return Vector3(v.z, v.y, -v.x);
}

template<>
Vector3 uvwToObject<NORM_Y>(const Vector3& v) 
{
	return Vector3(-v.x, v.z, v.y);
}

template<>
Vector3 uvwToObject<NORM_Z>(const Vector3& v) 
{
	return Vector3(v.x, v.y, v.z);
}

template<>
Vector3 uvwToObject<NORM_X_NEG>(const Vector3& v) 
{
	return Vector3(-v.z, v.y, v.x);
}

template<>
Vector3 uvwToObject<NORM_Y_NEG>(const Vector3& v) 
{
	return Vector3(v.x, -v.z, v.y);
}

template<>
Vector3 uvwToObject<NORM_Z_NEG>(const Vector3& v) 
{
	return Vector3(-v.x, v.y, -v.z);
}

////////////////////////////////////////////////////
//
// Object to UVW coords

template<>
Vector3 objectToUvw<NORM_X>(const Vector3& v) 
{
	return Vector3(-v.z, v.y, v.x);
}

template<>
Vector3 objectToUvw<NORM_Y>(const Vector3& v) 
{
	return Vector3(-v.x, v.z, v.y);
}

template<>
Vector3 objectToUvw<NORM_Z>(const Vector3& v) 
{
	return Vector3(v.x, v.y, v.z);
}

template<>
Vector3 objectToUvw<NORM_X_NEG>(const Vector3& v) 
{
	return Vector3(v.z, v.y, -v.x);
}

template<>
Vector3 objectToUvw<NORM_Y_NEG>(const Vector3& v) 
{
	return Vector3(v.x, v.z, -v.y);
}

template<>
Vector3 objectToUvw<NORM_Z_NEG>(const Vector3& v) 
{
	return Vector3(-v.x, v.y, -v.z);
}



// Given a point in object coords and a normal Id, returns the Vector3 where:
// x:  U
// y:  V
// z:  W (out // away from the face)
// in the coordinate system of the Face given by the NormalId
// Right hand rule applies
//
// For +/- X and Z faces, (front, back, left, right) - V always points up
// For +/- Y faces (top, bottom) - V always points to the back (+z)

Vector3 uvwToObject(const Vector3& uvwPt, NormalId faceId)
{
	switch (faceId)
	{
	case NORM_X:	
		return uvwToObject<NORM_X>(uvwPt);
	case NORM_Y:
		return uvwToObject<NORM_Y>(uvwPt);
	case NORM_Z:
		return uvwToObject<NORM_Z>(uvwPt);
	case NORM_X_NEG:
		return uvwToObject<NORM_X_NEG>(uvwPt);
	case NORM_Y_NEG:
		return uvwToObject<NORM_Y_NEG>(uvwPt);
	case NORM_Z_NEG:
		return uvwToObject<NORM_Z_NEG>(uvwPt);
	default:
		RBXASSERT(0);
		return Vector3::unitX();
	}
}

Vector3 objectToUvw(const Vector3& objectPt, NormalId faceId)
{
	switch (faceId)
	{
	case NORM_X:	
		return objectToUvw<NORM_X>(objectPt);
	case NORM_Y:
		return objectToUvw<NORM_Y>(objectPt);
	case NORM_Z:
		return objectToUvw<NORM_Z>(objectPt);
	case NORM_X_NEG:
		return objectToUvw<NORM_X_NEG>(objectPt);
	case NORM_Y_NEG:
		return objectToUvw<NORM_Y_NEG>(objectPt);
	case NORM_Z_NEG:
		return objectToUvw<NORM_Z_NEG>(objectPt);
	default:
		RBXASSERT(0);
		return Vector3::unitX();
	}
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////

template<NormalId normalId>
Vector3 normalIdToVector3Internal()
{
	RBXASSERT(validNormalId(normalId));
	Vector3 answer;
	int xyz = normalId % 3;
	answer[xyz] = (normalId < NORM_X_NEG) ? 1.0f : -1.0f;
	return answer;
}

const Vector3& normalIdToVector3(NormalId normalId)
{
	switch (normalId)
	{
	case NORM_X:
		{
			static Vector3 v(normalIdToVector3Internal<NORM_X>());		
			return v;
		}
	case NORM_Y:
		{
			static Vector3 v(normalIdToVector3Internal<NORM_Y>());		
			return v;
		}
	case NORM_Z:
		{
			static Vector3 v(normalIdToVector3Internal<NORM_Z>());		
			return v;
		}
	case NORM_X_NEG:
		{
			static Vector3 v(normalIdToVector3Internal<NORM_X_NEG>());	
			return v;
		}
	case NORM_Y_NEG:
		{
			static Vector3 v(normalIdToVector3Internal<NORM_Y_NEG>());	
			return v;
		}
	case NORM_Z_NEG:
		{
			static Vector3 v(normalIdToVector3Internal<NORM_Z_NEG>());	
			return v;
		}
	default:
		{
			RBXASSERT(0);
			return Vector3::zero();
		}
	}
}

Matrix3 normalIdToMatrix3Internal(NormalId normalId)
{
	Vector3 uInObject = uvwToObject(Vector3::unitX(), normalId);
	Vector3 vInObject = uvwToObject(Vector3::unitY(), normalId);
	Vector3 wInObject = uvwToObject(Vector3::unitZ(), normalId);

	RBXASSERT(uInObject == uvwToObject(objectToUvw(uInObject, normalId), normalId));
	RBXASSERT(vInObject == uvwToObject(objectToUvw(vInObject, normalId), normalId));
	RBXASSERT(wInObject == uvwToObject(objectToUvw(wInObject, normalId), normalId));

	return Matrix3(	uInObject.x,	vInObject.x,	wInObject.x,
					uInObject.y,	vInObject.y,	wInObject.y,
					uInObject.z,	vInObject.z,	wInObject.z);
}

const Matrix3& normalIdToMatrix3(NormalId normalId)
{
	switch (normalId)
	{
	case NORM_X:
		{
			static Matrix3 x = normalIdToMatrix3Internal(NORM_X);		return x;
		}
	case NORM_Y:
		{
			static Matrix3 y = normalIdToMatrix3Internal(NORM_Y);		return y;
		}
	case NORM_Z:
		{
			static Matrix3 z = normalIdToMatrix3Internal(NORM_Z);		return z;
		}
	case NORM_X_NEG:
		{
			static Matrix3 xn = normalIdToMatrix3Internal(NORM_X_NEG);	return xn;
		}
	case NORM_Y_NEG:
		{
			static Matrix3 yn = normalIdToMatrix3Internal(NORM_Y_NEG);	return yn;
		}
	case NORM_Z_NEG:
		{
			static Matrix3 zn = normalIdToMatrix3Internal(NORM_Z_NEG);	return zn;
		}
	default:
		{
			RBXASSERT(0);
			return Matrix3::identity();
		}
	}
}

NormalId Vector3ToNormalId(const Vector3& v)
{
	RBXASSERT(		(v == Vector3::unitX())
				||	(v == Vector3::unitY())
				||	(v == Vector3::unitZ())
				||	(v == -Vector3::unitX())
				||	(v == -Vector3::unitY())
				||	(v == -Vector3::unitZ())	);

	if (v.x == 1.0f) {
		return NORM_X;
	}
	else if (v.y == 1.0f) {
		return NORM_Y;
	}
	else if (v.z == 1.0f) {
		return NORM_Z;
	}
	else if (v.x == -1.0f) {
		return NORM_X_NEG;
	}
	else if (v.y == -1.0f) {
		return NORM_Y_NEG;
	}
	else if (v.z == -1.0f) {
		return NORM_Z_NEG;
	}
	else {
		RBXASSERT(0);		// This function assumes a unit, orthogally aligned Vector3
		return NORM_UNDEFINED;
	}
}

// just use the Z axis of the matrix as the normal vector
NormalId Matrix3ToNormalId(const Matrix3& m)
{
	return Vector3ToNormalId( m.column(2) );
}




// LEGACY - Deprecated
// old stuff - need to inspect and see if it is really objectToUvw or uvwToObject

// Note - this is actually doing uvwToObject
Vector3 mapToUvw_Legacy(const Vector3& ptInObject, NormalId faceId)
{
	return uvwToObject(ptInObject, faceId);
}


} // namespace
