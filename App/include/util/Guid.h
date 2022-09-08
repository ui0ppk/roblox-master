
#pragma once

#include <string>
#include <stdlib.h>

#include "util/name.h"
#include "util/Object.h"

#include "rbx/intrusive_ptr_target.h"
#include <boost/intrusive_ptr.hpp>

namespace RBX
{
	// A simple class that is a globally unique identifier
	class Guid : boost::noncopyable
	{
	public:
		struct Scope
		{
		public:
			Scope() { setNull(); }

			void setNull() { name = &RBX::Name::getNullName(); }
			bool isNull() const {
				return *name == RBX::Name::getNullName();
			}
			void set( const std::string& s )
			{
				name = &RBX::Name::declare(s.c_str());
			}
            void set( const char* s )
            {
                name = &RBX::Name::declare(s);
            }

			const RBX::Name* getName() const {
				return name;
			}

			int compare(const Scope& other) const
			{
				return name->compare(*other.name);
			}
			bool operator ==(const Scope& other) const {
				return name->compare(*other.name) == 0;
			}
			bool operator  <(const Scope& other) const {
				return name->compare(*other.name) < 0;
			}

			static const Scope& null();

		private:
			static Scope nullScope;

			const RBX::Name* name;
		};

		struct Data
		{
			Scope scope;
			int index;
			bool operator ==(const Data& other) const;
			bool operator <(const Data& other) const;
			// For debugging only. A string that is not guaranteed to be unique
			std::string readableString(int scopeLength = 4) const;
		};
	private:
		Data data;
	public:
		Guid();
		bool operator ==(const Guid& other) const { return data == other.data; }
		bool operator <(const Guid& other) const { return data < other.data; }

		// Compare 2 pairs of Guids. a0-a1 and b0-b1 are commutativity. Any item may be NULL
		static int compare(const Guid* a, const Guid* b);
		static int compare(const Guid* a0, const Guid* a1, const Guid* b0, const Guid* b1);
	
		// Used for serialization:
		void assign(Data data);
		void extract(Data &data) const { data = this->data; }

		void copyDataFrom(const Guid& other) {
			Data data;
			other.extract(data);
			this->assign(data);
		}

		// For debugging only. A string that is not guaranteed to be unique
		std::string readableString(int scopeLength = 4) const { return data.readableString(scopeLength); }

		static const RBX::Guid::Scope& getLocalScope();

		static void generateRBXGUID(RBX::Guid::Scope& result);
		
		// Creates a string like this: RBXc200e36038c511ceae6208002b2b79ef
		static void generateRBXGUID(std::string& result);

		// Creates a string like this: {c200e360-38c5-11ce-ae62-08002b2b79ef}
		static void generateStandardGUID(std::string& result);
	};
	
    inline size_t hash_value(const Guid::Data& data)
    {
        size_t result = 0;
        boost::hash_combine(result, data.scope.getName());
        boost::hash_combine(result, data.index);
        return result;
    }

	// A base class for objects that contain a Guid and that want lookup-by-Guid
	template<class T> 
	class RBXBaseClass GuidItem
	{
	public:
		// A Registry maintains lookup information for Guids.
		// Each instance can belong to only one Registry.
		// Usually a Registry is associated with a DataModel
		class Registry
			: public rbx::quick_intrusive_ptr_target<Registry>
		{
			friend class GuidItem;
			typedef boost::unordered_map<Guid::Data, weak_ptr<T> > Map;
			Map map;
			RBX::mutex mutex;

			Registry() 
			{}
		public:
			static boost::intrusive_ptr<Registry> create()
			{
				return boost::intrusive_ptr<Registry>(new Registry());
			}

			~Registry()
			{
				RBXASSERT(map.size() ==0);
			}

			// Returns true for empty Guid data, false for unregistered Guid data
			bool lookupByGuid(const Guid::Data& data, shared_ptr<T> &result)
			{
				if(data.scope.isNull())
				{
					result.reset();
					return true;
				}

				RBX::mutex::scoped_lock lock(mutex);
				typename Map::const_iterator iter = GuidItem::Registry::map.find(data);
				if (iter!=map.end())
				{
					result = iter->second.lock();
				}
				else
				{
					result.reset();
				}
                return !!result;
			}

			shared_ptr<T> getByGuid(const Guid::Data& data)
			{
				shared_ptr<T> result;
				lookupByGuid(data, result);
				return result;
			}
			
			// Ensure that this Guid is in the registry (thread-safe)
			void registerGuid(const T* item)
			{
				reg(item);
			}

			// Assigns a new guid to item. (not thread-safe)
			void assignGuid(T* item, const Guid::Data& guidData)
			{
				if (item->registry)
					item->registry->unregister(item);

				item->guid.assign(guidData);

				reg(item);

				item->onGuidChanged();
			}

            void tryUnregister(GuidItem* item)
            {
                // In ClientReplicator::streamOutInstance(), we are traverse all the part's descendants and unregister them
                // It is possible some descendants have been already GC'd and unregistered earlier from the same GC loop in GCJob::gcRegion()
                // Here we simply skip it if the item has already been unregistered.
                if (item->registry)
                {
                    unregister(item);
                }
            }

			void unregister(GuidItem* item)
			{
				RBXASSERT(item->registry.get()==this);
				Guid::Data data;
				item->guid.extract(data);
				{
					RBX::mutex::scoped_lock lock(mutex);
					int num = map.erase(data);
                    RBXASSERT(num == 1);
				}
				item->registry.reset();
			}

		private:
			void reg(const T* item)
			{
				if (!item->registry)
				{
					Guid::Data data;
					item->guid.extract(data);
					RBX::mutex::scoped_lock lock(mutex);
					if (!item->registry)	// thread-safe check
					{
                        map[data] = weak_from(const_cast<T*>(item));
						item->registry = this;
					}
				}
				else
					RBXASSERT(item->registry.get()==this);
			}

		};
		friend class Registry;

	private:
		mutable boost::intrusive_ptr<Registry> registry;
		Guid guid;

	public:
		GuidItem()
		{
		}

		~GuidItem()
		{ 
			if (registry)
				registry->unregister(this); 
		}

		const Guid& getGuid() const					
		{ 
			return guid;
		}
	};
};

