#include "stdafx.h"

#include "V8DataModel/CollectionService.h"
#include "V8DataModel/Configuration.h"

namespace RBX
{


const char* const sCollectionService = "CollectionService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<CollectionService, shared_ptr<const Instances>(std::string)> func_GetCollection(&CollectionService::getCollection, "GetCollection", "class", Security::None);
Reflection::EventDesc<CollectionService, void(shared_ptr<Instance>)> event_collectionItemAdded(&CollectionService::itemAddedSignal, "ItemAdded", "instance");
Reflection::EventDesc<CollectionService, void(shared_ptr<Instance>)> event_collectionItemRemoved(&CollectionService::itemRemovedSignal, "ItemRemoved", "instance");
REFLECTION_END();


CollectionService::CollectionService()
	:DescribedNonCreatable<CollectionService, Instance, sCollectionService>(sCollectionService)
{

}

shared_ptr<const Instances> CollectionService::getCollection(const Name& className)
{
	return getCollection(className.toString());
}

shared_ptr<const Instances> CollectionService::getCollection(std::string className)
{
	const CollectionMap::iterator iter = collections.find(className);
	if(iter != collections.end()){
		return iter->second->read();
	}
	return shared_ptr<const Instances>();
}

void CollectionService::removeInstance(shared_ptr<Instance> instance)
{
	const std::string& className = instance->getDescriptor().name.toString();

	CollectionMap::iterator collectionIter = collections.find(className);

	RBXASSERT(collectionIter != collections.end());
	Instances::iterator iter;
	shared_ptr<Instances> c(collectionIter->second->write());

	iter = std::find( c->begin(), c->end(), instance);
	if(iter != c->end()){
		// Fast-remove. This can make a huge speed improvement over regular remove
		*iter = c->back();
		c->pop_back();

		itemRemovedSignal(instance);
	}
	else{
		RBXASSERT(0);
	}
}
void CollectionService::addInstance(shared_ptr<Instance> instance)
{
	const std::string& className = instance->getDescriptor().name.toString();

	CollectionMap::iterator collectionIter = collections.find(className);
	if(collectionIter == collections.end()){
		collections[className].reset(new copy_on_write_ptr<Instances>());
	}

	shared_ptr<Instances> c(collections[className]->write());
	c->push_back(instance);

	itemAddedSignal(instance);
}



}