#include "stdafx.h"
#include "voxel2/Mesher.h"

#include "voxel2/Grid.h"
#include "voxel2/MaterialTable.h"

#include "rbx/Profiler.h"

namespace RBX { namespace Voxel2 { namespace Mesher {

	static const unsigned char kVertexIndexTable[][3] =
	{
		{0, 0, 0},
		{1, 0, 0},
		{1, 1, 0},
		{0, 1, 0},

		{0, 0, 1},
		{1, 0, 1},
		{1, 1, 1},
		{0, 1, 1},
	};

	static const unsigned char kEdgeVertexTable[12][2][3] =
	{
		{ {0, 0, 0}, {1, 0, 0} },
		{ {1, 0, 0}, {1, 1, 0} },
		{ {1, 1, 0}, {0, 1, 0} },
		{ {0, 1, 0}, {0, 0, 0} },

		{ {0, 0, 1}, {1, 0, 1} },
		{ {1, 0, 1}, {1, 1, 1} },
		{ {1, 1, 1}, {0, 1, 1} },
		{ {0, 1, 1}, {0, 0, 1} },

		{ {0, 0, 0}, {0, 0, 1} },
		{ {1, 0, 0}, {1, 0, 1} },
		{ {1, 1, 0}, {1, 1, 1} },
		{ {0, 1, 0}, {0, 1, 1} },
	};

	static const Vector3 kTextureBasisU[18] =
	{
		Vector3(0, 0, -1),
		Vector3(0, 0, 1),
		Vector3(1, 0, 0),
		Vector3(-1, 0, 0),

		Vector3(0.7, 0, -0.7),
		Vector3(-0.7, 0, -0.7),
		Vector3(0.7, 0, 0.7),
		Vector3(-0.7, 0, 0.7),

		Vector3(1, 0, 0),

		Vector3(0.7, -0.7, 0),
		Vector3(0.7, 0.7, 0),
		Vector3(1, 0, 0),
		Vector3(1, 0, 0),

		Vector3(-1, 0, 0),

		Vector3(-0.7, -0.7, 0),
		Vector3(-0.7, 0.7, 0),
		Vector3(-1, 0, 0),
		Vector3(-1, 0, 0),
	};

	static const Vector3 kTextureBasisV[18] =
	{
		Vector3(0, -1, 0),
		Vector3(0, -1, 0),
		Vector3(0, -1, 0),
		Vector3(0, -1, 0),

		Vector3(0, -1, 0),
		Vector3(0, -1, 0),
		Vector3(0, -1, 0),
		Vector3(0, -1, 0),

		Vector3(0, 0, 1),

		Vector3(0, 0, 1),
		Vector3(0, 0, 1),
		Vector3(0, -0.7, 0.7),
		Vector3(0, 0.7, 0.7),

		Vector3(0, 0, -1),

		Vector3(0, 0, -1),
		Vector3(0, 0, -1),
		Vector3(0, -0.7, -0.7),
		Vector3(0, 0.7, -0.7),
	};

	static unsigned short gEdgeTable[3*3*3*3*3*3*3*3];

	void prepareTables()
	{
		for (int i0 = 0; i0 < 81; ++i0)
            for (int i1 = 0; i1 < 81; ++i1)
			{
				int t[2][2][2] =
				{
					{
						{ i0 % 3, (i0 / 3) % 3 },
						{ (i0 / 9) % 3, (i0 / 27) % 3 }
					},
					{
						{ i1 % 3, (i1 / 3) % 3 },
						{ (i1 / 9) % 3, (i1 / 27) % 3 }
					}
				};

				int edgemask = 0;

				for (int i = 0; i < 12; ++i)
				{
					const unsigned char (&e)[2][3] = kEdgeVertexTable[i];

					int p0x = e[0][0], p0y = e[0][1], p0z = e[0][2], p1x = e[1][0], p1y = e[1][1], p1z = e[1][2];

					if (t[p0x][p0y][p0z] != t[p1x][p1y][p1z])
						edgemask |= 1 << i;
				}

				gEdgeTable[i0 + 81 * i1] = edgemask;
			}
	}

	struct GridVertex
	{
		unsigned char tag;
		unsigned char occupancy;
	};

