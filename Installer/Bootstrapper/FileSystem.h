#pragma once

#include <string>

class FileSystem
{
public:
	typedef enum { RobloxUserApplicationData, RobloxUserApplicationDataLow, RobloxCommonApplicationData, RobloxUserPrograms, RobloxCommonPrograms, RobloxProgramFiles, Desktop, RoamingAppData, AppData } FolderType;
	static std::wstring getSpecialFolder(FolderType folder, bool create, const char* subDirectory = 0, bool appendRoblox = true);
	static bool IsFileExists(const TCHAR* name);
	static bool IsFolderExists(const TCHAR* name);
};
