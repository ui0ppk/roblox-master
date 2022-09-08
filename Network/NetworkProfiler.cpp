#include "NetworkProfiler.h"

#ifdef NETWORK_PROFILER

#include "SQLite3Plugin/sqlite3.h"
#include "SQLite3Plugin/SQLiteClientLoggerPlugin.h"
#include "rbx/Debug.h"
#include "RakNetTypes.h"

namespace RBX {
namespace Network {

const char* const columnNames = "tag, offset, bitSize, layer1, layer2, layer3, layer4, layer5, layer6, layer7, layer8, layer9";

NetworkProfiler::NetworkProfiler(void)
: deepestLayer(0)
, networkSettings(&NetworkSettings::singleton())
, loggerPlugin(NULL)
{
}

NetworkProfiler::~NetworkProfiler(void)
{
	Disconnect();
}

bool NetworkProfiler::CanProfile()
{
	if (networkSettings->profiling)
	{
		Connect();
		if (networkSettings->profilerTimedSeconds > 0.f)
		{
			if (profilerTimer.delta().seconds() > networkSettings->profilerTimedSeconds )
			{
				// timeout, stop profiling
				networkSettings->profiling = false;
				Disconnect();
				return false; // note, early return
			}
		}
		return true;
	}
	else
	{
		Disconnect();
		return false;
	}
}

void NetworkProfiler::Connect()
{
	if (!connected)
	{
		loggerPlugin = new RakNet::SQLiteClientLoggerPlugin();
		packetizedTCP.AttachPlugin(loggerPlugin);
		packetizedTCP.Start(0,0);
		RakNet::SystemAddress serverAddress = packetizedTCP.Connect(networkSettings->profilerServerIp.c_str(), networkSettings->profilerServerPort, true);
		if (serverAddress == RakNet::UNASSIGNED_SYSTEM_ADDRESS)
		{
			StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Failed to connect to profiler log server.");
			packetizedTCP.Stop();
			packetizedTCP.DetachPlugin(loggerPlugin);
			delete loggerPlugin;
			networkSettings->profiling = false;
		}
		else
		{
			loggerPlugin->SetServerParameters(serverAddress, "roblox.db3");
			connected = true;
			if (networkSettings->profilerTimedSeconds > 0.f)
			{
				profilerTimer.reset();
			}
		}
	}
}

void NetworkProfiler::Disconnect()
{
	if (connected)
	{
		loggerPlugin->ClearResultHandlers();
		packetizedTCP.Stop();
		packetizedTCP.DetachPlugin(loggerPlugin);
		delete loggerPlugin;
		dataBlobStack.clear();
		connected = false;
	}
}

NetworkProfiler* NetworkProfiler::singleton()
{
	static NetworkProfiler networkProfiler;
	return &networkProfiler;
}

void NetworkProfiler::logPacket(const std::string& type, const RakNet::Packet* packet)
{
	if (CanProfile())
	{
		const char* profilerTag = networkSettings->profilerTag.c_str();
		rakSqlLog("general", "tag, packetType, packetBitSize", (profilerTag, type.c_str(), packet->bitSize));
	}
}

void NetworkProfiler::startCpuProfiling(int tag)
{
    if (networkSettings->profilecpu)
    {
        cpuProfilingStats[tag].newSample();
    }
}

void NetworkProfiler::stepCpuProfiling(int tag)
{
    if (networkSettings->profilecpu)
    {
        cpuProfilingStats[tag].step();
    }
}

void NetworkProfiler::outputCpuProfiling()
{
    for (int tag=0; tag<PROFILER_TAG_COUNT; tag++)
    {
        if (cpuProfilingStats[tag].getNumSample() > 0)
        {
            // output the stats
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "[%d] profiling results (%d samples):", tag, cpuProfilingStats[tag].getNumSample());
            float lastStepDelta = 0.0f;
            for (int i=0; cpuProfilingStats[tag].stepDelta[i]>0.f; i++)
            {
                float deltaBetweenSteps;
                float stepDelta = cpuProfilingStats[tag].stepDelta[i];
                deltaBetweenSteps = stepDelta - lastStepDelta;
                lastStepDelta = stepDelta;
                StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Step %d: %f (delta %f, total %f)", i+1, stepDelta, deltaBetweenSteps, deltaBetweenSteps*cpuProfilingStats[tag].getNumSample());
            }
        }
    }
}

void NetworkProfiler::startProfiling(const std::string& dataBlobName, const RakNet::BitStream* bitStream)
{
	if (CanProfile())
	{
		dataBlobStack.push_back(DataBlobInfo(dataBlobName, bitStream->GetReadOffset()));
		if (dataBlobStack.size() > deepestLayer)
		{
			deepestLayer = dataBlobStack.size();
		}
	}
}

void NetworkProfiler::endProfiling(const std::string& dataBlobName, const RakNet::BitStream* bitStream)
{
	if (CanProfile())
	{
		DataBlobInfo dataBlobInfo = dataBlobStack.back();
		RBXASSERT(dataBlobName == dataBlobInfo.name); // Make sure startProfiling and endProfiling are always in pairs
		RBXASSERT(dataBlobStack.size() > 0);
		const char* profilerTag = networkSettings->profilerTag.c_str();
		if (dataBlobStack.size() == deepestLayer) // only log the leaf
		{
			if (!NetworkSettings::singleton().profilerOneColumnPerStack)
			{
				std::string layers = "";
				for (std::vector<DataBlobInfo>::iterator iter = dataBlobStack.begin(); iter != dataBlobStack.end(); iter++)
				{
					layers+=((*iter).name + ".");
				}
				rakSqlLog("report", "tag, offset, bitsize, layers", (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, layers.c_str()));
			}
			else
			{
				switch (dataBlobStack.size())
				{
				case 1:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), "", "", "", "", "", "", "", ""));
					break;
				case 2:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), "", "", "", "", "", "", ""));
					break;
				case 3:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), "", "", "", "", "", ""));
					break;
				case 4:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), dataBlobStack.at(3).name.c_str(), "", "", "", "", ""));
					break;
				case 5:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), dataBlobStack.at(3).name.c_str(), dataBlobStack.at(4).name.c_str(), "", "", "", ""));
					break;
				case 6:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), dataBlobStack.at(3).name.c_str(), dataBlobStack.at(4).name.c_str(), dataBlobStack.at(5).name.c_str(), "", "", ""));
					break;
				case 7:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), dataBlobStack.at(3).name.c_str(), dataBlobStack.at(4).name.c_str(), dataBlobStack.at(5).name.c_str(), dataBlobStack.at(6).name.c_str(), "", ""));
					break;
				case 8:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), dataBlobStack.at(3).name.c_str(), dataBlobStack.at(4).name.c_str(), dataBlobStack.at(5).name.c_str(), dataBlobStack.at(6).name.c_str(), dataBlobStack.at(7).name.c_str(), ""));
					break;
				case 9:
					rakSqlLog("details", columnNames, (profilerTag, dataBlobInfo.bitStreamOffset, bitStream->GetReadOffset() - dataBlobInfo.bitStreamOffset, dataBlobStack.at(0).name.c_str(), dataBlobStack.at(1).name.c_str(), dataBlobStack.at(2).name.c_str(), dataBlobStack.at(3).name.c_str(), dataBlobStack.at(4).name.c_str(), dataBlobStack.at(5).name.c_str(), dataBlobStack.at(6).name.c_str(), dataBlobStack.at(7).name.c_str(), dataBlobStack.at(8).name.c_str()));
					break;
				default:
					RBXASSERT(false && "Overflowing the network profiler data layer stack (9). Maybe add more?");
				}
			}
		}
		deepestLayer = dataBlobStack.size(); // this will prevent the non-leaf node to be printed
		dataBlobStack.pop_back();
	}
}
}
}

#endif
