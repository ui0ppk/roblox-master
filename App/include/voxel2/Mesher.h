#pragma once

#include "Util/G3DCore.h"
#include "Util/Vector3int32.h"

#include "voxel2/Grid.h"

namespace RBX { namespace Voxel2 {

    class MaterialTable;

    namespace Mesher
    {
        struct Vertex
        {
            Vector3 position;

            unsigned int border: 1;
            unsigned int reserved: 7;
            unsigned int material: 8;
            unsigned int seed: 16;
        };

        struct GraphicsVertex
        {
            Vector3 position;
            Color4uint8 normal; // xyz = normal, w = vertex index (0-2)
            Color4uint8 material[3]; // x = layer index, y = normal segment (0-17), zw = random seed
        };

        struct GraphicsVertexPacked
        {
            Vector3int16 position;
            short id; // vertex index (0-2)
            Color4uint8 normal; // xyz = normal, w = random seed 0
            Color4uint8 material0; // xyz = layer index (0-?), w = random seed 1
            Color4uint8 material1; // xyz = normal segment (0-17), w = random seed 2
        };

        struct Options
        {
            const MaterialTable* materials;
            bool generateWater;
        };

        struct BasicMesh
        {
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;

            static bool isWater(const Vertex& v0, const Vertex& v1, const Vertex& v2)
            {
                return (v0.material == Cell::Material_Water || v1.material == Cell::Material_Water || v2.material == Cell::Material_Water);
            }
        };

        struct GraphicsMesh
        {
            std::vector<GraphicsVertex> vertices;
            std::vector<unsigned int> solidIndices;
            std::vector<unsigned int> waterIndices;
        };

        struct GraphicsMeshPacked
        {
            std::vector<GraphicsVertexPacked> vertices;
            std::vector<unsigned int> solidIndices;
            std::vector<unsigned int> waterIndices;
        };
     
		void prepareTables();

        BasicMesh generateGeometry(const Box& box, const Vector3int32& offset, int lod, const Options& options);
        GraphicsMesh generateGraphicsGeometry(const BasicMesh& mesh, const Options& options);

        GraphicsMeshPacked generateGraphicsGeometryPacked(const BasicMesh& mesh, const Vector4& packInfo, const Options& options);

        struct TriangleAdjacency
        {
            enum
            {
                None = -1,
                Multiple = -2
            };

            int neighbor[3];
        };

        void generateAdjacency(std::vector<TriangleAdjacency>& result, const BasicMesh& mesh);
        void generateEdgeFlags(std::vector<unsigned char>& result, const BasicMesh& mesh, float cutoff);

		typedef const Vector3 TextureBasis[18];

		const TextureBasis& getTextureBasisU();
		const TextureBasis& getTextureBasisV();
    };

} }
