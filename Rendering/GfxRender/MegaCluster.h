#pragma once

#include "RenderNode.h"

#include "GfxBase/GfxPart.h"
#include "reflection/Property.h"
#include "rbx/signal.h"
#include "Util/G3DCore.h"
#include "Util/SpatialRegion.h"

#include "Voxel/AreaCopy.h"
#include "Voxel/CellChangeListener.h"
#include "Voxel/ChunkMap.h"
#include "Voxel/Grid.h"

namespace RBX
{
namespace Graphics
{
	class MegaCluster: public GfxPart, public Voxel::CellChangeListener
	{
	public:
		MegaCluster(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part);
		~MegaCluster();

		virtual void updateEntity(bool assetsUpdated);

		void setIgnoreWaterUpdatesForTesting(bool val);

		
        struct TerrainVertexFFP
		{
			Vector3 pos;
			Vector3 normal;
			Vector2 uv;

            inline void create(const Vector3int16& pos_, const Color4uint8& normal_, const Vector2int16& uv_)
            {
                pos = Vector3(pos_);
                normal = Vector3((normal_.r - 127) / 127, (normal_.g - 127) / 127, (normal_.b - 127) / 127);
                uv = Vector2(uv_) / 2048;
            }

            inline void create(const Vector3int16& pos_, const Color4uint8& normal_, const Vector2int16& uv_, const Vector2int16& uvlow_, 
				const Color4uint8& edgeDistances_, const Color4uint8& tangent_)
			{
                create(pos_, normal_, uv_);
			}
		};

		static void	  generateAndReturnSolidGeometry(Voxel::Grid* voxelGrid, const SpatialRegion::Id& pos, std::vector<TerrainVertexFFP>* verticesOut);
		static void	  generateAndReturnWaterGeometry(Voxel::Grid* voxelGrid, const SpatialRegion::Id& pos, std::vector<TerrainVertexFFP>* verticesOut);
      
        static const std::string kTerrainTexClose;
        static const std::string kTerrainTexFar;
        static const std::string kTerrainTexNormals;
        static const std::string kTerrainTexSpecular;

	private:
		// For rendering, we want a copy of data with fringe.
		// * X buffer needs to be even on both sides for half byte materials
        // * XZ buffer needs to have 2 on both sides for water queries
		// * Y buffer needs to be 2 on the top side for water queries
        // * Y buffer needs to be 1 on the bottom side for outlines
		typedef Voxel::AreaCopy<
			Voxel::kXZ_CHUNK_SIZE + 4,
			Voxel::kY_CHUNK_SIZE + 3,
			Voxel::kXZ_CHUNK_SIZE + 4> RenderArea;

		// Cell location to add to the smallest cell in a SpatialRegion to get
		// the min lookup coordinate used when copying data into the AreaCopy.
		static const Vector3int16 kMinCellOffset;

        static const unsigned kMaxIndexBufferSize32Bit = 65536;
        static const unsigned kMaxIndexBufferSize16Bit = 16384;

        struct TerrainVertex
        {
            Color4uint8 pos;
            Color4uint8 normal;
            Vector2int16 uv[2];
            Color4uint8 edgeDistances;
            Color4uint8 tangent;

            inline void create(const Vector3int16& pos_, const Color4uint8& normal_, const Vector2int16& uv_, const Vector2int16& uvlow_,
                const Color4uint8& edgeDistances_, const Color4uint8& tangent_)
            {
                pos = Color4uint8(pos_.x, pos_.y, pos_.z, 1);
                normal = normal_;
                uv[0] = uv_;
                uv[1] = uvlow_;
                edgeDistances = edgeDistances_;
                tangent = tangent_;
            }
        };

        struct WaterVertex
        {
            Color4uint8 pos;
            Color4uint8 normal;

            inline void create(const Vector3int16& pos_, const Color4uint8& normal_, const Vector2int16& uv_)
            {
                pos = Color4uint8(pos_.x, pos_.y, pos_.z, 1);
                normal = normal_;
            }
        };

		struct ChunkData
		{
			// estimate conversion factor for converting terrain quads into
			// equivalent parts for purposes of estimate FRM culling
			static const int kApproximateQuadsPerPart = 12;

			unsigned int solidQuads;
			unsigned int waterQuads;
			
			bool solidDirty;
			bool waterDirty;
		
			shared_ptr<RenderNode> node;
			RenderEntity* solidEntity;
			RenderEntity* waterEntity;
			
			ChunkData(): solidEntity(NULL), waterEntity(NULL), solidQuads(0), waterQuads(0), solidDirty(false), waterDirty(false)
			{
			}
		};
		
		virtual void unbind();
		virtual void invalidateEntity();
		
		// CellChangeListener override
		virtual void terrainCellChanged(const Voxel::CellChangeInfo& info);

		// GfxBinding override
		virtual void updateChunk(const SpatialRegion::Id& pos, bool isWaterChunk);

		static void downloadChunkGeometry(Voxel::Grid* voxelGrid, const SpatialRegion::Id& pos, bool solidUpdate, bool waterUpdate);

		void markDirty(const SpatialRegion::Id& pos, bool solidDirty, bool waterDirty);

		void updateChunkGeometry(const SpatialRegion::Id& pos, bool solidUpdate, bool waterUpdate);

        RenderNode* updateChunkNode(const SpatialRegion::Id& pos);
		RenderEntity* createSolidGeometry(const SpatialRegion::Id& pos, unsigned int* outQuads);
		RenderEntity* createWaterGeometry(const SpatialRegion::Id& pos, unsigned int* outQuads);
		
		RenderEntity* createGeometry(RenderNode* node, const shared_ptr<VertexBuffer>& vbuf, const shared_ptr<IndexBuffer>& ibuf, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, bool isWater);
		
		const shared_ptr<VertexLayout>& getVertexLayout(bool isWater);
        const shared_ptr<IndexBuffer>& getSharedIB();
		
        const shared_ptr<Material>& getSolidMaterial();

		VisualEngine* visualEngine;

        bool useShaders;
		bool ignoreWaterUpdatesForTesting;
		
		Voxel::Grid* storage;
		RenderArea renderArea;

		Voxel::ChunkMap<ChunkData> chunks;

        shared_ptr<IndexBuffer> sharedIB;
        shared_ptr<VertexLayout> vertexLayouts[2];

        shared_ptr<Material> solidMaterial;
	};

}
}
