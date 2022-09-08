#pragma once

#include "V8World/Geometry.h"
#include "V8World/Mesh.h"
#include "V8World/Block.h"
#include "V8World/BulletGeometryPoolObjects.h"
#include "Extras/ConvexDecomposition/ConvexDecomposition.h"

#include "v8world/KDTree.h"

#define PHYSICS_SERIAL_VERSION 3

namespace RBX 
{
	class CSGConvex;
	class ConvexPoly;
	class Block;
    
    class KDTreeMeshWrapper: public Allocator<KDTreeMeshWrapper>
	{
	public:
		KDTreeMeshWrapper(const std::string& str);
		~KDTreeMeshWrapper();

        const KDTree& getTree() const { return tree; }

	private:
		std::vector<Vector3> vertices;
		std::vector<unsigned int> indices;
        KDTree tree;
	};

	class TriangleMesh : public Geometry
    {
	public:
		typedef GeometryPool<std::string, BulletDecompWrapper, StringComparer> BulletDecompPool;
		typedef GeometryPool<std::string, KDTreeMeshWrapper, StringComparer> KDTreeMeshPool;

	private:
		//decompData
		int version;
		BulletDecompPool::Token compound;

		KDTreeMeshPool::Token kdTreeMesh;
        Vector3 kdTreeScale;

		// Needed for basic Dragger functions
		Block* boundingBoxMesh;

		typedef Geometry Super;
		float centerToCornerDistance;				

		Matrix3 getMomentHollow(float mass) const;

		/*override*/ void setSize(const G3D::Vector3& _size);

	public:
		TriangleMesh() : version(PHYSICS_SERIAL_VERSION), centerToCornerDistance(0.0), boundingBoxMesh(NULL)
		{			
			boundingBoxMesh = new Block();
			bulletCollisionObject.reset(new btCollisionObject());
		}			
		~TriangleMesh();

		// Getters
        const BulletDecompWrapper* getCompound() const { return compound ? &*compound : NULL; }

		int getVersion() { return version; }

		static bool validateDataVersions(const std::string& data, int& version);
		static bool validateIsBlockData(const std::string& data);

		// Physics Data Setters
		void setStaticMeshData(const std::string &key, const std::string& data, const btVector3& scale = btVector3(1.0f, 1.0f, 1.0f));
		bool setCompoundMeshData(const std::string &key, const std::string& data, const btVector3& scale = btVector3(1.0f, 1.0f, 1.0f));

		// Updates Physics Data
		void updateObjectScale(const std::string& decompKey, const std::string &decompStr, const G3D::Vector3& scale, const G3D::Vector3& meshScale = Vector3(-1, -1, -1));

		// Deserializations
		static std::string generateDecompositionData(int numTriangles, const unsigned int* triangleIndexBase, int numVertices, btScalar* vertexBase);
		static std::string generateConvexHullData(int numTriangles, const unsigned int* triangleIndexBase, int numVertices, const btVector3* vertexBase);
		static BulletDecompWrapper::ShapeType* retrieveDecomposition(const std::string& str);
		static std::string generateStaticMeshData(const std::vector<unsigned int>& indices, std::vector<btVector3>& vertices);
		static void readConvexHullData(std::vector<float> &vertices, unsigned int &numVertices, std::vector<unsigned int> &indices, unsigned int &numIndices, btTransform &trans, std::stringstream &stream);
		static void readPrefixData(btVector3 &scale, int &currentVersion, std::stringstream &stream);
		
		// HOUSEKEEPING
		static std::vector<CSGConvex> getDecompConvexes(const std::string& data, int& currentVersion, btVector3 &scale, bool dataHasScale = false);
		static void serializeConvexHullData(const btTransform& transform, const unsigned int numVertices, const float* verticesBase, 
											const unsigned int numIndices, const unsigned int* indicesBase, std::stringstream &outstream);

		// Creates decomposition data
		std::string generateDecompositionGeometry(const std::vector<btVector3> &vertices, const std::vector<unsigned int> &indices);

		// UTIL
		static const std::string getPlaceholderData();
		static const std::string getBlockData();

		// Primitive Overrides
		/*override*/ virtual bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal);

		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_TRI_MESH;}
		/*override*/ virtual CollideType getCollideType() const	{return COLLIDE_BULLET;}

		// Real Radius
		/*override*/ virtual float getRadius() const {return centerToCornerDistance;}

		// Real Corner
		/*override*/ virtual Vector3 getCenterToCorner(const Matrix3& rotation) const
		{
			if (boundingBoxMesh)
				return boundingBoxMesh->getCenterToCorner(rotation);
			else
				return Vector3(centerToCornerDistance, centerToCornerDistance, centerToCornerDistance);
		}

		// Moment
		/*override*/ virtual Matrix3 getMoment(float mass) const {
			return getMomentHollow(mass);
		}	

		// Dragger Functions
		size_t closestSurfaceToPoint( const Vector3& pointInBody ) const;
		Plane getPlaneFromSurface( const size_t surfaceId ) const;
		virtual CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;
		Vector3 getSurfaceNormalInBody( const size_t surfaceId ) const;
		size_t getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const;
		int getNumSurfaces( void ) const { return boundingBoxMesh->getMesh()->numFaces(); }
		Vector3 getSurfaceVertInBody( const size_t surfaceId, const int vertId ) const;
		int getNumVertsInSurface( const size_t surfaceId ) const;
		bool vertOverlapsFace( const Vector3& pointInBody, const size_t surfaceId ) const;

        /*override*/virtual bool findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const;
        /*override*/virtual bool FacesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const;
        /*override*/virtual bool FaceVerticesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const;
        /*override*/virtual bool FaceEdgesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const;
		
		/*override*/ bool setUpBulletCollisionData(void);

	};

	class CSGConvex
	{
	public:
		std::vector<btVector3> vertices;
		std::vector<unsigned int> indices;
		btTransform transform;
	};

	class BulletConvexDecomposition : public ConvexDecomposition::ConvexDecompInterface
	{
	private:
		std::stringstream streamChildren;  //binary string stream for vertex data
		
	public:
		BulletConvexDecomposition() {}

		virtual void ConvexDecompResult(ConvexDecomposition::ConvexResult &result);
		void addStreamChildren(std::stringstream &streamString);

	};
} // namespace
