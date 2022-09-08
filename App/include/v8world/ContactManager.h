/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "G3D/Array.h"
#include "Util/ConcurrencyValidator.h"
#include "Util/G3DCore.h"
#include "Util/HitTestFilter.h"
#include "Util/SpatialRegion.h"
#include "Util/SystemAddress.h"
#include "Voxel/CellChangeListener.h"
#include "Util/Extents.h"
#include "Util/Region3.h"
#include "Voxel2/GridListener.h"
#include "v8world/TerrainPartition.h"
#include "v8tree/Instance.h"
#include "util/PartMaterial.h"

#include "rbx/DenseHash.h"

#include <set>
#include <boost/unordered_set.hpp>
#include <boost/scoped_ptr.hpp>

namespace RBX {
namespace Graphics {
class CullableSceneNode;
} }

namespace RBX {

	namespace Profiling
	{
		class CodeProfiler;
	}

	class Primitive;
	class Contact;
	class Joint;
	class World;
	class ContactManagerSpatialHash;
	class MegaClusterInstance;

	namespace Voxel { class Grid; }
	namespace Voxel2 { class Grid; }

	class ContactManager:
		public Voxel::CellChangeListener,
		public Voxel2::GridListener
	{
		ConcurrencyValidator		concurrencyValidator;
		ContactManagerSpatialHash*	spatialHash;
        Primitive*              myMegaClusterPrim;
		World*					world;

		typedef boost::unordered_set<SpatialRegion::Id, SpatialRegion::Id::boost_compatible_hash_value> UpdatedTerrainRegionsSet;
		UpdatedTerrainRegionsSet updatedTerrainRegions;

		typedef boost::unordered_set<Vector3int32> UpdatedTerrainChunksSet;
		UpdatedTerrainChunksSet updatedTerrainChunks;

		std::vector<TerrainPartitionSmooth::ChunkResult> tempChunks;
		std::vector<Primitive*> tempPrimitives;

        static Vector3 dummySurfaceNormal;
        static PartMaterial dummySurfaceMaterial;

		Contact* createContact(Primitive* p0, Primitive* p1);

		// TODO: All private and public *Hit() methods have too many arguments!
		// Refactor to use struct to bundle arguments
		Primitive* getSlowHit(	const G3D::Array<Primitive*>& primitives,
								const RbxRay& unitRay,					
								const G3D::Array<const Primitive*>& ignorePrim,
								const HitTestFilter* filter,
								Vector3& hitPointWorld,
                                Vector3& surfaceNormal,
                                PartMaterial& surfaceMaterial,
								float maxDistance,
								bool& stopped) const;

		Primitive* getFastHit(	const RbxRay& worldRay,						// implies distance as well
								const G3D::Array<const Primitive*>& ignorePrim,	// set to NULL to not use	
								const HitTestFilter* filter,				// set to NULL to not use
								Vector3& hitPointWorld,
								bool& stopped,
								bool terrainCellsAreCubes,
                                bool ignoreWater,
                                Vector3& surfaceNormal,
                                PartMaterial& surfaceMaterial) const;

		/*override*/ virtual void terrainCellChanged(const Voxel::CellChangeInfo& info);
		/*override*/ virtual void onTerrainRegionChanged(const Voxel2::Region& region);

		bool checkMegaClusterWaterContact(Primitive* p, const Vector3int16& extentStart,
				const Vector3int16& extentEnd, const Vector3int16& extentSize);
		bool checkMegaClusterSmallTerrainContact(Primitive* otherPrim, const Vector3int16& extentStart,
				const Vector3int16& extentEnd, const Vector3int16& extentSize,
				bool cellChanged);
		bool checkMegaClusterBigTerrainContact(Primitive* p);
		void checkMegaClusterContact(Primitive* p, bool checkTerrain, bool checkWater, bool cellChanged);

		void applyDeferredMegaClusterChanges();

		bool checkSmoothClusterSolidContact(Primitive* p);
		bool checkSmoothClusterWaterContact(Primitive* p);
		void checkSmoothClusterContact(Primitive* p, bool cellChanged);

		void applyDeferredSmoothClusterChanges();

		bool setUpbulletCollisionShapes(Primitive* p0, Primitive* p1);

		Voxel::Grid* getVoxelGrid();
		Voxel2::Grid* getSmoothGrid();

	public:

		ContactManager(World* world);
		~ContactManager();

		/////////////////////////////////////////////
		// General Inquiry
		//
		ContactManagerSpatialHash* getSpatialHash() {return spatialHash;}

		// Returns NULL on no hit
		Primitive* getHit(	const RbxRay& worldRay, 
							const std::vector<const Primitive*>* ignorePrim,		// set to NULL to not use	
							const HitTestFilter* filter,							// set to NULL to not use
                            Vector3& hitPointWorld,
							bool terrainCellsAreCubes = false,
                            bool ignoreWater = false,
                            Vector3& surfaceNormal = dummySurfaceNormal,
                            PartMaterial& surfaceMaterial = dummySurfaceMaterial) const;

		void getPrimitivesTouchingExtents(
							const Extents& extents, 
							const Primitive* ignore,
							int maxCount,
							G3D::Array<Primitive*>& found);

		void getPrimitivesTouchingExtents(
							const Extents& extents,
							const boost::unordered_set<const Primitive*>& ignorePrimitives,
							int maxCount,
							G3D::Array<Primitive*>& found);

		void getPrimitivesOverlapping(const Extents& extents, DenseHashSet<Primitive*>& result);

		bool intersectingGroundPlane(const G3D::Array<Primitive*>& check, float yHeight);
		bool intersectingOthers(Primitive* check, float overlapIgnored);
		bool intersectingOthers(const G3D::Array<Primitive*>& check, float overlapIgnored);
		bool intersectingOthers(Primitive* check, const std::set<Primitive*>& checkSet, float overlapIgnored);
		bool intersectingMySimulation(Primitive* check, RBX::SystemAddress myLocalAddress, float overlapIgnored);

		shared_ptr<const Instances> getPartCollisions(Primitive* check);

		/////////////////////////////////////////////
		// From the collision engine
		//
		void onNewPair(Primitive* p0, Primitive* p1);	
		void onNewPair(RBX::Graphics::CullableSceneNode* p0, RBX::Graphics::CullableSceneNode* p1) { RBXASSERT(0); } 

		void checkTerrainContact(Primitive* p);
        void checkTerrainContact(RBX::Graphics::CullableSceneNode* p0) {}

        bool primitiveIsExcludedFromSpatialHash(Primitive* p);
        bool primitiveIsExcludedFromSpatialHash(RBX::Graphics::CullableSceneNode* p0) {return false;}
		
		void releasePair(Primitive* p0, Primitive* p1);
		void releasePair(RBX::Graphics::CullableSceneNode* p0, RBX::Graphics::CullableSceneNode* p1) { RBXASSERT(0); }

		/////////////////////////////////////////////
		// From the world
		//
		void onPrimitiveAdded(Primitive* p);
		void onPrimitiveRemoved(Primitive* p);
		void onPrimitiveExtentsChanged(Primitive* p);
		void onPrimitiveGeometryChanged(Primitive* p);
		void onPrimitiveAssembled(Primitive* p);

		void onAssemblyMovedFromStep(Assembly& a);

		void applyDeferredTerrainChanges();

		void fastClear();
		void doStats();			// spit out hash stats

		///////////////////////////////////////////
		// Profiler
		boost::scoped_ptr<Profiling::CodeProfiler> profilingBroadphase;

		/////////////////////////////////////////////
		// LEGACY
		Primitive* getHitLegacy(	const RbxRay& originDirection, 
									const Primitive* ignorePrim,			// set to NULL to not use	
									const HitTestFilter* filter,	// set to NULL to not use
									Vector3& hitPointWorld,
									float& distanceToHit,
									const float& maxSearchDepth,
									bool ignoreWater) const;			

        Primitive* getMegaClusterPrimitive( void ) const { return myMegaClusterPrim; }

		bool terrainCellsInRegion3(Region3 region) const;
		Vector3 findUpNearestLocationWithSpaceNeeded(const float maxSearchDepth, const Vector3 &startCenter, const Vector3 &spaceNeededToCorner);
	};

} // namespace
