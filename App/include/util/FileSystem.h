#pragma once

#include "FastLog.h"

#include <string>
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/system/error_code.hpp>

DYNAMIC_LOGGROUP(FileSystem)

namespace RBX
{

enum FileSystemDir
{
    DirAppData = 0,
    DirPicture,
    DirVideo,
    DirExe
};

namespace FileSystem
{
    boost::filesystem::path getUserDirectory(bool create, FileSystemDir dir, const char *subDirectory = 0);
    boost::filesystem::path getCacheDirectory(bool create, const char* subDirectory);
    boost::filesystem::path getTempFilePath();
    boost::filesystem::path getLogsDirectory();

    void clearCacheDirectory(const char* subDirectory);
};
} // namespace RBX
