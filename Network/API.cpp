/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */


// third time's the charm
#include "network/API.h"
#include "Client.h"
#include "Server.h"
#include "ServerReplicator.h"
#include "ClientReplicator.h"
#include "Players.h"
#include "Player.h"
#include "Marker.h"

#include "NetworkSettings.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/GlobalSettings.h"
#include "V8DataModel/HackDefines.h"
#include "V8DataModel/Workspace.h"
#include "GuidRegistryService.h"
#include "RakNetVersion.h"
#include "util/Utilities.h"
#include "FastLog.h"

// RakNet
#include "StringCompressor.h"
#include "StringTable.h"

#include "VMProtect/VMProtectSDK.h"

#include <boost/algorithm/string.hpp>

FASTSTRINGVARIABLE(ClientExternalBrowserUserAgent, "Roblox/WinInet")

#define ROBLOX_URL_IDENTIFIER "roblox.com/"
#define ROBLOXLABS_URL_IDENTIFIER ".robloxlabs.com/"

#pragma comment (lib, "Ws2_32.lib")

std::string RBX::Network::versionB;
std::string RBX::Network::securityKey;
unsigned int RBX::initialProgramHash = 0;

#if RAKNET_PROTOCOL_VERSION!=5
#error
#endif


RBX_REGISTER_CLASS(RBX::Network::Client);
RBX_REGISTER_CLASS(RBX::Network::Server);
RBX_REGISTER_CLASS(RBX::Network::Player);
RBX_REGISTER_CLASS(RBX::Network::Players);
RBX_REGISTER_CLASS(RBX::NetworkSettings);
RBX_REGISTER_CLASS(RBX::Network::Peer);
RBX_REGISTER_CLASS(RBX::Network::Marker);
RBX_REGISTER_CLASS(RBX::Network::Replicator);
RBX_REGISTER_CLASS(RBX::Network::ServerReplicator);
RBX_REGISTER_CLASS(RBX::Network::ClientReplicator);
RBX_REGISTER_CLASS(RBX::Network::GuidRegistryService);

RBX_REGISTER_ENUM(PacketPriority);
RBX_REGISTER_ENUM(PacketReliability);
RBX_REGISTER_ENUM(RBX::Network::FilterResult);
RBX_REGISTER_ENUM(RBX::Network::Player::MembershipType);
RBX_REGISTER_ENUM(RBX::Network::Player::ChatMode);
RBX_REGISTER_ENUM(RBX::Network::Players::PlayerChatType);
RBX_REGISTER_ENUM(RBX::Network::Players::ChatOption);
RBX_REGISTER_ENUM(RBX::NetworkSettings::PhysicsSendMethod);
RBX_REGISTER_ENUM(RBX::NetworkSettings::PhysicsReceiveMethod);

namespace RBX {
	namespace Network {
		// Prevent string compressors from being created at the same time
		class SafeInitFree
		{
		public:
			SafeInitFree()
			{
				RakNet::StringCompressor::AddReference();
				RakNet::StringTable::AddReference();
			}
			~SafeInitFree()
			{
				RakNet::StringCompressor::RemoveReference();
				RakNet::StringTable::RemoveReference();
			}
		};
	}
}

static bool _isPlayerAuthenticationEnabled;

bool RBX::Network::isPlayerAuthenticationEnabled()
{
	return _isPlayerAuthenticationEnabled;
}

bool RBX::Network::isNetworkClient(const Instance* context)
{
	return ServiceProvider::find<Client>(context) != NULL;
}

#if defined(RBX_RCC_SECURITY)
static shared_ptr<RBX::Network::ServerReplicator> createSecureReplicator(RakNet::SystemAddress a, RBX::Network::Server* s, RBX::NetworkSettings* networkSettings)
{
	return RBX::Creatable<RBX::Instance>::create<RBX::Network::CheatHandlingServerReplicator>(a, s, networkSettings);
}
#endif

static void initVersion1()
{

	// security key: generated externally (version+platform+product+salt), modify per release, send from client to server

#if (defined(_WIN32) && (defined(LOVE_ALL_ACCESS) || defined(_NOOPT) || defined(_DEBUG) || defined(RBX_TEST_BUILD))) || (defined(__APPLE__) && defined(__arm__)) || defined(__ANDROID__) || (defined(__APPLE__) && defined(__aarch64__)) || defined(RBX_PLATFORM_DURANGO)
	// If we are Apple iOS, Android, or if we are Windows noopt/debug/test, use this fixed key:
	// INTERNALiosapp, 2e427f51c4dab762fe9e3471c6cfa1650841723b
	RBX::Network::securityKey = RBX::rot13("2r427s51p4qno762sr9r3471p6psn1650841723o");
#elif defined(_WIN32)
	//0.235.0pcplayeraskljfLUZF, sha1: 2d83a36443889941b170fe68cfe3cc6d718b1abb, then rot13 and put below
	RBX::Network::securityKey = RBX::rot13("2q83n36443889941o170sr68psr3pp6q718o1noo");
#elif defined(__APPLE__) && defined(__i386)
	//0.235.0macplayeraskljfLUZF, sha1: a7377e56dedd0d8e0fecafbec93cf00f0b2edca1, then rot13 and put below
	RBX::Network::securityKey = RBX::rot13("n7377r56qrqq0q8r0srpnsorp93ps00s0o2rqpn1");
#else
    #error "Unsupported platform"
#endif

	RBX::Network::versionB += '7';
	RBX::Network::versionB += (char)79;
}

