#include "stdafx.h"
#include "security/FuzzyTokens.h"
#include "VMProtect/VMProtectSDK.h"

namespace RBX
{
    unsigned long long Security::teaDecrypt(unsigned long long inTag)
    {  
        Tokens::binaryTag tag;
        tag.asFull = inTag;
        // Tiny Encryption Algorithm (TEA).  Used only for randomization. (decryption)
        uint32_t v0 = tag.asHalf[0];
        uint32_t v1 = tag.asHalf[1];
        const uint32_t delta = 0xD203172E;
        uint32_t k0 = 0x5A96E9DE;
        const uint32_t k1 = 0x7B80D8E4;
        const uint32_t k2 = 0xCC969B58;
        const uint32_t k3 = 0x2B99050C;
        uint32_t sum = 0x4062E5C0;
        for (int i = 0; i < 32; ++i)
        {
            v1 -= ((v0 << 9) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
            v0 -= ((v1 << 9) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
            sum -= delta;
        }
        tag.asHalf[0] = v0;
        tag.asHalf[1] = v1;
        return tag.asFull;
    }

    unsigned long long Security::teaEncrypt(unsigned long long inTag)
    {
        Tokens::binaryTag tag;
        tag.asFull = inTag;
        uint32_t v0 = tag.asHalf[0];
        uint32_t v1 = tag.asHalf[1];
        uint32_t k0 = 0x5A96E9DE;
        const uint32_t k1 = 0x7B80D8E4;
        const uint32_t k2 = 0xCC969B58;
        const uint32_t k3 = 0x2B99050C;
        const uint32_t delta = 0xD203172E;
        uint32_t sum = 0;
        for (int i = 0; i < 32; ++i)
        {
            sum += delta;
            v0 += ((v1 << 9) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
            v1 += ((v0 << 9) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        }
        tag.asHalf[0] = v0; 
        tag.asHalf[1] = v1;
        return tag.asFull;
    }

    ClientFuzzySecurityToken Tokens::sendStatsToken(0);
    unsigned int Tokens::simpleToken = 0;
    ClientFuzzySecurityToken Tokens::apiToken(0xFFFFFFFFFFFFFFFFULL);

    ClientFuzzySecurityToken::ClientFuzzySecurityToken(unsigned long long inTag)
    {
        tag.asFull = inTag;
    }

    unsigned long long ClientFuzzySecurityToken::crypt()
    {
        boost::mutex::scoped_lock lock(tokenMutex);
        prevTag = savedTag;
        tag.asFull = Security::teaEncrypt(tag.asFull);
        savedTag = tag;
        return tag.asFull;
    }

    ServerFuzzySecurityToken::ServerFuzzySecurityToken(unsigned long long inTag, unsigned int ignoreFlags)
        : ignoreFlags(ignoreFlags)
    {
        lastTag.asFull = inTag;
    }

    // There is current some difficult to find bug that allows one security packet
    // to be dropped or not sent.  This scheme was not designed to make this case
    // easy.
    unsigned long long ServerFuzzySecurityToken::decrypt(unsigned long long inTag)
    {
        // Attempt to see if this is the expected next in sequence
        unsigned long long decryptedValue = Security::teaDecrypt(inTag);
        unsigned long long decodedValue = decryptedValue ^ lastTag.asFull;
        if (!(decodedValue >> 32))
        {
            lastTag.asFull = inTag;
            return decodedValue;
        }

        // Set up LUT with only values where the client could set the flag.
        unsigned int oneHotLut[4] = {0,0,0,0};
        int popCnt = 0;
        const unsigned int lastHackFlags = lastTag.asFull & 0xFFFFFFFF;
        unsigned int ignoreCopy = ignoreFlags;
        for (int i = 0; i < 4; ++i)
        {
            // -x = ~x+1.  if x has consecutive 0's on the right side,  they become 1
            // +1 will cause a carry all the way until the first 0 (which was a 1 in x)
            // the carry stops there, so all bits to the left will be 0 -- the same as
            // they would be in x & ~x.  Thus the rightmost 1 is found efficiently.
            unsigned int rightmostOne = ((~ignoreCopy+1) & ignoreCopy);
            if (rightmostOne & ~lastHackFlags)
            {
                oneHotLut[popCnt] = rightmostOne;
                ++popCnt;
            }
            ignoreCopy &= ~rightmostOne;
        }

        // Disable this automatically if there are too many ignoreFlags.
        if ( (~ignoreCopy+1) & ignoreCopy)
        {
            lastTag.asFull = inTag;
            return 0;
        }

        // try up to all of the possibilities.
        for (int i = 0; i < (1 << popCnt); ++i)
        {
            // create the mask
            unsigned int mask = 0;
            for (int j = 0; j < popCnt; ++j)
            {
                if (i & (1 << j))
                {
                    mask |= oneHotLut[j];
                }
            }

            // create the missing data and check the msbs for tamper.
            unsigned long long testTag = Security::teaEncrypt(lastTag.asFull ^ mask);
            unsigned long long testDecode = (testTag ^ decryptedValue);
            unsigned int testCheck = testDecode >> 32;
            if (!testCheck)
            {
                lastTag.asFull = inTag;
                return testDecode;
            }
        }

        // can't test any more that this.
        lastTag.asFull = inTag;
        return decodedValue;
    }


}