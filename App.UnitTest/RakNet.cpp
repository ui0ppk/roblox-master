#include <boost/test/unit_test.hpp>

#include "GetTime.h"
#include "Kbhit.h"
#include "MessageIdentifiers.h"
#include "PacketLogger.h"
#include "PluginInterface2.h"
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "Rand.h"
#include "rbx/CEvent.h"
#include "ReliabilityLayer.h"

#include "Network/RakNetFast.h"
#include "util/G3DCore.h"

#include <stdio.h> // Printf

BOOST_AUTO_TEST_SUITE(RakNet)

class Peer : RakNet::PluginInterface2
{
public:
	bool success;
	RakNet::RakPeerInterface* const rakPeer;

	RakNet::RakPeerInterface* operator->() { return rakPeer; }
	Peer()
		:success(false)
		,rakPeer(RakNet::RakPeerInterface::GetInstance())
	{
		rakPeer->AttachPlugin(this);
	}
	~Peer()
	{
		rakPeer->Shutdown(500);
		rakPeer->DetachPlugin(this);
		RakNet::RakPeerInterface::DestroyInstance(rakPeer);
	}

	virtual void OnNewConnection(const RakNet::SystemAddress &systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
	{
		success = true;
	}

	virtual void OnFailedConnectionAttempt(RakNet::Packet *packet, RakNet::PI2_FailedConnectionAttemptReason failedConnectionAttemptReason)
	{
		BOOST_FAIL("OnFailedConnectionAttempt");
	}

};


BOOST_AUTO_TEST_CASE(SimpleConnect)
{
	Peer server;
	server->SetMaximumIncomingConnections(10);
	SocketDescriptor socketDescriptor(1234,0);
	BOOST_REQUIRE(server->Startup(10, &socketDescriptor, 1) == RakNet::RAKNET_STARTED);

	Peer client;
	SocketDescriptor socketDescriptor2;
	client->Startup(10, &socketDescriptor2, 1);
	BOOST_REQUIRE(client->Connect("localhost", 1234, 0, 0) == RakNet::CONNECTION_ATTEMPT_STARTED);

	while (!client.success && !server.success)
	{
		Packet* packet = client.rakPeer->Receive();
		if (packet!=0)
			client.rakPeer->DeallocatePacket(packet);
		packet = server.rakPeer->Receive();
		if (packet!=0)
			server.rakPeer->DeallocatePacket(packet);

		boost::this_thread::sleep(boost::posix_time::milliseconds(20));
	}
}

namespace {

template<class T>
void runRakNetFastT( RakNet::BitStream &s, bool write, RakNet::RakNetRandom& r )
{
	T t = (T)r.RandomMT();
	if( write )
	{
		s.Write( t );
	}
	else
	{
		T t1 = (T)0;
		RBX::Network::readFastT( s, t1 );

		if( t != t1 )
		{
			BOOST_REQUIRE(!"runRakNetFastT");
		}
	}
}

void runRakNetFastTBool( RakNet::BitStream &s, bool write, RakNet::RakNetRandom& r )
{
	bool t = (r.RandomMT() & 1) == 0;
	if( write )
	{
		s.Write( t );
	}
	else
	{
		bool t1 = false;
		RBX::Network::readFastT( s, t1 );
		if( t != t1 )
		{
			BOOST_REQUIRE(!"runRakNetFastTBool");
		}
	}
}

void runRakNetFastVector3( RakNet::BitStream &s, bool write, RakNet::RakNetRandom& r )
{
	RBX::Vector3 vec;
	do {
		// Placing very large and very small values together breaks the tolerance checks.
		// Overall, this is a crap form of compression reducing to only 83.33%.
		
		vec.x = (r.FrandomMT() - 0.5f) * (float)(r.RandomMT() & 0xffff);
		vec.y = (r.FrandomMT() - 0.5f) * (float)(r.RandomMT() & 0xffff);
		vec.z = (r.FrandomMT() - 0.5f) * (float)(r.RandomMT() & 0xffff);
	} while( fabs( vec.x ) < 0.001f || fabs( vec.y ) < 0.001f || fabs( vec.z ) < 0.001f )
		/**/;

	if( write )
	{
		s.WriteVector(vec.x, vec.y, vec.z);
	}
	else
	{
		RBX::Vector3 vec1;
		vec1 = RBX::Vector3::zero();
		RBX::Network::readVectorFast( s, vec1.x, vec1.y, vec1.z );

		bool x = ( fabs(1.0f - vec.x/vec1.x) < 0.001f );
		bool y = ( fabs(1.0f - vec.y/vec1.y) < 0.001f );
		bool z = ( fabs(1.0f - vec.z/vec1.z) < 0.001f );
		if( !x || !y || !z )
		{
			BOOST_REQUIRE(!"runRakNetFastVector3");
		}
	}
}

template<class T>
void runRakNetFastN( RakNet::BitStream &s, bool write, RakNet::RakNetRandom& r )
{
	static unsigned int nBits = sizeof(T) * 8;
	static unsigned int nBitsUsed;
	do
	{
		nBitsUsed = r.RandomMT() % nBits;
	} while( nBitsUsed == 0 )
		/**/;

	T t = (T)r.RandomMT();
	t &= (T)( ((uint64_t)1 << nBitsUsed) - (uint64_t)1 );

	if( write )
	{
		s.WriteBits((const unsigned char*)&t, nBitsUsed);
	}
	else
	{
		T t1 = (T)0;
		RBX::Network::readFastN( s, t1, nBitsUsed );
		if( t != t1 )
		{
			BOOST_REQUIRE(!"runRakNetFastN");
		}
	}
}


void runRakNetFast( RakNet::BitStream &s, bool write )
{
	RakNet::RakNetRandom r; r.SeedMT( 0x00c0ffee );

	for( int i=0; i < 17; ++i )
	{
		// big endian series of bytes that fill the variable.
		runRakNetFastT<float>(s, write, r);
		runRakNetFastT<double>(s, write, r);
		runRakNetFastT<int8_t>(s, write, r);
		runRakNetFastT<uint8_t>(s, write, r);
		runRakNetFastT<char>(s, write, r);
		runRakNetFastT<int16_t>(s, write, r);
		runRakNetFastT<uint16_t>(s, write, r);
		runRakNetFastT<int32_t>(s, write, r);
		runRakNetFastT<long>(s, write, r);
		runRakNetFastT<unsigned long>(s, write, r);
		runRakNetFastT<uint32_t>(s, write, r);
		runRakNetFastT<int64_t>(s, write, r);
		runRakNetFastT<uint64_t>(s, write, r);

		runRakNetFastTBool(s, write, r);
		runRakNetFastVector3( s, write, r);

		// Little endian range of bits that fit within the size of the variable.
		runRakNetFastN<int8_t>(s, write, r);
		runRakNetFastN<uint8_t>(s, write, r);
		runRakNetFastN<char>(s, write, r);
		runRakNetFastN<int16_t>(s, write, r);
		runRakNetFastN<uint16_t>(s, write, r);
		runRakNetFastN<int32_t>(s, write, r);
		runRakNetFastN<long>(s, write, r);
		runRakNetFastN<unsigned long>(s, write, r);
		runRakNetFastN<uint32_t>(s, write, r);
		runRakNetFastN<int64_t>(s, write, r);
		runRakNetFastN<uint64_t>(s, write, r);
	}
}

}  // namespace {


BOOST_AUTO_TEST_CASE(RakNetFast)
{
	 RakNet::BitStream s;

	 runRakNetFast( s, true );
	 runRakNetFast( s, false );

	 BOOST_REQUIRE( s.GetReadOffset() == s.GetWriteOffset() );
}


BOOST_AUTO_TEST_SUITE_END()

