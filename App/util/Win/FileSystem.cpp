// This prevent inclusion of winsock.h in windows.h, which prevents windows redifinition errors
// Look at winsock2.h for details, winsock2.h is #included from boost.hpp & other places.
#include "stdafx.h"

#ifdef _WIN32
#define _WINSOCKAPI_  

#include "Util/FileSystem.h"

#include <ATLPath.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>

#include "shlobj.h"
#include "rbx/Debug.h"
#include "util/standardout.h"


namespace RBX
{
namespace FileSystem
{

boost::filesystem::path getUserDirectory(bool create, FileSystemDir dir, const char *subDirectory)
{
	if (dir == DirExe)
	{
		WCHAR pathBuffer[MAX_PATH];
		::GetModuleFileNameW(0, pathBuffer, MAX_PATH);
        boost::filesystem::path path(pathBuffer);
        path.remove_filename();

		if (subDirectory)
		{
			path /= subDirectory;
        	if (create)
            {
                boost::system::error_code ec;
                boost::filesystem::create_directories(path, ec);
            }
    	}

        return path;
	}

	DWORD flags = create ? CSIDL_FLAG_CREATE : 0;
    boost::filesystem::path robloxDir = "Roblox";
	if (subDirectory)
		robloxDir /= subDirectory;

	WCHAR pathBuffer[MAX_PATH];
	HRESULT hr;
	switch (dir)
	{
		case DirAppData:
			hr = SHGetFolderPathAndSubDirW(NULL, CSIDL_LOCAL_APPDATA | flags, NULL, SHGFP_TYPE_CURRENT, robloxDir.native().c_str(), pathBuffer);
			break;
		case DirPicture:
			hr = SHGetFolderPathAndSubDirW(NULL, CSIDL_MYPICTURES | flags, NULL, SHGFP_TYPE_CURRENT, robloxDir.native().c_str(), pathBuffer);
			break;
		case DirVideo:
			hr = SHGetFolderPathAndSubDirW(NULL, CSIDL_MYVIDEO | flags, NULL, SHGFP_TYPE_CURRENT, robloxDir.native().c_str(), pathBuffer);
			break;
		default:
			RBXASSERT(false);
			hr = S_FALSE;
			break;
	}
		
	if ((hr!=S_OK) && (dir == DirAppData))
    {
		// Try this one:
		hr = SHGetFolderPathAndSubDirW(NULL, CSIDL_COMMON_APPDATA | flags, NULL, SHGFP_TYPE_CURRENT, robloxDir.native().c_str(), pathBuffer);
    }

	if (hr==S_OK)
		return pathBuffer;
	else
		return "";
}

} // namespace Filesystem
} // namespace RBX
#endif // #ifdef _WIN32
