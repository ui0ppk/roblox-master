#pragma once
//#include "pdh.h"

#include "boost/weak_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "rbx/Debug.h"
#include "rbx/threadsafe.h"

namespace RBX
{
	template<class T>
	class ScopedSingleton
	{
	private:
		static int& initCount()
		{
			static int initcount = 0;
			return initcount;
		}
	protected:
		// use this to validate usage pattern.
		// some usage patterns will want to check that this doesn't 
		// increase more than 1.
		static int getInitCount()
		{
			return initCount();
		}
		SAFE_STATIC(rbx::spin_mutex, sync)
		SAFE_STATIC(boost::weak_ptr<T>, s_instance)
	public:
		static boost::shared_ptr<T> getInstance()
		{
			rbx::spin_mutex::scoped_lock lock(sync());
			
			shared_ptr<T> result = s_instance().lock();

			if (!result)
			{
				initCount()++;
				result = shared_ptr<T>(new T());
				s_instance() = result;
			}

			return result;
		}
        static boost::shared_ptr<T> getInstanceOptional()
        {
            return s_instance().lock();
        }
	};

}
