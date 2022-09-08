/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/MovementHistory.h"
#include "v8datamodel/PartInstance.h"
#include "v8world/Primitive.h"
#include "../NetworkSettings.h"

namespace RBX {

DYNAMIC_FASTINTVARIABLE(MaxNodesPerPathPacket, 3)
DYNAMIC_FASTINTVARIABLE(NodeIntervalCapMS, 100)

    MovementHistory::MovementHistory(const CoordinateFrame& cFrame, const Velocity& velocity, const Time& timeStamp)
        : lastUpdateTime(timeStamp)
        , baselineCFrame(cFrame)
        , baselineVelocity(velocity)
        , timeSpanSec(0.f)
        , startIndex(0)
        , size(0)
        , checksum(0)
    {
    }

    bool MovementHistory::hasHistory(float accumulatedError) const
    {
        if (accumulatedError > MH_TOLERABLE_COMPRESSION_ERROR)
        {
            // too much accumulated error, let's just send anything we have
            return true;
        }
        else if (checksum == 0)
        {
            return false;
        }
        else
        {
            // has history
            return true;
        }
    }

    void MovementHistory::clearNodeHistory()
    {
        startIndex = 0;
        size = 0;
        timeSpanSec = 0.f;
        lastUpdateTime = Time();//Time::now<Time::Fast>();
        for (size_t i=0; i<MH_NUM_MAX_NODES; i++)
        {
            movementNodes[i].setZero();
        }
        checksum = 0;
    }

