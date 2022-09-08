#pragma once

#include "Item.h"
#include "Replicator.h"
#include "Util/ProgramMemoryChecker.h"

namespace RBX {
    namespace Network {

        class Replicator::HashItem : public Item
        {
            PmcHashContainer hashes;
            unsigned long long fuzzyToken;
            unsigned long long apiToken;
            unsigned long long prevApiToken;
        public:
            HashItem(Replicator* replicator, const PmcHashContainer* const hashes, unsigned long long fuzzyToken,
                unsigned long long apiToken, unsigned long long prevApiToken);

            /*implement*/ virtual bool write(RakNet::BitStream& bitStream);
        };

    }
}
