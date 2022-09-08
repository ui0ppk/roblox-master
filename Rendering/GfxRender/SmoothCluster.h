#pragma once

#include "RenderNode.h"

#include "GfxBase/GfxPart.h"
#include "reflection/Property.h"
#include "rbx/signal.h"
#include "Util/G3DCore.h"
#include "Util/SpatialRegion.h"

#include "Voxel/ChunkMap.h"

#include "voxel2/GridListener.h"

namespace RBX { namespace Voxel2 { class MaterialTable; class Grid; class Region; } }

namespace RBX
{
namespace Graphics
{
	class SmoothClusterBase: public GfxPart, public Voxel2::GridListener
	{
	public:
		SmoothClusterBase(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part);
		~SmoothClusterBase();

        void reloadMaterialTable();

	protected:
		virtual void unbind();
		virtual void invalidateEntity();
		
		std::pair<RenderEntity*, RenderEntity*> uploadGeometry(RenderNode* node, const Vector4& unpackInfo,
            size_t vertexSize, const void* vertices, size_t vertexCount,
            const std::vector<unsigned int>& solidIndices, const std::vector<unsigned int>& waterIndices);

		const shared_ptr<VertexLayout>& getVertexLayout();
		
        const shared_ptr<Material>& getSolidMaterial();

        void updateMaterialConstants();

		VisualEngine* visualEngine;
		
		Voxel2::Grid* grid;

        shared_ptr<VertexLayout> vertexLayout;

        shared_ptr<Material> solidMaterial;

		Voxel2::MaterialTable* materialTable;

        std::vector<float> materialConstants;
	};
	
	class SmoothClusterChunked: public SmoothClusterBase
	{
	public:
		SmoothClusterChunked(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part);
		~SmoothClusterChunked();

		virtual void updateEntity(bool assetsUpdated);

	private:
		static const Vector3int32 kChunkSizeLog2;
		static const Vector3int32 kChunkSize;
		static const int kChunkBorder;

		struct ChunkData
		{
			// estimate conversion factor for converting terrain quads into
			// equivalent parts for purposes of estimate FRM culling
			static const int kApproximateQuadsPerPart = 12;

			bool dirty;
		
			shared_ptr<RenderNode> node;
			RenderEntity* solidEntity;
			RenderEntity* waterEntity;
			
			ChunkData(): solidEntity(NULL), waterEntity(NULL), dirty(false)
			{
			}
		};
		
		// GridListener override
        virtual void onTerrainRegionChanged(const Voxel2::Region& region);

		// GfxBinding override
		virtual void updateChunk(const SpatialRegion::Id& pos, bool isWaterChunk);

		void markDirty(const SpatialRegion::Id& pos);

        RenderNode* updateChunkNode(const SpatialRegion::Id& pos);
		std::pair<RenderEntity*, RenderEntity*> createChunkGeometry(const SpatialRegion::Id& pos, unsigned int* outQuads);
		
		Voxel::ChunkMap<ChunkData> chunks;
	};

	class SmoothClusterLOD: public SmoothClusterBase
	{
	public:
		SmoothClusterLOD(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part);
		~SmoothClusterLOD();

		virtual void updateEntity(bool assetsUpdated);

	private:
		static const int kChunkSizeLog2;
		static const int kChunkBorder;
		static const int kChunkLevels;

		enum ChunkFlags
		{
			Flag_NeedsUpdate = 1 << 0,
			Flag_NeedsSplit = 1 << 1,
			Flag_NeedsMerge = 1 << 2,
		};

		typedef std::pair<Vector3int32, unsigned int> ChunkId;

		struct Chunk
		{
			// estimate conversion factor for converting terrain quads into
			// equivalent parts for purposes of estimate FRM culling
			static const int kApproximateQuadsPerPart = 12;

			ChunkId id;
			unsigned int flags;
		
			shared_ptr<RenderNode> node;
			RenderEntity* solidEntity;
			RenderEntity* waterEntity;
			
			Chunk(): flags(0), solidEntity(NULL), waterEntity(NULL)
			{
			}

			Voxel2::Region getRegion() const;
		};
		
		static ChunkId getParentChunk(const ChunkId& id);
		static ChunkId getChildChunk(const ChunkId& id, int x, int y, int z);
		static std::pair<Vector3, float> getChunkBounds(const ChunkId& id);

		// GridListener override
        virtual void onTerrainRegionChanged(const Voxel2::Region& region);

		void markDirty(const ChunkId& id);

		Chunk& getChunk(const ChunkId& id);
		Chunk* findChunk(const ChunkId& id);
		void removeChunk(const ChunkId& id);

		Chunk* findDirtyChunk();

        void updateChunk(Chunk& chunk);
        RenderNode* updateChunkNode(Chunk& chunk);
		std::pair<RenderEntity*, RenderEntity*> createChunkGeometry(Chunk& chunk, unsigned int* outQuads);

		boost::unordered_map<ChunkId, Chunk> chunks;
	};

}
}
