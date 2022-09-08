#pragma once

#include "Item.h"
#include "Replicator.h"

namespace RBX {

class Instance;
namespace Reflection {
	class PropertyDescriptor;
}

namespace Network {


class DeserializedChangePropertyItem : public DeserializedItem
{
public:

	RBX::Guid::Data id;
	shared_ptr<Instance> instance;
	const Reflection::PropertyDescriptor* propertyDescriptor;
	Reflection::Variant value;

	bool versionReset; // client only

	DeserializedChangePropertyItem();
	~DeserializedChangePropertyItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::ChangePropertyItem : public PooledItem
{
	const Reflection::PropertyDescriptor& desc;
	const shared_ptr<const Instance> instance;

public:
	ChangePropertyItem(Replicator* replicator,
		const shared_ptr<const Instance>& instance,
		const Reflection::PropertyDescriptor& desc);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

}
}
