#include "stdafx.h"

#include "v8world/KDTree.h"

#include "BulletCollision/CollisionShapes/btTriangleCallback.h"

DYNAMIC_FASTINTVARIABLE(SmoothTerrainPhysicsRayAabbSlop, 0)

namespace RBX {

struct RayNode
{
    unsigned int index;
    float tmin;
    float tmax;

    RayNode()
    {
    }

    RayNode(unsigned int index, float tmin, float tmax)
    : index(index)
    , tmin(tmin)
    , tmax(tmax)
    {
    }
};

static int getOutcode(const Vector3& p, const Vector3& aabbMin, const Vector3& aabbMax)
{
	return (p.x < aabbMin.x ? 0x01 : 0x0) |
		   (p.x > aabbMax.x ? 0x08 : 0x0) |
		   (p.y < aabbMin.y ? 0x02 : 0x0) |
		   (p.y > aabbMax.y ? 0x10 : 0x0) |
		   (p.z < aabbMin.z ? 0x4 : 0x0) |
		   (p.z > aabbMax.z ? 0x20 : 0x0);
}

static bool rayAabb(const Vector3& rayFrom, const Vector3& rayTo, const Vector3& aabbMin, const Vector3& aabbMax, float& paramMin, float& paramMax)
{
	int	sourceOutcode = getOutcode(rayFrom, aabbMin, aabbMax);
	int targetOutcode = getOutcode(rayTo, aabbMin, aabbMax);

	if ((sourceOutcode & targetOutcode) == 0x0)
	{
		Vector3 rayDir = rayTo - rayFrom;

        float lambdaMin = paramMin;
        float lambdaMax = paramMax;
		int bit = 1;

		for (int j = 0; j < 2; ++j)
		{
			const Vector3& aabbBound = j ? aabbMax : aabbMin;

			for (int i = 0; i != 3; ++i)
			{
				if (sourceOutcode & bit)
				{
					float lambda = (aabbBound[i] - rayFrom[i]) / rayDir[i];
                    
                    if (lambdaMin > lambda)
                        lambdaMin = lambda;
				}
				else if (targetOutcode & bit) 
				{
					float lambda = (aabbBound[i] - rayFrom[i]) / rayDir[i];

                    if (lambdaMax < lambda)
                        lambdaMax = lambda;
				}

				bit <<= 1;
			}
		}

		if (lambdaMin <= lambdaMax)
		{
			paramMin = lambdaMin;
            paramMax = lambdaMax;

			return true;
		}
	}

	return false;
}

template <typename T> static T* getBuffer(T (&stackBuffer)[32], boost::scoped_array<T>& heapBuffer, size_t bufferSize)
{
    if (bufferSize <= sizeof(stackBuffer) / sizeof(stackBuffer[0]))
        return stackBuffer;

    heapBuffer.reset(new T[bufferSize]);
    return heapBuffer.get();
}

static void queryAABBStackless(const KDTree* tree, btTriangleCallback* callback, const Vector3& aabbMin, const Vector3& aabbMax)
{
    unsigned int stackBuffer[32];
    boost::scoped_array<unsigned int> heapBuffer;
    unsigned int* buffer = getBuffer(stackBuffer, heapBuffer, tree->depth);

    size_t bufferOffset = 0;

    buffer[bufferOffset++] = 0;

    while (bufferOffset > 0)
    {
        size_t index = buffer[--bufferOffset];

        const KDNode& node = tree->nodes[index];

        if (node.isLeaf())
        {
            size_t triangleCount = node.leaf.triangleCount;

            for (size_t i = 0; i < triangleCount; ++i)
            {
                unsigned int tri = node.leaf.triangles[i];

                unsigned int i0 = tree->indices[3 * tri + 0];
                unsigned int i1 = tree->indices[3 * tri + 1];
                unsigned int i2 = tree->indices[3 * tri + 2];

                btVector3 data[3];
                data[0] = btVector3(tree->vertexPositions[i0].x, tree->vertexPositions[i0].y, tree->vertexPositions[i0].z);
                data[1] = btVector3(tree->vertexPositions[i1].x, tree->vertexPositions[i1].y, tree->vertexPositions[i1].z);
                data[2] = btVector3(tree->vertexPositions[i2].x, tree->vertexPositions[i2].y, tree->vertexPositions[i2].z);

                callback->processTriangle(data, 0, tri);
            }
        }
        else
        {
            int axis = node.branch.axis;
            unsigned int childIndex = node.branch.childIndex;

            RBXASSERT(bufferOffset + 2 <= tree->depth);

            if (node.branch.splits[1] <= aabbMax[axis])
                buffer[bufferOffset++] = childIndex + 1;

            if (node.branch.splits[0] >= aabbMin[axis])
                buffer[bufferOffset++] = childIndex + 0;
        }
    }
}

static void rayTriangle(KDTree::RayResult& result, const Vector3& raySource, const Vector3& rayDir, const Vector3& v0, const Vector3& v1, const Vector3& v2, const KDTree* tree, unsigned int tri)
{
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 P = rayDir.cross(edge2);

    float det = edge1.dot(P);
    if (det <= 0)
        return;

    Vector3 T = raySource - v0;

    float u = T.dot(P);
    if (u < 0 || u > det)
        return;

    Vector3 Q = T.cross(edge1);

    float v = rayDir.dot(Q);
    if (v < 0 || u + v > det)
        return;

    float t = edge2.dot(Q) / det;

    if (t < 0 || t >= result.fraction)
        return;

    result = KDTree::RayResult(t, tree, tri);
}

static void queryRayStackless(const KDTree* tree, KDTree::RayResult& result, const Vector3& raySource, const Vector3& rayTarget, float tmin, float tmax)
{
    RayNode stackBuffer[32];
    boost::scoped_array<RayNode> heapBuffer;
    RayNode* buffer = getBuffer(stackBuffer, heapBuffer, tree->depth);

    Vector3 rayDir = rayTarget - raySource;

    size_t bufferOffset = 0;

    buffer[bufferOffset++] = RayNode(0, tmin, tmax);

    while (bufferOffset > 0)
    {
        RayNode rn = buffer[--bufferOffset];

        if (rn.tmin >= result.fraction)
            continue;

        const KDNode& node = tree->nodes[rn.index];

        if (node.isLeaf())
        {
            size_t triangleCount = node.leaf.triangleCount;

            for (size_t i = 0; i < triangleCount; ++i)
            {
                unsigned int tri = node.leaf.triangles[i];

                unsigned int i0 = tree->indices[3 * tri + 0];
                unsigned int i1 = tree->indices[3 * tri + 1];
                unsigned int i2 = tree->indices[3 * tri + 2];

                const Vector3& v0 = tree->vertexPositions[i0];
                const Vector3& v1 = tree->vertexPositions[i1];
                const Vector3& v2 = tree->vertexPositions[i2];

                rayTriangle(result, raySource, rayDir, v0, v1, v2, tree, tri);
            }
        }
        else
        {
            int axis = node.branch.axis;
            unsigned int childIndex = node.branch.childIndex;

            RBXASSERT(bufferOffset + 2 <= tree->depth);

            float sa = raySource[axis];
            float da = rayDir[axis];

            if (da == 0)
            {
                if (node.branch.splits[0] >= sa)
                    buffer[bufferOffset++] = RayNode(childIndex + 0, rn.tmin, rn.tmax);

                if (node.branch.splits[1] <= sa)
                    buffer[bufferOffset++] = RayNode(childIndex + 1, rn.tmin, rn.tmax);
            }
            else
            {
                // start with the node that's closer to the ray origin
                int i0 = (da > 0) ? 0 : 1;
                int i1 = 1 - i0;

                float t0 = (node.branch.splits[i0] - sa) / da;
                float t1 = (node.branch.splits[i1] - sa) / da;

                if (t1 <= rn.tmax)
                    buffer[bufferOffset++] = RayNode(childIndex + i1, std::max(t1, rn.tmin), rn.tmax);

                if (t0 >= rn.tmin)
                    buffer[bufferOffset++] = RayNode(childIndex + i0, rn.tmin, std::min(t0, rn.tmax));
            }
        }
    }
}

KDTree::KDTree()
: vertexPositions(0)
, vertexMaterials(0)
, indices(0)
, depth(0)
{
}

void KDTree::queryAABB(btTriangleCallback* callback, const Vector3& aabbMin, const Vector3& aabbMax) const
{
    if (nodes.empty())
        return;

    Vector3 clampedMin = aabbMin.max(extentsMin);
    Vector3 clampedMax = aabbMax.min(extentsMax);

    queryAABBStackless(this, callback, clampedMin, clampedMax);
}

void KDTree::queryRay(RayResult& result, const Vector3& raySource, const Vector3& rayTarget) const
{
    if (nodes.empty())
        return;

    float tmin = 0, tmax = 1;

	if (!rayAabb(raySource, rayTarget, extentsMin, extentsMax, tmin, tmax))
		return;

    // btRayAabbExact is not too exact - floating point imprecision means we could lose the ray hit unless we extend the min/max bounds a bit
	if (DFInt::SmoothTerrainPhysicsRayAabbSlop)
	{
		tmin = std::max(tmin - DFInt::SmoothTerrainPhysicsRayAabbSlop / 1000.f, 0.f);
		tmax = std::min(tmax + DFInt::SmoothTerrainPhysicsRayAabbSlop / 1000.f, 1.f);
	}

    queryRayStackless(this, result, raySource, rayTarget, tmin, tmax);
}

Vector3 KDTree::getTriangleNormal(unsigned int triangle) const
{
    unsigned int i0 = indices[3 * triangle + 0];
    unsigned int i1 = indices[3 * triangle + 1];
    unsigned int i2 = indices[3 * triangle + 2];

    const Vector3& v0 = vertexPositions[i0];
    const Vector3& v1 = vertexPositions[i1];
    const Vector3& v2 = vertexPositions[i2];

    return (v1 - v0).unitCross(v2 - v0);
}
    
unsigned char KDTree::getMaterial(unsigned int triangle, const Vector3& position) const
{
    unsigned int i0 = indices[3 * triangle + 0];
    unsigned int i1 = indices[3 * triangle + 1];
    unsigned int i2 = indices[3 * triangle + 2];
    
    const Vector3& v0 = vertexPositions[i0];
    const Vector3& v1 = vertexPositions[i1];
    const Vector3& v2 = vertexPositions[i2];
    
    unsigned char m0 = vertexMaterials[i0];
    unsigned char m1 = vertexMaterials[i1];
    unsigned char m2 = vertexMaterials[i2];

    float d0 = (v0 - position).squaredLength();
    float d1 = (v1 - position).squaredLength();
    float d2 = (v2 - position).squaredLength();

    return (d0 < d1 && d0 < d2) ? m0 : (d1 < d2) ? m1 : m2;
}


struct KDTreeBuilder
{
    struct Triangle
    {
        unsigned int index;

