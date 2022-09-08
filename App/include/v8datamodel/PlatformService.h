#pragma once

#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"
#include <stdint.h>
#include "v8datamodel/InputObject.h"

namespace RBX {
	extern const char* const sPlatformService;

    // used for startGame2()
    enum GameJoinType
    {
        GameJoin_Normal,   // id = place ID
        GameJoin_Instance, // id = game instance ID
        GameJoin_Follow,   // id = roblox player ID -or- playerName = name of the player to follow. non-empty playerName takes precedence
		GameJoin_PMPCreator, // id = placeID		
        GameJoin_PartyLeader, // id not used
        GameJoin_Party,    // id not used
        GameJoin__MaxCnt, // not used
    };

    enum GameStartResult
    {
        GameStart_Weird      = -1, // this should never happen, talk to Max ASAP
        GameStart_OK         = 0, // ok
        GameStart_Already    = 1, // the game is already running
        GameStart_WebError   = 2, // could not connect to the web server
        GameStart_NoAccess   = 3, // access denied by the web server
        GameStart_NoInstance = 4, // game instance not found (e.g. dead)
        GameStart_Full       = 5, // game instance is full
        GameStart_NoPlayer   = 6, // could not follow the player
    };

    enum AwardResult
    {
        Award_OK        = 0, // ok
        Award_Fail      = 1, // tried sending but failed, (net/xblive/roblox error)
        Award_NotFound  = 2, // Achievement not found
        Award_NoUser    = 3, // user not logged in
    };

	enum AccountAuthResult
	{
		AccountAuth_Error					= -1,
		AccountAuth_Success					= 0,
		AccountAuth_InProgress				= 1,
		AccountAuth_AccountUnlinked			= 2,
		AccountAuth_MissingGamePad			= 3,
		AccountAuth_NoUserDetected			= 4,
		AccountAuth_HttpErrorDetected		= 5,
		AccountAuth_SignUpDisabled			= 6,
		AccountAuth_Flooded					= 7,
		AccountAuth_LeaseLocked				= 8,
		AccountAuth_AccountLinkingDisabled	= 9,
		AccountAuth_InvalidRobloxUser		= 10,
		AccountAuth_RobloxUserAlreadyLinked	= 11,
		AccountAuth_XboxUserAlreadyLinked	= 12,
		AccountAuth_IllegalChildAccountLinking = 13,
		AccountAuth_InvalidPassword			= 14,
		AccountAuth_UsernamePasswordNotSet	= 15,
		AccountAuth_UsernameAlreadyTaken	= 16,
		AccountAuth_InvalidCredentials      = 17,
	};

	enum ReturnToEngageScreenStatus
	{
		ReturnToEngage_Unknown = 0,
		ReturnToEngage_SignOut = 1,
		ReturnToEngage_Removed = 2,
		ReturnToEngage_InvalidSession = 3,
		ReturnToEngage_UnlinkSuccess = 4,
		ReturnToEngage_DisplayInfoChange = 5,
		ReturnToEngage_ControllerChange = 6,
		ReturnToEngage_AccountReselect = 7
	};

	enum PlatformDatamodelType
	{
		AppShellDatamodel = 0,
		GameDatamodel = 1
	};

	enum PlatformPurchaseResult
	{
		PurchaseResult_Error = -1,			// Unknown error?
		PurchaseResult_Success = 0,			// user successful purchase on platform; still needs transaction resolution on web
		PurchaseResult_UserCancelled = 1,	// user cancelled the purchase request from platform ui
		PurchaseResult_ConsumeRequestFail = 2,	//issues with roblox resolving purchase
		PurchaseResult_RobuxUpdated = 3,	//	
		PurcahseResult_NoActionNeeded		//
	};

    enum XboxKeyBoardType
    {
        xbKeyBoard_Default, // The default keyboard. 
        xbKeyBoard_EmailSmtpAddress, // Email address keyboard. Includes the @ and .com keys. 
        xbKeyBoard_Number, // The number keyboard. Contains numbers and a decimal point. 
        xbKeyBoard_Password, // The password keyboard. Contains the text input pattern for a password. 
        xbKeyBoard_Search, // The search keyboard. Includes auto-complete. 
        xbKeyBoard_TelephoneNumber, // The telephone keyboard. Mimics the telephone keypad.  
        xbKeyBoard_Url // The Url keyboard. Includes the .com key.
    };

    enum VoiceChatState
    {
        voiceChatState_Available,
        voiceChatState_Muted,
        voiceChatState_NotInChat,
        voiceChatState_UnknownUser
    };

