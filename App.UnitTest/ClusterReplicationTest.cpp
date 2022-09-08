#include <boost/test/unit_test.hpp>

#include "Client.h"
#include "ClientReplicator.h"
#include "ClusterUpdateBuffer.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "Replicator.h"
#include "Security/SecurityContext.h"
#include "Server.h"
#include "ServerReplicator.h"
#include "Util/stringbuffer.h"
#include "Util/VarInt.h"
#include "V8DataModel/MegaCluster.h"

using namespace RBX;
using namespace Network;
using namespace RBX::Voxel;

//////////////////////////////////////////////////
// Utilities

struct ReadWriteBuffer {
	shared_ptr<RakNet::BitStream> writeStream;
	shared_ptr<RakNet::BitStream> readStream;

	ReadWriteBuffer() : writeStream(new RakNet::BitStream) {
		updateRead();
	}

	void updateRead() {
		readStream.reset(new RakNet::BitStream(
			writeStream->GetData(), writeStream->GetNumberOfBytesUsed(), true));
	}
};

struct ReplicatorTestWrapper {
	shared_ptr<Instance> storage;
	shared_ptr<MegaClusterInstance> cluster;
	shared_ptr<Replicator> replicator;
	unsigned int packetSize;

private:
	ReplicatorTestWrapper(shared_ptr<Instance> storage,
		shared_ptr<MegaClusterInstance> cluster, shared_ptr<Replicator> replicator, bool listenToChanges = false) :
			storage(storage), cluster(cluster), replicator(replicator), packetSize(1492)
	{
		cluster->initialize();
		replicator->addReplicationData(cluster, true, false);
	}
public:

	static shared_ptr<ReplicatorTestWrapper> server(shared_ptr<MegaClusterInstance> cluster, bool listenToChanges = false) {
		Security::Impersonator impersonate(Security::CmdLine_);

		shared_ptr<Server> server(Creatable<Instance>::create<Server>());
		shared_ptr<ServerReplicator> replicator(Creatable<Instance>::create<ServerReplicator>(
			RakNet::SystemAddress(), server.get(), &NetworkSettings::singleton()));
		return shared_ptr<ReplicatorTestWrapper>(
			new ReplicatorTestWrapper(server, cluster, replicator, listenToChanges));
	}

	static shared_ptr<ReplicatorTestWrapper> client(shared_ptr<MegaClusterInstance> cluster) {
		Security::Impersonator impersonate(Security::CmdLine_);

		shared_ptr<Client> client(Creatable<Instance>::create<Client>());
		shared_ptr<ClientReplicator> replicator(Creatable<Instance>::create<ClientReplicator>(
			RakNet::SystemAddress(), client.get(), RakNet::SystemAddress(), &NetworkSettings::singleton()));
		return shared_ptr<ReplicatorTestWrapper>(new ReplicatorTestWrapper(client, cluster, replicator, false));
	}

    ~ReplicatorTestWrapper()
    {
        if (cluster)
        {
            bool res = replicator->disconnectReplicationData(cluster);
            RBXASSERT(res);
        }
    }
	// use this if you want to disable a replicator before it would
	// be destroyed
	void testClose()
    {
        RBXASSERT(cluster);

		replicator->onServiceProvider(NULL, NULL);
        cluster.reset();
	}

	Replicator::ClusterReplicationData& getReplicationData() {
		BOOST_REQUIRE(replicator->megaClusterInstance);
		return replicator->clusterReplicationData;
	}
	void sendClusterDeltasHelper(ReadWriteBuffer& readWriteBuffer) {
		replicator->sendClusterContent(
			readWriteBuffer.writeStream, getReplicationData().updateBuffer, packetSize);
		// add end-of-data int
		*(readWriteBuffer.writeStream) << (int)(0xffff);
		readWriteBuffer.updateRead();
	}
	void receiveClusterHelper(ReadWriteBuffer& readWriteBuffer) {
		replicator->receiveCluster(*(readWriteBuffer.readStream), cluster.get(), false);
	}
	void sendClusterChunksHelper(ReadWriteBuffer& readWriteBuffer) {
        replicator->sendClusterContent(readWriteBuffer.writeStream, getReplicationData().initialSendIterator, packetSize);
		*(readWriteBuffer.writeStream) << (int)(0xffff);
		readWriteBuffer.updateRead();
	}
};

struct PacketCountTestParameters {
	unsigned int PlayerCount;
	unsigned int BigUpdatePlayerCount;
	unsigned int BigUpdateDimX;
	unsigned int BigUpdateDimY;
	unsigned int BigUpdateDimZ;
	unsigned int DesiredSendHertz;
	float MaxSecondsToResolveNetworkUpdates;
};

static inline Cell createCell(CellBlock cellBlock, CellOrientation cellOrientation) {
	Cell result;
	result.solid.setBlock(cellBlock);
	result.solid.setOrientation(cellOrientation);
	return result;
}

static unsigned int countPackets(shared_ptr<ReplicatorTestWrapper>& sender, shared_ptr<ReplicatorTestWrapper>& receiver) {
	unsigned int packets = 0;
	unsigned int previousSize = sender->getReplicationData().updateBuffer.size();
	BOOST_CHECK(previousSize > 0);
	while (sender->getReplicationData().updateBuffer.size() > 0) {
		packets++;
		ReadWriteBuffer buffer;
		sender->sendClusterDeltasHelper(buffer);
		BOOST_REQUIRE(sender->getReplicationData().updateBuffer.size() < previousSize);
		BOOST_REQUIRE_LT(buffer.writeStream->GetNumberOfBytesUsed(), 1.05 * sender->packetSize);
		previousSize = sender->getReplicationData().updateBuffer.size();
		receiver->receiveClusterHelper(buffer);
	}
	return packets;
}

