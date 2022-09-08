#pragma once

#include "rbx/declarations.h"
#include <string>
#include <istream>

namespace RBX {

	class RBXInterface MD5Hasher
	{
	public:
		static MD5Hasher* create();
		virtual ~MD5Hasher() {}
		virtual void addData(std::istream& data) = 0;
		virtual void addData(const std::string& data) = 0;
		virtual void addData(const char* data, size_t nBytes) = 0;
		virtual const std::string& toString() = 0;
		virtual const char* c_str() = 0;

        virtual void toBuffer(char (&result)[16]) = 0;

		// Before 03-12-07 the hashing function didn't pad bytes with '0'
		static std::string convertToLegacyHash(std::string hash);
	};
	
	std::string CollectMd5Hash(const std::string& fileName);
    std::string ComputeMd5Hash(const std::string& data);

}//namespace