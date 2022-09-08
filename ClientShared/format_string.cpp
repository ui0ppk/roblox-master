#ifdef WIN32
#include <Objbase.h>
#include <windows.h>
#endif
#include "format_string.h"


#if defined(__APPLE__) || defined(__ANDROID__)
inline int _vscprintf(const char* format, va_list argptr) 
{
	return vsnprintf(NULL, 0, format, argptr);
}
static const size_t _TRUNCATE = 0;
inline int vsnprintf_s(char *buffer,
		size_t sizeOfBuffer,
		size_t count,
		const char *format,
		va_list argptr) {
   return vsnprintf(buffer, sizeOfBuffer, format, argptr);
}
#endif

std::string convert_w2s(const std::wstring &str)
{
#ifdef _WIN32
	std::string result;
	LPSTR szText = new CHAR[str.size()+1];
	WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, szText, (int)str.size()+1, 0, 0);
	result = szText;
	delete[] szText;
	return result;
#else
	std::string result;
	std::copy(str.begin(), str.end(), std::back_inserter(result));
	return result;
#endif
}
  
std::wstring convert_s2w(const std::string &str)
{
#ifdef _WIN32
	std::wstring result;
	LPWSTR szTextW = new WCHAR[str.size()+1];
	MultiByteToWideChar(0, 0, str.c_str(), -1, szTextW, (int)str.size()+1);
	result = szTextW;
	delete[] szTextW;
	return result;
#else
	std::wstring result;
	std::copy(str.begin(), str.end(), std::back_inserter(result));
	return result;
#endif
}


std::string vformat(const char *fmt, va_list argPtr) {
    // We draw the line at a 1MB string.
    const int maxSize = 1000000;

    // If the string is less than 161 characters,
    // allocate it on the stack because this saves
    // the malloc/free time.
    const int bufSize = 161;
	char stackBuffer[bufSize];

    int actualSize = _vscprintf(fmt, argPtr) + 1;

    if (actualSize > bufSize) {

        // Now use the heap.
        char* heapBuffer = NULL;

        if (actualSize < maxSize) {

            heapBuffer = (char*)malloc(maxSize + 1);
			if (!heapBuffer)
				throw std::bad_alloc();
            vsnprintf_s(heapBuffer, maxSize, _TRUNCATE, fmt, argPtr);
            heapBuffer[maxSize] = '\0';
        } else {
            heapBuffer = (char*)malloc(actualSize);
			vsnprintf_s(heapBuffer, actualSize-1, _TRUNCATE, fmt, argPtr);
			heapBuffer[actualSize-1] = '\0';
        }

        std::string formattedString(heapBuffer);
        free(heapBuffer);
        return formattedString;
    } else {
		vsnprintf_s(stackBuffer, bufSize-1, _TRUNCATE, fmt, argPtr);
		stackBuffer[bufSize-1] = '\0';
        return std::string(stackBuffer);
    }
}

std::string format_string(const char* fmt,...) {
    va_list argList;
    va_start(argList,fmt);
    std::string result = vformat(fmt, argList);
    va_end(argList);
	
    return result;
}

#ifdef WIN32

std::wstring vformat(const wchar_t *fmt, va_list argPtr) {
    // We draw the line at a 1MB string.
    const int maxSize = 1000000;

    // If the string is less than 161 characters,
    // allocate it on the stack because this saves
    // the malloc/free time.
    const int bufSize = 161;
	wchar_t stackBuffer[bufSize];

    int actualSize = _vscwprintf(fmt, argPtr) + 1;

    if (actualSize > bufSize) {

        // Now use the heap.
        wchar_t* heapBuffer = NULL;

        if (actualSize < maxSize) {

            heapBuffer = (wchar_t*)malloc((maxSize + 1)*sizeof(wchar_t));
			if (!heapBuffer)
				throw std::bad_alloc();
			_vsnwprintf_s(heapBuffer, maxSize, _TRUNCATE, fmt, argPtr);
			heapBuffer[maxSize] = '\0';
        } else {
            heapBuffer = (wchar_t*)malloc(actualSize*sizeof(wchar_t));
			_vsnwprintf_s(heapBuffer, actualSize-1, _TRUNCATE, fmt, argPtr);
			heapBuffer[actualSize-1] = '\0';
        }

        std::wstring formattedString(heapBuffer);
        free(heapBuffer);
        return formattedString;
    } else {
		_vsnwprintf_s(stackBuffer, bufSize-1, _TRUNCATE, fmt, argPtr);
		stackBuffer[bufSize-1] = '\0';
        return std::wstring(stackBuffer);
    }
}

std::wstring format_string(const wchar_t* fmt,...) {
    va_list argList;
    va_start(argList,fmt);
    std::wstring result = vformat(fmt, argList);
    va_end(argList);

    return result;
}
#endif

std::vector<std::string> splitOn(
    const std::string& str,
    const char&        delimeter,
    const bool         trimEmpty )
{
    std::vector<std::string> tokens;

    std::size_t begin = 0;
    std::size_t end = str.find(delimeter);

    if ( end != std::string::npos )
    {
        while ( end != std::string::npos )
        {
            std::string tmp = str.substr( begin, end - begin );
            if ( !tmp.empty() || !trimEmpty )
                tokens.push_back( tmp );

            begin = end + 1;
            end = str.find( delimeter, begin );

            if ( end == std::string::npos )
            {
                tmp = str.substr( begin, str.length() - begin );
                if ( !tmp.empty() || !trimEmpty )
                    tokens.push_back( tmp );
            }
        }
    }
    else if ( !str.empty() || !trimEmpty )
        tokens.push_back( str );

    return tokens;
}

std::vector<std::wstring> splitOn(
    const std::wstring& str,
    const wchar_t&      delimeter,
    const bool          trimEmpty )
{
    std::vector<std::wstring> tokens;

    std::size_t begin = 0;
    std::size_t end = str.find(delimeter);

    if ( end != std::wstring::npos )
    {
        while ( end != std::wstring::npos )
        {
            std::wstring tmp = str.substr( begin, end - begin );
            if ( !tmp.empty() || !trimEmpty )
                tokens.push_back( tmp );

            begin = end + 1;
            end = str.find( delimeter, begin );

            if ( end == std::wstring::npos )
            {
                tmp = str.substr( begin, str.length() - begin );
                if ( !tmp.empty() || !trimEmpty )
                    tokens.push_back( tmp );
            }
        }
    }
    else if ( !str.empty() || !trimEmpty )
        tokens.push_back( str );

    return tokens;
}