	static void pushQuad(std::vector<unsigned int>& ib,
		const std::vector<Vertex>& vb,
		unsigned int v0, unsigned int v1, unsigned int v2, unsigned int v3,
		bool flip)
	{
		RBXASSERT(v0 < vb.size() && v1 < vb.size() && v2 < vb.size() && v3 < vb.size());

		unsigned int m0 = vb[v0].material;
		unsigned int m1 = vb[v1].material;
		unsigned int m2 = vb[v2].material;
		unsigned int m3 = vb[v3].material;

		// For quads with a material transition we pick the diagonal that minimizes interpolation artifacts
		bool flipdiag = (m1 == m3) && (m1 == m2 || m1 == m0);

		unsigned int v[] = {v0, v1, v2, v3};

		// Note: indices here are arranged in the order "ABC CBD" ("strip order") to make it easy to extract diagonal later
		static const unsigned int kOffsetTable[2][2][6] =
		{
			{
				{ 1, 0, 2, 2, 0, 3 },
				{ 1, 2, 0, 0, 2, 3 },
			},
			{
				{ 0, 3, 1, 1, 3, 2 },
				{ 0, 1, 3, 3, 1, 2 },
            }
		};

		const unsigned int* offsets = kOffsetTable[flipdiag][flip];

		ib.push_back(v[offsets[0]]);
		ib.push_back(v[offsets[1]]);
		ib.push_back(v[offsets[2]]);
		ib.push_back(v[offsets[3]]);
		ib.push_back(v[offsets[4]]);
		ib.push_back(v[offsets[5]]);
	}

    static Vector3 round(const Vector3& v)
	{
        int x = (v.x < 0) ? int(v.x - 0.5) : int(v.x + 0.5);
        int y = (v.y < 0) ? int(v.y - 0.5) : int(v.y + 0.5);
        int z = (v.z < 0) ? int(v.z - 0.5) : int(v.z + 0.5);

        return Vector3(x, y, z);
	}

	static size_t avalanche(size_t v)
	{
		v += ~(v << 15);
		v ^= (v >> 10);
		v += (v << 3);
		v ^= (v >> 6);
		v += ~(v << 11);
		v ^= (v >> 16);

		return v;
	}

    static unsigned int computeSeed(const Vector3int32& p)
	{
        size_t result = 0;

		boost::hash_combine(result, p.x);
		boost::hash_combine(result, p.y);
		boost::hash_combine(result, p.z);

        return avalanche(result);
	}

	static Vector3 computePoint(const Vector3& smooth, float cellSize, const MaterialTable* materials, unsigned char material, size_t seed)
	{
		const MaterialTable::Material& m = materials->getMaterial(material);

		Vector3 center = Vector3(cellSize * 0.5f);

        switch (m.deformation)
        {
        case MaterialTable::Deformation_Shift:
			return smooth + (Vector3((seed & 255) / 255.f, ((seed >> 8) & 255) / 255.f, ((seed >> 16) & 255) / 255.f) * 2.f - Vector3(1.f)) * (m.parameter * cellSize);

        case MaterialTable::Deformation_Cubify:
			return lerp(smooth, center, m.parameter);

        case MaterialTable::Deformation_Quantize:
			return round((smooth - center) / m.parameter) * m.parameter + center;

        case MaterialTable::Deformation_Barrel:
            return Vector3(smooth.x, G3D::lerp(smooth.y, center.y, m.parameter), smooth.z);

        case MaterialTable::Deformation_Water:
            return Vector3(smooth.x, smooth.y - m.parameter, smooth.z);

        default:
            return smooth;
        }
	}

	inline std::pair<unsigned int, unsigned char> reduceMaterials(const std::pair<unsigned int, unsigned char>& m0, const std::pair<unsigned int, unsigned char>& m1)
	{
		if (m0.second == m1.second)
			return std::make_pair(m0.first + m1.first, m0.second);
		else if (m0.first != m1.first)
			return m0.first > m1.first ? m0 : m1;
		else
			return m0.second < m1.second ? m0 : m1;
	}

