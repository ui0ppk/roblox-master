#pragma once

#include "rbx/debug.h"
#include "rbx/atomic.h"
#include "rbx/declarations.h"
#include "boost/cast.hpp"

/// Forward Declarations
namespace rbx
{
	/*
		quick_intrusive_ptr_target<> is a mix-in that implements all functions required 
		to use boost::intrusive_ptr.
		maxRefs should be used if Count is a byte or short and there is the risk of an
		overflow. Some algorithms do not have this risk. To avoid writing less performant
		code, maxRefs is not strictly enforced. In a multithreaded environment you should
		pick a "reasonable" value that is less than std::numeric_limits<Count>::max. For
		example, if Count=byte, then maxRefs could be 240
	*/
	template<class T, typename Count, Count maxRefs>
	class quick_intrusive_ptr_target;

	/*
		intrusive_ptr_target<> is a mix-in that implements all functions required 
		to use boost::intrusive_ptr and rbx::intrusive_weak_ptr. If you don't need
		weak reference support, then use quick_intrusive_ptr_target
		TODO: Allow custom allocators
	*/
	template<class T, typename Count, Count maxStrong, Count maxWeak>
	class intrusive_ptr_target;

}

/// Template Specialization for boost intrusive ptrs
namespace boost
{
	/// Template Specialization for boost intrusive ptrs for quick_intrusive_ptr_target
	template<class T, typename Count, Count maxRefs>
	void intrusive_ptr_add_ref(const rbx::quick_intrusive_ptr_target<T, Count, maxRefs> * p);
	template<class T, typename Count, Count maxRefs>
    void intrusive_ptr_release(const rbx::quick_intrusive_ptr_target<T, Count, maxRefs> * p);

	/// Template Specialization for boost intrusive ptrs for intrusive_ptr_target
	template<class T, typename Count, Count maxStrong, Count maxWeak>
    void intrusive_ptr_add_ref(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p);
	template<class T, typename Count, Count maxStrong, Count maxWeak>
    void intrusive_ptr_release(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p);
	template<class T, typename Count, Count maxStrong, Count maxWeak>
    bool intrusive_ptr_expired(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p);
	template<class T, typename Count, Count maxStrong, Count maxWeak>
    bool intrusive_ptr_try_lock(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p);
	template<class T, typename Count, Count maxStrong, Count maxWeak>
    void intrusive_ptr_add_weak_ref(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p);
	template<class T, typename Count, Count maxStrong, Count maxWeak>
    void intrusive_ptr_weak_release(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p);
}


namespace rbx
{
	class too_many_refs : public std::exception
	{
	public:
	    virtual const char* what() const throw()
		{
			return "too many refs";
		}
	};


#pragma pack(push)
#pragma pack(8)	// Packing is useful if Count is short or byte
	template<class T, typename Count = int, Count maxRefs = 0 >
	class RBXBaseClass quick_intrusive_ptr_target
	{
	private:
		rbx::atomic<Count> refs;
	public:
		inline quick_intrusive_ptr_target() { refs = 0; }
		friend void boost::intrusive_ptr_add_ref<>(const quick_intrusive_ptr_target<T, Count, maxRefs>* p);
		friend void boost::intrusive_ptr_release<>(const quick_intrusive_ptr_target<T, Count, maxRefs>* p);
	};
#pragma pack(pop)	

	
	template<class T, typename Count = int, Count maxStrong = 0, Count maxWeak = maxStrong >
	class RBXBaseClass intrusive_ptr_target
	{
	private:
		// The "counts" struct is placed in memory at the head of the object
#pragma pack(push)
#pragma pack(8)	// Packing is useful if Count is short or byte
		struct counts
		{
			rbx::atomic<Count> strong;   // #shared
			rbx::atomic<Count> weak;     // #weak + (#shared != 0)
			counts()
			{
				strong = 0;
				weak = 1;
			}
		};
#pragma pack(pop)

		static inline counts* fetch(const T* t)
		{
			return reinterpret_cast<counts*>((char*) t - sizeof(counts));
		}
	public:
		void* operator new(std::size_t t)
		{
			void* c = ::malloc(sizeof(counts) + t);

			// placement new the counts:
			new (c) counts();

			return (char*)c + sizeof(counts);
		}

