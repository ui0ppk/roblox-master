/**
 * ChangeHistory.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */
#include "stdafx.h"

// C/C++ Headers
#include <stack>

// 3rd Party Headers
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

// Roblox Headers
#include "v8datamodel/changehistory.h"
#include "v8datamodel/datamodel.h"
#include "v8datamodel/workspace.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/JointInstance.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/Stats.h"
#include "util/ScopedAssign.h"
#include "script/ModuleScript.h"
#include "script/script.h"
#include "FastLog.h"
#include "v8datamodel/PartOperation.h"
#include "v8datamodel/CSGDictionaryService.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"

#include "Voxel2/Grid.h"

LOGGROUP(TerrainCellListener)

LOGVARIABLE(ChangeHistoryService, 0)

FASTFLAGVARIABLE(TeamCreate9938FixEnabled, true)

const char* const RBX::sChangeHistoryService = "ChangeHistoryService";

namespace RBX
{
	namespace Reflection
	{
		template<>
		EnumDesc<ChangeHistoryService::RuntimeUndoBehavior>::EnumDesc()
			:EnumDescriptor("RuntimeUndoBehavior")
		{
			addPair(RBX::ChangeHistoryService::Aggregate, "Aggregate");
			addPair(RBX::ChangeHistoryService::Snapshot, "Snapshot");
			addPair(RBX::ChangeHistoryService::Hybrid, "Hybrid");
		}
	}
}

using namespace RBX;
using namespace Reflection;

REFLECTION_BEGIN();
static BoundFuncDesc<ChangeHistoryService, void(bool)> func_SetEnabled(&ChangeHistoryService::setEnabled, "SetEnabled", "state", Security::Plugin);
static BoundFuncDesc<ChangeHistoryService, void(std::string)> func_SetWaypoint(&ChangeHistoryService::requestWaypoint2, "SetWaypoint", "name", Security::Plugin);
static BoundFuncDesc<ChangeHistoryService, void()> func_ResetWaypoints(&ChangeHistoryService::resetBaseWaypoint, "ResetWaypoints", Security::Plugin);
static BoundFuncDesc<ChangeHistoryService, void()> func_Redo(&ChangeHistoryService::playLua, "Redo", Security::Plugin);
static BoundFuncDesc<ChangeHistoryService, void()> func_Undo(&ChangeHistoryService::unplayLua, "Undo", Security::Plugin);
static BoundFuncDesc<ChangeHistoryService, shared_ptr<const Reflection::Tuple>()> func_CanUndo(&ChangeHistoryService::canUnplay2, "GetCanUndo", Security::Plugin);
static BoundFuncDesc<ChangeHistoryService, shared_ptr<const Reflection::Tuple>()> func_CanRedo(&ChangeHistoryService::canPlay2, "GetCanRedo", Security::Plugin);

static EventDesc<ChangeHistoryService, void(std::string)> event_OnUndo(&ChangeHistoryService::undoSignal, "OnUndo", "waypoint", Security::Plugin);
static EventDesc<ChangeHistoryService, void(std::string)> event_OnRedo(&ChangeHistoryService::redoSignal, "OnRedo", "waypoint", Security::Plugin);
REFLECTION_END();

RBX::ChangeHistoryService::RuntimeUndoBehavior RBX::ChangeHistoryService::runtimeUndoBehavior = RBX::ChangeHistoryService::Aggregate;

#define RBX_GET_CELL_DETAIL(cellData)                  (cellData & 0xff)
#define RBX_GET_CELL_MATERIAL(cellData)                (Voxel::CellMaterial)((cellData>>8) & 0xff)
#define RBX_GET_CELL_INDEX(cellData)                   (cellData>>16)
#define RBX_SET_CELL_DATA(cellIndex, detail, material) ((cellIndex<<16) | (material<<8)| (detail))

#define RBX_MAX_CLUSTER_CELLS (Voxel::kXZ_CHUNK_SIZE * Voxel::kXZ_CHUNK_SIZE * Voxel::kY_CHUNK_SIZE)

static Vector3int16 cellIndexToVector3(const boost::uint32_t &cellIndex) 
{	return Vector3int16(cellIndex & 0x1f, cellIndex>>10, (cellIndex>>5) & 0x1f); }

static bool compareClusterData_(const unsigned int &lhs, const unsigned int &rhs)
{	return (RBX_GET_CELL_INDEX(lhs) < RBX_GET_CELL_INDEX(rhs)); }

static const PropertyDescriptor* getScriptSourceDescriptorIfScript(shared_ptr<Instance> instance)
{
	if (dynamic_cast<Script*>(instance.get()))
	{
		return &Script::prop_EmbeddedSourceCode;
	}
	else if (Instance::fastDynamicCast<ModuleScript>(instance.get()))
	{
		return &ModuleScript::prop_Source;
	}
	else
	{
		return NULL;
	}
}

static bool isReplicatedChange()
{
	return RBX::Security::Context::current().identity == RBX::Security::Replicator_;
}

class ChangeHistoryStatsItem : public Stats::Item
{
public:

	ChangeHistoryStatsItem()
	{
		setName("ChangeHistory");
	}

	static shared_ptr<ChangeHistoryStatsItem> create(ChangeHistoryService& service)
	{
		shared_ptr<ChangeHistoryStatsItem> stats = Creatable<Instance>::create<ChangeHistoryStatsItem>();

        stats->createChildItem<int>("Data Size",boost::bind(&ChangeHistoryService::getWaypointDataSize,&service));
        stats->createChildItem<int>("Stack Size",boost::bind(&ChangeHistoryService::getWaypointCount,&service));

        return stats;
    }   
};

class ChangeHistoryService::Item
{
public:
	ChangeHistoryService* changeHistory;
	shared_ptr<Instance> instance;
	
	enum Action { None, Create, Change, Delete, Replicate };
	Action action;
	
	// if creating, the parent property. otherwise null
	shared_ptr<Instance> parent;	
	
	// if creating, all properties except the parent property
	// if changing, all properties that have changed
	// if deleting, then empty
	typedef std::map<const PropertyDescriptor*, Variant> Properties;
	Properties properties;
	
	// map <chunkIndex, chunkCells>
	typedef std::vector<unsigned int>           ChunkCells;
	typedef boost::unordered_map<SpatialRegion::Id, ChunkCells, SpatialRegion::Id::boost_compatible_hash_value> ClusterCells;
	ClusterCells clusterCells;

    // map <chunkIndex, chunkCells>
    static const unsigned int kChunkSizeLog2 = 3;
	typedef boost::unordered_map<Vector3int32, Voxel2::Box> SmoothClusterCells;
    SmoothClusterCells smoothClusterCells;

    unsigned int order;
	
	bool isEmpty() const
	{
		return action==None;
	}
private:
	void apply(const std::pair<const PropertyDescriptor*, Variant>& pair) const
	{
		if (pair.first == &Instance::propParent)
			if (instance->getIsParentLocked())
				return;
		pair.first->setVariant(instance.get(), pair.second);
	}
	
