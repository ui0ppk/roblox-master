#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "Util/Math.h"
#include "rbx/Debug.h"
#include "RbxAssert.h"
#include "G3D/CollisionDetection.h"

#include <vector>

namespace RBX {

float SpatialHashStatic::hashGridSize(int level)	{return (float)(cellMinSize << level);}
float SpatialHashStatic::hashGridRecip(int level)	{return 1.0f / SpatialHashStatic::hashGridSize(level);}
size_t SpatialHashStatic::numBuckets(int level)		{return 65536;}

Vector3int32 SpatialHashStatic::realToHashGrid(int level, const Vector3& realPoint)
{
	Vector3 gridPoint = realPoint * SpatialHashStatic::hashGridRecip(level);
	Vector3int32 hashGrid = Vector3int32::floor(gridPoint);		// 4 grids per hash bucket
	return hashGrid;
}

ExtentsInt32 SpatialHashStatic::scaleExtents(int smallLevel, int bigLevel, const ExtentsInt32& smallExtents)
{
	RBXASSERT_SLOW(smallLevel < bigLevel);
	int delta = bigLevel - smallLevel;
	return smallExtents.shiftRight(delta);
}

Extents SpatialHashStatic::hashGridToRealExtents(int level, const Vector3int32& hashGrid)
{
	Extents answer(	hashGridToReal(level, hashGrid),	
					hashGridToReal(level, hashGrid + Vector3int32::one())
					);

	RBXASSERT_VERY_FAST(Math::isIntegerVector3(answer.min()));
	RBXASSERT_VERY_FAST(Math::isIntegerVector3(answer.max()));

	return answer;
}


Vector3 SpatialHashStatic::hashGridToReal(int level, const Vector3int32& hashGrid)
{
	return hashGrid.toVector3() * SpatialHashStatic::hashGridSize(level);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#define SHP template<class Primitive, class Contact, class ContactManager, int MAX_LEVELS>

// statics - templated SpatialHash
SHP const int SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::rootLevel = MAX_LEVELS-1;

SHP SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::SpatialHash(World* world, ContactManager* contactManager, int maxCellsPerPrimitive) 
	: world(world)
	, contactManager(contactManager)
	, maxCellsPerPrimitive(maxCellsPerPrimitive)
	, nodesOut(0)
	, maxBucket(0)
	, numTreeNodesTotal(0)
{
	setup();
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::setup()
{
	for (int i=0; i<MAX_LEVELS; i++) {
		hashTables[i].resize(SpatialHashStatic::numBuckets(i));
		memset(&hashTables[i][0], 0, sizeof(hashTables[i][0])*hashTables[i].size());
	}
}

SHP SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::~SpatialHash()
{
	cleanup();
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::cleanup() 
{
	// nb: make all checks match with fastClear as well.
	RBXASSERT_SPATIAL_HASH(validateTallyTreeNodes());
	RBXASSERT_SPATIAL_HASH(validateNoNodesOut());
	RBXASSERT(nodesOut == 0);
}

SHP const Extents SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::calcNewExtents(Primitive* p) 
{
	return SpatialHashStatic::safeExtents(p->getFastFuzzyExtents());
}



SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::SpatialNode* SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::newNode(int level, int hash, const Vector3int32& grid)
{
	SpatialNode* answer;
    answer = new SpatialNode(level, hash, grid);
	
	if (level == 0) {
		answer->treeNode = NULL;
	}
	else {
		answer->treeNode = findTreeNode(level, hash, grid);
		if (! answer->treeNode)
			answer->treeNode = createTreeNode(level, hash, grid);
		answer->treeNode->refByPrimitives++;

		RBXASSERT(level == answer->treeNode->level);
	}

	return answer;
}



SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::TreeNode *SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::findTreeNode(
	int level, int hash, const Vector3int32 &gridCoord)
{
	TreeNode * tn = hashTables[level][hash].treeNodes;
	while (tn) {
		RBXASSERT(tn->hashId == hash);
		RBXASSERT(tn->level == level);
		if (tn->gridId == gridCoord) {
			return tn;
		}
		tn = tn->next;
	}
	return NULL;
}

SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::TreeNode *SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::createTreeNode(
	int level, int hash, const Vector3int32 &gridCoord)
{
	TreeNode *tn;

    tn = new TreeNode();
	
	numTreeNodesTotal ++;

	tn->refByPrimitives = 0;
	tn->level = level;
	tn->hashId = hash;
	tn->gridId = gridCoord;

	RBXASSERT_SPATIAL_HASH(validateTreeNodeNotHere(tn, level, hash));

	tn->next = hashTables[level][hash].treeNodes;
	hashTables[level][hash].treeNodes = tn;

	return tn;
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::_retireTreeNode(TreeNode* tn) {

	TreeNode** p = &hashTables[tn->level][tn->hashId].treeNodes;
	while (*p != tn) {
		p = &((*p)->next);
	}
	*p = tn->next;
	tn->next = NULL;		

    delete tn;
	
	numTreeNodesTotal --;
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::retireTreeNode(TreeNode* tn)
{
	if (--tn->refByPrimitives) {
		// still being used, do nothing
		return;
	}

	if (tn->childMask) {
		// This tree node still has children, i.e. it's part of the tree.
		// Therefore, even though no SpatialNode is using this treenode (which
		// in turn means this treenode corresponds to no primitives), it
		// needs to be kept.
		return;
	}

	// this treenode's refcount is 0 AND has no children 
	//retire it and adjust hierarchy

	int tnlevel = tn->level;
	Vector3int32 tngridId(tn->gridId);
	_retireTreeNode(tn);

	// notify ancestors that this has been removed
	removeTreeNodeChild(tnlevel, tngridId);
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::insertNodeToPrimitive(SpatialNode* node, 
										Primitive* p, 
										const Vector3int32& grid, 
										int hash)
{
	RBXASSERT(node->level == p->getSpatialNodeLevel());

	node->primitive = p;
	node->gridId = grid;
	node->hashId = hash;
	
	RBXASSERT_SPATIAL_HASH(validateInsertNodeToPrimitive(node, p, grid, hash));
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::returnNode(SpatialNode* node)
{
    delete node;
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::removeNodeFromHash(SpatialNode* remove)
{
	SpatialNode** nodePtr = &hashTables[remove->getLevel()][remove->hashId].nodes;

	while (*nodePtr != remove) {
		nodePtr = &((*nodePtr)->nextHashLink);
	}
	*nodePtr = remove->nextHashLink;
}

SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::SpatialNode* SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::findNode(Primitive* p, const Vector3int32& grid)
{
	const int l = p->getSpatialNodeLevel();

	RBXASSERT_SPATIAL_HASH(static_cast<SpatialNode*>(p->spatialNodes)->getLevel() == l);
	RBXASSERT(SpatialHashStatic::numBuckets(l) == hashTables[l].size());

	int hash = SpatialHashStatic::getHash(l, grid);
	SpatialNode* node = hashTables[l][hash].nodes;
	if (node == NULL)
		return NULL;
	while ((node->primitive != p) || (node->gridId != grid)) {
		node = node->nextHashLink;
		if (! node) {
			break;
		}
	}

	return node;
}


SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::oldExtentsOverlap(Primitive* me, Primitive* other)
{
	const int myLevel = me->getSpatialNodeLevel();
	const int otherLevel = other->getSpatialNodeLevel();
	RBXASSERT(myLevel >= 0);
	RBXASSERT(otherLevel >= 0);

	bool answer = false;

	if (myLevel == otherLevel) {
		answer = ExtentsInt32::overlapsOrTouches(me->getOldSpatialExtents(), other->getOldSpatialExtents());
	} else {
		Primitive* big = (myLevel > otherLevel) ? me : other;
		Primitive* smallPrim = (big == me) ? other : me;
		ExtentsInt32 smallInBig = SpatialHashStatic::scaleExtents(smallPrim->getSpatialNodeLevel(), big->getSpatialNodeLevel(), smallPrim->getOldSpatialExtents());
		answer = ExtentsInt32::overlapsOrTouches(big->getOldSpatialExtents(), smallInBig);
	}

	return answer;
}

SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::findOtherNodesInLevel0Cell(SpatialNode* destroy)
{
	RBXASSERT(destroy->getLevel() == 0);
	Vector3int32 g = destroy->gridId;
	int hash = destroy->hashId;

	SpatialNode* node_sameHash = hashTables[0][hash].nodes;
	while (node_sameHash) {
		if (node_sameHash->gridId == g) 
		{
#ifdef _DEBUG
			Primitive* other = node_sameHash->primitive;
			RBXASSERT(other != destroy->primitive);
#endif
			return true;
		}
		node_sameHash = node_sameHash->nextHashLink;
	}
	return false;
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::destroyNode(SpatialNode* destroy)
{
	RBXASSERT_SPATIAL_HASH(validateTallyTreeNodes());
	RBXASSERT_SPATIAL_HASH(validateRemoveNodeFromPrimitive(destroy));
	RBXASSERT_SPATIAL_HASH(destroy->getLevel() == destroy->primitive->getSpatialNodeLevel());

	// This is insignificant for timing
	removeNodeFromHash(destroy);

	// for all lower level nodes
	if (destroy->getLevel() > 0) 
	{
		RBXASSERT(destroy->treeNode);
		retireTreeNode(destroy->treeNode);
	} 
	else 
	{
		if (!findOtherNodesInLevel0Cell(destroy)) {		// this (conceptual) leaf cell is empty; maintain hierachy
			removeTreeNodeChild(0, destroy->gridId);
		}
	}

	returnNode(destroy);
	nodesOut--;

	RBXASSERT_SPATIAL_HASH(validateTallyTreeNodes());
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::removeTreeNodeChild(int childLevel, Vector3int32 &childGridCoord)
{
	int childHash;
	RBXASSERT((childHash = SpatialHashStatic::getHash(childLevel, childGridCoord), 1));

	for (int l=childLevel+1; l < MAX_LEVELS; l++) {
		Vector3int32 g;
		g.x = childGridCoord.x>>(l-childLevel); g.y = childGridCoord.y>>(l-childLevel); g.z = childGridCoord.z>>(l-childLevel);
		int hash = SpatialHashStatic::getHash(l, g);

		TreeNode *tn = findTreeNode(l, hash, g);
		RBXASSERT(tn);
		int offset = ((childGridCoord.x>>(l-childLevel-1)) & 1)+
			(((childGridCoord.y>>(l-childLevel-1)) & 1)<<1) + (((childGridCoord.z>>(l-childLevel-1)) & 1) << 2);
		RBXASSERT(tn->children[offset] == childHash);
		// remove child
		tn->removeChild(offset);

		// if there's still children
		if (tn->refByPrimitives==0 && !tn->childMask) {
			// this means the child that we've just removed is the only
			// reason why this treenode existed. Now it's removed
			_retireTreeNode(tn); 
		} else {
			// if this treenode is not deleted, no need to check parent
			break;
		}
		childHash = hash;
	}
}

SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::hashHasPrimitive(int level, Primitive* p, int hash, const Vector3int32& grid)
{
	SpatialNode* test = hashTables[level][hash].nodes;
	while (test) {
		if ((test->primitive == p) && (test->gridId == grid)) {
			return true;
		}
		test = test->nextHashLink;
	}
	return false;
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::addContactFromChildren(TreeNode *tn, Primitive *p)
{

	unsigned short *children = tn->children;
	Vector3int32 baseGrid;
	baseGrid.x = tn->gridId.x << 1;
	baseGrid.y = tn->gridId.y << 1;
	baseGrid.z = tn->gridId.z << 1;
	for (int i=0; i<8; i++) {
		if (tn->hasChild(i)) {
			Vector3int32 g;
			g.x = baseGrid.x + (i & 1);
			g.y = baseGrid.y + ((i & 2)>>1);
			g.z = baseGrid.z + ((i & 4)>>2);
			RBXASSERT(tn->level - 1 >= 0);
			SpatialNode *tryNode = hashTables[ tn->level - 1 ][ children[i] ].nodes;
			while (tryNode) {
				Primitive* other = tryNode->primitive;
				if ((other != p) && (tryNode->gridId == g )) {
					if (Primitive::getContact(p, other) == NULL) 
                    {
						contactManager->onNewPair(p, other);
					}
				}
				else {
					RBXASSERT(g != tryNode->gridId);
				}
				tryNode = tryNode->nextHashLink;
			}

			//recursive
			if (tn->level > 1) {
				TreeNode *c = findTreeNode(tn->level - 1, children[i], g);
				RBXASSERT(c);
				addContactFromChildren(c, p);
			}
		}
	}
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::addNode(Primitive* p, const Vector3int32& grid, bool addContact)
{
	RBXASSERT(p->getSpatialNodeLevel() != -1);

	int level = p->getSpatialNodeLevel();

	int hash = SpatialHashStatic::getHash(level, grid);

	RBXASSERT_SPATIAL_HASH(validateTallyTreeNodes());
	RBXASSERT_VERY_FAST(!hashHasPrimitive(level, p, hash, grid));

	nodesOut++;
	SpatialNode* addedNode = newNode(level, hash, grid);

#ifdef _DEBUG
	if (level > 0) {
		TreeNode *tn_save = addedNode->treeNode;
		RBXASSERT(addedNode->getLevel() == tn_save->level);
	}
#endif

	// 1. Put in the primitive's linked list
	insertNodeToPrimitive(addedNode, p, grid, hash);

	// 2. This hash's linked list of nodes
	SpatialNode* tryNode = hashTables[level][hash].nodes;
	addedNode->nextHashLink = tryNode;
	hashTables[level][hash].nodes = addedNode;

	// 3. Cycle through the pre-existing nodes - see if any are hit
	int numNodes = 1;	// start with 1 - this one

	// For nodes at the same level, and higher levels nodes containing this node
	Vector3int32 g = grid;
	int prevHash = -1;
	bool needHiearchyUpdate = true;

	for (int l=level; l < MAX_LEVELS; l++) {
		if (l > level) {
			g.x >>= 1; g.y >>= 1; g.z >>= 1;
			hash = SpatialHashStatic::getHash(l, g);
			tryNode = hashTables[l][hash].nodes;
		}

		if (Primitive::hasGetFirstContact && addContact)
		{
			while (tryNode) {
				RBXASSERT( l < MAX_LEVELS);

				numNodes++;
				Primitive* other = tryNode->primitive;
                if ((other != p) && (tryNode->gridId == g)) {
					if (Primitive::getContact(p, other) == NULL)
                    {
				        contactManager->onNewPair(p, other);
					}
				}
				else {
					RBXASSERT(g != tryNode->gridId);
				}
				tryNode = tryNode->nextHashLink;
			}
		}

		// maintain hiearchy information
		if (l > level && needHiearchyUpdate) {
			TreeNode *tn = findTreeNode(l, hash, g);
			if (!tn)
				tn = createTreeNode(l, hash, g);
			int offset = ((grid.x>>(l-level-1)) & 1)+
				(((grid.y>>(l-level-1)) & 1)<<1) + (((grid.z>>(l-level-1)) & 1) << 2);
			RBXASSERT(0<=offset && offset<=7 && prevHash >= 0);
			// establish child "pointer"
			RBXASSERT(!tn->hasChild(offset) || tn->children[offset]==prevHash);
			
			if (tn->hasChild(offset))
				needHiearchyUpdate = false; // this and upper levels tree-nodes has already been set
			else
				tn->setChild(offset, prevHash);

			if (level > 0)
			{
				RBXASSERT(addedNode->treeNode && addedNode->getLevel() == addedNode->treeNode->level);
			}
		}

		// save immediately-lower-level hashId
		prevHash = hash;
	}

	// add contacts from children
	if (Primitive::hasGetFirstContact && addContact && addedNode->getLevel() > 0) {
		RBXASSERT(addedNode && addedNode->treeNode->level == addedNode->getLevel());
		addContactFromChildren(addedNode->treeNode, p);
	}

	maxBucket = std::max(maxBucket, numNodes);

	RBXASSERT_SPATIAL_HASH(validateTallyTreeNodes());
}



SHP int SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::computeLevel(const Primitive* p, const Extents& extents)
{
	const float extra = static_cast<float>(SpatialHashStatic::cellMinSize * 2.0);		// extra buffer for thin objects
	Vector3 size = extents.size() + Vector3(extra, extra, extra);
	float volume = size.x * size.y * size.z;

	int maxLevel =  p->requestFixed() ? SpatialHashStatic::maxLevelForAnchored : MAX_LEVELS - 1;

	float maxVolumeThisLevel = static_cast<float>((SpatialHashStatic::cellMinSize * SpatialHashStatic::cellMinSize * SpatialHashStatic::cellMinSize)* maxCellsPerPrimitive);

	for (int answerLevel = 0; answerLevel < maxLevel; ++answerLevel)
	{
		if (volume < maxVolumeThisLevel) { 
			return answerLevel;
		}
		maxVolumeThisLevel *= 8;
	}
	return maxLevel;
}

/*
SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::computeMinMax(const Primitive* p, const Extents& extents, Vector3int32& min, Vector3int32& max)
{
	RBXASSERT(p->getSpatialNodeLevel() != -1);
	computeMinMax(p->getSpatialNodeLevel(), extents, min, max);
}
*/

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::onPrimitiveAdded(Primitive* p, bool addContact) {
	if( contactManager->primitiveIsExcludedFromSpatialHash(p) )
        return;

	primitiveAdded(p, addContact);

	if (!coarseMovementCallbacks.empty()) {
		typename CoarseMovementCallback::UpdateInfo info;
		info.updateType = CoarseMovementCallback::UpdateInfo::UPDATE_TYPE_Insert;
		
		info.newLevel = p->getSpatialNodeLevel();
		info.newSpatialExtents = p->getOldSpatialExtents();

		for (size_t i = 0; i < coarseMovementCallbacks.size(); ++i) {
			coarseMovementCallbacks[i]->coarsePrimitiveMovement(p, info);
		}
	}
}

// Note -use FastFuzzyExtents when not in the middle of a simulation step
// Use FastFuzzyExtentsNoCompute when in a step
SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::primitiveAdded(Primitive* p, bool addContact)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT_SPATIAL_HASH(validateContacts(p));
	RBXASSERT_SPATIAL_HASH(p->spatialNodes == NULL);
	RBXASSERT(p->getSpatialNodeLevel() == -1);

	Extents newExtentsFloat = calcNewExtents(p);
	int level = computeLevel(p, newExtentsFloat);

	Vector3int32 newMin, newMax;
	SpatialHashStatic::computeMinMax(level, newExtentsFloat, newMin, newMax);

	p->setSpatialNodeLevel(level);
	ExtentsInt32 newExtents(newMin, newMax);
	p->setOldSpatialExtents(newExtents);

	changeMinMax(p, &newExtents, NULL, &newExtents, addContact);

	RBXASSERT(p->getSpatialNodeLevel() == level);
	RBXASSERT_SPATIAL_HASH(validateContacts(p));
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::changeMinMax(	Primitive* p,
																					const ExtentsInt32* change,
																					const ExtentsInt32* oldBox,
																					const ExtentsInt32* newBox,
																					bool addContact)
{
	bool newEqualsChange = (newBox == change);
	bool oldEqualsChange = (oldBox == change);

	for (int i = change->low.x; i <= change->high.x; ++i) {
	for (int j = change->low.y; j <= change->high.y; ++j) {
	for (int k = change->low.z; k <= change->high.z; ++k) {
		Vector3int32 v(i, j, k);
		const bool inNew = newEqualsChange || (newBox && newBox->contains(v));
		const bool inOld = oldEqualsChange || (oldBox && oldBox->contains(v));
        if (inNew && !inOld) {
			addNode(p, v, addContact);				// only update bucket counts when moving to avoid 
		}										// big counts around 0,0,0
		else if (inOld && !inNew) {
			SpatialNode* destroyMe = findNode(p, v);
			RBXASSERT(destroyMe);
			destroyNode(destroyMe);
		}
	}
	}
	}
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::fastClear()
{
	FastClearSpatialNode fastClearSpatialNode(this);
	FastClearTreeNode fastClearTreeNode(this);

	for (int level = 0; level < MAX_LEVELS; level ++) 
	{
		for (int hashId=0; hashId<(int)SpatialHashStatic::numBuckets(level); hashId++) 
		{
			SpatialNode *sn = hashTables[level][hashId].nodes;
			while (sn) 
			{
				SpatialNode *nodeToFree = sn;
				sn = nodeToFree->nextHashLink;
				fastClearSpatialNode(nodeToFree);
                delete nodeToFree;
			}

			TreeNode *tn = hashTables[level][hashId].treeNodes;
			while (tn) 
			{
				TreeNode *nodeToFree = tn;
				tn = nodeToFree->next;
				RBXASSERT((fastClearTreeNode(nodeToFree), true)); // only run this if asserts are on.
                delete nodeToFree;
			};
		}
	}

    Allocator<SpatialNode>::releaseMemory();
    Allocator<TreeNode>::releaseMemory();

	maxBucket = 0;

	cleanup();

	setup();
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::checkAndReleaseContacts(Primitive *p)
{
	if (Primitive::hasGetFirstContact )
	{
		outOfContact.clear();

		Contact * contact = p->getFirstContact();
		while (contact) {
			Primitive *other = contact->otherPrimitive(p);
			contact = p->getNextContact(contact);
            if( contactManager->primitiveIsExcludedFromSpatialHash(other) )
                    continue;
			bool overlap = oldExtentsOverlap(p, other);
			RBXASSERT_SPATIAL_HASH(overlap == validateNodesOverlap(p, other));
			if (!overlap) {
				outOfContact.append(other);
			}
		}
		for (int i=0; i<(int)outOfContact.size(); i++)
			contactManager->releasePair(p, outOfContact[i]);
	}
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::primitiveExtentsChanged(Primitive* p, const Extents& extents)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT_SPATIAL_HASH(validateContacts(p));
	RBXASSERT_SPATIAL_HASH(p->spatialNodes != NULL);
	RBXASSERT(p->getSpatialNodeLevel() != -1);

	Vector3int32 newMin, newMax;

	// For now, never change a primitive's level in the hierarchy, once it
	// has been determined at creation time
	SpatialHashStatic::computeMinMax(p->getSpatialNodeLevel(), extents, newMin, newMax);

	if (	(newMin == p->getOldSpatialMin()) 
		&&	(newMax == p->getOldSpatialMax())) {
		return;
	}

	ExtentsInt32 oldBox(p->getOldSpatialExtents());
	ExtentsInt32 newBox(newMin, newMax);
	p->setOldSpatialExtents(newBox);

	if (ExtentsInt32::overlapsOrTouches(oldBox, newBox)) {
		ExtentsInt32 unionBox = ExtentsInt32::unionExtents(oldBox, newBox);
		changeMinMax(p, &unionBox, &oldBox, &newBox);
	}
	else {
		changeMinMax(p, &oldBox, &oldBox, NULL);
		RBXASSERT_SPATIAL_HASH(p->spatialNodeCount == 0);
		changeMinMax(p, &newBox, NULL, &newBox);
	}

	checkAndReleaseContacts(p);

	RBXASSERT_SPATIAL_HASH(validateContacts(p));
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::onPrimitiveRemoved(Primitive* p) {
	primitiveRemoved(p);
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::primitiveRemoved(Primitive* p)
{
	WriteValidator writeValidator(concurrencyValidator);
	RBXASSERT_SPATIAL_HASH(validateContacts(p));
	RBXASSERT(p->getSpatialNodeLevel() != -1 || contactManager->primitiveIsExcludedFromSpatialHash(p));

	changeMinMax(p, &p->getOldSpatialExtents(), &p->getOldSpatialExtents(), NULL);
	p->setSpatialNodeLevel(-1); 
	p->setOldSpatialExtents(ExtentsInt32::empty());
	RBXASSERT_SPATIAL_HASH(p->spatialNodeCount == 0);
	
	if (Primitive::hasGetFirstContact )
	{
		outOfContact.clear();
		
		for (int i = 0; i < p->getNumContacts(); i++)
			outOfContact.append(p->getContactOther(i));
		
		for (int i = 0; i < (int)outOfContact.size(); i++)
			contactManager->releasePair(p, outOfContact[i]);
	}

	RBXASSERT_SPATIAL_HASH(validateContacts(p));
}

// Note -use FastFuzzyExtents when not in the middle of a simulation step
// Use FastFuzzyExtentsNoCompute when in a step
SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::onPrimitiveExtentsChanged(Primitive* p)
{
    if( contactManager->primitiveIsExcludedFromSpatialHash(p) )
        return;

    contactManager->checkTerrainContact(p);

	Extents newExtentsFloat = calcNewExtents(p);
	int newLevel = computeLevel(p, newExtentsFloat);
	int oldLevel = p->getSpatialNodeLevel();
	int delta = newLevel - oldLevel;
	ExtentsInt32 preUpdateSpatialExtents = p->getOldSpatialExtents();

	if ((delta > 0) || (delta < -1))		// grow always, shrink only if 2 steps down
	{
		primitiveRemoved(p);
		primitiveAdded(p, true);
	}
	else 
	{
		primitiveExtentsChanged(p, newExtentsFloat);
	}

	ExtentsInt32 postUpdateSpatialExtents = p->getOldSpatialExtents();
	if (!coarseMovementCallbacks.empty() &&
			(newLevel != oldLevel || preUpdateSpatialExtents != postUpdateSpatialExtents)) {
		typename CoarseMovementCallback::UpdateInfo info;
		info.updateType = CoarseMovementCallback::UpdateInfo::UPDATE_TYPE_Change;
		info.oldLevel = oldLevel;
		info.oldSpatialExtents = preUpdateSpatialExtents;
		info.newLevel = newLevel;
		info.newSpatialExtents = postUpdateSpatialExtents;

		for (size_t i = 0; i < coarseMovementCallbacks.size(); ++i) {
			coarseMovementCallbacks[i]->coarsePrimitiveMovement(p, info);
		}
	}
}

// Now that primitives are assembled into mechanisms we know the full topology to filter
// internal contacts.  So let's query the spatial hash to create the contacts and rely on
// onNewPair() to do the filtering

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::onPrimitiveAssembled(Primitive* p)
{
	if( contactManager->primitiveIsExcludedFromSpatialHash(p) )
		return;

	RBXASSERT_SPATIAL_HASH(validateContacts(p));
	RBXASSERT_SPATIAL_HASH(p->spatialNodes != NULL);
	RBXASSERT(p->getSpatialNodeLevel() != -1);

	WriteValidator writeValidator(concurrencyValidator);

	Extents newExtents = calcNewExtents(p);
	int level = computeLevel(p, newExtents);
	Vector3int32 newMin, newMax;
	SpatialHashStatic::computeMinMax(level, newExtents, newMin, newMax);

	for (int i = newMin.x; i <= newMax.x; ++i) {
		for (int j = newMin.y; j <= newMax.y; ++j) {
			for (int k = newMin.z; k <= newMax.z; ++k) {
				Vector3int32 grid(i, j, k);				
				SpatialNode* thisNode = findNode(p, grid);
				if (!thisNode)
					continue;

				int hash = SpatialHashStatic::getHash(level, grid);

				// This hash's linked list of nodes
 				SpatialNode* tryNode = hashTables[level][hash].nodes;

				// For nodes at the same level, and higher levels nodes containing this node
				Vector3int32 g = grid;

				for (int l=level; l < MAX_LEVELS; l++) {
					if (l > level) {
						g.x >>= 1; g.y >>= 1; g.z >>= 1;
						hash = SpatialHashStatic::getHash(l, g);
						tryNode = hashTables[l][hash].nodes;
					}

					while (tryNode) {
						RBXASSERT( l < MAX_LEVELS);

						Primitive* other = tryNode->primitive;
						if ((other != p) && (tryNode->gridId == g)) {
							if (Primitive::getContact(p, other) == NULL)
							{
								contactManager->onNewPair(p, other);
							}
						}			
						tryNode = tryNode->nextHashLink;
					}
				}

				// add contacts from children
				if (thisNode->getLevel() > 0) {
					RBXASSERT(thisNode && thisNode->treeNode->level == thisNode->getLevel());
					addContactFromChildren(thisNode->treeNode, p);
				}
			}
		}
	}
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesTouchingGrids(const Extents& extents, 
											   const Primitive* ignore, 
											   std::size_t maxCount, 
											   boost::unordered_set<Primitive*>& answer)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	RBXASSERT(answer.size() == 0);


	Vector3int32 min, max;
	G3D::Array<Primitive*> foundThisGrid;		// prevent allocations
	
	for (int level=0; level<MAX_LEVELS; level++) {
		if (! hashTables[level].size())
			continue;

		SpatialHashStatic::computeMinMax(level, extents, min, max);

		for (int i = min.x; i <= max.x; ++i) {
			for (int j = min.y; j <= max.y; ++j) {
				for (int k = min.z; k <= max.z; ++k) {
					foundThisGrid.fastClear();
					getPrimitivesInGrid(level, Vector3int32(i, j, k), foundThisGrid);

					for (int ff = 0; ff < foundThisGrid.size(); ++ff) {
						Primitive* p = foundThisGrid[ff];
						if (	(p != ignore) 
							&&	(answer.find(p) == answer.end())
							&&	(extents.overlapsOrTouches(calcNewExtents(p))))
						{
							answer.insert(p);

							//Break out early if we've hit our limit
							if(maxCount > 0 && answer.size() >= maxCount)
								return;
						}
					}
				}
			}
		}
	}
	RBXASSERT(foundThisGrid.size() < 200);
}

// same as above function, but we use a set of all primitives to-be-ignored for use with an ancestor check
SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesTouchingGrids(const Extents& extents, 
																								 const boost::unordered_set<const Primitive*>& ignoreSet, 
											   std::size_t maxCount, 
											   boost::unordered_set<Primitive*>& answer)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	RBXASSERT(answer.size() == 0);


	Vector3int32 min, max;
	G3D::Array<Primitive*> foundThisGrid;		// prevent allocations
	
	for (int level=0; level<MAX_LEVELS; level++) {
		if (! hashTables[level].size())
			continue;

		SpatialHashStatic::computeMinMax(level, extents, min, max);

		for (int i = min.x; i <= max.x; ++i) {
			for (int j = min.y; j <= max.y; ++j) {
				for (int k = min.z; k <= max.z; ++k) {
					foundThisGrid.fastClear();
					getPrimitivesInGrid(level, Vector3int32(i, j, k), foundThisGrid);

					for (int ff = 0; ff < foundThisGrid.size(); ++ff) {
						Primitive* p = foundThisGrid[ff];
						if (	
							(answer.find(p) == answer.end())
							&&	(extents.overlapsOrTouches(calcNewExtents(p))))
						{
							// ancestor check here
							if ( ignoreSet.find(p) == ignoreSet.end() ) {
								answer.insert(p);

								//Break out early if we've hit our limit
								if(maxCount > 0 && answer.size() >= maxCount)
									return;
							}
						}
					}
				}
			}
		}
	}
	RBXASSERT(foundThisGrid.size() < 200);
}

SHP template <typename Set> void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesOverlapping(const Extents& extents, Set& answer)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	for (int level=0; level<MAX_LEVELS; level++) {
		if (!hashTables[level].size())
			continue;
		
		// A small negative offset is added to extentsMax before computeMinMax to prevent querying extra layer of cells for perfectly aligned/sized extents
		Extents adjustedExtents(extents.min(), (extents.max() - Vector3(0.01f, 0.01f, 0.01f)).max(extents.min()));

		Vector3int32 min, max;
		SpatialHashStatic::computeMinMax(level, adjustedExtents, min, max);

		Vector3 minReal = SpatialHashStatic::hashGridToReal(level, min);
		Vector3 maxReal = SpatialHashStatic::hashGridToReal(level, max + Vector3int32(1, 1, 1));
		
		bool skipOverlapTest = extents.contains(minReal) && extents.contains(maxReal);

		for (int i = min.x; i <= max.x; ++i) {
			for (int j = min.y; j <= max.y; ++j) {
				for (int k = min.z; k <= max.z; ++k) {
					Vector3int32 grid(i, j, k);
					int hash = SpatialHashStatic::getHash(level, grid);

					SpatialNode* node = hashTables[level][hash].nodes;

					while (node) {
						if (node->gridId == grid) {
							Primitive* p = node->primitive;

							if (skipOverlapTest || extents.overlapsOrTouches(calcNewExtents(p))) {
								answer.insert(p);
							}
						}
						node = node->nextHashLink;
					}
				}
			}
		}
	}
}

SHP template <typename Set> void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesOverlappingRec(const Extents& extents, Set& answer)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

    // A small negative offset is added to extentsMax before computeMinMax to prevent querying extra layer of cells for perfectly aligned/sized extents
    Extents adjustedExtents(extents.min(), (extents.max() - Vector3(0.01f, 0.01f, 0.01f)).max(extents.min()));

	int level = MAX_LEVELS - 1;

    Vector3int32 min, max;
    SpatialHashStatic::computeMinMax(level, adjustedExtents, min, max);

	// If the tested region is contained within the extents we don't have to perform precise overlap tests
	// This is very important since tests require reading Primitive memory which leads to extra cache misses.
	Vector3 minReal = SpatialHashStatic::hashGridToReal(level, min);
	Vector3 maxReal = SpatialHashStatic::hashGridToReal(level, max + Vector3int32(1, 1, 1));

	bool skipOverlapTest = extents.contains(minReal) && extents.contains(maxReal);

    for (int i = min.x; i <= max.x; ++i) {
        for (int j = min.y; j <= max.y; ++j) {
            for (int k = min.z; k <= max.z; ++k) {
                Vector3int32 grid(i, j, k);
                int hash = SpatialHashStatic::getHash(level, grid);

				getPrimitivesOverlappingRec(skipOverlapTest ? NULL : &extents, answer, level, hash, grid);
            }
        }
    }
}

SHP template <typename Set> void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesOverlappingRec(const Extents* extents, Set& answer, int level, int hash, const Vector3int32& gridCoord)
{
	// Look for nodes at current level
    SpatialNode* node = hashTables[level][hash].nodes;

    while (node)
	{
        if (node->gridId == gridCoord)
		{
            Primitive* p = node->primitive;

            if (!extents || extents->overlapsOrTouches(calcNewExtents(p)))
                answer.insert(p);
        }

        node = node->nextHashLink;
    }

	// Look for nodes at the next level with smaller cells
	if (level > 0)
    {
        TreeNode* treeNode = hashTables[level][hash].treeNodes;

        while (treeNode)
        {
            if (treeNode->gridId == gridCoord)
            {
                for (int child = 0; child < 8; ++child)
                    if (treeNode->hasChild(child))
                    {
						int childHash = treeNode->children[child];
						Vector3int32 childGrid = getChildGrid(gridCoord, child);

                        getPrimitivesOverlappingRec(extents, answer, level - 1, childHash, childGrid);
                    }

				// Just one tree node for every cell, no need to look further
                return;
            }

            treeNode = treeNode->next;
        }
    }
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesInGrid(int level, const Vector3int32& grid, G3D::Array<Primitive*>& found)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	int hash = SpatialHashStatic::getHash(level, grid);

	SpatialNode* node = hashTables[level][hash].nodes;

	while (node) {
		if (node->gridId == grid) {
			RBXASSERT_IF_VALIDATING(!found.contains(node->primitive));
			found.append(node->primitive);
		}
		node = node->nextHashLink;
	}
}

// find primitives at all levels--the input grid coord is for level 0
SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesInGrid(const Vector3int32& grid, G3D::Array<Primitive*>& found)
{
	RBXASSERT(found.size() == 0);

	Vector3int32 g = grid;

	for (int level = 0; level < MAX_LEVELS; 
		level++, g.x>>=1, g.y>>=1, g.z>>=1) {
		getPrimitivesInGrid(level, g, found);
	}
}


SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getNextGrid(Vector3int32& grid, 
							  const RbxRay& unitRay, 
							  float maxDistance)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	RBXASSERT_VERY_FAST(unitRay.direction().isUnit());

	int low[3], high[3];
	for (int i = 0; i < 3; ++i) {
		low[i] =  (unitRay.direction()[i] < 0.0) ? -1 : 0;
		high[i] = (unitRay.direction()[i] > 0.0) ?  1 : 0;
	}

	maxDistance += SpatialHashStatic::hashGridSize(0) * 2.0f;	// collision detection returns the first hit in a grid box - this could
											// be farther away than the actual ultimate hit point
	float maxDistanceSquared = maxDistance * maxDistance;

	for (int nz = 1; nz <= 3; ++ nz) {			// number of non zeros - 1: adjacent face (6), edge (12), corner (8)
	for (int i = low[0]; i <= high[0]; ++i) {
	for (int j = low[1]; j <= high[1]; ++j) {
	for (int k = low[2]; k <= high[2]; ++k) {
		if ((std::abs(i) + std::abs(j) + std::abs(k)) == nz) {	// start with this number of nonzeros
			Vector3int32 offset(i, j, k);
			Extents extents = SpatialHashStatic::hashGridToRealExtents(0, grid + offset);
			AABox box(extents.min(), extents.max());
			Vector3 location;
			bool inside;
			bool result = G3D::CollisionDetection::collisionLocationForMovingPointFixedAABox( unitRay.origin(), unitRay.direction(), box, location, inside );
			if( inside || result )
			{
				if ((location - unitRay.origin()).squaredMagnitude() < maxDistanceSquared) {
					grid = grid + offset;
					return true;
				}
			}
		}
	}}}}
	return false;
}


SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::TreeNode* SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getFirstRoot()
{
	TreeNode * tn = NULL;
	for(size_t id = 0; id < SpatialHashStatic::numBuckets(rootLevel) && tn == NULL; id++)
	{
		tn = hashTables[rootLevel][id].treeNodes;
	}
	return tn;
}

SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::TreeNode* SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getNextRoot(TreeNode* prevRoot)
{
	if(prevRoot)
	{
		if(prevRoot->next)
		{
			return prevRoot->next;
		}
		else
		{
			// go to next non-empty hash grid
			TreeNode * tn = NULL;
			for(size_t id = prevRoot->hashId+1; id < SpatialHashStatic::numBuckets(rootLevel) && tn == NULL; id++)
			{
				tn = hashTables[rootLevel][id].treeNodes;
			}
			return tn;
		}
	}
	else
	{
		return NULL;
	}

}

SHP typename SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::TreeNode* SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getChild(TreeNode* parent, int octant)
{
	RBXASSERT(parent->hasChild(octant));
	if(parent->level > 0)
	{
		return findTreeNode(parent->level-1, parent->children[octant], getChildGrid(parent->gridId, octant));
	}
	else
	{
		return NULL;
	}
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::getPrimitivesInTreeNode(TreeNode* tn, G3D::Array<Primitive*>& primitives)
{
	SpatialNode* node = hashTables[tn->level][tn->hashId].nodes;

	while (node) {
		if (node->gridId == tn->gridId) {
			RBXASSERT_IF_VALIDATING(!primitives.contains(node->primitive));
			primitives.append(node->primitive);
		}
		node = node->nextHashLink;
	}
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::visitPrimitivesInSpace(SpaceFilter* filter, const Vector3& visitDir)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	int visitOrder[8];
	SpatialHashStatic::makeVisitOrder(visitOrder, visitDir);

	typedef std::pair<TreeNode*, IntersectResult> TreeNodePair;
	std::vector<TreeNodePair> roots;

	// get all the root nodes, sort by visitDir. (after testing for intersect)
	for(TreeNode* root = getFirstRoot(); root; root = getNextRoot(root))
	{
		IntersectResult childIntersect = filter->Intersects(SpatialHashStatic::hashGridToRealExtents(root->level, root->gridId));

		if(childIntersect == irNone)
		{
			continue; // no interesct at all from this node and all child nodes.
		}
		roots.push_back(TreeNodePair(root, childIntersect));
	}

	if(visitDir != Vector3::zero())
	{
		SortOffsetByVisitDir sortPred(visitDir);

		std::sort(roots.begin(), roots.end(), sortPred);
	}

	for(size_t i = 0; i < roots.size(); ++i)
	{
		TreeNode* tn = roots[i].first;
		visitPrimitivesInSpaceWorker(tn, tn->level, tn->hashId, tn->gridId, visitOrder, roots[i].second /*childintersect*/, filter, visitDir);
	}
	
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::visitPrimitivesInSpace(SpaceFilter* filter)
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	

	std::priority_queue<NodeInfo> nodestovisit;

	bool bContinueIterating = true; // becomes false when onPrimitives returns false

	// get all the root nodes, sort by visitDir. (after testing for intersect)
	for(TreeNode* root = getFirstRoot(); root; root = getNextRoot(root))
	{
		Extents extents = SpatialHashStatic::hashGridToRealExtents(root->level, root->gridId);
		IntersectResult childIntersect = filter->Intersects(extents);
		if(childIntersect == irNone)
		{
			continue; // no interesct at all from this node and all child nodes.
		}
		nodestovisit.push(NodeInfo(root, aRecurseTreeNode, childIntersect, filter->Distance(extents)));
	}

	while(!nodestovisit.empty() && bContinueIterating)
	{
		NodeInfo nodeinfo(nodestovisit.top());
		nodestovisit.pop();
		NodeBase* node = nodeinfo.node; 
		int level = node->level;
		int hashId = node->hashId;
		const RBX::Vector3int32& gridId = node->gridId;

		// check the children
		if(nodeinfo.action == aRecurseTreeNode)
		{
			// all pushed NodeBases are TreeNode if they are not level 0.
			TreeNode* tn = static_cast<TreeNode*>(node);

			int childLevel = level-1;
			for(int childoffset = 0; childoffset < 8; ++childoffset)
			{
				if(!tn->hasChild(childoffset))
				{
					continue; // no child, skip.
				}

				Vector3int32 childGridId = getChildGrid(tn->gridId, childoffset);
				int childHashId = tn->children[childoffset];

				IntersectResult childIntersect = irFull;
				Extents childExtents = SpatialHashStatic::hashGridToRealExtents(childLevel, childGridId);
				if(nodeinfo.intersectResult == irPartial) // must keep checking bounds
				{
					childIntersect = filter->Intersects(childExtents);
					if(childIntersect == irNone)
					{
						continue; // no interesct at all from this node and all child nodes.
					}
				}

				TreeNode* childTreeNode = NULL;
				if(childLevel > 0)
				{
					childTreeNode = findTreeNode(childLevel, childHashId, childGridId);
					RBXASSERT(childTreeNode);
					RBXASSERT(childTreeNode->gridId == childGridId);
					RBXASSERT(childTreeNode->level == childLevel);
					RBXASSERT(tn->children[childoffset] == childTreeNode->hashId);

					// we have childTreeNode;
					nodestovisit.push(NodeInfo(childTreeNode, aRecurseTreeNode, childIntersect, filter->Distance(childExtents)));
				}
				else // since we don't have treenodes at the leaf level, we must "reach down" from the above level, instead of "recursing".
				{
					// no childTreeNode, must pass a spatialNode;
					SpatialNode* snode = hashTables[childLevel][childHashId].nodes;

					bool extentsNeedCalc = true;
					float extentsDistance = 0;

					while (snode) {
						if (snode->gridId == childGridId) 
						{
							if(extentsNeedCalc) // only calculate distance for this extents 0 or 1 times.
							{
								extentsDistance = filter->Distance(childExtents);
							}
#ifndef PRECISE_SORTING
							nodestovisit.push(NodeInfo(snode, aVisitAllSiblingsSpatialNodes, childIntersect, extentsDistance));
							break;
#else
							// precise sorting.
							nodestovisit.push(NodeInfo(snode, aVisitSingleSpatialNode, childIntersect, extentsDistance)));
#endif
						}
						snode = snode->nextHashLink;
					}

				}

			}
		}

		if (nodeinfo.action != aVisitSingleSpatialNode )
		{
			// this treenode could have sibling spatial nodes.
			SpatialNode* snode = hashTables[level][hashId].nodes;
			while (snode && bContinueIterating) {
				if (snode->gridId == gridId) 
				{
#ifndef PRECISE_SORTING
						bContinueIterating = filter->onPrimitive(snode->primitive, nodeinfo.intersectResult, nodeinfo.distance);
#else
						nodestovisit.push(NodeInfo(snode, aVisitSingleSpatialNode, nodeinfo.intersectResult, filter->Distance(snode->primitive->getFastFuzzyExtents())));
#endif
				}
				snode = snode->nextHashLink;
			}
		}
		else //if (nodeinfo.action == aVisitSingleSpatialNode)
		{
			bContinueIterating = filter->onPrimitive(static_cast<SpatialNode*>(node)->primitive, nodeinfo.intersectResult, nodeinfo.distance);
		}
	}
	
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::visitPrimitivesInSpaceWorker(TreeNode* tn, int level, int hashId, const RBX::Vector3int32& gridId, int* visitOrder, IntersectResult intersectResult, SpaceFilter* filter, const Vector3& visitDir)
{
	RBXASSERT(intersectResult == irFull || intersectResult == irPartial);

	// check the children
	if(tn)
	{
		int childLevel = level-1;
		for(int offseti = 0; offseti < 8; ++offseti)
		{
			int childoffset = visitOrder[offseti];
			if(!tn->hasChild(childoffset))
			{
				continue; // no child, skip.
			}

			Vector3int32 childGridId = getChildGrid(tn->gridId, childoffset);
			int childHashId = tn->children[childoffset];

			IntersectResult childIntersect = irFull;
			if(intersectResult == irPartial) // must keep checking bounds
			{
				childIntersect = filter->Intersects(SpatialHashStatic::hashGridToRealExtents(childLevel, childGridId));
				if(childIntersect == irNone)
				{
					continue; // no interesct at all from this node and all child nodes.
				}
			}

			TreeNode* childTreeNode = NULL;
			if(childLevel > 0)
			{
				childTreeNode = findTreeNode(childLevel, childHashId, childGridId);
				RBXASSERT(childTreeNode);
				RBXASSERT(childTreeNode->gridId == childGridId);
				RBXASSERT(childTreeNode->level == childLevel);
				RBXASSERT(tn->children[childoffset] == childTreeNode->hashId);

			}

			visitPrimitivesInSpaceWorker(childTreeNode, childLevel, childHashId, childGridId, visitOrder, childIntersect, filter, visitDir);
		}
	}

	// visit the primitives 
	SpatialNode* node = hashTables[level][hashId].nodes;

	bool bContinue = true;
	while (node && bContinue) {
		if (node->gridId == gridId) {
			bContinue = filter->onPrimitive(node->primitive, intersectResult, 0);
		}

		node = node->nextHashLink;
	}
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//
//  Debugging - all of these should RBXASSERT(assertingSpatialHash)
//
SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateInsertNodeToPrimitive(SpatialNode* node, 
																									Primitive* p, 
																									const Vector3int32& grid, 
																									int hash)
{
	RBXASSERT(assertingSpatialHash);

#ifdef _RBX_DEBUGGING_SPATIAL_HASH
	// Primitive P's linked list of nodes
	SpatialNode* oldFirst = static_cast<SpatialNode*>(p->spatialNodes);
	p->spatialNodes = node;

	node->nextPrimitiveLink = oldFirst;
	node->prevPrimitiveLink = NULL;
	if (oldFirst) {
		oldFirst->prevPrimitiveLink = node;
	}
	p->spatialNodeCount++;
#endif
	return true;
}

SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateRemoveNodeFromPrimitive(SpatialNode* node)
{
	RBXASSERT(assertingSpatialHash);

#ifdef _RBX_DEBUGGING_SPATIAL_HASH
	SpatialNode* prev = node->prevPrimitiveLink;
	SpatialNode* next = node->nextPrimitiveLink;

	if (next) {
		next->prevPrimitiveLink = prev;
	}
	if (prev) {
		prev->nextPrimitiveLink = next;
	}
	else {	// !prev
		node->primitive->spatialNodes = next;
	}
	--node->primitive->spatialNodeCount;		
#endif

	return true;
}


SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateNodesOverlap(Primitive* me, Primitive* other)
{
	RBXASSERT(assertingSpatialHash);

#ifdef _RBX_DEBUGGING_SPATIAL_HASH

	SpatialNode* myNode = static_cast<SpatialNode*>(me->spatialNodes);
	SpatialNode* otherNode = static_cast<SpatialNode*>(other->spatialNodes);
	
	if (!myNode || !otherNode)
		return false;

	if (myNode->getLevel() == otherNode->getLevel()) {
		while (myNode) {
			SpatialNode* hashNode = hashTables[myNode->getLevel()][myNode->hashId].nodes;
			while (hashNode) {
				if ((hashNode->primitive == other) && (hashNode->gridId == myNode->gridId)) {
					return true;
				}
				hashNode = hashNode->nextHashLink;
			}
			myNode = myNode->nextPrimitiveLink;
		}
	} else {
		SpatialNode *lower, *higher;
		Primitive *primHigher;

		if (myNode->getLevel() > otherNode->getLevel()) {
			lower = otherNode;
			higher = myNode;
			primHigher = me;
		} else {
			lower = myNode;
			higher = otherNode;
			primHigher = other;
		}
		

		int levelDiff = higher->getLevel() - lower->getLevel();
		while (lower) {
			Vector3int32 gridCoordHigher;
			gridCoordHigher.x = (lower->gridId.x >> levelDiff);
			gridCoordHigher.y = (lower->gridId.y >> levelDiff);
			gridCoordHigher.z = (lower->gridId.z >> levelDiff);

			int hashHigher = getHash(higher->getLevel(), gridCoordHigher);

			SpatialNode* node = hashTables[higher->getLevel()][hashHigher].nodes;
			while (node) {
				if ((node->primitive == primHigher) && (node->gridId == gridCoordHigher)) {
					return true;
				}
				node = node->nextHashLink;
			}
			lower = lower->nextPrimitiveLink;
		}
	}
#endif

	return false;
}


SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateTallyTreeNodes()
{
	RBXASSERT(assertingSpatialHash);

	int numTreeNodesInUse=0, numTreeNodesInPool=0;
	for (int level = 0; level < MAX_LEVELS; level ++) {
		for (int hashId=0; hashId<(int)SpatialHashStatic::numBuckets(level); hashId++) {
			TreeNode *tn = hashTables[level][hashId].treeNodes;
			while (tn) {
				numTreeNodesInUse ++;
				RBXASSERT(tn->childMask || tn->refByPrimitives);
				tn = tn->next;
			}
		}
	}
	return numTreeNodesInUse + numTreeNodesInPool == numTreeNodesTotal;
}


SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateTreeNodeNotHere(TreeNode* tn, int level, int hash) 
{
	RBXASSERT(assertingSpatialHash);

	TreeNode *p = hashTables[level][hash].treeNodes;
	while (p) {
		RBXASSERT(p != tn);
		p = p->next;
	}
	return true;
}

SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateNoNodesOut() 
{
	RBXASSERT(assertingSpatialHash);

	for (int l=0; l < MAX_LEVELS; l++) 
	{
		RBXASSERT(hashTables[l].size() == SpatialHashStatic::numBuckets(l));
		for (size_t i = 0; i < SpatialHashStatic::numBuckets(l); ++i) 
		{
			RBXASSERT(hashTables[l][i].nodes == NULL);
		}
	}
	return true;
}

SHP bool SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::validateContacts(Primitive* p)
{
	RBXASSERT(assertingSpatialHash);

#ifdef _RBX_DEBUGGING_SPATIAL_HASH

	__if_exists(Primitive::getFirstContact)
	{
		Contact* c = p->getFirstContact();

		// 1. For each contact, confirm there is a hash collision
		while (c) {
			Primitive* other = c->otherPrimitive(p);
			RBXASSERT_SPATIAL_HASH(validateNodesOverlap(p, other));
			RBXASSERT_SPATIAL_HASH(validateNodesOverlap(p, other));
			c = p->getNextContact(c);
		}

		// 2. For each node with other, confirm there is a contact
		SpatialNode* myNode = static_cast<SpatialNode*>(p->spatialNodes);
		while (myNode) {
			SpatialNode* hashNode = hashTables[myNode->getLevel()][myNode->hashId].nodes;
			while (hashNode) {
				if ((hashNode->primitive != p) && (hashNode->gridId == myNode->gridId)) {
					RBXASSERT_SPATIAL_HASH(Primitive::getContact(p, hashNode->primitive));
				}
				hashNode = hashNode->nextHashLink;
			}
			myNode = myNode->nextPrimitiveLink;
		}
	}
#endif

	return true;
}


SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::doStats() const
{
#if 0
bool operator<(const Vector3int32& a, const Vector3int32& b) {
	for (int i = 0; i < 3; ++i)
	{
		if (a[i] < b[i]) {
			return true;
		}
		else if (a[i] > b[i]) {
			return false;
		}
	}
	return false;
}

size_t computeNumNodes(SpatialNode* node) 
{
	int answer = 0;
	while (node) 
	{
		answer++;
		node = node->nextHashLink;
	}
	return answer;
}

size_t computeNumGrids(SpatialNode* node) 
{
	std::set<Vector3int32> grids;
	while (node) 
	{
		grids.insert(node->gridId);
		node = node->nextHashLink;
	}
	return grids.size();
}

	// little test of hash function, not normally run
	std::set<int> num_list1;
	std::set<int> num_list2;
	std::set<int> num_list3;
	std::set<int> num_list4;
/*
	for(int n = 0; n < 100000; n++)
	{
		// come up with some vectors using the full sample space
		Vector3int32 v;
		v.x = (int)((rand() << 16) ^ rand());
		v.y = (int)((rand() << 16) ^ rand());
		v.z = (int)((rand() << 16) ^ rand());

		int key = getHash(v);

		if(num_list1.find(key) == num_list1.end()) { num_list1.insert(key); continue; }
		if(num_list2.find(key) == num_list2.end()) { num_list2.insert(key); continue; }
		if(num_list3.find(key) == num_list3.end()) { num_list3.insert(key); continue; }
		if(num_list4.find(key) == num_list4.end()) { num_list4.insert(key); continue; }
	}

	int u1 = num_list1.size();
	int u2 = num_list2.size();
	int u3 = num_list3.size();
	int u4 = num_list4.size();

	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Random Hash distribution 1: %d   2: %d   3: %d   4: %d", u1, u2, u3, u4);
*/

	std::vector<size_t> counts(numBuckets(), 0);
	std::vector<size_t> differentGrids(numBuckets(), 0);

	for (size_t i = 0; i < numBuckets(); ++i)
	{
		counts[i] = computeNumNodes(nodes[i]);
		differentGrids[i] = computeNumGrids(nodes[i]);
	}

	sort(counts.begin(), counts.end());
	sort(differentGrids.begin(), differentGrids.end());

	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "CURRENT HASH DISTRIBUTION");
	size_t numSlots = 100;
	for (size_t i = 0; i < numSlots; ++i)
	{
		size_t index = ( (i + 1) * numBuckets() / numSlots ) - 1;
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Slot: %d      Count: %d", i, counts[index]);
	}


	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "TOP HASH DISTRIBUTION");
	for (size_t i = 0; i < numSlots; ++i)
	{
		size_t index = numBuckets() - 1 - i;
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Slot: %d      Count: %d", i, counts[index]);
	}

	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "TOP GRID COLLISIONS DISTRIBUTION");
	for (size_t i = 0; i < numSlots; ++i)
	{
		size_t index = numBuckets() - 1 - i;
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Slot: %d      Count: %d", i, differentGrids[index]);
	}
#endif
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::registerCoarseMovementCallback(
		CoarseMovementCallback* callback) {
	coarseMovementCallbacks.push_back(callback);
}

SHP void SpatialHash<Primitive, Contact, ContactManager, MAX_LEVELS>::unregisterCoarseMovementCallback(
		CoarseMovementCallback* callback) {
	typename std::vector<CoarseMovementCallback*>::iterator itr =
        std::find(coarseMovementCallbacks.begin(), coarseMovementCallbacks.end(), callback);
    if (itr != coarseMovementCallbacks.end()) {
        coarseMovementCallbacks.erase(itr);
	}
}

} // namespace
