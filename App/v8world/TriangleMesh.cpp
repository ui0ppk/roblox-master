#include "stdafx.h"


#include "V8World/TriangleMesh.h"
#include "G3D/CollisionDetection.h"
#include "G3D/Sphere.h"
#include "Util/Units.h"
#include "Util/Math.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "V8World/Tolerance.h"
#include "V8World/MegaClusterPoly.h"

#include "LinearMath/btConvexHull.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.h"
#include "Extras/ConvexDecomposition/ConvexBuilder.h"

FASTFLAG(CSGPhysicsLevelOfDetailEnabled)

namespace CSGPhys
{
	static const char* blockDataKey = "BLOCK";
	static const unsigned int blockKeyLen = 5;
};

namespace RBX {

const std::string physicsKeyTag("CSGPHS");

KDTreeMeshWrapper::KDTreeMeshWrapper(const std::string& str)
{
	btVector3 scale;
	int currentVersion;

	std::vector<CSGConvex> meshConvexes = TriangleMesh::getDecompConvexes(str, currentVersion, scale);

	for (auto& convex: meshConvexes)
	{
		unsigned int indexOffset = vertices.size();

		for (auto& v: convex.vertices)
		{
            btVector3 tv = convex.transform * v;
			vertices.push_back(Vector3(tv.x(), tv.y(), tv.z()));
		}

		for (auto& i: convex.indices)
			indices.push_back(indexOffset + i);
	}

    if (!vertices.empty() && !indices.empty())
        tree.build(&vertices[0], NULL, vertices.size(), &indices[0], indices.size() / 3);
}

KDTreeMeshWrapper::~KDTreeMeshWrapper()
{
}

TriangleMesh::~TriangleMesh()
{
	if (boundingBoxMesh)
		delete boundingBoxMesh;
}

void TriangleMesh::setStaticMeshData(const std::string& key, const std::string& data, const btVector3& scale)
{
	if (!validateDataVersions(data, version))
		return;

	kdTreeMesh = KDTreeMeshPool::getToken(key, data);
	kdTreeScale = Vector3(scale.x(), scale.y(), scale.z());
}

bool validateDataHeader(std::stringstream& streamOut)
{
	// Read Header
	char fileId[6]={0};
	streamOut.read(fileId, 6);

	if (strncmp(fileId, "CSGPHS", 6) != 0)
	{
		// error
		return false;
	}

	return true;
}

bool TriangleMesh::validateDataVersions(const std::string& data, int& version)
{
	std::stringstream stream(data);

	BOOST_STATIC_ASSERT(sizeof(version) == 4);

	// Read Header
	int currentVersion = PHYSICS_SERIAL_VERSION;
    
	if (FFlag::CSGPhysicsLevelOfDetailEnabled)
	{
		if (!validateDataHeader(stream))
		{
			version = -1;
			return false;
		}
	}
	else
	{
		char fileId[6]={0};
		stream.read(fileId, 6);
		if (strncmp(fileId, "CSGPHS", 6) != 0)
		{
			// error
			version = -1;
			return false;
		}
	}

	stream.read(reinterpret_cast<char*>(&version), sizeof(int));
    if (version != currentVersion)
    {
        return false;
    }
	return true;
}

bool TriangleMesh::validateIsBlockData(const std::string& data)
{
	std::stringstream stream(data);
	if (!validateDataHeader(stream))
	{
		return false;
	}

	int version;
	stream.read(reinterpret_cast<char*>(&version), sizeof(int));
	if (version != 0)
	{
		return false;
	}

	char blockKeyword[CSGPhys::blockKeyLen] = {0};
	stream.read(blockKeyword, CSGPhys::blockKeyLen);
	if (strncmp(blockKeyword, CSGPhys::blockDataKey, CSGPhys::blockKeyLen) == 0)
	{
		return true;
	}

	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent,
		GA_CATEGORY_STUDIO,  "CSGPhys", "CSGPhys_PlaceholderData", 0, false));
	return false;
}

