/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/CSGMesh.h"

#include "V8World/TriangleMesh.h"

#include "util/Lcmrand.h"

#include <algorithm>
#include <fstream>
#include <streambuf>
#include <sstream>

#include "VMProtect/VMProtectSDK.h"
#include "Util/MD5Hasher.h"

FASTFLAGVARIABLE(FixGlowingCSG, true)

using namespace G3D;

#ifndef CSG_KERNEL_OLD

namespace RBX {

namespace {
CSGMeshFactory *csgMeshFactory = 0;
}

void CSGMeshFactory::set(CSGMeshFactory* factory)
{
    csgMeshFactory = factory;
}

CSGMeshFactory* CSGMeshFactory::singleton()
{
    if (csgMeshFactory)
        return csgMeshFactory;

    static CSGMeshFactory* meshFactory = new CSGMeshFactory;
    return meshFactory;
}

CSGMesh* CSGMeshFactory::createMesh()
{
    return new CSGMesh;
}

CSGMesh::CSGMesh()
    : version(2)
    , brepVersion(1)
    , badMesh(false)
{
}

CSGMesh::~CSGMesh()
{
}

void CSGMesh::clearMesh()
{
    vertices.clear();
    indices.clear();
}

//////////////////////////////////////////////////////////////////////
// The below code is to provide a first line of defense against expert users
// injecting random data or their own data into the geometry stream.
// It is not expected to completely prevent attempts at injection but
// it will slow down the process so that we can move the generation of
// the mesh to a service on a server.
///////////////////////////////////////////////////////////////////////

namespace {

void generateRandomString(char *s, const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        s[i] = (char)rand();
    }
}

}

const size_t saltSize = 16;
const size_t hashSize = 16;

std::string CSGMesh::createHash(const std::string saltIn) const
{
	VMProtectBeginMutation("17");

    const size_t verticesSize = vertices.size() * sizeof(CSGVertex);
    const size_t indicesSize = indices.size() * sizeof(unsigned int);
    size_t buffSize = verticesSize + indicesSize + saltSize;
    
    std::vector<unsigned char> byteBuffer(buffSize);
    std::vector<char> hash(saltSize + hashSize);
    
    std::string salt = saltIn;

    if (salt.empty())
    {
        salt.resize(saltSize);
        generateRandomString(&salt[0], salt.size());
    }

    size_t copyOffset = 0;
    memcpy(&byteBuffer[copyOffset], &vertices[0], verticesSize);

    copyOffset += verticesSize;
    memcpy(&byteBuffer[copyOffset], &indices[0], indicesSize);

    copyOffset += indicesSize;
    memcpy(&byteBuffer[copyOffset], salt.c_str(), salt.size());

    LcmRand randGen;
    for (size_t i = 0; i < buffSize; i++)
    {
        std::swap(byteBuffer[i], byteBuffer[randGen.value() % buffSize]);
    }
    
  	boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
    hasher->addData((const char*)&byteBuffer[0], byteBuffer.size());

    memcpy(&hash[0], hasher->toString().c_str(), hashSize);
    memcpy(&hash[hashSize], salt.c_str(), saltSize);

    std::string hashStr(&hash[0], hashSize + saltSize);

    VMProtectEnd();

    return hashStr;
}

void CSGMesh::computeDecalRemap()
{
    if (!FFlag::FixGlowingCSG) return;

    for (unsigned i = 0; i < 6; ++i)
    {
        decalVertexRemap[i].clear();
        decalIndexRemap[i].clear();
    }

    std::vector<unsigned> tmpTranslation;
    tmpTranslation.resize(vertices.size());

    for (unsigned vi = 0; vi < vertices.size(); ++vi)
    {
        unsigned face = vertices[vi].extra.r - 1;
        RBXASSERT(face < 6);
        decalVertexRemap[face].push_back(vi);
        tmpTranslation[vi] = decalVertexRemap[face].size() - 1;
    }

    for (unsigned ii = 0; ii < indices.size(); ++ii)
    {
        unsigned face = vertices[indices[ii]].extra.r - 1;
        decalIndexRemap[face].push_back(tmpTranslation[indices[ii]]);
    }
}

void xorBuffer(std::string& buffer)
{
    const size_t basicEncryptionKeySize = 31;
    
    LcmRand randGen;
    std::string basicEncryptionKey;
    basicEncryptionKey.resize(basicEncryptionKeySize);

    for (size_t i =0; i < basicEncryptionKey.size(); i++)
        basicEncryptionKey[i] = randGen.value() % CHAR_MAX;

    for (size_t i = 0; i < buffer.size(); i++)
        buffer[i] = buffer[i] ^ basicEncryptionKey[i % basicEncryptionKey.size()];
}

std::string headerTag("CSGMDL");

