#include "stdafx.h"

#include "V8DataModel/DataStoreService.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/PartCookie.h"
#include "v8datamodel/PathfindingService.h"
#include "Util/RobloxGoogleAnalytics.h"

#include "Network/Players.h"

#include "v8world/World.h"
#include "v8world/ContactManagerSpatialHash.h"

#include "Voxel/Grid.Chunk.h"
#include "voxel2/Grid.h"

#include "reflection/EnumConverter.h"

#include "rbx/Profiler.h"

LOGVARIABLE(PathfindingDetail, 0);
LOGVARIABLE(PathfindingPerf, 0);

DYNAMIC_FASTINTVARIABLE(PathfindingMaxDistance, 512);

DYNAMIC_FASTINTVARIABLE(PathfindingJobRunsPerSecond, 10);
DYNAMIC_FASTINTVARIABLE(PathfindingChunksPerInvokation, 10);
DYNAMIC_FASTINTVARIABLE(PathfindingAgeToCollectChunks, 1000);
DYNAMIC_FASTINTVARIABLE(PathfindingCollectPeriod, 100);

DYNAMIC_FASTINTVARIABLE(PathfindingDefaultBucketNum, 2048);
DYNAMIC_FASTINTVARIABLE(PathfindingVerticalChunkClamp, 1);

DYNAMIC_FASTINTVARIABLE(PathfindingSmoothIterations, 5);
DYNAMIC_FASTINTVARIABLE(PathfindingAverageWindow, 7);

RBX_REGISTER_ENUM(Path::PathStatus);

namespace RBX {
	const char* const sPathfindingService = "PathfindingService";

	const float PATHFINDING_FALL_COST = 1.0f;
	const float PATHFINDING_WALK_COST = 1.0f;
	const float PATHFINDING_WALK_DIAGLONAL_COST = sqrt(2);

	float kMaxWorldCoordinate = (float)((std::numeric_limits<G3D::int16>::max()-2*kVoxelChunkSizeXZ)*Voxel::kCELL_SIZE);

    REFLECTION_BEGIN();
	static Reflection::PropDescriptor<PathfindingService, float> prop_EmptyCutoff
		("EmptyCutoff", category_Data, &PathfindingService::getEmptyCutoff, &PathfindingService::setEmptyCutoff);
	
	static Reflection::BoundYieldFuncDesc<PathfindingService, shared_ptr<Instance>(Vector3, Vector3, float)> 
		func_computeRawPathAsync(&PathfindingService::computeRawPathAsync, "ComputeRawPathAsync", "start", "finish", "maxDistance", Security::None);

	static Reflection::BoundYieldFuncDesc<PathfindingService, shared_ptr<Instance>(Vector3, Vector3, float)> 
		func_computeSmoothPathAsync(&PathfindingService::computeSmoothPathAsync, "ComputeSmoothPathAsync", "start", "finish", "maxDistance", Security::None);

	static Reflection::BoundYieldFuncDesc<Path, int(int)> 
		func_checkOcclusionAsync(&Path::checkOcclusionAsync, "CheckOcclusionAsync", "start", Security::None);

	static Reflection::BoundFuncDesc<Path, shared_ptr<const Reflection::ValueArray>(void) > prop_Points
		(&Path::getPointCoordinates, "GetPointCoordinates", Security::None);
    REFLECTION_END();

	namespace Reflection
	{
		template<> Reflection::EnumDesc<Path::PathStatus>::EnumDesc()
			:RBX::Reflection::EnumDescriptor("PathStatus")
		{
			addPair(Path::Success, "Success");
			addPair(Path::ClosestNoPath, "ClosestNoPath");
			addPair(Path::ClosestOutOfRange, "ClosestOutOfRange");
			addPair(Path::FailStartNotEmpty, "FailStartNotEmpty");
			addPair(Path::FailFinishNotEmpty, "FailFinishNotEmpty");
		}
	}

    REFLECTION_BEGIN();
	static Reflection::EnumPropDescriptor<Path, Path::PathStatus> prop_PathStatus
		("Status", category_Data, &Path::getStatus, NULL);
    REFLECTION_END();

	class PathfindingJob : public DataModelJob
	{
	public:
		shared_ptr<PathfindingService> pathfindingService;
		double desiredHz;

		PathfindingJob(PathfindingService* owner)
			: DataModelJob("PathfindingJob", DataModelJob::Write, false, shared_from(DataModel::get(owner)), Time::Interval(0.01)),
			pathfindingService(shared_from(owner))
		{
			desiredHz = DFInt::PathfindingJobRunsPerSecond;

			cyclicExecutive = true;
		}