template <typename Iterator>
static unsigned int countChunkPackets(Iterator& iterator, shared_ptr<ReplicatorTestWrapper>& sender, shared_ptr<ReplicatorTestWrapper>& receiver) {
	unsigned int previousCount = iterator.size();
	unsigned int bytesUsed = 0;
	while(iterator.size() > 0) {
		ReadWriteBuffer buffer;
		sender->sendClusterChunksHelper(buffer);
		BOOST_CHECK(iterator.size() < previousCount);
		BOOST_REQUIRE_LT(buffer.writeStream->GetNumberOfBytesUsed(), 1.05 * sender->packetSize);
		previousCount = iterator.size();
		bytesUsed += buffer.writeStream->GetNumberOfBytesUsed();
		receiver->receiveClusterHelper(buffer);
	}

	return (bytesUsed + sender->packetSize - 1) / sender->packetSize;
}

static unsigned int countChunkPackets(shared_ptr<ReplicatorTestWrapper>& sender, shared_ptr<ReplicatorTestWrapper>& receiver) {
    sender->getReplicationData().initialSendIterator = RBX::ClusterChunksIterator(sender->cluster->getVoxelGrid()->getNonEmptyChunks());

    return countChunkPackets(sender->getReplicationData().initialSendIterator, sender, receiver);
}

static void runPacketCountTest(const PacketCountTestParameters& params) {
	// IMPORTANT NOTE:
	// All clients share one cluster! This works for this test
	// because:
	// 1) The test does not check consistency of clusters between clients
	// 2) Each simulated client is working in a different cluster chunk
	// 3) We network out each client's updates as their added
	// 4) Multiple replicators can be connected to the same cluster
	// 5) Temporary clients are used when sending data (they are detached
	//    from the cluster before more data is sent)

	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	std::vector<shared_ptr<ReplicatorTestWrapper> > serverWrappers(params.PlayerCount);

	for (unsigned int i = 0; i < params.PlayerCount; ++i) {
		serverWrappers[i] = ReplicatorTestWrapper::server(serverCluster);
	}

	ReadWriteBuffer buffer;

	Vector3int16 cell1Loc(1, 1, 1);
	Cell newCell1 = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_NegX);
	CellMaterial cell1Material = CELL_MATERIAL_Sand;

	Vector3int16 cell2Loc(2, 2, 2);
	Cell newCell2 = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_X);
	CellMaterial cell2Material = CELL_MATERIAL_Granite;

	for (unsigned int i = 0; i < params.PlayerCount; ++i) {
		shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

		Vector3int16 chunkOffset( (i % 16) * 32, 17, (i / 16) * 32 );
		if (i < params.BigUpdatePlayerCount) {
			Vector3int16 offset(0, 0, 0);
			for (offset.y = 0; offset.y < (int)params.BigUpdateDimY; ++offset.y) {
				for (offset.z = 0; offset.z < (int)params.BigUpdateDimZ; ++offset.z) {
					for (offset.x = 0; offset.x < (int)params.BigUpdateDimX; ++offset.x) {
						clientCluster->getVoxelGrid()->setCell(chunkOffset + offset,
							newCell1, cell1Material);
					}
				}
			}
		} else {
			clientCluster->getVoxelGrid()->setCell(chunkOffset + cell1Loc,
				newCell1, cell1Material);
			clientCluster->getVoxelGrid()->setCell(chunkOffset + cell2Loc,
				newCell2, cell2Material);
		}

		unsigned int packetCount = countPackets(clientWrapper, serverWrappers[i]);
		
		BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
		BOOST_CHECK(packetCount < params.DesiredSendHertz);

		clientWrapper->testClose();
	}

	unsigned int previousSize = serverWrappers[0]->getReplicationData().updateBuffer.size();	
	// Check that we have at least some data
	BOOST_REQUIRE_GE((int)previousSize, 50);

	shared_ptr<ReplicatorTestWrapper> clientWrapper1(ReplicatorTestWrapper::client(clientCluster));
	unsigned int packetCount1 = countPackets(serverWrappers[0], clientWrapper1);
	// assert that we can send out the required information in time
	BOOST_CHECK(packetCount1 < params.MaxSecondsToResolveNetworkUpdates * params.DesiredSendHertz);
	// use a warn to log the number of packets used
	// BOOST_WARN_EQUAL(0, packetCount1);

	shared_ptr<ReplicatorTestWrapper> clientWrapper2(ReplicatorTestWrapper::client(clientCluster));
	unsigned int packetCount2 = countPackets(serverWrappers[params.PlayerCount - 1], clientWrapper2);
	// assert that we can send out the required information in time
	BOOST_CHECK(packetCount2 < params.MaxSecondsToResolveNetworkUpdates * params.DesiredSendHertz);
	// use a warn to log the number of packets used
	// BOOST_WARN_EQUAL(0, packetCount2);
}

////////////////////////////////////////////
// End of utilities, start of tests

BOOST_AUTO_TEST_SUITE( ClusterDeltaBuffer )

BOOST_AUTO_TEST_CASE( BitSetConstantsMakeSense ) {
	unsigned int bits = 8 * sizeof(UintSet::BitGroup);
	BOOST_CHECK(bits > 0);
	BOOST_CHECK_EQUAL(UintSet::kBitInGroupMask, bits - 1);
	int shift = -1;
	while (bits) {
		shift++;
		bits>>=1;
	}
	BOOST_CHECK_EQUAL(UintSet::kShiftForGroup, shift);

	BOOST_CHECK_EQUAL(1 << kXZ_CHUNK_SIZE_AS_BITSHIFT, kXZ_CHUNK_SIZE);
	BOOST_CHECK_EQUAL(kXZ_CHUNK_SIZE - 1, kXZ_CHUNK_SIZE_AS_BITMASK);
	BOOST_CHECK_EQUAL(1 << kY_CHUNK_SIZE_AS_BITSHIFT, kY_CHUNK_SIZE);
	BOOST_CHECK_EQUAL(kY_CHUNK_SIZE - 1, kY_CHUNK_SIZE_AS_BITMASK);
}