	static void extractGridVertices(GridVertex* gv, const Box& box, const Options& options)
	{
		int sizeX = box.getSizeX(), sizeY = box.getSizeY(), sizeZ = box.getSizeZ();
		int sizeXZ = sizeX * sizeZ;

		unsigned int tagCutoff = options.generateWater ? Cell::Material_Air : Cell::Material_Water;

        for (int y = 0; y < sizeY; ++y)
            for (int z = 0; z < sizeZ; ++z)
			{
				GridVertex* gvrow = gv + sizeXZ * y + sizeX * z;
				const Cell* row = box.readRow(0, y, z);

				for (int x = 0; x < sizeX; ++x)
				{
					const Cell& c = row[x];
					GridVertex& v = gvrow[x];

					v.tag = (c.getMaterial() <= tagCutoff) ? 0 : (c.getMaterial() == Cell::Material_Water) ? 1 : 2;
					v.occupancy = c.getOccupancy();
				}
			}
	}

	static Vertex generateVertex(GridVertex* gv, const Box& box, const Vector3int32& offset, int lod, const Options& options, int x, int y, int z, int edgemask)
	{
		float cellSize = 1 << lod;

		int sizeX = box.getSizeX(), sizeY = box.getSizeY(), sizeZ = box.getSizeZ();
		int sizeXZ = sizeX * sizeZ;

		Vector3 corner = Vector3(offset.x, offset.y, offset.z) + Vector3(x, y, z) * cellSize;

		size_t ecount = 0;
		Vector3 eavg;

		// add vertices
		for (int i = 0; i < 12; ++i)
		{
			if (edgemask & (1 << i))
			{
				const unsigned char (&e)[2][3] = kEdgeVertexTable[i];

				int p0x = e[0][0], p0y = e[0][1], p0z = e[0][2], p1x = e[1][0], p1y = e[1][1], p1z = e[1][2];

				const GridVertex& g0 = gv[(x + p0x) + sizeXZ * (y + p0y) + sizeX * (z + p0z)];
				const GridVertex& g1 = gv[(x + p1x) + sizeXZ * (y + p1y) + sizeX * (z + p1z)];

				float occScale = 1.f / (Cell::Occupancy_Max + 1);
				float t = g0.tag > g1.tag ? (g0.occupancy + 1) * occScale : 1 - (g1.occupancy + 1) * occScale;

				eavg += lerp(Vector3(p0x, p0y, p0z) * cellSize, Vector3(p1x, p1y, p1z) * cellSize, t);
				ecount++;
			}
		}

		// compute materials
		unsigned int vcount = 0;
		std::pair<unsigned int, unsigned char> vmat[8] = {};

		for (int i = 0; i < 8; ++i)
		{
			const Cell& c = box.get(x + kVertexIndexTable[i][0], y + kVertexIndexTable[i][1], z + kVertexIndexTable[i][2]);

			if (c.getMaterial() > Cell::Material_Water)
			{
				vmat[vcount] = std::make_pair(c.getOccupancy() + 1, c.getMaterial());
				vcount++;
			}
		}

		// reduce materials
		std::pair<unsigned int, unsigned char> vmr;

		if (vcount > 0)
		{
			vmr = reduceMaterials(reduceMaterials(vmat[0], vmat[1]), reduceMaterials(vmat[2], vmat[3]));

			if (vcount > 4)
				vmr = reduceMaterials(vmr, reduceMaterials(reduceMaterials(vmat[4], vmat[5]), reduceMaterials(vmat[6], vmat[7])));
		}
		else
		{
			vmr.second = Cell::Material_Water;
		}

		unsigned char material = vmr.second;

		bool border = (x == 0 || x == sizeX - 2 || y == 0 || y == sizeY - 2 || z == 0 || z == sizeZ - 2);

		unsigned int seed = computeSeed(Vector3int32(x, y, z) + (offset >> lod));

		Vector3 point = computePoint(eavg / float(ecount), cellSize, options.materials, material, seed);
		Vector3 position = corner + G3D::clamp(point, Vector3(), Vector3(cellSize)) + Vector3(0.5f);

		Vertex v = { position, border, 0, material, seed };

		return v;
	}

