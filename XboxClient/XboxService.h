#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "boost\weak_ptr.hpp"
#include "XbLiveUtils.h"
#include "XboxGameController.h"
#include "XboxMultiplayerManager.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/PlatformService.h"
#include "v8datamodel/TeleportCallback.h"

using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::System;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::Xbox::Services;

namespace RBX
{
	class DataModel;
    class DataModelJob;
    class Verb;
    class Game;
    class ViewBase;
}

class Marshaller;
class RenderJob;
class ControllerBuffer;
class UserIDTranslator;
class KeyboardProvider;

class XboxPlatform: public RBX::IPlatformAPI, public RBX::TeleportCallback
{
public:
    XboxPlatform();
    virtual ~XboxPlatform(); // not that it matters...

	void onProtocolActivated( Windows::ApplicationModel::Activation::IProtocolActivatedEventArgs^ args );
	void onWindowActivatedChanged(Windows::UI::Core::WindowActivatedEventArgs^ args);

    void tick(); // called by main
    std::pair<unsigned, unsigned> getRenderSize() const;
    void  requestGameShutdown(bool teleport) override; // called by exit verb

	void sendConsumeAllRequest(bool is_purchase); 
	void returnToEngagementScreen(RBX::ReturnToEngageScreenStatus status);
	void attemptToSignActiveUserIn(std::string userToSignIn, IController^ controllerToUse);

	void startEventHandlers();
	void stopEventHandlers();

	void suspendViewXbox();
	void resumeViewXbox();
    void onNormalResume();

	void xbEventPlayerSessionStart();
	void xbEventPlayerSessionEnd();
	void xbEventPlayerSessionPause();
	void xbEventPlayerSessionResume();
	
    virtual void voiceChatSetMuteState(int userId, bool mute);
    virtual unsigned voiceChatGetState(int userId);

	User^ currentUser;
	IController^ currentController;
	Microsoft::Xbox::Services::XboxLiveContext^ xboxLiveContext;

private:

    virtual RBX::AccountAuthResult		performAuthorization(RBX::InputObject::UserInputType gamepadId, bool unLinkedCheck) override;

    virtual RBX::GameStartResult	startGame3( RBX::GameJoinType gt, int id );
    virtual int    netConnectionCheck() override;
    virtual void   setScreenResolution(double px, double py) override;
    virtual int    fetchFriends(RBX::InputObject::UserInputType gamepadId, std::string* result);
	virtual int    popupHelpUI();
	virtual int    launchPlatformUri(const std::string baseUri);
	virtual void   popupGameInviteUI();
    virtual int    popupPartyUI(RBX::InputObject::UserInputType gamepadId);
	virtual int    popupProfileUI(RBX::InputObject::UserInputType gamepadId, std::string xuid);
	virtual int	   popupAccountPickerUI(RBX::InputObject::UserInputType gamepadId);
	virtual int	   getPMPCreatorId();
	virtual int    getTitleId();
	virtual shared_ptr<const RBX::Reflection::ValueTable> getVersionIdInfo();
    virtual void   showKeyBoard(std::string& title, std::string& description, std::string& defaultText, unsigned keyboardType, RBX::DataModel* dm);

	virtual shared_ptr<const RBX::Reflection::ValueTable> getPlatformUserInfo() override;

	// linking functions
	virtual int		performAccountLink(const std::string& accountName, const std::string& password, std::string* response) override;
	virtual int		performUnlinkAccount(std::string* response) override;
	virtual int		performSetRobloxCredentials(const std::string& accountName, const std::string& password, std::string* response) override;
	virtual RBX::AccountAuthResult	performHasRobloxCredentials() override;
	virtual RBX::AccountAuthResult	performHasLinkedAccount() override;

	virtual int		fetchInventoryInfo(shared_ptr<RBX::Reflection::ValueArray> values);
	virtual int		fetchCatalogInfo(shared_ptr<RBX::Reflection::ValueArray> values);
	virtual RBX::PlatformPurchaseResult	   requestPurchase(const std::string& productId);

	virtual int getPlatformPartyMembers(shared_ptr<RBX::Reflection::ValueArray> result);
	virtual int	getInGamePlayers(shared_ptr<RBX::Reflection::ValueArray> result);

    virtual RBX::AwardResult awardAchievement(const std::string& eventName) override;
    virtual RBX::AwardResult setHeroStat(const std::string& eventName, double* value) override;

