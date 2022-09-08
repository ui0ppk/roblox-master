/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "RoundRobinPhysicsSender.h"
#include "Replicator.h"
#include "Compressor.h"

#include "Network/Player.h"

#include "GetTime.h"
#include "RakPeerInterface.h"
#include "NetworkSettings.h"
#include "ConcurrentRakPeer.h"
#include "Client.h"

#include "V8World/SendPhysics.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PhysicsService.h"

#include "raknet/Source/RakNetStatistics.h"

using namespace RBX;
using namespace RBX::Network;

SYNCHRONIZED_FASTFLAG(PhysicsPacketSendWorldStepTimestamp)

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

RoundRobinPhysicsSender::RoundRobinPhysicsSender(Replicator& replicator)
: PhysicsSender(replicator)
, lastCharacterSendTime(Time::now<Time::Fast>())
{
}


void RoundRobinPhysicsSender::sendPhysicsData(RakNet::BitStream& bitStream, const Assembly* assembly)
{
	RBXASSERT(assembly);

	if (assembly)
	{
        senderStats = NULL;
        if (replicator.settings().trackPhysicsDetails)
        {
            senderStats = &(replicator.replicatorStats.physicsSenderStats);
        }

		const PartInstance* part = PartInstance::fromConstPrimitive(assembly->getConstAssemblyPrimitive());

		if (canSend(part, assembly, bitStream))
		{
			Player* player = replicator.findTargetPlayer();
			ModelInstance* m = player ? player->getCharacter() : 0;
			bool detailed = m && m->isAncestorOf(part);

			if (replicator.isStreamingEnabled())
			{
				bitStream << false; // token: not done with packet
				bitStream << false; // not CFrame only

				if (replicator.trySerializeId(bitStream, part))
				{
					RakNet::BitSize_t characterBytes = bitStream.GetNumberOfBytesUsed();
                    CoordinateFrame cFrame = part->getCoordinateFrame();
                    float accumulatedError;
					sendMechanism(bitStream, part, detailed, currentStepTimestamp, cFrame, accumulatedError, NULL);
					if (senderStats && detailed) {
						senderStats->details.characterAnim.increment();
						senderStats->details.characterAnimSize.sample(bitStream.GetNumberOfBytesUsed() - characterBytes);
					}
				}
				else
                {
					replicator.serializeId(bitStream, NULL); // token: done with this mechanism
                }
			}
			else if (replicator.trySerializeId(bitStream, part))
			{			
				RakNet::BitSize_t characterBytes = bitStream.GetNumberOfBytesUsed();
                CoordinateFrame cFrame = part->getCoordinateFrame();
                float accumulatedError;
				sendMechanism(bitStream, part, detailed, currentStepTimestamp, cFrame, accumulatedError, NULL);
				if (senderStats && detailed) {
					senderStats->details.characterAnim.increment();
					senderStats->details.characterAnimSize.sample(bitStream.GetNumberOfBytesUsed() - characterBytes);
				}
			}
		}
	}
}


const SimJob* RoundRobinPhysicsSender::findTargetPlayerCharacterSimJob()
{
	if (const Player* player = replicator.findTargetPlayer())
	{
		if (const Primitive* primitive = player->getConstCharacterRoot())
		{
			return SimJob::getConstSimJobFromPrimitive(primitive);
		}
	}
	return NULL;
}

class RoundRobinPhysicsSender::JobSender : public boost::noncopyable
{
	// Hacky way to keep track of the first object sent, since it may be sent again after reportClosestMechanism
	const Assembly* firstA;
	const SimJob* firstM;

	RoundRobinPhysicsSender& sender;
	ConcurrentRakPeer* rakPeer;
	const int maxStreamSize;

	shared_ptr<RakNet::BitStream> bitStream;
	PacketPriority packetPriority;

	unsigned int emptySize;
	

	void openPacket()
	{
		bitStream.reset(new RakNet::BitStream(maxStreamSize));
		*bitStream << (unsigned char) ID_TIMESTAMP;
		RakNet::Time now = RakNet::GetTime();
		*bitStream << now;
		*bitStream << (unsigned char) ID_PHYSICS;
		emptySize = bitStream->GetNumberOfBitsUsed();
		++packetCount;
		itemCount = 0;

	}

	void openPacketPhysicsOffset(RakNet::Time& newTimeStamp)
	{
		bitStream.reset(new RakNet::BitStream(maxStreamSize));
		*bitStream << (unsigned char) ID_TIMESTAMP;
		RakNet::Time now = RakNet::GetTime();
		*bitStream << now;
		*bitStream << (unsigned char) ID_PHYSICS;
		*bitStream << newTimeStamp;
		emptySize = bitStream->GetNumberOfBitsUsed();
		++packetCount;
		itemCount = 0;

	}

