#include "stdafx.h"

#include "V8DataModel/Folder.h"

namespace RBX {

const char* const sFolder = "Folder";

Folder::Folder()
	:DescribedCreatable<Folder, Instance, sFolder>()
{
	setName("Folder");
}

bool Folder::askAddChild(const Instance* instance) const 
{ 
	const Instance *parent = getParent();
	if (parent != NULL)
	{
		return parent->canAddChild(instance, false);
	} else {
		return true;
	}
};
bool Folder::askForbidChild(const Instance* instance) const { return !askAddChild(instance);} ;

bool Folder::askSetParent(const Instance* instance) const
{
	return true;
};

bool Folder::askForbidParent(const Instance* instance) const { return !askSetParent(instance); };

}