    // Teleport support
    virtual void doTeleport(const std::string& url, const std::string& ticket, const std::string& script) override;
    virtual bool isTeleportEnabled() const override;
    virtual std::string xBox_getGamerTag() const override;
private:
	bool activated; // whether or not the window is in focus
	bool trySignInOnActivation; // set to true when user is logged out and needs to be relogged in

    Marshaller* marshaller; // thread marshaller
    RBX::Game*  intro;   // main menu
    RBX::Game*  game;  // game being played (or null)
    RBX::Game*  current; // what the view is bound to, points to either *intro or *game
    RBX::ViewBase* view; // renderer
    RBX::Verb*  exitVerb; // handles game exit from ESC menu
    RBX::mutex  gameMutex; // use this mutex if modifying game or exitVerb
    volatile long gameShutdownGuard; // prevents shutdown requests from piling up

	// makes sure we don't miss input during low FPS
	shared_ptr<ControllerBuffer> controllerBuffer;

    shared_ptr<RenderJob>     renderJob;
    shared_ptr<XboxGameController> controller;
    shared_ptr<UserIDTranslator> userTranslator;
    shared_ptr<RBX::DataModelJob> partyPoller;
	shared_ptr<RBX::DataModelJob> validSessionPoller;
    shared_ptr<RBX::DataModelJob> voiceChatPoller;

    scoped_ptr<KeyboardProvider> keyboard;
	shared_ptr<XboxMultiplayerManager> multiplayerManager;

    struct PartyStatus
    {
        std::string currentPartyGuid;
        std::string pollPartyGuid;
        bool        leaderInGame;
        DWORD       lastCheckTime;
        bool        tried;
        PartyStatus(): lastCheckTime(0), leaderInGame(false), tried(false) {}
    } 
    partyStatus;

	struct GameInviteHandle{
		std::string targetUserXuid;
		int followUserId;
		int pmpCreator; // used for Play My Place
		GameInviteHandle(): followUserId(-1), targetUserXuid("") {}
	}
	gameInviteHandle;

    void initVoiceChatPoller();
    void removeVoiceChatPoller();

	GUID playerSessionGuid;// used by xbox to track user session info 

    void setPartyPollStatus(const std::string& newGuid, bool leaderIn ); // can only be called on marshaller thread
    void setPartyCreateStatus( const std::string& current ); // can only be called on marshaller thread
    void setPartyTriedStatus();
    void handlePartyJoin(); // can only be called on marshaller thread
    void initPartyPoller();
	void removePartyPoller();
    RBX::GameStartResult prepareGameLaunch( std::string& authenticationUrl, std::string& authenticationTicket, std::string& scriptUrl, RBX::GameJoinType joinType, int id ); // fetches auth, ticket, script
    RBX::GameStartResult initGame( const std::string& authUrl, const std::string& ticket, const std::string joinScriptUrl );

    void switchView( RBX::Game* to ); // 0 to unbind from both

	// these are called when we gain/lose focus of our window
	void onActivated();
	void onDeactivated();

	volatile long returnToEngagementFlag;
	volatile long consumeAllFlag;

	RBX::mutex controllerPairingMutex;

	void storeLocalGamerPicture(XboxLiveContext^ xboxLiveContext);

	void checkValidSessionAsync(); // return to engagement screen if fails
	void initValidSessionPoller();
	void removeValidSessionPoller();

	void initializeAuthEventHandlers();	
	void initializeNetworkConnectionEventHandlers();

	void sampleInput();
	void startControllerBuffering();
	void endControllerBuffering();

	User^ tryToSignInActiveUser(IController ^ controllerToUse);
	void attemptToReconnectController(std::string userToSignIn);

	User^ getActiveUser();

	std::vector<IGamepad^> getGamepadsForCurrentUser();
	void handleCurrentControllerChanged();

	std::vector< Marketplace::CatalogItemDetails^ > catalogItemDetails;

    bool setRichPresence();
	void getCatalogInfo();
	bool purchaseItem( const std::wstring& wstrSignedOffer );
    void requestGameShutdown_nolock(bool teleport);
	bool checkXboxPrivileges(unsigned privilegeID);
    void setGameProgress();

	void shutdownDataModels();

	void fireGameJoinedEvent(RBX::GameStartResult);		// wrapper for firing game joined event

	RBX::AccountAuthResult xboxAuthHttpHelper( const std::string& url, std::string* response, bool isPost);
	RBX::AccountAuthResult authAndInitUser();	
};
