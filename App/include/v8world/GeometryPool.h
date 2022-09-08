#pragma once
#include "rbx/threadsafe.h"
#include "rbx/debug.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace RBX {

		struct Vector3Comparer	{
			bool operator()(const Vector3& a, const Vector3& b) const {
				if (a.x<b.x)	return true;
				if (a.x>b.x)	return false;
				if (a.y<b.y)	return true;
				if (a.y>b.y)	return false;
				if (a.z<b.z)	return true;
				return false;
			}
			// boost::hash function
			std::size_t operator()(Vector3 const& v) const
			{
				size_t result = boost::hash<float>()(v.x);
				boost::hash_combine(result, v.y);
				boost::hash_combine(result, v.z);
				return result;
			}
		};		

		struct Vector3_2Ints
		{
			Vector3 vectPart;
			int	int1;
			int int2;
			bool operator==(const Vector3_2Ints& b) const
			{
				return vectPart==b.vectPart && int1==b.int1 && int2==b.int2;
			}
		};

		struct Vector3_2IntsComparer	{
			bool operator()(const Vector3_2Ints& a, const Vector3_2Ints& b) const {
				if (a.vectPart.x<b.vectPart.x)	return true;
				if (a.vectPart.x>b.vectPart.x)	return false;
				if (a.vectPart.y<b.vectPart.y)	return true;
				if (a.vectPart.y>b.vectPart.y)	return false;
				if (a.vectPart.z<b.vectPart.z)	return true;
				if (a.vectPart.z>b.vectPart.z)	return false;
				if (a.int1<b.int1)	return true;
				if (a.int1>b.int1)	return false;
				if (a.int2<b.int2)	return true;
				return false;
			}

			// boost::hash
			std::size_t operator()(Vector3_2Ints const& v) const
			{
				size_t result = Vector3Comparer()(v.vectPart);
				boost::hash_combine(result, v.int1);
				boost::hash_combine(result, v.int2);
				return result;
			}
		};	

		struct StringComparer	{
			bool operator()(const std::string& a, const std::string& b) const 
			{
				return (a < b);
			}
		};		

		struct IntComparer	{
			bool operator()(const int& a, const int& b) const 
			{
				return (a < b);
			}
		};	

		struct FloatComparer	{
			bool operator()(const float& a, const float& b) const 
			{
				return (a < b);
			}
		};		

		template<class Key, class Value, typename Comparer> 
		class GeometryPool 
		{
        private:
            struct Entry;
			typedef std::map<Key, Entry*, Comparer> Map;

            struct Entry
            {
                Value value;
                size_t count;
                typename Map::iterator iterator;
                
                Entry(const Key& key)
                : value(key)
                , count(0)
                {
                }
            };

			class StaticData
			{
			public:
				Map map;
				rbx::spin_mutex mutex;
			};

			SAFE_STATIC(StaticData, staticData);

			static StaticData& getStaticData()
            {
				return staticData();
			}

		public:
            // This is modeled after unique_ptr with custom deleter GeometryPool::returnToken
			class Token
			{
                Token(const Token& token);
                Token& operator=(const Token& token);

			public:
                Token(): entry(0)
                {
                }

				explicit Token(Entry* entry)
                : entry(entry)
				{
                }
                
                Token(Token&& other)
                : entry(other.entry)
                {
                    other.entry = 0;
                }

				~Token()
                {
                    if (entry)
                        GeometryPool::returnToken(entry);
				}
                
                Token& operator=(Token&& other)
                {
                    if (entry)
                        GeometryPool::returnToken(entry);
                        
                    entry = other.entry;
                    other.entry = 0;
                    
                    return *this;
                }
                
                const Value& operator*() const
                {
                    return entry->value;
                }
                
                const Value* operator->() const
                {
                    return &entry->value;
                }

				// This is slightly horrible but we don't have C++11 explicit operator bool
				operator void*() const
                {
                    return entry;
                }

            private:
                Entry* entry;
			};

			static void init() { staticData(); }

			static Token getToken(const Key& key, const Key& data)
			{
				StaticData &d = getStaticData();
				rbx::spin_mutex::scoped_lock lock(d.mutex);

				typename Map::iterator it = d.map.find(key);
                Entry* entry = (it != d.map.end()) ? it->second : 0;
                
                if (!entry)
                {
                    entry = new Entry(data);
                    entry->iterator = d.map.insert(typename Map::value_type(key, entry)).first;
                }
                
                entry->count++;
                
                return Token(entry);
			}

			static Token getToken(const Key& key)
			{
                return getToken(key, key);
			}

			static void returnToken(Entry* entry)
			{
				StaticData &d = getStaticData();
				rbx::spin_mutex::scoped_lock lock(d.mutex);
                
				RBXASSERT(entry->count > 0);
                entry->count--;
                
                if (entry->count == 0)
                {
                    RBXASSERT(entry == entry->iterator->second);

    				d.map.erase(entry->iterator);
                    delete entry;
                }
			}
            
            static int getSize()
            {
				StaticData &d = getStaticData();
				rbx::spin_mutex::scoped_lock lock(d.mutex);
                
                return d.map.size();
            }
		};

} // namespace RBX
