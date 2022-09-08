#include "Util/Hash.h"

namespace RBX {

	unsigned int Hash::hash(const void* data, size_t bytes) 
	{
		unsigned int answer = 0;
		hashAppend(answer, data, bytes);
		return answer;
	};


	void Hash::hashAppend(unsigned int& currentHash, const void* data, size_t bytes)
	{
		const unsigned char* charArray = static_cast<const unsigned char*>(data);
		unsigned int b = 378551;
		unsigned int a = 63689;

		for(unsigned int i = 0; i < bytes; i++)
		{
			currentHash = currentHash*a + charArray[i];
			a = a*b;
		}

		currentHash = currentHash & 0x7FFFFFFF;
	}

	void Hash::hashAppend(unsigned int& currentHash, unsigned int append)
	{
		hashAppend(currentHash, &append, 4);
	}


	unsigned int Hash::hash(const std::string& str)
	{
		return hash(&str[0], str.length());
	};


} // namespace