std::string TriangleMesh::generateDecompositionGeometry(const std::vector<btVector3> &vertices, const std::vector<unsigned int> &indices)
{
	if (vertices.size() > 0 && indices.size() > 0)
	{
		// Reset Physics version, otherwise process placeholder data will loop endlessly
		version = PHYSICS_SERIAL_VERSION;
		std::string decompStr = generateDecompositionData(indices.size()/3, &indices[0], vertices.size(), (btScalar*) &vertices[0].x());
		return decompStr;
	}
	return "";
}

Matrix3 TriangleMesh::getMomentHollow(float mass) const
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


bool TriangleMesh::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal)
{
	if (!kdTreeMesh)
		return false;

	const float maxDistance = MC_SEARCH_RAY_MAX;
	
	Vector3 from = rayInMe.origin() / kdTreeScale;
	Vector3 to = (rayInMe.origin() + maxDistance * rayInMe.direction()) / kdTreeScale;
	
	KDTree::RayResult result;
	kdTreeMesh->getTree().queryRay(result, from, to);

	if (result.hasHit())
	{
		surfaceNormal = result.tree->getTriangleNormal(result.triangle);
		localHitPoint = (from + result.fraction * (to - from)) * kdTreeScale;
		return true;
	}

	return false;
}

void TriangleMesh::setSize(const G3D::Vector3& _size)
{
	Super::setSize(_size);
	RBXASSERT(getSize() == _size);

	centerToCornerDistance = 0.5f * _size.magnitude();	

	boundingBoxMesh->setSize(_size);
}

bool TriangleMesh::setCompoundMeshData(const std::string& key, const std::string& data, const btVector3& scale)
{
	if (!validateDataVersions(data, version))
		return false;

	//Convert Mesh Scale to String and append it to str before passing it off to geometry pool
	std::stringstream scaleStream;
	unsigned int scaleStride = sizeof(btVector3);
	scaleStream.write(reinterpret_cast<const char*>(&scaleStride), sizeof(unsigned int));
	scaleStream.write(reinterpret_cast<const char*>(&scale), scaleStride);
	std::string scaleStr(scaleStream.str().c_str(), scaleStream.str().size());

	// look at geometry pool for the equivalent decomp data before creating a new one
	try
	{
		compound = BulletDecompPool::getToken(scaleStr + key, scaleStr + data);
	}
	catch (std::exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Solid Model - failed to load physics data: %s", e.what());
		return false;
	}

	return true;
}

bool TriangleMesh::setUpBulletCollisionData(void)
{
	if (compound)
	{
		bulletCollisionObject->setCollisionShape(const_cast<BulletDecompWrapper::ShapeType*>(compound->getCompound()));
		return true;
	}
	else
		return false;
}

// Update Physics Data
void TriangleMesh::updateObjectScale(const std::string& decompKey,const std::string& decompStr, const G3D::Vector3& scale, const G3D::Vector3& meshScale)
{
	if (decompStr != "")
	{
		setCompoundMeshData(decompKey, decompStr, btVector3(scale.x, scale.y, scale.z));
		if (meshScale.x > 0.0f && meshScale.y > 0.0f && meshScale.z > 0.0f)
		{
			setStaticMeshData(decompKey, decompStr, btVector3(meshScale.x, meshScale.y, meshScale.z));
			return;
		}
		setStaticMeshData(decompKey, decompStr, btVector3(scale.x, scale.y, scale.z));
	}
}

