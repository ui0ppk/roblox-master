#pragma once

#include "Item.h"
#include "Replicator.h"

namespace RBX { namespace Network {

class DeserializedTagItem : public DeserializedItem
{
public:
	int id;

	DeserializedTagItem();
	~DeserializedTagItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::TagItem : public Item
{
	int id;
	boost::function<bool()> readyCallback;
public:
	TagItem(Replicator* replicator, int id, boost::function<bool()> readyCallback);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

	
}}
