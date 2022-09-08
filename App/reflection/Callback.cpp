#include "stdafx.h"

#include "reflection/Callback.h"
#include "reflection/object.h"

using namespace RBX;
using namespace RBX::Reflection;

CallbackDescriptor::CallbackDescriptor(ClassDescriptor& classDescriptor, const char* name, Attributes attributes, Security::Permissions security, bool async)
	: MemberDescriptor(classDescriptor, name, NULL, attributes, security)
    , async(async)
{
	// TODO: This could be moved up into MemberDescriptor if it were templated...
	classDescriptor.MemberDescriptorContainer<CallbackDescriptor>::declare(this);		
}

SyncCallbackDescriptor::SyncCallbackDescriptor(ClassDescriptor& classDescriptor, const char* name, Attributes attributes, Security::Permissions security)
	: CallbackDescriptor(classDescriptor, name, attributes, security, false)
{
}

void SyncCallbackDescriptor::setGenericCallbackHelper(DescribedBase* object, const GenericFunction& function) const
{
	shared_ptr<GenericFunction> ptr(new GenericFunction(function));
	setGenericCallback(object, ptr);
}

AsyncCallbackDescriptor::AsyncCallbackDescriptor(ClassDescriptor& classDescriptor, const char* name, Attributes attributes, Security::Permissions security)
	: CallbackDescriptor(classDescriptor, name, attributes, security, true)
{
}

void AsyncCallbackDescriptor::setGenericCallbackHelper(DescribedBase* object, const GenericFunction& function) const
{
	shared_ptr<GenericFunction> ptr(new GenericFunction(function));
	setGenericCallback(object, ptr);
}

void AsyncCallbackDescriptor::callGenericImpl(shared_ptr<AsyncCallbackDescriptor::GenericFunction> function, shared_ptr<Tuple> args,
    AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)
{
    try
    {
        (*function)(args, resumeFunction, errorFunction);
    }
    catch (const RBX::base_exception& e)
    {
        errorFunction(e.what());
    }
}