	void applyClusterData(const std::pair<SpatialRegion::Id, ChunkCells> &chunkCellData) const
	{
		ChunkCells chunkCells = chunkCellData.second;
		for (ChunkCells::const_iterator chunkCellsIter = chunkCells.begin(); chunkCellsIter != chunkCells.end(); ++chunkCellsIter)
		{
			//before setting values apply appropriate conversions
			changeHistory->setCell(chunkCellData.first, 
					cellIndexToVector3(RBX_GET_CELL_INDEX(*chunkCellsIter)),
					Voxel::Cell::readUnsignedCharFromDeprecatedUse(RBX_GET_CELL_DETAIL(*chunkCellsIter)),
					RBX_GET_CELL_MATERIAL(*chunkCellsIter));
		}
	}

	void applySmoothClusterData(const Vector3int32& chunkId, const Voxel2::Box& cells) const
    {
        Voxel2::Region chunkRegion = Voxel2::Region::fromChunk(chunkId, Item::kChunkSizeLog2);

        changeHistory->getTerrain()->getSmoothGrid()->write(chunkRegion, cells);
    }
	
	void addValue(const PropertyDescriptor& propertyDescriptor)
	{
		if (propertyDescriptor.isReadOnly() || propertyDescriptor.isWriteOnly())
			return;
		if (!propertyDescriptor.canXmlWrite())
			return;
		
		if (!propertyDescriptor.canReplicate() && propertyDescriptor != PartOperation::desc_ChildData && propertyDescriptor != PartOperation::prop_CollisionFidelity)
			return;
		
		Variant value;
		propertyDescriptor.getVariant(instance.get(), value);
		//if (properties.find(&propertyDescriptor)==properties.end())
		//	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService::recording %s.%s\n", instance->getName().c_str(), propertyDescriptor.name.c_str());
		properties[&propertyDescriptor] = value;
	}
	
	void addValueIfNotParentProperty(const Property& propertyDescriptor)
	{
		// treat parent as a special case
		if (&propertyDescriptor.getDescriptor()!=&Instance::propParent)
			addValue(propertyDescriptor.getDescriptor());
	}
	
	
	void absorbProp(const std::pair<const PropertyDescriptor*, Variant>& pair)
	{
		properties[pair.first] = pair.second;
	}
	
	void absorbClusterData(const std::pair<SpatialRegion::Id, ChunkCells> &chunkCellData)
	{
		ClusterCells::iterator clusterCellsIter = clusterCells.find(chunkCellData.first);
		if (clusterCellsIter == clusterCells.end())
		{
			clusterCells.insert(chunkCellData);
			return;
		}
		
		ChunkCells chunkCellsToCopy = chunkCellData.second, &chunkCellsLocal = clusterCellsIter->second, tempVector(RBX_MAX_CLUSTER_CELLS, 0);
		
		//copy local data in temp vector
		for (ChunkCells::const_iterator chunkCellsLocalIter = chunkCellsLocal.begin(); chunkCellsLocalIter != chunkCellsLocal.end(); ++chunkCellsLocalIter)
			tempVector[RBX_GET_CELL_INDEX(*chunkCellsLocalIter)] = *chunkCellsLocalIter;
		
		//copy data to be assigned in temp vector
		for (ChunkCells::const_iterator chunkCellsToCopyIter = chunkCellsToCopy.begin(); chunkCellsToCopyIter != chunkCellsToCopy.end(); ++chunkCellsToCopyIter)
			tempVector[RBX_GET_CELL_INDEX(*chunkCellsToCopyIter)] = *chunkCellsToCopyIter;
		
		//clear local data
		ChunkCells().swap(chunkCellsLocal);
		chunkCellsLocal.reserve(RBX_MAX_CLUSTER_CELLS);
		
		//again copy modified data
		for (ChunkCells::const_iterator tempVectorIter = tempVector.begin(); tempVectorIter != tempVector.end(); ++tempVectorIter)
		{
			if (*tempVectorIter & 0xffff)
				chunkCellsLocal.push_back(*tempVectorIter);
		}
		
		//clear off unwanted memory
		ChunkCells(chunkCellsLocal).swap(chunkCellsLocal);
	}

	void absorbSmoothClusterData(const Vector3int32& chunkId, const Voxel2::Box& cells)
	{
		smoothClusterCells[chunkId] = cells;

        action = Change;
	}
	
public:
	Item(ChangeHistoryService* changeHistory, shared_ptr<Instance> instance)
	:instance(instance)
	,changeHistory(changeHistory)
	,action(None)
    ,order(0)
	{
	}
	
	Item() 
		:action(None)
        ,order(0)
	{}

    int computeDataSize() const
    {
        int total = 0;

		// add the properties
        Properties::const_iterator propertyIter = properties.begin();
        for ( ; propertyIter != properties.end() ; ++propertyIter )
            total += propertyIter->first->getDataSize(instance.get());

		// add the terrain clusters
        for (ClusterCells::const_iterator cellIter = clusterCells.begin(); cellIter != clusterCells.end() ; ++cellIter)
            total += cellIter->second.size() * sizeof(int);

		for (SmoothClusterCells::const_iterator cellIter = smoothClusterCells.begin(); cellIter != smoothClusterCells.end() ; ++cellIter)
			total += cellIter->second.getSizeX() * cellIter->second.getSizeY() * cellIter->second.getSizeZ() * sizeof(Voxel2::Cell);

        return total;
    }

	void absorb(const Item& next)
	{
		RBXASSERT(action==Create || action == Change);
		RBXASSERT(next.action!=Delete);
		if (next.parent)
			this->parent = next.parent;
		
		std::for_each(next.properties.begin(), next.properties.end(), boost::bind(&Item::absorbProp, this, _1));
		
		std::for_each(next.clusterCells.begin(), next.clusterCells.end(), boost::bind(&Item::absorbClusterData, this, _1));
		
		for (SmoothClusterCells::const_iterator it = next.smoothClusterCells.begin(); it != next.smoothClusterCells.end(); ++it)
			absorbSmoothClusterData(it->first, it->second);
	}
	
	void onSetWaypoint()
	{
		ChunkCells tempVector;
		unsigned int vectorMemory = 0;
			
		for (ClusterCells::iterator iter = clusterCells.begin(); iter != clusterCells.end(); ++iter)
		{
			ChunkCells &cells = iter->second;
			tempVector.reserve(RBX_MAX_CLUSTER_CELLS);
				
			for (ChunkCells::const_iterator cells_iter = cells.begin(); cells_iter != cells.end(); ++cells_iter)
			{
				if (*cells_iter & 0xffff)
					tempVector.push_back(*cells_iter);
			}
				
			//assign the temp vector
			ChunkCells().swap(cells);
			cells.reserve(tempVector.size());
			cells.assign(tempVector.begin(), tempVector.end());
				
			//clear temp vector
			ChunkCells().swap(tempVector);
				
			vectorMemory += (sizeof(unsigned int)*cells.size())+sizeof(cells);
		}
		
		FASTLOG1(FLog::ChangeHistoryService, "Waypoint set, memory consumed: %u KB", (vectorMemory+(sizeof(unsigned int)*clusterCells.size())+sizeof(clusterCells))/1024);
	}
	
