#pragma once

#include "Item.h"
#include "Replicator.h"

namespace RBX {

class Instance;
namespace Reflection {
	class RefPropertyDescriptor;
}

namespace Network {

class Replicator::ReferencePropertyChangedItem : public PooledItem
{
	const shared_ptr<const Instance> instance;
	const Reflection::RefPropertyDescriptor& desc;
	bool newValueIsNull;
	Guid::Data newValueGuid;

public:
	ReferencePropertyChangedItem(Replicator* replicator,
		const shared_ptr<const Instance>& instance,
		const Reflection::RefPropertyDescriptor& desc);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
};

}
}