// Serialization/Deserialization
std::string TriangleMesh::generateDecompositionData(int numTriangles, const unsigned int* triangleIndexBase, int numVertices, btScalar* vertexBase)
{
		int currentVersion;
		std::stringstream decompStream; 
		BulletConvexDecomposition	convexDecomposition;
		//-----------------------------------
		// Bullet Convex Decomposition
		//-----------------------------------
		btAlignedObjectArray<float> vertices;
		btAlignedObjectArray<unsigned int> indices;
		vertices.reserve(numVertices*3);
		indices.reserve(numTriangles*3);

		for(int vi = 0;vi<numVertices; vi++)
		{
			int index = vi*4;
			btVector3 vec;
			vec[0] = vertexBase[index];
			vec[1] = vertexBase[index+1];
			vec[2] = vertexBase[index+2];
			vertices.push_back(vec[0]);
			vertices.push_back(vec[1]);
			vertices.push_back(vec[2]);
		}

		for(int i = 0;i<numTriangles;i++)
		{
			int index = i*3;
			unsigned int i0,i1,i2;
			i0 = triangleIndexBase[index];
			i1 = triangleIndexBase[index+1];
			i2 = triangleIndexBase[index+2];
			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);
		}

		unsigned int depth;
		unsigned int maxVerticesPerHull;
		float cpercent;
		float ppercent;
		float skinWidth  = 0.0;

		depth				= 6;
		cpercent			= 5;
		ppercent			= 5;
		maxVerticesPerHull	= 32;


		ConvexDecomposition::DecompDesc desc;
		desc.mVcount       = numVertices;
		desc.mVertices     = &vertices[0];
		desc.mTcount       = numTriangles;
		desc.mIndices      = &indices[0];
		desc.mDepth        = depth;
		desc.mCpercent     = cpercent;
		desc.mPpercent     = ppercent;
		desc.mMaxVertices  = maxVerticesPerHull;
		desc.mSkinWidth    = skinWidth;

		desc.mCallback = &convexDecomposition;
		desc.mCallbackOnlyNeedsHull = true;

		ConvexBuilder cb(desc.mCallback);
		cb.process(desc);

		currentVersion = PHYSICS_SERIAL_VERSION;

		// Generate String Stream
		decompStream.write("CSGPHS", 6);
		decompStream.write(reinterpret_cast<const char*>(&currentVersion), sizeof(currentVersion));


		// Add convex child data to stream
		convexDecomposition.addStreamChildren(decompStream);

		std::string decompData(decompStream.str().c_str(), decompStream.str().size());

		unsigned int emptyLength = physicsKeyTag.size() + sizeof(currentVersion);

		if (decompData.length() <= emptyLength)
		{
			throw std::runtime_error("Generated empty Decomposition");
		}

		return decompData;
}

std::string TriangleMesh::generateConvexHullData(int numTriangles, const unsigned int* triangleIndexBase, int numVertices, const btVector3* vertexBase)
{
	int currentVersion;
	std::stringstream decompStream; 

	HullDesc hd;
	hd.mFlags = QF_TRIANGLES;
	hd.mVcount = static_cast<unsigned int>(numVertices);
	hd.mVertices = vertexBase;
	hd.mVertexStride = sizeof(btVector3);

	HullLibrary hl;
	HullResult hr;
	if (hl.CreateConvexHull (hd, hr) == QE_FAIL)
	{
		throw std::runtime_error("Could not generate convex hull");
	}
	else
	{
		currentVersion = PHYSICS_SERIAL_VERSION;

		// Generate String Stream
		decompStream.write("CSGPHS", 6);
		decompStream.write(reinterpret_cast<const char*>(&currentVersion), sizeof(currentVersion));

		// Create Placeholder Translation
		btTransform trans;
		trans.setIdentity();

		btAlignedObjectArray<float>			vertFloats;
		vertFloats.reserve(hr.mNumOutputVertices*3);

		// Have to align here for serialization because btVector3 is 4-Dim
		for(unsigned int vi = 0;vi< hr.mNumOutputVertices; vi++)
		{
			vertFloats.push_back(hr.m_OutputVertices[vi][0]);
			vertFloats.push_back(hr.m_OutputVertices[vi][1]);
			vertFloats.push_back(hr.m_OutputVertices[vi][2]);
		}

		unsigned int numVertexFloat = hr.mNumOutputVertices * 3;
		unsigned int numIndices = hr.mNumIndices;
		serializeConvexHullData(trans, numVertexFloat, &vertFloats[0], numIndices, &hr.m_Indices[0], decompStream);

		std::string decompData(decompStream.str());

		unsigned int emptyLength = physicsKeyTag.size() + sizeof(currentVersion);

		if (decompData.length() <= emptyLength)
		{
			throw std::runtime_error("Generated empty Decomposition");
		}

		return decompData;
	}
}