	void recordDelete()
	{
		parent.reset();
		properties.clear();
		if (action==Create || isReplicatedChange())
			action = None;
		else
			action = Delete;
	}
	
	void recordCreate()
	{
		RBXASSERT(action==Delete || action==None);
		// Record all properties except for propParent
		// TODO: Use Replicator::getDefault to avoid writing properties that are the same as the default?
		std::for_each(instance->properties_begin(), instance->properties_end(), boost::bind(&Item::addValueIfNotParentProperty, this, _1));
		parent = shared_from(instance->getParent());

		if (isReplicatedChange())
			action = Replicate;
		else
			action = Create;
	}

	void addClusterChunk(const Voxel::Grid* voxelStore, const SpatialRegion::Id& id)
    {
        Region3int16 regionCoords = SpatialRegion::inclusiveVoxelExtentsOfRegion(id);
        Voxel::Grid::Region region = voxelStore->getRegion(regionCoords.getMinPos(), regionCoords.getMaxPos());
        
        if (!region.isGuaranteedAllEmpty())
        {
            Vector3int16 startLocation = regionCoords.getMinPos();
            ChunkCells& cells = recordClusterDataGetChunk(id);
            
            for (Voxel::Grid::Region::iterator it = region.begin(); it != region.end(); ++it)
            {
                recordClusterDataSetCell(cells, it.getCurrentLocation() - startLocation, it.getCellAtCurrentLocation(), it.getMaterialAtCurrentLocation());
            }
        }
    }

	void addSmoothClusterRegion(const Voxel2::Grid* grid, const Voxel2::Region& region, bool ignoreEmpty)
    {
        std::vector<Vector3int32> chunks = region.getChunkIds(Item::kChunkSizeLog2);

        for (size_t i = 0; i < chunks.size(); ++i)
		{
            Vector3int32 chunkId = chunks[i];
			Voxel2::Region chunkRegion = Voxel2::Region::fromChunk(chunkId, Item::kChunkSizeLog2);
			Voxel2::Box chunkBox = grid->read(chunkRegion);

			if (!ignoreEmpty || !chunkBox.isEmpty())
				absorbSmoothClusterData(chunkId, chunkBox);
		}
	}

	void addClusterData(const Voxel::Grid* voxelStore)
	{
		if (!voxelStore || voxelStore->getNonEmptyCellCount() == 0)
			return;

        std::vector<SpatialRegion::Id> chunks = voxelStore->getNonEmptyChunks();

        for (size_t i = 0; i < chunks.size(); ++i)
            addClusterChunk(voxelStore, chunks[i]);
	}

	void addSmoothClusterData(const Voxel2::Grid* grid)
	{
		if (!grid || !grid->isAllocated())
			return;

        std::vector<Voxel2::Region> regions = grid->getNonEmptyRegions();

        for (size_t i = 0; i < regions.size(); ++i)
            addSmoothClusterRegion(grid, regions[i], /* ignoreEmpty= */ true);
	}

	void recordProperty(const PropertyDescriptor* descriptor)
	{
		addValue(*descriptor);
		if (action!=Create)
		{
			action = Change;
			parent = shared_from(instance->getParent());
			//add parent changed property, it doesn't get added in addValue
			if (descriptor==&Instance::propParent)
				properties[descriptor] = parent;
		}
		else if (descriptor==&Instance::propParent)
			parent = shared_from(instance->getParent());
	}
	
	ChunkCells& recordClusterDataGetChunk(const SpatialRegion::Id& chunkIndex)
	{
		ClusterCells::iterator clusterCellsIter = clusterCells.find(chunkIndex);
		if (clusterCellsIter == clusterCells.end())
		{
			ChunkCells cells(RBX_MAX_CLUSTER_CELLS, 0);
			clusterCells[chunkIndex] = cells;
			return clusterCells[chunkIndex];
		}
		else 
		{
			return clusterCellsIter->second;
		}
	}
	
	void recordClusterDataSetCell(ChunkCells& cells, const Vector3int16 &cellPos, const Voxel::Cell &cellDetail, const Voxel::CellMaterial &material)
	{
		boost::uint32_t cellIndex = ((cellPos.x) | (cellPos.z << 5) | (cellPos.y << 10));
		unsigned int cellData = RBX_SET_CELL_DATA(cellIndex, Voxel::Cell::asUnsignedCharForDeprecatedUses(cellDetail), material);
		
		cells[cellIndex] = cellData;
		
		action = Change;
	}
	
	void recordClusterData(const SpatialRegion::Id &chunkIndex, const Vector3int16 &cellPos, const Voxel::Cell &cellDetail, const Voxel::CellMaterial &material)
	{
		ChunkCells& cells = recordClusterDataGetChunk(chunkIndex);
		
		recordClusterDataSetCell(cells, cellPos, cellDetail, material);
	}

	void updateClusterData(const SpatialRegion::Id &chunkIndex, const Vector3int16 &cellPos, const Voxel::Cell &cellDetail, const Voxel::CellMaterial &material)
	{
		ChunkCells& cells = recordClusterDataGetChunk(chunkIndex);
		boost::uint32_t cellIndex = ((cellPos.x) | (cellPos.z << 5) | (cellPos.y << 10));
		unsigned int cellData = RBX_SET_CELL_DATA(cellIndex, Voxel::Cell::asUnsignedCharForDeprecatedUses(cellDetail), material);
		for (auto& cell: cells)
		{
			if (RBX_GET_CELL_INDEX(cell) == cellIndex)
			{
				cell = cellData;
				break;
			}
		}
	}

	void updateSmoothClusterData(const Vector3int32& chunkId, const Voxel2::Grid* grid)
	{
		Voxel2::Region chunkRegion = Voxel2::Region::fromChunk(chunkId, Item::kChunkSizeLog2);
		Voxel2::Box chunkBox = grid->read(chunkRegion);
		smoothClusterCells[chunkId] = chunkBox;	
	}
	
	void play() const
	{
		switch (action)
		{
			case Delete:
				//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService Delete %s 0x%X\n", instance->getName().c_str(), instance.get());
				instance->setParent(NULL);
				break;
			case Change:
			{
				//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService Change %s 0x%X\n", instance->getName().c_str(), instance.get());
				std::for_each(properties.begin(), properties.end(), boost::bind(&Item::apply, this, _1));
				if (changeHistory->withLongRunningOperation)
					changeHistory->withLongRunningOperation(boost::bind(&Item::playClusterChange, this), "Redo in progress...");
				else 
					playClusterChange();
			}
				break;
			case Create:
			case Replicate:
				//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService Create %s 0x%X\n", instance->getName().c_str(), instance.get());
				std::for_each(properties.begin(), properties.end(), boost::bind(&Item::apply, this, _1));
				if (properties.find(&Instance::propParent) == properties.end())
					instance->setParent(parent.get());
				break;
            default:
                break;
		}
	}
	void unplay() const
	{
		switch (action)
		{
			case Delete:
				unplayDelete();
				break;
			case Change:
				//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService Un-Change %s 0x%X\n", instance->getName().c_str(), instance.get());
				unplayChange();
				break;
			case Create:
				//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService Un-Create %s 0x%X\n", instance->getName().c_str(), instance.get());
				instance->setParent(NULL);
				break;
            default:
                break;
		}
	}

