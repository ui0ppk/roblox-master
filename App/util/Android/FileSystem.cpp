// !!!WARNING!!!
// Do NOT call back into FASTLOG here, since FASTLOG heavily depends on this,
// and we could end up getting a SIGABRT from a lock being set multiple times.
#include "util/FileSystem.h"

#include <iostream>

#include <boost/filesystem.hpp>

#include "RbxFormat.h"
#include "FastLog.h"
#include "rbx/Debug.h"

namespace RBX
{
namespace JNI
{
std::string fileSystemFilesDir; // Set in JNIRobloxSettings.cpp.
} // namespace JNI
} // namespace RBX

using namespace RBX;
using namespace RBX::JNI;

namespace RBX
{
namespace FileSystem
{

boost::filesystem::path getUserDirectory(bool create, FileSystemDir dir, const char *subDirectory)
{
	boost::filesystem::path path;

	if (subDirectory)
	{
		path /= subDirectory;
	}

	switch (dir)
	{
	case DirAppData:
		path /= "appData";
		break;
	case DirExe:
		path /= "exe";
		break;
	case DirPicture:
		path /= "picture";
		break;
	case DirVideo:
		path /= "video";
		break;
	}
    return getCacheDirectory(true, path.native().c_str());
}

}}
