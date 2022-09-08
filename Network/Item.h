#pragma once

#include "rbx/Declarations.h"
#include "rbx/boost.hpp"
#include "rbx/Debug.h"
#include "rbx/RbxTime.h"
#include "util/Memory.h"
#include "boost/intrusive/list.hpp"

namespace RakNet {
	class BitStream;
}

namespace RBX { 
namespace Network {

class Replicator;

struct ItemTag;
typedef boost::intrusive::list_base_hook< boost::intrusive::tag<ItemTag> > ItemHook;

class RBXBaseClass Item : boost::noncopyable, public ItemHook
{	
public:
	enum ItemType { 
		ItemTypeEnd = 0, 
		ItemTypeDelete = 1, 
		ItemTypeNew = 2, 
		ItemTypeChangeProperty = 3, 
		ItemTypeMarker = 4,
		ItemTypePing = 5,
		ItemTypePingBack = 6,
		ItemTypeEventInvocation = 7,
		ItemTypeRequestCharacter = 8,
		ItemTypeRocky = 9,	// Cheat reporting
		ItemTypePropAcknowledgement = 10,
		ItemTypeJoinData = 11,
		ItemTypeUpdateClientQuota = 12,
		ItemTypeStreamData = 13,
		ItemTypeRegionRemoval = 14,
		ItemTypeInstanceRemoval = 15,
		ItemTypeTag = 16,
		ItemTypeStats = 17,
        ItemTypeHash = 18,
		ItemTypeMaxValue = 18,
	};

protected:
	Replicator& replicator;
	Item(Replicator& replicator):replicator(replicator){}

public:
	RBX::Time timestamp;

	virtual ~Item() {}
	virtual bool write(RakNet::BitStream& bitStream) = 0;
	static void writeItemType(RakNet::BitStream& stream, Item::ItemType value);
	static void readItemType(RakNet::BitStream& stream, Item::ItemType& value);
};

// Memory pool enabled Item
class PooledItem : public Item, public AutoMemPool::Object
{
public:
	PooledItem(Replicator& replicator) : Item(replicator) {}
	virtual ~PooledItem() {}
};

// Debugs and checks for re-entrant calls.  Right now NOT thread safe.   Just here for safety watch.

class RBXBaseClass ItemQueue : boost::noncopyable
{
public:
	typedef boost::intrusive::list<Item, boost::intrusive::base_hook<ItemHook> > ItemList;
	
private:
	ItemList itemList;

	volatile int inCode;

	bool validate() const;

	bool preValidate(int i);

	bool postValidate(int i);

public:
	ItemQueue();

	~ItemQueue();

	bool empty() const;
	size_t size() const;
	RBX::Time::Interval head_wait() const;
	RBX::Time head_time() const;

	void deleteAll();

	void clear();

	bool pop_if_present(Item*& item);

	void push_back(Item* item);

	void push_front(Item* item);

	void push_front_preserve_timestamp(Item* item);

	ItemList::iterator begin() { return itemList.begin(); }
	ItemList::iterator end() { return itemList.end(); }
};

class RBXBaseClass DeserializedItem
{
protected:
	Item::ItemType type;
public:
	DeserializedItem() : type(Item::ItemTypeEnd) {};
	virtual ~DeserializedItem() {};

	Item::ItemType getType() { return type; }

	virtual void process(Replicator& replicator) = 0;
};
	

}}	// namespace

