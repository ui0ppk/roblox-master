#include "stdafx.h"

#include "Util/Utilities.h"
#include "rbx/Debug.h"
#include "RbxAssert.h"
#include "G3D/g3dmath.h"
#include "G3D/Random.h"
#include <limits>
#include "boost/lexical_cast.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include "Util/Http.h"

DYNAMIC_FASTINTVARIABLE(ExternalHttpResponseTimeoutMillis, 30000)
DYNAMIC_FASTINTVARIABLE(ExternalHttpRequestSizeLimitKB, 1024)
DYNAMIC_FASTINTVARIABLE(ExternalHttpResponseSizeLimitKB, 4096)


const std::string RBX::Http::kGameSessionHeaderKey = "Roblox-Session-Id";
const std::string RBX::Http::kGameIdHeaderKey = "Roblox-Game-Id";
const std::string RBX::Http::kPlaceIdHeaderKey = "Roblox-Place-Id";
const std::string RBX::Http::kRequesterHeaderKey = "Requester";
const std::string RBX::Http::kPlayerCountHeaderKey = "PlayerCount";
const std::string RBX::Http::kAccessHeaderKey = "accesskey";
const std::string RBX::Http::kAssetTypeKey = "AssetType";
const std::string RBX::Http::kRBXAuthenticationNegotiation = "RBXAuthenticationNegotiation";
const std::string RBX::Http::kContentTypeDefaultUnspecified = "*/*";
const std::string RBX::Http::kContentTypeUrlEncoded = "application/x-www-form-urlencoded";
const std::string RBX::Http::kContentTypeApplicationJson = "application/json";
const std::string RBX::Http::kContentTypeApplicationXml = "application/xml";
const std::string RBX::Http::kContentTypeTextPlain = "text/plain";
const std::string RBX::Http::kContentTypeTextXml = "text/xml";
std::string RBX::Http::lastCsrfToken = "";
boost::mutex RBX::Http::lastCsrfTokenMutex;

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "objbase.h"
#include <windows.h>
#include <wincrypt.h>

