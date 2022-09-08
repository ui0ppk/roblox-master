#pragma once

#include "V8World/BasicSpatialHashPrimitive.h"
#include "Util/G3DCore.h"
#include "Util/Memory.h"
#include "Util/ConcurrencyValidator.h"
#include "rbx/Debug.h"
#include "rbx/object_pool.h"

#include <boost/unordered_set.hpp>
#include <boost/pool/pool.hpp>

namespace RBX {



	class World;
	class Extents;

	class NodeBase
	{
	public:
		NodeBase(short level, int hashId, const Vector3int32& gridId)
		: level(level)
		, hashId(hashId)
		, gridId(gridId)
		{};
		NodeBase()
		: level(-1)
		, hashId(-1)
		{};
		
		~NodeBase() {
			level = -2;
			hashId = -2;
		}
		
		short level;
		int hashId;
		Vector3int32 gridId;
		
		int getLevel() {
			RBXASSERT(level >= -1);
			return level; 
		}
	};
	
	enum enumAction
	{
		aRecurseTreeNode,
		aVisitSingleSpatialNode,
		aVisitAllSiblingsSpatialNodes
	};
	
	struct NodeInfo
	{
		
		NodeInfo(NodeBase* node, enumAction action, IntersectResult intersectResult, float distance)
		: node(node)
		, action(action)
		, intersectResult(intersectResult)
		, distance(distance)
		{};
		NodeBase* node;
		enumAction action;
		IntersectResult intersectResult;
		float distance;
		
		// transform distance into priority (lower distance, higher priority == invert sign)
		bool operator < (const NodeInfo& r) const
		{
			return distance > r.distance;
		}
	};
	
	
	class SpatialHashStatic {
	public:
		// in SpatialHashMultiRes.inl
		static const int cellMinSize;
		static const int maxLevelForAnchored;
		inline static float hashGridSize(int level);
		inline static float hashGridRecip(int level);
		inline static size_t numBuckets(int level);
		inline static Extents hashGridToRealExtents(int level, const Vector3int32& hashGrid);
		inline static ExtentsInt32 scaleExtents(int smallLevel, int bigLevel, const ExtentsInt32& smallExtents);
		inline static Vector3int32 realToHashGrid(int level, const Vector3& realPoint);
		inline static Vector3 hashGridToReal(int level, const Vector3int32& hashGrid);
		
		// in SpatialHashMultiRes.cpp
		static int getHash(int level, const Vector3int32& grid);
		static void computeMinMax(const int level, const Extents& extents, Vector3int32& min, Vector3int32& max);
		static void makeVisitOrder(int* offsets, const Vector3& visitDir);

		static const Extents safeExtents(const Extents& e) {
			RBXASSERT(Extents(e.min(), e.max()) == e);
			if (e.isNanInf()) {
				RBXASSERT(0);
				return Extents::zero();
			}
			else {
				return e;
			}
		}
	};

	template<class Primitive, class Contact, class ContactManager, int MAX_LEVELS>
	class SpatialHash {	
	public:
		struct SpaceFilter
		{
			virtual IntersectResult Intersects(const Extents& extents) = 0;
			virtual float Distance(const Extents& extents) { return 0; };
			// return false to break iteration.
			virtual bool onPrimitive(Primitive* p, IntersectResult intersectResult, float distance) = 0;
		};

		// Implement this pure virtual class in order to listen for coarse
		// movement events. See registerCoarseMovementCallback for more details.
		class CoarseMovementCallback {
		public:
			struct UpdateInfo {
				enum UpdateType {
					UPDATE_TYPE_Insert = 0,
					UPDATE_TYPE_Change,
					MAX_UPDATE_TYPES
				};

				// for Insert update type, only new{Level,SpatialExtents} are valid
				// for Changed update type, both old and new info is valid
				UpdateType updateType;

				int oldLevel;
				ExtentsInt32 oldSpatialExtents;
				int newLevel;
				ExtentsInt32 newSpatialExtents;
			};
			// This callback may be invoked when a part is altered from lua,
			// or other sensitive areas. Implementers of this method should
			// avoid modifying the parts of Primitive relating to its extents
			// and/or location to avoid re-entrant behavior and unexpected
			// interactions.
			virtual void coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info) = 0;
		};

		SpatialHash(World* world, ContactManager* contactManager, int maxCellsPerPrimitive);
		~SpatialHash();

		// loosely sorted.
		void visitPrimitivesInSpace(SpaceFilter* filter, const Vector3& visitDir);
		// strict sorting of nodes according the the return value of filter->Distance().
		void visitPrimitivesInSpace(SpaceFilter* filter);

		void fastClear();

		void onPrimitiveAdded(Primitive* p, bool addContact = true);
		void onPrimitiveRemoved(Primitive* p);
		void onPrimitiveExtentsChanged(Primitive* p);
		void onPrimitiveAssembled(Primitive* p);
	
