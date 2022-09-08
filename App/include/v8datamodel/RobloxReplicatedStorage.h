#pragma once

#include "V8Tree/Service.h"

namespace RBX {

	extern const char* const sRobloxReplicatedStorage;

	class RobloxReplicatedStorage
		: public DescribedCreatable<RobloxReplicatedStorage, Instance, sRobloxReplicatedStorage, Reflection::ClassDescriptor::INTERNAL, Security::RobloxScript>
		, public Service
	{
	public:
		RobloxReplicatedStorage();
	};

}
