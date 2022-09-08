#pragma once 
#include "Reflection/Reflection.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "v8world/ContactManagerSpatialHash.h"
#include "util/DoubleEndedVector.h"

#include "voxel/ChunkMap.h"
#include "voxel/Voxelizer.h"
#include "voxel2/GridListener.h"

#include <boost/pool/pool_alloc.hpp>

namespace RBX {
	class PathfindingJob;
	class Path;

	extern const char* const sPathfindingService;
	class PathfindingService
		:public DescribedCreatable<PathfindingService, Instance, sPathfindingService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
		, public ContactManagerSpatialHash::CoarseMovementCallback
		, public Voxel::CellChangeListener
		, public Voxel2::GridListener
	{
	public:
		PathfindingService();
		float emptyCutoff;

		void computePathAsync(Vector3 start, Vector3 finish, float maxDistance, bool isSmooth,
			boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction);

		void computeRawPathAsync(Vector3 start, Vector3 finish, float maxDistance, 
			boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction);

		void computeSmoothPathAsync(Vector3 start, Vector3 finish, float maxDistance, 
			boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction);

		enum SolidState
		{
			EMPTY,
			SOLID,
			OUTSIDE
		};

		int checkPoints(int startPoint, const std::vector<Vector3>& points, unsigned char emptyCutoff);

		void markChunksDirty(const ExtentsInt32& chunkExtents);
        void coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info) override;

		void terrainCellChanged(const Voxel::CellChangeInfo& info) override;
		void onTerrainRegionChanged(const Voxel2::Region& region) override;

		void executeThrottledRequests();

		float getEmptyCutoff() const { return emptyCutoff; };
		void setEmptyCutoff(float value);

		int countOrComputeDirtyChunks(const Vector3int16& startCell, const Vector3int16& finishCell, float maxDistance, bool compute);

		struct PathfindingRequest
		{
			Vector3int16 startCell, finishCell;
			int maxDistance;
			boost::function<void(std::string)> errorFunction;

			virtual void execute(PathfindingService* service) = 0;
			virtual ~PathfindingRequest(){};
		};

		void scheduleRequest(const shared_ptr<PathfindingRequest>& request);

		void smoothPath(std::vector<Vector3>& points, unsigned char emptyCutoff, int smoothWindow);

	protected:
        void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) override;

	private:
		shared_ptr<PathfindingJob> job;
		DoubleEndedVector<shared_ptr<PathfindingRequest> > throttledRequests;
		Voxel::ChunkMap<Voxel::OccupancyChunk> chunkMap;
		Voxel::Voxelizer voxelizer;

		enum SideState
		{
			BLOCKED,
			NOTBLOCKED,
			NOTBLOCKEDABOVE
		};

		struct CreatePathRequest : public PathfindingRequest
		{
			Vector3 start, finish;
			unsigned char emptyCutoff;
			bool isSmooth;

			boost::function<void(shared_ptr<Instance>) > resumeFunction;
			void execute(PathfindingService* service);
		};

		struct Node
		{
			Node(const Vector3int16& pos, const Vector3int16& parent, float totalPath)
				: pos(pos), parent(parent), totalPath(totalPath), closed(false)
			{
			}

			std::multimap<float, 
					boost::unordered_map<
					Vector3int16
					,Node
					,boost::hash<Vector3int16>
					,std::equal_to<Vector3int16>
					,boost::fast_pool_allocator<Vector3int16, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> 
					>::iterator>::iterator itWeight;

			Vector3int16 pos, parent;
			float totalPath;
			bool closed;			
		};

		bool adjustCell(Vector3int16& cellVoxel, Vector3& cellWorld, const Vector3int16& start, unsigned char emptyCutoff);

		bool adjustCellSideways(Vector3int16& cellVoxel, Vector3& cellWorld, const Vector3int16& start, unsigned char emptyCutoff);
		
		typedef boost::unordered_map<
 			Vector3int16
 			,Node
 			,boost::hash<Vector3int16>
 			,std::equal_to<Vector3int16>
 			,boost::fast_pool_allocator<Vector3int16, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> 
 			> NodesByPos;

		typedef std::multimap<
			float 
			,NodesByPos::iterator 
			,std::less<float>
			,boost::fast_pool_allocator<float, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> 
		> OpenNodesByWeight;

		struct PathfindingState
		{
			Vector3int16 start, finish;
			Vector3int16 closestCell;
			float closestDistance;
			int closestCost;

			OpenNodesByWeight openNodesByWeight;
			NodesByPos nodesByPos;
			int maxDistance;

			PathfindingState();

			void addOpenNode(const Node& node);
			Node popTopOpenNode();

			bool checkOpenNodesCounts();
		};

		SolidState isSolid(const Vector3int16& pos, unsigned char emptyCutoff);
		void computePath(CreatePathRequest& request);

		void collectOldChunks();
		unsigned currentFrameNum;

		Voxel::OccupancyChunk* getChunkById(const SpatialRegion::Id& id);
		Voxel::OccupancyChunk* lastChunk;
		SpatialRegion::Id lastChunkId;

	};

	extern const char* const sPath;

	class Path
		:public DescribedNonCreatable<Path, Instance, sPath, Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
		std::vector<Vector3> points;
		weak_ptr<PathfindingService> service;
	public:
		typedef enum {
			Success = 0,
			ClosestNoPath = 1,
			ClosestOutOfRange = 2,
			FailStartNotEmpty = 3,
			FailFinishNotEmpty = 4,
		} PathStatus;

		Path(weak_ptr<PathfindingService> service, PathStatus status);
		void addPoint(const Vector3& point);

		shared_ptr<const Reflection::ValueArray> getPointCoordinates();

		void checkOcclusionAsync(int startPoint, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

		const std::vector<Vector3>& getPoints() { return points; };
		void reverse();

		PathStatus getStatus() const { return status; };
	private:
		struct OcclusionRequest : public PathfindingService::PathfindingRequest
		{
			shared_ptr<Path> path;
			int startPoint;
			boost::function<void(int)> resumeFunction;
			unsigned char emptyCutoff;

			virtual void execute(PathfindingService* service);
		};

		PathStatus status;
	};

};
