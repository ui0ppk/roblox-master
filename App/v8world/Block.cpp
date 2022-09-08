#include "stdafx.h"

#include "V8World/Block.h"
#include "V8World/BlockCorners.h"
#include "V8World/BlockMesh.h"
#include "Util/Math.h"
#include "G3D/CollisionDetection.h"

namespace RBX {

	using namespace POLY;

/** VERTICES are in x,y,z order, so polarity is
 vertID		x,y,z 
0			1,1,1
1			1,1,-1
2			1,-1,1
3			1,-1,-1
4			-1,1,1
5			-1,1,-1
6			-1,-1,1
7			-1,-1,-1
*/

/** FACES / PLANEIDS
0			+x
1			+y
2			+z
3			-x
4			-y
5			-z
*/

/** EDGE ID's
Same as faceID's - i.e., there are 12 edges, they correspond to the
first three faces (+x,+y,+z) as the normal direction (NORM_X, NORM_Y, NORM_Z)
along with a vertex from the face 

Every other normal is negative so that the normals on any different face (4 edges) are all
unique

EDGE		Normal		Vertex
0			X			0
1			-X			2
2			X			3
3			-X			1
4			Y			0
5			-Y			1
6			Y			5
7			-Y			4
8			Z			0
9			-Z			4
10			Z			6
11			-Z			2
*/			

/** VERTEX ORDERING ON FACES
	Vertexes are ordered so:
	1.  They proceed counter-clockwise about the face
	2.  The first zero is in the "+,+" position for the positive plane orientation

	x+ plane:		y,z coords (y right, z up)
	y+ plane:		z,x coords (z right, x up)
	z+ plane:		x,y coords (x right, y up)

	-x plane:		z,y coordinates
	-y plane:		x,z coordinates
	-z plane:		y,x coordinates
*/

const int Block::BLOCK_FACE_TO_VERTEX[6][4] = 
{
0,2,3,1,		// x+
0,1,5,4,		// y+
0,4,6,2,		// z+
4,5,7,6,		// x-
2,6,7,3,		// y-
1,3,7,5			// z-
};


/** Gives the edge that connects this vertex with the next one on the face,
	in counter-clockwise order
Index	Face	Vertex	Edge	EdgeVertex	EdgeID
0		x		0		Y		0			4
1		x		2		Z		2			11
2		x		3		Y		1			5
3		x		1		Z		0			8
4		y		0		Z		0			8
5		y		1		X		1			3
6		y		5		Z		4			9
7		y		4		X		0			0
8		z		0		X		0			0
9		z		4		Y		4			7
10		z		6		X		2			1
11		z		2		Y		0			4

12		-x		4		z		4			9
13		-x		5		y		5			6
14		-x		7		z		6			10
15		-x		6		y		4			7
16		-y		2		x		2			1
17		-y		6		z		6			10
18		-y		7		x		3			2
19		-y		3		z		2			11
20		-z		1		y		1			5
21		-z		3		x		3			2
22		-z		7		y		5			6
23		-z		5		x		1			3
*/

const int Block::BLOCK_FACE_VERTEX_TO_EDGE[6][4] = 
{
4,11,5,8,
8,3,9,0,
0,7,1,4,
9,6,10,7,
1,10,2,11,
5,2,6,3
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void Block::init()
{
	BlockMeshPool::init();
	BlockCornersPool::init();
    
	// Make sure we have token references for the default part size so that it never goes away
	// This resolves the issue of slow Block structure construction happening every time we create a part
	Vector3 initialSize = Vector3(4.0f, 1.2f, 2.0f);

	static BlockMeshPool::Token meshToken = BlockMeshPool::getToken(initialSize);
	static BlockCornersPool::Token cornersToken = BlockCornersPool::getToken(initialSize);
}

void Block::buildMesh()
{
	Vector3 key = getSize();
	blockMesh =	BlockMeshPool::getToken(key);
	mesh = 	blockMesh->getMesh();
}


void Block::setSize(const G3D::Vector3& _size)
{
	Super::setSize(_size);
	RBXASSERT(_size == getSize());

	Vector3 key = getSize() * 0.5f;

	blockCorners = BlockCornersPool::getToken(key);
	vertices = blockCorners->getVertices();

	if (bulletCollisionObject)
		updateBulletCollisionData();
}


/*
Matrix3 Block::getMomentSolid(float mass) const
{
	Vector3 size = getSize();
	float c = mass / 12.0f;

	// size if from one edge to the other
	Vector3 answer = 
		Vector3(
			c * (size.y * size.y + size.z * size.z),
			c * (size.x * size.x + size.z * size.z),
			c * (size.x * size.x + size.y * size.y)
		);

	return Math::fromDiagonal(answer);
}
*/

// See scanned calulations in V8 Technical Doc
Matrix3 Block::getMomentHollow(float mass) const
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









bool Block::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal)
{
	Vector3 halfRealSize = getSize() * 0.5;

	bool inside = false;
	return G3D::CollisionDetection::collisionLocationForMovingPointFixedAABox(
							rayInMe.origin(),
							rayInMe.direction(),
							AABox(-halfRealSize, halfRealSize),
							localHitPoint,
							inside,
                            surfaceNormal);
}

Vector3 Block::getCenterToCorner(const Matrix3& rotation) const
{
	Vector3 maxValue = G3D::abs(rotation * vertices[0]);
	for (int i = 1; i < 4; ++i) {
		maxValue = maxValue.max(G3D::abs(rotation * vertices[i]));
	}
	return maxValue;
}



float Block::getVolume() const
{
	Vector3 size = getSize();
	return size.x * size.y * size.z;
}


const Vector3* Block::getCornerPoint(const Vector3int16& clip) const {
	int x = clip.x > 0 ? 0 : 1;
	int y = clip.y > 0 ? 0 : 1;
	int z = clip.z > 0 ? 0 : 1;

	return &vertices[x*4 + y*2 + z];
}


const Vector3* Block::getEdgePoint(const Vector3int16& clip, RBX::NormalId& normalID) const {

	// normal is from the negative "zero" point positive
	if (clip.x == 0) {
		normalID = RBX::NORM_X;
		int y = clip.y > 0 ? 0 : 1;
		int z = clip.z > 0 ? 0 : 1;
		return &vertices[4 + y*2 + z];
	}
	if (clip.y == 0) {
		normalID = RBX::NORM_Y;
		int x = clip.x > 0 ? 0 : 1;
		int z = clip.z > 0 ? 0 : 1;
		return &vertices[x*4 + 2 + z];
	}
	RBXASSERT(!clip.z);
	{
		normalID = RBX::NORM_Z;
		int x = clip.x > 0 ? 0 : 1;
		int y = clip.y > 0 ? 0 : 1;
		return &vertices[x*4 + y*2 + 1];
	}
}


const Vector3* Block::getPlanePoint(const Vector3int16& clip, RBX::NormalId& normalID) const {

	// normal is from the negative "zero" point positive
	if (clip.x != 0) {
		normalID = clip.x > 0 ? RBX::NORM_X : RBX::NORM_X_NEG;
		int x = clip.x > 0 ? 0 : 1;
		return &vertices[x*4];		// either +x,+y,+z or -x,+y,+z
	}
	if (clip.y != 0) {
		normalID = clip.y > 0 ? RBX::NORM_Y : RBX::NORM_Y_NEG;
		int y = clip.y > 0 ? 0 : 1;
		return &vertices[y*2];		
	}
	RBXASSERT(clip.z);
	{
		normalID = clip.z > 0 ? RBX::NORM_Z : RBX::NORM_Z_NEG;
		int z = clip.z > 0 ? 0 : 1;
		return &vertices[z];
	}
}

GeoPairType Block::getBallBlockInfo(int onBorder, const Vector3int16 clip, const Vector3* &offset, 
										RBX::NormalId& normalID) {


	// ball plane - only clipped to one plane
	if (onBorder == 1) {
		offset = getPlanePoint(clip, normalID);
		return BALL_PLANE_PAIR;
	}
	else {
		// ball edge - clipped to two planes
		if (onBorder == 2) {
			offset = getEdgePoint(clip, normalID);
			return BALL_EDGE_PAIR;
		}
		// ball point - clipped to three planes
		else {
			offset = getCornerPoint(clip);
			return BALL_POINT_PAIR;
		}
	}
}

GeoPairType Block::getBallInsideInfo(const Vector3& ray, const Vector3* &offset, RBX::NormalId& normalID) 
{
	float min = FLT_MAX;	// used to be inf() - slow compares?
	const Vector3& l = vertices[0];		// all positive;

	for (int i = 0; i < 3; i++) {
		float temp = l[i] - ray[i];
		if (temp < min) {
			min = temp;
			normalID = static_cast<RBX::NormalId>(i);
		}
		temp = ray[i] + l[i];
		if (temp < min) {
			min = temp;
			normalID = static_cast<RBX::NormalId>(i+3);
		}
	}
	RBXASSERT(min != FLT_MAX);

	offset = (normalID > RBX::NORM_Z) ? &vertices[7] : &vertices[0];
	return BALL_PLANE_PAIR;
}

// needs to handle inside and outside point
void Block::projectToFace(Vector3& ray, Vector3int16& clip, int& onBorder) 
{
	onBorder = 0;
	const Vector3& l = vertices[0];		// all positive;

	if (ray.x > l.x) {ray.x = l.x;		onBorder++;		clip.x = 1;}
	if (ray.x < -l.x) {ray.x = -l.x;	onBorder++;		clip.x = -1;}

	if (ray.y > l.y) {ray.y = l.y;		onBorder++;		clip.y = 1;}
	if (ray.y < -l.y) {ray.y = -l.y;	onBorder++;		clip.y = -1;}

	if (ray.z > l.z) {ray.z = l.z;		onBorder++;		clip.z = 1;}
	if (ray.z < -l.z) {ray.z = -l.z;	onBorder++;		clip.z = -1;}
}

// for +x plane:		y,z coordinates
// for +y plane:		z,x coordinates
// for +z plane:		x,y coordinates
// for -x plane:		z,y coordinates
// for -y plane:		x,z coordinates
// for -z plane:		y,x coordinates

	
Vector2 Block::getProjectedVertex(const Vector3& vertex, RBX::NormalId normalID) {

	Vector2 ans;
	switch (normalID) {	
				case (RBX::NORM_X):		ans.x = vertex.y;	ans.y = vertex.z;	return ans;
				case (RBX::NORM_Y):		ans.x = vertex.z;	ans.y = vertex.x;	return ans;
				case (RBX::NORM_Z):		ans.x = vertex.x;	ans.y = vertex.y;	return ans;
				case (RBX::NORM_X_NEG):	ans.x = vertex.z;	ans.y = vertex.y;	return ans;
				case (RBX::NORM_Y_NEG):	ans.x = vertex.x;	ans.y = vertex.z;	return ans;
				case (RBX::NORM_Z_NEG):	ans.x = vertex.y;	ans.y = vertex.x;	return ans;

				// suppress compiler warning
				default:			return ans;
	}
}


int Block::getClosestEdge(const Matrix3& rotation, NormalId normalID, const Vector3& crossAxis) 
{
	Vector3 axisInBody = Math::vectorToObjectSpace(crossAxis, rotation);

	Vector2 projected = getProjectedVertex(axisInBody, normalID);

	if (projected.y > 0) {
		if (projected.x > 0)	{	return normalID*4;		}
		else					{	return normalID*4 + 1;	}
	}
	else {
		if (projected.x > 0)	{	return normalID*4 + 3;	}
		else					{	return normalID*4 + 2;	}
	}		
}

CoordinateFrame Block::getSurfaceCoordInBody( const size_t surfaceId ) const
{
	CoordinateFrame aCS;

	// the face reference coord origin is the midpoint b/t the 2 and 3 vertex (so this is the base of the side faces)
	//aCS.translation = 0.5 * (mesh->getFace(surfaceId)->getVertex(2)->getOffset() + mesh->getFace(surfaceId)->getVertex(3)->getOffset());

	// Alternative for origin to preserve pre-existing block to block grid snapping.
	// That is, don't snap to center of face's base edge, snap to the end point.
	// However, this will prevent symmetry when snapping block to special shape (i.e. prism)
	aCS.translation = mesh->getFace(surfaceId)->getVertex(2)->getOffset();
	aCS.rotation = Math::getWellFormedRotForZVector(mesh->getFace(surfaceId)->normal());

	return aCS;
}

bool Block::setUpBulletCollisionData(void)
{
	if (!bulletCollisionObject)
		updateBulletCollisionData();

	return true;
}

void Block::updateBulletCollisionData()
{
	if (!bulletCollisionObject)
		bulletCollisionObject.reset(new btCollisionObject());

	bulletBoxShape = BulletBoxShapePool::getToken(getSize());
	bulletCollisionObject->setCollisionShape(const_cast<btBoxShape*>(bulletBoxShape->getShape()));
}

} // namespace