		/*override*/ Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, desiredHz);
		}
		/*override*/ Job::Error error(const Stats& stats)
		{
			return computeStandardErrorCyclicExecutiveSleeping(stats, desiredHz);
		}
		/*override*/ TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
		{
			pathfindingService->executeThrottledRequests();
			return TaskScheduler::Stepped;
		}
	};

	Vector3int16 getDiagonalOffsetByIndex(int dir)
	{
		switch (dir)
		{
		case 0: return Vector3int16(1,0,-1);
		case 1: return Vector3int16(-1,0,-1);
		case 2: return Vector3int16(-1,0,1);
		case 3: return Vector3int16(1,0,1);

		default:
			RBXASSERT(false);
			return Vector3int16(0,0,0);
		}
	}

	Vector3int16 getOffsetByIndex(int dir)
	{
		switch (dir)
		{
		case 0: return Vector3int16(1,0,0);
		case 1: return Vector3int16(0,0,-1);
		case 2: return Vector3int16(-1,0,0);
		case 3: return Vector3int16(0,0,1);

		default:
			RBXASSERT(false);
			return Vector3int16(0,0,0);
		}
	}
	
	PathfindingService::PathfindingService() 
		: lastChunkId(SpatialRegion::Id(std::numeric_limits<G3D::int16>::max(), std::numeric_limits<G3D::int16>::max(), std::numeric_limits<G3D::int16>::max())),
		lastChunk(NULL), 
		currentFrameNum(0),
		voxelizer(true /* collisionTransparency */)
	{
		emptyCutoff = 40/255.0f;
	}

	void PathfindingService::setEmptyCutoff(float value)
	{
		if(value < 0.0f || value > 1.0f)
		{
			throw std::runtime_error("Value is outside expected range");
		}

		emptyCutoff = value;
	}

	inline float diffDistance(const Vector3int16& start, const Vector3int16& finish)
	{
		Vector3int16 delta = finish - start;
		return sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
	}
	
	void PathfindingService::PathfindingState::addOpenNode(const Node& node)
	{
		FASTLOG3(FLog::PathfindingDetail, "Adding open node: %i %i %i", node.pos.x, node.pos.y, node.pos.z);

		NodesByPos::iterator it = nodesByPos.find(node.pos);
		if(it != nodesByPos.end())
		{
			FASTLOG1F(FLog::PathfindingDetail, "Open node already exist with cost %f", it->second.totalPath);

			if(node.totalPath < it->second.totalPath)
			{
				FASTLOG1F(FLog::PathfindingDetail, "New node is better, opening node with distance %f", it->second.itWeight->first);
				if(!it->second.closed)
				{
					openNodesByWeight.erase(it->second.itWeight);
					it->second.itWeight = openNodesByWeight.end();
				}

				it->second.closed = false;
				it->second.parent = node.parent;
				it->second.totalPath = node.totalPath;

			} 
			else
			{
				return;
  			}
		}
		else
		{
			std::pair<NodesByPos::iterator, bool> result = nodesByPos.insert(NodesByPos::value_type(node.pos, node));
			RBXASSERT(result.second);
			it = result.first;
		}

		float rawDistance = diffDistance(finish, node.pos);
		float distance = rawDistance  + node.totalPath;
		
		FASTLOG3(FLog::PathfindingDetail, "Creating open node: %i %i %i", node.pos.x, node.pos.y, node.pos.z);
		FASTLOG2F(FLog::PathfindingDetail, "Distance: %f, raw distance: %f", distance, rawDistance);
		FASTLOG3(FLog::PathfindingDetail, "New open node parent: %i %i %i", node.parent.x, node.parent.y, node.parent.z);

		OpenNodesByWeight::iterator itWeight = openNodesByWeight.insert(std::make_pair(distance, it));
		it->second.itWeight = itWeight;

		RBXASSERT_SLOW(checkOpenNodesCounts());

		if(rawDistance < closestDistance)
		{
			FASTLOG1F(FLog::PathfindingDetail, "Distance is less than previous closest distance %f", closestDistance);
			closestDistance = rawDistance;
			closestCell = node.pos;
			closestCost = node.totalPath;
		}
	}

	Vector3 getPoint(const std::vector<Vector3>& points, int index, int size)
	{
		if (index < 0) {
			RBXASSERT((-index) < size);
			// Inverse around point zero, so result = point_0 + (point_0 - point_i) = 2*point_0 - point_i
			return 2*points[0] - points[-index];
		} else if (index >= size) 
		{
			RBXASSERT((2*(size-1) - index) >= 0);
			// Inverse around last point (size-1), index is inversed the same way
			return 2*points[size-1] - points[2*(size-1) - index];
		}

		return points[index];
	}
	
	void PathfindingService::smoothPath(std::vector<Vector3>& points, unsigned char emptyCutoff, int smoothWindow)
	{
		int size = points.size();
		
		if(smoothWindow >= size)
		{
			// Find largest odd number less or equal to size
			smoothWindow = (size - 1) | 1;
		}

		if(smoothWindow == 1)
			return;

		float aveCoeff = 1.0f / smoothWindow;

		int halfAve = smoothWindow / 2;

		Vector3 ave = Vector3(0,0,0);
		for(int i = -halfAve; i <= halfAve; i++)
		{
			ave += getPoint(points, i, size) * aveCoeff;
		}

		std::vector<Vector3> newPoints(points.size());
		newPoints[0] = points[0];
		newPoints[size-1] = points[size-1];

		for(int i = 1; i < size-1; i++)
		{
			newPoints[i] = points[i];
			ave += getPoint(points, i+halfAve, size) * aveCoeff;
			ave -= getPoint(points, i-halfAve-1, size) * aveCoeff;

			ave.y = points[i].y;

			FASTLOG1(FLog::PathfindingDetail, "Processing point %i", i);
			FASTLOG3F(FLog::PathfindingDetail, "Average: %f %f %f", ave.x, ave.y, ave.z);

			Vector3int16 floor = Vector3int16(fastFloorInt(ave.x-0.5f), ave.y, fastFloorInt(ave.z-0.5f));
			Vector3int16 ceil = Vector3int16(fastCeilInt(ave.x-0.5f), ave.y, fastCeilInt(ave.z-0.5f));

			FASTLOG4(FLog::PathfindingDetail, "Floor: %i %i, ceil: %i %i", floor.x, floor.z, ceil.x, ceil.z);

			if(floor == ceil)
			{
				if(isSolid(floor, emptyCutoff) != EMPTY)
				{
					FASTLOG3(FLog::PathfindingDetail, "Floor is same as ceil, but %i %i %i is not empty", floor.x, floor.y, floor.z);
					continue;
				}
			} 
			else
			{
				if(isSolid(Vector3int16(ceil.x, ceil.y, ceil.z), emptyCutoff) != EMPTY)
				{
					FASTLOG3(FLog::PathfindingDetail, "Skipping, %i %i %i is not empty", ceil.x, ceil.y, ceil.z);
					continue;
				}
				if(isSolid(Vector3int16(floor.x, ceil.y, ceil.z), emptyCutoff) != EMPTY)
				{
					FASTLOG3(FLog::PathfindingDetail, "Skipping, %i %i %i is not empty", floor.x, ceil.y, ceil.z);
					continue;
				}
				if(isSolid(Vector3int16(floor.x, ceil.y, floor.z), emptyCutoff) != EMPTY)
				{
					FASTLOG3(FLog::PathfindingDetail, "Skipping, %i %i %i is not empty", floor.x, ceil.y, floor.z);
					continue;
				}
				if(isSolid(Vector3int16(ceil.x, ceil.y, floor.z), emptyCutoff) != EMPTY)
				{
					FASTLOG3(FLog::PathfindingDetail, "Skipping, %i %i %i is not empty", ceil.x, ceil.y, floor.z);
					continue;
				}
			}

			Vector3int16 cellBelow = Vector3int16(fastFloorInt(ave.x), fastFloorInt(ave.y-1), fastFloorInt(ave.z));

			if(isSolid(cellBelow, emptyCutoff) != SOLID)
			{
				FASTLOG3(FLog::PathfindingDetail, "Skipping, nottom %i %i %i is not solid", cellBelow.x, cellBelow.y, cellBelow.z);
				continue;
			}

			newPoints[i] = ave;
		}

		swap(points, newPoints);
	}

	bool PathfindingService::PathfindingState::checkOpenNodesCounts()
	{
		int countOpen = 0;
		for(NodesByPos::iterator it = nodesByPos.begin(); it != nodesByPos.end(); ++it) 
		{
			if(!it->second.closed)
				countOpen++;
		}

		return countOpen == openNodesByWeight.size();
	}

	PathfindingService::Node PathfindingService::PathfindingState::popTopOpenNode()
	{
		RBXASSERT(openNodesByWeight.size() != 0);

		OpenNodesByWeight::iterator top = openNodesByWeight.begin();
		Node* result = &(top->second->second);

		RBXASSERT(result->closed == false);

		FASTLOG1F(FLog::PathfindingDetail, "---- Popping top open node, weight: %f", top->first);
		FASTLOG3(FLog::PathfindingDetail,  "Popping top open node, coordinate: %i %i %i", result->pos.x, result->pos.y, result->pos.z);

		openNodesByWeight.erase(top);
		result->itWeight = openNodesByWeight.end();

		result->closed = true;

		RBXASSERT_SLOW(checkOpenNodesCounts());

		return *result;
	}

	bool PathfindingService::adjustCellSideways(Vector3int16& cellVoxel, Vector3& cellWorld, const Vector3int16& start, unsigned char emptyCutoff)
	{
		int bestDotResult = 0;
		int bestDir = -1;

		Vector3int16 direction = start - cellVoxel;

		FASTLOG3(FLog::PathfindingDetail, "Adjusting cell sideways, cell: %i %i %i", cellVoxel.x, cellVoxel.y, cellVoxel.z);
		FASTLOG3(FLog::PathfindingDetail, "Direction to start: %i %i %i", direction.x, direction.y, direction.z);

		for(int dir = 0; dir < 4; dir++)
		{
			Vector3int16 offset = getOffsetByIndex(dir);
			Vector3int16 side = cellVoxel + offset;
			if(isSolid(side, emptyCutoff) == EMPTY)
			{
				FASTLOG4(FLog::PathfindingDetail, "Empty for dir %i, side cell: %i %i %i", dir, side.x, side.y, side.z);

				int dotResult = direction.dot(offset);

				if(bestDir == -1 || dotResult > bestDotResult)
				{
					FASTLOG1(FLog::PathfindingDetail, "Dot is better than old result: %i, storing as best", dotResult);
					bestDir = dir;
					bestDotResult = dotResult;
				}
			}
		}

		if(bestDir == -1)
			return false;
		
		cellVoxel = cellVoxel + getOffsetByIndex(bestDir);
		cellWorld = cellWorld + Vector3(getOffsetByIndex(bestDir))*Voxel::kCELL_SIZE;

		FASTLOG3(FLog::PathfindingDetail, "Final cell: %i %i %i", cellVoxel.x, cellVoxel.y, cellVoxel.z);
		return true;
	}

	bool PathfindingService::adjustCell(Vector3int16& cellVoxel, Vector3& cellWorld, const Vector3int16& start, unsigned char emptyCutoff)
	{
		if(isSolid(cellVoxel, emptyCutoff) == EMPTY)
			return true;

		FASTLOG(FLog::PathfindingDetail, "Trying sides");
		if(adjustCellSideways(cellVoxel, cellWorld, start, emptyCutoff))
			return true;

		FASTLOG(FLog::PathfindingDetail, "Trying up");
		Vector3int16 cellVoxelUp = cellVoxel + Vector3int16(0,1,0);
		Vector3 cellWorldUp = cellWorld + Vector3(0,Voxel::kCELL_SIZE,0);
		if(isSolid(cellVoxelUp, emptyCutoff) == EMPTY)
		{
			cellVoxel = cellVoxelUp;
			cellWorld = cellWorldUp;
			return true;
		}

		FASTLOG(FLog::PathfindingDetail, "Trying up & sides");
		if(adjustCellSideways(cellVoxelUp, cellWorldUp, start, emptyCutoff))
		{
			cellVoxel = cellVoxelUp;
			cellWorld = cellWorldUp;
			return true;
		}

		return false;
	}

	void PathfindingService::collectOldChunks()
	{
		FASTLOG(FLog::PathfindingPerf, "Starting collecting old chunks");

		std::vector<SpatialRegion::Id> chunkIds = chunkMap.getChunks();

		FASTLOG1(FLog::PathfindingDetail, "Collecting chunks, size: %u", chunkIds.size());

		for(std::vector<SpatialRegion::Id>::iterator it = chunkIds.begin(); it != chunkIds.end(); ++it)
		{
			Voxel::OccupancyChunk* chunk = chunkMap.find(*it);
			if((chunk->age + DFInt::PathfindingAgeToCollectChunks) < currentFrameNum)
			{
				if(*it == lastChunkId)
				{
					lastChunkId = SpatialRegion::Id(std::numeric_limits<G3D::int16>::max(), std::numeric_limits<G3D::int16>::max(), std::numeric_limits<G3D::int16>::max());
					lastChunk = NULL;
				}

				FASTLOG3(FLog::PathfindingDetail, "Erasing chunk: %i %i %i", it->value().x, it->value().y, it->value().z);
				chunkMap.erase(*it);
			}
		}

		FASTLOG(FLog::PathfindingPerf, "Finish collecting chunks");
	}

	int manhattanDistance(const Vector3int16& start, const Vector3int16& finish)
	{
		Vector3int16 diff = finish - start;

		return abs(diff.x) + abs(diff.y) + abs(diff.z);
	}

	
	int PathfindingService::countOrComputeDirtyChunks(const Vector3int16& startCell, const Vector3int16& finishCell, float maxDistance, bool compute)
	{
        RBXPROFILER_SCOPE("Voxel", "countOrComputeDirtyChunks");
        
		int minPathDistance = manhattanDistance(startCell, finishCell);

		int offset = std::max(0,(int)ceil((maxDistance - minPathDistance*Voxel::kCELL_SIZE) / (2*Voxel::kCELL_SIZE * Voxel::kXZ_CHUNK_SIZE)));

		int offsetY = std::max(offset, DFInt::PathfindingVerticalChunkClamp);

		DataModel* dataModel = DataModel::get(this);
		ContactManager* contactManager = dataModel->getWorkspace()->getWorld()->getContactManager();
		MegaClusterInstance* terrain = boost::polymorphic_downcast<MegaClusterInstance*>(dataModel->getWorkspace()->getTerrain());

		Vector3int16 startId = SpatialRegion::regionContainingVoxel(startCell).value(); 
		Vector3int16 finishId = SpatialRegion::regionContainingVoxel(finishCell).value();

		Vector3int16 minId = startId.min(finishId) + Vector3int16(-offset,-offsetY,-offset);
		Vector3int16 maxId = startId.max(finishId) + Vector3int16(offset,offsetY,offset);

		FASTLOG3(FLog::PathfindingDetail, "Min voxel chunk: %i %i %i", minId.x, minId.y, minId.z);
		FASTLOG3(FLog::PathfindingDetail, "Max voxel chunk: %i %i %i", maxId.x, maxId.y, maxId.z);

		int count = 0;

		// Voxelize
		for(int x = minId.x; x <= maxId.x; x++)
			for(int y = minId.y; y <= maxId.y; y++)
				for(int z = minId.z; z <= maxId.z; z++)
				{
					FASTLOG3(FLog::PathfindingDetail, "Voxelizing: %i %i %i", x, y, z);
					Voxel::OccupancyChunk* chunk = chunkMap.find(SpatialRegion::Id(x,y,z));
					if(!chunk)
					{
						count++;
						if(compute)
						{
							Voxel::OccupancyChunk& newChunk = chunkMap.insert(SpatialRegion::Id(x,y,z));
							newChunk.index = Vector3int32(x,y,z);
							newChunk.dirty = false;
							newChunk.age = currentFrameNum;
							voxelizer.occupancyUpdateChunk(newChunk, terrain, contactManager);
						}
					}
					else if(chunk->dirty)
					{
						count++;
						if(compute)
						{
							chunk->dirty = false;
							chunk->age = currentFrameNum;
							voxelizer.occupancyUpdateChunk(*chunk, terrain, contactManager);
						}
					}
				}

		FASTLOG1(FLog::PathfindingPerf, "Found %u dirty chunks", count);

		return count;
	}

	PathfindingService::PathfindingState::PathfindingState() : nodesByPos(DFInt::PathfindingDefaultBucketNum)
	{
	}

	void PathfindingService::computePath(CreatePathRequest& request)
	{
        RBXPROFILER_SCOPE("Voxel", "computePath");

		Vector3int16 startCell = request.startCell;
		Vector3int16 finishCell = request.finishCell;
		unsigned char emptyCutoff = request.emptyCutoff;

		int maxDistance = request.maxDistance;

		if(!adjustCell(finishCell, request.start, startCell, emptyCutoff)) {
			shared_ptr<Path> path = Creatable<Instance>::create<Path>(weak_from(this), Path::FailFinishNotEmpty);
			request.resumeFunction(path);
			return;
		}

		if(!adjustCell(startCell, request.finish, finishCell, emptyCutoff)) {
			shared_ptr<Path> path = Creatable<Instance>::create<Path>(weak_from(this), Path::FailStartNotEmpty);
			request.resumeFunction(path);
			return;
		}

		PathfindingState state;
		state.start = startCell;
		state.finish = finishCell;
		state.maxDistance = ((int)ceil(maxDistance/Voxel::kCELL_SIZE)) * PATHFINDING_WALK_COST;

		state.closestCell = startCell;
		state.closestDistance = INT_MAX;

		Vector3int16 parentCell;

		state.addOpenNode(Node(startCell, startCell, 0));

		Path::PathStatus status = Path::ClosestNoPath;

		while(state.openNodesByWeight.size() > 0)
		{
			Node node = state.popTopOpenNode();

			if(node.pos == finishCell)
			{
				// Success!!
				parentCell = node.parent;

				FASTLOG3(FLog::PathfindingDetail, "Found a path from node %i %i %i", parentCell.x, parentCell.y, parentCell.z);
				FASTLOG1F(FLog::PathfindingDetail, "Total path: %f", node.totalPath);
				status = Path::Success;
				break;
			}

			if(node.totalPath > state.maxDistance)
			{
				FASTLOG3(FLog::PathfindingDetail, "Found an outside node %i %i %i", node.pos.x, node.pos.y, node.pos.z);

				// There's open space outside the range, take closest item
				parentCell = state.closestCell;
				FASTLOG3(FLog::PathfindingDetail, "Returning closest node %i %i %i", parentCell.x, parentCell.y, parentCell.z);

				status = Path::ClosestOutOfRange;

				break;
			}
			SolidState solidUnder = isSolid(node.pos + Vector3int16(0,-1,0), emptyCutoff);

			FASTLOG1(FLog::PathfindingDetail, "Solid under: %i", solidUnder);

			// If underneath is empty, the only thing to do is fall
			if(solidUnder == EMPTY)
			{
				Vector3int16 newNodePos = node.pos + Vector3int16(0,-1,0);
				state.addOpenNode(Node(newNodePos, node.pos, node.totalPath + PATHFINDING_FALL_COST));
			} 
			else if(solidUnder == SOLID)
			{
				SideState sideState[4] = { BLOCKED, BLOCKED, BLOCKED, BLOCKED };

				for(unsigned dir = 0; dir < 4; dir++)
				{
					Vector3int16 side = node.pos + getOffsetByIndex(dir);
					Vector3int16 sideAbove = side + Vector3int16(0,1,0);

					FASTLOG4(FLog::PathfindingDetail, "Dir: %u, cell to check: %i %i %i", dir, side.x, side.y, side.z);

					SolidState solidAtSameLevel = isSolid(side, emptyCutoff);
					SolidState solidAbove = isSolid(sideAbove, emptyCutoff);

					FASTLOG2(FLog::PathfindingDetail, "Solid at same level: %i, solid above: %i", solidUnder, solidAbove);

					if(solidAtSameLevel == EMPTY)
					{
						state.addOpenNode(Node(side, node.pos, node.totalPath + PATHFINDING_WALK_COST));
						sideState[dir] = NOTBLOCKED;
					}
					else if(solidAtSameLevel == SOLID && solidAbove == EMPTY)
					{
						state.addOpenNode(Node(sideAbove, node.pos, node.totalPath + PATHFINDING_WALK_COST));
						sideState[dir] = NOTBLOCKEDABOVE;
					}
				}

				// Check diagonal states
				for(unsigned dir = 0; dir < 4; dir++)
				{
					if(sideState[dir] != BLOCKED && sideState[dir] == sideState[(dir+1) % 4] )
					{
						Vector3int16 diagSide = node.pos + getDiagonalOffsetByIndex(dir);
						Vector3int16 diagSideAbove = diagSide + Vector3int16(0,1,0);

						FASTLOG4(FLog::PathfindingDetail, "Diag dir: %u, cell to check: %i %i %i", dir, diagSide.x, diagSide.y, diagSide.z);

						SolidState solidAtSameLevel = isSolid(diagSide, emptyCutoff);
						SolidState solidAbove = isSolid(diagSideAbove, emptyCutoff);

						FASTLOG2(FLog::PathfindingDetail, "Solid at same level: %i, solid above: %i", solidUnder, solidAbove);
						if(solidAtSameLevel == EMPTY && sideState[dir] == NOTBLOCKED)
						{
							state.addOpenNode(Node(diagSide, node.pos, node.totalPath + PATHFINDING_WALK_DIAGLONAL_COST));
						}
						else if(solidAtSameLevel == SOLID && solidAbove == EMPTY && sideState[dir] == NOTBLOCKEDABOVE)
						{
							state.addOpenNode(Node(diagSideAbove, node.pos, node.totalPath + PATHFINDING_WALK_DIAGLONAL_COST));
						}
					}
				}
			}

			FASTLOG3(FLog::PathfindingDetail, "Adding closed node at %i %i %i", node.pos.x, node.pos.y, node.pos.z);
			RBXASSERT(node.closed);
		}

		std::vector<Vector3> pathPoints;

		shared_ptr<Path> path = Creatable<Instance>::create<Path>(weak_from(this), status);

		if(status != Path::Success)
		{
			FASTLOG3(FLog::PathfindingDetail, "After checking everything, returning closest node %i %i %i", parentCell.x, parentCell.y, parentCell.z);

			parentCell = state.closestCell;
		}
		else
		{
			pathPoints.push_back(request.isSmooth ?  Voxel::worldSpaceToCellSpace(request.finish) : Vector3(request.finishCell)+Vector3(0.5f, 0.5f,0.5f));
		}

		while(parentCell != startCell)
		{
			pathPoints.push_back(Vector3(parentCell)+Vector3(0.5f,0.5f,0.5f));
			NodesByPos::iterator itOpen = state.nodesByPos.find(parentCell);
			RBXASSERT(itOpen != state.nodesByPos.end());
			parentCell = itOpen->second.parent;

			FASTLOG3(FLog::PathfindingDetail, "Steping back to %i %i %i", parentCell.x, parentCell.y, parentCell.z);
		}

		pathPoints.push_back(request.isSmooth ?  Voxel::worldSpaceToCellSpace(request.start) : Vector3(request.startCell)+Vector3(0.5f, 0.5f, 0.5f));

		std::reverse(pathPoints.begin(), pathPoints.end());

		if(request.isSmooth)
		{
			for(int i = 0; i < DFInt::PathfindingSmoothIterations; i++)
				smoothPath(pathPoints, emptyCutoff, DFInt::PathfindingAverageWindow);
		}
		
		for(std::vector<Vector3>::iterator it = pathPoints.begin(); it != pathPoints.end(); ++it)
		{
			path->addPoint(*it);
		}

		FASTLOG1(FLog::PathfindingPerf, "Result: ", state.openNodesByWeight.size() > 0);

		request.resumeFunction(path);
	}

	void PathfindingService::computeRawPathAsync(Vector3 start, Vector3 finish, float maxDistance, boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		computePathAsync(start, finish, maxDistance, false, resumeFunction, errorFunction);
	}

	void PathfindingService::computeSmoothPathAsync(Vector3 start, Vector3 finish, float maxDistance, boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		computePathAsync(start, finish, maxDistance, true, resumeFunction, errorFunction);
	}

	void PathfindingService::computePathAsync(Vector3 start, Vector3 finish, float maxDistance, bool isSmooth, boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			DataModel* dm = DataModel::get(this);
			boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent,GA_CATEGORY_GAME, "PathfindingService", 
				RBX::StringConverter<int>::convertToString(dm->getPlaceID()).c_str(), 0,false));
		}

		
		if(maxDistance > DFInt::PathfindingMaxDistance) 
		{
			errorFunction("MaxDistance is too large");
			return;
		}

		if(start.max(finish).max() > kMaxWorldCoordinate)
		{
			errorFunction("Start or Finish are outside of supported range");
			return;
		}

		if(start.min(finish).min() < -kMaxWorldCoordinate)
		{
			errorFunction("Start or Finish are outside of supported range");
			return;
		}

		if (!job)
		{
			job.reset(new PathfindingJob(this));
			TaskScheduler::singleton().add(job);

			DataModel* dataModel = DataModel::get(this);
			ContactManagerSpatialHash* cmSpatialHash = dataModel->getWorkspace()->getWorld()->getContactManager()->getSpatialHash();
			cmSpatialHash->registerCoarseMovementCallback(this);

			MegaClusterInstance* terrain = static_cast<MegaClusterInstance*>(dataModel->getWorkspace()->getTerrain());

			if (terrain)
			{
				if (terrain->isSmooth())
					terrain->getSmoothGrid()->connectListener(this);
				else
					terrain->getVoxelGrid()->connectListener(this);
			}
		}

		FASTLOG1F(FLog::PathfindingPerf, "Pathfinding request, distance: %f", maxDistance);

		FASTLOG4F(FLog::PathfindingDetail, "Pathfinding request, start: %f %f %f, max distance: %f", start.x, start.y, start.z, maxDistance);
		FASTLOG3F(FLog::PathfindingDetail, "Pathfinding request, finish: %f %f %f", finish.x, finish.y, finish.z);

		Vector3int16 startCell = Voxel::worldToCell_floor(start);
		Vector3int16 finishCell = Voxel::worldToCell_floor(finish);

		FASTLOG3(FLog::PathfindingDetail, "Starting cell: %i %i %i", startCell.x, startCell.y, startCell.z);
		FASTLOG3(FLog::PathfindingDetail, "Finish cell: %i %i %i", finishCell.x, finishCell.y, finishCell.z);

		CreatePathRequest request;
		request.start = start;
		request.finish = finish;
		request.startCell = startCell;
		request.finishCell = finishCell;
		request.maxDistance = maxDistance;
		request.resumeFunction = resumeFunction;
		request.errorFunction = errorFunction;
		request.isSmooth = isSmooth;
		request.emptyCutoff = (unsigned char)(emptyCutoff*255.0f);

		if(countOrComputeDirtyChunks(startCell, finishCell, maxDistance, false) == 0)
			request.execute(this);
		else
		{
			scheduleRequest(shared_ptr<PathfindingRequest>(new CreatePathRequest(request)));
		}

		FASTLOG(FLog::PathfindingPerf, "Finished computeRawPathAsync");
	}

	
	void PathfindingService::scheduleRequest(const shared_ptr<PathfindingRequest>& request)
	{
		throttledRequests.push_back(request);
	}

	void PathfindingService::CreatePathRequest::execute(PathfindingService* service)
	{
		service->computePath(*this);
	}

	PathfindingService::SolidState PathfindingService::isSolid(const Vector3int16& pos, unsigned char emptyCutoff)
	{
		SpatialRegion::Id id = SpatialRegion::regionContainingVoxel(pos);
		Vector3int16 offset = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(pos);

		Voxel::OccupancyChunk* chunk = getChunkById(id);
		if(!chunk)
			return OUTSIDE;

		unsigned char occupancy = chunk->occupancy[offset.y][offset.z][offset.x];
		return occupancy > emptyCutoff ? SOLID : EMPTY;
	}

	void PathfindingService::executeThrottledRequests()
	{
		currentFrameNum++;

		if((currentFrameNum % DFInt::PathfindingCollectPeriod) == 0)
		{
			collectOldChunks();
		}

		if(throttledRequests.size() == 0)
			return;

		int budget = DFInt::PathfindingChunksPerInvokation;
		FASTLOG(FLog::PathfindingPerf, "Started executing throttled requests");

		FASTLOG1(FLog::PathfindingDetail, "Number of pending paths: %u, current frame: %u", throttledRequests.size());

		do 
		{
            RBXPROFILER_SCOPE("Voxel", "executeRequest");

			shared_ptr<PathfindingRequest> request;
			throttledRequests.pop_front(&request);
			FASTLOG3(FLog::PathfindingDetail, "Popped request, start cell: %i %i %i", request->startCell.x, request->startCell.y, request->startCell.z);
			FASTLOG3(FLog::PathfindingDetail, "Popped request, finish cell: %i %i %i", request->finishCell.x, request->finishCell.y, request->finishCell.z);

            RBXPROFILER_LABELF("Voxel", "(%d %d %d) -> (%d %d %d)", request->startCell.x, request->startCell.y, request->startCell.z, request->finishCell.x, request->finishCell.y, request->finishCell.z);

			FASTLOG(FLog::PathfindingPerf, "Started computing occupancy");

			int chunksToCompute = countOrComputeDirtyChunks(request->startCell, request->finishCell, request->maxDistance, true);
			budget -= chunksToCompute;

			FASTLOG1(FLog::PathfindingPerf, "Finished computing occupancy, %u chunks", chunksToCompute);

			FASTLOG2(FLog::PathfindingDetail, "Computed %u chunks, current budget: %i", chunksToCompute, budget);

			FASTLOG(FLog::PathfindingPerf, "Started computing path");
			request->execute(this);
			FASTLOG(FLog::PathfindingPerf, "Finished computing path");
		}
		while (budget > 0 && throttledRequests.size() > 0);

		FASTLOG(FLog::PathfindingPerf, "Finished executing throttled requests");
	}

	ExtentsInt32 hashToVoxelChunk(const ExtentsInt32& extents, int level)
	{
		ExtentsInt32 voxelCellCoords = extents.shiftLeft(level + 1);

		Vector3int32 voxelShift = Vector3int32(5,4,5);
		RBXASSERT((1 << voxelShift.x) == kVoxelChunkSizeXZ);
		RBXASSERT((1 << voxelShift.y) == kVoxelChunkSizeY);
		RBXASSERT((1 << voxelShift.z) == kVoxelChunkSizeXZ);

		ExtentsInt32 voxelChunkCoords = voxelCellCoords.shiftRight(voxelShift);
		return voxelChunkCoords;
	}

	void PathfindingService::markChunksDirty(const ExtentsInt32& extents)
	{
		for(int x = extents.low.x; x <= extents.high.x; x++)
			for(int y = extents.low.y; y <= extents.high.y; y++)
				for(int z = extents.low.z; z <= extents.high.z; z++)
				{
					Voxel::OccupancyChunk* chunk = getChunkById(SpatialRegion::Id(x,y,z));
					if(chunk)
						chunk->dirty = true;
				}
	}

	void PathfindingService::coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info)
	{
		PartInstance* part = PartInstance::fromPrimitive(p);
		if(part->getCookie() & PartCookie::IS_HUMANOID_PART)
		{
			return;
		}

		ExtentsInt32 newVoxelExtents = hashToVoxelChunk(info.newSpatialExtents, info.newLevel);
		markChunksDirty(newVoxelExtents);

		if(info.updateType == UpdateInfo::UPDATE_TYPE_Change && info.newLevel != info.oldLevel && info.newSpatialExtents != info.oldSpatialExtents)
		{
			ExtentsInt32 oldVoxelExtents = hashToVoxelChunk(info.oldSpatialExtents, info.oldLevel);
			markChunksDirty(oldVoxelExtents);
		}
	}

	void PathfindingService::terrainCellChanged(const Voxel::CellChangeInfo& info)
	{
		if(info.afterCell.solid.getBlock() == info.beforeCell.solid.getBlock())
			return;

		SpatialRegion::Id chunkId = SpatialRegion::regionContainingVoxel(info.position);
		Voxel::OccupancyChunk* chunk = getChunkById(chunkId);
		if(chunk)
			chunk->dirty = true;
	}
	
	void PathfindingService::onTerrainRegionChanged(const Voxel2::Region& region)
	{
		if (region.empty())
			return;

		SpatialRegion::Id regionIdMin = SpatialRegion::regionContainingVoxel(region.begin().toVector3int16());
		SpatialRegion::Id regionIdMax = SpatialRegion::regionContainingVoxel(region.end().toVector3int16() - Vector3int16(1, 1, 1));

		for (int y = regionIdMin.value().y; y <= regionIdMax.value().y; ++y)
			for (int z = regionIdMin.value().z; z <= regionIdMax.value().z; ++z)
				for (int x = regionIdMin.value().x; x <= regionIdMax.value().x; ++x)
					if (Voxel::OccupancyChunk* chunk = getChunkById(SpatialRegion::Id(x, y, z)))
						chunk->dirty = true;
	}
	
	void PathfindingService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		if(oldProvider)
		{
			DataModel* dataModel = DataModel::get(oldProvider);

			if(job)
			{
				if(dataModel->getWorkspace())
				{
					MegaClusterInstance* terrain = static_cast<MegaClusterInstance*>(dataModel->getWorkspace()->getTerrain());

					if (terrain)
					{
						if (terrain->isSmooth())
							terrain->getSmoothGrid()->disconnectListener(this);
						else
							terrain->getVoxelGrid()->disconnectListener(this);
					}
				}

				ContactManagerSpatialHash* cmSpatialHash = dataModel->getWorkspace()->getWorld()->getContactManager()->getSpatialHash();
				cmSpatialHash->unregisterCoarseMovementCallback(this);
				TaskScheduler::singleton().removeBlocking(job);
				job.reset();
			}
		}
	}


	int PathfindingService::checkPoints(int startPoint, const std::vector<Vector3>& points, unsigned char emptyCutoff)
	{
		for(std::vector<Vector3>::const_iterator it = points.begin() + startPoint; it != points.end(); ++it)
		{
			Vector3int16 cell = fastFloorInt16(*it);
			if(isSolid(cell, emptyCutoff) != EMPTY)
				return it - (points.begin() + startPoint);
		}

		return -1;
	}

	Voxel::OccupancyChunk* PathfindingService::getChunkById(const SpatialRegion::Id& id)
	{
		if(id == lastChunkId)
			return lastChunk;
		else
		{
			lastChunk = chunkMap.find(id);
			lastChunkId = id;
			return lastChunk;
		}
	}

	const char* const sPath = "Path";

	Path::Path(weak_ptr<PathfindingService> service, PathStatus status) : service(service), status(status)
	{
	}

	void Path::addPoint(const Vector3& point)
	{
		points.push_back(point);
	}

	void Path::reverse()
	{
		std::reverse(points.begin(), points.end());
	}

	void Path::OcclusionRequest::execute(PathfindingService* service)
	{
		int result = service->checkPoints(startPoint, path->getPoints(), emptyCutoff);
		resumeFunction(result == -1 ? -1 : (result + startPoint));
	}

	void Path::checkOcclusionAsync(int startPoint, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<PathfindingService> pathfindingService = service.lock();
		if(!pathfindingService)
		{
			errorFunction("PathfindingService no longer exists");
			return;
		}

		if(startPoint >= (int)points.size() || startPoint < 0)
		{
			errorFunction("Start point value is invalid");
			return;
		}

		Vector3 minPoint,maxPoint;
		minPoint = points[startPoint];
		maxPoint = points[startPoint];

		for(int i = startPoint+1; i < (int)points.size(); i++)
		{
			Vector3 point = points[i];
			minPoint = minPoint.min(point);
			maxPoint = maxPoint.max(point);
		}

		Vector3int16 startCell = fastFloorInt16(minPoint);
		Vector3int16 finishCell = fastFloorInt16(maxPoint);

		int dirtyChunks = pathfindingService->countOrComputeDirtyChunks(startCell, finishCell, 0, false);

		OcclusionRequest request;
		request.startCell = startCell;
		request.finishCell = finishCell;
		request.startPoint = startPoint;
		request.path = shared_from(this);
		request.maxDistance = 0;
		request.resumeFunction = resumeFunction;
		request.errorFunction = errorFunction;
		request.emptyCutoff = (unsigned char)(pathfindingService->getEmptyCutoff() * 255.0f);

		if(dirtyChunks == 0)
		{
			request.execute(pathfindingService.get());
		}
		else
		{
			pathfindingService->scheduleRequest(shared_ptr<OcclusionRequest>(new OcclusionRequest(request)));
		}
	}

	shared_ptr<const Reflection::ValueArray> Path::getPointCoordinates()
	{
		shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray);

		for(std::vector<Vector3>::iterator it = points.begin(); it != points.end(); ++it)
		{
			Vector3 cellPos = *it;		
			result->push_back(Voxel::cellSpaceToWorldSpace(cellPos));
		}

		return result;
	}
}

	
