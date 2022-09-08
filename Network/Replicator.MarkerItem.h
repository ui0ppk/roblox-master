#pragma once

#include "Item.h"
#include "Replicator.h"

namespace RBX {
namespace Network {

class DeserializedMarkerItem : public DeserializedItem
{
public:
	int id;

	DeserializedMarkerItem() { type = Item::ItemTypeMarker; }
	~DeserializedMarkerItem() {}
	/*implement*/ void process(Replicator& replicator) { replicator.readMarkerItem(this); }
};

class Replicator::MarkerItem : public Item
{
	int id;
public:

	MarkerItem(Replicator* replicator, int id);

	/*implement*/virtual bool write(RakNet::BitStream& bitStream);
	
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

}
}
