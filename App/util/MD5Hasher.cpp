#include "stdafx.h"

#include "Util/MD5Hasher.h"
#include "Util/StandardOut.h"
#include "StringConv.h"



#include "Util/md5.h"
// Windows needs digest length defined
#define MD5_DIGEST_LENGTH 16


namespace RBX
{
	extern void ThrowLastError(const char* message);

	class MD5HasherImpl : public MD5Hasher
	{
		MD5_CTX context;

        unsigned char resultBuffer[MD5_DIGEST_LENGTH];
		std::string resultString;
        bool resultReady;

	public:
		MD5HasherImpl()
			: resultReady(false)
		{
			MD5_Init(&context);
		}

		~MD5HasherImpl()
		{
		}

		virtual void addData(std::istream& data)
		{
            RBXASSERT(!resultReady);

			// TODO: Should I do this reset here????
			data.clear();
			data.seekg(0, std::ios_base::beg);
			do
			{
				char buffer[1024];
				data.read(buffer, 1024);
				addData(buffer, (size_t)data.gcount());
			}
			while (data.gcount()>0);
		}

		virtual void addData(const std::string& data)
		{
            RBXASSERT(!resultReady);

			MD5_Update(&context, (void*)data.data(), data.length());
		}

		virtual void addData(const char* data, size_t nBytes)
		{
            RBXASSERT(!resultReady);

			MD5_Update(&context, (void*)data, nBytes);
		}

		virtual const std::string& toString()
		{
            finish();

			return resultString;
		}

        virtual void toBuffer(char (&result)[16])
		{
			BOOST_STATIC_ASSERT(sizeof(result) == sizeof(resultBuffer));

            finish();

			memcpy(result, resultBuffer, sizeof(resultBuffer));
		}

		virtual const char* c_str()
		{
            finish();

			return resultString.c_str();
		}

        void finish()
		{
			if (!resultReady)
			{
				resultReady = true;

				MD5_Final(resultBuffer, &context);
                
                resultString.reserve(MD5_DIGEST_LENGTH * 2);

				for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i)
                {
                    const char* hex = "0123456789abcdef";

                    resultString += hex[(resultBuffer[i] >> 4) & 15];
                    resultString += hex[(resultBuffer[i] >> 0) & 15];
                }
			}
		}
	};

	std::string MD5Hasher::convertToLegacyHash(std::string hash)
	{
		std::string result;
		for (int i=0; i<32; i+=2)
		{
			if (hash[i]!='0')
				result += hash[i];
			result += hash[i+1];
		}
		return result;
	}	

	MD5Hasher* MD5Hasher::create()
	{
		return new MD5HasherImpl();
	}
	
	std::string CollectMd5Hash(const std::string& fileName)
	{
		boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
		std::ifstream input(utf8_decode(fileName).c_str(),std::ios_base::binary);
		hasher->addData(input);
		return hasher->toString();
	}

    std::string ComputeMd5Hash(const std::string& data)
    {
        MD5HasherImpl impl;
        impl.addData(data);
        return impl.toString();
    }
    
} //namespace