	void unplay_CFrame() const
	{
		Properties::const_iterator iter = properties.find(&PartInstance::prop_CFrame);
		if (iter != properties.end())
			unplayProperty(*iter);
	}
	
	bool getCellData(const SpatialRegion::Id& chunkIndex, boost::uint32_t cellIndex, unsigned int &cellData)
	{
		ClusterCells::iterator clusterCellsIter = clusterCells.find(chunkIndex);
		if (clusterCellsIter == clusterCells.end())
			return false;
		
		const ChunkCells &chunkCells = clusterCellsIter->second;
		unsigned int compareData = RBX_SET_CELL_DATA(cellIndex, 0, 0);
		
		ChunkCells::const_iterator it_start = std::lower_bound(chunkCells.begin(), chunkCells.end(), compareData, compareClusterData_);
		ChunkCells::const_iterator it_end   = std::upper_bound(chunkCells.begin(), chunkCells.end(), compareData, compareClusterData_);
		
		if (it_start == it_end)
			return false;
		
		cellData = *it_start;
		return true;
	}
	
private:
	void unplayProperty(const std::pair<const PropertyDescriptor*, Variant>& pair) const;
	void unplayClusterData(const std::pair<SpatialRegion::Id, ChunkCells> &chunkCellData) const;
	void unplaySmoothClusterData(const Vector3int32& chunkId) const;
	
	void unplayChange() const
	{
		std::for_each(properties.begin(), properties.end(), boost::bind(&Item::unplayProperty, this, _1));
		if (changeHistory->withLongRunningOperation)
			changeHistory->withLongRunningOperation(boost::bind(&Item::unplayClusterChange, this), "Undo in progress...");
		else
			unplayClusterChange();
	}
	
	void unplayClusterChange() const
	{ 
		std::for_each(clusterCells.begin(), clusterCells.end(), boost::bind(&Item::unplayClusterData, this, _1));

		for (SmoothClusterCells::const_iterator it = smoothClusterCells.begin(); it != smoothClusterCells.end(); ++it)
			unplaySmoothClusterData(it->first);
	}
	
	void playClusterChange() const
	{
		std::for_each(clusterCells.begin(), clusterCells.end(), boost::bind(&Item::applyClusterData, this, _1));

		for (SmoothClusterCells::const_iterator it = smoothClusterCells.begin(); it != smoothClusterCells.end(); ++it)
			applySmoothClusterData(it->first, it->second);
	}
	
	void unplayDelete() const;
};

class ChangeHistoryService::Waypoint
{
	typedef boost::unordered_map<Instance*, Item> Items;
	Items items;
    unsigned int itemsOrderMax;
	
	ChangeHistoryService* changeHistory;

	void selectModifiedParts(bool isPlay);

    struct ItemOrderComparator
    {
        bool operator()(const Item* lhs, const Item* rhs) const
        {
            return lhs->order < rhs->order;
        }
    };
	
    std::vector<const Item*> getItemsOrdered() const
    {
        std::vector<const Item*> result;
        result.reserve(items.size());

		for (Items::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            RBXASSERT(it->second.order > 0);
            result.push_back(&it->second);
        }

        std::sort(result.begin(), result.end(), ItemOrderComparator());

        return result;
    }
public:
	std::string name;
	Waypoint(ChangeHistoryService* changeHistory):changeHistory(changeHistory), itemsOrderMax(0) {}
	void play();
	void unplay();
	
	bool isEmpty() const
	{
		for (Items::const_iterator it = items.begin(); it != items.end(); ++it)
			if (!it->second.isEmpty())
				return false;

		return true;
	}
	void removeItem(Instance* instance)
	{
        items.erase(instance);
	}

	Item* findItem(Instance* instance)
	{
		Items::iterator iter = items.find(instance);
		if (iter==items.end())
			return NULL;
		else
            return &iter->second;
	}

	Item& getItem(const shared_ptr<Instance>& instance)
	{
		Items::iterator iter = items.find(instance.get());
		if (iter != items.end())
            return iter->second;
		
        Item& item = items[instance.get()];

		item = Item(changeHistory, instance);
        item.order = ++itemsOrderMax;
		
		return item;
	}
	
	void addItem(const Item& item)
	{
		RBXASSERT_SLOW(!findItem(item.instance.get()));

        Item& copy = items[item.instance.get()];
        copy = item;
        copy.order = ++itemsOrderMax;
	}

	void absorb(const Waypoint* next)
	{
        std::vector<const Item*> itemsOrdered = next->getItemsOrdered();

		for (size_t i = 0; i < itemsOrdered.size(); ++i)
		{
			const Item& src = *itemsOrdered[i];

			Item* dst = findItem(src.instance.get());
			if (dst)
			{
				if (src.action==Item::Delete)
					removeItem(src.instance.get());
				else
					dst->absorb(src);
			}
			else
			{
				//				RBXASSERT(src.action==Item::Create);	// TODO  - this assert is going off
				addItem(src);
			}
		}
	}
	
	void onSetWaypoint()
	{
        for (Items::iterator it = items.begin(); it != items.end(); ++it)
            it->second.onSetWaypoint();
    }

    int computeDataSize() const
    {
        int total = 0;

        for (Items::const_iterator it = items.begin(); it != items.end(); ++it)
            total += it->second.computeDataSize();

        return total;
    }
};



void ChangeHistoryService::Item::unplayDelete() const
{
	// Go back in time to the first matching Create action. 
	// Play the action, and then go forward, applying Change actions
	
	std::stack<Item*> changeItems;
	
	Waypoints::iterator iter = changeHistory->unplayWaypoint;
    Item* deleteItem = (*iter)->findItem(instance.get());
	
	bool createdItem = false;
	while (!createdItem && iter!=changeHistory->waypoints.begin())
	{
		--iter;
		Item* item = (*iter)->findItem(instance.get());
		if (item)
		{
			RBXASSERT(item->action!=Delete);
			if (item->action==Create || item->action==Replicate)
			{
				item->play();
				createdItem = true;
			}
			else
				changeItems.push(item);
		}
	}
	
	// OK. The item has been re-created. Now update its properties to the latest values (unwind the stack)
	while (!changeItems.empty())
	{
		changeItems.top()->play();
		changeItems.pop();
	}

    if ( deleteItem )
    {
        // restore any properties that were saved with the delete.
        // for scripts, the deleted item also has the current source code.
        std::for_each(
            deleteItem->properties.begin(), 
            deleteItem->properties.end(), 
            boost::bind(&Item::apply,deleteItem,_1) );
    }
};