	static void generateIndices(std::vector<unsigned int>& ib, const std::vector<Vertex>& vb, const GridVertex* gv, const unsigned int* gp, int sizeX, int sizeY, int sizeZ)
	{
		int sizeXZ = sizeX * sizeZ;

        for (int y = 1; y + 1 < sizeY; ++y)
            for (int z = 1; z + 1 < sizeZ; ++z)
                for (int x = 1; x + 1 < sizeX; ++x)
                {
                    const GridVertex& v000 = gv[(x + 0) + sizeXZ * (y + 0) + sizeX * (z + 0)];
                    const GridVertex& v100 = gv[(x + 1) + sizeXZ * (y + 0) + sizeX * (z + 0)];
                    const GridVertex& v010 = gv[(x + 0) + sizeXZ * (y + 1) + sizeX * (z + 0)];
                    const GridVertex& v001 = gv[(x + 0) + sizeXZ * (y + 0) + sizeX * (z + 1)];

                    // add quads
                    if (v000.tag != v100.tag)
                    {
                        pushQuad(ib, vb,
                            gp[(x + 0) + sizeXZ * (y + 0) + sizeX * (z + 0)],
                            gp[(x + 0) + sizeXZ * (y - 1) + sizeX * (z + 0)],
                            gp[(x + 0) + sizeXZ * (y - 1) + sizeX * (z - 1)],
                            gp[(x + 0) + sizeXZ * (y + 0) + sizeX * (z - 1)],
                            v000.tag > v100.tag);
                    }

                    if (v000.tag != v010.tag)
                    {
                        pushQuad(ib, vb,
                            gp[(x + 0) + sizeXZ * (y + 0) + sizeX * (z + 0)],
                            gp[(x - 1) + sizeXZ * (y + 0) + sizeX * (z + 0)],
                            gp[(x - 1) + sizeXZ * (y + 0) + sizeX * (z - 1)],
                            gp[(x + 0) + sizeXZ * (y + 0) + sizeX * (z - 1)],
                            v000.tag < v010.tag);
                    }

                    if (v000.tag != v001.tag)
                    {
                        pushQuad(ib, vb,
                            gp[(x + 0) + sizeXZ * (y + 0) + sizeX * (z + 0)],
                            gp[(x - 1) + sizeXZ * (y + 0) + sizeX * (z + 0)],
                            gp[(x - 1) + sizeXZ * (y - 1) + sizeX * (z + 0)],
                            gp[(x + 0) + sizeXZ * (y - 1) + sizeX * (z + 0)],
                            v000.tag > v001.tag);
                    }
                }
	}

	BasicMesh generateGeometry(const Box& box, const Vector3int32& offset, int lod, const Options& options)
	{
		if (box.isEmpty())
			return BasicMesh();

		RBXPROFILER_SCOPE("Voxel", "generateGeometry");

		int sizeX = box.getSizeX(), sizeY = box.getSizeY(), sizeZ = box.getSizeZ();
		RBXASSERT(sizeX > 2 && sizeY > 2 && sizeZ > 2);

		int sizeXZ = sizeX * sizeZ;

		boost::scoped_array<GridVertex> gv(new GridVertex[sizeX * sizeZ * sizeY]);

		extractGridVertices(gv.get(), box, options);

        BasicMesh result;

        boost::scoped_array<unsigned int> gp(new unsigned int[sizeX * sizeZ * sizeY]);

        for (int y = 0; y + 1 < sizeY; ++y)
            for (int z = 0; z + 1 < sizeZ; ++z)
			{
				int offsetYZ = sizeXZ * y + sizeX * z;

				int tagi0 = gv[offsetYZ].tag + 3 * (gv[offsetYZ + sizeX].tag + 3 * (gv[offsetYZ + sizeXZ].tag + 3 * gv[offsetYZ + sizeXZ + sizeX].tag));

                for (int x = 0; x + 1 < sizeX; ++x)
                {
					int offsetNextXYZ = offsetYZ + x + 1;

					int tagi1 = gv[offsetNextXYZ].tag + 3 * gv[offsetNextXYZ + sizeX].tag + 9 * gv[offsetNextXYZ + sizeXZ].tag + 27 * gv[offsetNextXYZ + sizeXZ + sizeX].tag;

					int edgemask = gEdgeTable[tagi0 + 81 * tagi1];

					tagi0 = tagi1;

                    if (edgemask != 0)
					{
						Vertex v = generateVertex(gv.get(), box, offset, lod, options, x, y, z, edgemask);

                        gp[x + sizeXZ * y + sizeX * z] = result.vertices.size();

                        result.vertices.push_back(v);                    
					}
                }
			}

		generateIndices(result.indices, result.vertices, gv.get(), gp.get(), sizeX, sizeY, sizeZ);

        return result;
	}

