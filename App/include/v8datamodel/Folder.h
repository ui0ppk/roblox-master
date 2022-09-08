/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"

namespace RBX {

	extern const char* const sFolder;
	class Folder
		: public DescribedCreatable<Folder, Instance, sFolder>
	{
	private:
		typedef DescribedCreatable<Folder, Instance, sFolder> Super;
	public:
		Folder();

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ bool askForbidChild(const Instance* instance) const;
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const;
	};
}