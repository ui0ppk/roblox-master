#pragma once

#include <string>

namespace RBX {
#if defined(_WIN32)
	typedef std::wstring SysPathString;
#else // assume we're on Linux / Unix, which usually don't require conversions
	typedef std::string SysPathString;
#endif

	std::string utf8_encode(const SysPathString &path);
	SysPathString utf8_decode(const std::string &path);
}
