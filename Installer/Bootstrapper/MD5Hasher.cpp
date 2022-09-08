#include "StdAfx.h"
#include "MD5Hasher.h"
#include "format_string.h"

extern void throwLastError(BOOL result, const std::string& message);

#if 1
MD5Hasher::MD5Hasher(void)
{
	throwLastError(CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT), "Failed CryptAcquireContext");

	try
	{
		throwLastError(CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash), "Failed CryptCreateHash");
	}
	catch (...)
	{
		CryptReleaseContext(hProv, 0);
		throw;
	}
}

MD5Hasher::~MD5Hasher(void)
{
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
}

void MD5Hasher::addData(std::istream& data)
{
	// TODO: Should I do this reset here????
	data.clear();
	data.seekg( 0, std::ios_base::beg );
	do
	{
		char buffer[1024];
		data.read(buffer, 1024);
		addData(buffer, data.gcount());
	}
	while (data.gcount()>0);
}

void MD5Hasher::addData(const std::string& data)
{
	throwLastError(CryptHashData(hHash, (const BYTE*)data.c_str(), data.length(), 0), "Failed CryptHashData");
}

void MD5Hasher::addData(const char* data, size_t nBytes)
{
	throwLastError(CryptHashData(hHash, (const BYTE*)data, nBytes, 0), "Failed CryptHashData");
}

const char* MD5Hasher::c_str()
{
	if (result.size()==0)
	{
		DWORD hashLength;
		DWORD foo = sizeof(hashLength);
		throwLastError(CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*) &hashLength, &foo, 0), "Failed CryptGetHashParam HP_HASHSIZE");
						
		if (hashLength > 256)
			throw std::runtime_error("hashLength is too long");
		BYTE data[256];

		throwLastError(CryptGetHashParam(hHash, HP_HASHVAL, data, &hashLength, 0), "Failed CryptGetHashParam HP_HASHVAL");
		
		for (size_t i=0; i<hashLength; i++)
		{
			std::string s = format_string("%02x", data[i]);
			result += s; 
		}
	}

	return result.c_str();
}
#endif