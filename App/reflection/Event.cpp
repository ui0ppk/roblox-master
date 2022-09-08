#include "stdafx.h"

#include "reflection/event.h"
#include "reflection/Object.h"

using namespace RBX;
using namespace RBX::Reflection;

EventDescriptor::EventDescriptor(ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes)
	:MemberDescriptor(classDescriptor, name, "Signals", attributes, security)
{
	// TODO: This could be moved up into MemberDescriptor if it were templated...
	classDescriptor.MemberDescriptorContainer<EventDescriptor>::declare(this);		
}

std::size_t hash_value(const Event& evt)
{
	std::size_t result = boost::hash<const EventDescriptor*>()(evt.getDescriptor());
	boost::hash_combine(result, evt.getInstance().get());
	return result;
}

RBX::Reflection::RemoteEventCommon::Attributes RBX::Reflection::RemoteEventCommon::Attributes::deprecated( Functionality flags, const MemberDescriptor* preferred )
{
	RemoteEventCommon::Attributes result(flags);
	result.isDeprecated = true;
	result.preferred = preferred;
	return result;
}

void EventSource::processRemoteEvent(const EventDescriptor& descriptor, const EventArguments& args, const SystemAddress& source)
{
    descriptor.fireEvent(this, args);
}

void EventSource::raiseEventInvocation(const EventDescriptor& descriptor, const EventArguments& args, const SystemAddress* target)
{
}