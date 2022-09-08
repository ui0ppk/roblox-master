#include "stdafx.h"

#include "v8tree/service.h"
#include "boost/cast.hpp"
#include "rbx/atomic.h"

namespace RBX
{

	const char* const sServiceProvider = "ServiceProvider";

    REFLECTION_BEGIN();
	Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> ServiceProvider::func_FindService(&ServiceProvider::findPublicServiceByClassNameString, "FindService", "className", Security::None);
	Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> ServiceProvider::func_GetService(&ServiceProvider::getPublicServiceByClassNameString, "GetService", "className", Security::None);
	Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> ServiceProvider::dep_service(&ServiceProvider::getPublicServiceByClassNameString, "service", "className", Security::None, Reflection::Descriptor::Attributes::deprecated(ServiceProvider::func_GetService));
	Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> ServiceProvider::dep_GetService(&ServiceProvider::getPublicServiceByClassNameString, "getService", "className", Security::None, Reflection::Descriptor::Attributes::deprecated(ServiceProvider::func_GetService));

	static Reflection::EventDesc<ServiceProvider, void()> event_Closing(&ServiceProvider::closingSignal, "Close");
	static Reflection::EventDesc<ServiceProvider, void()> event_ClosingLate(&ServiceProvider::closingLateSignal, "CloseLate", Security::LocalUser);
	static Reflection::EventDesc<ServiceProvider, void(shared_ptr<Instance>)> event_ServiceAdded(&ServiceProvider::serviceAddedSignal, "ServiceAdded", "service");
	static Reflection::EventDesc<ServiceProvider, void(shared_ptr<Instance>)> event_ServiceRemoving(&ServiceProvider::serviceRemovingSignal, "ServiceRemoving", "service");
    REFLECTION_END();

	ServiceProvider::ServiceProvider()
	{
	}

	ServiceProvider::ServiceProvider(const char* name): Super(name)
	{
	}

	size_t ServiceProvider::newIndex()
	{
        static rbx::atomic<int> index(-1);
		return ++index;
	}

	shared_ptr<Instance> ServiceProvider::getPublicServiceByClassNameString(std::string sName)
	{
		const Name& serviceName = RBX::Name::lookup(sName);
		if (serviceName.empty())
			throw RBX::runtime_error("'%s' is not a valid Service name", sName.c_str());

		shared_ptr<Instance> result = ServiceProvider::create(this, serviceName);
		if(Service* service = dynamic_cast<Service*>(result.get())){
			if(service->isPublic){
				return result;
			}
		}
		return shared_ptr<Instance>();
	}


	shared_ptr<Instance> ServiceProvider::findPublicServiceByClassNameString(std::string sName)
	{
		const Name& serviceName = RBX::Name::lookup(sName);
		if (serviceName.empty())
			throw RBX::runtime_error("'%s' is not a valid Service name", sName.c_str());

		shared_ptr<Instance> result = findServiceByClassName(serviceName);
		if(Service* service = dynamic_cast<Service*>(result.get())){
			if(service->isPublic){
				return result;
			}
		}
		return shared_ptr<Instance>();
	}

	shared_ptr<Instance> ServiceProvider::findServiceByClassName(const RBX::Name& className) const
	{
		std::map<const RBX::Name*, shared_ptr<Instance> >::iterator iter = serviceMap.find(&className);
		if (iter!=serviceMap.end())
			return iter->second;
		else
			return shared_ptr<Instance>();
	}

	void ServiceProvider::onDescendantRemoving(const shared_ptr<Instance>& instance)
	{
		instance->onServiceProvider(this, NULL);
		Super::onDescendantRemoving(instance);
	}

	void ServiceProvider::onDescendantAdded(Instance* instance)
	{
		Super::onDescendantAdded(instance);
		instance->onServiceProvider(NULL, this);
	}

	void ServiceProvider::onChildRemoving(Instance* instance)
	{
		std::map<const RBX::Name*, shared_ptr<Instance> >::iterator iter = serviceMap.find(&instance->getClassName());
		if (iter!=serviceMap.end())
			serviceRemovingSignal(iter->second);
		Super::onChildRemoving(instance);
	}

	void ServiceProvider::clearServices()
	{
		RBXASSERT(numChildren()==0);
		serviceArray.clear();
		serviceMap.clear();
	}
		
	void ServiceProvider::onChildAdded(Instance* instance)
	{
		// There should be only one ServiceProvider, I think
		RBXASSERT(Instance::fastDynamicCast<ServiceProvider>(instance)==NULL);

		Super::onChildAdded(instance);

		Service* service = dynamic_cast<Service*>(instance);
		if (service!=NULL)
		{
			shared_ptr<Instance> i = shared_from(instance);
			if (instance->getClassName()!=RBX::Name::getNullName())
			{
				RBXASSERT_SLOW(findServiceByClassName(instance->getClassName())==NULL);
				instance->lockParent();
				serviceMap[&instance->getClassName()] = i;
			}
			serviceAddedSignal(i);
		}
	}

	shared_ptr<Instance> ServiceProvider::createChild(const RBX::Name& className, RBX::CreatorRole creatorRole)
	{
		// If the childElement is data for a Service, then have an existing Service read the data
		// rather than creating a new Service.
		shared_ptr<Instance> instance = findServiceByClassName(className);
		if (instance)
			return instance;
		else
			return Super::createChild(className, creatorRole);
	}

	shared_ptr<Instance> ServiceProvider::create(Instance* context, const RBX::Name& name)
	{
		while (context!=NULL)
		{
			ServiceProvider* serviceProvider = context->fastDynamicCast<ServiceProvider>();
			if (serviceProvider!=NULL)
			{
				shared_ptr<Instance> child = serviceProvider->createChild(name, EngineCreator);
				if(dynamic_cast<Service*>(child.get())){
					child->setParent(serviceProvider);
					return child;
				}
				return shared_ptr<Instance>();
			}
			context = context->getParent();
		}
		return shared_ptr<Instance>();
	}

}
