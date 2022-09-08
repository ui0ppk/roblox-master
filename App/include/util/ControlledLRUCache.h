#pragma once

#include "Util/LRUCache.h"
namespace RBX
{
	enum CacheSizeEnforceMethod { CACHE_ENFORCE_MEMORY_SIZE, CACHE_ENFORCE_OBJECT_COUNT };

	template<class Key,class Data> 
	class ControlledLRUCache
	{
	private:
		unsigned long maxSize;
		CacheSizeEnforceMethod enforceMethod;
		boost::scoped_ptr< LRUCache<Key,Data> > evictableCache;		// items that are processed and ok to delete from cache
		boost::scoped_ptr< LRUCache<Key,Data> > pinnedCache;		// items that are pending processing/use
public:
	public:
		ControlledLRUCache( const unsigned long maxSize, CacheSizeEnforceMethod method = CACHE_ENFORCE_OBJECT_COUNT) : maxSize(maxSize), enforceMethod(method)
		{
			if (method == CACHE_ENFORCE_OBJECT_COUNT)
				evictableCache.reset(new SizeEnforcedLRUCache<Key, Data>(maxSize));
			else if (method == CACHE_ENFORCE_MEMORY_SIZE)
				evictableCache.reset(new MemEnforcedLRUCache<Key, Data>(maxSize));
			else
				RBXASSERT(false);

			pinnedCache.reset(new LRUCache<Key, Data>());
		}

		~ControlledLRUCache() 
		{}

		inline const unsigned long size() 
		{
			return evictableCache->size() + pinnedCache->size(); 
		}

		inline const unsigned long memSize()
		{
			return evictableCache->memSize() + pinnedCache->memSize();
		}

		void clear() 
		{
			evictableCache.clear();
			pinnedCache.clear();
		}

		inline bool exists( const Key &key ) const 
		{
			return (evictableCache->exists(key) || pinnedCache->exists(key));
		}
		inline bool remove( const Key &key )
		{
			bool result = false;
			result = evictableCache->remove(key) || result;
			result = pinnedCache->remove(key) || result;
			return result;
		}

		inline void markEvictable(const Key& key)
		{
			Data data;
			unsigned long size;
			if(pinnedCache->fetch(key, &data, &size)){
				internalMakeEvictable(key, data, size);
			}	
		}

		inline bool fetch( const Key &key, Data* result, bool makeEvictable) 
		{
			if(evictableCache->fetch(key, result))
				return true;

			unsigned long size;
			
			if(pinnedCache->fetch(key, result, &size)){
				if(!makeEvictable) 
					return true;
				
				//Make it evictable by moving it into the evictableCache
				internalMakeEvictable(key, *result, size);
				return true;
			}

			//Didn't find it, return false
			return false;
		}

		inline void resize( unsigned long newSize)
		{
			maxSize = newSize;
			evictableCache->resize(newSize);
			pinnedCache->resize(newSize);

			unsigned long curSize = (enforceMethod == CACHE_ENFORCE_MEMORY_SIZE) ? memSize() : size();
			while((curSize > newSize) && (evictableCache->size() > 0))
			{
				evictableCache->removeLeastRecentlyUsed();
				curSize = (enforceMethod == CACHE_ENFORCE_MEMORY_SIZE) ? memSize() : size();
			}

			RBXASSERT((enforceMethod == CACHE_ENFORCE_MEMORY_SIZE ? memSize() : size()) <= newSize);
		}

		inline void insert( const Key &key, const Data &data, unsigned long dataSize = 0 ) 
		{
			//First remove it from evictableCache, since it will go into pinnedCache now
			evictableCache->remove(key);
			pinnedCache->remove(key);

			unsigned int new_size = (enforceMethod == CACHE_ENFORCE_MEMORY_SIZE) ? (this->memSize() + dataSize) : (this->size() + 1);
			if(new_size > maxSize) { 
				//We are full, see if we have evictable space
				if(evictableCache->size() > 0){
					//Something is evictable, kick it out
					evictableCache->removeLeastRecentlyUsed();
				}
			}

			pinnedCache->insert(key, data, dataSize);
		}
		inline bool isFull()
		{
			return pinnedCache->size() >= maxSize;
		}


		inline bool evictAll()
		{
			if(!pinnedCache->empty()){
				evictableCache->insert(pinnedCache->begin(), pinnedCache->end());
				pinnedCache->clear();
				return true;
			}
			return false;
		}
		private:
			void internalMakeEvictable(const Key& key, const Data& data, unsigned long dataSize)
			{
				evictableCache->insert(key,data, dataSize);
				pinnedCache->remove(key);
							
				RBXASSERT(size() <= maxSize);
			}
	};

	template<class Key,class Data> 
	class ConcurrentControlledLRUCache
	{
	private:
		RBX::ControlledLRUCache<Key, Data> cache;
		boost::mutex mutex;

		unsigned long resetCounter;
		unsigned long heartbeatCounter;
	public:
		ConcurrentControlledLRUCache(unsigned long size, unsigned long resetCounter, CacheSizeEnforceMethod enforceMethod = CACHE_ENFORCE_OBJECT_COUNT)
			:cache(size, enforceMethod)
			,resetCounter(resetCounter)
			,heartbeatCounter(0)
		{}

		inline bool fetch( const Key &key, Data* result, bool makeEvictable) 
		{
			boost::mutex::scoped_lock lock(mutex);
			return cache.fetch(key, result, makeEvictable);
		}

		inline void resize( unsigned long newSize)
		{
			boost::mutex::scoped_lock lock(mutex);
			cache.resize(newSize);
		}

		inline void insert( const Key &key, const Data &data, unsigned long dataSize = 0) 
		{
			boost::mutex::scoped_lock lock(mutex);
			return cache.insert(key, data, dataSize);
		}

		inline bool remove( const Key &key ) 
		{
			boost::mutex::scoped_lock lock(mutex);
			return cache.remove(key);
		}
		
		inline void markEvictable(const Key& key)
		{
			boost::mutex::scoped_lock lock(mutex);
			cache.markEvictable(key);
		}

		inline bool isFull()
		{
			boost::mutex::scoped_lock lock(mutex);
			return cache.isFull();
		}

		inline bool evictAll()
		{
			boost::mutex::scoped_lock lock(mutex);
			return cache.evictAll();
		}

		inline void onHeartbeat()
		{
			if(++heartbeatCounter >= resetCounter){
				heartbeatCounter = 0;
				evictAll();
			}
		}
	};
}