void ChangeHistoryService::Item::unplayProperty(const std::pair<const PropertyDescriptor*, Variant>& pair) const
{
	// Go back in time and apply the first matching property
	const bool isParentProp = pair.first==&Instance::propParent;
	
	Waypoints::iterator iter = changeHistory->unplayWaypoint;
	while (iter!=changeHistory->waypoints.begin())
	{
		--iter;
		Item* item = (*iter)->findItem(instance.get());
		if (item)
		{
			RBXASSERT(item->action!=Delete);
			Properties::iterator find = item->properties.find(pair.first);
			if (find != item->properties.end())
			{
				item->apply(*find);
				return;
			}
			if (isParentProp && (item->action == Create || item->action == Change || (FFlag::TeamCreate9938FixEnabled && (item->action == Replicate))))
			{
				// Special-case the parent property
				instance->setParent(item->parent.get());
				return;
			}
		}
	}
}

void ChangeHistoryService::Item::unplayClusterData(const std::pair<SpatialRegion::Id, ChunkCells> &chunkCellData) const
{
	SpatialRegion::Id chunkIndex = chunkCellData.first;
    unsigned int cellData = 0;
	const ChunkCells &chunkCells   = chunkCellData.second;
	Vector3int16 cellPosInChunk;

	boost::uint32_t cellIndexInChunk;
	bool cellFound = false;
	
	for (ChunkCells::const_iterator chunkCellsIter = chunkCells.begin(); chunkCellsIter != chunkCells.end(); ++chunkCellsIter)
	{
		cellIndexInChunk = RBX_GET_CELL_INDEX(*chunkCellsIter);
		cellPosInChunk   = cellIndexToVector3(cellIndexInChunk);
		
		cellFound = false;
		
		//Go back in time to get the first matching cell and apply it's state
		Waypoints::iterator iter = changeHistory->unplayWaypoint;
		while (iter!=changeHistory->waypoints.begin())
		{
			--iter;
			Item* item = (*iter)->findItem(instance.get());
			if (item)
			{
				if (item->getCellData(chunkIndex, cellIndexInChunk, cellData))
				{
					changeHistory->setCell(chunkIndex, cellPosInChunk,
						Voxel::Cell::readUnsignedCharFromDeprecatedUse(RBX_GET_CELL_DETAIL(cellData)),
						RBX_GET_CELL_MATERIAL(cellData));
					cellFound = true;
					break;
				}
			}
		}
		
		//if no cell found then set the default status
		if (!cellFound)
			changeHistory->setCell(chunkIndex, cellPosInChunk,
				Voxel::Constants::kUniqueEmptyCellRepresentation, Voxel::CELL_MATERIAL_Water);
	}
}

void ChangeHistoryService::Item::unplaySmoothClusterData(const Vector3int32& chunkId) const
{
	Waypoints::iterator iter = changeHistory->unplayWaypoint;
	while (iter!=changeHistory->waypoints.begin())
	{
		--iter;
		Item* item = (*iter)->findItem(instance.get());
		if (item)
		{
			SmoothClusterCells::iterator it = item->smoothClusterCells.find(chunkId);

			if (it != item->smoothClusterCells.end())
			{
				applySmoothClusterData(chunkId, it->second);
                return;
			}
		}
	}

	unsigned int chunkSize = 1 << kChunkSizeLog2;
    
	applySmoothClusterData(chunkId, Voxel2::Box(chunkSize, chunkSize, chunkSize));
}

ChangeHistoryService::ChangeHistoryService(void)
	:Super(sChangeHistoryService)
	,playing(false)
	,recording(0)
	,enabled(false)
    ,dataSize(0)
{
	playWaypoint = waypoints.end();
	unplayWaypoint = waypoints.end();
	runStartWaypoint = waypoints.end();
}

template< class T >void delete_helper(T* ptr){ delete( ptr );};

ChangeHistoryService::~ChangeHistoryService()
{
	if (recording)
		delete recording;
}

void ChangeHistoryService::setEnabled(bool state)
{
	if (state==enabled)
		return;
	enabled = state;
	if (state)
		attach();
	else
		dettach();
}

shared_ptr<const Reflection::Tuple> ChangeHistoryService::canUnplay2()
{
	std::string name;
	bool can = enabled && getUnplayWaypoint(name);
	if (can)
	{
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(2));
		result->values[0] = true;
		result->values[1] = name;
		return result;
	}
	else
	{
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(1));
		result->values[0] = false;
		return result;
	}
}

shared_ptr<const Reflection::Tuple> ChangeHistoryService::canPlay2()
{
	std::string name;
	bool can = enabled && getPlayWaypoint(name);
	if (can)
	{
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(2));
		result->values[0] = true;
		result->values[1] = name;
		return result;
	}
	else
	{
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(1));
		result->values[0] = false;
		return result;
	}
}


bool ChangeHistoryService::getPlayWaypoint(std::string& name, int steps) const
{
	if (!enabled)
		return false;
	
	Waypoints::iterator waypoint = playWaypoint;
	for (int i=0; i<steps; ++i)
	{
		if (waypoint==waypoints.end())
			return false;
		++waypoint;
	}
	if (waypoint==waypoints.end())
		return false;
	else
	{
		name = (*waypoint)->name;
		return true;
	}
}

bool ChangeHistoryService::getUnplayWaypoint(std::string& name, int steps) const
{
	if (!enabled)
		return false;
	
	Waypoints::iterator waypoint = unplayWaypoint;
	for (int i=0; i<steps; ++i)
	{
		if (waypoint==waypoints.begin())
			return false;
		--waypoint;
	}
	if (waypoint==waypoints.end())
		return false;
	else if (waypoint==waypoints.begin())
		return false;	// can't undo the very first waypoint!
	else
	{
		name = (*waypoint)->name;
		return true;
	}
}

void ChangeHistoryService::requestWaypoint(const char* name, const Instance* context)
{
	ChangeHistoryService* s = ServiceProvider::find<ChangeHistoryService>(context);
	if (s)
		s->requestWaypoint(name);
}

void ChangeHistoryService::resetBaseWaypoint()
{
	if (!enabled)
		return;
	
	// Record the current state, and then merge everything to a single waypoint
	setWaypoint("base");
	while (waypoints.size()>1)
		mergeFirstTwoWaypoints();

    waypointChangedSignal();
}

void ChangeHistoryService::setWaypoint(const char* name)
{
	if (!enabled)
		return;
	
    if (runtimeUndoBehavior == Snapshot)
        if (runService && runService->isRunState())
            reportMissedPhysicsChanges(shared_from(dataModel->getWorkspace()));

    trimWaypoints();

	if (recording && !recording->isEmpty())
	{
		recording->name = name;
		recording->onSetWaypoint();
		waypoints.push_back(recording);
        computeDataSize();

		playWaypoint = waypoints.end();
		unplayWaypoint = --waypoints.end();
		recording = new Waypoint(this);
	}

    waypointChangedSignal();
}

void ChangeHistoryService::requestWaypoint(const char* name)
{
	CSGDictionaryService* dictionaryService = ServiceProvider::create< CSGDictionaryService >(this);
	NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(this);
	dictionaryService->clean();
	nrDictionaryService->clean();

	if (checkSettingWaypoint() == Accept)
		setWaypoint(name);
}

