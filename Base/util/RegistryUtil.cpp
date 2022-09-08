/**
 @file RegistryUtil.cpp

 @created 2006-04-06
 @edited  2006-04-24

 Copyright 2000-2006, Morgan McGuire.
 All rights reserved.
*/
// This prevent inclusion of winsock.h in windows.h, which prevents windows redifinition errors
// Look at winsock2.h for details, winsock2.h is #included from boost.hpp & other places.
#ifdef _WIN32
#define _WINSOCKAPI_  


#include "Util/RegistryUtil.h"

#undef min
#undef max

#include "Rbx/Debug.h"


// declare HKEY constants as needed for VC6
#if !defined(HKEY_PERFORMANCE_DATA)
    #define HKEY_PERFORMANCE_DATA       (( HKEY ) ((LONG)0x80000004) )
#endif
#if !defined(HKEY_PERFORMANCE_TEXT)
    #define HKEY_PERFORMANCE_TEXT       (( HKEY ) ((LONG)0x80000050) )
#endif
#if !defined(HKEY_PERFORMANCE_NLSTEXT)
    #define HKEY_PERFORMANCE_NLSTEXT    (( HKEY ) ((LONG)0x80000060) )
#endif

namespace RBX {

// static helpers
static HKEY getKeyFromString(const char* str, size_t length);


bool RegistryUtil::keyExists(const std::string& key) {
    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));
    HKEY openKey;
    INT32 result = RegOpenKeyEx(hkey, (key.c_str() + pos + 1), 0, KEY_READ, &openKey);

    if ( result == ERROR_SUCCESS ) {
        RegCloseKey(openKey);
        return true;
    } else {
        return false;
    }
}

bool RegistryUtil::read32bitNumber(const std::string& key, INT32& valueData) {

    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));

    size_t valuePos = key.rfind('\\');
    
    if ( valuePos != std::string::npos ) {
        std::string subKey = key.substr(pos + 1, valuePos - pos - 1);
        std::string value = key.substr(valuePos + 1, key.size() - valuePos);

        HKEY openKey;
        INT32 result = RegOpenKeyEx(hkey, subKey.c_str(), 0, KEY_READ, &openKey);

        if ( result == ERROR_SUCCESS ) {
            UINT32 dataSize = sizeof(INT32);
            result = RegQueryValueEx(openKey, value.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(&valueData), reinterpret_cast<LPDWORD>(&dataSize));

            RBXASSERT(result == ERROR_SUCCESS && "Could not read registry key value.");

            RegCloseKey(openKey);
            return (result == ERROR_SUCCESS);
        }
    }
    return false;
}

bool RegistryUtil::readBinaryData(const std::string& key, BYTE* valueData, UINT32& dataSize) {
    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));

    size_t valuePos = key.rfind('\\');
    
    if ( valuePos != std::string::npos ) {
        std::string subKey = key.substr(pos + 1, valuePos - pos - 1);
        std::string value = key.substr(valuePos + 1, key.size() - valuePos);

        HKEY openKey;
        INT32 result = RegOpenKeyEx(hkey, subKey.c_str(), 0, KEY_READ, &openKey);

        if ( result == ERROR_SUCCESS ) {

            if ( valueData == NULL ) {
                result = RegQueryValueEx(openKey, value.c_str(), NULL, NULL, NULL, reinterpret_cast<LPDWORD>(&dataSize));
            } else {
                result = RegQueryValueEx(openKey, value.c_str(), NULL, NULL, valueData, reinterpret_cast<LPDWORD>(&dataSize));
            }

            RBXASSERT(result == ERROR_SUCCESS && "Could not read registry key value.");

            RegCloseKey(openKey);
            return (result == ERROR_SUCCESS);
        }
    }

    return false;
}

bool RegistryUtil::readString(const std::string& key, std::string& valueData) {
    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));

    size_t valuePos = key.rfind('\\');
    
    if ( valuePos != std::string::npos ) {
        std::string subKey = key.substr(pos + 1, valuePos - pos - 1);
        std::string value = key.substr(valuePos + 1, key.size() - valuePos);

        HKEY openKey;
        INT32 result = RegOpenKeyEx(hkey, subKey.c_str(), 0, KEY_READ, &openKey);

        if ( result == ERROR_SUCCESS ) {
            UINT32 dataSize = 0;

            result = RegQueryValueEx(openKey, value.c_str(), NULL, NULL, NULL, reinterpret_cast<LPDWORD>(&dataSize));

            if ( result == ERROR_SUCCESS ) {
                char* tmpStr = new char[dataSize];
                memset(tmpStr, 0, dataSize);

                result = RegQueryValueEx(openKey, value.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(tmpStr), reinterpret_cast<LPDWORD>(&dataSize));
                
                if ( result == ERROR_SUCCESS ) {
                    valueData = tmpStr;
                }

				delete[] tmpStr;
            }
            //RBXASSERT(result == ERROR_SUCCESS && "Could not read registry key value.");

            RegCloseKey(openKey);
            return (result == ERROR_SUCCESS);
        }
    }

    return false;
}

