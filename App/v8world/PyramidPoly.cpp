#include "stdafx.h"

#include "V8World/PyramidPoly.h"
#include "V8World/Mesh.h"

#include "V8World/GeometryPool.h"
#include "V8World/PyramidMesh.h"
#include "Util/Math.h"


namespace RBX {

void PyramidPoly::buildMesh()
{
	Vector3_2Ints key;
	key.vectPart = getSize();
	key.int1 = numSides;
	key.int2 = numSlices;

	pyramidMesh =	PyramidMeshPool::getToken(key);

	mesh = 	pyramidMesh->getMesh();

}

void PyramidPoly::setGeometryParameter(const std::string& parameter, int value)
{
	if (parameter == "NumSides") {
		setNumSides(value);
	}
	else if (parameter == "NumSlices") {
		setNumSlices(value);
	}
	else {
		RBXASSERT(0);
	}
}

int PyramidPoly::getGeometryParameter(const std::string& parameter) const
{
	if (parameter == "NumSides") {
		return numSides;
	}
	else if (parameter == "NumSlices") {
		return numSlices;
	}
	else {
		RBXASSERT(0);
		return 0;
	}
}


void PyramidPoly::setNumSides( int num ) 
{ 
	if (num != numSides) {
		numSides = num;
		buildMesh();
	}
}

void PyramidPoly::setNumSlices( int num ) 
{ 
	if (num != numSlices) {
		numSlices = num;
		buildMesh();
	}
}

Matrix3 PyramidPoly::getMoment(float mass) const
{
	Vector3 size = getSize();

	float area = 2 * (size.x * size.y + size.y * size.z + size.z * size.x);
	Vector3 I;
	for (int i = 0; i < 3; i++) {
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;
		float x = size[i];		// main axis;
		float y = size[j];
		float z = size[k];
		
		float Ix = (mass / (2.0f * area)) * (		(y*y*y*z/3.0f)
												+	(y*z*z*z/3.0f)
												+	(x*y*z*z)
												+	(x*y*y*y/3.0f)
												+	(x*y*y*z)
												+	(x*z*z*z/3.0f)	);
		I[i] = Ix;
	}
	return Math::fromDiagonal(I);
}

Vector3 PyramidPoly::getCofmOffset( void ) const
{
    return pyramidMesh->GetLocalCofMFromMesh();
}

CoordinateFrame PyramidPoly::getSurfaceCoordInBody( const size_t surfaceId ) const
{
	CoordinateFrame aCS;

	// Set rotation
	aCS.rotation = Math::getWellFormedRotForZVector(mesh->getFace(surfaceId)->normal());

	// Set origin
	Vector3 bMin, bMax, bCenter;
	Vector3 xF = aCS.rotation * Vector3::unitX();
	Vector3 yF = aCS.rotation * Vector3::unitY();

	// For  bottom surface use the center of the oriented bounding box.
	// For side surfaces use centroid, but still use the bottom bounding box for snapping offset.
	// This gives pyramid a continuous snapping experience as you drag from bottom to side.
	float originAdjustX = 0.0f;
	float originAdjustY = 0.0f;
	if( surfaceId == mesh->numFaces() - 1 )
	{
		mesh->getFace(surfaceId)->getOrientedBoundingBox(xF, yF, bMin, bMax, bCenter);
		aCS.translation = bCenter;
		// Adjust origin to account for even or odd stud pattern
		originAdjustX = Math::evenWholeNumberFuzzy((bMin - bMax).dot(xF)) ? 0.0f : 0.5f;
		originAdjustY = Math::evenWholeNumberFuzzy((bMin - bMax).dot(yF)) ? 0.0f : 0.5f;
	}
	else
	{
		mesh->getFace(mesh->numFaces() - 1)->getOrientedBoundingBox(xF, yF, bMin, bMax, bCenter);
		aCS.translation = (mesh->getFace(surfaceId)->getVertex(0)->getOffset() + mesh->getFace(surfaceId)->getVertex(1)->getOffset()) * 0.5;
		originAdjustX = Math::evenWholeNumberFuzzy((bMin - bMax).dot(xF)) ? 0.0f : 0.5f;
	}

	aCS.translation += originAdjustX * xF + originAdjustY * yF;

	return aCS;

}
size_t PyramidPoly::getFaceFromLegacyNormalId( const NormalId nId ) const
{
	// return -1 if does not exist
	// *** NOTE: Pyramids with more than 8 sides don't have their sides mapped to legacy normals.
	RBXASSERT( mesh->numFaces() > 3 );
	size_t faceId = -1;
	switch(nId)
	{
		case NORM_X:
			switch(numSides)
			{
				case 3:
				case 4:
				case 5:
				case 6:
					faceId = 1;
					break;
				case 8:
					faceId = 2;
					break;
				default:
					faceId = -1;
					break;
			}
			break;
		case NORM_X_NEG:
			switch(numSides)
			{
				case 3:
					faceId = 2;
					break;
				case 4:
					faceId = 3;
					break;
				case 5:
				case 6:
					faceId = 4;
					break;
				case 8:
					faceId = 6;
					break;
				default:
					faceId = -1;
					break;
			}
			break;
		case NORM_Y:
			faceId = -1;
			break;
		case NORM_Y_NEG:
			faceId = mesh->numFaces() - 1;
			break;
		case NORM_Z:
			switch(numSides)
			{
				case 3:
				case 4:
				case 5:
				case 6:
				case 8:
					faceId = 0;
					break;
				default:
					faceId = -1;
					break;
			}
			break;
		case NORM_Z_NEG:
			switch(numSides)
			{
				case 4:
				case 5:
					faceId = 2;
					break;
				case 6:
					faceId = 3;
					break;
				case 8:
					faceId = 4;
					break;
				case 3:
				default:
					faceId = -1;
					break;
			}
			break;
        default:
            break;
	}

	return faceId;
}

} // namespace RBX