        Vector3 midpoint;
    };

    template <int Axis> struct TriangleAxisSplitter
    {
        float divider;

        TriangleAxisSplitter(float divider)
        : divider(divider)
        {
        }

        bool operator()(const Triangle& lhs) const
        {
            return lhs.midpoint[Axis] < divider;
        }
    };

    std::pair<size_t, Extents> build(size_t vertexCount, size_t triangleCount)
    {
        if (triangleCount == 0)
            return std::make_pair(0, Extents());

        triangles.resize(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            unsigned int i0 = indices[i * 3 + 0];
            unsigned int i1 = indices[i * 3 + 1];
            unsigned int i2 = indices[i * 3 + 2];

            Triangle& t = triangles[i];

			t.index = i;
			t.midpoint = (vertices[i0] + vertices[i1] + vertices[i2]) * (1.f / 3);
        }

        nodes->clear();
        nodes->push_back(KDNode());

        return split(0, 0, triangleCount);
    }

    std::pair<size_t, Extents> split(size_t index, size_t begin, size_t end)
    {
        if (end - begin <= 2)
        {
            KDNode& node = (*nodes)[index];
            node.leaf.axis = 3;
            node.leaf.triangleCount = end - begin;

            Extents e;

            for (size_t i = begin; i < end; ++i)
            {
                unsigned int tri = triangles[i].index;
                unsigned int i0 = indices[tri * 3 + 0];
                unsigned int i1 = indices[tri * 3 + 1];
                unsigned int i2 = indices[tri * 3 + 2];

                node.leaf.triangles[i - begin] = tri;

                e.expandToContain(vertices[i0]);
                e.expandToContain(vertices[i1]);
                e.expandToContain(vertices[i2]);
            }

            return std::make_pair(1, e);
        }
        else
        {
            // gather midpoint stats
            Vector3 min = Vector3::maxFinite();
            Vector3 max = Vector3::minFinite();
            Vector3 avg = Vector3();

            size_t size = end - begin;

            for (size_t i = begin; i < end; ++i)
            {
                const Vector3& mp = triangles[i].midpoint;

                min = min.min(mp);
                max = max.max(mp);
                avg += mp;
            }

            avg /= size;

            // partition triangles
            Vector3 ext = max - min;

            int axis;
            std::vector<Triangle>::iterator it;

            if (ext.x > ext.y && ext.x > ext.z)
            {
                axis = 0;
                it = std::partition(triangles.begin() + begin, triangles.begin() + end, TriangleAxisSplitter<0>(avg.x));
            }
            else if (ext.y > ext.z)
            {
                axis = 1;
                it = std::partition(triangles.begin() + begin, triangles.begin() + end, TriangleAxisSplitter<1>(avg.y));
            }
            else
            {
                axis = 2;
                it = std::partition(triangles.begin() + begin, triangles.begin() + end, TriangleAxisSplitter<2>(avg.z));
            }

            // repartition in half to keep balance
            size_t pr = it - triangles.begin();
            size_t partitioned = pr - begin;

            if (partitioned <= size / 4 || partitioned >= size - size / 4)
                pr = begin + size / 2;

            // recurse
            size_t childIndex = nodes->size();

            nodes->push_back(KDNode());
            nodes->push_back(KDNode());

            auto ln = split(childIndex + 0, begin, pr);
            auto rn = split(childIndex + 1, pr, end);

            KDNode& node = (*nodes)[index];
            node.branch.splits[0] = ln.second.max()[axis];
            node.branch.splits[1] = rn.second.min()[axis];
            node.branch.axis = axis;
            node.branch.childIndex = childIndex;

            Extents e = ln.second;
            e.expandToContain(rn.second);

            return std::make_pair(std::max(ln.first, rn.first) + 1, e);
        }
    }

    const Vector3* vertices;
    const unsigned int* indices;
    std::vector<KDNode>* nodes;

    std::vector<Triangle> triangles;
};

void KDTree::build(const Vector3* vertexPositions, const unsigned char* vertexMaterials, size_t vertexCount, const unsigned int* indices, size_t triangleCount)
{
    KDTreeBuilder builder;
    builder.vertices = vertexPositions;
    builder.indices = indices;
    builder.nodes = &this->nodes;

    auto p = builder.build(vertexCount, triangleCount);

    this->vertexPositions = vertexPositions;
    this->vertexMaterials = vertexMaterials;
    this->indices = indices;
    this->depth = p.first;
    this->extentsMin = p.second.min();
    this->extentsMax = p.second.max();
}

}