    void MovementHistory::getMovementNodeList(const Time& lastCutOffTime, const Time& currentCutOffTime, std::deque<MovementHistory::MovementNode>& result, bool crossPacketCompression, const CoordinateFrame& lastSendCFrame, CoordinateFrame& outCalculatedBaselineCFrame, Vector3& outCalculatedLinearVelocity) const
    {
        RBXASSERT(startIndex < MH_NUM_MAX_NODES);
        RBXASSERT(size <= MH_NUM_MAX_NODES);
        RBXASSERT(result.size() == 0);
        int numNodeToSkip = 0;
        float requestedTimeInterval = (currentCutOffTime - lastCutOffTime).seconds();

        if (requestedTimeInterval < 0 || timeSpanSec <= 0.f || lastCutOffTime.isZero())
        {
            return;
        }

		if (lastUpdateTime < lastCutOffTime)
			return;

        if (size > 0)
        {
            float estimatedNodeInterval = timeSpanSec / size;
            if (estimatedNodeInterval > 0.f)
            {
                numNodeToSkip = Math::iFloor(requestedTimeInterval / estimatedNodeInterval / (DFInt::MaxNodesPerPathPacket + 1));
				float nodeIntervalCap = (float)DFInt::NodeIntervalCapMS / 1000.0f; // convert to seconds
                if (numNodeToSkip * estimatedNodeInterval > nodeIntervalCap)
                {
                    // don't be too aggressive with node concatenation
                    numNodeToSkip = Math::iFloor(nodeIntervalCap /estimatedNodeInterval);
                }
                RBXASSERT(numNodeToSkip * estimatedNodeInterval <= nodeIntervalCap);
            }
        }

        float totalTime = 0.f;

        if (crossPacketCompression)
        {
            RBXASSERT(size > 0);

            // create a node for the baseline cframe based on the baseline cframe of previous packet
            MovementNode baselineCFrameDeltaNode(lastSendCFrame, baselineCFrame, 0.f);
            result.push_back(baselineCFrameDeltaNode);
            // reconstruct the last cframe based on compressed value
            Vector3 baselineCFrameTranslationDelta;
            decompress(baselineCFrameDeltaNode, baselineCFrameTranslationDelta);
            outCalculatedBaselineCFrame = lastSendCFrame;
            outCalculatedBaselineCFrame.translation -= baselineCFrameTranslationDelta;
            RBXASSERT(baselineCFrameDeltaNode.translation.precisionLevel == 255 || (outCalculatedBaselineCFrame.translation-baselineCFrame.translation).magnitude() < (baselineCFrameDeltaNode.translation.precisionLevel+1)*MH_MIN_PRECISION*2);
        }

        if (size > 0)
        {
            size_t lastNodeIndex = startIndex+size-1;
            if (lastNodeIndex >= MH_NUM_MAX_NODES)
            {
                lastNodeIndex -= MH_NUM_MAX_NODES;
            }
            result.push_back(movementNodes[lastNodeIndex]); // last node, always send, we need it to (relatively) accurately calculate the velocity
            totalTime += getSecFrom2Ms(movementNodes[lastNodeIndex].delta2Ms);

            size_t numNodeProceeded = 1;
            while (totalTime == 0.f && size > numNodeProceeded)
            {
                // we get a node with no time, this is not good for calculating the velocity. Let's merge an extra node.
                result.pop_back();
                MovementNode node = concatNode(lastNodeIndex, ++numNodeProceeded);
                result.push_back(node);
                totalTime += getSecFrom2Ms(node.delta2Ms);
            }

            if (totalTime == 0.f)
            {
                // not a valid path, abort
                result.clear();
                return;
            }

            // calculate the linear velocity
            Vector3 lastNodeTranslationDelta;
            decompress(movementNodes[lastNodeIndex], lastNodeTranslationDelta);
            outCalculatedLinearVelocity = lastNodeTranslationDelta / totalTime;
            RBXASSERT(!Math::isNanInfVector3(outCalculatedLinearVelocity));

            for (size_t i=numNodeProceeded; i<size; i++)
            {
                size_t cursor = startIndex+(size-i-1);
                if (cursor >= MH_NUM_MAX_NODES)
                {
                    cursor -= MH_NUM_MAX_NODES;
                }
                // concat nodes
                if (numNodeToSkip > 0)
                {
                    size_t numNodeToConcat = numNodeToSkip+1;
                    if (numNodeToConcat > size-i)
                    {
                        // can't concat that many nodes, just concat the last few
                        numNodeToConcat = size-i;
                    }
                    if (numNodeToConcat > 1)
                    {
                        MovementNode node = concatNode(cursor, numNodeToConcat);
                        result.push_back(node);
                        i+=(numNodeToConcat-1);
                        totalTime += getSecFrom2Ms(node.delta2Ms);
                    }
                    else
                    {
                        result.push_back(movementNodes[cursor]);
                        totalTime += getSecFrom2Ms(movementNodes[cursor].delta2Ms);
                    }
                }
                else
                {
                    result.push_back(movementNodes[cursor]);
                    totalTime += getSecFrom2Ms(movementNodes[cursor].delta2Ms);
                }
                if (totalTime > requestedTimeInterval)
                {
                    // overflow, remove the last node and exit the loop
                    totalTime -= getSecFrom2Ms(result.back().delta2Ms);
                    result.pop_back();
                    // TODO reduce the interval and try to add the leftover
                    break;
                }
            }
        }
    }

    void MovementHistory::popFront()
    {
        RBXASSERT(startIndex < MH_NUM_MAX_NODES);
        RBXASSERT(size <= MH_NUM_MAX_NODES);
        if (size > 0)
        {
            float deltaTime = getSecFrom2Ms(movementNodes[startIndex].delta2Ms);
            checksum -= (!movementNodes[startIndex].isZero());
            timeSpanSec -= deltaTime;
            if (timeSpanSec < 0.f)
            {
                RBXASSERT(size == 1);
                timeSpanSec = 0.f;
            }
            movementNodes[startIndex].setZero();
            size--;
            if (size == 0)
            {
                timeSpanSec = 0.f;
            }
            startIndex++;
            if (startIndex >= MH_NUM_MAX_NODES)
            {
                startIndex -= MH_NUM_MAX_NODES;
            }
        }
    }

