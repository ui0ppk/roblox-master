#pragma once

#include "boost/intrusive_ptr.hpp"

namespace rbx
{
	/*
		An extension to boost::intrusive_ptr.
		To use this class, the target class needs to implement
		some functions in addition to those required by boost::instrusive_ptr:

			bool intrusive_ptr_expired(const T* p);
			bool intrusive_ptr_try_lock(const T* p);
			void intrusive_ptr_add_weak_ref(const T* p);
			void intrusive_ptr_weak_release(const T* p);

		Note: rbx::intrusive_ptr_target is a nice wrapper class that implements
		      all required functions

	*/
	template<class T> 
	class intrusive_weak_ptr
	{
		T* p_;

	public:
		inline intrusive_weak_ptr(): p_(0)
		{
		}

		inline intrusive_weak_ptr(T * p): p_(p)
		{
			if(p_ != 0) boost::intrusive_ptr_add_weak_ref(p_);
		}

	    template<class U>
		inline intrusive_weak_ptr( const intrusive_weak_ptr<U>& rhs)
		: p_( 0 )
		{
			if (!rhs.expired()) 
			{	
				p_ = rhs.raw();			
				boost::intrusive_ptr_add_weak_ref(p_);
			}
		}

		inline intrusive_weak_ptr( const intrusive_weak_ptr& rhs)
		: p_( 0 )
		{
			if (!rhs.expired()) 
			{	
				p_ = rhs.raw();			
				boost::intrusive_ptr_add_weak_ref(p_);
			}
		}

	    template<class U>
		inline intrusive_weak_ptr( const boost::intrusive_ptr<U>& rhs)
		: p_( rhs.get() )
		{
			if( p_ != 0 ) boost::intrusive_ptr_add_weak_ref(p_);
		}

	    template<class U>
		inline intrusive_weak_ptr& operator=(T * p)
		{
			reset(p);
			return *this;
		}

	    template<class U>
		inline intrusive_weak_ptr& operator=(const boost::intrusive_ptr<U> & rhs)
		{
			reset(rhs.get());
			return *this;
		}

		inline intrusive_weak_ptr& operator=(const rbx::intrusive_weak_ptr<T>& rhs)
		{
			reset(rhs.raw());
			return *this;
		}

	    template<class U>
		inline intrusive_weak_ptr& operator=(const rbx::intrusive_weak_ptr<U>& rhs)
		{
			reset(rhs.raw());
			return *this;
		}

		inline ~intrusive_weak_ptr()
		{
			if( p_ != 0 ) boost::intrusive_ptr_weak_release(p_);
		}

		inline void reset()
		{
			if( p_ != 0 ) 
			{
				boost::intrusive_ptr_weak_release(p_);
				p_ = 0;
			}
		}

		inline void reset(T* p)
		{
			if( p_ != 0 ) boost::intrusive_ptr_weak_release(p_);
			p_ = p;
			if( p_ != 0 ) boost::intrusive_ptr_add_weak_ref(p_);
		}

		inline boost::intrusive_ptr<T> lock() const
		{
			if (p_ && boost::intrusive_ptr_try_lock(p_))
				return boost::intrusive_ptr<T>(p_, false);
			else
				return boost::intrusive_ptr<T>();
		}

		inline bool expired() const
		{
			return (!p_ || boost::intrusive_ptr_expired(p_));
		}

		// TODO: Can we hide this?
		inline T* raw() const
		{
			return p_;
		}
	};	
}