    // This interface is used to communicate with the underlying platform.
    // Most functions are called from a separate thread and thus may be blocking.
    struct IPlatformAPI
    {
        // performs player authentication, returns a json string with auth token to (*result), e.g.
        // { "Gamertag": "<gamer tag>", "XboxUserId": "<userid>" } and 0 - on success
        // { "Gamertag": "Cancelled", "XboxUserId": "Cancelled" }  and 1 - if the user cancels auth
        virtual AccountAuthResult	performAuthorization(InputObject::UserInputType gamepadId, bool unLinkedCheck)  = 0;

		// performs account linking for user
		virtual int		performAccountLink(const std::string& accountName, const std::string& password, std::string* response) = 0;
		virtual int		performUnlinkAccount(std::string* response) = 0;
		virtual int		performSetRobloxCredentials(const std::string& accountName, const std::string& password, std::string* response) = 0;
		virtual AccountAuthResult	performHasRobloxCredentials() = 0;
		virtual AccountAuthResult	performHasLinkedAccount() = 0;

        // launches a new game
        // returns 0 on success, 1 if a game is already running, -1 on error
        virtual GameStartResult startGame3( GameJoinType joinType, int id ) = 0;

        // requests a game shutdown, returning immediately
        // if there's no game running, does nothing
        // even if there are several outstanding requests (oops), they are all cleared when the game is actually shut down
        // parameter must always be set to false.
        virtual void requestGameShutdown(bool) = 0;

        // tests network connection, returns:
        // 0  - all good
        // >0 - have problems
        // -1 - error
        virtual int     netConnectionCheck() = 0;

        // fetches a friend list and fills (*result) with a JSON detailing player's friends and their status
        // returns 0 on success, -1 on error
        virtual int fetchFriends(InputObject::UserInputType gamepadId, std::string* result) = 0;

		//pop up the system Help UI
		// returns 0 on success, -1 on error
		virtual int popupHelpUI() = 0;

		//launch an application on the platform for a given Uri
		// returns 0 on success, -1 on error
		virtual int launchPlatformUri(const std::string baseUri) = 0;

        // pops up the system Party UI
        // returns 0 on success, -1 on error
        virtual int popupPartyUI(InputObject::UserInputType gamepadId) = 0;

		// pops up a system profile UI
		// return 0 on success, -1 on error
		virtual int popupProfileUI(InputObject::UserInputType gamepadId, std::string uid) = 0;

		virtual int	popupAccountPickerUI(InputObject::UserInputType gamepadId) = 0 ;

		virtual void popupGameInviteUI() = 0;
        // shows xbox keyboard.
        virtual void showKeyBoard(std::string& title, std::string& description, std::string& defaultText, unsigned keyboardType, DataModel* dm) = 0;

        // guess what
        // px, py = percentages of the nominal resolution, both constrained to [0.1 ... 1.0]
        virtual void    setScreenResolution(double px, double py) = 0;
		
		// request the Catalog info from XboxLiveServices
		// result = JSON table of the catalog info on success
		//		{
		//			"Name" =				(string),
		//			"ReducedName" =			(string),
		//			"Description" =			(string),
		//			"ProductId" =			(string),
		//			"IsBundle" =			(bool),
		//			"IsPartOfAnyBundle" =	(bool),
		//			"TitleId" =				(int),
		//			"DisplayPrice" =		(string),
		//			"DisplayListPrice" =	(string)
		//		}
		virtual int		fetchCatalogInfo(shared_ptr<Reflection::ValueArray> values) = 0;


		// request the Platform User Inventory info from XboxLiveServices
		// result = JSON table of the inventory info on success
		//			{
		//				"ProductId" = (string)__name__
		//				"ConsumableBalance" = (int)__count__
		//			}
		virtual int		fetchInventoryInfo(shared_ptr<RBX::Reflection::ValueArray> values) = 0;

		// request the party members from XboxLiveServices
		// result = list of party members
		virtual int		getPlatformPartyMembers(shared_ptr<Reflection::ValueArray> result) = 0;

		// request the MPSD members from XboxLiveServices
		// result = list of MPSD members by {XUID: RobloxId} pairs
		virtual int		getInGamePlayers(shared_ptr<Reflection::ValueArray> result) = 0;

		// request to make a purchase of a Robux ammount
		// productId = an id recieved from fetchCatalogInfo();
		virtual PlatformPurchaseResult requestPurchase(const std::string& productId) = 0;

		virtual int getPMPCreatorId() = 0;
		virtual int getTitleId() = 0;

		virtual shared_ptr<const Reflection::ValueTable> getVersionIdInfo() = 0;

		virtual shared_ptr<const Reflection::ValueTable> getPlatformUserInfo() = 0;