		void getPrimitivesInGrid(const Vector3int32& grid, G3D::Array<Primitive*>& primitives);
		bool getNextGrid(Vector3int32& grid, const RbxRay& unitRay, float maxDistance);

		// find all primitives that touch the same grids as touched by extents
		void getPrimitivesTouchingGrids(
			const Extents& extents,
			const Primitive* ingore,
			std::size_t maxCount, 
			boost::unordered_set<Primitive*>& answer);

		void getPrimitivesTouchingGrids(
			const Extents& extents, 
			const boost::unordered_set<const Primitive*>& ignoreSet, 
			std::size_t maxCount, 
			boost::unordered_set<Primitive*>& answer);

		// This function iteratively processes cells that overlap with extents, which is faster on small regions
		template <typename Set> void getPrimitivesOverlapping( const Extents& extents, Set& answer);

		// This function recursively processes cells that overlap with extents, which is faster on large regions
		template <typename Set> void getPrimitivesOverlappingRec(const Extents& extents, Set& answer);

		// inquiry
		int	getNodesOut() const {return nodesOut;}
		int getMaxBucket() const {return maxBucket;}

		void doStats() const;

		// Callback mechanism that allows outside systems to be notified when
		// parts exibit significant movement (relative to their size). A
		// movement is considered significant if it causes the part to enter
		// or leave a region of space, where region size determined by
		// primitive size. The size of the coarsest region is
		// SpatialHashStatic::hashGridSize(MAXLEVELS - 1) so callers cannot
		// depend on this callback being fired for smaller region movements,
		// but in practice the majority of primitives use smaller regions.
		void registerCoarseMovementCallback(CoarseMovementCallback* callback);
		void unregisterCoarseMovementCallback(CoarseMovementCallback* callback);

	private:
		class TreeNode;
		class SpatialNode;
		typedef std::pair<TreeNode*, IntersectResult> TreeNodePair;

		// sort by the dot product of( the position of the offsets in space by the visitDir ).
		struct SortOffsetByVisitDir
		{
			SortOffsetByVisitDir(const Vector3& visitDir) 
			: visitDir(visitDir) {};
			const Vector3& visitDir;
			
			bool operator()(const TreeNodePair& a, const TreeNodePair& b)
			{
				Vector3 va((float)a.first->gridId.x, (float)a.first->gridId.y, (float)a.first->gridId.z);
				Vector3 vb((float)b.first->gridId.x, (float)b.first->gridId.y, (float)b.first->gridId.z);
				return va.dot(visitDir) < vb.dot(visitDir);
			}
		};
		
		struct FastClearSpatialNode
		{
			SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>* hash;
			FastClearSpatialNode(SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>* h) : hash(h) {};
			void operator()(SpatialNode* node)
			{
				node->primitive->setOldSpatialExtents(ExtentsInt32::empty());
				hash->nodesOut--;
			}
		};

		struct FastClearTreeNode
		{
			SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>* hash;
			FastClearTreeNode(SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>* h) : hash(h) {};
			void operator()(TreeNode* node)
			{
				node->refByPrimitives = 0;
				node->next = 0;
				hash->numTreeNodesTotal--;
			}
		};

		class TreeNode : protected NodeBase, public Allocator<TreeNode> {
		protected:
			friend class SpatialHash;
			friend class SpatialNode;

			unsigned short children[8];
			unsigned char childMask;
			int refByPrimitives;
			TreeNode *next;

			void reset() {
				refByPrimitives = 0;
				this->level = -1;
				this->hashId = -1;
				next = NULL;
				childMask = 0;
				for (int i=0; i<8; i++)
					children[i] = 0xffff;
			}

			void setChild(int i, unsigned int child) {
				childMask |=  (1<<i);
				children[i] = child;
			}

			void removeChild(int i) {
				childMask &= ~(1<<i);
				children[i] = 0xffff;
			}

		public:
			TreeNode() {
				reset();
			}

			~TreeNode() {
				RBXASSERT(refByPrimitives == 0);
				RBXASSERT(next == NULL);
				next = NULL;
			}

			unsigned char hasChild(int i) {
				return childMask & (1<<i);
			}

		};


		class SpatialNode : protected NodeBase, public Allocator<SpatialNode> {
		protected:
			friend class SpatialHash;
			friend class TreeNode;

			Primitive*			primitive;			// primitive associated with this node
			SpatialNode*		nextHashLink;		// next node for this hash
	#ifdef _RBX_DEBUGGING_SPATIAL_HASH
			SpatialNode*		nextPrimitiveLink;	// next node for this primitive
			SpatialNode*		prevPrimitiveLink;	// prior node for this primitive
	#endif
			TreeNode *treeNode;

