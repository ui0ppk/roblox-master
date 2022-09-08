#include "Replicator.RockyItem.h"

#include "Item.h"
#include "VMProtect/VMProtectSDK.h"
#include "NetPmc.h"
#include "ClientReplicator.h"
#include "v8datamodel/DataModel.h"
#include <boost/bind.hpp>

namespace RBX {
namespace Network {

Replicator::RockyItem::RockyItem(Replicator* replicator, MccReport& report)
		: Item(*replicator), report(report) {}

bool Replicator::RockyItem::write(RakNet::BitStream& bitStream) {
	writeItemType(bitStream, ItemTypeRocky);
    bitStream << static_cast<unsigned char>(RockeyMccReportClient);
    bitStream << report.memcheckRunTime;
    bitStream << report.memcheckDoneTime;
    bitStream << report.mccRunTime;
    bitStream << report.badAppRunTime;
    bitStream << report.localChecksEncoded;
	return true;
}

#ifndef RBX_STUDIO_BUILD
Replicator::NetPmcResponseItem::NetPmcResponseItem(Replicator* replicator, uint32_t response, uint64_t correct, uint8_t idx)
    : Item(*replicator)
    , response(response)
    , correct(correct)
    , idx(idx) { }

bool Replicator::NetPmcResponseItem::write(RakNet::BitStream& bitStream)
{
	writeItemType(bitStream, ItemTypeRocky);
    bitStream << static_cast<unsigned char>(RockeyNetPmcResponse);
    bitStream << idx;
    bitStream << response;
    bitStream << correct;
	return true;
}

Replicator::RockyDbgItem::RockyDbgItem(Replicator* replicator, const std::vector<CallChainInfo>& info)
    : Item(*replicator)
    , info(info)
{}

bool Replicator::RockyDbgItem::write(RakNet::BitStream& bitStream)
{
	writeItemType(bitStream, ItemTypeRocky);
    bitStream << static_cast<unsigned char>(RockeyCallInfo);
    bitStream << static_cast<uint8_t>(info.size());
    for (size_t i = 0; i < static_cast<uint8_t>(info.size()); ++i)
    {
        bitStream << info[i].handler;
        bitStream << info[i].ret;
    }
	return true;
}
#endif

#ifdef RBX_RCC_SECURITY
Replicator::NetPmcChallengeItem::NetPmcChallengeItem(Replicator* replicator, uint8_t idx)
    : Item(*replicator)
    , idx(idx) { }

bool Replicator::NetPmcChallengeItem::write(RakNet::BitStream& bitStream)
{
	writeItemType(bitStream, ItemTypeRocky);
    bitStream << static_cast<unsigned char>(RockeyNetPmcChallenge);
    bitStream << idx;
    const RBX::Security::NetPmcChallenge& key = RBX::Security::netPmcKeys[idx]; 
    bitStream << key.base;
    bitStream << key.size;    
    bitStream << key.seed; 
    bitStream << key.result;
	return true;
}
#endif


void DeserializedRockyItem::process(Replicator& replicator) 
{
#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD)
	ClientReplicator* rep = rbx_static_cast<ClientReplicator*>(&replicator);
    DataModel::get(rep)->submitTask(
        boost::bind(&ClientReplicator::doNetPmcCheck, shared_from(rep), idx, challenge), DataModelJob::Write);
#endif
}

shared_ptr<DeserializedItem> Replicator::RockyItem::read(Replicator& replicator, RakNet::BitStream& bitStream)
{
	shared_ptr<DeserializedRockyItem> deserializedData(new DeserializedRockyItem());

	ClientReplicator* rep = rbx_static_cast<ClientReplicator*>(&replicator);
	rep->readRockyItem(bitStream, deserializedData->idx, deserializedData->challenge);
	
	return deserializedData;
}

}
}
