#pragma once

#include "RBX/Debug.h"

// Engine assertions often cause the game to stop running.
// Until we can fix these, turn them off.
//#define RBX_DEBUGENGINE

#ifdef RBX_DEBUGENGINE
#define RBX_ENGINE_ASSERT(expr) RBXASSERT(expr)
#else
#define RBX_ENGINE_ASSERT(expr) ((void)0)
#endif