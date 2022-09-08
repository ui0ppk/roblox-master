#pragma once

#include "Item.h"
#include "Replicator.h"
#include "Streaming.h"

#include <boost/scoped_ptr.hpp>

namespace RBX {

class Instance;

namespace Network {

class DeserializedDeleteInstanceItem : public DeserializedItem
{
public:
	RBX::Guid::Data id;

	DeserializedDeleteInstanceItem();
	~DeserializedDeleteInstanceItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::DeleteInstanceItem : public PooledItem
{
	const IdSerializer::Id id;
	struct InstanceDetails
	{
		std::string className;
		std::string guid;
	};
	boost::scoped_ptr<InstanceDetails> details;
public:
	DeleteInstanceItem(Replicator* replicator, const shared_ptr<const Instance>& instance);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

}
}