BOOST_AUTO_TEST_CASE( UintSetEncoding ) {
	const Vector3int16 baseCellOffset(0,16,0);
	for (int y = 0; y < 5; ++y) {
		for (int z = 0; z < 5; ++z) {
			for (int x = 0; x < 5; ++x) {
				const Vector3int16 expected(17 + x,17 + y, 17 + z);
				unsigned int intVal;
				ClusterUpdateBuffer::computeUintRepresentingLocationInChunk(
					expected, &intVal);

				Vector3int16 actual;
				ClusterUpdateBuffer::computeGlobalLocationFromUintRepresentation(
					intVal, baseCellOffset, &actual);
				BOOST_CHECK_EQUAL(expected.x, actual.x);
				BOOST_CHECK_EQUAL(expected.y, actual.y);
				BOOST_CHECK_EQUAL(expected.z, actual.z);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( LengthBitSetOrdering )
{
	ClusterUpdateBuffer updateBuffer;
	for (unsigned int i = 0; i < 32; ++i) {
		BOOST_CHECK_EQUAL(i, updateBuffer.size());
		updateBuffer.push(Vector3int16(i, 0, 0));
	}
	for (unsigned int i = 0; i < 32; ++i) {
		Vector3int16 out;
		updateBuffer.pop(&out);
		BOOST_CHECK_EQUAL(i, out.x);
	}

	for (unsigned int i = 0; i < 32; ++i) {
		updateBuffer.push(Vector3int16(31 - i, 0, 0));
	}
	for (unsigned int i = 0; i < 32; ++i) {
		Vector3int16 out;
		updateBuffer.pop(&out);
		BOOST_CHECK_EQUAL(i, out.x);
	}
}

BOOST_AUTO_TEST_CASE( BitSetDoesNotStoreDuplicates )
{
	UintSet s;
	
	const Vector3int16 testLocation(1,2,3);
	unsigned int info;
	ClusterUpdateBuffer::computeUintRepresentingLocationInChunk(
		testLocation, &info);
	BOOST_CHECK(!s.contains(info));
	BOOST_CHECK_EQUAL(0, s.size());
	s.insert(info);
	BOOST_CHECK(s.contains(info));
	BOOST_CHECK_EQUAL(1, s.size());
	s.insert(info);
	BOOST_CHECK(s.contains(info));
	BOOST_CHECK_EQUAL(1, s.size());

	ClusterUpdateBuffer updateBuffer;
	BOOST_CHECK(!updateBuffer.chk(testLocation));
	BOOST_CHECK_EQUAL(0, updateBuffer.size());
	updateBuffer.push(testLocation);
	BOOST_CHECK(updateBuffer.chk(testLocation));
	BOOST_CHECK_EQUAL(1, updateBuffer.size());
	updateBuffer.push(testLocation);
	BOOST_CHECK(updateBuffer.chk(testLocation));
	BOOST_CHECK_EQUAL(1, updateBuffer.size());
}

BOOST_AUTO_TEST_CASE( UpdatesGroupedByChunk )
{
	ClusterUpdateBuffer updateBuffer;
	for (unsigned int i = 0; i < 32; ++i) {
		updateBuffer.push(Vector3int16(i, 0, 0));
		updateBuffer.push(Vector3int16(i, 20, 0));
		BOOST_CHECK_EQUAL((i + 1) * 2, updateBuffer.size());
	}

    Vector3int16 first;
    updateBuffer.pop(&first);

    BOOST_REQUIRE(first == Vector3int16(0, 0, 0) || first == Vector3int16(0, 20, 0));

    int firstY = first.y;
    int secondY = first.y == 0 ? 20 : 0;

	for (unsigned int i = 1; i < 32; ++i) {
		Vector3int16 out;
		updateBuffer.pop(&out);
		BOOST_CHECK_EQUAL(firstY, out.y);
		BOOST_CHECK_EQUAL(i, out.x);
		BOOST_CHECK_EQUAL(64 - (i + 1), updateBuffer.size());
	}
	for (unsigned int i = 0; i < 32; ++i) {
		Vector3int16 out;
		updateBuffer.pop(&out);
		BOOST_CHECK_EQUAL(secondY, out.y);
		BOOST_CHECK_EQUAL(i, out.x);
		BOOST_CHECK_EQUAL(32 - (i + 1), updateBuffer.size());
	}
}

BOOST_AUTO_TEST_CASE( UpdatesContinueByChunk )
{
	ClusterUpdateBuffer updateBuffer;
	updateBuffer.push(Vector3int16(40,0,0));
	updateBuffer.push(Vector3int16(41,0,0));
	updateBuffer.push(Vector3int16(42,0,0));

	Vector3int16 read;
	updateBuffer.pop(&read);
	BOOST_CHECK_EQUAL(Vector3int16(40,0,0), read);

	updateBuffer.push(Vector3int16(0,0,0));

    Vector3int16 read1, read2, read3;
    updateBuffer.pop(&read1);
    updateBuffer.pop(&read2);
    updateBuffer.pop(&read3);

    if (read1 == Vector3int16(41,0,0))
    {
        BOOST_CHECK_EQUAL(Vector3int16(41,0,0), read1);
        BOOST_CHECK_EQUAL(Vector3int16(42,0,0), read2);
        BOOST_CHECK_EQUAL(Vector3int16(0,0,0), read3);
    }
    else
    {
        BOOST_CHECK_EQUAL(Vector3int16(0,0,0), read1);
        BOOST_CHECK_EQUAL(Vector3int16(41,0,0), read2);
        BOOST_CHECK_EQUAL(Vector3int16(42,0,0), read3);
    }
}

BOOST_AUTO_TEST_CASE ( GrowRetainsOrder )
{
	ClusterUpdateBuffer updateBuffer;
	for (unsigned int i = 0; i < 64; ++i) {
		updateBuffer.push(Vector3int16(0, i / 32, i % 32));
		BOOST_CHECK_EQUAL(i + 1, updateBuffer.size());
	}
	for (unsigned int i = 0; i < 32; ++i) {
		Vector3int16 out;
		updateBuffer.pop(&out);
		BOOST_CHECK_EQUAL(i / 32, out.y);
		BOOST_CHECK_EQUAL(i % 32, out.z);
	}
	// try to force a grow
	for (unsigned int i = 64; i < 256; ++i) {
		updateBuffer.push(Vector3int16(0, i / 32, i % 32));
		BOOST_CHECK_EQUAL(i + 1 - 32, updateBuffer.size());
	}

	// check prepend grow
	updateBuffer.push(Vector3int16(0, 0, 0));
	Vector3int16 out;
	updateBuffer.pop(&out);
	BOOST_CHECK_EQUAL(0, out.y);
	BOOST_CHECK_EQUAL(0, out.z);

	for (unsigned int i = 32; i < 256; ++i) {
		Vector3int16 out;
		updateBuffer.pop(&out);
		BOOST_CHECK_EQUAL(i / 32, out.y);
		BOOST_CHECK_EQUAL(i % 32, out.z);
	}
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( ClusterDeltaReplication )

// #ifndef _DEBUG // this test is too slow for debug mode
// BOOST_AUTO_TEST_CASE( VarIntEncodeDecode )
// {
// 	for (unsigned int i = 0; i < (1 << 21); ++i) {
// 		ReadWriteBuffer buffer;
// 		RBX::VarInt<>::encode(buffer.writeStream.get(), i << 3);
// 		buffer.updateRead();
// 		unsigned int result;
// 		RBX::VarInt<>::decode(*(buffer.readStream), &result);
// 		BOOST_CHECK_EQUAL(i << 3, result);
// 	}
// }
// #endif

BOOST_AUTO_TEST_CASE( CreatingStateSanityCheck )
{
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));
	
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	Vector3int16 testCellLocation(12, 12, 12);
	Cell newCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);

	clientCluster->getVoxelGrid()->setCell(testCellLocation,
		newCell, CELL_MATERIAL_Grass);

	BOOST_CHECK_EQUAL(1, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());

	ReadWriteBuffer buffer;
	clientWrapper->sendClusterDeltasHelper(buffer);

	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(32 + (1 + 1 + 48 + 14 + 2 + 16 + 2) + (1 + 1) + 32, buffer.writeStream->GetNumberOfBitsUsed());
	BOOST_CHECK_EQUAL(19, buffer.readStream->GetNumberOfBytesUsed());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(Constants::kUniqueEmptyCellRepresentation, serverCluster->getVoxelGrid()->getCell(testCellLocation));

	serverWrapper->receiveClusterHelper(buffer);

	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(1, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(newCell, serverCluster->getVoxelGrid()->getCell(testCellLocation));
}

BOOST_AUTO_TEST_CASE( DuplicateEncoding )
{
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));
	
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	Vector3int16 testCellLocations[4] =
	{
		Vector3int16(12, 12, 12),
		Vector3int16(13, 12, 12),
		Vector3int16(14, 12, 12),
		Vector3int16(15, 12, 12),
	};
	Cell newCell1 = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	Cell newCell2 = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_Z);

	clientCluster->getVoxelGrid()->setCell(testCellLocations[0],
		newCell1, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(testCellLocations[1],
		newCell1, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(testCellLocations[2],
		newCell2, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(testCellLocations[3],
		newCell2, CELL_MATERIAL_Grass);

	BOOST_CHECK_EQUAL(4, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());

	ReadWriteBuffer buffer;
	clientWrapper->sendClusterDeltasHelper(buffer);
	serverWrapper->receiveClusterHelper(buffer);

	BOOST_CHECK_EQUAL(newCell1, serverCluster->getVoxelGrid()->getCell(testCellLocations[0]));
	BOOST_CHECK_EQUAL(newCell1, serverCluster->getVoxelGrid()->getCell(testCellLocations[1]));
	BOOST_CHECK_EQUAL(newCell2, serverCluster->getVoxelGrid()->getCell(testCellLocations[2]));
	BOOST_CHECK_EQUAL(newCell2, serverCluster->getVoxelGrid()->getCell(testCellLocations[3]));
}

BOOST_AUTO_TEST_CASE( CanSetCellsInLargestChunk )
{
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));
	
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	Vector3int16 testCellLocation1(12, 12, 12);
	Vector3int16 testCellLocation2(490, 49, 490);
	Cell newCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);

	clientCluster->getVoxelGrid()->setCell(testCellLocation1,
		newCell, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(testCellLocation2,
		newCell, CELL_MATERIAL_Grass);

	BOOST_CHECK_EQUAL(2, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(Constants::kUniqueEmptyCellRepresentation, serverCluster->getVoxelGrid()->getCell(testCellLocation1));
	BOOST_CHECK_EQUAL(Constants::kUniqueEmptyCellRepresentation, serverCluster->getVoxelGrid()->getCell(testCellLocation2));

	ReadWriteBuffer buffer;
	clientWrapper->sendClusterDeltasHelper(buffer);
	serverWrapper->receiveClusterHelper(buffer);

	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(2, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(newCell, serverCluster->getVoxelGrid()->getCell(testCellLocation1));
	BOOST_CHECK_EQUAL(newCell, serverCluster->getVoxelGrid()->getCell(testCellLocation2));
}

BOOST_AUTO_TEST_CASE( ShipsInTheNight ) {
	// 1) server and client both queue output to eachother for the same cell
	//    - Client should _not_ have a material change
	//    - Server should have a material change
	// 2) client receives delta, and doesn't generate a new update
	// 3) server receives delta, and generates a new update
	// 4) client receives bounce from server, becomes consistent with server
	//    and does _not_ generate a bounce update

	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));
	
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	Vector3int16 testCellLocation(12, 12, 12);
	Cell baseCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	CellMaterial baseMaterial = CELL_MATERIAL_Grass;

	clientCluster->getVoxelGrid()->setCell(testCellLocation, baseCell, baseMaterial);

	countPackets(clientWrapper, serverWrapper);
	countPackets(serverWrapper, clientWrapper);

	// verify starting state
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(baseCell, serverCluster->getVoxelGrid()->getCell(testCellLocation));
	BOOST_CHECK_EQUAL(baseMaterial, serverCluster->getVoxelGrid()->getCellMaterial(testCellLocation));
	BOOST_CHECK_EQUAL(baseCell, clientCluster->getVoxelGrid()->getCell(testCellLocation));
	BOOST_CHECK_EQUAL(baseMaterial, clientCluster->getVoxelGrid()->getCellMaterial(testCellLocation));

	Cell newClientCell = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_NegZ);
	CellMaterial newClientMaterial = baseMaterial;
	clientCluster->getVoxelGrid()->setCell(testCellLocation, newClientCell, newClientMaterial);

	Cell newServerCell = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_X);
	CellMaterial newServerMaterial = CELL_MATERIAL_Sand;
	serverCluster->getVoxelGrid()->setCell(testCellLocation, newServerCell, newServerMaterial);

	ReadWriteBuffer serverToClient, clientToServer;

	clientWrapper->sendClusterDeltasHelper(clientToServer);
	serverWrapper->sendClusterDeltasHelper(serverToClient);

	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());

	clientWrapper->receiveClusterHelper(serverToClient);
	serverWrapper->receiveClusterHelper(clientToServer);

	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(testCellLocation));
	BOOST_CHECK_EQUAL(newClientMaterial, serverCluster->getVoxelGrid()->getCellMaterial(testCellLocation));
	BOOST_CHECK_EQUAL(newServerCell, clientCluster->getVoxelGrid()->getCell(testCellLocation));
	BOOST_CHECK_EQUAL(newServerMaterial, clientCluster->getVoxelGrid()->getCellMaterial(testCellLocation));

	ReadWriteBuffer lastBuffer;
	serverWrapper->sendClusterDeltasHelper(lastBuffer);
	clientWrapper->receiveClusterHelper(lastBuffer);

	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(testCellLocation));
	BOOST_CHECK_EQUAL(newClientMaterial, serverCluster->getVoxelGrid()->getCellMaterial(testCellLocation));
	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(testCellLocation));
	BOOST_CHECK_EQUAL(newClientMaterial, clientCluster->getVoxelGrid()->getCellMaterial(testCellLocation));
}

