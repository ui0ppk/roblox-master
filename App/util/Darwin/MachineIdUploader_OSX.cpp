#include "Util/MachineIdUploader.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <ifaddrs.h>

using namespace RBX;

// This is only needed to satisfy mac compiler:
const int MachineIdUploader::MacAddress::kBytesInMacAddress;

bool nameLooksLikeEthernet(const sockaddr_dl* dlAddress) {
    return dlAddress->sdl_nlen >= 2 &&
        dlAddress->sdl_data[0] == 'e' &&
        dlAddress->sdl_data[1] == 'n';
}

bool MachineIdUploader::fillMachineId(MachineId *out) {
    ifaddrs* addresses;
    
    if (getifaddrs(&addresses) == 0) {
        for (ifaddrs* current = addresses; current != NULL;
                current = current->ifa_next) {
            sockaddr_dl* dlAddress = (sockaddr_dl*)(current->ifa_addr);
            if (current->ifa_addr->sa_family == AF_LINK &&
                    nameLooksLikeEthernet(dlAddress) &&
                    dlAddress->sdl_type == IFT_ETHER) {
                out->macAddresses.resize(out->macAddresses.size() + 1);
                size_t bytesToCopy = std::min(MacAddress::kBytesInMacAddress, (int)dlAddress->sdl_alen);
                
                // mac address starts after name data (nlen bytes).
                char* source = dlAddress->sdl_data + dlAddress->sdl_nlen;
                for (size_t i = 0; i < bytesToCopy; ++i) {
                    out->macAddresses.back().address[i] = source[i];
                }
            }
        }
        
        freeifaddrs(addresses);

        return true;
    }
    
    return false;
}
