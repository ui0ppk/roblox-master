#include "stdafx.h"
#include "FunctionMarshaller.h"

#undef min
#undef max

#include "util/StandardOut.h"
#include "rbx/boost.hpp"

namespace RBX {

FunctionMarshaller::FunctionMarshaller(DWORD threadID)
	: refCount(0)
	, postedAsyncMessage(0)
{
	this->threadID = threadID;
	HWND hWnd = this->Create(NULL, 0, 0, WS_POPUP);
	ATLASSERT(hWnd!=NULL);
}

FunctionMarshaller::~FunctionMarshaller()
{
	boost::function<void()>* f;
	while (asyncCalls.pop_if_present(f))
		delete f;

	ATLASSERT(threadID == GetCurrentThreadId());

#ifdef _DEBUG
	{
		boost::recursive_mutex::scoped_lock lock(
			staticData().windowsCriticalSection);

		ATLASSERT (refCount==0);
		// Nobody is using this window
		ATLASSERT (staticData().windows.find(threadID) == \
			staticData().windows.end());
	}
#endif

}

FunctionMarshaller* FunctionMarshaller::GetWindow()
{
	// Share a common FunctionMarshaller in a given Thread

	boost::recursive_mutex::scoped_lock lock(
		staticData().windowsCriticalSection);

	DWORD threadID = GetCurrentThreadId();
	std::map<DWORD, FunctionMarshaller*>::iterator find =
		staticData().windows.find(threadID);
	if (find != staticData().windows.end())
	{
		// We already created a window, so use it again
		find->second->refCount++;
		return find->second;
	}
	else
	{
		// Create a new window
		FunctionMarshaller* window = new FunctionMarshaller(threadID);
		staticData().windows[threadID] = window;
		window->refCount++;
		return window;
	}

}

void FunctionMarshaller::ReleaseWindow(FunctionMarshaller* window)
{
	boost::recursive_mutex::scoped_lock lock(
		staticData().windowsCriticalSection);

	window->refCount--;
	if (window->refCount==0)
	{
		// Nobody is using this window
		staticData().windows.erase(window->threadID);
		window->DestroyWindow();
	}
}

LRESULT FunctionMarshaller::OnAsyncEvent(UINT uMsg, WPARAM wParam,
										 LPARAM lParam, BOOL& bHandled)
{
	bHandled = TRUE;

	postedAsyncMessage.swap(0);

	boost::function<void()>* f;
	while (asyncCalls.pop_if_present(f))
	{
		try
		{
			(*f)();
		}
		catch (std::exception& e)
		{
			delete f;
			StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
			throw;
		}
		delete f;
	}
	return S_OK;
}

LRESULT FunctionMarshaller::OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam,
									BOOL& bHandled)
{
	bHandled = TRUE;
	Closure* closure = (Closure*)lParam;
	try
	{
		(*closure->f)();
	}
	catch (std::exception& e)
	{
		StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
		throw;
	}
	return S_OK;
}

void FunctionMarshaller::Execute(boost::function<void()> job)
{
	if (threadID == GetCurrentThreadId())
		job();
	else
	{
		Closure closure;
		closure.f = &job;
		if (S_OK != SendMessage(WM_EVENT, 0, (LPARAM)&closure))
			throw std::runtime_error(closure.errorMessage);
	}
}

void FunctionMarshaller::Submit(boost::function<void()> job)
{
	boost::function<void()>* f = new boost::function<void()>(job);
	asyncCalls.push(f);
	if (postedAsyncMessage.swap(1) == 0)
		PostMessage(WM_ASYNCEVENT, 0, 0);
}

void FunctionMarshaller::ProcessMessages()
{
	MSG stMsg = { 0 };
	while(::PeekMessage(&stMsg, this->m_hWnd, WM_ASYNCEVENT, WM_ASYNCEVENT,
		PM_REMOVE))
	{
		TranslateMessage(&stMsg);
		DispatchMessage(&stMsg);
	}
}

void FunctionMarshaller::OnFinalMessage(HWND hWnd)
{
	delete this;
}

FunctionMarshaller::StaticData::~StaticData()
{
	for (std::map<DWORD, FunctionMarshaller*>::iterator iter =
		windows.begin(); iter != windows.end(); ++iter)
	{
		iter->second->DestroyWindow();
	}
}

}  // namespace RBX