		public:
			SpatialNode(int l, int hashId, const Vector3int32& gridId)
				: NodeBase(l, hashId, gridId)
				, nextHashLink(0)
				, primitive(NULL)
				, treeNode(NULL)
			#ifdef _RBX_DEBUGGING_SPATIAL_HASH
				, nextPrimitiveLink(0)
				, prevPrimitiveLink(0)
			#endif
			{}

			~SpatialNode()
			{
				treeNode = NULL;
				primitive = NULL;
				nextHashLink = NULL;
			}
		};

		class SpatialHashTableEntry {
		public:
			SpatialNode *nodes;
			TreeNode *treeNodes;
		};

	protected: // default settings override on construction
		static const int rootLevel;

	private:
		ConcurrencyValidator concurrencyValidator;

		const int maxCellsPerPrimitive;

		int numTreeNodesTotal;

		World*								world;
		ContactManager*						contactManager;
		std::vector<SpatialHashTableEntry>	hashTables[MAX_LEVELS];
		int									nodesOut;
		int									maxBucket;
		G3D::Array<Primitive*>				outOfContact;		// temp buffer
		std::vector<CoarseMovementCallback*> coarseMovementCallbacks;

		SpatialNode* newNode(int level, int hash, const Vector3int32& grid);
		void returnNode(SpatialNode* node);

		TreeNode * findTreeNode(
			int level, int hash, const Vector3int32 &gridCoord);
		TreeNode * createTreeNode(
			int level, int hash, const Vector3int32 &gridCoord);
		void _retireTreeNode(TreeNode* tn);
		void retireTreeNode(TreeNode* tn);
		void removeTreeNodeChild(int childLevel, Vector3int32 &childGridCoord);

		bool findOtherNodesInLevel0Cell(SpatialNode* destroy);

		void checkAndReleaseContacts(Primitive *p);

		void addContactFromChildren(TreeNode *tn, Primitive *p);

		int computeLevel(const Primitive* p, const Extents& extents);

		inline bool oldExtentsOverlap(Primitive* p0, Primitive* p1);

		bool hashHasPrimitive(int level, Primitive* p, int hash, const Vector3int32& grid);

		SpatialNode* findNode(Primitive* p, const Vector3int32& grid);
		void removeNodeFromHash(SpatialNode* remove);

		void insertNodeToPrimitive(SpatialNode* node, Primitive* p, const Vector3int32& grid, int hash);

		void addNode(Primitive* p, const Vector3int32& grid, bool addContact = true);
		void destroyNode(SpatialNode* destroy);

		void changeMinMax(	Primitive* p,
							const ExtentsInt32* change,
							const ExtentsInt32* oldBox,
							const ExtentsInt32* newBox,
							bool addContact = true);
		void primitiveAdded(Primitive* p, bool addContact);
		void primitiveRemoved(Primitive* p);
		void primitiveExtentsChanged(Primitive* p, const Extents& extents);

		// remove these once we can confirm "boost::pool" objects work
		object_pool<TreeNode, roblox_allocator> treeNodeAllocator;
		object_pool<SpatialNode, roblox_allocator> spatialNodeAllocator;
		//

		inline Vector3int32 getChildGrid(const Vector3int32& grid, int offset) 
		{
			return Vector3int32(
				(grid.x << 1) + (offset & 1), // bit 0 of offset is x coord.
				(grid.y << 1) + ((offset & 2) >> 1), // bit 1 of offset is y coord.
				(grid.z << 1) + ((offset & 4) >> 2)  // bit 2 of offset is z coord.
				);
		}

		static const Extents calcNewExtents(Primitive* p);

		void visitPrimitivesInSpaceWorker(TreeNode* tn, int level, int hashId, const RBX::Vector3int32& gridId, int* visitOrder, IntersectResult intersectResult, SpaceFilter* filter, const Vector3& visitDir);

		template <typename Set> void getPrimitivesOverlappingRec(const Extents* extents, Set& answer, int level, int hash, const Vector3int32& gridCoord);

	private:
		void setup();
		void cleanup();

		void getPrimitivesInGrid(int level, const Vector3int32& grid, G3D::Array<Primitive*>& primitives);

		// octree interface
		TreeNode* getFirstRoot();
		TreeNode* getNextRoot(TreeNode* prevRoot);

		TreeNode* getChild(TreeNode* parent, int octant);
		void getPrimitivesInTreeNode(TreeNode* treenode, G3D::Array<Primitive*>& primitives);

	public: // for unit testing

		// DEBUGGING only
		bool validateInsertNodeToPrimitive(SpatialNode* node, Primitive* p, const Vector3int32& grid, int hash);
		bool validateRemoveNodeFromPrimitive(SpatialNode* node);
		bool validateNodesOverlap(Primitive* p0, Primitive* p1);
		bool validateTallyTreeNodes();
		bool validateTreeNodeNotHere(TreeNode* tn, int level, int hash);
		bool validateContacts(Primitive* p);	// debug only
		bool validateNoNodesOut();
	};

}	// namespace

#include "v8World/SpatialHashMultiRes.inl"