		void operator delete( void * p )
		{
			counts* c = fetch(reinterpret_cast<T*>(p));
			// operator delete should only be called if this object
			// never got touched by the intrusive_ptr functions
			RBXASSERT(c->strong == 0);
			RBXASSERT(c->weak == 1);
			::free(c);
		}

		friend void boost::intrusive_ptr_add_ref<>(const intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p);
		friend void boost::intrusive_ptr_release<>(const intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p);
		friend void boost::intrusive_ptr_add_weak_ref<>(const intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p);
		friend bool boost::intrusive_ptr_expired<>(const intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p);
		friend bool boost::intrusive_ptr_try_lock<>(const intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p);
		friend void boost::intrusive_ptr_weak_release<>(const intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p);
	};	 
}


namespace boost
{
	/// Template specialization quick_intrusive_ptr_target
	template<class T, typename Count, Count maxRefs>
	void intrusive_ptr_add_ref(const rbx::quick_intrusive_ptr_target<T, Count, maxRefs>* p)
	{
		if (maxRefs > 0 && p->refs >= maxRefs)
			throw rbx::too_many_refs();

		const_cast<rbx::quick_intrusive_ptr_target<T, Count, maxRefs>*>(p)->refs++;
	}

	template<class T, typename Count, Count maxRefs>
	void intrusive_ptr_release(const rbx::quick_intrusive_ptr_target<T, Count, maxRefs>* p)
	{
		RBXASSERT(p->refs > 0);
		if (--(const_cast<rbx::quick_intrusive_ptr_target<T, Count, maxRefs>*>(p)->refs) == 0)
			delete static_cast<const T*>(p);
	} 



	/// Template specialization intrusive_ptr_target
	template<class T, typename Count, Count maxStrong, Count maxWeak>
	void intrusive_ptr_add_ref(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p)
	{
		typename rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::counts* c = rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::fetch(static_cast<const T*>(p));
		
		if (maxStrong > 0)
		{
			if (++(c->strong) > maxStrong)
				throw rbx::too_many_refs();
		}
		else
		{
			c->strong++;
			RBXASSERT(c->strong < std::numeric_limits<Count>::max() - 10);
		}
	}

	template<class T, typename Count, Count maxStrong, Count maxWeak>
	void intrusive_ptr_release(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>* p)
	{
		const T* t = static_cast<const T*>(p);
        typedef typename rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::counts Counts;
		Counts* c = rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::fetch(t);
        
		if (--(c->strong) == 0)
		{
			// placement delete the object, but not the counts
			t->~T();
		
			if (--(c->weak) == 0)
			{
				// placement delete the counts and reclaim composite object memory
                c->Counts::~counts();
				::free((void*)c);
			}
		}
	} 

	template<class T, typename Count, Count maxStrong, Count maxWeak>
	void intrusive_ptr_add_weak_ref(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p)
	{
		typename rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::counts* c = rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::fetch(static_cast<const T*>(p));

		RBXASSERT(c->strong > 0);

		if (maxWeak > 0)
		{
			if (++(c->weak) > maxWeak + 1)	// weak already has a ref because of the strong refs
				throw rbx::too_many_refs();
		}
		else
		{
			++(c->weak);
			RBXASSERT(c->weak < std::numeric_limits<Count>::max() - 10);
		}
	}

	template<class T, typename Count, Count maxStrong, Count maxWeak>
	bool intrusive_ptr_expired(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p)
	{
		typename rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::counts* c = rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::fetch(static_cast<const T*>(p));
		return c->strong == 0;
	}

	template<class T, typename Count, Count maxStrong, Count maxWeak>
	bool intrusive_ptr_try_lock(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p)
	{
		typename rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::counts* c = rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::fetch(static_cast<const T*>(p));

		while (true)
		{
			Count tmp = c->strong;
			if( tmp == 0 ) 
				return false;
			if (c->strong.compare_and_swap(tmp + 1, tmp) == tmp)
				return true;
		}
	}

	template<class T, typename Count, Count maxStrong, Count maxWeak>
	void intrusive_ptr_weak_release(const rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak> * p)
	{
		const T* t = static_cast<const T*>(p);
        typedef typename rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::counts Counts;
		Counts* c = rbx::intrusive_ptr_target<T, Count, maxStrong, maxWeak>::fetch(t);
        
		if (--(c->weak) == 0)
		{
			RBXASSERT(c->strong == 0);
			// placement delete the counts and reclaim composite object memory
            c->Counts::~counts();

			::free((void*)c);
		}
	}
}
