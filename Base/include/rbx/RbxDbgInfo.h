
#pragma once

#include <cstddef>

#define PLACE_HISTORY 4
#define DBG_STRING_MAX 128

namespace RBX
{

	// Global struct with the sole purpose of being accessible in minidump

	struct RbxDbgInfo 
	{
		static RbxDbgInfo s_instance;
		RbxDbgInfo(); 

		size_t cbMaterials;
		size_t cbTextures;
		size_t cbMeshes;
		size_t cbEstFreeTextureMem;

		size_t cCommitTotal;
		size_t cCommitLimit;
		size_t cPhysicalTotal;
		size_t cPhysicalAvailable;
		size_t cbPageSize;
		size_t cKernelPaged;
		size_t cKernelNonPaged;
		size_t cSystemCache;
		size_t HandleCount;
		size_t ProcessCount;
		size_t ThreadCount;

		char GfxCardName [DBG_STRING_MAX];
		char GfxCardDriverVersion [DBG_STRING_MAX];
		char GfxCardVendorName[DBG_STRING_MAX];
		size_t TotalVideoMemory;

		char CPUName[DBG_STRING_MAX];
		size_t NumCores;

		char AudioDeviceName[DBG_STRING_MAX];
		char ServerIP[DBG_STRING_MAX];
		
		// Index 0 is always the last place visited
		union{
			long PlaceIDs[PLACE_HISTORY];
			struct
			{
				long Place0, Place1, Place2, Place3;
			};
		};
		long PlaceCounter;
		long PlayerID;


		static void SetGfxCardName(const char* s);
		static void SetGfxCardDriverVersion(const char* s);
		static void SetGfxCardVendor(const char* s);
		static void SetCPUName(const char* s);
		static void SetServerIP(const char* s);


		static void AddPlace(long ID);
		static void RemovePlace(long ID);
	};
}