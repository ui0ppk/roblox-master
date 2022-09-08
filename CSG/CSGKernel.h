/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/CSGMesh.h"

#include <boost/unordered_map.hpp>
#include <set>

#include "util/Extents.h"
#include "util/Vector3int32.h"

class sgCObject;

namespace RBX {

class triangulationVertex
{
public:
    triangulationVertex() : duplicateCount(1) {;}
    int duplicateCount;
    int neighborVert[2];  // previous - next
    unsigned int neighborCreaseFlag[2];
};

class CSGHalfEdge
{
public:
    CSGHalfEdge();
    int startVert;
    int prevEdge;
    int nextEdge;
    int oppEdge;
    int face;
    enum
    {
        normalCrease = 0x1,
        colorCrease = 0x2,
        uvCrease = 0x4
    };
    unsigned int creaseFlag;
    bool creaseSet;
};

class CSGMeshSgCore : public CSGMesh
{
public:
  	CSGMeshSgCore();
    CSGMeshSgCore(const CSGMeshSgCore& mesh);
    ~CSGMeshSgCore();

    virtual CSGMesh* clone() const;

    virtual CSGMeshSgCore& operator=(const CSGMeshSgCore& mesh);

    virtual bool isValid() const;

    virtual void translate( const G3D::Vector3& translation );
    virtual void applyCoordinateFrame(G3D::CoordinateFrame cFrame);
    virtual void applyTranslation(const G3D::Vector3& trans);
    virtual void applyScale(const G3D::Vector3& size);
    virtual void applyColor(const G3D::Vector3& color);
    virtual void triangulate();
    virtual bool newTriangulate();
    virtual void weldMesh(bool positionOnly = false);

    virtual std::string getBRepBinaryString() const;
    virtual void setBRepFromBinaryString(const std::string& str);
	sgCObject* brepFromBinaryString(const std::string& str) const;

    virtual void buildBRep();
    
    virtual bool unionMesh(const CSGMesh* a, const CSGMesh* b);
    virtual bool intersectMesh(const CSGMesh* a, const CSGMesh* b);
    virtual bool subractMesh(const CSGMesh* a, const CSGMesh* b);

    virtual size_t clusterVertices( float resolution );
    virtual bool makeHalfEdges( std::vector< int>& vertexEdges );

    virtual G3D::Vector3 extentsCenter();
    virtual G3D::Vector3 extentsSize();

private:
    class EditData
    {
    public:
        EditData(CSGMeshSgCore* mesh);
        EditData(const EditData& editData);
        ~EditData();

        EditData& operator=(const EditData& editData);
        sgCObject* clone() const;
        void destroy();

        void setShape(sgCObject* shapeIn);
        sgCObject* getShape() const { return shape; }

    private:
        sgCObject* shape;
        CSGMeshSgCore* mesh;
    };

    bool sgCoreUnion(const CSGMeshSgCore& a, const CSGMeshSgCore& b);
    bool sgCoreSubtract(const CSGMeshSgCore& a, const CSGMeshSgCore& b);

    EditData editData;

    std::vector<CSGHalfEdge> halfEdges;
    RBX::Extents extents;
    void makeExtents();
};

class CSGMeshFactorySgCore : public CSGMeshFactory
{
public:
    virtual CSGMesh* createMesh();
};

class CSGClustering
{
public:
    CSGClustering( std::vector<unsigned int>& indices, std::vector<CSGVertex>& vertices,
        const Vector3& minpos, float invres );
    void cluster();
    size_t extractVertices();
protected:
    std::vector<CSGVertex>& m_vertices;
    std::vector<unsigned int>& m_indices;
    const Vector3& m_minpos;
    float m_invres;

    class VertexCluster
    {
    public:
        std::set<G3D::uint64> posclasses;
        std::set<int> indices;
    };
    std::vector< VertexCluster >clusters;

    typedef boost::unordered_map <G3D::uint64,int> IPosClassMap;
    IPosClassMap posclasses;

    typedef RBX::Vector3int32 v3i2[2];
    G3D::uint64 makeKey( const v3i2& v, unsigned int ii );
    IPosClassMap::iterator addPos( G3D::uint64 key, int indx );
    void addPosClasses( G3D::uint64 key[8], int indx );
    void mergeClasses( IPosClassMap::iterator it[8] );
};

} // namespace RBX
