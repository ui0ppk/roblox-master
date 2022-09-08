/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/Debug.h"
#include "Util/Name.h"
#include <string>
#include <map>

#include "Security/ApiSecurity.h"
#include "Security/FuzzyTokens.h"
#include "V8DataModel/HackDefines.h"

#include "rbx/boost.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/enable_shared_from_this.hpp"

using boost::shared_ptr;
using boost::scoped_ptr;
using boost::weak_ptr;
using boost::enable_shared_from_this;


namespace RBX
{
    namespace Reflection
    {
        class DescribedBase;
    }

	enum CreatorRole
	{
		ReplicationCreator,
		SerializationCreator,
		ScriptingCreator,
		EngineCreator
	};


	template<class T, class U> boost::shared_ptr<T> shared_polymorphic_downcast(const boost::shared_ptr<U>& r)
	{
		BOOST_ASSERT(dynamic_cast<T *>(r.get()) == r.get());
		return boost::static_pointer_cast<T>(r);
	}

	template<class T, class U> boost::shared_ptr<T> shared_dynamic_cast(const boost::shared_ptr<U>& r)
	{
		return boost::dynamic_pointer_cast<T>(r);
	}

	template<class T, class U> boost::shared_ptr<T> shared_static_cast(const boost::shared_ptr<U>& r)
	{
		return boost::static_pointer_cast<T>(r);
	}

	// Use this to convert an object to its corresponding boost::shared_ptr
	template<class T> boost::shared_ptr<T> shared_from(T* r)
	{
		return r ? boost::static_pointer_cast<T>(r->shared_from_this()) : boost::shared_ptr<T>();
	}

	template<class T> weak_ptr<T> weak_from(T* r)
	{
		return r ? boost::static_pointer_cast<T>(r->shared_from_this()) : boost::shared_ptr<T>();
	}

	// Use this to downcast an object to a shared_ptr
	template<class T, class U> shared_ptr<T> shared_from_polymorphic_downcast(enable_shared_from_this<U>* r)
	{
		return r ? shared_polymorphic_downcast<T>(r->shared_from_this()) : shared_ptr<T>(); 
	}

	template<class T, class U> shared_ptr<T> shared_from_static_cast(enable_shared_from_this<U>* r)
	{
		return r ? shared_static_cast<T>(r->shared_from_this()) : shared_ptr<T>();
	}

	template<class T, class U> shared_ptr<T> shared_from_dynamic_cast(enable_shared_from_this<U>* r)
	{
		return r ? shared_dynamic_cast<T>(r->shared_from_this()) : shared_ptr<T>();
	}

    template <class T> inline bool weak_equal(const weak_ptr<T>& lhs, const weak_ptr<T>& rhs)
    {
        return !(lhs < rhs) && !(rhs < lhs);
    }

	class RBXInterface ICreator
	{
	public:
		virtual shared_ptr<Reflection::DescribedBase> create() const = 0;
	};

	template<class Class>
	class RBXBaseClass Creatable
	{
	public:
		class Deleter
		{
		public:
			void operator()(Class* instance)
			{
				Class::predelete(instance);
				delete instance;
			}
		};

