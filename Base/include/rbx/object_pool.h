
#pragma once

#include "boost/pool/object_pool.hpp"


namespace RBX {

// T must have a non-throwing destructor
template <typename T, typename UserAllocator>
class object_pool: public boost::object_pool<T, UserAllocator>
{
  protected:
		struct CallDestructor
		{
			void operator()(T* item)
			{
				item->~T();
			}
		} callDestructor;
	public:
#ifndef _WIN32
	// gcc can't access typdef from base class...
	typedef typename boost::pool<UserAllocator>::size_type size_type;
#endif
	
    // This constructor parameter is an extension!
	explicit object_pool<T, UserAllocator>(const size_type next_size = 32)
		:boost::object_pool<T, UserAllocator>(next_size) { }

	template<class F>
	void for_each(F& f)
	{
	  // handle trivial case
	  if (!this->list.valid())
		return;

	  boost::details::PODptr<size_type> iter = this->list;
	  boost::details::PODptr<size_type> next = iter;

	  // Start 'freed_iter' at beginning of free list
	  void * freed_iter = this->first;

	  const size_type partition_size = this->alloc_size();

	  do
	  {
		// increment next
		next = next.next();

		// delete all contained objects that aren't freed

		// Iterate 'i' through all chunks in the memory block
		for (char * i = iter.begin(); i != iter.end(); i += partition_size)
		{
		  // If this chunk is free
		  if (i == freed_iter)
		  {
			// Increment freed_iter to point to next in free list
			  freed_iter = boost::simple_segregated_storage<size_type>::nextof(freed_iter);

			// Continue searching chunks in the memory block
			continue;
		  }

		  // This chunk is not free (allocated), so call f
		  f(static_cast<T *>(static_cast<void *>(i)));
		  // and continue searching chunks in the memory block
		}

		// increment iter
		iter = next;
	  } while (iter.valid());
	}

	// go through all objects, calling desctructors.
	// then use store's purge operation (which doesn't call destructors)
	void clear()
	{
		for_each(callDestructor);

		boost::pool<UserAllocator>::purge_memory();
	}


};

} // RBX

