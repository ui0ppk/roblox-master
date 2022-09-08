#include "GuidRegistryService.h"

const char* const RBX::Network::sGuidRegistryService = "GuidRegistryService";

RBX::Network::GuidRegistryService::GuidRegistryService(void)
:registry(Registry::create())
{
}

RBX::Network::GuidRegistryService::~GuidRegistryService(void)
{
}
