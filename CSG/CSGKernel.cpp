/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */

#include "CSGKernel.h"
#include "V8DataModel/CSGMesh.h"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/once.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
#include <streambuf>
#include <sstream>

#include "g3d/g3dmath.h"
#include "g3d/Ray.h"
#include "g3d/CollisionDetection.h"
#include "g3d/vectorMath.h"

#include <math.h>
#include <map>

#include <sgCore.h>
#include "util/FileSystem.h"
#include "FastLog.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "../Win/LogManager.h"
#endif

FASTFLAGVARIABLE(CSGExportFailure, false);

static std::string lastFileError = "";

using namespace G3D;

namespace RBX {

CSGMesh* CSGMeshFactorySgCore::createMesh()
{
    return new CSGMeshSgCore;
}

void CSGMeshSgCore::weldMesh(bool positionOnly)
{
	std::vector<CSGVertex> rawTris;
	rawTris.resize(indices.size());
	
	for (unsigned int i = 0; i < indices.size(); i++)
	{
		rawTris[i] = vertices[indices[i]];
	}

    std::vector<unsigned int> emptyIndices;
	indices.swap(emptyIndices);
	indices.reserve(rawTris.size());
    std::vector<CSGVertex> emptyVec;
	vertices.swap(emptyVec);

	for (unsigned int i = 0; i < rawTris.size(); i++)
	{
		bool found = false;
		unsigned int foundIndex = 0;
		for (unsigned int u = 0; u < vertices.size(); u++)
		{
			if ((rawTris[i].pos - vertices[u].pos).length() < 0.001f)
            {
                if (positionOnly)
                {
					found = true;
					foundIndex = u;
                }
                else if ((rawTris[i].normal - vertices[u].normal).length() < 0.001f &&
					rawTris[i].color == vertices[u].color && 
					rawTris[i].uv == vertices[u].uv)
				{
					found = true;
					foundIndex = u;
				}
            }
		}
		if (found)
		{
			indices.push_back(foundIndex);
			continue;
		}
		unsigned indexOffset = unsigned(vertices.size());
		vertices.push_back(rawTris[i]);
		indices.push_back(indexOffset);
	}
}

CSGMeshSgCore::EditData::EditData(CSGMeshSgCore* meshIn)
: shape(0)
, mesh(meshIn)
{}

CSGMeshSgCore::EditData::~EditData()
{
    destroy();
}

CSGMeshSgCore::EditData::EditData(const CSGMeshSgCore::EditData& editData)
: shape(0)
, mesh(editData.mesh)
{
    shape = editData.clone();
}

CSGMeshSgCore::EditData& CSGMeshSgCore::EditData::operator=(const CSGMeshSgCore::EditData& editData)
{
    setShape(editData.clone());
    return *this;
}

sgCObject* CSGMeshSgCore::EditData::clone() const
{
    // The sgCObject* clone function does not do a true clone.
    // Use the object to bit array code path instead to get a true hierarchy clone.
    return mesh->brepFromBinaryString(mesh->getBRepBinaryString());
}

void CSGMeshSgCore::EditData::destroy()
{
    if (shape)
        sgDeleteObject(shape);
    shape = 0;
}

void CSGMeshSgCore::EditData::setShape(sgCObject* shapeIn)
{
    destroy();
    shape = shapeIn;
}

void initKernelOnce()
{
    sgInitKernel();
    sgC3DObject::AutoTriangulate(false, ::SG_VERTEX_TRIANGULATION);
}

void initKernel()
{
  	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initKernelOnce, flag);
}

CSGMeshSgCore::CSGMeshSgCore()
: editData(this)
{
    initKernel();
}

CSGMeshSgCore::CSGMeshSgCore(const CSGMeshSgCore& mesh)
: editData(this)
{
    initKernel();

    vertices = mesh.vertices;
    indices = mesh.indices;
    version = mesh.version;
    badMesh = mesh.badMesh;
    editData = mesh.editData;

    for (unsigned i = 0; i < 6; ++i)
    {
        decalIndexRemap[i] = mesh.decalIndexRemap[i];
        decalVertexRemap[i] = mesh.decalVertexRemap[i];
    }
}

CSGMeshSgCore::~CSGMeshSgCore()
{
}

CSGMeshSgCore& CSGMeshSgCore::operator=(const CSGMeshSgCore& mesh)
{
    vertices = mesh.vertices;
    indices = mesh.indices;
    version = mesh.version;
    badMesh = mesh.badMesh;
    editData = mesh.editData;
    return *this;
}

void removeAllDXFFiles()
{
	boost::filesystem::path path = RBX::FileSystem::getUserDirectory(true, RBX::DirAppData, "logs");
    boost::system::error_code ec;

	if (path.empty())
		return;

	for (boost::filesystem::directory_iterator iter(path, ec), endIter; iter != endIter; ++iter)
	{
		if (0 == iter->path().extension().compare(boost::filesystem::path(".dxf"))) // ugh
			boost::filesystem::remove(iter->path(), ec);
	}
}

void removePreviousErrorFiles()
{
	if (!lastFileError.empty())
	{
		std::remove((lastFileError + "A.dxf").c_str());
		std::remove((lastFileError + "B.dxf").c_str());
	}
	else
	{
		removeAllDXFFiles();
	}

	lastFileError = "";
}

void logError(sgCObject* obj1, sgCObject* obj2, bool unionOperation = true)
{
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
	removePreviousErrorFiles();

	std::string path = MainLogManager::getMainLogManager()->MakeLogFileName(unionOperation ? "_csgU" : "_csgN");
	path = path.substr(0, path.size() - 4);
	lastFileError = path;
	
	sgGetScene()->AttachObject(obj1);
	sgFileManager::ExportDXF(sgGetScene(), (path + "A.dxf").c_str());
	sgGetScene()->DetachObject(obj1);

	sgGetScene()->AttachObject(obj2);
	sgFileManager::ExportDXF(sgGetScene(), (path + "B.dxf").c_str());
	sgGetScene()->DetachObject(obj2);
#endif
}

void gather3DObjects(sgCObject* obj, std::vector<sgC3DObject*>& objects);

