#ifndef RBXFORMAT_H
#define RBXFORMAT_H

#include <string>
#include <stdio.h>
#include <cstdarg>
#include <stdexcept>

#include "RbxBase.h"

#ifdef __APPLE__
#include <objc/objc.h>
#endif


#ifdef _WIN32

#define strcasecmp stricmp

#else

#define sprintf_s snprintf
#define sscanf_s sscanf

#ifndef DWORD
#define DWORD uint32_t
#endif

#ifndef ARRRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/(sizeof(((x)[0]))))
#endif

#endif // ifndef _WIN32

#if defined(__GNUC__)
#define RBX_PRINTF_ATTR(string_index, first_to_check) __attribute__((format(printf, string_index, first_to_check)))
#else
#define RBX_PRINTF_ATTR(string_index, first_to_check)
#endif

namespace RBX {
	// Convenience function:
	std::runtime_error runtime_error(const char* fmt, ...) RBX_PRINTF_ATTR(1, 2);

#ifdef RBX_PLATFORM_IOS
	typedef std::runtime_error base_exception;
	
	class physics_receiver_exception : public base_exception
	{
	public:
		physics_receiver_exception(const std::string& m) : base_exception(m) {}
	};

	class network_stream_exception : public base_exception
	{
	public:
		network_stream_exception(const std::string& m) : base_exception(m) {}
	};

#else
	typedef std::exception base_exception;

	class physics_receiver_exception : public base_exception
	{
		const std::string msg;
	public:
		physics_receiver_exception(const std::string& m) : msg(m) {}
		~physics_receiver_exception() throw() {}
		const char* what() const throw()
		{
			return msg.c_str();
		}
	};

	class network_stream_exception : public base_exception
	{
		const std::string msg;
	public:
		network_stream_exception(const std::string& m) : msg(m) {}
		~network_stream_exception() throw() {}
		const char* what() const throw()
		{
			return msg.c_str();
		}
	};
#endif


/**
  Produces a string from arguments of the style of printf.  This avoids
  problems with buffer overflows when using sprintf and makes it easy
  to use the result functionally.  This function is fast when the resulting
  string is under 160 characters (not including terminator) and slower
  when the string is longer.
 */


std::string format(
    const char*                 fmt
    ...) RBX_PRINTF_ATTR(1, 2);
/**
  Like format, but can be called with the argument list from a ... function.
 */
std::string vformat(
    const char*                 fmt,
    va_list                     argPtr);

std::string trim_trailing_slashes(const std::string &path);

}; // namespace

#endif
