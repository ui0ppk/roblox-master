#include "stdafx.h"

#include "v8datamodel/ServerStorage.h"
#include "V8DataModel/GameBasicSettings.h"
#include "network/Players.h"

using namespace RBX;

const char* const RBX::sServerStorage = "ServerStorage";

ServerStorage::ServerStorage(void)
{
	setName(sServerStorage);
}

bool ServerStorage::askAddChild(const Instance* instance) const
{
	return RBX::Network::Players::backendProcessing(this) || RBX::GameBasicSettings::singleton().inStudioMode();
}