    inline std::pair<Vector3, Vector3> computeNormals(const Vector3& hard, const Vector3& soft, const MaterialTable* materials, unsigned char material)
	{
		const MaterialTable::Material& m = materials->getMaterial(material);

        switch (m.type)
        {
        case MaterialTable::Type_Hard:
            return std::make_pair(hard, hard);

        case MaterialTable::Type_HardSoft:
            if (hard.dot(soft) > 0.75)
                return std::make_pair(hard, soft);
            else
                return std::make_pair(hard, hard);

        default:
            return std::make_pair(soft, soft);
        }
	}

	inline int getNormalSegment2D_4(float x, float z)
	{
		if (fabsf(x) > fabsf(z))
			return 0 + (x < 0);
		else
			return 2 + (z < 0);
	}

	inline int getNormalSegment2D_8(float x, float z)
	{
		float ax = fabsf(x);
		float az = fabsf(z);

		if (ax > az * 2)
			return 0 + (x < 0);
		else if (az > ax * 2)
			return 2 + (z < 0);
		else
			return 4 + 2 * (x < 0) + (z < 0);
	}

	inline int getNormalSegmentDefault(const Vector3& normal)
	{
		if (normal.y > 0.9)
			return 8;
		else if (normal.y > 0.4)
			return 9 + getNormalSegment2D_4(normal.x, normal.z);
		else if (normal.y < -0.8)
			return 13;
		else if (normal.y < -0.6)
			return 14 + getNormalSegment2D_4(normal.x, normal.z);
		else
			return getNormalSegment2D_8(normal.x, normal.z);
	}

	inline int getNormalSegmentCube(const Vector3& normal)
	{
		float ax = fabsf(normal.x);
		float az = fabsf(normal.z);

		if (normal.y > ax && normal.y > az)
			return 8;
		else if (normal.y < -ax && normal.y < -az)
			return 13;
		else if (ax > az)
			return 0 + (normal.x < 0);
		else
			return 2 + (normal.z < 0);
	}

    inline int getNormalSegment(const Vector3& normal, MaterialTable::Mapping mapping)
	{
        switch (mapping)
        {
        case MaterialTable::Mapping_Cube:
            return getNormalSegmentCube(normal);
        default:
            return getNormalSegmentDefault(normal);
        }
    }

	inline Color3uint8 packNormal(const Vector3& normal)
	{
        float x = normal.x * 127.f + 127.5f;
        float y = normal.y * 127.f + 127.5f;
        float z = normal.z * 127.f + 127.5f;

        return Color3uint8(int(x), int(y), int(z));
	}

	inline Color4uint8 packMaterial(const MaterialTable* materials, unsigned int material, const Vector3& normal, unsigned int seed)
	{
		const MaterialTable::Material& desc = materials->getMaterial(material);

		int ns = getNormalSegment(normal, desc.mapping);
		int layer = (ns >= 13) ? desc.bottomLayer : (ns >= 8) ? desc.topLayer : desc.sideLayer;

		return Color4uint8(layer, ns, seed, seed >> 8);
	}
    