void ChangeHistoryService::mergeFirstTwoWaypoints()
{
	Waypoints::iterator next = ++waypoints.begin();

	//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService::mergeFirstTwoWaypoints %s\n", (*next)->name.c_str());

	waypoints.front()->absorb(*next);
	
	if (unplayWaypoint==next)
		--unplayWaypoint;

	if (runStartWaypoint==next)
		runStartWaypoint = waypoints.end();

	if (playWaypoint==next)
		++playWaypoint;
	
	delete *next;
	waypoints.erase(next);

    computeDataSize();
}

template<class I>
static bool isInRange(I i, I begin, I end)
{
	for (; begin != end; ++begin)
		if (i == begin)
			return true;
	return false;
}

void ChangeHistoryService::computeDataSize()
{
    dataSize = 0;

    Waypoints::const_iterator iter = waypoints.begin();
    for ( ; iter != waypoints.end() ; ++iter )
        dataSize += (*iter)->computeDataSize();
}

void ChangeHistoryService::trimWaypoints()
{
	if (!enabled)
		return;
	
	if (playWaypoint != waypoints.end())
	{
		if (isInRange(runStartWaypoint, playWaypoint, waypoints.end()))
		{
			RBXASSERT(runStartWaypoint != waypoints.end());
			runStartWaypoint = waypoints.end();
		}

		std::for_each(playWaypoint, waypoints.end(), delete_helper<Waypoint>);
		waypoints.erase(playWaypoint, waypoints.end());

		playWaypoint = waypoints.end();
		unplayWaypoint = --waypoints.end();

        computeDataSize();
	}

	while ( waypoints.size() > minWaypoints )
    {
        if ( waypoints.size() > maxWaypoints )
        {
		    mergeFirstTwoWaypoints();
            continue;
        }

        if ( dataSize > maxMemoryUsage )
        {
		    mergeFirstTwoWaypoints();
            continue;
        }

        break;
    }
}

void ChangeHistoryService::clearWaypoints()
{
	if (recording)
	{
		delete recording;
		recording = new Waypoint(this);
	}
	std::for_each(waypoints.begin(), waypoints.end(), delete_helper<Waypoint>);
	waypoints.clear();
	playWaypoint = waypoints.end();
	unplayWaypoint = waypoints.begin();
    dataSize = 0;

    waypointChangedSignal();
}

void ChangeHistoryService::dettach()
{
	clearWaypoints();
	
	itemAddedConnection.disconnect();
	itemRemovedConnection.disconnect();
	itemChangedConnection.disconnect();
	
	if (megaClusterInstance)
	{
		if (megaClusterInstance->isSmooth())
			megaClusterInstance->getSmoothGrid()->disconnectListener(this);
		else
            megaClusterInstance->getVoxelGrid()->disconnectListener(this);

		megaClusterInstance.reset();
	}
}

void ChangeHistoryService::attach()
{
	RBXASSERT(recording==NULL);
	RBXASSERT(waypoints.size()==0);
	recording = new Waypoint(this);
	playWaypoint = waypoints.end();
	unplayWaypoint = waypoints.begin();
	
	// Populate the current state of the world
	dataModel->visitDescendants(boost::bind(&ChangeHistoryService::onItemAdded, this, _1));
	resetBaseWaypoint();
	
	itemAddedConnection = dataModel->onDemandWrite()->descendantAddedSignal.connect(boost::bind(&ChangeHistoryService::onItemAdded, this, _1));
	itemRemovedConnection = dataModel->onDemandWrite()->descendantRemovingSignal.connect(boost::bind(&ChangeHistoryService::onItemRemoved, this, _1));
	itemChangedConnection = dataModel->itemChangedSignal.connect(boost::bind(&ChangeHistoryService::onItemChanged, this, _1, _2));
}

void ChangeHistoryService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		runService.reset();
		runTransitionConnection.disconnect();
		dettach();
		dataModel = NULL;

        if ( statsItem ) 
        {
			statsItem->setParent(NULL);
			statsItem.reset();
		}
	}
	
	Super::onServiceProvider(oldProvider, newProvider);
	
	if (newProvider)
	{
		dataModel = boost::polymorphic_downcast<DataModel*>(newProvider);

        runService = shared_from(ServiceProvider::create<RunService>(newProvider));
        runTransitionConnection = runService->runTransitionSignal.connect(boost::bind(&ChangeHistoryService::onRunTransition, this, _1));

        Stats::StatsService* stats = ServiceProvider::create<Stats::StatsService>(newProvider);
		if (stats) 
        {
			statsItem = ChangeHistoryStatsItem::create(*this);
			statsItem->setParent(stats);
		}
	}
}

ChangeHistoryService::CheckResult ChangeHistoryService::checkSettingWaypoint()
{
	if (!runService || !runService->isRunState())
		return Accept;

	switch (runtimeUndoBehavior)
	{
	default:
		RBXASSERT(false);
		return Accept;
	case Hybrid:
		return Accept; // recording is legal at runtime
	case Aggregate:
		return Reject;	// We don't want to record anything
	case Snapshot:
		return Accept;
	}
}

void ChangeHistoryService::onItemAdded(shared_ptr<Instance> instance)
{
	RBXASSERT(enabled);
	
	if (playing)
		return;
	
	if (!recording)
		return;
	
	if (!isRecordable(instance.get()))
		return;
	
	//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ChangeHistoryService::onItemAdded %s\n", instance->getDescriptor().name.c_str());
	
	Item& item = isReplicatedChange() ? (*waypoints.begin())->getItem(instance) : recording->getItem(instance);
	item.recordCreate();

	shared_ptr<RBX::MegaClusterInstance> cluster = Instance::fastSharedDynamicCast<RBX::MegaClusterInstance>(instance);
	if(cluster)
	{
		RBXASSERT(megaClusterInstance == NULL);
		if (megaClusterInstance == NULL) {
			megaClusterInstance = cluster;

			if (megaClusterInstance->isSmooth())
			{
				Voxel2::Grid* smoothGrid = megaClusterInstance->getSmoothGrid();

				smoothGrid->connectListener(this);

				item.addSmoothClusterData(smoothGrid);
			}
			else
			{
                Voxel::Grid* voxelGrid = megaClusterInstance->getVoxelGrid();

                voxelGrid->connectListener(this);

                item.addClusterData(voxelGrid);
			}
		}
	}
}

void ChangeHistoryService::terrainCellChanged(const Voxel::CellChangeInfo& info)
{
	if (playing || !recording || !megaClusterInstance)
		return;

	Vector3int16 cell(info.position);
	if (isReplicatedChange())
	{
		Waypoints::iterator iter = waypoints.end();
		while (iter != waypoints.begin())
		{
			--iter;

			bool forceUpdate = (iter == waypoints.begin());
			Item* item = forceUpdate ? &(*iter)->getItem(megaClusterInstance) : (*iter)->findItem(megaClusterInstance.get());
			if (item)
			{
				unsigned int cellData;
				SpatialRegion::Id chunkIndex = SpatialRegion::regionContainingVoxel(cell);
				Vector3int16 cellPos = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(cell);
				boost::uint32_t cellIndex = ((cellPos.x) | (cellPos.z << 5) | (cellPos.y << 10));

				// make sure we update the base waypoint if no other waypoint has cluster modifications
				if (forceUpdate || item->getCellData(chunkIndex, cellIndex, cellData))
				{				
					item->updateClusterData(SpatialRegion::regionContainingVoxel(cell), 
							                SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(cell), 
											info.afterCell, info.afterMaterial);
					break;
				}
			}
		}
	}
	else
	{
		recording->getItem(megaClusterInstance).recordClusterData(
			SpatialRegion::regionContainingVoxel(cell), SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(cell), info.afterCell, info.afterMaterial);
	}
}

