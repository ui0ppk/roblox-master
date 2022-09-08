/**
 * FunctionMarshaller.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Standard C/C++ Headers
#include <map>

// Roblox Headers
#include "rbx/threadsafe.h"

namespace RBX { 

class CEvent;

// A very handy class for marshaling a function across Windows threads (sync and async)
class FunctionMarshaller
{
public:

private:
	struct StaticData
	{
		std::map<DWORD, FunctionMarshaller*> windows;
		boost::recursive_mutex windowsCriticalSection;	// TODO: Would non-recursive be safe here?
		~StaticData();
	};
	SAFE_STATIC(StaticData, staticData)

	int refCount;
	DWORD threadID;
	FunctionMarshaller(DWORD threadID);
	~FunctionMarshaller();

public:
	// TODO: Wrap with a reference counter and then remove ~StaticData() cleanup code and remove ReleaseWindow()
	static FunctionMarshaller* GetWindow();
	static void ReleaseWindow(FunctionMarshaller* window);
	
	static void handleAppEvent(void *pClosure);
	static void freeAppEvent(void *pClosure);
	
	struct Closure
	{
		boost::function<void()>* f;
		std::string errorMessage;
		RBX::CEvent *waitEvent;
	};

	void Execute(boost::function<void()> job, CEvent *waitEvent);
	void Submit(boost::function<void()> job);

	// Call this only from the Window's thread
	void ProcessMessages() {};
};


}