    GraphicsMesh generateGraphicsGeometry(const BasicMesh& mesh, const Options& options)
	{
		if (mesh.indices.empty())
			return GraphicsMesh();

		RBXPROFILER_SCOPE("Voxel", "generateGraphicsGeometry");

        size_t triangleCount = mesh.indices.size() / 3;

		GraphicsMesh result;

		result.vertices.resize(mesh.indices.size());
		result.solidIndices.reserve(mesh.indices.size());
		result.waterIndices.reserve(mesh.indices.size());

        // water flags
        std::vector<char> iswater(triangleCount);

		// build normals
		std::vector<Vector3> softnormals(mesh.vertices.size());
        std::vector<Vector3> hardnormals(triangleCount);

		for (size_t i = 0; i < triangleCount; ++i)
		{
			unsigned int i0 = mesh.indices[3 * i + 0];
			unsigned int i1 = mesh.indices[3 * i + 1];
			unsigned int i2 = mesh.indices[3 * i + 2];

            const Vertex& v0 = mesh.vertices[i0];
            const Vertex& v1 = mesh.vertices[i1];
            const Vertex& v2 = mesh.vertices[i2];

			Vector3 vn = cross(v1.position - v0.position, v2.position - v0.position);

			softnormals[i0] += vn;
			softnormals[i1] += vn;
			softnormals[i2] += vn;

			hardnormals[i] = normalize(vn);

            iswater[i] = BasicMesh::isWater(v0, v1, v2);
		}

		for (size_t i = 0; i < mesh.vertices.size(); ++i)
		{
			softnormals[i] = normalize(softnormals[i]);
		}

        for (size_t i = 0; i < triangleCount; ++i)
		{
			unsigned int i0 = mesh.indices[3 * i + 0];
			unsigned int i1 = mesh.indices[3 * i + 1];
			unsigned int i2 = mesh.indices[3 * i + 2];

			const Vertex& v0 = mesh.vertices[i0];
			const Vertex& v1 = mesh.vertices[i1];
			const Vertex& v2 = mesh.vertices[i2];

            Vector3 hn = hardnormals[i];

            std::pair<Vector3, Vector3> n0 = computeNormals(hn, softnormals[i0], options.materials, v0.material);
            std::pair<Vector3, Vector3> n1 = computeNormals(hn, softnormals[i1], options.materials, v1.material);
            std::pair<Vector3, Vector3> n2 = computeNormals(hn, softnormals[i2], options.materials, v2.material);

			Color3uint8 pn0 = packNormal(n0.first);
			Color3uint8 pn1 = packNormal(n1.first);
			Color3uint8 pn2 = packNormal(n2.first);

			Color4uint8 m0 = packMaterial(options.materials, v0.material, n0.second, v0.seed);
			Color4uint8 m1 = packMaterial(options.materials, v1.material, n1.second, v1.seed);
			Color4uint8 m2 = packMaterial(options.materials, v2.material, n2.second, v2.seed);

			GraphicsVertex gv0 = { v0.position, Color4uint8(pn0, 0), m0, m1, m2 };
			GraphicsVertex gv1 = { v1.position, Color4uint8(pn1, 1), m0, m1, m2 };
			GraphicsVertex gv2 = { v2.position, Color4uint8(pn2, 2), m0, m1, m2 };

			result.vertices[3 * i + 0] = gv0;
			result.vertices[3 * i + 1] = gv1;
			result.vertices[3 * i + 2] = gv2;

            if (v0.border + v1.border + v2.border == 0)
			{
				if (iswater[i])
				{
					result.waterIndices.push_back(3 * i + 0);
                    result.waterIndices.push_back(3 * i + 1);
                    result.waterIndices.push_back(3 * i + 2);
				}
				else
				{
                    result.solidIndices.push_back(3 * i + 0);
                    result.solidIndices.push_back(3 * i + 1);
                    result.solidIndices.push_back(3 * i + 2);

                    RBXASSERT((i ^ 1) < triangleCount);

                    if (iswater[i ^ 1])
                    {
                        result.solidIndices.push_back(3 * i + 0);
                        result.solidIndices.push_back(3 * i + 2);
                        result.solidIndices.push_back(3 * i + 1);
                    }
				}
			}
		}

		return result;
	}
    
    inline Vector3int16 packPosition(const Vector3& position, const Vector4& packInfo)
    {
        float x = position.x * packInfo.w + packInfo.x + 0.5f;
        float y = position.y * packInfo.w + packInfo.y + 0.5f;
        float z = position.z * packInfo.w + packInfo.z + 0.5f;
        
        return Vector3int16(int(x), int(y), int(z));
    }

