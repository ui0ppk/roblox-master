#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/BlockCorners.h"
#include "V8World/BlockMesh.h"
#include "V8World/BulletGeometryPoolObjects.h"

#include "V8Kernel/ContactParams.h"
#include "Util/NormalID.h"
#include "rbx/Debug.h"

namespace RBX {

	class Block : public Poly {
	friend class TriangleMesh;
	private:
		typedef Poly Super;
	public:
		typedef GeometryPool<Vector3, POLY::BlockMesh, Vector3Comparer> BlockMeshPool;
		typedef GeometryPool<Vector3, POLY::BlockCorners, Vector3Comparer> BlockCornersPool;
		typedef GeometryPool<Vector3, BulletBoxShapeWrapper, Vector3Comparer> BulletBoxShapePool;
	private:
		BlockCornersPool::Token blockCorners;
		BlockMeshPool::Token blockMesh;
		BulletBoxShapePool::Token bulletBoxShape;

		const Vector3*	vertices;	// in Real World units, object coords - shortcut to wrapper data

		static const int BLOCK_FACE_TO_VERTEX[6][4];
		static const int BLOCK_FACE_VERTEX_TO_EDGE[6][4];

		// loading GeoPair stuff
		const Vector3* getCornerPoint(const Vector3int16& clip) const;
		const Vector3* getEdgePoint(const Vector3int16& clip, NormalId& normalID) const;
		const Vector3* getPlanePoint(const Vector3int16& clip, NormalId& normalID) const;

		Matrix3 getMomentHollow(float mass) const; 

		/*override*/ void setSize(const G3D::Vector3& _size);

		// Primitive Overrides
		/*override*/ virtual bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal);

		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_BLOCK;}
		/*override*/ virtual CollideType getCollideType() const	{return COLLIDE_BLOCK;}

	public:
		// Real Corner
		/*override*/ virtual Vector3 getCenterToCorner(const Matrix3& rotation) const;

	private:
		// Moment
		/*override*/ virtual Matrix3 getMoment(float mass) const {return getMomentHollow(mass);}

		// Volume
		/*override*/ float getVolume() const;

		// Poly Overrides
		/*override*/ void buildMesh();
		
		void updateBulletCollisionData();

	public:
		Block() : vertices(NULL) {}

		~Block() {}

		static void init();

		///////////////////////////////////////////////////////////////
		// 
		// Block Specific Collision Detection
		void projectToFace(Vector3& ray, Vector3int16& clip, int& onBorder);

		GeoPairType getBallInsideInfo(const Vector3& ray, const Vector3* &offset,
											NormalId& normalID);
		GeoPairType getBallBlockInfo(int onBorder, const Vector3int16 clip, const Vector3* &offset, 
											NormalId& normalID);

		inline const float* getVertices() const {
			return (float*)vertices;
		}

		inline const Vector3& getExtent() const {
			return vertices[0];
		}

		const Vector3* getFaceVertex(NormalId faceID, int vertID) const {
			return &vertices[  BLOCK_FACE_TO_VERTEX[faceID][vertID]   ];
		}

		int getClosestEdge(const Matrix3& rotation, NormalId normalID, const Vector3& crossAxis);

		// tricky - given a face and a vertex on it, find the edge
		// assumes that the vertices are in counterclockwise order on the face,
		// and gives the edge that connects this vertex with the next one in 
		// counter-clockwise order
		inline int faceVertexToEdge(NormalId faceID, int vertID) {
			return BLOCK_FACE_VERTEX_TO_EDGE[faceID][vertID];
		}

		// same as the previsous, but gives the edge that
		// connects with the next in clockwise order
		inline int faceVertexToClockwiseEdge(NormalId faceID, int vertID) {
			return 12 + BLOCK_FACE_VERTEX_TO_EDGE[faceID][vertID];
		}

		const Vector3* getEdgeVertex(int edgeId) const {
			if (edgeId < 12) {
				return &vertices[ Block::BLOCK_FACE_TO_VERTEX[edgeId / 4][edgeId % 4] ];
			}
			else {
				int ccwEdge = edgeId - 12;		// convert to regular..
				NormalId faceId = (NormalId) (ccwEdge / 4);
				RBXASSERT(validNormalId(faceId));
				int vertId = ccwEdge+1 % 4;		// one higher - add
				return &vertices[ Block::BLOCK_FACE_TO_VERTEX[faceId][vertId] ];
			}
		}	

		// returns X,-X,X,-X,Y,-Y,Y,-Y,Z,-Z,Z,-Z
		inline NormalId getEdgeNormal(int edgeId) {
			NormalId ans = static_cast<NormalId>((edgeId / 4) + (3*(edgeId % 2)));
			if (edgeId > 12) {
				ans = static_cast<NormalId>((ans + 3) % 6);
			}
			return ans;
		}

		Vector2 getProjectedVertex(const Vector3& vertex, NormalId normalID);

		// Currently used by dragger
		/*override*/ CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;

		/*override*/ bool setUpBulletCollisionData(void);
	};

} // namespace