void gatherGroup(sgCGroup* group, std::vector<sgC3DObject*>& objects)
{
    sgCObject*  curObj = group->GetChildrenList()->GetHead();
    while (curObj)
    {
        gather3DObjects(curObj, objects);
        curObj = group->GetChildrenList()->GetNext(curObj);
    }
}

void gather3DObjects(sgCObject* obj, std::vector<sgC3DObject*>& objects)
{
    switch(obj->GetType())
    {
        case SG_OT_GROUP:
        {
            gatherGroup(reinterpret_cast<sgCGroup*>(obj), objects);
            break;
        }
        case SG_OT_3D:
        {
            objects.push_back(reinterpret_cast<sgC3DObject*>(obj->Clone()));
            break;
        }
        default:
            break;
    }
}

void applyMatrixTo3DObjects(sgCObject* obj, const sgCMatrix& matrix);

void applyMatrixToGroup(sgCGroup* group, const sgCMatrix& matrix)
{
    sgCObject*  curObj = group->GetChildrenList()->GetHead();
    while (curObj)
    {
        applyMatrixTo3DObjects(curObj, matrix);
        curObj = group->GetChildrenList()->GetNext(curObj);
    }
}

void applyMatrixTo3DObjects(sgCObject* obj, const sgCMatrix& matrix)
{
    switch(obj->GetType())
    {
        case SG_OT_GROUP:
        {
            applyMatrixToGroup(reinterpret_cast<sgCGroup*>(obj), matrix);
            break;
        }
        case SG_OT_3D:
        {
            sgC3DObject* obj3D = reinterpret_cast<sgC3DObject*>(obj);
    
            obj3D->Transform(matrix);
            break;
        }
        default:
            break;
    }
}

void applyColorTo3DObjects(sgCObject* obj, const Vector3& value);

void applyColorToGroup(sgCGroup* group, const Vector3& value)
{
    sgCObject*  curObj = group->GetChildrenList()->GetHead();
    while (curObj)
    {
        applyColorTo3DObjects(curObj, value);
        curObj = group->GetChildrenList()->GetNext(curObj);
    }
}

void applyColorTo3DObjects(sgCObject* obj, const Vector3& value)
{
    switch(obj->GetType())
    {
        case SG_OT_GROUP:
        {
            applyColorToGroup(reinterpret_cast<sgCGroup*>(obj), value);
            break;
        }
        case SG_OT_3D:
        {
            sgC3DObject* obj3D = reinterpret_cast<sgC3DObject*>(obj);
            SG_POINT color;
            color.x = value.x;
            color.y = value.y;
            color.z = value.z;
            obj3D->SetColor(color);
            break;
        }
        default:
            break;
    }
}

void applyScaleTo3DObjects(sgCObject* obj, const Vector3& scale);

void applyScaleToGroup(sgCGroup* group, const Vector3& scale)
{
    sgCObject*  curObj = group->GetChildrenList()->GetHead();
    while (curObj)
    {
        applyScaleTo3DObjects(curObj, scale);
        curObj = group->GetChildrenList()->GetNext(curObj);
    }
}

void applyScaleTo3DObjects(sgCObject* obj, const Vector3& scale)
{
    switch(obj->GetType())
    {
        case SG_OT_GROUP:
        {
            applyScaleToGroup(reinterpret_cast<sgCGroup*>(obj), scale);
            break;
        }
        case SG_OT_3D:
        {
            sgC3DObject* obj3D = reinterpret_cast<sgC3DObject*>(obj);
            SG_POINT scaleDp;
            scaleDp.x = scale.x;
            scaleDp.y = scale.y;
            scaleDp.z = scale.z;
            obj3D->Scale(scaleDp);
            break;
        }
        default:
            break;
    }
}

void applyTranslationTo3DObjects(sgCObject* obj, const Vector3& translation);

void applyTranslationToGroup(sgCGroup* group, const Vector3& translation)
{
    sgCObject*  curObj = group->GetChildrenList()->GetHead();
    while (curObj)
    {
        applyTranslationTo3DObjects(curObj, translation);
        curObj = group->GetChildrenList()->GetNext(curObj);
    }
}

void applyTranslationTo3DObjects(sgCObject* obj, const Vector3& translation)
{
    switch(obj->GetType())
    {
        case SG_OT_GROUP:
        {
            applyTranslationToGroup(reinterpret_cast<sgCGroup*>(obj), translation);
            break;
        }
        case SG_OT_3D:
        {
            sgC3DObject* obj3D = reinterpret_cast<sgC3DObject*>(obj);
            SG_POINT translationDp;
            translationDp.x = translation.x;
            translationDp.y = translation.y;
            translationDp.z = translation.z;
            obj3D->Translate(translationDp);
            break;
        }
        default:
            break;
    }
}


void calcFlatNormal(CSGVertex& vertA, CSGVertex& vertB, CSGVertex& vertC)
{
    Vector3 normal = (vertB.pos - vertA.pos).cross(vertC.pos - vertA.pos);

    // Keep the magnitude of the cross product to use as a weighting for the
    // average.
    vertA.normal = vertB.normal = vertC.normal = normal;
}

void calcFlatTangent(CSGVertex& vertA, CSGVertex& vertB, CSGVertex& vertC)
{
    const Vector3& v1 = vertA.pos;
    const Vector3& v2 = vertB.pos;
    const Vector3& v3 = vertC.pos;
    const Vector2& w1 = vertA.uv;
    const Vector2& w2 = vertB.uv;
    const Vector2& w3 = vertC.uv;
 
    float x1 = v2.x - v1.x;
    float x2 = v3.x - v1.x;
    float y1 = v2.y - v1.y;
    float y2 = v3.y - v1.y;
    float z1 = v2.z - v1.z;
    float z2 = v3.z - v1.z;
    float s1 = w2.x - w1.x;
    float s2 = w3.x - w1.x;
    float t1 = w2.y - w1.y;
    float t2 = w3.y - w1.y;
 
    float r = (s1 * t2 - s2 * t1);
    if (r != 0)
        r = 1.0f / r;
    else
        r = 1.0f;

    Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);

    vertA.tangent = vertB.tangent = vertC.tangent = sdir;
}

