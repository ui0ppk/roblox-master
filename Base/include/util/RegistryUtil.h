#pragma once

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "pdh.h"

#include <string>

namespace RBX {

/** 
    Provides generalized Windows registry querying.

    All key names are one string in the format:
        "[base key]\[sub-keys]\value"

    [base key] can be any of the following:
        HKEY_CLASSES_ROOT
        HKEY_CURRENT_CONFIG
        HKEY_CURRENT_USER
        HKEY_LOCAL_MACHINE
        HKEY_PERFORMANCE_DATA
        HKEY_PERFORMANCE_NLSTEXT
        HKEY_PERFORMANCE_TEXT
        HKEY_USERS

    keyExists() should be used to validate a key before reading or writing
    to ensure that a debug assert or false return is for a different error.
*/
class RegistryUtil {

public:
    /** returns true if the key exists */
    static bool keyExists(const std::string& key);

    /** returns false if the key could not be read for any reason. */
    static bool read32bitNumber(const std::string& key, INT32& valueData);

    /** 
      Reads an arbitrary amount of data from a binary registry key.
      returns false if the key could not be read for any reason.
    
      @param valueData pointer to the output buffer of sufficient size. Pass NULL as valueData in order to have available data size returned in dataSize.
      @param dataSize size of the output buffer.  When NULL is passed for valueData, contains the size of available data on successful return.
    */
    static bool readBinaryData(const std::string& key, BYTE* valueData, UINT32& dataSize);

    /** returns false if the key could not be read for any reason. */
    static bool readString(const std::string& key, std::string& valueData);

    /** returns false if the key could not be written for any reason. */
    static bool write32bitNumber(const std::string& key, INT32 valueData);

    /** 
      Writes an arbitrary amount of data to a binary registry key.
      returns false if the key could not be written for any reason.
    
      @param valueData pointer to the input buffer
      @param dataSize size of the input buffer that should be written
    */
    static bool writeBinaryData(const std::string& key, const BYTE* valueData, UINT32 dataSize);

    /** returns false if the key could not be written for any reason. */
    static bool writeString(const std::string& key, const std::string& valueData);

};

} // namespace RBX

#endif // _WIN32