    void MovementHistory::pushBack(MovementNode node)
    {
        RBXASSERT(startIndex < MH_NUM_MAX_NODES);
        RBXASSERT(size <= MH_NUM_MAX_NODES);
        if (size == MH_NUM_MAX_NODES)
        {
            // overflowing, pop the first node
            popFront();
            RBXASSERT(size == MH_NUM_MAX_NODES-1);
        }
        size_t newNodeIndex = startIndex + size;
        if (newNodeIndex >= MH_NUM_MAX_NODES)
        {
            newNodeIndex -= MH_NUM_MAX_NODES;
        }
        movementNodes[newNodeIndex] = node;
        size++;
        timeSpanSec += getSecFrom2Ms(node.delta2Ms);
        checksum += (!node.isZero());
        RBXASSERT(size <= MH_NUM_MAX_NODES);
    }

    MovementHistory::MovementNode MovementHistory::concatNode(size_t lastIndex, size_t numNodesToConcat) const
    {
        RBXASSERT(numNodesToConcat <= size);
        RBXASSERT(lastIndex < MH_NUM_MAX_NODES);
        Vector3 resultVector;
        Vector3 tempVector;
        size_t totalTime = 0;
        for (size_t i=0; i<numNodesToConcat; i++)
        {
            size_t curIndex;
            if (i > lastIndex)
            {
                curIndex = lastIndex + MH_NUM_MAX_NODES - i;
            }
            else
            {
                curIndex = lastIndex - i;
            }
            
            decompress(movementNodes[curIndex], tempVector);
            resultVector += tempVector;
            totalTime += movementNodes[curIndex].delta2Ms;
        }
        MovementNode resultNode;
        compress(resultVector, resultNode);
        if (totalTime > 255)
        {
            totalTime = 255;
        }
        resultNode.delta2Ms = totalTime;
        return resultNode;
    }

    void MovementHistory::addNode(const CoordinateFrame& cFrame, const Velocity& velocity, const Time& timeStamp)
    {
        baselineVelocity = velocity;
        if (lastUpdateTime.isZero())
        {
            // first node, just set last cframe
            lastUpdateTime = timeStamp;
            baselineCFrame = cFrame;
            return;
        }

        double timeElapsed = (timeStamp - lastUpdateTime).seconds();

        // create and add the new node
        MovementNode newNode(cFrame, baselineCFrame, timeElapsed);
        pushBack(newNode);

        lastUpdateTime = timeStamp;
        baselineCFrame = cFrame;
    }

    int8_t MovementHistory::compress(float v, uint8_t precisionLevel)
    {
        float precision = MH_MIN_PRECISION*(float)(precisionLevel+1);
        float delta = v / precision;
        if(delta < -128.f)
        {
            delta = -128.f;
        }
        else if (delta > 127.f)
        {
            delta = 127.f;
        }
        return (int8_t)delta;
    }

    void MovementHistory::compress(Vector3 delta, MovementNode& outMovementNode)
    {
        float primeValue = Math::maxAxisLength(delta);
        float optimalPrecision = primeValue / 128.f;
        float precisionLevel = optimalPrecision / MH_MIN_PRECISION;
        if (precisionLevel < 1.f)
        {
            outMovementNode.translation.precisionLevel = 0;
        }
        else if (precisionLevel > 255.f)
        {
            outMovementNode.translation.precisionLevel = 255;
        }
        else
        {
            outMovementNode.translation.precisionLevel = (uint8_t)(precisionLevel);
        }
        outMovementNode.translation.dX = compress(delta.x, outMovementNode.translation.precisionLevel);
        outMovementNode.translation.dY = compress(delta.y, outMovementNode.translation.precisionLevel);
        outMovementNode.translation.dZ = compress(delta.z, outMovementNode.translation.precisionLevel);
    }

    float MovementHistory::decompress(int8_t v, uint8_t precisionLevel)
    {
        return (float)(v*(precisionLevel+1))*MH_MIN_PRECISION;
    }

    void MovementHistory::decompress(MovementNode node, Vector3& outTranslation)
    {
        outTranslation.x = decompress(node.translation.dX, node.translation.precisionLevel);
        outTranslation.y = decompress(node.translation.dY, node.translation.precisionLevel);
        outTranslation.z = decompress(node.translation.dZ, node.translation.precisionLevel);
    }
} // namespace