    GraphicsMeshPacked generateGraphicsGeometryPacked(const BasicMesh& mesh, const Vector4& packInfo, const Options& options)
	{
		if (mesh.indices.empty())
			return GraphicsMeshPacked();

		RBXPROFILER_SCOPE("Voxel", "generateGraphicsGeometryPacked");

        size_t triangleCount = mesh.indices.size() / 3;

		GraphicsMeshPacked result;

		result.vertices.reserve(mesh.indices.size());
		result.solidIndices.reserve(mesh.indices.size());
		result.waterIndices.reserve(mesh.indices.size());

        // water flags
        std::vector<char> iswater(triangleCount);

		// build normals
		std::vector<Vector3> softnormals(mesh.vertices.size());
        std::vector<Vector3> hardnormals(triangleCount);

		for (size_t i = 0; i < triangleCount; ++i)
		{
			unsigned int i0 = mesh.indices[3 * i + 0];
			unsigned int i1 = mesh.indices[3 * i + 1];
			unsigned int i2 = mesh.indices[3 * i + 2];

            const Vertex& v0 = mesh.vertices[i0];
            const Vertex& v1 = mesh.vertices[i1];
            const Vertex& v2 = mesh.vertices[i2];

			Vector3 vn = cross(v1.position - v0.position, v2.position - v0.position);

			softnormals[i0] += vn;
			softnormals[i1] += vn;
			softnormals[i2] += vn;

			hardnormals[i] = normalize(vn);

            iswater[i] = BasicMesh::isWater(v0, v1, v2);
		}

		for (size_t i = 0; i < mesh.vertices.size(); ++i)
		{
			softnormals[i] = normalize(softnormals[i]);
		}

        for (size_t i = 0; i < triangleCount; ++i)
		{
			unsigned int i0 = mesh.indices[3 * i + 0];
			unsigned int i1 = mesh.indices[3 * i + 1];
			unsigned int i2 = mesh.indices[3 * i + 2];

			const Vertex& v0 = mesh.vertices[i0];
			const Vertex& v1 = mesh.vertices[i1];
			const Vertex& v2 = mesh.vertices[i2];

            if (v0.border + v1.border + v2.border != 0)
                continue;

            Vector3 hn = hardnormals[i];

            std::pair<Vector3, Vector3> n0 = computeNormals(hn, softnormals[i0], options.materials, v0.material);
            std::pair<Vector3, Vector3> n1 = computeNormals(hn, softnormals[i1], options.materials, v1.material);
            std::pair<Vector3, Vector3> n2 = computeNormals(hn, softnormals[i2], options.materials, v2.material);

			Color3uint8 pn0 = packNormal(n0.first);
			Color3uint8 pn1 = packNormal(n1.first);
			Color3uint8 pn2 = packNormal(n2.first);

			Color4uint8 m0 = packMaterial(options.materials, v0.material, n0.second, 0);
			Color4uint8 m1 = packMaterial(options.materials, v1.material, n1.second, 0);
			Color4uint8 m2 = packMaterial(options.materials, v2.material, n2.second, 0);
            
            Color4uint8 mp0 = Color4uint8(m0.r, m1.r, m2.r, v1.seed);
            Color4uint8 mp1 = Color4uint8(m0.g, m1.g, m2.g, v2.seed);

            Vector3int16 p0 = packPosition(v0.position, packInfo);
            Vector3int16 p1 = packPosition(v1.position, packInfo);
            Vector3int16 p2 = packPosition(v2.position, packInfo);
            
			GraphicsVertexPacked gv0 = { p0, 0, Color4uint8(pn0, v0.seed), mp0, mp1 };
			GraphicsVertexPacked gv1 = { p1, 1, Color4uint8(pn1, v0.seed), mp0, mp1 };
			GraphicsVertexPacked gv2 = { p2, 2, Color4uint8(pn2, v0.seed), mp0, mp1 };

            size_t gi0 = result.vertices.size();
			result.vertices.push_back(gv0);

            size_t gi1 = result.vertices.size();
			result.vertices.push_back(gv1);

            size_t gi2 = result.vertices.size();
			result.vertices.push_back(gv2);
            
			if (iswater[i])
			{
				result.waterIndices.push_back(gi0);
                result.waterIndices.push_back(gi1);
                result.waterIndices.push_back(gi2);
			}
			else
			{
                result.solidIndices.push_back(gi0);
                result.solidIndices.push_back(gi1);
                result.solidIndices.push_back(gi2);

                RBXASSERT((i ^ 1) < triangleCount);

                if (iswater[i ^ 1])
                {
                    result.solidIndices.push_back(gi0);
                    result.solidIndices.push_back(gi2);
                    result.solidIndices.push_back(gi1);
                }
    		}
		}

		return result;
	}

