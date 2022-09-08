#pragma once

#include "V8Tree/Instance.h"
#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>

namespace RBX
{
	// A Service is an instance that is "Singleton" in the scope of its containing
	// ServiceProvider. In other words, a Service-derived class is unique within the
	// child tree of a ServiceProvider.
	class Service 
	{
	public:
		const bool isPublic;
	protected:
		Service(bool isPublic=true)
			:isPublic(isPublic)
		{
		}
		~Service()
		{
		}
	};

	// Design decision: ServiceProvider liberally uses mutable members and declares const
	// member functions that change data. The idea here is that a ServiceProvider doesn't really
	// change fundamentally when creating and providing Services. It lets clients get a service
	// when const
	// TODO: consider the relative merits of returning Services as shared_ptr: safer, but more likely to lead to ptr leaks?
	extern const char* const sServiceProvider;
	class ServiceProvider
		: public DescribedNonCreatable<ServiceProvider, Instance, sServiceProvider>
	{
	private:
		typedef DescribedNonCreatable<ServiceProvider, Instance, sServiceProvider> Super;
		static Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> func_FindService;
		static Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> func_GetService;
		static Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> dep_service;
		static Reflection::BoundFuncDesc<ServiceProvider, shared_ptr<Instance>(std::string)> dep_GetService;
		typedef std::vector< shared_ptr<Instance> > ServiceArray;
		mutable ServiceArray serviceArray;
		mutable std::map<const RBX::Name*, shared_ptr<Instance> > serviceMap;
	public:
		rbx::signal<void()> closingSignal;
		rbx::signal<void()> closingLateSignal;
		rbx::signal<void(shared_ptr<Instance> service)> serviceAddedSignal;
		rbx::signal<void(shared_ptr<Instance> service)> serviceRemovingSignal;

		ServiceProvider();
		ServiceProvider(const char* name);

		template<class ServiceClass>
		ServiceClass* find() const
		{
			BOOST_STATIC_ASSERT((boost::is_base_of<Service, ServiceClass>::value));
			BOOST_STATIC_ASSERT((boost::is_base_of<Instance, ServiceClass>::value));

			size_t index = getClassIndex<ServiceClass>();

			ServiceClass* service;
			if (index+1>serviceArray.size())
			{
				// serviceArray has an empty entry for this service now
				serviceArray.resize(index+1, shared_ptr<Instance>());
				service = NULL;
			}
			else
			{
				service = static_cast<ServiceClass*>(serviceArray[index].get());
				if (service!=NULL)
					return service;
			}

			if (ServiceClass::isNullClassName())
				return NULL;

			// See if the service is in the className map
			shared_ptr<Instance> i = findServiceByClassName(ServiceClass::className());
			if (!i)
				return NULL;
			service = boost::polymorphic_downcast<ServiceClass*>(i.get());
			serviceArray[index] = i;
			return service;
		}

		//mutable RBX::reentrant_concurrency_catcher threadGuard;
		template<class ServiceClass>
		ServiceClass* create() const
		{
			//RBX::reentrant_concurrency_catcher::scoped_lock lock(threadGuard);

			ServiceClass* service = this->find<ServiceClass>();
			if (service==NULL)
			{
				// If all else fails, create the service and put it in the table and map
				shared_ptr<ServiceClass> s = Creatable<Instance>::create<ServiceClass>();

				service = s.get();

				// setParent can throw, so do not mutate any internal state until
				// setParent has returned without throwing.
				service->setAndLockParent(const_cast<ServiceProvider*>(this));
				size_t index = getClassIndex<ServiceClass>();
				serviceArray[index] = s;				

				// By now onDescendantAdded will have been called, which will add the service to serviceMap:
				RBXASSERT((ServiceClass::className()==RBX::Name::getNullName()) || serviceMap.find(&ServiceClass::className())!=serviceMap.end());
				
			}
			return service;
		}

		static const ServiceProvider* findServiceProvider(const Instance* context)
		{
			if (const Instance* root = Instance::getRootAncestor(context)) {
				if (const ServiceProvider* serviceProvider = Instance::fastDynamicCast<ServiceProvider>(root)) {
					return serviceProvider;
				}
			}
			return NULL;
		}

		template<class ServiceClass>
		static ServiceClass* find(const Instance* context)
		{
			if (const ServiceProvider* serviceProvider = findServiceProvider(context))
				return serviceProvider->find<ServiceClass>();
			return NULL;
		}

		template<class ServiceClass>
		static ServiceClass* find(const ServiceProvider* serviceProvider)
		{
			if (serviceProvider!=NULL)
				return serviceProvider->find<ServiceClass>();
			return NULL;
		}

		template<class ServiceClass>
		static ServiceClass* create(const Instance* context)
		{
			if (const ServiceProvider* serviceProvider = findServiceProvider(context))
				return serviceProvider->create<ServiceClass>();
			return NULL;
		}

			template<class ServiceClass>
		static ServiceClass* create(const ServiceProvider* serviceProvider)
		{
			if (serviceProvider!=NULL)
				return serviceProvider->create<ServiceClass>();
			return NULL;
		}
		
		// Less efficient factory functions that use classNames. Use only if
		// you don't know the type of Service you want.
		static shared_ptr<Instance> create(Instance* context, const RBX::Name& name);
		shared_ptr<Instance> getPublicServiceByClassNameString(std::string name); 
		
	protected:
		/*override*/ shared_ptr<Instance> createChild(const RBX::Name& className, RBX::CreatorRole creatorRole);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
		/*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoving(Instance* child);

		/* override */ bool askAddChild(const Instance* instance) const
		{
			return dynamic_cast<const Service*>(instance) != NULL;
		}

		void clearServices();

	private:
		// Returns a new number each time it is called (starting at 0)
		static size_t newIndex();

		template<class ServiceClass>
		static size_t doGetClassIndex()
		{
			static size_t index = newIndex();
			return index;
		}

		template<class ServiceClass>
		static void callDoGetClassIndex()
		{
			doGetClassIndex<ServiceClass>();
		}

		shared_ptr<Instance> findServiceByClassName(const RBX::Name& className) const;
		shared_ptr<Instance> findPublicServiceByClassNameString(std::string name);
		
	private:
		template<class ServiceClass>
		static size_t getClassIndex()
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&callDoGetClassIndex<ServiceClass>, flag);
			return doGetClassIndex<ServiceClass>();
		}

	};

	
	// A convenience class. Given an Instance* context, it will find the Service. You can treat it
	// like a Service*. Please note that it might be NULL if the context isn't a child of a ServiceProvider (like DataModel)
	template<class S>
	class ServiceClient
	{
		Instance* context;
		mutable shared_ptr<S> service;
	public:
		ServiceClient(Instance* context)
			:context(context)
		{
		}
		bool isNull() const
		{
			return findService()==NULL;
		}
		operator S*()
		{
			return createService();
		}
		operator const S*() const
		{
			return createService();
		}
		const S* operator->() const
		{
			return createService();
		}
		S* operator->()
		{
			return createService();
		}
	private:
		S* findService() const
		{
			if (!service)
				service = shared_from(ServiceProvider::find<S>(context));
			return service.get();
		}
		S* createService() const
		{
			if (!service)
				service = shared_from(ServiceProvider::create<S>(context));
			return service.get();
		}
	};

}