void ChangeHistoryService::onTerrainRegionChanged(const Voxel2::Region& region)
{
	if (playing || !recording || !megaClusterInstance)
		return;

	if (isReplicatedChange())
	{
		std::vector<Vector3int32> chunkIds = region.getChunkIds(Item::kChunkSizeLog2);
		for (const auto& chunkId: chunkIds)
		{			
			Waypoints::iterator iter = waypoints.end();
			while (iter != waypoints.begin())
			{
				--iter;
				bool forceUpdate = (iter == waypoints.begin());
				Item* item = forceUpdate ? &(*iter)->getItem(megaClusterInstance) : (*iter)->findItem(megaClusterInstance.get());
				if (item)
				{
					// make sure we update the base waypoint if no other waypoint has cluster modifications
					if (forceUpdate || (item->smoothClusterCells.find(chunkId) != item->smoothClusterCells.end()))
					{
						item->updateSmoothClusterData(chunkId, megaClusterInstance->getSmoothGrid());	
						break;
					}
				}
			}
		}
	}
	else
	{
		recording->getItem(megaClusterInstance).addSmoothClusterRegion(megaClusterInstance->getSmoothGrid(), region, /* ignoreEmpty= */ false);
	}
}

void ChangeHistoryService::setCell(const SpatialRegion::Id& chunkPos, const Vector3int16& cellInChunk,
		Voxel::Cell detail, Voxel::CellMaterial material)
{
	megaClusterInstance->getVoxelGrid()->setCell(SpatialRegion::globalVoxelCoordinateFromRegionAndRelativeCoordinate(chunkPos, cellInChunk), detail, material);
}

bool ChangeHistoryService::isRecordable(Instance* instance)
{
	// Accept Joints, even in a non-serializable service:
	if (instance->fastDynamicCast<JointInstance>())
		return true;
	
	// Ignore selection:
	if (dynamic_cast<ISelectionBase*>(instance))
		return false;
	
	// Non-serializable objects shouldn't be recorded for undo/redo
	// This includes most services
	const ClassDescriptor& d(instance->getDescriptor());
	bool b = d.isSerializable();
	if (!b)
		return false;
	
	Instance* parent = instance->getParent();
	
	if (!parent)
		return false;	// we're done
	
	if (parent == dataModel)
		return true;	// we're done
	
	// Walk up the tree
	return isRecordable(parent);
}

void ChangeHistoryService::onItemRemoved(shared_ptr<Instance> instance)
{
	RBXASSERT(enabled);
	
	if (playing)
		return;
	
	if (!recording)
		return;
	
	if (!isRecordable(instance.get()))
		return;
	
	if (isReplicatedChange())
	{
		Waypoints::iterator iter = waypoints.begin();
		while (iter != waypoints.end())
		{
			(*iter)->removeItem(instance.get());
			if ((*iter)->isEmpty())
			{
				if (iter == unplayWaypoint)
					--unplayWaypoint;
				if (iter == playWaypoint)
					++playWaypoint;
				iter = waypoints.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
	else if (const PropertyDescriptor* scriptSourceProp = getScriptSourceDescriptorIfScript(instance))
    {
		// if we're deleting a script, save its contents first.
	    //  onItemChanged does not record script changes.
		//  we don't want there to be an undo of script changes on the stack because
		//  when undoing, the user wouldn't see any changes since the editor is not open.
		//  so instead, we store the current source code and restore it on undo.

        Item& item = recording->getItem(instance);
        item.recordProperty(scriptSourceProp);
        item.parent.reset();
        item.action = Item::Delete;
    }
    else
    {
	    recording->getItem(instance).recordDelete();
    }

	shared_ptr<RBX::MegaClusterInstance> cluster = Instance::fastSharedDynamicCast<RBX::MegaClusterInstance>(instance);
	if(cluster)
	{
		RBXASSERT(megaClusterInstance != NULL);
		RBXASSERT(megaClusterInstance == cluster);

		if (megaClusterInstance->isSmooth())
            megaClusterInstance->getSmoothGrid()->disconnectListener(this);
		else
            megaClusterInstance->getVoxelGrid()->disconnectListener(this);

		megaClusterInstance.reset();
	}
}

void ChangeHistoryService::onItemChanged(shared_ptr<Instance> instance, const PropertyDescriptor* descriptor)
{
	RBXASSERT(enabled);
	
	if (playing)
		return;
	
	if (!recording)
		return;

	if (!isRecordable(instance.get()))
		return;

    // Ignore script changes to the source code (script windows have their own undo/redo)
	if (getScriptSourceDescriptorIfScript(instance) == descriptor)
		return;

	// support recording of only name change and parent change for camera
	if ( instance->fastDynamicCast<Camera>() && (*descriptor != RBX::Instance::desc_Name) && (*descriptor != Instance::propParent) )
		return;

	if (isReplicatedChange())
	{
		Waypoints::iterator iter = waypoints.end();
		while (iter!=waypoints.begin())
		{
			--iter;
			Item* item = (*iter)->findItem(instance.get());
			if (item && item->properties.find(descriptor) != item->properties.end())
			{
				item->recordProperty(descriptor);
				break;
			}
		}
	}
	else
	{
		recording->getItem(instance).recordProperty(descriptor);
	}
}

void ChangeHistoryService::Waypoint::play()
{
    std::vector<const Item*> itemsOrdered = getItemsOrdered();

	for (size_t i = 0; i < itemsOrdered.size(); ++i)
    {
		try
        {
            itemsOrdered[i]->play();
        }
        catch (RBX::base_exception& ex)
        {
            RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, ex.what());
        }
    }

	selectModifiedParts(true);
}

void ChangeHistoryService::Waypoint::unplay()
{
    std::vector<const Item*> itemsOrdered = getItemsOrdered();

	for (size_t i = itemsOrdered.size(); i > 0; --i)
    {
		try
		{
			itemsOrdered[i-1]->unplay();
		}
		catch (RBX::base_exception& ex)
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, ex.what());
		}
    }

    // This is a hack because the CFrame property can be constrained by the physics engine.
    // To get around the fact that CFrame changes might be temporarily ignored, we try setting
    // them again here. If in the future we rework things so that there is a non-constrained
    // CFrame property then we can eliminate this ugly hack
    for (size_t i = 0; i < itemsOrdered.size(); ++i)
    {
        try
        {
            itemsOrdered[i]->unplay_CFrame();
        }
        catch (RBX::base_exception& ex)
        {
            RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, ex.what());
        }
    }
	
	selectModifiedParts(false);
}

