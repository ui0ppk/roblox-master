#include "stdafx.h"
#include "NetPmc.h"

// for teaDecrypt
#include "security/FuzzyTokens.h"

#include <stdlib.h>
#include <algorithm>

#include "Replicator.h"
#include "ServerReplicator.h"
#include "ClientReplicator.h"
#include "v8datamodel/DataModel.h"
#include "util/rbxrandom.h"

// Design notes from 4 December 2015:
// This is the Networked Program Memory Checker.
// In the first implementation, there was no easy way to distribute information to the
// RCC server.  However, the post-build release patcher did work.
//
// The flow works like this:
// (build)
// The patcher will generate a group of 128 different challenges -- different starting/ending/seed values
// These will be generated before the main PMC is run, as it will modify the .rdata section.
// These will be encrypted using salsa20.  This can be generated on the server.
// The results from each challenge will be generated and stored as well, but will be encrypted using tea.
//
// (server)
// The server will locally regenerate the decryption key sequence
// The server will send a selector + key to the game client
// This will be added to the "challengesInFlight"
//
// (client)
// The client will decrypt the info for this challenge
// The hash will be performed
// The client will send the selector, its response, and the encrypted correct response to the server.
//
// (server)
// The server will decrypt the correct response and compare it to the given response
// The inFlight list will be scanned and the entry removed.
// 
// if the inFlight list will have a limit on how large it can grow.
//
// This security mechanism will also add some checks to avoid running in places where it is likely to
// be debugged.
//
// The choice of tea/salsa was based on the existence of code online.
DYNAMIC_FASTINTVARIABLE(HashConfigP3, 4)     // players
DYNAMIC_FASTINTVARIABLE(HashConfigP4, 1000)  // parts
DYNAMIC_FASTINTVARIABLE(HashConfigP5, 1)     // down
DYNAMIC_FASTINTVARIABLE(HashConfigP6, 1)     // up
DYNAMIC_FASTINTVARIABLE(HashConfigP8, 64)    // pending limit

