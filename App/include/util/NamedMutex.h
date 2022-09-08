#pragma once

#ifdef _WIN32
#include <windows.h>

namespace RBX
{
class ScopedNamedMutex
{
    HANDLE hMutex;

public:
    ScopedNamedMutex(const char* name);
    ~ScopedNamedMutex();
};
} // namespace RBX
#endif // #ifdef _WIN32 