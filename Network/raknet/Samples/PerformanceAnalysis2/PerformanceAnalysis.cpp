
#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include "PacketLogger.h"
#include "GetTime.h"
#include "RakNetStatistics.h"
#include "MessageIdentifiers.h"
#include "Rand.h"
#include "Kbhit.h"
#include "ReliabilityLayer.h"
#include <stdio.h> // Printf
#include <windows.h> // Sleep

void DoSpeedTest(PacketReliability packetReliability);

RakPeerInterface *peer1;

void Server()
{
	peer1->SetMaximumIncomingConnections(10);
	SocketDescriptor socketDescriptor(1234,0);
	peer1->Startup(10, 0, &socketDescriptor, 1);
	
	printf("Testing RELIABLE_ORDERED\n");
	DoSpeedTest(RELIABLE_ORDERED);
}

void Client(const char* host)
{
	SocketDescriptor socketDescriptor;
	peer1->Startup(10, 0, &socketDescriptor, 1);
	peer1->Connect(host, 1234, 0, 0);
	printf("Connecting to server...\n");

	DWORD lastTime = ::GetTickCount();
	int packetsReceived = 0;
	int bitsReceived = 0;

	while (true)
	{
		while (Packet* p = peer1->Receive())
		{
			unsigned char packetIdentifier = ( unsigned char ) p->data[ 0 ];
			const char* name = PacketLogger::BaseIDTOString(packetIdentifier);
			if (name)
				printf("%s\n", name);
			peer1->DeallocatePacket(p);
		}

		Sleep(10);

		DWORD tickCount = ::GetTickCount();
		int deltaTicks = tickCount - lastTime;
		if (deltaTicks > 2000)
		{
			RakNetStatistics* rss1 = peer1->GetStatistics(peer1->GetSystemAddressFromIndex(0));
			if (rss1)
			{
				int deltaP = rss1->packetsReceived - packetsReceived;
				long deltaBits = rss1->bitsReceived - bitsReceived;
				printf("%s, %d/s, %dkbps\n", peer1->GetSystemAddressFromIndex(0).ToString(), deltaP * 1000 / deltaTicks, deltaBits / deltaTicks );
				packetsReceived = rss1->packetsReceived;
				bitsReceived = rss1->bitsReceived;

				lastTime = tickCount;
			}
		}
	}
}

void main(int argc, char *argv[])
{
	printf("Rev 21 noflow\n");
	scanf("%d", &ReliabilityLayer::ACK_PING_SAMPLES_BITS);
	printf("\nACK_PING_SAMPLES_BITS %d %d\n", ReliabilityLayer::ACK_PING_SAMPLES_BITS, 1 << ReliabilityLayer::ACK_PING_SAMPLES_BITS);

	peer1=RakNetworkFactory::GetRakPeerInterface();

	if (argc==1)
		Server();
	else
		Client(argv[1]);

	
	peer1->Shutdown(500);

	//	printf("Press any key to quit\n");
//	getch();
	RakNetworkFactory::DestroyRakPeerInterface(peer1);
}

class Sender
{
	SystemAddress address;
	PacketReliability packetReliability;
	DWORD lastPrintTime;
	DWORD lastSendTime;
	int packetsSent;
	int totalBitsSent;
	static const int dataSize = 1024;
	char dummyData[dataSize];
public:
	Sender(SystemAddress address, PacketReliability packetReliability)
		:address(address)
		,packetReliability(packetReliability)
	{
		lastPrintTime = ::GetTickCount();
		lastSendTime = ::GetTickCount();
		packetsSent = 0;
		totalBitsSent = 0;
		memset(dummyData,0,dataSize);
	}

	void step()
	{
		RakNetStatistics* rss1 = peer1->GetStatistics(address);
		if (rss1==0)
		{
			printf("\nPeer can't get statistics!");
			return;
		}

		DWORD tickCount = ::GetTickCount();
		int deltaTicks = tickCount - lastPrintTime;
		if (deltaTicks > 2000)
		{
			int deltaP = rss1->packetsSent - packetsSent;
			long deltaBits = rss1->totalBitsSent - totalBitsSent;
			printf("%s, %d/s, %dkbps\n", address.ToString(), deltaP * 1000 / deltaTicks, deltaBits / deltaTicks );
			packetsSent = rss1->packetsSent;
			totalBitsSent = rss1->totalBitsSent;

			lastPrintTime = tickCount;
		}

		if (rss1->messageSendBuffer[HIGH_PRIORITY]<10)
			// We want 20 * 1000 bytes = 20KBPS = 160kbps
			if (tickCount > lastSendTime + 1000 / 20)
			{
				peer1->Send(dummyData, dataSize, HIGH_PRIORITY, packetReliability, 0, address, false);
				lastSendTime = tickCount;
			}

	}
};

#include <vector>

typedef std::vector<Sender> Senders;

void DoSpeedTest(PacketReliability packetReliability)
{
	Senders senders;

	while (true)
	{
		::Sleep(0);

		for (int i = 0; i < senders.size(); ++i)
		{
			senders[i].step();
		}


		while (Packet* p = peer1->Receive())
		{
			unsigned char packetIdentifier = ( unsigned char ) p->data[ 0 ];
			const char* name = PacketLogger::BaseIDTOString(packetIdentifier);
			if (name)
				printf("%s %s\n", name, p->systemAddress.ToString());
			if (packetIdentifier == ID_NEW_INCOMING_CONNECTION)
			{
				printf("Adding client %s\n", p->systemAddress.ToString());
				senders.push_back(Sender(p->systemAddress, packetReliability));
			}
			peer1->DeallocatePacket(p);
		}
	}
}