BOOST_AUTO_TEST_CASE( ServerReceivesMoreUpdatesBeforeFinishedSendingOlderUpdates )
{
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> clientCluster2(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper2(ReplicatorTestWrapper::client(clientCluster2));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));
	shared_ptr<ReplicatorTestWrapper> serverWrapper2(ReplicatorTestWrapper::server(serverCluster));

	Vector3int16 testLocation(0, 0, 0);
	const Vector3int16 dim(30,14,30);
	const Cell newCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	for (testLocation.y = 0; testLocation.y < dim.y; ++testLocation.y) {
		for (testLocation.z = 0; testLocation.z < dim.z; ++testLocation.z) {
			for (testLocation.x = 0; testLocation.x < dim.x; ++testLocation.x) {
				clientCluster->getVoxelGrid()->setCell(testLocation, newCell,
					(CellMaterial)(((testLocation.x + testLocation.y + testLocation.z) % 15) + 1));
			}
		}
	}
	
	// 2 packets client1 => server
	{
		ReadWriteBuffer buffer1;
		clientWrapper->sendClusterDeltasHelper(buffer1);
		serverWrapper->receiveClusterHelper(buffer1);
		ReadWriteBuffer buffer2;
		clientWrapper->sendClusterDeltasHelper(buffer2);
		serverWrapper->receiveClusterHelper(buffer2);
	}

	// to make interleaved situation there needs to be more data for the
	// server to receive
	BOOST_REQUIRE(clientWrapper->getReplicationData().updateBuffer.size() > 0);

	// 1 packet server => client2
	{
		ReadWriteBuffer buffer;
		serverWrapper2->sendClusterDeltasHelper(buffer);
		clientWrapper2->receiveClusterHelper(buffer);
	}

	// rest of packets from client 1 => server
	int remainPackets = countPackets(clientWrapper, serverWrapper);
	BOOST_REQUIRE_GT(remainPackets, 2); // make sure there was a lot more data
	
	// sync back to all clients
	countPackets(serverWrapper, clientWrapper);
	countPackets(serverWrapper2, clientWrapper2);

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, serverWrapper2->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper2->getReplicationData().updateBuffer.size());
	
	for (testLocation.y = 0; testLocation.y < dim.y; ++testLocation.y) {
		for (testLocation.z = 0; testLocation.z < dim.z; ++testLocation.z) {
			for (testLocation.x = 0; testLocation.x < dim.x; ++testLocation.x) {
				BOOST_CHECK_EQUAL(newCell, clientCluster->getVoxelGrid()->getCell(testLocation));
				BOOST_CHECK_EQUAL(newCell, clientCluster2->getVoxelGrid()->getCell(testLocation));
				BOOST_CHECK_EQUAL(newCell, serverCluster->getVoxelGrid()->getCell(testLocation));
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( ClientDoesNotAcceptNetworkUpdatesForCellsInQueue )
{
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	const Vector3int16 commonLocation(23, 34, 45);
	const Vector3int16 serverOnlyLocation = commonLocation + Vector3int16(1,0,0);
	const Vector3int16 clientOnlyLocation = commonLocation + Vector3int16(0,0,1);
	const Cell newServerCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	const Cell newClientCell = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_Z);

	serverCluster->getVoxelGrid()->setCell(commonLocation, newServerCell, CELL_MATERIAL_Grass);
	serverCluster->getVoxelGrid()->setCell(serverOnlyLocation, newServerCell, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(clientOnlyLocation, newClientCell, CELL_MATERIAL_Grass);

	BOOST_CHECK_EQUAL(2, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(2, clientWrapper->getReplicationData().updateBuffer.size());

	{
		ReadWriteBuffer buffer;
		serverWrapper->sendClusterDeltasHelper(buffer);
		clientWrapper->receiveClusterHelper(buffer);
	}

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(2, clientWrapper->getReplicationData().updateBuffer.size());
	
	BOOST_CHECK_EQUAL(newServerCell, serverCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, serverCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(Constants::kUniqueEmptyCellRepresentation, serverCluster->getVoxelGrid()->getCell(clientOnlyLocation));

	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, clientCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(clientOnlyLocation));

	{
		ReadWriteBuffer buffer;
		clientWrapper->sendClusterDeltasHelper(buffer);
		serverWrapper->receiveClusterHelper(buffer);
	}

	// The server wrapper update size assert may get stale.
	// It depends on the server debounce policy.
	BOOST_CHECK_EQUAL(2, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());

	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, serverCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(clientOnlyLocation));

	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, clientCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(clientOnlyLocation));
}

BOOST_AUTO_TEST_CASE( ServerDoesAcceptNetworkUpdatesForCellsInQueue )
{
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	const Vector3int16 commonLocation(23, 34, 45);
	const Vector3int16 serverOnlyLocation = commonLocation + Vector3int16(1,0,0);
	const Vector3int16 clientOnlyLocation = commonLocation + Vector3int16(0,0,1);
	const Cell newServerCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	const Cell newClientCell = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_Z);

	serverCluster->getVoxelGrid()->setCell(commonLocation, newServerCell, CELL_MATERIAL_Grass);
	serverCluster->getVoxelGrid()->setCell(serverOnlyLocation, newServerCell, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell, CELL_MATERIAL_Grass);
	clientCluster->getVoxelGrid()->setCell(clientOnlyLocation, newClientCell, CELL_MATERIAL_Grass);

	BOOST_CHECK_EQUAL(2, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(2, clientWrapper->getReplicationData().updateBuffer.size());

	{
		ReadWriteBuffer buffer;
		clientWrapper->sendClusterDeltasHelper(buffer);
		serverWrapper->receiveClusterHelper(buffer);
	}

	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(3, serverWrapper->getReplicationData().updateBuffer.size());
	
	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(clientOnlyLocation));
	BOOST_CHECK_EQUAL(Constants::kUniqueEmptyCellRepresentation, clientCluster->getVoxelGrid()->getCell(serverOnlyLocation));

	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, serverCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(clientOnlyLocation));

	{
		ReadWriteBuffer buffer;
		serverWrapper->sendClusterDeltasHelper(buffer);
		clientWrapper->receiveClusterHelper(buffer);
	}

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());

	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, serverCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(newClientCell, serverCluster->getVoxelGrid()->getCell(clientOnlyLocation));

	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newServerCell, clientCluster->getVoxelGrid()->getCell(serverOnlyLocation));
	BOOST_CHECK_EQUAL(newClientCell, clientCluster->getVoxelGrid()->getCell(clientOnlyLocation));
}

