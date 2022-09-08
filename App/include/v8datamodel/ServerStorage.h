#pragma once

#include "V8Tree/Service.h"

namespace RBX {

	extern const char* const sServerStorage;

	class ServerStorage
		: public DescribedCreatable<ServerStorage, Instance, sServerStorage, Reflection::ClassDescriptor::PERSISTENT_LOCAL_INTERNAL>
		, public Service
	{
	public:
		ServerStorage();
	private:
		////////////////////////////////////////////////////////////////////////////////////
		// INSTANCE
		/*override*/ bool askAddChild(const Instance* instance) const;
	};

}