    void generateAdjacency(std::vector<TriangleAdjacency>& result, const BasicMesh& mesh)
    {
        size_t triangleCount = mesh.indices.size() / 3;

        std::vector<unsigned int> triangleCounts(mesh.vertices.size());

        for (size_t i = 0; i < triangleCount; ++i)
        {
            unsigned int i0 = mesh.indices[3 * i + 0];
            unsigned int i1 = mesh.indices[3 * i + 1];
            unsigned int i2 = mesh.indices[3 * i + 2];

            triangleCounts[i0]++;
            triangleCounts[i1]++;
            triangleCounts[i2]++;
        }

        std::vector<unsigned int> triangleOffsets(mesh.vertices.size());
        size_t triangleOffset = 0;

        for (size_t i = 0; i < mesh.vertices.size(); ++i)
        {
            triangleOffsets[i] = triangleOffset;
            triangleOffset += triangleCounts[i];
        }

        std::vector<unsigned int> triangles(triangleOffset);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            unsigned int i0 = mesh.indices[3 * i + 0];
            unsigned int i1 = mesh.indices[3 * i + 1];
            unsigned int i2 = mesh.indices[3 * i + 2];

            // Encode the next vertex index in triangle so that following loop is faster
            triangles[triangleOffsets[i0]++] = (i << 2) | 1;
            triangles[triangleOffsets[i1]++] = (i << 2) | 2;
            triangles[triangleOffsets[i2]++] = (i << 2) | 0;
        }

        result.resize(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            TriangleAdjacency& adj = result[i];

            adj.neighbor[0] = adj.neighbor[1] = adj.neighbor[2] = TriangleAdjacency::None;

            for (size_t e = 0; e < 3; ++e)
            {
                unsigned int i0 = mesh.indices[3 * i + (e == 2 ? 0 : e + 1)];
                unsigned int i1 = mesh.indices[3 * i + e];

                size_t count = triangleCounts[i0];
                size_t offset = triangleOffsets[i0] - count;

                for (size_t j = 0; j < triangleCounts[i0]; ++j)
                {
                    unsigned int trix = triangles[offset + j];
                    unsigned int tri = trix >> 2;

                    if (mesh.indices[3 * tri + (trix & 3)] == i1)
                    {
                        if (adj.neighbor[e] == TriangleAdjacency::None)
                            adj.neighbor[e] = tri;
                        else
                            adj.neighbor[e] = TriangleAdjacency::Multiple;
                    }
                }
            }
        }
    }

    void generateEdgeFlags(std::vector<unsigned char>& result, const BasicMesh& mesh, float cutoff)
    {
        size_t triangleCount = mesh.indices.size() / 3;

        std::vector<Vector3> hardnormals(triangleCount);

		for (size_t i = 0; i < triangleCount; ++i)
		{
			unsigned int i0 = mesh.indices[3 * i + 0];
			unsigned int i1 = mesh.indices[3 * i + 1];
			unsigned int i2 = mesh.indices[3 * i + 2];

			Vector3 a = mesh.vertices[i0].position;
			Vector3 b = mesh.vertices[i1].position;
			Vector3 c = mesh.vertices[i2].position;

			Vector3 vn = cross(b - a, c - a);

			hardnormals[i] = normalize(vn);
		}

        std::vector<TriangleAdjacency> triangleAdj;

        generateAdjacency(triangleAdj, mesh);

        result.resize(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            unsigned char flag = 0;

            const TriangleAdjacency& adj = triangleAdj[i];

            if (adj.neighbor[0] >= 0 && dot(hardnormals[i], hardnormals[adj.neighbor[0]]) > cutoff)
                flag |= 1;

            if (adj.neighbor[1] >= 0 && dot(hardnormals[i], hardnormals[adj.neighbor[1]]) > cutoff)
                flag |= 2;

            if (adj.neighbor[2] >= 0 && dot(hardnormals[i], hardnormals[adj.neighbor[2]]) > cutoff)
                flag |= 4;

            result[i] = flag;
        }
    }

	const TextureBasis& getTextureBasisU()
	{
		return kTextureBasisU;
	}

	const TextureBasis& getTextureBasisV()
	{
		return kTextureBasisV;
	}

} } }