void ChangeHistoryService::Waypoint::selectModifiedParts(bool isPlay)
{
	Workspace* pWorkspace = Workspace::findWorkspace(changeHistory);
	if (pWorkspace)
	{
		RBX::Selection* pSelection = RBX::ServiceProvider::create<RBX::Selection>(pWorkspace);
		if (pSelection)
		{
			 //TODO: Remove this extra for loop and have a single loop, once we remove the FastFlag
			const DataModel *pDataModel = Instance::fastDynamicCast<DataModel>(ServiceProvider::findServiceProvider(pWorkspace));

			boost::unordered_set<shared_ptr<RBX::Instance> > instancesToSelect;
			boost::unordered_set<RBX::Instance*> instancesParent;
			for (Items::iterator iter=items.begin(); iter!=items.end(); ++iter)
			{
                const Item& item = iter->second;
				if ( (Instance::fastDynamicCast<PartInstance>(item.instance.get()) || Instance::fastDynamicCast<ModelInstance>(item.instance.get()))
					&& ((item.action == Item::Change) || item.action == (isPlay ? Item::Create : Item::Delete)) )	
				{
					instancesToSelect.insert(item.instance);
					//make sure we do not add "Workspace", "StarterGui" etc, add only the top level items under these
					if ( item.instance->getParent() && (item.instance->getParent()->getParent() != pDataModel) )
						instancesParent.insert(item.instance->getParent());
				}
			}

			//filter out child instances
			shared_ptr<Instances> instances(new Instances);
			for (boost::unordered_set<shared_ptr<RBX::Instance> >::iterator iter = instancesToSelect.begin(); iter != instancesToSelect.end(); ++iter)
				if (instancesParent.find((*iter)->getParent()) == instancesParent.end())
					instances->push_back(*iter);

			if (!instances->empty())
				pSelection->setSelection(instances);
		}
	}
}

void ChangeHistoryService::play()
{
    RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
    playLua();
}

void ChangeHistoryService::playLua()
{
	if (playing)
		throw std::runtime_error("ChangeHistoryService is currently playing");
	
	if (!enabled)
		throw std::runtime_error("ChangeHistoryService is disabled");
	
	if (playWaypoint==waypoints.end())
		throw std::runtime_error("Attempt to play beyond change history");
	
	ScopedAssign<bool> assign(playing, true);
	
	RBXASSERT(playWaypoint!=waypoints.begin());

    if (recording)
    {
        recording->unplay();

        delete recording;
        recording = new Waypoint(this);
    }

	(*playWaypoint)->play();

    std::string name = (*playWaypoint)->name;
	
	unplayWaypoint = playWaypoint;
	++playWaypoint;

    waypointChangedSignal();
    redoSignal(name);
}

void ChangeHistoryService::unplay()
{
	RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
	unplayLua();
}

void ChangeHistoryService::unplayLua()
{
	if (playing)
		throw std::runtime_error("ChangeHistoryService is currently playing");
	
	if (!enabled)
		throw std::runtime_error("ChangeHistoryService is disabled");
	
	if (unplayWaypoint==waypoints.begin())
		throw std::runtime_error("Attempt to unplay before the change history");

	ScopedAssign<bool> assign(playing, true);

    if (recording)
    {
        (*unplayWaypoint)->absorb(recording);

        delete recording;
        recording = new Waypoint(this);
    }
	
	(*unplayWaypoint)->unplay();

    std::string name = (*unplayWaypoint)->name;

	playWaypoint = unplayWaypoint;
	--unplayWaypoint;

    waypointChangedSignal();
    undoSignal(name);
}

void RBX::ChangeHistoryService::reportMissedPhysicsChanges(shared_ptr<RBX::Instance> instance)
{
	RBXASSERT(enabled);
	RBXASSERT(!playing);
	RBXASSERT(recording);

	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get()))
	{
		RBXASSERT(isRecordable(instance.get()));

		Waypoints::iterator waypoint = unplayWaypoint;
		RBXASSERT(waypoint != waypoints.end());

		bool foundCFrame = false;
		bool foundLinearVelocity = false;
		bool foundRotationalVelocity = false;
		while (true) 
		{
			if (Item* item = (*waypoint)->findItem(instance.get()))
			{
				if (!foundCFrame)
				{
					Item::Properties::const_iterator iter2 = item->properties.find(&PartInstance::prop_CFrame);
					if (iter2 != item->properties.end())
					{
						const RBX::CoordinateFrame& v = iter2->second.cast<RBX::CoordinateFrame>();
						if (v != part->getCoordinateFrame())
							recording->getItem(instance).recordProperty(&PartInstance::prop_CFrame);
						foundCFrame = true;
					}
				}
				if (!foundLinearVelocity)
				{
					Item::Properties::const_iterator iter2 = item->properties.find(&PartInstance::prop_Velocity);
					if (iter2 != item->properties.end())
					{
						const RBX::Vector3& v = iter2->second.cast<RBX::Vector3>();
						if (v != part->getLinearVelocity())
							recording->getItem(instance).recordProperty(&PartInstance::prop_Velocity);
						foundLinearVelocity = true;
					}
				}
				if (!foundRotationalVelocity)
				{
					Item::Properties::const_iterator iter2 = item->properties.find(&PartInstance::prop_RotVelocity);
					if (iter2 != item->properties.end())
					{
						const RBX::Vector3& v = iter2->second.cast<RBX::Vector3>();
						if (v != part->getRotationalVelocity())
							recording->getItem(instance).recordProperty(&PartInstance::prop_RotVelocity);
						foundRotationalVelocity = true;
					}
				}

				if (foundCFrame && foundLinearVelocity && foundRotationalVelocity)
					break;
			}


			if (waypoint == waypoints.begin())
			{
				// nothing to report
				break;
			}
			--waypoint;
		}
	}

	instance->visitChildren(boost::bind(&ChangeHistoryService::reportMissedPhysicsChanges, this, _1));
}

void ChangeHistoryService::setRunWaypoint()
{
    if (runtimeUndoBehavior != Snapshot || !runService || !runService->isRunState())
        reportMissedPhysicsChanges(shared_from(dataModel->getWorkspace()));
	
	setWaypoint("Run");
}

bool ChangeHistoryService::isResetEnabled() const
{
	return isEnabled() && runStartWaypoint != waypoints.end();
}

void ChangeHistoryService::reset()
{
	// For now RS_STOPPED means somebody called "reset", which is slightly different than a true "stop".
	// Eventually we'll need to differentiate between Stop and Reset
	RBXASSERT(canUnplay());
	RBXASSERT(runStartWaypoint != waypoints.end());
	while (unplayWaypoint != runStartWaypoint)
		unplay();
	runStartWaypoint = waypoints.end();
}

void ChangeHistoryService::onRunTransition( RunTransition event )
{
	if (!isEnabled())
		return;

	switch (event.newState)
	{
	case RS_STOPPED:
	case RS_PAUSED:
		setRunWaypoint();
		break;

	case RS_RUNNING:
		if (event.oldState == RS_STOPPED)
		{
			runStartWaypoint = unplayWaypoint;
			RBXASSERT(runStartWaypoint != waypoints.end());
		}
		break;
	}
}
