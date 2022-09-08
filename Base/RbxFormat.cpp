#include "RbxFormat.h"
#include "RbxPlatform.h"
#include "FastLog.h"

#include <math.h>
#include "boost/scoped_array.hpp"

#define NEWLINE "\r\n"

#ifdef _MSC_VER
// disable: "C++ exception handler used"
#   pragma warning (push)
#   pragma warning (disable : 4530)
#endif // _MSC_VER

// If your platform does not have vsnprintf, you can find a
// implementation at http://www.ijs.si/software/snprintf/

namespace RBX {

	std::runtime_error runtime_error(const char* fmt,...) {
		va_list argList;
		va_start(argList,fmt);
		std::string result = vformat(fmt, argList);
		va_end(argList);

		return std::runtime_error(result);
	}

	std::string format(const char* fmt,...) {
		va_list argList;
		va_start(argList,fmt);
		std::string result = vformat(fmt, argList);
		va_end(argList);

		return result;
	}

	std::string vformat(const char *fmt, va_list argPtr) {

		if (!fmt)
			return "";

		// We draw the line at a 1MB string.
		const int maxSize = 1000000;

		// If the string is less than 161 characters,
		// allocate it on the stack because this saves
		// the malloc/free time.
		const int stackBufferSize = 161;

#ifdef _WIN32
		int actualSize = _vscprintf(fmt, argPtr);
		if (actualSize < stackBufferSize) {
			char stackBuffer[stackBufferSize];
			vsnprintf_s(stackBuffer, stackBufferSize, stackBufferSize, fmt, argPtr);
			return std::string(stackBuffer);
		} 
		else 
		{
			// Use the heap.
			boost::scoped_array<char> heapBuffer;

			if (actualSize > maxSize)
				actualSize = maxSize;

			heapBuffer.reset(new char[actualSize+1]);
			vsnprintf_s(heapBuffer.get(), actualSize+1, actualSize, fmt, argPtr);            

			std::string result(heapBuffer.get());
			return result;
		}
#else 
		char stackBuffer[stackBufferSize];
		int actualSize = vsnprintf(stackBuffer, stackBufferSize, fmt, argPtr);
		if (actualSize < stackBufferSize)
			return stackBuffer;

		// Use the heap.
		if (actualSize > maxSize)
			actualSize = maxSize;

		boost::scoped_array<char> heapBuffer(new char[actualSize + 1]);
		vsnprintf(heapBuffer.get(), actualSize + 1, fmt, argPtr);            
		heapBuffer[actualSize] = '\0';

		std::string result(heapBuffer.get());
		return result;
#endif

	}
} // namespace


#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#undef NEWLINE