BulletDecompWrapper::ShapeType* TriangleMesh::retrieveDecomposition(const std::string& str)
{
	BulletDecompWrapper::ShapeType* decomp = new BulletDecompWrapper::ShapeType();

	btVector3 scale;
	int currentVersion;

	std::vector<CSGConvex> meshConvexes = getDecompConvexes(str, currentVersion, scale, true);
	for (unsigned int i = 0; i < meshConvexes.size(); i++)
	{
		btAlignedObjectArray<btVector3> vertices;
		btTransform trans;

		btCollisionShape* convexShape = new btConvexHullShape(
			&( meshConvexes[i].vertices[0].getX()), meshConvexes[i].vertices.size(),sizeof(btVector3));
		convexShape->setLocalScaling(scale);
		convexShape->setMargin(bulletCollisionMargin);

		decomp->addChildShape(meshConvexes[i].transform,convexShape);
	}

	// Generation got bad data
	if (decomp->getNumChildShapes() == 0)
	{
		delete decomp;
		throw std::runtime_error("Decomp data has no children");
	}
#ifdef USE_GIMPACT
	decomp->updateBound();
#endif
	return decomp;
}

std::string TriangleMesh::generateStaticMeshData(const std::vector<unsigned int>& indices, std::vector<btVector3>& vertices)
{

	std::stringstream stream;    
    unsigned int numVertices = vertices.size();
	unsigned int vertexStride = sizeof(btVector3);
    stream.write(reinterpret_cast<const char*>(&numVertices), sizeof(unsigned int));
    stream.write(reinterpret_cast<const char*>(&vertexStride), sizeof(unsigned int));
	stream.write(reinterpret_cast<const char*>(&vertices[0]), vertexStride * numVertices);

    unsigned int numIndices = indices.size();
    stream.write(reinterpret_cast<const char*>(&numIndices), sizeof(unsigned int));
    stream.write(reinterpret_cast<const char*>(&indices[0]), sizeof(unsigned int) * numIndices);
    
    std::string buffer(stream.str().c_str(), stream.str().size());

    return buffer;
}

void TriangleMesh::readPrefixData(btVector3 &scale, int &currentVersion, std::stringstream &stream)
{
	// Read Scale
	unsigned int scaleStride = 0;
	stream.read(reinterpret_cast<char*>(&scaleStride), sizeof(unsigned int));
	stream.read(reinterpret_cast<char*>(&scale), scaleStride);

	// Read Version
	char fileId[6]={0};
	stream.read(fileId, 6);
	stream.read(reinterpret_cast<char*>(&currentVersion), sizeof(int));

}

void TriangleMesh::readConvexHullData(std::vector<float> &vertices, unsigned int &numVertices, std::vector<unsigned int> &indices, unsigned int &numIndices, btTransform &trans, std::stringstream &stream)
{
	btVector3 transformTrans;
	btQuaternion transformRot;
	unsigned int translationStride = 0;
	unsigned int rotationStride = 0;

	// Read Transform
	stream.read(reinterpret_cast<char*>(&translationStride), sizeof(unsigned int));
	stream.read(reinterpret_cast<char*>(&transformTrans), translationStride);
	stream.read(reinterpret_cast<char*>(&rotationStride), sizeof(unsigned int));
	stream.read(reinterpret_cast<char*>(&transformRot), rotationStride);

	// Set Transform
	trans.setOrigin(transformTrans);
	trans.setRotation(transformRot);

	// Read Vertices
	unsigned int vertexStride = 0;
	stream.read(reinterpret_cast<char*>(&numVertices), sizeof(unsigned int));
	stream.read(reinterpret_cast<char*>(&vertexStride), sizeof(unsigned int));
	vertices.resize(numVertices);
	stream.read(reinterpret_cast<char*>(&vertices[0]), vertexStride * numVertices);

	// Read Indices
	stream.read(reinterpret_cast<char*>(&numIndices), sizeof(unsigned int));
	indices.resize(numIndices);
	stream.read(reinterpret_cast<char*>(&indices[0]), sizeof(unsigned int) * numIndices);
}

