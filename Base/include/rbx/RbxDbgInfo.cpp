#include "rbx/RbxDbgInfo.h"

#include <string.h>

using namespace RBX;

RbxDbgInfo RbxDbgInfo::s_instance;

RbxDbgInfo::RbxDbgInfo()
{
	memset(this, 0, sizeof(RbxDbgInfo));
}

void RbxDbgInfo::AddPlace(long ID)
{
	// Shift all places to upper indices
	for(int i = PLACE_HISTORY-1; i > 0; i--)
	{
		s_instance.PlaceIDs[i]=s_instance.PlaceIDs[i-1];
	}
	s_instance.PlaceIDs[0] = ID;
	s_instance.PlaceCounter++;
}

void RbxDbgInfo::RemovePlace(long ID)
{
	s_instance.PlaceCounter--;
	for(int i = 0; i < PLACE_HISTORY; i++)
	{
		if(s_instance.PlaceIDs[i] == ID)
		{
			// Shift all places after it to lower indices
			for(int j = i; j < PLACE_HISTORY-1; j++)
			{
				s_instance.PlaceIDs[j] = s_instance.PlaceIDs[j+1];
			}
			s_instance.PlaceIDs[PLACE_HISTORY-1] = 0;
			return;
		}
	}
}

#pragma warning(push)
#pragma warning(disable:4996)
void RbxDbgInfo::SetGfxCardName(const char* s)
{	
	strncpy(s_instance.GfxCardName, s, DBG_STRING_MAX - 1);
	s_instance.GfxCardName[DBG_STRING_MAX - 1] = '\0';
}

void RbxDbgInfo::SetGfxCardDriverVersion(const char* s)
{
	strncpy(s_instance.GfxCardDriverVersion, s, DBG_STRING_MAX - 1);
	s_instance.GfxCardDriverVersion[DBG_STRING_MAX - 1] = '\0';
}

void RbxDbgInfo::SetGfxCardVendor(const char* s)
{	
	strncpy(s_instance.GfxCardVendorName, s, DBG_STRING_MAX - 1);
	s_instance.GfxCardVendorName[DBG_STRING_MAX - 1] = '\0';
}

void RbxDbgInfo::SetCPUName(const char* s)
{
	strncpy(s_instance.CPUName, s, DBG_STRING_MAX - 1);
	s_instance.CPUName[DBG_STRING_MAX - 1] = '\0';
}

void RbxDbgInfo::SetServerIP(const char* s)
{
	strncpy(s_instance.ServerIP, s, DBG_STRING_MAX - 1);
	s_instance.ServerIP[DBG_STRING_MAX - 1] = '\0';
}
#pragma warning(pop)
