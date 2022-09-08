#include "v8datamodel/PlatformService.h"
#include <boost/unordered_set.hpp>
#include "VoiceChatBase.h"
using namespace Microsoft::Xbox::Services;
using namespace Microsoft::Xbox::Services::Multiplayer;
namespace RBX
{
    class VoiceChatBase;
}

class XboxMultiplayerManager: public enable_shared_from_this <XboxMultiplayerManager>
{
public:	
	XboxMultiplayerManager(XboxLiveContext^ xblContext, GUID newPlayerSessionGUID,int newRobloxUserId = -1 );
	~XboxMultiplayerManager();

	void startMultiplayerListeners();
	void stopMultiplayerListeners();

	void joinMultiplayerSession(Platform::String^ handleID, Platform::String^ xuid);

	void createGameSession(int pmpCreatorId = 0);
	void joinGameSession(Platform::String^ gameSessionUri);
	void leaveGameSession();

	WriteSessionResult^ createPartySession();
	void joinPartySession(Platform::String^ partySessionUri);	
	void leavePartySession();

	MultiplayerSession^ createNewDefaultSession( Platform::String^ templateName, Platform::String^ sessionName, int pmpCreatorId = 0);

	void sendGameInvite();

    void setTeamChannel(unsigned char channelId);

	void recentPlayersList();

	void getInGamePlayers(shared_ptr<RBX::Reflection::ValueArray> values);
	int requestJoinParty();


    void updatePlayer(unsigned int rbxId, float distance, RBX::VoiceChatBase::TalkingDelta* talkingDeltaOut);
    void muteVoiceChatPlayer(unsigned rbxId);
    void unmuteVoiceChatPlayer(unsigned rbxId);
    unsigned voiceChatGetState(int userId);

	void xbEventMultiplayerRoundStart();
	void xbEventMultiplayerRoundEnd();	
	int getLocalRobloxUserId() {return localRobloxUserId;}

	int getPMPCreatorId();

	static bool parseFromXboxCustomJson(Platform::String^ jsonStr,std::string str, int* out);
private:
	XboxLiveContext^			xboxLiveContext;
	MultiplayerSession^         gameSession;
	MultiplayerSession^         partySession;

	GUID playerSessionGuid;

	RBX::Timer<RBX::Time::Precise>					gametimer;

	Windows::Foundation::EventRegistrationToken		sessionChangeToken;
	Windows::Foundation::EventRegistrationToken		subscriptionLostToken;

	int localRobloxUserId;

	RBX::mutex	stateLock;
	RBX::mutex	processLock;
	RBX::mutex	taskLock;
	RBX::mutex	voiceUsersLock;

	bool expectedLeaveSession;
	unsigned long long lastProcessedGameChange;

	void onSessionChanged( Platform::Object^ object, RealTimeActivity::RealTimeActivityMultiplayerSessionChangeEventArgs^ arg );
	void processSessionDeltas( MultiplayerSession^ currentSession, MultiplayerSession^ previousSession, unsigned long long lastChange );
	void onSubscriptionLost( Platform::Object^ object, RealTimeActivity::RealTimeActivityMultiplayerSubscriptionsLostEventArgs^ arg ); 

	//event MultiplayerStateChanged^ OnMultiplayerStateChanged;
	void processSessionChange( MultiplayerSession^ session );
	WriteSessionResult^ updateAndProcessSession( XboxLiveContext^ context, MultiplayerSession^ session, MultiplayerSessionWriteMode writeMode );
	WriteSessionResult^ writeSession( XboxLiveContext^ context, MultiplayerSession^ session, MultiplayerSessionWriteMode writeMode );

	inline bool isMultiplayerSessionChangeTypeSet( MultiplayerSessionChangeTypes value, MultiplayerSessionChangeTypes check )
	{ return (value & check) == check; }

    boost::shared_ptr<RBX::VoiceChatBase> voiceChat;

    typedef boost::unordered_map<unsigned, unsigned> RbxIdToSessionIdMap;
    RbxIdToSessionIdMap rbxIdToSessionId;
    boost::unordered_set<unsigned> mutedUsersRbxIds;     // Why is this here? It is to keep list of muted user while you play the game. We don't like it here too. 
}; 