BOOST_AUTO_TEST_CASE( ClientIsNotClobberedByServerMergingUpdatesAndBouncing ) {
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	const Vector3int16 commonLocation(23, 34, 45);

	ReadWriteBuffer clientToServer1, clientToServer2;
	const Cell newClientCell1 = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_Z);
	const Cell newClientCell2 = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_X);

	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell1, CELL_MATERIAL_Grass);
	clientWrapper->sendClusterDeltasHelper(clientToServer1);
	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell2, CELL_MATERIAL_Grass);
	clientWrapper->sendClusterDeltasHelper(clientToServer2);

	serverWrapper->receiveClusterHelper(clientToServer1);
	serverWrapper->receiveClusterHelper(clientToServer2);

	{
		ReadWriteBuffer serverToClient;
		serverWrapper->sendClusterDeltasHelper(serverToClient);
		clientWrapper->receiveClusterHelper(serverToClient);
	}

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());

	BOOST_CHECK_EQUAL(newClientCell2, clientCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newClientCell2, serverCluster->getVoxelGrid()->getCell(commonLocation));
}

BOOST_AUTO_TEST_CASE( ClientIsNotClobberedByQuickServerBounces ) {
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	const Vector3int16 commonLocation(23, 34, 45);

	const Cell newClientCell1 = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_Z);
	const Cell newClientCell2 = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_X);

	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell1, CELL_MATERIAL_Grass);

	{
		ReadWriteBuffer buffer;
		clientWrapper->sendClusterDeltasHelper(buffer);
		serverWrapper->receiveClusterHelper(buffer);
	}

	ReadWriteBuffer serverBounce;
	BOOST_CHECK_EQUAL(1, serverWrapper->getReplicationData().updateBuffer.size());
	serverWrapper->sendClusterDeltasHelper(serverBounce);

	// receive update right after making a local revision to the cell
	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell2, CELL_MATERIAL_Grass);
	clientWrapper->receiveClusterHelper(serverBounce);

	{
		ReadWriteBuffer buffer;
		clientWrapper->sendClusterDeltasHelper(buffer);
		serverWrapper->receiveClusterHelper(buffer);
	}

	{
		ReadWriteBuffer buffer;
		serverWrapper->sendClusterDeltasHelper(buffer);
		clientWrapper->receiveClusterHelper(buffer);
	}

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());

	BOOST_CHECK_EQUAL(newClientCell2, clientCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(newClientCell2, serverCluster->getVoxelGrid()->getCell(commonLocation));
}

