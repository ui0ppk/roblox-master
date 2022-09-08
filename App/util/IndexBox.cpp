/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/IndexBox.h"
#include "rbx/Debug.h"
#include "Util/Math.h"



namespace RBX {
/**
	Looking from positive X back through to negative X, z is left, y is up

   0    1       4    5

   2    3       6    7
   front    back (seen through front)
 */

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


const float IndexBox::INDEXBOX_FACE_TO_NORMAL[6][3] = 
{
// x     y       z
1.0,	0.0,	0.0,
0.0,	1.0,	0.0,
0.0,	0.0,	1.0, 
-1.0,	0.0,	0.0,
0.0,	-1.0,	0.0,
0.0,	0.0,	-1.0
};

const int IndexBox::INDEXBOX_FACE_TO_VERTEX[6][4] = 
{
1,0,2,3,		// x+
0,1,5,4,		// y+
0,4,6,2,		// z+
4,5,7,6,		// x-
2,6,7,3,		// y-
1,3,7,5			// z-
};

const int IndexBox::INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[12][4] = 
{
//  v0 v1 nL nR
    0, 2, 0, 2, 
    2, 3, 0, 4,
    3, 1, 0, 5,
    1, 0, 0, 1,

    0, 4, 2, 1,
    2, 6, 4, 2,
    3, 7, 5, 4,
    1, 5, 1, 5,

    6, 4, 3, 2,
    4, 5, 3, 1,
    5, 7, 3, 5,
    7, 6, 3, 4
};


const int IndexBox::INDEXBOX_FACE_EDGE_TO_NORMAL[6][4] = 
{
1,2,4,5,		// x+:	Normals are y+, z+, y-, z-, 
0,5,3,2,		// y+:	Normals are x+, z-, x-, z+
1,3,4,0,		// z+	Normals are y+, x-, y-, x+
1,5,4,2,		// x-	Normals are y+, z-, y-, z+
2,3,5,0,		// y-	Normals are z+, x-, z-, x+
0,4,3,1			// z-	Normals are x+, y-, x-, y+
};

IndexBox::IndexBox() {
}


/**
 Sets a field on four vertices.  Used by the constructor.
 */
#define setMany(i0, i1, i2, i3, field, extreme) \
    corner[i0].field = corner[i1].field = \
    corner[i2].field = corner[i3].field = \
    (extreme).field

IndexBox::IndexBox(
    const Vector3& min,
    const Vector3& max) 
{
	RBXASSERT(Math::lessThan(min, max));

    setMany(0, 1, 2, 3, x, max);
    setMany(4, 5, 6, 7, x, min);

    setMany(0, 1, 4, 5, y, max);
    setMany(2, 3, 6, 7, y, min);

    setMany(0, 2, 4, 6, z, max);
    setMany(1, 3, 5, 7, z, min);
}

#undef setMany


Vector3 IndexBox::getCenter() const {
    Vector3 c = corner[0];

    for (int i = 1; i < 8; i++) {
        c += corner[i];
    }

    return c / 8;
}


void IndexBox::getFaceCorners(int f, Vector3& v0, Vector3& v1, Vector3& v2, Vector3& v3) const 
{
    v0 = corner[ INDEXBOX_FACE_TO_VERTEX[f][0] ];
    v1 = corner[ INDEXBOX_FACE_TO_VERTEX[f][1] ];
    v2 = corner[ INDEXBOX_FACE_TO_VERTEX[f][2] ];
    v3 = corner[ INDEXBOX_FACE_TO_VERTEX[f][3] ];

    RBXASSERT((f >= 0) && (f < 6));
}


void IndexBox::getEdge(
    int                     e,
    Vector3&           v0,
    Vector3&           v1,
    Vector3&           nL,
    Vector3&           nR) const {

    RBXASSERT(e < 12);
    v0 = corner[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][0] ];
    v1 = corner[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][1] ];
    nL = ((Vector3*)INDEXBOX_FACE_TO_NORMAL)[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][2] ];
    nR = ((Vector3*)INDEXBOX_FACE_TO_NORMAL)[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][3] ];
}


// TODO - update to newer Vector4::set() function when ROBLOX using latest G3D

void tempSetV4(const Vector3& v, float _w, Vector4& set)
{
	set.x = v.x;
	set.y = v.y;
	set.z = v.z;
	set.w = _w;
}