void averageNormal(Vector3& resultNormal,
                   const Vector3& posA,
                   const Vector3& posB,
                   const Vector3& normalA,
                   const Vector3& normalB)
{
    static const float cosAngle = cos(G3D::toRadians(40));

    Vector3 distP = posB - posA;
    const float eps = 0.1f;

    if (fabs(distP.x) < eps &&
        fabs(distP.y) < eps &&
        fabs(distP.z) < eps)
    {
        float dotProd = normalA.unit().dot(normalB.unit());
        if (dotProd > cosAngle)
        {
            resultNormal += normalB;
        }
    }
}

void calcSmoothNormal(CSGVertex& vert, Vector3& normal, const std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t i = 0; i < vertices.size(); i++)
    {
        const CSGVertex& testVert = vertices[i];
        averageNormal(normal, vert.pos, testVert.pos, vert.normal, testVert.normal);
    }
}

void calcSmoothTangent(CSGVertex& vert, Vector3& tangent, const std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t i = 0; i < vertices.size(); i++)
    {
        const CSGVertex& testVert = vertices[i];
        averageNormal(tangent, vert.pos, testVert.pos, vert.tangent, testVert.tangent);
    }
}

void calcUV(CSGVertex& vert)
{
    Vector3& Pt = vert.pos;
	Vector3 unitNormal = vert.normal.unit();
    
    if (fabs(unitNormal.x) > fabs(unitNormal.y) &&
        fabs(unitNormal.x) > fabs(unitNormal.z))
    {
        vert.extra.r = unitNormal.x > 0 ? CSGVertex::UV_BOX_X : CSGVertex::UV_BOX_X_NEG;
    }
    else if (fabs(unitNormal.y) > fabs(unitNormal.z))
    {
        vert.extra.r = unitNormal.y > 0 ? CSGVertex::UV_BOX_Y : CSGVertex::UV_BOX_Y_NEG;
    }
    else
    {
       vert.extra.r = unitNormal.z > 0 ? CSGVertex::UV_BOX_Z : CSGVertex::UV_BOX_Z_NEG;
    }

    vert.uv = vert.generateUv(vert.pos);
}

void calcFlat( Vector3& normal, unsigned int& uvr,
              CSGVertex& vertA, CSGVertex& vertB, CSGVertex& vertC)
{
    normal = (vertB.pos - vertA.pos).cross(vertC.pos - vertA.pos);

    if (fabs(normal.x) > fabs(normal.y) &&
        fabs(normal.x) > fabs(normal.z))
    {
        uvr = normal.x > 0 ? CSGVertex::UV_BOX_X : CSGVertex::UV_BOX_X_NEG;
    }
    else if (fabs(normal.y) > fabs(normal.z))
    {
        uvr = normal.y > 0 ? CSGVertex::UV_BOX_Y : CSGVertex::UV_BOX_Y_NEG;
    }
    else
    {
        uvr = normal.z > 0 ? CSGVertex::UV_BOX_Z : CSGVertex::UV_BOX_Z_NEG;
    }
}

void calcFlatTangent(Vector3& tangent, CSGVertex& vertA, CSGVertex& vertB, CSGVertex& vertC)
{
    const Vector3& v1 = vertA.pos;
    const Vector3& v2 = vertB.pos;
    const Vector3& v3 = vertC.pos;
    const Vector2& w1 = vertA.uv;
    const Vector2& w2 = vertB.uv;
    const Vector2& w3 = vertC.uv;

    float x1 = v2.x - v1.x;
    float x2 = v3.x - v1.x;
    float y1 = v2.y - v1.y;
    float y2 = v3.y - v1.y;
    float z1 = v2.z - v1.z;
    float z2 = v3.z - v1.z;
    float s1 = w2.x - w1.x;
    float s2 = w3.x - w1.x;
    float t1 = w2.y - w1.y;
    float t2 = w3.y - w1.y;

    float r = (s1 * t2 - s2 * t1);
    if (r != 0)
        r = 1.0f / r;
    else
        r = 1.0f;

    tangent = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
}

bool triangulateObject(sgCObject* obj, unsigned int& counter, std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices);

bool triangulateGroup(sgCGroup* group, unsigned int& counter, std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices)
{
    sgCObject*  curObj = group->GetChildrenList()->GetHead();
    while (curObj)
    {
        triangulateObject(curObj, counter, vertices, indices);
        curObj = group->GetChildrenList()->GetNext(curObj);
    }
    return true;
}

bool triangulate3D(sgC3DObject* object, unsigned int& index, std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices)
{
    object->Triangulate(SG_VERTEX_TRIANGULATION);
    {
        SG_MATERIAL material;
        material.MaterialIndex = 0;
        material.TextureUVType = SG_CUBE_UV_TYPE;
        material.TextureScaleU = 1;
        material.TextureScaleV = 1;
        material.TextureShiftU = 0;
        material.TextureShiftV = 0;
        material.TextureSmooth = false;
        material.TextureMult = true;
        material.MixColorType = SG_BLEND_MIX_TYPE;
        object->SetMaterial(material);
    }

    const SG_ALL_TRIANGLES* triangles = reinterpret_cast<sgC3DObject*>(object)->GetTriangles();

    if (triangles)
    {
        for(int i = 0, j=0; i < 3*triangles->nTr; i += 3, j+=6)
        {
            CSGVertex vertA;
            vertA.color = Color4uint8(triangles->allColors[i].x*255, triangles->allColors[i].y*255, triangles->allColors[i].z*255, 255);
            vertA.pos = Vector3(triangles->allVertex[i].x, triangles->allVertex[i].y, triangles->allVertex[i].z);

            CSGVertex vertB;
            vertB.color = Color4uint8(triangles->allColors[i+1].x*255, triangles->allColors[i+1].y*255, triangles->allColors[i+1].z*255, 255);
            vertB.pos = Vector3(triangles->allVertex[i+1].x, triangles->allVertex[i+1].y, triangles->allVertex[i+1].z);

            CSGVertex vertC;
            vertC.color = Color4uint8(triangles->allColors[i+2].x*255, triangles->allColors[i+2].y*255, triangles->allColors[i+2].z*255, 255);
            vertC.pos = Vector3(triangles->allVertex[i+2].x, triangles->allVertex[i+2].y, triangles->allVertex[i+2].z);
           
            vertices.push_back(vertA);
            indices.push_back(index++);
            vertices.push_back(vertB);
            indices.push_back(index++);
            vertices.push_back(vertC);
            indices.push_back(index++);
        }
    }


    return true;
}