std::string RBX::sha1(const std::string& source)
{
	if (source.empty())
		return source;

	// clients send to server the hashed value of version, 
	// so here we convert "version" to hashed valued for comparison when new clients join
	std::string hashedValue;
	HCRYPTPROV context;
	if(CryptAcquireContext(&context, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		HCRYPTHASH hash;
		if (CryptCreateHash(context, CALG_SHA1, 0, 0, &hash))
		{
			if (CryptHashData(hash,(BYTE*)source.c_str(), source.size(), 0))
			{
				DWORD        dwHashLen;
				DWORD        dwHashLenSize = sizeof(DWORD);
				BYTE         *pbHash;
				CryptGetHashParam(hash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwHashLenSize, 0);
				if (pbHash = (BYTE *)malloc(dwHashLen))
				{
					CryptGetHashParam(hash, HP_HASHVAL, pbHash, &dwHashLen,0);

					std::stringstream ss;
					ss.setf(std::ios::hex, std::ios::basefield);
					ss.fill('0');
					for(unsigned int i = 0 ; i < dwHashLen ; i++)
						ss << std::setw(2) << (unsigned short)pbHash[i];

					hashedValue = ss.str();
					free(pbHash);
				}
			}
			CryptDestroyHash(hash);
		}
	}
	else
		RBXASSERT(0);

	CryptReleaseContext(context, 0);

	return hashedValue;
}

#endif


namespace RBX {

    std::string Http::getLastCsrfToken()
    {
        std::string result;
        {
            boost::mutex::scoped_lock l(lastCsrfTokenMutex);
            result = lastCsrfToken;
        }
        return result;
    }
    
    void Http::setLastCsrfToken(const std::string& newToken)
    {
        boost::mutex::scoped_lock l(lastCsrfTokenMutex);
        lastCsrfToken = newToken;
    }
    
	std::string rot13(std::string source)
	{
		std::string transformed;
		for (size_t i = 0; i < source.size(); ++i) {
			if (isalpha(source[i])) {
				if ((tolower(source[i]) - 'a') < 13)
					transformed.append(1, source[i] + 13);
				else
					transformed.append(1, source[i] - 13);
			} else {
				transformed.append(1, source[i]);
			}
		}
		return transformed;
	}

	template<>
	std::string StringConverter<bool>::convertToString(const bool& value)
	{
		return value ? "true" : "false";
	}
	
	template<>
	bool StringConverter<bool>::convertToValue(const std::string& text, bool& value)
	{
		if (text=="true" || text=="True" || text=="TRUE")
			value = true;
		else if (text=="false" || text=="False" || text=="FALSE")
			value = false;
		else 
			return false;
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// StringConverter<int>
	//

	template<>
	std::string StringConverter<int>::convertToString(const int& value)
	{
		char szText[16];
		_snprintf(szText, 16, "%d", value);
		return szText;
	}

	template<>
	std::string StringConverter<long>::convertToString(const long& value)
	{
		char szText[16];
		_snprintf(szText, 16, "%ld", value);
		return szText;
	}

	template<>
	bool StringConverter<int>::convertToValue(const std::string& text, int& value)
	{
		if (text.size()==0) {
			return false;
		}
		else {
			const char* s = text.c_str();
			for (int i = text.size() - 1; i >= 0; --i)
			{
				char c = s[i];
				if (i == 0 && c == '-')
					continue;
				if (c > '9')
					return false;
				if (c < '0')
					return false;
			}
			value = atoi(s);
			return true;
		}
	}

	template<>
	bool StringConverter<G3D::int16>::convertToValue(const std::string& text, G3D::int16& value)
	{
		if (text.size()==0) {
			return false;
		}
		else {
			const char* s = text.c_str();
			for (int i = text.size() - 1; i >= 0; --i)
			{
				char c = s[i];
				if (i == 0 && c == '-')
					continue;
				if (c > '9')
					return false;
				if (c < '0')
					return false;
			}
			value = atoi(s);
			return true;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// StringConverter<unsigned int>
	//

	template<>
	std::string StringConverter<unsigned int>::convertToString(const unsigned int& value)
	{
		char szText[16];
		_snprintf(szText, 16, "%u", value);
		return szText;
	}

	template<>
	bool StringConverter<unsigned int>::convertToValue(const std::string& text, unsigned int& value)
	{
		if (text.size()==0)
			return false;
		try
		{
			value = boost::lexical_cast<unsigned int>(text);
		}
		catch(boost::bad_lexical_cast &)
		{
			return false;
		}
		return true;
	}

	template<>
	bool StringConverter<long>::convertToValue(const std::string& text, long& value)
	{
		if (text.size()==0)
			return false;
		try
		{
			value = boost::lexical_cast<long>(text);
		}
		catch(boost::bad_lexical_cast &)
		{
			return false;
		}
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// StringConverter<string>
	//

	template<>
	std::string StringConverter<std::string>::convertToString(const std::string& value)
	{
		return value;
	}

	template<>
	bool StringConverter<std::string>::convertToValue(const std::string& text, std::string& value)
	{
		value = text;
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// StringConverter<double>
	//

	template<>
	bool StringConverter<double>::convertToValue(const std::string& text, double& value)
	{
		if (text.size()==0)
			return false;
		if (text=="INF")
			value = std::numeric_limits<double>::infinity();
		else if (text=="-INF")
			value = -std::numeric_limits<double>::infinity();
		else if (text=="NAN")
			value = std::numeric_limits<double>::quiet_NaN();
		else
			value = atof(text.c_str());
		return true;
	}

	template<>
	std::string StringConverter<double>::convertToString(const double& value)
	{
		if (value==std::numeric_limits<double>::infinity())
			return "INF";
		if (value==-std::numeric_limits<double>::infinity())
			return "-INF";
		if (!((value < 0.0) || (value >= 0.0)))
			return "NAN";

		char szText[64];
		_snprintf(szText, 64, "%.20g", value);
#ifdef _DEBUG
		double temp;
		RBXASSERT(StringConverter<double>::convertToValue(szText, temp));
		RBXASSERT(temp==value);
#endif
		return szText;
	}
	////////////////////////////////////////////////////////////////////////////////
	//
	// StringConverter<float>
	//


	template<>
	bool StringConverter<float>::convertToValue(const std::string& text, float& value)
	{
		if (text.size()==0)
			return false;
		if (text=="INF")
			value = std::numeric_limits<float>::infinity();
		else if (text=="-INF")
			value = -std::numeric_limits<float>::infinity();
		else if (text=="NAN")
			value = std::numeric_limits<float>::quiet_NaN();
		else
		{
			char* endPtr;
			value = static_cast<float>(::strtod(text.c_str(), &endPtr));
		}
		return true;
	}



	template<>
	std::string StringConverter<float>::convertToString(const float& value)
	{
		if (value==std::numeric_limits<float>::infinity())
			return "INF";
		if (value==-std::numeric_limits<float>::infinity())
			return "-INF";
		if (!((value < 0.0) || (value >= 0.0)))
			return "NAN";

		char szText[32];
		_snprintf(szText, 32, "%.9g", value);
#ifdef _DEBUG
		float temp;
		RBXASSERT(StringConverter<float>::convertToValue(szText, temp));
		RBXASSERT(temp==value);
#endif
		return szText;
	}

	bool isCamel(const char* name)
	{
		std::string temp(name);
		boost::to_upper(temp);
		return std::string(name)[0] != temp[0];
	}


}	// end namespace RBX


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag3 = 0;
    };
};
