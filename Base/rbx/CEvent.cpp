
#include "rbx/Debug.h"
#include "rbx/CEvent.h"

#ifndef _WIN32
	const int RBX::CEvent::cWAIT_OBJECT_0;
	const int RBX::CEvent::cWAIT_TIMEOUT;
	const int RBX::CEvent::cINFINITE;
#endif


void RBX::CEvent::Wait()
{
	WaitForSingleObject(*this, cINFINITE);
}

bool RBX::CEvent::Wait(int milliseconds)
{
	return WaitForSingleObject(*this, milliseconds) == cWAIT_OBJECT_0;
}

#ifdef RBX_CEVENT_BOOST

RBX::CEvent::~CEvent() throw()
{
}

RBX::CEvent::CEvent(bool bManualReset ):isSet(false),manualReset(bManualReset) {}

void RBX::CEvent::Set() throw()
{
	boost::unique_lock<boost::mutex> lock(mut);
	if (manualReset)
	{
		isSet = true;
		cond.notify_all();
	}
	else
	{
		isSet = true;
		cond.notify_one();
	}
}

int RBX::CEvent::WaitForSingleObject(CEvent& event, int milliseconds)
{
	if (milliseconds == cINFINITE)
	{
		boost::unique_lock<boost::mutex> lock(event.mut);
		if (!event.isSet)
			event.cond.wait(lock);
		if (!event.manualReset)
			event.isSet = false;
		return cWAIT_OBJECT_0;
	}
	else
	{
		boost::system_time const time = boost::get_system_time() + boost::posix_time::milliseconds(milliseconds);
		boost::unique_lock<boost::mutex> lock(event.mut);
		bool result = event.isSet || event.cond.timed_wait(lock, time);
		if (result && !event.manualReset)
			event.isSet = false;
		return result ? cWAIT_OBJECT_0 : cWAIT_TIMEOUT;
	}

}


#else

static void WINAPI RbxThrowLastWin32()
{
	DWORD dwError = ::GetLastError();
	HRESULT hr = HRESULT_FROM_WIN32( dwError );
	throw RBX::runtime_error("HRESULT = 0x%.8X", hr); 
}


RBX::CEvent::~CEvent() throw()
{
	if( m_h != NULL )
	{
		BOOL result = ::CloseHandle( m_h );
		RBXASSERT(result != 0);
		m_h = NULL;
	}
}


RBX::CEvent::CEvent(bool bManualReset )
:m_h( NULL )
{
	m_h = ::CreateEvent( NULL, bManualReset ? TRUE : FALSE, FALSE, NULL );
	if (!m_h)
		RbxThrowLastWin32();
}



void RBX::CEvent::Set() throw()
{
	if (m_h && !::SetEvent(m_h))
		RbxThrowLastWin32();
}

int RBX::CEvent::WaitForSingleObject(CEvent& event, int milliseconds)
{
	return ::WaitForSingleObject(event.m_h, milliseconds);
}

#endif
