#pragma once
#include "boost/noncopyable.hpp"
#include "RbxFormat.h"
#include "rbx/RbxTime.h"

#include <boost/thread.hpp>

namespace RBX
{

#ifndef _WIN32		
#define RBX_CEVENT_BOOST
#endif

	// TODO: This class is modeled heavily off of ATL::CEvent and should be
	//       cleaned up. Probably it should be split into 2 classes:
	//       Manual and Automatic
	class CEvent :
		public boost::noncopyable
	{
#ifdef RBX_CEVENT_BOOST
		const bool manualReset;
		volatile bool isSet;
		boost::condition_variable cond;
		boost::mutex mut;
#else
#ifdef _WIN32		
	private:
		void* m_h;
#endif	
#endif
	public:
		CEvent(bool bManualReset);
		~CEvent() throw();
		void Set() throw();
		void Wait();
		// TODO: Deprecate:
		bool Wait(int milliseconds);
		bool Wait(RBX::Time::Interval interval) { return Wait((int)(1000.0 * interval.seconds())); }

	private:
		static const int cWAIT_OBJECT_0 = 0;
		static const int cWAIT_TIMEOUT = 258;
		static const int cINFINITE = 0xFFFFFFFF;
		static int WaitForSingleObject(CEvent& event, int milliseconds);
	};
}

