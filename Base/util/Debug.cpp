/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "rbx/Debug.h"
#include "RbxAssert.h"
#include "RbxFormat.h"
#include <algorithm>

const int CRASHONASSERT = 255;

void ReleaseAssert(int channel, const char* msg) {
	if(channel == CRASHONASSERT)
		RBXCRASH(msg);
	else
		FLog::FastLog(channel, msg, 0);
}

namespace RBX
{
	// overload this in the debugger to pass by the crash
	volatile bool Debugable::doCrashEnabled = true;

	void Debugable::doCrash()
	{
		if(doCrashEnabled)
		{
			DebugBreak();
		}
	}

	#pragma optimize ("", off )

	void Debugable::doCrash(const char* message)
	{
#if defined(RBX_PLATFORM_DURANGO)
        OutputDebugStringA("ASSERTION FAILED: ");
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
        if (!IsDebuggerPresent()) 
            return;
#endif
		if (doCrashEnabled) 
		{
			DebugBreak();
		}
	}

	#pragma optimize ("", on )

} // rbx namespace
