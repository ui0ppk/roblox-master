#pragma once
#include "V8Tree/Instance.h"

namespace RBX {

extern const char* const sLocalWorkspace;

//A structure class whose contents will never be replicated
class LocalWorkspace :  
	public DescribedNonCreatable<LocalWorkspace, Instance, sLocalWorkspace, Reflection::ClassDescriptor::INTERNAL_LOCAL>
{
private:
	typedef DescribedNonCreatable<LocalWorkspace, Instance, sLocalWorkspace, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
public:
	LocalWorkspace();
};

}

