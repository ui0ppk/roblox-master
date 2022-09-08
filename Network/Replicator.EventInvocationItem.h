#pragma once

#include "Item.h"
#include "Replicator.h"

#include "Reflection/Event.h"

namespace RBX {
namespace Network {

class DeserializedEventInvocationItem : public DeserializedItem
{
public:
	shared_ptr<Instance> instance;
	const Reflection::EventDescriptor* eventDescriptor;
	shared_ptr<Reflection::EventInvocation> eventInvocation;

	DeserializedEventInvocationItem();
	~DeserializedEventInvocationItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::EventInvocationItem : public PooledItem
{
	const Reflection::EventDescriptor& desc;
	const shared_ptr<Instance> instance;
	const Reflection::EventArguments arguments;

public:
	EventInvocationItem(Replicator* replicator,
		const shared_ptr<Instance>& instance,
		const Reflection::EventDescriptor& desc,
		const Reflection::EventArguments& arguments);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

}
}
