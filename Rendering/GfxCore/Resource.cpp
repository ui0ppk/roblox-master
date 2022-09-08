#include "GfxCore/Resource.h"

#include "GfxCore/Device.h"

namespace RBX
{
namespace Graphics
{

Resource::Resource(Device* device)
    : device(device)
    , prev(NULL)
	, next(NULL)
{
	if (device->resourceListHead)
	{
		RBXASSERT(device->resourceListTail);

		prev = device->resourceListTail;
		device->resourceListTail->next = this;
		device->resourceListTail = this;
	}
	else
	{
		RBXASSERT(!device->resourceListTail);

		device->resourceListHead = this;
		device->resourceListTail = this;
	}
}

Resource::~Resource()
{
    if (prev)
		prev->next = next;
	else
	{
		RBXASSERT(device->resourceListHead == this);
		device->resourceListHead = next;
	}

    if (next)
		next->prev = prev;
	else
	{
		RBXASSERT(device->resourceListTail == this);
		device->resourceListTail = prev;
	}
}

void Resource::onDeviceLost()
{
}

void Resource::onDeviceRestored()
{
}

void Resource::setDebugName(const std::string& value)
{
    debugName = value;
}

}
}