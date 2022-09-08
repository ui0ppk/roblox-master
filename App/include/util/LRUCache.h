#pragma once

#include <list>
#include <vector>
#include <boost/unordered_map.hpp>
#include "Util/StandardOut.h"

namespace RBX
{

template<class Key, class Data> 
class LRUCache
{
public:

	typedef std::list< std::pair< Key, std::pair<unsigned long, Data> > > List;         ///< Main cache storage typedef
	typedef typename List::iterator List_Iter;                ///< Main cache iterator
	typedef typename List::const_iterator List_cIter;         ///< Main cache iterator (const)
	
	typedef boost::unordered_map<Key, List_Iter> Map;		  ///< Index typedef

	typedef typename Map::iterator Map_Iter;			      ///< Index iterator
	typedef typename Map::const_iterator Map_cIter;           ///< Index iterator (const)

protected:
	/// Main cache storage
	List list;
	/// Cache storage index
	Map index;

	unsigned long totalMemory;

public:
	LRUCache() : totalMemory(0) {}
	~LRUCache() 
	{ 
		this->clear(); 
	}

	inline void printContentNames()
	{
		for(List_Iter iter = list.begin(); iter != list.end(); ++iter)
		{
			StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "%s", iter->first.c_str());
		}
	}

	inline unsigned long size()
	{
		return list.size(); 
	}

	inline unsigned long memSize()
	{
		return this->totalMemory;
	}

	inline void clear()
	{
		list.clear();
		index.clear();
		totalMemory = 0;
	}

	inline bool exists( const Key &key ) const 
	{
		return index.find( key ) != index.end();
	}
	inline bool empty() const 
	{
		return list.empty();
	}
	inline bool remove( const Key &key )
	{
		Map_Iter miter = index.find( key );
		if( miter == index.end() ) return false;
		remove( miter );
		return true;
	}
	/*inline void touch( const Key &key ) 
	{
		internalTouch(key);
	}*/

	inline bool fetch( const Key &key, Data* result, unsigned long* size, bool touch = true ) 
	{
		Map_Iter miter = index.find( key );
		if( miter == index.end() ) return false;
		
		if(touch){
			this->internalTouch( key );
		}
		if(result){
			(*result) = miter->second->second.second; // map -> list -> pair -> value
		}
		if(size){
			(*size) = miter->second->second.first;
		}
		return true;
	}
	
	inline bool fetch( const Key &key, Data* result, bool touch = true ) 
	{
		unsigned long size = 0;
		return fetch(key, result, &size, touch);
	}

	inline virtual void resize( unsigned long newSize)
	{
		while( list.size() > newSize) {
			// Remove the last element.
			List_Iter liter = list.end();
			--liter;
			this->remove( liter->first );
		}
	}

	// returns the size of last removed element
	inline void removeLeastRecentlyUsed()
	{
		// Remove the last element.
		List_Iter liter = list.end();
		--liter;
		this->remove( liter->first );
	}

	inline virtual void insert( const Key &key, const Data &data, const unsigned long dataSize = 0) 
	{
		// Touch the key, if it exists, then replace the content.
		Map_Iter miter = this->internalTouch( key );
		if( miter != index.end() )
			this->remove( miter );

		// Ok, do the actual insert at the head of the list
		list.push_front( std::make_pair( key, std::make_pair(dataSize, data) ) );
		List_Iter liter = list.begin();

		// Store the index
		index.insert( std::make_pair( key, liter ) );

		totalMemory += dataSize;
	}

	inline void insert(List_cIter iter, List_cIter iterEnd) 
	{
		for(; iter != iterEnd; ++iter){
			insert(iter->first, iter->second.second, iter->second.first);
		}
	}

	inline List_Iter begin()
	{
		return list.begin();
	}
	inline List_Iter end()
	{
		return list.end();
	}
private:
	inline Map_Iter internalTouch( const Key &key )
	{
		Map_Iter miter = index.find( key );
		if( miter == index.end() ) return miter;
		// Move the found node to the head of the list.
		list.splice( list.begin(), list, miter->second );
		return miter;
	}
	inline void remove( const Map_Iter &miter )
	{
		totalMemory -= miter->second->second.first;
		list.erase( miter->second );
		index.erase( miter );
	}
};

template<class Key,class Data> 
class SizeEnforcedLRUCache : public LRUCache<Key, Data>
{
	typedef LRUCache<Key, Data> Super;

	/// Maximum size of the cache in elements
	unsigned long maxSize;

public:

	SizeEnforcedLRUCache(const unsigned long maxSize) 
		: maxSize(maxSize)
	{}
	~SizeEnforcedLRUCache() {}

	inline void resize( unsigned long newSize)
	{
		maxSize = newSize;
		Super::resize(maxSize);
	}

	inline void insert( const Key &key, const Data &data, const unsigned long dataSize = 0) 
	{
		Super::insert(key, data, dataSize);

		// Check to see if we need to remove an element due to exceeding max_size
		if( this->list.size() > maxSize ) {
			Super::removeLeastRecentlyUsed();
		}
	}
};

template<class Key, class Data>
class MemEnforcedLRUCache : public LRUCache<Key, Data>
{
	typedef LRUCache<Key, Data> Super;

	// Maximum memory size of all the elements in the cache
	unsigned long maxMemSize;

public:

	MemEnforcedLRUCache (const unsigned long maxSize) : maxMemSize(maxSize) {}

	inline virtual void resize( unsigned long newSize)
	{
		maxMemSize = newSize;
		while( this->totalMemory > maxMemSize ) 
		{
			Super::removeLeastRecentlyUsed();
			RBXASSERT(this->totalMemory >= 0);
		}
	}

	inline void insert( const Key &key, const Data &data, const unsigned long dataSize)
	{
		Super::insert(key, data, dataSize);

		// Check to see if we need to remove an element due to exceeding max_size
		while( this->totalMemory > maxMemSize ) {
			Super::removeLeastRecentlyUsed();
			RBXASSERT(this->totalMemory >= 0);
		}
	}
};

template<class Key,class Data> 
class ConcurrentLRUCache 
{
public:
		RBX::LRUCache<Key, Data> cache;
		boost::mutex mutex;
		public:
			ConcurrentLRUCache (int size) 
				: cache(size) 
			{}
			
			bool fetch(const Key& id, Data* result)
			{
				boost::mutex::scoped_lock lock(mutex);
				return cache.fetch(id, result);
			}

			void insert(const Key& id, const Data& data)
			{
				boost::mutex::scoped_lock lock(mutex);
				cache.insert(id, data);
			}
};

	
}