BOOST_AUTO_TEST_CASE( EmptyChunkFromNewVoxelStorageDoesNotDisruptNetwork ) {
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	const Vector3int16 commonLocation(23, 34, 45);
	const Cell newClientCell = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_Z);
	const Cell newServerCell = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);

	clientCluster->getVoxelGrid()->setCell(commonLocation, newClientCell, CELL_MATERIAL_Grass);
	serverCluster->getVoxelGrid()->setCell(commonLocation, newServerCell, CELL_MATERIAL_Grass);

	// verify client has chunks at cell location
	{
		Grid::Region r = clientCluster->getVoxelGrid()->getRegion(commonLocation, commonLocation);
		BOOST_CHECK(!r.isGuaranteedAllEmpty());
	}

	clientCluster->getVoxelGrid()->setCell(commonLocation, Constants::kUniqueEmptyCellRepresentation, CELL_MATERIAL_Water);

	// verify client voxel grid no longer has chunks at that location
	{
		Grid::Region r = clientCluster->getVoxelGrid()->getRegion(commonLocation, commonLocation);
		BOOST_CHECK(r.isGuaranteedAllEmpty());
	}

	// send a packet that includes that location (null chunk)
	{
		ReadWriteBuffer buffer;
		clientWrapper->sendClusterDeltasHelper(buffer);
		serverWrapper->receiveClusterHelper(buffer);
	}

	// verify server state is all good in the voxel grid
	BOOST_CHECK_EQUAL(Constants::kUniqueEmptyCellRepresentation, serverCluster->getVoxelGrid()->getCell(commonLocation));
	BOOST_CHECK_EQUAL(CELL_MATERIAL_Water, serverCluster->getVoxelGrid()->getCellMaterial(commonLocation));
}