void TriangleMesh::serializeConvexHullData(const btTransform& transform, const unsigned int numVertices, const float* verticesBase, 
											const unsigned int numIndices, const unsigned int* indicesBase, std::stringstream &outstream)
{
	btVector3 transformTrans = transform.getOrigin();
	btQuaternion transformRot = transform.getRotation();

	// serialization
	// - Translation 
	unsigned int translationStride = sizeof(btVector3);
	unsigned int rotationStride = sizeof(btQuaternion);
	outstream.write(reinterpret_cast<const char*>(&translationStride), sizeof(unsigned int));
	outstream.write(reinterpret_cast<const char*>(&transformTrans), translationStride);
	outstream.write(reinterpret_cast<const char*>(&rotationStride), sizeof(unsigned int));
	outstream.write(reinterpret_cast<const char*>(&transformRot), rotationStride);


	// - Vertices
	unsigned int vertexStride = sizeof(float);
	outstream.write(reinterpret_cast<const char*>(&numVertices), sizeof(unsigned int));
	outstream.write(reinterpret_cast<const char*>(&vertexStride), sizeof(unsigned int));
	outstream.write(reinterpret_cast<const char*>(verticesBase), vertexStride * numVertices);

	// - Indices
	outstream.write(reinterpret_cast<const char*>(&numIndices), sizeof(unsigned int));
	outstream.write(reinterpret_cast<const char*>(indicesBase), sizeof(unsigned int) * numIndices);
}

std::vector<CSGConvex> TriangleMesh::getDecompConvexes(const std::string& data, int& currentVersion, btVector3 &scale, bool dataHasScale)
{
	std::vector<CSGConvex> outputConvexes;
	std::stringstream stream(data);

	if (dataHasScale)
	{
		// Read Scale
		unsigned int scaleStride = 0;
		stream.read(reinterpret_cast<char*>(&scaleStride), sizeof(unsigned int));
		stream.read(reinterpret_cast<char*>(&scale), scaleStride);
	}
	else
	{
		scale = btVector3(1,1,1);
	}

	// Read Version
	char fileId[6]={0};
	stream.read(fileId, 6);
	stream.read(reinterpret_cast<char*>(&currentVersion), sizeof(int));

	while(stream.rdbuf()->in_avail() > 0)
	{
		CSGConvex currentConvex; 
	    btTransform trans;
		unsigned int numVertices = 0;
		unsigned int numIndices = 0;
		std::vector<float> hullVertices;
		btVector3 transformTrans;
		btQuaternion transformRot;
		unsigned int translationStride = 0;
		unsigned int rotationStride = 0;

		// Read Transform
		stream.read(reinterpret_cast<char*>(&translationStride), sizeof(unsigned int));
		stream.read(reinterpret_cast<char*>(&transformTrans), translationStride);
		stream.read(reinterpret_cast<char*>(&rotationStride), sizeof(unsigned int));
		stream.read(reinterpret_cast<char*>(&transformRot), rotationStride);

		// Set Transform
		trans.setOrigin(transformTrans);
		trans.setRotation(transformRot);

		// Read Vertices
		unsigned int vertexStride = 0;
		stream.read(reinterpret_cast<char*>(&numVertices), sizeof(unsigned int));
		stream.read(reinterpret_cast<char*>(&vertexStride), sizeof(unsigned int));
		hullVertices.resize(numVertices);
		stream.read(reinterpret_cast<char*>(&hullVertices[0]), vertexStride * numVertices);

		// Read Indices
		stream.read(reinterpret_cast<char*>(&numIndices), sizeof(unsigned int));
		currentConvex.indices.resize(numIndices);
		stream.read(reinterpret_cast<char*>(&currentConvex.indices[0]), sizeof(unsigned int) * numIndices);

		for (unsigned int i=0; i<(numVertices/3); i++)
		{
			btVector3 vertex(hullVertices[i*3], hullVertices[i*3+1], hullVertices[i*3+2]);

			currentConvex.vertices.push_back(vertex);
		}

		currentConvex.transform = trans;
		outputConvexes.push_back(currentConvex);
	}
	return outputConvexes;
}


