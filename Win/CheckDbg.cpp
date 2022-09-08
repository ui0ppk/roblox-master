#include "stdafx.h"
#include "CheckDbg.h"

#include <boost/functional/hash/hash.hpp>

static volatile int g_counter = 0;

static __forceinline DWORD GetFS()
{
	DWORD dw = 0;
	__asm
	{
		push eax    // Preserve the registers
		mov eax, fs:[0x18]  // Get the TIB's linear address
		mov dw, eax
		pop eax
	}
	return dw;
}

static __forceinline DWORD GetFlag(DWORD f)
{
	DWORD dw = 0;
	__asm
	{
		push eax    // Preserve the registers
		push ecx

		mov eax, f
		mov eax, dword ptr [eax + 0x30]
		mov ecx, dword ptr [eax]    // Get the whole DWORD
		
		mov dw, ecx

		pop eax
		pop ecx
	}

	return dw;
}

__declspec(noinline) bool isDbg1()
{
	DWORD fs = GetFS();
	g_counter += boost::hash_value(fs);
	g_counter += boost::hash_value(fs + g_counter);
	DWORD flag = GetFlag(fs);
	g_counter += boost::hash_value(fs + flag);
	return (flag & 0x00010000) ? true : false;
}

__declspec(noinline) bool isDbg2()
{
	DWORD fs = GetFS();
	g_counter += boost::hash_value(rand() + fs);
	DWORD flag = GetFlag(fs);
	g_counter += boost::hash_value((fs << 3) + flag);
	bool v = (flag & 0x00010000) ? true : false;
	g_counter += boost::hash_value((fs << 2) + flag + g_counter);
	return v;
}

__declspec(noinline) bool isDbg3()
{
	DWORD fs = GetFS();
	g_counter += boost::hash_value(rand() + g_counter);
	DWORD flag = GetFlag(fs);
	g_counter += boost::hash_value(rand() + g_counter +  (rand() << 3));
	return (flag & 0x00010000) ? true : false;
}