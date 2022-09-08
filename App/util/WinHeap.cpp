/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#ifdef _WIN32

#include "Util/WinHeap.h"
#include "Util/StandardOut.h"
#include <windows.h>

namespace RBX 
{
	namespace UTIL 
	{
		void setNoFrag(HANDLE heapHandle)
		{
			ULONG  HeapFragValue = 2;
			BOOL ok = HeapSetInformation(
							heapHandle,
							HeapCompatibilityInformation,
							&HeapFragValue,
							sizeof(HeapFragValue));

/*
			if (!ok)
			{
				char temp[512];
				sprintf(temp, "Unable to set low frag heap.  GetLastError returns %d\n", GetLastError());

				::MessageBox(	NULL, 
								temp,
								"Set Low Frag Heap", 
								MB_OK );
			}
			else
			{
				::MessageBox(	NULL, 
								"set heap worked!",
								"Set Low Frag Heap", 
								MB_OK );


			}

			ok = HeapQueryInformation(
							heapHandle,
							HeapCompatibilityInformation,
							&HeapFragValue,
							sizeof(HeapFragValue),
							NULL);
*/

		}

		void setWindowsNoFragHeap()
		{
			HANDLE processHeap = GetProcessHeap();
			setNoFrag(processHeap);

			HANDLE heaps[1025];
			DWORD nheaps = GetProcessHeaps(1024, heaps);
			for (DWORD i = 0; i < nheaps; i++) {
				if (heaps[i] != processHeap)
				{
					setNoFrag(heaps[i]);
				}
			}
		}


	}
}
#endif //_WIN32