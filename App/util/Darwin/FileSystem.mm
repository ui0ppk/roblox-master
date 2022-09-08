// This prevent inclusion of winsock.h in windows.h, which prevents windows redifinition errors
// Look at winsock2.h for details, winsock2.h is #included from boost.hpp & other places.
//#define _WINSOCKAPI_  


#include "Util/FileSystem.h"
#include "rbxformat.h"
#include "rbx/Debug.h"
#include <pwd.h>
#include <mach-o/dyld.h>

#include <Foundation/NSString.h>
#include <unistd.h>
#import <Foundation/Foundation.h>

namespace fs = boost::filesystem;

namespace RBX {

	boost::filesystem::path FileSystem::getUserDirectory(bool create, FileSystemDir dir, const char *subDirectory)
	{
#if RBX_PLATFORM_IOS
        // TODO : FIX
        #pragma warning( "IOS : fix folder creation" )
        if (false)
        {
            create = false;
        } else {
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString *documentDirPath = [paths objectAtIndex:0];
            
            std::string s = [documentDirPath cStringUsingEncoding:NSUTF8StringEncoding];
            s += "/";
            return s;
        }
        
#endif
		fs::path path;
		if (dir == DirExe) {
			char exePath[1024];
			uint exePathSize = sizeof(exePath);
			if (_NSGetExecutablePath(exePath, &exePathSize) != 0)
				return "";
			
			// Trim executable name
			std::string fullExePath = exePath;
			int lastSlash = fullExePath.find_last_of("/");
			if (lastSlash != -1) {
				fullExePath = fullExePath.substr(0, lastSlash);
			}
			
			path = fullExePath;
			if(subDirectory)
				path /= subDirectory;
		} else {
			struct passwd* pwd = getpwuid(getuid());

			path = pwd->pw_dir;
			
			std::string s = "Roblox/";
			
			if (subDirectory) {
				s += subDirectory;
			}
			
			switch (dir)
			{
				case DirAppData:
					path /= "Library";
					path /= s;
					break;
					
				case DirPicture:
					path /= "Pictures";
					path /= s;
					break;
					
				case DirVideo:
					path /= "Movies";
					path /= s;
					break;
					
				default:
					RBXASSERT(false);
					break;
			}
		}
    
        boost::system::error_code ec;
        if ( !boost::filesystem::exists(path, ec) && create ) {
			if ( !create_directories(path) ) {
				return "";
			}
		}

		return path;
	}
    
    boost::filesystem::path FileSystem::getLogsDirectory()
    {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
        NSString *documentDirPath = [paths objectAtIndex:0];
        std::string s = [documentDirPath cStringUsingEncoding:NSUTF8StringEncoding];
        
        boost::filesystem::path path = s;
        path /= "Logs";
        path /= "Roblox";
        
        boost::system::error_code ec;
        if (!boost::filesystem::exists(path, ec))
        {
            if (!create_directories(path))
            {
                // default to old behaviour
                return getCacheDirectory(true, "Logs");
            }
        }
        
        return path;
    }
    
} // namespace
