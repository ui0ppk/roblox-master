#include "stdafx.h"

#include "reflection/Function.h"
#include "reflection/YieldFunction.h"
#include "reflection/object.h"

using namespace RBX;
using namespace RBX::Reflection;

FunctionDescriptor::FunctionDescriptor(ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes)
	:MemberDescriptor(classDescriptor, name, "Function", attributes, security)
	,kind(Kind_Default)
{
	// TODO: This could be moved up into MemberDescriptor if it were templated...
	classDescriptor.MemberDescriptorContainer<FunctionDescriptor>::declare(this);		
}

YieldFunctionDescriptor::YieldFunctionDescriptor(ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes)
	:MemberDescriptor(classDescriptor, name, "YieldFunction", attributes, security)
{
	// TODO: This could be moved up into MemberDescriptor if it were templated...
	classDescriptor.MemberDescriptorContainer<YieldFunctionDescriptor>::declare(this);		
}

