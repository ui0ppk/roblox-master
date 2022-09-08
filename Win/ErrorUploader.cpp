/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#include "stdafx.h"

#undef min
#undef max

#include "ErrorUploader.h"
#include "LogManager.h"

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/concepts.hpp>  // source

#include "util/http.h"
#include "util/StandardOut.h"

#ifdef _WIN32 
// For mkdir
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include "_FindFirst.h"
#endif
#include "errno.h"

std::string ErrorUploader::MoveRelative(LPCTSTR fileName, std::string path)
{
	ATL::CPath dir = fileName;
	dir.RemoveFileSpec();
	dir.Append(path.c_str());
	if (!dir.FileExists())
		::_mkdir(dir);
	ATL::CPath file = fileName;
	file.StripPath();
	dir.Append(file);
	if (!::MoveFile(fileName, dir))
		::DeleteFile(fileName);	// just in case!
	return (LPCTSTR) dir;
}