namespace RBX {
namespace Security{
std::vector<uint32_t> hackFlagVector;
const volatile uint32_t kHackFlagVectorDecrypt = 1; // might not be random


#ifndef RBX_STUDIO_BUILD
const volatile NetPmcChallenge kChallenges[kNumChallenges] = {};

uint32_t netPmcHashCheck(const NetPmcChallenge& challenge)
{
    const uint32_t kMult2 = 0xBADA55;
    const uint32_t kMult1 = 0xC0FFEE11;
    unsigned int v1 = challenge.seed;
    unsigned int v2 = challenge.seed;
    unsigned int v3 = challenge.seed;
    unsigned int v4 = challenge.seed;
    const uintptr_t* p = reinterpret_cast<const uintptr_t*>(challenge.base);

    for (size_t i = 0; i < challenge.size/16; ++i)
    {
        v1 += *(p+0) * kMult2;
        v2 += *(p+1) * kMult2;
        v3 += *(p+2) * kMult2;
        v4 += *(p+3) * kMult2;
        v1 = _rotl(v1, 17);
        v2 = _rotl(v2, 17);
        v3 = _rotl(v3, 17);
        v4 = _rotl(v4, 17);
        v1 *= kMult1;
        v2 *= kMult1;
        v3 *= kMult1;
        v4 *= kMult1;
        p+=4;
    }
    return v1^v2+v3-v4;
}
#endif

#ifdef RBX_RCC_SECURITY
std::vector<NetPmcChallenge> netPmcKeys = generateNetPmcKeys();

NetPmcServer::NetPmcServer() : // some are set to safe, but odd defaults
    isGameCreator(true)
    , userHasActivity(false)
    , gameHasManyParts(false)
    , gameHasManyPlayers(false)
    , challengeIdx(0)
    , challengesRecv(0)
    , challengesSent(0)
{
    challenges.resize(kNumChallenges);
    for (size_t i = 0; i < kNumChallenges; ++i)
    {
        challenges[i] = i;
    }
    std::random_shuffle(challenges.begin(), challenges.end());
}

// the intent it to make it much harder to detect this mechanism by making it
// only work in places that are actually games.
bool NetPmcServer::canSendChallenge(const RBX::Network::Replicator* rep) /*non-const*/
{
    auto dm = DataModel::get(rep);
    if (dm)
    {
        auto remotePlayer = rep->getRemotePlayer();
        if (remotePlayer)
        {
            isGameCreator = (dm->getCreatorID() == remotePlayer->getUserID());
            if (!isGameCreator && !gameHasManyPlayers)
            {
                // recheck to see if there are enough players
                gameHasManyPlayers = (dm->getNumPlayers() > DFInt::HashConfigP3);
            }
            if (!isGameCreator && !gameHasManyParts)
            {
                // recheck to see if there are enough parts
                gameHasManyParts = (dm->getNumPartInstances() > DFInt::HashConfigP4);
            }
        }
    }
    userHasActivity = (rep->stats().kiloBytesReceivedPerSecond >= DFInt::HashConfigP5) && (rep->stats().kiloBytesSentPerSecond >= DFInt::HashConfigP6);
    return (!isGameCreator && gameHasManyPlayers && gameHasManyParts && userHasActivity);
}

unsigned int NetPmcServer::generateDebugInfo(const RBX::Network::Replicator* rep, uint32_t& sent, uint32_t& recv, uint32_t& pending) const
{
    unsigned int debugInfo = 0;

    debugInfo |= (isGameCreator ? (1<<0) : 0);
    debugInfo |= (gameHasManyPlayers ? (1<<1) : 0);
    debugInfo |= (gameHasManyParts ? (1<<2) : 0);
    debugInfo |= ((rep->stats().kiloBytesReceivedPerSecond >= DFInt::HashConfigP5) ? (1<<3) : 0);
    debugInfo |= ((rep->stats().kiloBytesSentPerSecond >= DFInt::HashConfigP6) ? (1<<4) : 0);

    debugInfo |= ((challengesRecv > 1) ? (1<<5) : 0);
    debugInfo |= ((challengesSent > 1) ? (1<<6) : 0);

    debugInfo |= ((challengesRecv > 32) ? (1<<7) : 0);
    debugInfo |= ((challengesSent > 32) ? (1<<8) : 0);

    debugInfo |= ((challengesRecv > 128) ? (1<<9) : 0);
    debugInfo |= ((challengesSent > 128) ? (1<<10) : 0);

    sent = challengesSent;
    recv = challengesRecv;
    pending = challengesInFlight.size();

    return debugInfo;
}

bool NetPmcServer::tooManyPending() const
{
    return (challengesInFlight.size() > static_cast<unsigned int>(DFInt::HashConfigP8));
}

uint8_t NetPmcServer::getRandomChallenge()
{
    if (challengeIdx == challenges.size()-1)
    {
       std::random_shuffle(challenges.begin(), challenges.end());
       challengeIdx = 0;
    }
    return challenges[challengeIdx++];
}

bool NetPmcServer::sendChallenge(uint8_t idx)
{
    challengesSent++;
    boost::mutex::scoped_lock lock(flightMutex);
    challengesInFlight.push_back(idx);

    return true;
}

bool NetPmcServer::removeFromList(uint8_t challengeNum)
{
    challengesRecv++;
    boost::mutex::scoped_lock lock(flightMutex);
    auto loc = std::find(challengesInFlight.begin(), challengesInFlight.end(), challengeNum);
    if (loc != challengesInFlight.end())
    {
        challengesInFlight.erase(loc);
        return true;
    }
    return false;
}

bool NetPmcServer::checkResult(uint8_t idx, uint32_t response, uint64_t correct) const
{
    uint64_t correctDecrypted = RBX::Security::teaDecrypt(correct);
    if (response != (correctDecrypted&0xFFFFFFFF))
    {
        // hash mismatch
        return false;
    }
    if (correctDecrypted>>32 != idx)
    {
        // attempted replay attack
        return false;
    }
    return true;
}
#endif

// From github: https://github.com/andres-erbsen/salsa20/blob/master/salsa20.c

// rotate x to left by n bits, the bits that go over the left edge reappear on the right
#define R(x,n) (((x) << (n)) | ((x) >> (32-(n))))

// addition wraps modulo 2^32
// the choice of 7,9,13,18 "doesn't seem very important" (spec)
#define quarter(a,b,c,d) do {\
    b ^= R(d+a, 7);\
    c ^= R(a+b, 9);\
    d ^= R(b+c, 13);\
    a ^= R(c+d, 18);\
} while (0)

void salsa20_words(uint32_t *out, uint32_t in[16]) {
    uint32_t x[4][4];
    int i;
    for (i=0; i<16; ++i) x[i/4][i%4] = in[i];
    for (i=0; i<10; ++i) { // 10 double rounds = 20 rounds
        // column round: quarter round on each column; start at ith element and wrap
        quarter(x[0][0], x[1][0], x[2][0], x[3][0]);
        quarter(x[1][1], x[2][1], x[3][1], x[0][1]);
        quarter(x[2][2], x[3][2], x[0][2], x[1][2]);
        quarter(x[3][3], x[0][3], x[1][3], x[2][3]);
        // row round: quarter round on each row; start at ith element and wrap around
        quarter(x[0][0], x[0][1], x[0][2], x[0][3]);
        quarter(x[1][1], x[1][2], x[1][3], x[1][0]);
        quarter(x[2][2], x[2][3], x[2][0], x[2][1]);
        quarter(x[3][3], x[3][0], x[3][1], x[3][2]);
    }
    for (i=0; i<16; ++i) out[i] = x[i/4][i%4] + in[i];
}

// inputting a key, message nonce, keystream index and constants to that transormation
void salsa20_block(uint8_t *out, uint8_t key[32], uint64_t nonce, uint64_t index) {
    static const char c[16] = "expand 32byte k"; // arbitrary constant
#define LE(p) ( _byteswap_ulong(*reinterpret_cast<const unsigned long*>(p)) )
    uint32_t in[16] = {LE(c),            LE(key),    LE(key+4),        LE(key+8),
        LE(key+12),       LE(c+4),    nonce&0xffffffff, nonce>>32,
        index&0xffffffff, index>>32,  LE(c+8),          LE(key+16),
        LE(key+20),       LE(key+24), LE(key+28),       LE(c+12)};
#undef LE
    uint32_t wordout[16];
    salsa20_words(wordout, in);
    int i;
    for (i=0; i<64; ++i) out[i] = 0xff & (wordout[i/4] >> (8*(i%4)));
}

// enc/dec: xor a message with transformations of key, a per-message nonce and block index
void salsa20(uint8_t *message, uint64_t mlen, uint8_t key[32], uint64_t nonce) {
    int i;
    uint8_t block[64];
    for (i=0; i<mlen; i++) {
        if (i%64 == 0) salsa20_block(block, key, nonce, i/64);
        message[i] ^= block[i%64];
    }
}

}
}

