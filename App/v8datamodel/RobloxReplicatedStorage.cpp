#include "stdafx.h"

#include "v8datamodel/RobloxReplicatedStorage.h"

using namespace RBX;

const char* const RBX::sRobloxReplicatedStorage = "RobloxReplicatedStorage";

RobloxReplicatedStorage::RobloxReplicatedStorage(void)
{
	setName(sRobloxReplicatedStorage);
	setRobloxLocked(true);
}