BOOST_AUTO_TEST_CASE( SendAFewChunksOfRepeats ) {
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	Cell cell = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_NegZ);
	CellMaterial material = CELL_MATERIAL_Sand;

	for (unsigned int x = 0; x < 64; ++x) {
		for (unsigned int y = 0; y < 16; ++y) {
			for (unsigned int z = 0; z < 32; ++z) {
				clientCluster->getVoxelGrid()->setCell(Vector3int16(x, y, z), cell, material);
			}
		}
	}

	countPackets(clientWrapper, serverWrapper);
	countPackets(serverWrapper, clientWrapper);

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());

	for (unsigned int x = 0; x < 64; ++x) {
		for (unsigned int y = 0; y < 16; ++y) {
			for (unsigned int z = 0; z < 32; ++z) {
				char messageBuf[256];
				snprintf(messageBuf, 256, "Server@(%d,%d,%d)", x, y, z); 
				BOOST_CHECK_MESSAGE(cell == serverCluster->getVoxelGrid()->getCell(Vector3int16(x,y,z)), messageBuf);
				BOOST_CHECK_MESSAGE(material == serverCluster->getVoxelGrid()->getCellMaterial(Vector3int16(x,y,z)), messageBuf);

				snprintf(messageBuf, 256, "Client@(%d,%d,%d)", x, y, z);
				BOOST_CHECK_MESSAGE(cell == clientCluster->getVoxelGrid()->getCell(Vector3int16(x,y,z)), messageBuf);
				BOOST_CHECK_MESSAGE(material == clientCluster->getVoxelGrid()->getCellMaterial(Vector3int16(x,y,z)), messageBuf);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( AllCellPermutations ) {

	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster));

	for (unsigned int i = 0; i <= 255; ++i) {
		Cell data = Cell::readUnsignedCharFromDeprecatedUse(i);
		if (data.solid.getBlock() != CELL_BLOCK_Empty) {
			Cell substitute;
			substitute.solid.setBlock((CellBlock)(data.solid.getBlock() % 5));
			substitute.solid.setOrientation(data.solid.getOrientation());
			data = substitute;
			for (unsigned char m = CELL_MATERIAL_Grass; m < CELL_MATERIAL_Water; ++m) {
				clientCluster->getVoxelGrid()->setCell(Vector3int16(i, 1, m), data, (CellMaterial)m);
			}
		} else {
			clientCluster->getVoxelGrid()->setCell(Vector3int16(i, 0, 0), data, CELL_MATERIAL_Water);
		}
	}

	countPackets(clientWrapper, serverWrapper);
	countPackets(serverWrapper, clientWrapper);

	BOOST_CHECK_EQUAL(0, serverWrapper->getReplicationData().updateBuffer.size());
	BOOST_CHECK_EQUAL(0, clientWrapper->getReplicationData().updateBuffer.size());

	for (unsigned int i = 0; i <= 255; ++i) {
		unsigned char data = i;
		if (Cell::readUnsignedCharFromDeprecatedUse(data).solid.getBlock() != CELL_BLOCK_Empty) {
			for (unsigned char m = 1; m < CELL_MATERIAL_Water; ++m) {
				BOOST_CHECK_EQUAL(
					clientCluster->getVoxelGrid()->getCell(Vector3int16(i, 1, m)),
					serverCluster->getVoxelGrid()->getCell(Vector3int16(i, 1, m)));
				BOOST_CHECK_EQUAL(
					clientCluster->getVoxelGrid()->getCellMaterial(Vector3int16(i, 1, m)),
					serverCluster->getVoxelGrid()->getCellMaterial(Vector3int16(i, 1, m)));
			}
		} else {
			BOOST_CHECK_EQUAL(
				clientCluster->getVoxelGrid()->getCell(Vector3int16(i, 0, 0)),
				serverCluster->getVoxelGrid()->getCell(Vector3int16(i, 0, 0)));
			BOOST_CHECK_EQUAL(
				clientCluster->getVoxelGrid()->getCellMaterial(Vector3int16(i, 0, 0)),
				serverCluster->getVoxelGrid()->getCellMaterial(Vector3int16(i, 0, 0)));
		}
	}
}

BOOST_AUTO_TEST_CASE( ManyPlayers ) {
	PacketCountTestParameters params;
	params.PlayerCount = 100;
	params.BigUpdatePlayerCount = 20;
	params.BigUpdateDimX = 20;
	params.BigUpdateDimY = 10;
	params.BigUpdateDimZ = 20;
	params.DesiredSendHertz = 30;
	params.MaxSecondsToResolveNetworkUpdates = 1;
	
	runPacketCountTest(params);
}

BOOST_AUTO_TEST_CASE( SmallerGameIsFast ) {
	PacketCountTestParameters params;
	params.PlayerCount = 10;
	params.BigUpdatePlayerCount = 2;
	params.BigUpdateDimX = 20;
	params.BigUpdateDimY = 10;
	params.BigUpdateDimZ = 20;
	params.DesiredSendHertz = 30;
	params.MaxSecondsToResolveNetworkUpdates = 0.1f;

	runPacketCountTest(params);
}

BOOST_AUTO_TEST_CASE( TinyGameIsRealTime ) {
	PacketCountTestParameters params;
	params.PlayerCount = 2;
	params.BigUpdatePlayerCount = 1;
	params.BigUpdateDimX = 20;
	params.BigUpdateDimY = 10;
	params.BigUpdateDimZ = 20;
	params.DesiredSendHertz = 30;
	params.MaxSecondsToResolveNetworkUpdates = 0.04f;

	runPacketCountTest(params);
}

BOOST_AUTO_TEST_SUITE_END()

static CellMaterial produceRandomMaterial(unsigned hash)
{
	CellMaterial material = (CellMaterial)((hash % (MAX_CELL_MATERIALS-1))+1);
	RBXASSERT(material >= CELL_MATERIAL_Grass && material <= CELL_MATERIAL_Water);

	return material;
}

static Cell produceRandomCell(CellMaterial material)
{
	Cell cell = createCell(CELL_BLOCK_Empty, CELL_ORIENTATION_NegZ);
	if (material != CELL_MATERIAL_Water) {
		int cellBlock = rand() % (CELL_BLOCK_HorizontalWedge+1);
		RBXASSERT(cellBlock < MAX_CELL_BLOCKS);
		int orientation = rand() % MAX_CELL_ORIENTATIONS;
		cell = createCell((CellBlock)cellBlock, (CellOrientation)orientation);
	}
	return cell;
}

static void generateReasonableTerrain(unsigned maxheight, unsigned maxdelta, MegaClusterInstance* cluster)
{
	// For each cell, generate random height, fill it with solid blocks and last one with random wedge
	// Change materials randomly per half-chunk

	Region3int16 extents(Vector3int16(0,0,0), Vector3int16(511, 63, 511));

	Vector2int16 prevHalfChunk(0,0);
	CellMaterial material = CELL_MATERIAL_Grass;
	unsigned averageHeight = 0;

	for (int x = extents.getMinPos().x; x < extents.getMaxPos().x; x++)
		for (int z = extents.getMinPos().z; z < extents.getMaxPos().z; z++)
		{
			Vector2int16 halfChunk = Vector2int16 (x / (kXZ_CHUNK_SIZE/2), z / (kXZ_CHUNK_SIZE/2));

			if(halfChunk != prevHalfChunk) {
				prevHalfChunk = halfChunk;
				material = produceRandomMaterial(unsigned(halfChunk.x + halfChunk.y));
				averageHeight = unsigned(halfChunk.x + halfChunk.y) % (maxheight - maxdelta);
			}

			int height = averageHeight + rand() % maxdelta;

			for (int y = 0; y < height; y++)
			{
				Cell cell = (y != height-1) ? 
					createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ) : produceRandomCell(material);
				if (material != CELL_MATERIAL_Water)
					cluster->getVoxelGrid()->setCell(Vector3int16(x,y,z), cell, material);
			}
		}
}

