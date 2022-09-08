#include "Teleporter.h"
#include "PlaceLauncher.h"
    
void Teleporter::teleportImpl(std::string url, std::string ticket, std::string script)
{
    PlaceLauncher::getPlaceLauncher().teleport(ticket, url, script);
}
