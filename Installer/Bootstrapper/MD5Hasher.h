#pragma once
#include "wincrypt.h"
#include <string>
class MD5Hasher
{
	std::string result;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
public:
	MD5Hasher(void);
	~MD5Hasher(void);
	void addData(std::istream& data);
	void addData(const std::string& data);
	void addData(const char* data, size_t nBytes);
	const std::string& toString()
	{
		c_str();
		return result;
	}

	const char* c_str();
};
