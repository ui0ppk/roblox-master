#pragma once

#include <istream>
#include <ostream>

#include <v8tree/Instance.h>

namespace RBX
{
	namespace SerializerBinary
	{
		enum SerializeFlags
		{
			sfHighCompression = 1 << 0,
			sfNoCompression = 1 << 1,
			sfInexactCFrame = 1 << 2
		};
		
		static const char kMagicHeader[] = "<roblox!";
		
		void serialize(std::ostream& out, const Instance* root, unsigned int flags = 0, const Instance::SaveFilter saveFilter = Instance::SAVE_ALL);
		void serialize(std::ostream& out, const Instances& instances, unsigned int flags = 0);
		
		void deserialize(std::istream& in, Instance* root);
		void deserialize(std::istream& in, Instances& result);
	};
}
