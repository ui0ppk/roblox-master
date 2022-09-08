#include "Replicator.ReferencePropertyChangedItem.h"

#include "Item.h"
#include "Replicator.h"

#include "Reflection/property.h"

#include "BitStream.h"

namespace RBX {
namespace Network {

Replicator::ReferencePropertyChangedItem::ReferencePropertyChangedItem(
		Replicator* replicator, const shared_ptr<const Instance>& instance,
		const Reflection::RefPropertyDescriptor& desc)
	: PooledItem(*replicator), instance(instance), desc(desc) {
	
	Instance* refInstance = DescribedBase::fastDynamicCast<Instance>(
		desc.getRefValue(instance.get()));
	if (refInstance != NULL) {
		refInstance->getGuid().extract(newValueGuid);
	} else {
		newValueGuid.scope.setNull();
	}
}

bool Replicator::ReferencePropertyChangedItem::write(RakNet::BitStream& bitStream) {
	replicator.writeChangedRefProperty(instance.get(), desc, newValueGuid, bitStream);
	return true;
}

}
}
