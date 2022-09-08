#include "stdafx.h"

#include "util/rbxrandom.h"

namespace RBX {
	unsigned int randomSeed()
	{
		std::string guid;
		RBX::Guid::generateStandardGUID(guid);
		unsigned int seed = 0;
		for (unsigned int i=0; i < guid.size() - 4; i += 4)
		{
			std::string section = guid.substr(i, 4);
			unsigned int seedSection;
			strncpy((char*) &seedSection, section.c_str(), 4);
			seed ^= seedSection;
		}
		return seed;
	}
}