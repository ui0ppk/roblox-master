#pragma once

// TODO: refactor this class so it doesn't use ATL

#ifndef _WIN32
// This code is platform-specific
#error
#endif

#include "atlbase.h"
#include "AtlWin.h"
#include <map>
#include "rbx/threadsafe.h"
#include "rbx/atomic.h"

namespace RBX {

// A very handy class for marshaling a function across Windows
// threads (sync and async)
class FunctionMarshaller
	: public ATL::CWindowImpl<FunctionMarshaller>
{
	const static int WM_EVENT = WM_USER + 101;		// See Q196026
	const static int WM_ASYNCEVENT = WM_USER + 102;		// See Q196026

	LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnAsyncEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	struct StaticData
	{
		std::map<DWORD, FunctionMarshaller*> windows;

		// TODO: Would non-recursive be safe here?
		boost::recursive_mutex windowsCriticalSection;
		~StaticData();
	};
	SAFE_STATIC(StaticData, staticData)

	rbx::safe_queue<boost::function<void()>*> asyncCalls;

	rbx::atomic<LONG> postedAsyncMessage;
	int refCount;
	DWORD threadID;

	FunctionMarshaller(DWORD threadID);
	~FunctionMarshaller();
public:
	// see Q196026
	DECLARE_WND_CLASS("Roblox.FunctionMarshaller")

	// TODO: Wrap with a reference counter and then remove ~StaticData()
	// cleanup code and remove ReleaseWindow()
	static FunctionMarshaller* GetWindow();

	static void ReleaseWindow(FunctionMarshaller* window);

	struct Closure
	{
		boost::function<void()>* f;
		std::string errorMessage;
	};

	// Executes the given function.
	void Execute(boost::function<void()> job);

	// Submits a function to be executed by a separate thread.
	void Submit(boost::function<void()> job);

	// Call this only from the Window's thread.
	void ProcessMessages();

	BEGIN_MSG_MAP(MarshaledListener)
		MESSAGE_HANDLER(WM_EVENT, OnEvent)
		MESSAGE_HANDLER(WM_ASYNCEVENT, OnAsyncEvent)
	END_MSG_MAP()

	virtual void OnFinalMessage(HWND hWnd);
};

}  // namespace RBX