        // awards an achievement by sending the named event
        // event names can be looked up on XDP or in XboxClient/xdpevents.h (around line 60)
        virtual AwardResult awardAchievement(const std::string& eventName) = 0;
        virtual AwardResult setHeroStat(const std::string& eventName, double* value) = 0;

        virtual void voiceChatSetMuteState(int userId, bool mute) = 0;
        virtual unsigned voiceChatGetState(int userId) = 0;
    };


    class PlatformService
        : public DescribedCreatable<PlatformService, Instance, sPlatformService, Reflection::ClassDescriptor::PERSISTENT_LOCAL>
        , public Service
    {
    private:
        typedef DescribedNonCreatable<PlatformService, Instance, sPlatformService> Super;

    public:
        PlatformService();
        
        static Reflection::BoundProp<float> desc_Brightness;
        static Reflection::BoundProp<float> desc_Contrast;
        static Reflection::BoundProp<float> desc_GrayscaleLevel;
        static Reflection::BoundProp<Color3> desc_TintColor;
        static Reflection::BoundProp<float> desc_BlurIntensity;

		static Reflection::PropDescriptor<PlatformService, int> prop_DatamodelType;

        rbx::signal<void(int)> viewChanged;
		rbx::signal<void(int)> gameJoinedSignal;
		rbx::signal<void()> userAuthCompleteSignal;	
		rbx::signal<void(int)> userAccountChangeSignal;
		rbx::signal<void(int)>	robuxAmountChangedSignal;
		rbx::signal<void(std::string)> networkStatusChangedSignal;
        rbx::signal<void(std::string)> keyboardClosedSignal;

		rbx::signal<void(std::string)> lostActiveUser;
		rbx::signal<void(std::string)> gainedActiveUser;

		rbx::signal<void(std::string)> lostUserGamepad;
		rbx::signal<void(std::string)> gainedUserGamepad;

        rbx::signal<void(int)> voiceChatUserTalkingStartSignal;
        rbx::signal<void(int)> voiceChatUserTalkingEndSignal;

		rbx::signal<void()> suspendSignal;

        void beginAuthorization(InputObject::UserInputType gamepadId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void beginAuthUnlinkCheck(InputObject::UserInputType gamepadId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void beginStartGame3(int mode, int placeId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void requestGameShutdown();
        void beginFetchFriends(InputObject::UserInputType gamepadId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void popupPartyUI(InputObject::UserInputType gamepadId, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void popupProfileUI(InputObject::UserInputType gamepadId, std::string uid, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void popupHelpUI();
		void launchPlatformUri(const std::string baseUri);
		void popupGameInviteUI();
		void popupAccountPickerUI(InputObject::UserInputType gamepadId);
		void beginGetPartyMembers(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginGetInGamePlayers(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		
		void beginAccountLink(std::string accountName, std::string password, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginUnlinkAccount(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginSetRobloxCredentials(std::string accountName, std::string password, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginHasLinkedAccount(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginHasRobloxCredentials(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void showKeyBoard(std::string title, std::string description, std::string defaultText, XboxKeyBoardType keyboardType);

		void beginGetPMPCreatorId(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		int getTitleId();

		shared_ptr<const Reflection::ValueTable> getVersionIdInfo();
		shared_ptr<const Reflection::ValueTable> getPlatformUserInfo();
		
		void beginGetInventoryInfo(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginGetCatalogInfo(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void beginPlatformStorePurchase(const std::string productId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void beginAwardAchievement(std::string eventName, boost::function<void(int)> resumeFn, boost::function<void(std::string)> errorFunction);
        void beginHeroStat(std::string eventName, double value, boost::function<void(int)> resumeFn, boost::function<void(std::string)> errorFunction);

        void changeScreenResolution(double px, double py);

        void setPlatform( IPlatformAPI* iface, PlatformDatamodelType newDatamodelType );
		PlatformDatamodelType getPlatformDatamodelType() const;

        void voiceChatSetMuteState(int userId, bool mute);
        VoiceChatState voiceChatGetState(int userId);

        // image post process, kinda hacked in for now. 
        float blurIntensity;
        float brightness;
        float contrast;
        float grayscaleLevel;
        Color3 tintColor;

    private:
        IPlatformAPI* platform;
		PlatformDatamodelType datamodelType;

        volatile long baFlag, bnccFlag, bsgFlag, bffFlag, ppuiFlag, ppruiFlag, bgiiFlag, bgciFlag, bpspFlag, 
			alFlag, bgpmFlag, bgigpFlag; // flags for reenterancy prevention
    };
}