	void closePacket()
	{
		if (bitStream->GetNumberOfBitsUsed()>emptySize)
		{
			// Packet "end" tag
			if (sender.replicator.isStreamingEnabled())
				*bitStream << true;
			else
				sender.replicator.serializeId(*bitStream, NULL);

			// Send ID_PHYSICS
            ReplicatorStats::PhysicsSenderStats& physStats =
                sender.replicator.physicsSenderStats();
			rakPeer->Send(bitStream, packetPriority, UNRELIABLE, PHYSICS_CHANNEL, sender.replicator.remotePlayerId, false);
			physStats.physicsPacketsSent.sample();
			physStats.physicsPacketsSentSmooth.sample();
			physStats.physicsPacketsSentSize.sample(bitStream->GetNumberOfBytesUsed());

			sender.itemsPerPacket.sample(itemCount);
		}
		bitStream.reset();
	}
public:
	int packetCount;
	unsigned int itemCount;
	
	JobSender(RoundRobinPhysicsSender& sender, ConcurrentRakPeer *peer)
		:sender(sender)
		,rakPeer(peer)
		,firstA(NULL)
		,firstM(NULL)
		,packetPriority(sender.replicator.settings().getPhysicsSendPriority())
		,packetCount(0)
		,itemCount(0)
		,maxStreamSize(sender.replicator.getPhysicsMtuSize())
	{
	}
	void close()
	{
		if (bitStream)
			closePacket();
	}
	~JobSender()
	{
		close();
	}

	// Returns true if it wants another sample
	bool report(const SimJob& simJob)
	{
		if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
		{
			float timeOffsetDueToPhysics = 0.0f;
			Workspace* w = ServiceProvider::find<RBX::Workspace>(&sender.replicator);
			if (w)
			{
				timeOffsetDueToPhysics = w->getWorld()->getUpdateExpectedStepDelta();		
			}
			
			// Generates timestamp for this frame, minus a small offset based on
			// missed or extra world steps taken.
			RakNet::Time passStamp = (RakNet::Time)((TaskScheduler::singleton().lastCyclcTimestamp +  Time::Interval(sender.replicator.getRakOffsetTime() - timeOffsetDueToPhysics)).timestampSeconds()* 1000.0);
			
			if (!bitStream)
				openPacketPhysicsOffset(passStamp);
		}
		else
		{
			if (!bitStream)
				openPacket();
		}
		
		sender.sendPhysicsData(*bitStream, simJob.getConstAssembly());
		
		const int newSize = bitStream->GetNumberOfBytesUsed();
		
		itemCount++;

		return newSize < maxStreamSize;
	}

	bool operator()(SimJob& m)
	{
		return report(m);
	}
};


void RoundRobinPhysicsSender::step()
{	
	if (Instance::fastDynamicCast<Client>(replicator.getParent()))
	{
		float bufferHealth = replicator.rakPeer->GetBufferHealth();
		if ((Time::nowFast() - lastBufferCheckTime).seconds() > 3.0)
		{
			if (bufferHealth <= 0.5)
			{
				// buffer is growing, reduce packets per step
				if (sendPacketsPerStep > 1) // don't go below 1
					sendPacketsPerStep--;
			}
			else if (bufferHealth >= 0.9)
			{
				// buffer health is good, push more data
				Workspace* w = ServiceProvider::find<RBX::Workspace>(&replicator);
				if (w)
				{
					if ((itemsPerPacket.value() * sendPacketsPerStep) < w->getWorld()->getSendPhysics()->getNumSimJobs())
						sendPacketsPerStep++;
				}
			}

			lastBufferCheckTime = Time::nowFast();
		}
	}
}

int RoundRobinPhysicsSender::sendPacket(int maxPackets, PacketPriority packetPriority,
		ReplicatorStats::PhysicsSenderStats* stats)
{
	if (replicator.getBufferCountAvailable(maxPackets, packetPriority) <= 0)
		return 0;

	JobSender jobSender(*this, replicator.rakPeer.get());

	// TODO: Check timer and feed these gradually?
	Workspace* w = ServiceProvider::find<RBX::Workspace>(&replicator);
	if (w)
	{
		const SimJob* characterSimJob = findTargetPlayerCharacterSimJob();

		if (characterSimJob)
		{
			// For a minimal level of responsiveness, occasionally send the character with a high priority
			const Time t = Time::now<Time::Fast>();
			if (t > lastCharacterSendTime + Time::Interval(0.1))
			{
				jobSender.report(*characterSimJob);
				lastCharacterSendTime = t;
			}
			else
			{
				characterSimJob = NULL;	// don't ignore me below
			}
		}

		int numSimJobs = w->getWorld()->getSendPhysics()->getNumSimJobs();
		int numSimJobsReported = 0;

		for (int i = 0; i < maxPackets; i++)
		{
			numSimJobsReported += w->getWorld()->getSendPhysics()->reportSimJobs(jobSender, jobStagePos, characterSimJob, numSimJobs-numSimJobsReported);

			if (numSimJobsReported >= numSimJobs)
				break;

			jobSender.close();
		}
	}
	
	// jobSend close automatically on destruction

	return jobSender.packetCount;
};

