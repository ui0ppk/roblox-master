#include "stdafx.h"

#include "v8datamodel/ReplicatedStorage.h"

using namespace RBX;

const char* const RBX::sReplicatedStorage = "ReplicatedStorage";

ReplicatedStorage::ReplicatedStorage(void)
{
	setName(sReplicatedStorage);
}