void BulletConvexDecomposition::ConvexDecompResult(ConvexDecomposition::ConvexResult &result)
{
	// Create Placeholder Translation
	btTransform trans;
	trans.setIdentity();

	if (FFlag::CSGPhysicsLevelOfDetailEnabled)
	{
		unsigned int numVertexFloats = result.mHullVcount * 3;
		unsigned int numIndices = result.mHullTcount * 3;
		TriangleMesh::serializeConvexHullData(trans, numVertexFloats, &result.mHullVertices[0], numIndices, &result.mHullIndices[0], streamChildren);
	}
	else
	{
		btVector3 transformTrans = trans.getOrigin();
		btQuaternion transformRot = trans.getRotation();

		// serialization
		// - Translation 
		unsigned int translationStride = sizeof(btVector3);
		unsigned int rotationStride = sizeof(btQuaternion);
		streamChildren.write(reinterpret_cast<const char*>(&translationStride), sizeof(unsigned int));
		streamChildren.write(reinterpret_cast<const char*>(&transformTrans), translationStride);
		streamChildren.write(reinterpret_cast<const char*>(&rotationStride), sizeof(unsigned int));
		streamChildren.write(reinterpret_cast<const char*>(&transformRot), rotationStride);


		// - Vertices
		unsigned int numVertices = result.mHullVcount * 3;
		unsigned int vertexStride = sizeof(float);
		streamChildren.write(reinterpret_cast<const char*>(&numVertices), sizeof(unsigned int));
		streamChildren.write(reinterpret_cast<const char*>(&vertexStride), sizeof(unsigned int));
		streamChildren.write(reinterpret_cast<const char*>(&result.mHullVertices[0]), vertexStride * numVertices);

		// - Indices
		unsigned int numIndices = result.mHullTcount * 3;
		streamChildren.write(reinterpret_cast<const char*>(&numIndices), sizeof(unsigned int));
		streamChildren.write(reinterpret_cast<const char*>(&result.mHullIndices[0]), sizeof(unsigned int) * numIndices);
	}
}

const std::string TriangleMesh::getPlaceholderData()
{
	std::stringstream decompStream;
	int nonDecompVersion = 0;
	decompStream.write("CSGPHS", 6);
	decompStream.write(reinterpret_cast<const char*>(&nonDecompVersion), sizeof(nonDecompVersion));
	std::string decompData(decompStream.str());
	return decompData;
}

const std::string TriangleMesh::getBlockData()
{
	std::string result = getPlaceholderData();
	result.append(CSGPhys::blockDataKey);
	return result;
}

void BulletConvexDecomposition::addStreamChildren(std::stringstream &streamString)
{
	streamString.write(streamChildren.str().c_str(), streamChildren.str().size());
}

size_t TriangleMesh::closestSurfaceToPoint( const Vector3& pointInBody ) const
{
	return boundingBoxMesh->closestSurfaceToPoint(pointInBody);
}

Plane TriangleMesh::getPlaneFromSurface( const size_t surfaceId ) const 
{
	return boundingBoxMesh->getPlaneFromSurface(surfaceId);
}

Vector3 TriangleMesh::getSurfaceNormalInBody( const size_t surfaceId ) const 
{
	return boundingBoxMesh->getSurfaceNormalInBody(surfaceId);
}

Vector3 TriangleMesh::getSurfaceVertInBody( const size_t surfaceId, const int vertId ) const 
{
	return boundingBoxMesh->getSurfaceVertInBody(surfaceId, vertId);
}

size_t TriangleMesh::getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const 
{
	return boundingBoxMesh->getMostAlignedSurface(vecInWorld, objectR);
}

int TriangleMesh::getNumVertsInSurface( const size_t surfaceId ) const
{
	return boundingBoxMesh->getNumVertsInSurface(surfaceId);
}

bool TriangleMesh::vertOverlapsFace( const Vector3& pointInBody, const size_t surfaceId ) const
{
	return boundingBoxMesh->vertOverlapsFace(pointInBody, surfaceId);
}

CoordinateFrame TriangleMesh::getSurfaceCoordInBody( const size_t surfaceId ) const
{
	return boundingBoxMesh->getSurfaceCoordInBody(surfaceId);
}

bool TriangleMesh::findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const
{
	return boundingBoxMesh->findTouchingSurfacesConvex(myCf, myFaceId, otherGeom, otherCf, otherFaceId);
}

bool TriangleMesh::FacesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const
{
	return boundingBoxMesh->FacesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol);
}

bool TriangleMesh::FaceVerticesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const
{
	return boundingBoxMesh->FaceVerticesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol);
}

bool TriangleMesh::FaceEdgesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const
{
	return boundingBoxMesh->FaceEdgesOverlapped(myCf, myFaceId, otherGeom, otherCf, otherFaceId, tol);
}

} // namespace RBX


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag11 = 0;
    };
};