void IndexBox::getEdge(
    int                     e,
    Vector4&           v0,
    Vector4&           v1,
    Vector3&           nL,
    Vector3&           nR) const {

    RBXASSERT(e < 12);
//    v0.set(corner[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][0] ], 1);
//    v1.set(corner[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][1] ], 1);
    tempSetV4(corner[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][0] ], 1, v0);
    tempSetV4(corner[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][1] ], 1, v1);

    nL = ((Vector3*)INDEXBOX_FACE_TO_NORMAL)[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][2] ];
    nR = ((Vector3*)INDEXBOX_FACE_TO_NORMAL)[ INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[e][3] ];
}


void IndexBox::getTextureCornersCentered(int f, const Vector3& halfSize, Vector2& t0, Vector2& t1, 
								 Vector2& t2, Vector2& t3)
{
	float face1, face2;
	switch (f) {
		case 0:		face1 = halfSize[2];	face2 = halfSize[1];	break;
		case 1:		face1 = halfSize[2];	face2 = halfSize[0];	break;
		case 2:		face1 = halfSize[0];	face2 = halfSize[1];	break;
		case 3:		face1 = halfSize[2];	face2 = halfSize[1];	break;
		case 4:		face1 = halfSize[0];	face2 = halfSize[2];	break;
		case 5:		face1 = halfSize[1];	face2 = halfSize[0];	break;
		default:	
			RBXASSERT(false);
			face1 = 0;	
			face2 = 0;	
			break;
	}

	float minX = -face1 + 0.5f;
	float minY = -face2 + 0.5f;
	float maxX = face1 + 0.5f;
	float maxY = face2 + 0.5f;

	t0 = Vector2(minX, minY);
	t1 = Vector2(maxX, minY);
	t2 = Vector2(maxX, maxY);
	t3 = Vector2(minX, maxY);
}

void IndexBox::getTextureCornersGrid(int f, const Vector3& halfSize, Vector2& t0, Vector2& t1, 
								 Vector2& t2, Vector2& t3)
{
	float face1, face2;
	switch (f) {
		case 0:		face1 = halfSize[2];	face2 = halfSize[1];	break;
		case 1:		face1 = halfSize[2];	face2 = halfSize[0];	break;
		case 2:		face1 = halfSize[0];	face2 = halfSize[1];	break;
		case 3:		face1 = halfSize[2];	face2 = halfSize[1];	break;
		case 4:		face1 = halfSize[0];	face2 = halfSize[2];	break;
		case 5:		face1 = halfSize[1];	face2 = halfSize[0];	break;
		default: RBXASSERT(false); face1 = 0; face2 = 0; break;
	}

	float minX = 0;
	float minY = 0;
	float maxX = face1*2;
	float maxY = face2*2;

	t0 = Vector2(minX, minY);
	t1 = Vector2(maxX, minY);
	t2 = Vector2(maxX, maxY);
	t3 = Vector2(minX, maxY);
}


bool IndexBox::culledBy(const Plane* plane, int numPlanes) const {

    // See if there is one plane for which all
    // of the vertices are on the wrong side
    for (int p = 0; p < numPlanes; p++) {
        bool culled = true;
        int v = 0;

        // Assume this plane culls all points.  See if there is a point
        // not culled by the plane.
        while ((v < 8) && culled) {
            culled = !plane[p].halfSpaceContains(corner[v]);
            v++;
        }

        if (culled) {
            // This plane culled the IndexBox
            return true;
        }
    }

    // None of the planes could cull this IndexBox
    return false;
}



bool IndexBox::contains(
    const Vector3&      point) const {

    // Form axes from three edges, transform the point into that
    // space, and perform 3 interval tests

    Vector3 u = corner[4] - corner[0];
    Vector3 v = corner[3] - corner[0];
    Vector3 w = corner[1] - corner[0];

	Matrix3 M = Matrix3(u.x, v.x, w.x,
		                        u.y, v.y, w.y,
				                u.z, v.z, w.z);

    // M^-1 * (point - corner[0]) = point in unit cube's object space
    // compute the inverse of M
    Vector3 osPoint = M.inverse() * (point - corner[0]);

    return
        (osPoint.x >= 0) && 
        (osPoint.y >= 0) &&
        (osPoint.z >= 0) &&
        (osPoint.x <= 1) &&
        (osPoint.y <= 1) &&
        (osPoint.z <= 1);
}


} // namespace