		template<class T>
		static shared_ptr<T> create()
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(), Deleter());
            shared_ptr<T> (*thisFunction)() = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}
		template<class T, typename P1>
		static shared_ptr<T> create(P1 p1)
		{		
			shared_ptr<T> obj = shared_ptr<T>(new T(p1), Deleter());
            shared_ptr<T> (*thisFunction)(P1) = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;			
		}
		template<class T, typename P1, typename P2>
		static shared_ptr<T> create(P1 p1, P2 p2)
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(p1, p2), Deleter());
            shared_ptr<T> (*thisFunction)(P1, P2) = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}
		template<class T, typename P1, typename P2, typename P3>
		static shared_ptr<T> create(P1 p1, P2 p2, P3 p3)
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(p1, p2, p3), Deleter());
            shared_ptr<T> (*thisFunction)(P1, P2, P3) = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}
		template<class T, typename P1, typename P2, typename P3, typename P4>
		static shared_ptr<T> create(P1 p1, P2 p2, P3 p3, P4 p4)
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(p1, p2, p3, p4), Deleter());
            shared_ptr<T> (*thisFunction)(P1, P2, P3, P4) = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}
		template<class T, typename P1, typename P2, typename P3, typename P4, typename P5>
		static shared_ptr<T> create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(p1, p2, p3, p4, p5), Deleter());
            shared_ptr<T> (*thisFunction)(P1, P2, P3, P4, P5) = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}
		template<class T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
		static shared_ptr<T> create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6)
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(p1, p2, p3, p4, p5, p6), Deleter());
            shared_ptr<T> (*thisFunction)(P1, P2, P3, P4, P5, P6) = &create;
            checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}
		template<class T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
		static shared_ptr<T> create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7)
		{
			shared_ptr<T> obj = shared_ptr<T>(new T(p1, p2, p3, p4, p5, p6, p7), Deleter());
			shared_ptr<T> (*thisFunction)(P1, P2, P3, P4, P5, P6, P7) = &create;
			checkRbxCaller<kCallCheckCallersCode, callCheckSetBasicFlag<HATE_RETURN_CHECK> >(reinterpret_cast<void*>(thisFunction));
			return obj;
		}

		static std::map<const Name*, const ICreator*>& getCreators()
        {
			// TODO: replace with a faster lookup map, like the Loki AssocVector?
			static std::map<const Name*, const ICreator*> creators;
			return creators;
		}

		static shared_ptr<Class> createByName(const Name& name, CreatorRole creatorRole)
        {
			std::map<const Name*, const ICreator*>::iterator iter = getCreators().find(&name);
			if (iter!=getCreators().end()){
				if(shared_ptr<Class> result = shared_polymorphic_downcast<Class>(iter->second->create())){
					switch(creatorRole)
					{
					case ReplicationCreator: 
						return result;
					case SerializationCreator:
						if(result->getDescriptor().isSerializable())
							return result;
						break;
					case ScriptingCreator:
						if(result->getDescriptor().isScriptCreatable())
							return result;
						break;
					case EngineCreator:
						return result;
					}
				}
			}
			return shared_ptr<Class>();
		}
		
		// Get object creator by className
		static const ICreator* getCreator(const Name& name)
		{
			std::map<const Name*, const ICreator*>::iterator iter = getCreators().find(&name);
			return (iter!=getCreators().end()) ? iter->second : NULL;
		}

	private:
		Creatable(); // this is a utility class
	};

	template <class Class, class BaseClass, const char* const& sClassName, class FactoryClass>
	class FactoryProduct : public BaseClass {
		class Creator : public ICreator {
		private:
			static int isConstructedTrue() {return 666;}
			static int isConstructed;				// debugging - if constructed, == 666

			const RBX::Name& getClassNameUnconstructed() const {
				return RBX::Name::declare<sClassName>();
			}

		public:
			static bool wasConstructed() {return isConstructed == isConstructedTrue();}

			/* override */ shared_ptr<Reflection::DescribedBase> create() const {
				RBXASSERT(wasConstructed());
				return Creatable<FactoryClass>::template create<Class>();
			}

			const RBX::Name& getClassName() const {
				RBXASSERT(wasConstructed());
				return RBX::Name::declare<sClassName>();
			}

			Creator()
            {
				// Register this creator for create-by-className
				const RBX::Name& name = getClassNameUnconstructed();
                
                auto& creators = Creatable<FactoryClass>::getCreators();

				RBXASSERT(creators.find(&name)==creators.end());
				RBXASSERT(!wasConstructed());

				creators[&name] = this;
				isConstructed = isConstructedTrue();

				RBXASSERT(creators.find(&name)!=creators.end());
				RBXASSERT(wasConstructed());
			}

			~Creator() {
                auto& creators = Creatable<FactoryClass>::getCreators();

				RBXASSERT(wasConstructed());
				creators.erase(&getClassName());
			}
		};
	private:
#ifdef _WIN32
		static const Creator creatorPrivate;
#else
		static Creator creatorPrivate;
#endif

	protected:
		FactoryProduct() 
		{
		}
		template<class Arg0>
		FactoryProduct(Arg0 arg0):BaseClass(arg0)
		{
		}
		template<class Arg0, class Arg1>
		FactoryProduct(Arg0 arg0, Arg1 arg1):BaseClass(arg0,arg1)
		{
		}
		virtual ~FactoryProduct() 
		{
		}
		static const Creator& static_getCreator() { 
			RBXASSERT(Creator::wasConstructed());
			return creatorPrivate; 
		}
	public:
		const ICreator& getCreator() { 
			return static_getCreator(); 
		}

		static const RBX::Name& className() { return static_getCreator().getClassName(); };
		static bool isNullClassName() { 
			RBXASSERT(!className().empty());
			return false; 
		};
		const RBX::Name& getClassName() const { return static_getCreator().getClassName(); };

		// Convenient static functions for creating an instance of this class:
		// TODO: Refactor: rename createInstance --> create, but also need to rename AbstractFactoryProduct::create to something else
		static shared_ptr<Class> createInstance() {
			return Creatable<FactoryClass>::template create<Class>();
		}
		template<typename P1>
		static shared_ptr<Class> createInstance(P1 p1)
		{
			return Creatable<FactoryClass>::template create<Class>(p1);
		}
		template<typename P1, typename P2>
		static shared_ptr<Class> createInstance(P1 p1, P2 p2)
		{
			return Creatable<FactoryClass>::template create<Class>(p1, p2);
		}
		template<typename P1, typename P2, typename P3>
		static shared_ptr<Class> createInstance(P1 p1, P2 p2, P3 p3)
		{
			return Creatable<FactoryClass>::template create<Class>(p1, p2, p3);
		}
		template<typename P1, typename P2, typename P3, typename P4>
		static shared_ptr<Class> createInstance(P1 p1, P2 p2, P3 p3, P4 p4)
		{
			return Creatable<FactoryClass>::template create<Class>(p1, p2, p3, p4);
		}
	};

	// Static Defination Go Here
	// creatorPrivate was a const earlier, but that gives gcc error while trying to define the variable with const qualification.
	// gcc error: expected nested-name-specifier before 'const'
	// This is not a proper way to fix, but I do not have a choice right now. The variable is in a private section of a class so should be safe.
	template <class Class, class BaseClass, const char* const& sClassName, class FactoryClass>