bool RegistryUtil::write32bitNumber(const std::string& key, INT32 valueData) {

    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));

    size_t valuePos = key.rfind('\\');
    
    if ( valuePos != std::string::npos ) {
        std::string subKey = key.substr(pos + 1, valuePos - pos - 1);
        std::string value = key.substr(valuePos + 1, key.size() - valuePos);

        HKEY openKey;
        INT32 result = RegOpenKeyEx(hkey, subKey.c_str(), 0, KEY_ALL_ACCESS, &openKey);

        if ( result == ERROR_SUCCESS ) {
            result = RegSetValueEx(openKey, value.c_str(), NULL, REG_DWORD, reinterpret_cast<const BYTE*>(&valueData), sizeof(INT32));

            RBXASSERT(result == ERROR_SUCCESS && "Could not write registry key value.");

            RegCloseKey(openKey);
            return (result == ERROR_SUCCESS);
        }
    }
    return false;
}

bool RegistryUtil::writeBinaryData(const std::string& key, const BYTE* valueData, UINT32 dataSize) {
    RBXASSERT(valueData);

    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));

    size_t valuePos = key.rfind('\\');
    
    if ( valuePos != std::string::npos ) {
        std::string subKey = key.substr(pos + 1, valuePos - pos - 1);
        std::string value = key.substr(valuePos + 1, key.size() - valuePos);

        HKEY openKey;
        INT32 result = RegOpenKeyEx(hkey, subKey.c_str(), 0, KEY_ALL_ACCESS, &openKey);

        if ( result == ERROR_SUCCESS ) {

            if (valueData) {
                result = RegSetValueEx(openKey, value.c_str(), NULL, REG_BINARY, reinterpret_cast<const BYTE*>(valueData), dataSize);
            }

            RBXASSERT(result == ERROR_SUCCESS && "Could not write registry key value.");

            RegCloseKey(openKey);
            return (result == ERROR_SUCCESS);
        }
    }

    return false;
}

bool RegistryUtil::writeString(const std::string& key, const std::string& valueData) {
    size_t pos = key.find('\\', 0);
    if ( pos == std::string::npos ) {
        return false;
    }

    HKEY hkey = getKeyFromString(key.c_str(), pos);

    if ( hkey == NULL ) {
        return false;
    }

    RBXASSERT(key.size() > (pos + 1));

    size_t valuePos = key.rfind('\\');
    
    if ( valuePos != std::string::npos ) {
        std::string subKey = key.substr(pos + 1, valuePos - pos - 1);
        std::string value = key.substr(valuePos + 1, key.size() - valuePos);

        HKEY openKey;
        INT32 result = RegOpenKeyEx(hkey, subKey.c_str(), 0, KEY_ALL_ACCESS, &openKey);

        if ( result == ERROR_SUCCESS ) {
            result = RegSetValueEx(openKey, value.c_str(), NULL, REG_SZ, reinterpret_cast<const BYTE*>(valueData.c_str()), (valueData.size() + 1));                
            RBXASSERT(result == ERROR_SUCCESS && "Could not write registry key value.");

            RegCloseKey(openKey);
            return (result == ERROR_SUCCESS);
        }
    }

    return false;
}


// static helpers
static HKEY getKeyFromString(const char* str, UINT32 length) {
    RBXASSERT(str);

    if (str) {
        if ( strncmp(str, "HKEY_CLASSES_ROOT", length) == 0 ) {
            return HKEY_CLASSES_ROOT;
        } else if  ( strncmp(str, "HKEY_CURRENT_CONFIG", length) == 0 ) {
            return HKEY_CURRENT_CONFIG;
        } else if  ( strncmp(str, "HKEY_CURRENT_USER", length) == 0 ) {
            return HKEY_CURRENT_USER;
        } else if  ( strncmp(str, "HKEY_LOCAL_MACHINE", length) == 0 ) {
            return HKEY_LOCAL_MACHINE;
        } else if  ( strncmp(str, "HKEY_PERFORMANCE_DATA", length) == 0 ) {
            return HKEY_PERFORMANCE_DATA;
        } else if  ( strncmp(str, "HKEY_PERFORMANCE_NLSTEXT", length) == 0 ) {
            return HKEY_PERFORMANCE_NLSTEXT;
        } else if  ( strncmp(str, "HKEY_PERFORMANCE_TEXT", length) == 0 ) {
            return HKEY_PERFORMANCE_TEXT;
        } else if  ( strncmp(str, "HKEY_CLASSES_ROOT", length) == 0 ) {
            return HKEY_CLASSES_ROOT;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

} // namespace G3D
#endif