std::string CSGMesh::toBinaryString() const
{
	std::stringstream stream;
    
    stream.write(headerTag.c_str(), headerTag.size());

    stream.write(reinterpret_cast<const char*>(&version), sizeof(version));

    std::string hash = createHash();

    stream.write(reinterpret_cast<const char*>(hash.c_str()), hashSize + saltSize);

    unsigned int numVertices = vertices.size();
    unsigned int vertexStride = sizeof(CSGVertex);
    stream.write(reinterpret_cast<const char*>(&numVertices), sizeof(unsigned int));
    stream.write(reinterpret_cast<const char*>(&vertexStride), sizeof(unsigned int));
    stream.write(reinterpret_cast<const char*>(&vertices[0]), vertexStride * numVertices);

    unsigned int numIndices = indices.size();
    stream.write(reinterpret_cast<const char*>(&numIndices), sizeof(unsigned int));
    stream.write(reinterpret_cast<const char*>(&indices[0]), sizeof(unsigned int) * numIndices);
    
    std::string buffer(stream.str().c_str(), stream.str().size());

    xorBuffer(buffer);

    return buffer;
}

std::string CSGMesh::toBinaryStringForPhysics() const
{
	std::vector<btVector3> vertexPositions;
	for (unsigned int i = 0; i < vertices.size(); i++)
		vertexPositions.push_back(btVector3(vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z));

	return TriangleMesh::generateStaticMeshData(indices, vertexPositions);
}

bool CSGMesh::fromBinaryString(const std::string& str)
{
    std::string buffer(str.c_str(), str.size());

    xorBuffer(buffer);

    std::stringstream stream(buffer);
    int currentVersion = version;

    std::string fileId;
    fileId.resize(headerTag.size());
    stream.read(&fileId[0], headerTag.size());
    
    if (fileId != headerTag)
        return false;

    stream.read(reinterpret_cast<char*>(&version), sizeof(int));

    if (version != currentVersion)
        return false;

	std::string hash;
    hash.resize(hashSize + saltSize);
    stream.read(reinterpret_cast<char*>(&hash[0]), hashSize + saltSize);
    
    unsigned int numVertices = 0;
    unsigned int vertexStride = 0;
    stream.read(reinterpret_cast<char*>(&numVertices), sizeof(unsigned int));
    stream.read(reinterpret_cast<char*>(&vertexStride), sizeof(unsigned int));

    if (vertexStride != sizeof(CSGVertex))
        return false;

    vertices.resize(numVertices);
    stream.read(reinterpret_cast<char*>(&vertices[0]), vertexStride * numVertices);

    unsigned int numIndices = 0;
    stream.read(reinterpret_cast<char*>(&numIndices), sizeof(unsigned int));
    indices.resize(numIndices);
    stream.read(reinterpret_cast<char*>(&indices[0]), sizeof(unsigned int) * numIndices);

    std::string salt(&hash[hashSize], saltSize);

    std::string newHash = createHash(salt);

    if (hash != newHash)
        badMesh = true;

    computeDecalRemap();
    
	return true;
}

void CSGMesh::set(const std::vector<CSGVertex>& verticesIn, const std::vector<unsigned int>& indicesIn)
{
    vertices = verticesIn;
    indices = indicesIn;
}

CSGMesh* CSGMesh::clone() const
{
    CSGMesh* mesh = new CSGMesh(*this);
    return mesh;
}

void CSGVertex::generateUv()
{
    switch (extra.r)
	{
    case CSGVertex::UV_BOX_X:
        uv = Vector2(-pos.z, -pos.y);
        break;
	case CSGVertex::UV_BOX_X_NEG:
        uv = Vector2(pos.z, -pos.y);
        break;
	case CSGVertex::UV_BOX_Y:
        uv = Vector2(-pos.x, -pos.z);
        break;
	case CSGVertex::UV_BOX_Y_NEG:
        uv = Vector2(pos.x, -pos.z);
        break;
	case CSGVertex::UV_BOX_Z:
        uv = Vector2(pos.x, -pos.y);
        break;
	case CSGVertex::UV_BOX_Z_NEG:
        uv = Vector2(-pos.x, -pos.y);
        break;                       
    }
}

Vector2 CSGVertex::generateUv(const Vector3& posIn) const
{
    Vector2 uvResult;

    switch (extra.r)
	{
    case CSGVertex::NO_UV_GENERATION:
        uvResult = uv;
        break;
    case CSGVertex::UV_BOX_X:
        uvResult = Vector2(-posIn.z, -posIn.y);
        break;
	case CSGVertex::UV_BOX_X_NEG:
        uvResult = Vector2(posIn.z, -posIn.y);
        break;
	case CSGVertex::UV_BOX_Y:
        uvResult = Vector2(-posIn.x, -posIn.z);
        break;
	case CSGVertex::UV_BOX_Y_NEG:
        uvResult = Vector2(posIn.x, -posIn.z);
        break;
	case CSGVertex::UV_BOX_Z:
        uvResult = Vector2(posIn.x, -posIn.y);
        break;
	case CSGVertex::UV_BOX_Z_NEG:
        uvResult = Vector2(-posIn.x, -posIn.y);
        break;                       
    }
    return uvResult;
}

bool CSGMesh::isNotEmpty() const
{
	if ((getVertices().size() > 0) && (getIndices().size() > 0))
		return true;

	return false;
}

} // namespace RBX

#endif