#ifdef _WIN32
	typename const FactoryProduct<Class, BaseClass, sClassName, FactoryClass>::Creator FactoryProduct<Class, BaseClass, sClassName, FactoryClass>::creatorPrivate;
#else
	typename FactoryProduct<Class, BaseClass, sClassName, FactoryClass>::Creator FactoryProduct<Class, BaseClass, sClassName, FactoryClass>::creatorPrivate;
#endif

	template <class Class, class BaseClass, const char* const& sClassName, class FactoryClass>
	int FactoryProduct<Class, BaseClass, sClassName, FactoryClass>::Creator::isConstructed;
	
	// For objects that should NOT be creatable by a factory
	template <class BaseClass, const char* const& sClassName>
	class NonFactoryProduct : public BaseClass
	{
	public:
		NonFactoryProduct():BaseClass() {}
		
		template<class Arg0>
		NonFactoryProduct(Arg0 arg0):BaseClass(arg0) {}
		template<class Arg0, class Arg1>
		NonFactoryProduct(Arg0 arg0, Arg1 arg1):BaseClass(arg0, arg1) {}
		template<class Arg0, class Arg1, class Arg2>
		NonFactoryProduct(Arg0 arg0, Arg1 arg1, Arg2 arg2):BaseClass(arg0, arg1, arg2) {}
		template<class Arg0, class Arg1, class Arg2, class Arg3>
		NonFactoryProduct(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3):BaseClass(arg0, arg1, arg2, arg3) {}

		static const RBX::Name& className() 
		{ 
			return RBX::Name::declare<sClassName>();
		};
		
		static bool isNullClassName() { 
			RBXASSERT(className().empty() == (sClassName==NULL));
			return sClassName==NULL; 
		};
		const RBX::Name& getClassName() const {
			return className();
		}
	};
}	// namespace RBX