BOOST_AUTO_TEST_SUITE(ClusterChunksReplication)
BOOST_AUTO_TEST_CASE(ChunkSend) {
	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	const int seed = 42;
	srand(seed);

	serverCluster->initialize();

	// Fill inside of 2x2x2 with random crap. Use offset to get to 8 chunks
	Vector3int16 offset(kXZ_CHUNK_SIZE/2, kY_CHUNK_SIZE/2, kXZ_CHUNK_SIZE/2);

	for (int y = 0; y < kY_CHUNK_SIZE; y++)
		for (int x = 0; x < kXZ_CHUNK_SIZE; x++)
			for (int z = 0; z < kXZ_CHUNK_SIZE; z++)
			{
				CellMaterial material = produceRandomMaterial(rand());
				Cell cell = produceRandomCell(material);
				serverCluster->getVoxelGrid()->setCell(offset + Vector3int16(x,y,z), cell, material);
			}

	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster, true));

	countChunkPackets(serverWrapper, clientWrapper);

	for (int y = 0; y < 2*kY_CHUNK_SIZE; y++)
		for (int x = 0; x < 2*kXZ_CHUNK_SIZE; x++)
			for (int z = 0; z < 2*kXZ_CHUNK_SIZE; z++)
			{
				Cell cellOrig = serverCluster->getVoxelGrid()->getCell(Vector3int16(x,y,z));
				Cell cellReplicated = clientCluster->getVoxelGrid()->getCell(Vector3int16(x,y,z));
				BOOST_CHECK_EQUAL(cellOrig, cellReplicated);
			}
}

BOOST_AUTO_TEST_CASE(EmptyTerrainDoesNotGetReplicatedToNewPlayers) {
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());

	serverCluster->initialize();

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);

	serverCluster->getVoxelGrid()->setCell(Vector3int16(0,0,0), cell, CELL_MATERIAL_Grass);

	// connect a client with a non-empty mega cluster
	{
		shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster, true));
		shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
		shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

		BOOST_CHECK(!clientCluster->isAllocated());

		countChunkPackets(serverWrapper, clientWrapper);

		BOOST_CHECK(clientCluster->isAllocated());

		serverWrapper->testClose();
		clientWrapper->testClose();
	}

	serverCluster->getVoxelGrid()->setCell(Vector3int16(0,0,0), Constants::kUniqueEmptyCellRepresentation, CELL_MATERIAL_Water);

	// connect a client with an empty mega cluster
	{
		shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster, true));
		shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
		shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));

		BOOST_CHECK(!clientCluster->isAllocated());

		countChunkPackets(serverWrapper, clientWrapper);

		BOOST_CHECK(!clientCluster->isAllocated());

		serverWrapper->testClose();
		clientWrapper->testClose();
	}
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ClusterChunksReplicationStress)
BOOST_AUTO_TEST_CASE(SendRandomTerrain) {
	// Disabling RBXASSERT checks speeds up this test somewhat:
	// FLog::Asserts = 0;
	int DesiredSendHertz = 30;
	float TargetSeconds = 20;
	const int seed = 42;
	srand(seed);

	shared_ptr<MegaClusterInstance> clientCluster(Creatable<Instance>::create<MegaClusterInstance>());
	shared_ptr<ReplicatorTestWrapper> clientWrapper(ReplicatorTestWrapper::client(clientCluster));
	shared_ptr<MegaClusterInstance> serverCluster(Creatable<Instance>::create<MegaClusterInstance>());
	serverCluster->initialize();
	generateReasonableTerrain(kY_CHUNK_SIZE, 2, serverCluster.get());
	// Create replicator, allow sending chunks
	shared_ptr<ReplicatorTestWrapper> serverWrapper(ReplicatorTestWrapper::server(serverCluster, true));

	int packets = countChunkPackets(serverWrapper, clientWrapper);

	BOOST_CHECK_LE(packets,DesiredSendHertz*TargetSeconds);
	// BOOST_WARN_EQUAL(0, packets);
}


BOOST_AUTO_TEST_SUITE_END()
