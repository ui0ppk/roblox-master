/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include <boost/circular_buffer.hpp>
#include "G3DCore.h"
#include "rbx/rbxTime.h"
#include "boost/thread/mutex.hpp"
#include "Math.h"

namespace RBX {

#define MH_NUM_MAX_NODES 80
#define MH_MIN_PRECISION 0.01f
#define MH_TOLERABLE_COMPRESSION_ERROR 1.f


    typedef unsigned char uint8_t;
    typedef signed char int8_t;

    class MovementHistory 
    {

    public:
        struct DeltaCompressedTranslation
        {
            // in terms of MIN_PRECISION
            uint8_t precisionLevel;
            int8_t dX;
            int8_t dY;
            int8_t dZ;
        };

        struct MovementNode
        {
            DeltaCompressedTranslation translation;
            uint8_t delta2Ms;
            MovementNode()
            {
                setZero();
            }
            MovementNode(const CoordinateFrame& newCFrame, const CoordinateFrame& oldCFrame, float deltaSecs)
            {
                Vector3 delta = newCFrame.translation - oldCFrame.translation;
                compress(delta, *this);
                if (deltaSecs < 0.f)
                {
                    delta2Ms = 0;
                }
                else if (deltaSecs > 0.510f)
                {
                    delta2Ms = 255; // overflow, use 255 to indicate the long gap
                }
                else
                {
                    delta2Ms = (uint8_t)(deltaSecs*500.f);
                }
            }
            bool isZero() const
            {
                return translation.dX == 0 && translation.dY == 0 && translation.dZ == 0;
            }
            void setZero()
            {
                translation.precisionLevel = 0;
                translation.dX = 0;
                translation.dY = 0;
                translation.dZ = 0;
                delta2Ms = 0;
            }
            // rotation will be estimated by interpolation
        };

        static const MovementHistory& getDefaultHistory()
        {
            static CoordinateFrame zeroCFrame;
            static Velocity zeroVelocity;
            static MovementHistory defaultMovementHistory(zeroCFrame, zeroVelocity, Time());
            return defaultMovementHistory;
        }

        MovementHistory(const CoordinateFrame& cFrame, const Velocity& velocity, const Time& timeStamp);
        ~MovementHistory()
        {}

        void clearNodeHistory();
        void addNode(const CoordinateFrame& cFrame, const Velocity& velocity, const Time& timeStamp);

        size_t getNumNodes() const {return size;}
        bool hasHistory(float accumulatedError) const;
        void getMovementNodeList(const Time& lastCutOffTime, const Time& currentCutOffTime, std::deque<MovementNode>& result, bool crossPacketCompression, const CoordinateFrame& lastSentCFrame, CoordinateFrame& outCalculatedBaselineCFrame, Vector3& outCalculatedLinearVelocity) const;
        const CoordinateFrame& getBaselineCFrame() const {return baselineCFrame;}
        const Velocity& getBaselineVelocity() const {return baselineVelocity;}

        static float decompress(int8_t v, uint8_t precisionLevel);
        static void decompress(MovementNode node, Vector3& outTranslation);

        const Time& getLastUpdateTime() const {return lastUpdateTime;}
        float getTimeSpan() const {return timeSpanSec;}

        static float getSecFrom2Ms(uint8_t delta2MS)
        {
            return ((float)delta2MS)/500.f;
        }

    private:
        CoordinateFrame baselineCFrame;
        Velocity baselineVelocity;
        Time lastUpdateTime;
        float timeSpanSec;
        int checksum;

        MovementNode movementNodes[MH_NUM_MAX_NODES];
        size_t startIndex;
        size_t size;
        void popFront();
        void pushBack(MovementNode node);
        MovementNode concatNode(size_t lastIndex, size_t numNodesToConcat) const;

        static int8_t compress(float v, uint8_t precisionLevel);
        static void compress(Vector3 translation, MovementNode& outMovementNode);
    };

} // namespace
