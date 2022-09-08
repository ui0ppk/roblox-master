#pragma once

#include "SQLite3Plugin/SQLite3ClientPlugin.h"
#include "SQLite3Plugin/SQLiteClientLoggerPlugin.h"
#include "PacketizedTCP.h"
#include "NetworkSettings.h"
#include "Util.h"

#ifdef NETWORK_PROFILER
#define NETPROFILE_LOG(typeStr, packetPtr) RBX::Network::NetworkProfiler::singleton()->logPacket(typeStr, packetPtr)
#define NETPROFILE_START(dataBlobNameStr, bitStreamPtr) RBX::Network::NetworkProfiler::singleton()->startProfiling(dataBlobNameStr, bitStreamPtr)
#define NETPROFILE_END(dataBlobNameStr, bitStreamPtr) RBX::Network::NetworkProfiler::singleton()->endProfiling(dataBlobNameStr, bitStreamPtr)
#define CPUPROFILER_START(tag) RBX::Network::NetworkProfiler::singleton()->startCpuProfiling(tag);
#define CPUPROFILER_STEP(tag) RBX::Network::NetworkProfiler::singleton()->stepCpuProfiling(tag);
#define CPUPROFILER_OUTPUT() RBX::Network::NetworkProfiler::singleton()->outputCpuProfiling();
#else
#define NETPROFILE_LOG(typeStr, packetPtr)
#define NETPROFILE_START(dataBlobNameStr, bitStreamPtr)
#define NETPROFILE_END(dataBlobNameStr, bitStreamPtr)
#define CPUPROFILER_START(tag)
#define CPUPROFILER_STEP(tag)
#define CPUPROFILER_OUTPUT()
#endif

#ifdef NETWORK_PROFILER

namespace RBX
{
namespace Network
{

class NetworkProfiler
{
public:
	class DataBlobInfo
	{
	public:
		DataBlobInfo(const std::string& _name, RakNet::BitSize_t _bitStreamOffset)
		{
			name = _name;
			bitStreamOffset = _bitStreamOffset; 
		}
		std::string name;
		RakNet::BitSize_t bitStreamOffset;
	};

    enum ProfilerTags
    {
        PROFILER_streamOutPart,
        PROFILER_jointRemoval,
        PROFILER_gcStep,
        PROFILER_TAG_3,
        PROFILER_TAG_4,
        PROFILER_TAG_5,
        PROFILER_TAG_6,
        PROFILER_TAG_7,
        PROFILER_TAG_8,
        PROFILER_TAG_9,
        PROFILER_TAG_COUNT
    };

    class CpuProfilingStat
    {
        int currentStep;
        int numSample;
        RBX::Timer<RBX::Time::Precise> timer;

    public:
        double stepDelta[256]; // maximum 256 steps
        CpuProfilingStat()
        {
            reset();
        }
        inline void newSample() { timer.reset(); numSample++; currentStep = 0; }
        inline void step()
        {
            RBXASSERT(numSample>0);
            RBXASSERT(currentStep < 256);
            stepDelta[currentStep] = (stepDelta[currentStep] * (numSample-1) + timer.delta().seconds())/numSample;
            currentStep++;
        }
        int getNumSample() {return numSample;}
        void reset()
        {
            for (int i=0; i<numSample; i++)
            {
                stepDelta[i] = 0.0f;
            }
            currentStep = 0;
            numSample = 0;
        }
    };

    CpuProfilingStat cpuProfilingStats[PROFILER_TAG_COUNT]; 

	static NetworkProfiler* singleton();
	void logPacket(const std::string& type, const RakNet::Packet* packet);

	void startProfiling(const std::string& dataBlobName, const RakNet::BitStream* bitStream);
	void endProfiling(const std::string& dataBlobName, const RakNet::BitStream* bitStream);

    void startCpuProfiling(int);
    void stepCpuProfiling(int);
    void outputCpuProfiling();

	virtual ~NetworkProfiler(void);
private:
	// members
	RakNet::PacketizedTCP packetizedTCP;
	RakNet::SQLiteClientLoggerPlugin* loggerPlugin;
	const RakNet::BitStream* currentBitStream;
	std::vector<DataBlobInfo> dataBlobStack; // use vector to make use of its iterator
	std::size_t deepestLayer;
	bool connected;
	RBX::Timer<RBX::Time::Fast> profilerTimer;
	NetworkSettings* networkSettings;

	// functions
	NetworkProfiler(void);
	void Connect();
	void Disconnect();
	bool CanProfile();
};

}
}

#endif
