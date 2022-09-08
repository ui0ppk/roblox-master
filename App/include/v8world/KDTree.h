#pragma once

#include "util/G3DCore.h"

class btTriangleCallback;

namespace RBX {

union KDNode
{
    struct
    {
        // left child contents is to the left of splits[0] along axis
        // right child contents is to the right of splits[1] along axis
        float splits[2];

        // axis index; must be 0/1/2 (X/Y/Z)
        unsigned int axis: 2;

        // left child is at childIndex; right child is at childIndex+1
        unsigned int childIndex: 30;
    } branch;

    struct
    {
        unsigned int triangles[2];

        // axis index; must be 3 so that isLeaf() can distinguish nodes
        unsigned int axis: 2;

        unsigned int triangleCount: 30;
    } leaf;

    bool isLeaf() const
    {
        return branch.axis == 3;
    }
};

struct KDTree
{
    const Vector3* vertexPositions;
    const unsigned char* vertexMaterials;
    const unsigned int* indices;

    std::vector<KDNode> nodes;
    size_t depth;
    Vector3 extentsMin;
    Vector3 extentsMax;

    struct RayResult
    {
        float fraction;
        const KDTree* tree;
        unsigned int triangle;

        RayResult(): fraction(1), tree(NULL), triangle(0)
        {
        }

        RayResult(float fraction, const KDTree* tree, unsigned int triangle): fraction(fraction), tree(tree), triangle(triangle)
        {
        }
        
        bool hasHit() const
        {
            return tree != 0;
        }
    };
    
    KDTree();

    void build(const Vector3* vertexPositions, const unsigned char* vertexMaterials, size_t vertexCount, const unsigned int* indices, size_t triangleCount);

    void queryAABB(btTriangleCallback* callback, const Vector3& aabbMin, const Vector3& aabbMax) const;
    void queryRay(RayResult& result, const Vector3& raySource, const Vector3& rayTarget) const;
    
    Vector3 getTriangleNormal(unsigned int triangle) const;
    unsigned char getMaterial(unsigned int triangle, const Vector3& position) const;
};

}