bool triangulateObject(sgCObject* obj, unsigned int& counter, std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices)
{
    switch(obj->GetType())
    {
        case SG_OT_GROUP:
            return triangulateGroup(reinterpret_cast<sgCGroup*>(obj), counter, vertices, indices);
        case SG_OT_3D:
            return triangulate3D(reinterpret_cast<sgC3DObject*>(obj), counter, vertices, indices);
        default:
            break;
    }
    return false;
}

bool CSGMeshSgCore::newTriangulate()
{
    unsigned int counter = 0;

    vertices.clear();
    indices.clear();
    if (!editData.getShape())
        return true;
    triangulateObject(editData.getShape(), counter, vertices, indices);
    size_t nIndices = indices.size();
    int nFaces = nIndices / 3;
    std::vector<Color4uint8> flatColors;
    flatColors.resize(nFaces);
    int n = 0;
    for (size_t i = 0; i < nIndices; i+=3, n++)
    {
        flatColors[n] = vertices[indices[i  ]].color;
    }
    size_t maxFacesPerVertex = clusterVertices( 0.001f );
    for ( auto i = indices.begin(); i != indices.end(); )
    {
        int i0, i1, i2;
        i0 = *i;
        i1 = *(i+1);
        i2 = *(i+2);
        if ( i0 == i1 ||
            i1 == i2 ||
            i2 == i0 )
        {
            std::vector<Color4uint8>::iterator c = flatColors.begin();
            auto x = i - indices.begin();
            c += x/3;
            flatColors.erase( c );
            i = indices.erase(i, i+3);
        }
        else
            i += 3;
    }
    std::vector<int> vertexEdges;
    vertexEdges.resize( vertices.size(), -1 );
    if ( !makeHalfEdges( vertexEdges ) )
    {
        return false;
    }

    nIndices = indices.size();
    nFaces = nIndices / 3;

    std::vector<Vector3> flatNormals;
    std::vector<unsigned int> flatUVrs;

    flatNormals.resize(nFaces);
    flatUVrs.resize(nFaces);

    n = 0;
    for (size_t i = 0; i < nIndices; i+=3, n++)
    {
        CSGVertex& vertA = vertices[indices[i  ]];
        CSGVertex& vertB = vertices[indices[i+1]];
        CSGVertex& vertC = vertices[indices[i+2]];
        calcFlat(flatNormals[n], flatUVrs[n],
            vertA, vertB, vertC);
    }

    // find creased edges
    static const float cosAngle = cos(G3D::toRadians(40));
    for ( size_t e = 0; e < halfEdges.size(); e++ )
    {
        CSGHalfEdge& hE = halfEdges[e];
        if ( !hE.creaseSet )
        {
            if ( hE.oppEdge >= 0 )
            {
                CSGHalfEdge& hEo = halfEdges[hE.oppEdge];
                int f1 = hE.face;
                int f2 = hEo.face;
                float dotProd = flatNormals[f1].unit().dot(flatNormals[f2].unit());
                if (dotProd < cosAngle)
                    hE.creaseFlag |= CSGHalfEdge::normalCrease;
                if ( flatUVrs[f1] != flatUVrs[f2] )
                    hE.creaseFlag |= CSGHalfEdge::uvCrease;
                if ( flatColors[f1] != flatColors[f2] )
                    hE.creaseFlag |= CSGHalfEdge::colorCrease;
                hE.creaseSet = true;
                hEo.creaseFlag = hE.creaseFlag;
                hEo.creaseSet = true;
            }
        }
    }

    // duplicate vertices for creases, put them in circular lists
    size_t nVerts = vertices.size();
    std::vector<triangulationVertex> triVerts;
    triVerts.resize(nVerts);

    int *vertFaces = new int[maxFacesPerVertex];
    unsigned int *creaseFlags = new unsigned int[maxFacesPerVertex];
    int *creaseFaces = new int[maxFacesPerVertex];
    for ( size_t iVert = 0; iVert < nVerts; iVert++ )
    {
        int vertexFaceCount = 0;
        int creaseFaceCount = 0;
        CSGVertex& currentVertexRef = vertices[iVert];
        triangulationVertex& currentTriVertexRef = triVerts[iVert];
        currentTriVertexRef.neighborVert[0] = currentTriVertexRef.neighborVert[1] = iVert;
        int firstVertexHalfEdge = vertexEdges[iVert];
        currentVertexRef.extra.r = flatUVrs[halfEdges[firstVertexHalfEdge].face];
        currentVertexRef.generateUv();
        CSGVertex currentVertex = currentVertexRef;
        int iterateVertexHalfEdge = firstVertexHalfEdge;
        do
        {
            CSGHalfEdge& hE = halfEdges[iterateVertexHalfEdge];
            if ( hE.oppEdge < 0 )
            {
                delete[] vertFaces;
                delete[] creaseFlags;
                delete[] creaseFaces;
                return false;
            }
            CSGHalfEdge& hEo = halfEdges[hE.oppEdge];
            int f = hEo.face;
            if ( hE.creaseFlag != hEo.creaseFlag )
            {
                delete[] vertFaces;
                delete[] creaseFlags;
                delete[] creaseFaces;
                return false;
            }
            if ( hE.creaseFlag != 0 )
            {
                creaseFlags[creaseFaceCount] = hE.creaseFlag;
                creaseFaces[creaseFaceCount++] = vertexFaceCount;
            }
            vertFaces[vertexFaceCount++] = f;
            iterateVertexHalfEdge = hEo.nextEdge;
        }
        while ( iterateVertexHalfEdge != firstVertexHalfEdge );
        if ( creaseFaceCount > 1 )
        {
            int newIVert = vertices.size();
            currentTriVertexRef.duplicateCount = creaseFaceCount;
            currentTriVertexRef.neighborCreaseFlag[0] = creaseFlags[creaseFaceCount-1];
            currentTriVertexRef.neighborCreaseFlag[1] = creaseFlags[0];
            currentTriVertexRef.neighborVert[0] = newIVert + creaseFaceCount-2;
            currentTriVertexRef.neighborVert[1] = newIVert;
            int firstVert = iVert;
            int previ = iVert;
            int newNverts = newIVert + creaseFaceCount-1;
            vertices.resize( newNverts, currentVertex );
            triVerts.resize( newNverts );
            for ( int iCreaseFace = 0; iCreaseFace < creaseFaceCount-1; iCreaseFace++, newIVert++ )
            {
                CSGVertex& newvx = vertices[newIVert];
                triangulationVertex& newVt = triVerts[newIVert];
                newVt.duplicateCount = creaseFaceCount;
                newVt.neighborVert[0] = previ;
                newVt.neighborVert[1] = iCreaseFace == creaseFaceCount-2 ? firstVert : newIVert+1;
                newVt.neighborCreaseFlag[0] = creaseFlags[iCreaseFace];
                newVt.neighborCreaseFlag[1] = creaseFlags[iCreaseFace+1];
                int currentCreaseFace = creaseFaces[iCreaseFace];
                int nextCreaseFace = creaseFaces[iCreaseFace+1];
                newvx.extra.r = flatUVrs[vertFaces[currentCreaseFace]];
                newvx.generateUv();
                for ( int cv = currentCreaseFace; cv < nextCreaseFace; cv++ )
                {
                    int faceIndex0 = vertFaces[cv] * 3;
                    bool rplcd = false;
                    for ( int r = 0; !rplcd && r < 3; r++ )
                        if ( rplcd = (indices[faceIndex0+r] == iVert) )
                            indices[faceIndex0+r] = newIVert;
                }
                previ = newIVert;
            }
        }
    }
    delete[] vertFaces;
    delete[] creaseFlags;
    delete[] creaseFaces;

    nVerts = vertices.size();
    std::vector<Vector3> normals;
    std::vector<Vector3> tangents;
    normals.resize(nVerts);
    tangents.resize(nVerts);

    n = 0;
    for (size_t i = 0; i < nIndices; i+=3, n++)
    {
        Vector3 fNormal = flatNormals[n];
        normals[indices[i  ]] += fNormal;
        normals[indices[i+1]] += fNormal;
        normals[indices[i+2]] += fNormal;

        Vector3 flatTangent;
        calcFlatTangent( flatTangent, vertices[indices[i  ]],
            vertices[indices[i+1]],
            vertices[indices[i+2]] );
        tangents[indices[i  ]] += flatTangent;
        tangents[indices[i+1]] += flatTangent;
        tangents[indices[i+2]] += flatTangent;

        auto fColor = flatColors[n];
        vertices[indices[i  ]].color = fColor;
        vertices[indices[i+1]].color = fColor;
        vertices[indices[i+2]].color = fColor;
    }

    std::vector<Vector3> cNormals;
    std::vector<Vector3> cTangents;
    cNormals.resize(nVerts);
    cTangents.resize(nVerts);

    // average normals and tangents over non-specific creases
    std::vector<Vector3>* cNormalsTangents[2] = { &cNormals, &cTangents };
    Vector3 vectorNormalTangent[2];
    unsigned int creasFlagNT[2];
    creasFlagNT[0] = CSGHalfEdge::normalCrease;
    creasFlagNT[1] = CSGHalfEdge::uvCrease;
    for ( size_t iVert = 0; iVert < nVerts; iVert++ )
    {
        vectorNormalTangent[0] = normals[iVert];
        vectorNormalTangent[1] = tangents[iVert];
        cNormals[iVert] += normals[iVert];
        cTangents[iVert] += tangents[iVert];
        CSGVertex vert = vertices[iVert];
        int vertDuplicateCount = triVerts[iVert].duplicateCount;
        if ( vertDuplicateCount > 1 )
        {
            int nc;
            bool creased;

            for ( int normtang = 0; normtang < 2; normtang++ ) //do for normal and tangent
            {
                nc = 0;
                for ( int neighbor = 0; neighbor < 2; neighbor++ ) // walk both ways
                {
                    creased = false;
                    CSGVertex wVert = vert;
                    int nextVert = iVert;
                    while ( !creased && nc < vertDuplicateCount-1 )
                    {
                        if ( !( creased = ( triVerts[nextVert].neighborCreaseFlag[neighbor] & creasFlagNT[normtang] ) != 0 ) )
                        {
                            nextVert = triVerts[nextVert].neighborVert[neighbor];
                            (*cNormalsTangents[normtang])[nextVert] += vectorNormalTangent[normtang];
                            nc++;
                        }
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < nVerts; i++)
    {
        CSGVertex& vert = vertices[i];
        vert.tangent = cTangents[i].directionOrZero();
        vert.normal = cNormals[i].directionOrZero();
    }

    computeDecalRemap();

    return true;
}

void CSGMeshSgCore::triangulate()
{
    unsigned int counter = 0;

    vertices.clear();
    indices.clear();
    if (!editData.getShape())
        return;
    triangulateObject(editData.getShape(), counter, vertices, indices);

    std::vector<Vector3> normals;
    std::vector<Vector3> tangents;

    normals.resize(vertices.size());
    tangents.resize(vertices.size());

    for (size_t i = 0; i < indices.size(); i+=3)
    {
        CSGVertex& vertA = vertices[indices[i]];
        CSGVertex& vertB = vertices[indices[i+1]];
        CSGVertex& vertC = vertices[indices[i+2]];
        calcFlatNormal(vertA, vertB, vertC);
        calcUV(vertA);
        calcUV(vertB);
        calcUV(vertC);
        calcFlatTangent(vertA, vertB, vertC);
    }

    for (size_t i = 0; i < vertices.size(); i++)
    {
        CSGVertex& vert = vertices[i];
        calcSmoothNormal(vert, normals[i], vertices, indices);
        calcSmoothTangent(vert, tangents[i], vertices, indices);
    }

    for (size_t i = 0; i < vertices.size(); i++)
    {
        CSGVertex& vert = vertices[i];
        if (tangents[i].length() > 0)
            vert.tangent = tangents[i].unit();
        if (normals[i].length() > 0)
            vert.normal = normals[i].unit();
    }

    weldMesh();
}

bool CSGMeshSgCore::sgCoreUnion(const CSGMeshSgCore& a, const CSGMeshSgCore& b)
{
    if (!a.editData.getShape() || !b.editData.getShape())
        return false;

    std::vector<sgC3DObject*> objects;
    gather3DObjects(a.editData.getShape(), objects);
    gather3DObjects(b.editData.getShape(), objects);
    
    for (size_t i = 0; i < objects.size(); i++)
    {
        if (objects[i] == NULL)
            continue;

        for (size_t o = 0; o < objects.size(); o++)
        {
            if (i == o)
                continue;

            if (objects[o] == NULL)
                continue;

            if (objects[i] == NULL)
                break;

            int errcode = 0;

			if (FFlag::CSGExportFailure)
				logError(objects[i], objects[o]);
			
            sgCGroup* group = sgBoolean::Union(*objects[i], *objects[o], errcode);

            if (errcode == 2)
			{
                return false;
			}

            if (group)
            {
                int numChildren = group->GetChildrenList()->GetCount();
                std::vector<sgCObject*> allChildren(numChildren);
                group->BreakGroup(&allChildren[0]);
                sgDeleteObject(group);
                
                for (size_t r = 0; r < allChildren.size(); r++)
                {
                    objects.push_back((sgC3DObject*)allChildren[r]);
                }

                sgDeleteObject(objects[i]);
                sgDeleteObject(objects[o]);
                objects[i] = NULL;
                objects[o] = NULL;
            }
        }
    }

    size_t shrinkSize = 0;
    for (size_t i = 0; i < objects.size(); i++)
    {
        if (objects[i] == NULL)
            continue;

        objects[shrinkSize] = objects[i];
        shrinkSize++;
    }
    objects.resize(shrinkSize);

    if (objects.size() > 0)
    {
        editData.setShape(sgCGroup::CreateGroup((sgCObject**)&objects[0], int(objects.size())));
    }
    return true;
}

bool CSGMeshSgCore::sgCoreSubtract(const CSGMeshSgCore& a, const CSGMeshSgCore& b)
{
    if (!a.editData.getShape() || !b.editData.getShape())
        return false;

    std::vector<sgC3DObject*> objectsA;
    gather3DObjects(a.editData.getShape(), objectsA);
    std::vector<sgC3DObject*> objectsB;
    gather3DObjects(b.editData.getShape(), objectsB);
    
    for (size_t i = 0; i < objectsA.size(); i++)
    {
        if (objectsA[i] == NULL)
            continue;

        bool matchFound = false;

        for (size_t o = 0; o < objectsB.size(); o++)
        {
            if (objectsA[i] == NULL || objectsB[o] == NULL)
                continue;

            int errcode = 0;

			if (FFlag::CSGExportFailure)
				logError(objectsA[i], objectsB[o], false);

            sgCGroup* group = sgBoolean::Sub(*objectsA[i], *objectsB[o], errcode);

            if (errcode == 4)
            {
                sgDeleteObject(objectsA[i]);
                objectsA[i] = 0;
            }
            else if (errcode > 1)
			{
                return false;
            }
            else if (group)
            {
                int numChildren = group->GetChildrenList()->GetCount();
                std::vector<sgCObject*> allChildren(numChildren);
                group->BreakGroup(&allChildren[0]);

                sgDeleteObject(group);
                sgDeleteObject(objectsA[i]);
                objectsA[i] = 0;

                if (allChildren.size() > 0)
                    objectsA[i] = (sgC3DObject*)allChildren[0];

                for (size_t r = 1; r < allChildren.size(); r++)
                {
                    objectsA.push_back((sgC3DObject*)allChildren[r]);
                }

                matchFound = true;
            }
        }
    }

    for (size_t i = 0; i < objectsB.size(); i++)
    {
        sgDeleteObject(objectsB[i]);
    }

    size_t shrinkSize = 0;
    for (size_t i = 0; i < objectsA.size(); i++)
    {
        if (objectsA[i] == NULL)
            continue;

        objectsA[shrinkSize] = objectsA[i];
        shrinkSize++;
    }
    objectsA.resize(shrinkSize);

    if (objectsA.size() > 0)
    {
        editData.setShape(sgCGroup::CreateGroup((sgCObject**)&objectsA[0], int(objectsA.size())));
    }
    else
    {
        editData.destroy();
    }
    return true;
}

bool CSGMeshSgCore::unionMesh(const CSGMesh* a, const CSGMesh* b)
{
    const CSGMeshSgCore* sgMeshA = dynamic_cast<const CSGMeshSgCore*>(a);
    const CSGMeshSgCore* sgMeshB = dynamic_cast<const CSGMeshSgCore*>(b);

    if (!sgMeshA || !sgMeshB)
        return false;

    return sgCoreUnion(*sgMeshA, *sgMeshB);
}

bool CSGMeshSgCore::intersectMesh(const CSGMesh* a, const CSGMesh* b)
{
    // Not implemented yet.
    return false;
}

bool CSGMeshSgCore::subractMesh(const CSGMesh* a, const CSGMesh* b)
{
    const CSGMeshSgCore* sgMeshA = dynamic_cast<const CSGMeshSgCore*>(a);
    const CSGMeshSgCore* sgMeshB = dynamic_cast<const CSGMeshSgCore*>(b);

    if (!sgMeshA || !sgMeshB)
        return false;

    return sgCoreSubtract(*sgMeshA, *sgMeshB);
}

void CSGMeshSgCore::applyCoordinateFrame(CoordinateFrame cFrame)
{
    if (!editData.getShape())
        return;
    
    float dmatrix[16];

    dmatrix[0] = cFrame.rotation[0][0];
    dmatrix[1] = cFrame.rotation[0][1];
    dmatrix[2] = cFrame.rotation[0][2];
    dmatrix[3] = 0.0;

    dmatrix[4] = cFrame.rotation[1][0];
    dmatrix[5] = cFrame.rotation[1][1];
    dmatrix[6] = cFrame.rotation[1][2];
    dmatrix[7] = 0.0;

    dmatrix[8] = cFrame.rotation[2][0];
    dmatrix[9] = cFrame.rotation[2][1];
    dmatrix[10] = cFrame.rotation[2][2];
    dmatrix[11] = 0.0;

    dmatrix[12] = 0.0;
    dmatrix[13] = 0.0;
    dmatrix[14] = 0.0;
    dmatrix[15] = 1.0;

    sgCMatrix matrix(dmatrix);

    applyMatrixTo3DObjects(editData.getShape(), matrix);
    applyTranslationTo3DObjects(editData.getShape(), cFrame.translation);
}

void CSGMeshSgCore::applyTranslation(const G3D::Vector3& trans)
{
    if (!editData.getShape())
        return;
    applyTranslationTo3DObjects(editData.getShape(), trans);
}

void CSGMeshSgCore::applyScale(const G3D::Vector3& scale)
{
    if (!editData.getShape())
        return;

    applyScaleTo3DObjects(editData.getShape(), scale);
}

void CSGMeshSgCore::applyColor(const G3D::Vector3& color)
{
    if (!editData.getShape())
        return;

    applyColorTo3DObjects(editData.getShape(), color);
}

void CSGMeshSgCore::buildBRep()
{
    if (vertices.size() == 0 || indices.size() < 3)
        return;

    std::vector<SG_VERT> points;
    std::vector<SG_INDEX_TRIANGLE> triIndices;

    for (size_t i = 0; i < vertices.size(); i++)
    {
        SG_VERT point;
        point.x = vertices[i].pos.x;
        point.y = vertices[i].pos.y;
        point.z = vertices[i].pos.z;
        point.r = float(vertices[i].color.r)/255.0f;
        point.g = float(vertices[i].color.g)/255.0f;
        point.b = float(vertices[i].color.b)/255.0f;
        points.push_back(point);
    }

    for (size_t i = 0; i < indices.size(); i+=3)
    {
        SG_INDEX_TRIANGLE index;
        index.ver_indexes[0] = indices[i];
        index.ver_indexes[1] = indices[i+1];
        index.ver_indexes[2] = indices[i+2];
        triIndices.push_back(index);
    }

    editData.setShape(sgFileManager::ObjectFromTriangles(&points[0], int(points.size()), &triIndices[0], int(triIndices.size()), 45.0f * float(pi()) / 180.0f));

    if (!editData.getShape())
        return;
}

std::string CSGMeshSgCore::getBRepBinaryString() const
{
    if (!editData.getShape())
        return "";

    unsigned long arraySize = 0;
    const char* objectByteArray = (const char*)sgFileManager::ObjectToBitArray(editData.getShape(), arraySize);
    
    if (arraySize == 0)
        return "";
    
    std::stringstream stream;
    stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
    stream.write(reinterpret_cast<const char*>(&arraySize), sizeof(unsigned long));
    stream.write(objectByteArray, arraySize);

    return stream.str();
}

sgCObject* CSGMeshSgCore::brepFromBinaryString(const std::string& str) const
{
    if (str.empty())
        return NULL;

    int brepVersion;
    std::stringstream stream(str);
    stream.read(reinterpret_cast<char*>(&brepVersion), sizeof(brepVersion));

    unsigned long arraySize = 0;
    stream.read(reinterpret_cast<char*>(&arraySize), sizeof(unsigned long));

    std::string objectByteArray;
    objectByteArray.resize(arraySize);

    stream.read(&objectByteArray[0], arraySize);
    
    sgCObject* object = sgFileManager::BitArrayToObject((const void*)(objectByteArray.c_str()), (unsigned long)(objectByteArray.size()));

    return object;
}


void CSGMeshSgCore::setBRepFromBinaryString(const std::string& str)
{
    if (str.empty())
        return;

    sgCObject* object = brepFromBinaryString(str);

    if (object)
    {
        editData.setShape(object);
    }
}

bool CSGMeshSgCore::isValid() const
{
    return editData.getShape() != NULL;
}

CSGMesh* CSGMeshSgCore::clone() const
{
    CSGMesh* mesh = new CSGMeshSgCore(*this);
    return mesh;
}

CSGClustering::CSGClustering( std::vector<unsigned int>& indices, std::vector<CSGVertex>& vertices,
                             const Vector3& minimumExtentsPosition, float invres ):
m_indices(indices),
    m_vertices(vertices),
    m_minpos( minimumExtentsPosition ),
    m_invres( invres )
{
    clusters.reserve( m_indices.size() );
}

CSGClustering::IPosClassMap::iterator CSGClustering::addPos( uint64 key, int indx )
{
    IPosClassMap::iterator it = posclasses.find(key);
    if ( it == posclasses.end() )
    {
        VertexCluster vc;
        vc.posclasses.insert( key );
        vc.indices.insert( indx );
        int ci = clusters.size();
        clusters.push_back( vc );
        auto ip = posclasses.insert( IPosClassMap::value_type( key, ci ) );
        it = ip.first;
    }
    else
    {
        clusters[(it->second)].indices.insert( indx );
    }
    return it;
}

void CSGClustering::mergeClasses( IPosClassMap::iterator it[8] )
{
    std::set<int>ci;
    for ( int i = 0; i < 8; i++ )
        ci.insert( it[i]->second );
    if ( ci.size() > 1 )
    {
        auto cit = ci.begin();
        int ci0 = *cit;
        VertexCluster& vc0 = clusters[ci0];
        cit++;
        while ( cit != ci.end() )
        {
            VertexCluster& vc1 = clusters[*cit];
            for ( auto it = vc1.indices.begin(); it != vc1.indices.end(); it++ )
            {
                vc0.indices.insert( *it );
            }
            for ( auto it = vc1.posclasses.begin(); it != vc1.posclasses.end(); it++ )
            {
                vc0.posclasses.insert( *it );
            }
            vc1.indices.clear();
            vc1.posclasses.clear();
            cit++;
        }
        for ( int i = 0; i < 8; i++ )
            it[i]->second = ci0;
    }
}

void CSGClustering::addPosClasses( G3D::uint64 key[8], int indx )
{
    IPosClassMap::iterator it[8];
    for ( unsigned int i = 0; i < 8; i++ )
    {
        it[i] = addPos( key[i], indx );
    }
    mergeClasses( it );
}

uint64 CSGClustering::makeKey( const v3i2& v, unsigned int ii )
{
    unsigned int i0, i1, i2;
    uint64 li = ii & 7;
    i0 = ii & 1; ii >>= 1;
    i1 = ii & 1; ii >>= 1;
    i2 = ii & 1;
    uint64 x = static_cast<uint64>( v[i0].x );
    uint64 y = static_cast<uint64>( v[i1].y );
    uint64 z = static_cast<uint64>( v[i2].z );
    return ( ( li << 60 ) | ( (z & 0xfffffULL) << 40 ) | ( (y & 0xfffffULL) << 20 )  | (x & 0xfffffULL) );
}

void CSGClustering::cluster()
{
    for ( unsigned int i = 0; i < m_indices.size(); i++ )
    {
        const Vector3& vpos = m_vertices[m_indices[i]].pos;
        const Vector3int32 ivpos = Vector3int32::floor ( ( vpos - m_minpos ) * m_invres );
        const Vector3int32 iclass0 = ivpos >> 1;
        const Vector3int32 iclass1 = ivpos - iclass0;
        const v3i2 iclass = { iclass0, iclass1 };
        uint64 pkey[8];
        for ( unsigned int p = 0; p < 8; p++ )
        {
            pkey[p] = makeKey( iclass, p );
        }
        addPosClasses( pkey, i );
    }
}

size_t CSGClustering::extractVertices()
{
    std::vector<CSGVertex> newVertices;

    size_t maxFaceCount = 0;
    for ( unsigned int i = 0; i < clusters.size(); i++ )
    {
        if ( !clusters[i].indices.empty() )
        {
            auto it = clusters[i].indices.begin();
            unsigned int vi = newVertices.size();
            newVertices.push_back( m_vertices[m_indices[*it]] );
            maxFaceCount = max( maxFaceCount, clusters[i].indices.size() );
            for( ; it != clusters[i].indices.end(); it++ )
            {
                m_indices[*it] = vi;
            }
        }
    }
    m_vertices.swap( newVertices );
    return maxFaceCount;
}

size_t CSGMeshSgCore::clusterVertices( float resolution )
{
    makeExtents();

    const Vector3& minimumExtentsPosition = extents.min();
    float szsz = extents.size().max();
    float invres = 1.0f / resolution;
    //float maxres = 2097150.0f * szsz;  // 0x1ffffe
    float maxres = 1048574.0f * szsz;  // 0xffffe
    if ( invres > maxres )
        invres = maxres;
    CSGClustering clustering( indices, vertices, minimumExtentsPosition, invres );
    clustering.cluster();
    return clustering.extractVertices();
}

void CSGMeshSgCore::makeExtents()
{
    extents = Extents::negativeMaxExtents();

    for ( std::vector<CSGVertex>::const_iterator iter = vertices.begin(); iter != vertices.end(); ++iter)
        extents.expandToContain( (*iter).pos );
}

void CSGMeshSgCore::translate( const G3D::Vector3& translation )
{
    for ( size_t v = 0; v < vertices.size(); v++ )
    {
        vertices[v].pos += translation;
    }
    extents.shift(translation );
}

static const int hix3p[] = {2,0,1};
static const int hix3n[] = {1,2,0};
typedef boost::unordered_map <unsigned int,int> HalfEdgeMap;

CSGHalfEdge::CSGHalfEdge():
    creaseFlag(0),
    creaseSet(false),
    oppEdge(-1)
{
}

bool CSGMeshSgCore::makeHalfEdges( std::vector< int>& vertexEdges )
{
    HalfEdgeMap oppositeEdgesMap;

    unsigned int nIndices = indices.size();
    halfEdges.resize( nIndices );
    CSGHalfEdge hE;
    unsigned int nFaces = nIndices / 3;
    int iFace3 = 0;
    for ( unsigned int iFace = 0; iFace < nFaces; iFace++, iFace3 += 3 )
    {
        for ( int h = 0; h < 3; h++ )
        {
            int currentIndex = iFace3 + h;
            CSGHalfEdge& hE = halfEdges[currentIndex];
            hE.face = iFace;
            unsigned int currentVertex, nextVertex;
            int nextIndex = iFace3 + hix3n[h];
            currentVertex = hE.startVert = indices[currentIndex];
            if ( vertexEdges[currentVertex] == -1 )
            {
                vertexEdges[currentVertex] = currentIndex;
            }
            nextVertex = indices[nextIndex];
            hE.prevEdge = iFace3 + hix3p[h];  // prevIndex
            hE.nextEdge = nextIndex;
            auto inspair = oppositeEdgesMap.insert( HalfEdgeMap::value_type( currentVertex<<16 | nextVertex, currentIndex ) );
            if( ! inspair.second )
            {
                return false;
            }
            HalfEdgeMap::iterator oppositeEdgeIterator = oppositeEdgesMap.find( nextVertex<<16 | currentVertex );
            if ( oppositeEdgeIterator != oppositeEdgesMap.end() )
            {
                int oppositeEdgeIndex = oppositeEdgeIterator->second;
                hE.oppEdge = oppositeEdgeIndex;
                halfEdges[oppositeEdgeIndex].oppEdge = currentIndex;
            }
        }
    }
    return true;
}

G3D::Vector3 CSGMeshSgCore::extentsCenter()
{
    return extents.center();
}
G3D::Vector3 CSGMeshSgCore::extentsSize()
{
    return extents.size();
}

} // namespace RBX