static void initVersion2()
{
	// TODO: Obfuscate even more?
	RBX::Network::versionB += "l";
	RBX::Network::versionB += 'E';
}



void RBX::Network::initWithServerSecurity()
{
	initVersion1();
	initWithoutSecurity();
#if defined(RBX_RCC_SECURITY)
	_isPlayerAuthenticationEnabled = true;
	Server::createReplicator = createSecureReplicator;
#endif
	initVersion2();
}


void RBX::Network::initWithPlayerSecurity()
{
	initVersion1();
	initWithoutSecurity();
	initVersion2();
}

void RBX::Network::initWithCloudEditSecurity()
{
	// Keep this in sync with initWithoutSecurity password
	versionB = "";
	versionB += "^";
	versionB += (char)17;
}

void RBX::Network::initWithoutSecurity()
{
	VMProtectBeginMutation("22");
	_isPlayerAuthenticationEnabled = false;

	static SafeInitFree safeInitFree;

	// Forces registration of Descriptors
	Client::classDescriptor();
	Server::classDescriptor();
	versionB += "^";
	Player::classDescriptor();
	Players::classDescriptor();
	GlobalAdvancedSettings::classDescriptor();
	NetworkSettings::classDescriptor();
	versionB += (char)17;

	// Force instantiation of NetworkSettings singleton
	NetworkSettings::singleton();
	VMProtectEnd();
}

void RBX::Network::setVersion(const char* version)
{
	if (version)
		versionB = version;
}

void RBX::Network::setSecurityVersions(const std::vector<std::string>& versions)
{
	Server::setAllowedSecurityVersions(versions);
}

bool RBX::Network::isTrustedContent(const char* url)
{
	if(!RBX::ContentProvider::isUrl(url))
		return false;

	bool kSkipNetworkTrustedContentCheck = true;
	if (kSkipNetworkTrustedContentCheck)
	{
		return true;
	}

	std::string urlString(url);
	boost::algorithm::to_lower(urlString);

    bool isRobloxLabsUrl = false;
	size_t foundPos = urlString.find(ROBLOX_URL_IDENTIFIER);
	if(foundPos == std::string::npos)
    {
        foundPos = urlString.find(ROBLOXLABS_URL_IDENTIFIER);
        if(foundPos == std::string::npos)
            return false;
        
        isRobloxLabsUrl = true;
    }
    
	urlString = urlString.substr(foundPos,std::string::npos); // remove all of string before URL_IDENTIFIER
    
     // put our iterator at end of URL_IDENTIFIER
    if(isRobloxLabsUrl)
        foundPos = sizeof(ROBLOXLABS_URL_IDENTIFIER) - 1;
    else
        foundPos = sizeof(ROBLOX_URL_IDENTIFIER) - 1;

	while(foundPos < urlString.size() && (urlString[foundPos] == '\\' || urlString[foundPos] == '/'))
		++foundPos;

	if(foundPos >= urlString.size())
		return false;

	return	urlString.substr(foundPos,5)	== "asset"			|| 
		urlString.substr(foundPos,4)		== "game"			|| 
		urlString.substr(foundPos,9)		== "analytics"		||
		urlString.substr(foundPos,3)		== "ide"			|| 
		urlString.substr(foundPos,6)		== "images"			||
		urlString.substr(foundPos,6)		== "thumbs"			||
		urlString.substr(foundPos,2)		== "ui"				||
		urlString.substr(foundPos,11)		== "persistence"	||
		urlString.substr(foundPos,8)		== "rolesets"		||
		urlString.substr(foundPos,4)		== "auth"			||
		urlString.substr(foundPos,8)		== "currency"		||
		urlString.substr(foundPos,11)		== "marketplace"	||
		urlString.substr(foundPos,9)		== "ownership"		||
		urlString.substr(foundPos,13)		== "placerolesets";
}

#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
namespace {
void isDebuggedDirectThreadFunc(weak_ptr<RBX::DataModel> weakDataModel) {
#if !defined(LOVE_ALL_ACCESS) && !defined(_NOOPT) && !defined(_DEBUG)
	VMProtectBeginMutation("23");
	static const int kSleepBetweenChecksMillis = 1500;

	while (true) {
		shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock();
		if (!dataModel) { break; }

		unsigned int mask = static_cast<unsigned int>(
			VMProtectIsDebuggerPresent(true /*check for kernel debuggers too*/)) *
			HATE_DEBUGGER;
		dataModel->addHackFlag(mask);

		boost::this_thread::sleep(boost::posix_time::milliseconds(kSleepBetweenChecksMillis));
	}
	VMProtectEnd();
#endif
}
}

void RBX::spawnDebugCheckThreads(weak_ptr<RBX::DataModel> dataModel) {
	boost::thread t(boost::bind(&isDebuggedDirectThreadFunc, dataModel));
}
#endif
