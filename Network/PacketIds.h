
#pragma once

#include "MessageIdentifiers.h"
#include "PacketPriority.h"

namespace RBX { namespace Network {

	enum {
	ID_SET_GLOBALS = ID_USER_PACKET_ENUM,   // Sent by Server to Client

	ID_TEACH_DESCRIPTOR_DICTIONARIES,

	ID_DATA,
	ID_REQUEST_MARKER,

	ID_PHYSICS,
	ID_PHYSICS_TOUCHES,

	ID_CHAT_ALL,
	ID_CHAT_TEAM,
	ID_REPORT_ABUSE,

	ID_SUBMIT_TICKET,
	
	ID_CHAT_GAME,
	ID_CHAT_PLAYER,
	ID_CLUSTER,

	ID_PROTOCAL_MISMATCH,

	ID_SPAWN_NAME,

    ID_PROTOCOL_SYNC,
    ID_SCHEMA_SYNC,

    ID_PLACEID_VERIFICATION,

    ID_DICTIONARY_FORMAT,

	ID_HASH_MISMATCH,
	ID_SECURITYKEY_MISMATCH,

	ID_REQUEST_STATS
	};

	// Order of packets on a connection:
	//  Client --> Server ID_NEW_INCOMING_CONNECTION
	//  If server accepts connection, the following happen:
	//  Client --> Server ID_SUBMIT_TICKET (contains client info like network protocol version)
	//  Server --> Client ID_SET_GLOBALS (contains all the top replication containers, stuff like workspace, lighting, etc.)
	//  Client --> Server ID_INSTANCE_NEW
	//  Client --> Server ID_INSTANCE_NEW
	//  ...
	//  Client --> Server ID_REQUEST_CHARACTER

	const PacketPriority PHYSICS_GENERAL_PRIORITY = MEDIUM_PRIORITY;
	const int PHYSICS_CHANNEL = 0;

	const PacketPriority DATAMODEL_PRIORITY = MEDIUM_PRIORITY;
	const PacketReliability DATAMODEL_RELIABILITY = RELIABLE_ORDERED;
	const int DATA_CHANNEL = 0;

	const PacketPriority CHAT_PRIORITY = HIGH_PRIORITY;
	const PacketReliability CHAT_RELIABILITY = RELIABLE;
	const int CHAT_CHANNEL = 2;

} }
