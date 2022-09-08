#pragma once
#include "boost/thread/mutex.hpp"

namespace RBX
{
    // This is designed to be an anti-tamper security reporting mechanism.
    // Add flag is based on the number of bits in the flag, and succeeds
    // with probability 1 - 0.5**N per transmitted packet.  This might
    // seem like an issue, but most of the checks will re-trigger an addition
    // meaning an exploiter is unlikely to be in the game for more than 30
    // seconds.  Attempts to modify the packet are very likely to fail.

    namespace Security
    {
        unsigned long long teaDecrypt(unsigned long long inTag);
        unsigned long long teaEncrypt(unsigned long long inTag);
    }

    namespace Tokens
    {
        union binaryTag
        {
            unsigned int asHalf[2];
            unsigned long long asFull;
        };
    }

    class ClientFuzzySecurityToken 
    {
        Tokens::binaryTag tag;
        Tokens::binaryTag prevTag;
        Tokens::binaryTag savedTag;
        boost::mutex tokenMutex;
    public:
        ClientFuzzySecurityToken(unsigned long long inTag);
        void set(unsigned long long inTag);

#ifdef WIN32
        __forceinline
#else
        inline
#endif
        void addFlagFast(unsigned long long flags)
        {
            tag.asFull |= flags;
        }

#ifdef WIN32
        __forceinline
#else
        inline
#endif
        void addFlagSafe(unsigned long long flags)
        {
            boost::mutex::scoped_lock lock(tokenMutex);
            tag.asFull |= flags;
        }
        unsigned long long crypt();
        unsigned long long getPrev()
        {
            return prevTag.asFull;
        }
    };

    // The server can check this.  Each '1' transmitted from the client has
    // a 50% chance of being a '0' here, thus it it fuzzy.
    class ServerFuzzySecurityToken
    {
        Tokens::binaryTag lastTag;
        Tokens::binaryTag tag;
        unsigned int ignoreFlags;
    public:
        ServerFuzzySecurityToken(unsigned long long inTag, unsigned int ignoreFlags = 0);
        void setLastTag(unsigned long long tag) { lastTag.asFull = tag;}
        unsigned long long decrypt(unsigned long long inTag);
    };

    namespace Tokens
    {
        extern ClientFuzzySecurityToken sendStatsToken;
        extern unsigned int simpleToken;
        extern ClientFuzzySecurityToken apiToken;
    }

}

