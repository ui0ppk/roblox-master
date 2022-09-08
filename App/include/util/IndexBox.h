/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/G3DCore.h"


namespace RBX {

class IndexBox {

private:
    /**
		Looking from positive X back through to negative X, z is left, y is up
       0    1       4    5
    
       2    3       6    7
       front    back (seen through front)
     */

    Vector3 corner[8];
public:

    IndexBox();
    IndexBox(
        const Vector3&      min,
        const Vector3&      max);

    virtual ~IndexBox() {}

    Vector3 getCenter() const;

    Vector3 getCorner(int i) const {
        return corner[i];
    }

	Vector3 getFaceNormal(int f) const {
		return Vector3(	INDEXBOX_FACE_TO_NORMAL[f][0], 
						INDEXBOX_FACE_TO_NORMAL[f][1], 
						INDEXBOX_FACE_TO_NORMAL[f][2]);	
	} 
	
	Vector3 getEdgeNormal(int f, int e) const {
		return getFaceNormal(INDEXBOX_FACE_EDGE_TO_NORMAL[f][e]);
	}

    /**
     Returns the four corners of a face (0 <= f < 6).
     The corners are returned to form a counter clockwise quad facing outwards.
     */
    void getFaceCorners(
        int                 f,
        Vector3&            v0,
        Vector3&            v1,
        Vector3&            v2,
        Vector3&            v3) const;

    /** The edge travels from v0 to v1. nR is to the right and nL is to the left.*/
    void getEdge(
        int                     e,
        Vector3&           v0,
        Vector3&           v1,
        Vector3&           nL,
        Vector3&           nR) const;

    void getEdge(
        int                e,
        Vector4&           v0,
        Vector4&           v1,
        Vector3&           nL,
        Vector3&           nR) const;

	static void getTextureCornersCentered(
		int					f,
		const Vector3&		halfSize, 
		Vector2&			t0, 
		Vector2&			t1,
		Vector2&			t2,
		Vector2&			t3);

	static void getTextureCornersGrid(
		int					f,
		const Vector3&		halfSize, 
		Vector2&			t0, 
		Vector2&			t1,
		Vector2&			t2,
		Vector2&			t3);
	/**
     Returns true if this IndexBox is culled by the provided set of 
     planes.  The IndexBox is culled if there exists at least one plane
     whose halfspace the entire IndexBox is not in.
     */
    bool culledBy(
		const Plane*  plane,
        int                 numPlanes) const;


    bool contains(
        const Vector3&      point) const;

	static const int INDEXBOX_FACE_TO_VERTEX[6][4];
	static const float INDEXBOX_FACE_TO_NORMAL[6][3]; 
	static const int INDEXBOX_FACE_EDGE_TO_NORMAL[6][4];

    /** normal indices are into INDEXBOX_FACE_TO_NORMAL array */
    static const int INDEXBOX_EDGE_TO_VERTEX_AND_NORMALS[12][4];
};

} // namespace
