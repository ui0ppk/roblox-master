#include "stdafx.h"

#include "V8DataModel/PlatformService.h"

#include "Reflection/EnumConverter.h"

extern void dprintf( const char* fmt, ... );

#ifndef RBX_PLATFORM_DURANGO // TODO: refactor the whole dprintf() thingy
void dprintf( const char* fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );
    RBX::StandardOut::singleton()->print( RBX::MESSAGE_OUTPUT, RBX::vformat(fmt, ap) );
    va_end(ap);
}
#endif

namespace RBX {

const char* const sPlatformService = "PlatformService";

static Reflection::BoundYieldFuncDesc<PlatformService, int(InputObject::UserInputType)> func_beginAuth(&PlatformService::beginAuthorization, "BeginAuthorization", "gamepadId", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int(InputObject::UserInputType)> func_beginAuth2(&PlatformService::beginAuthUnlinkCheck, "BeginAuthUnlinkCheck", "gamepadId", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int(int,int)> func_beginStartGame3(&PlatformService::beginStartGame3, "BeginStartGame3", "mode", "id", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, void(void)> func_requestGameShutdown(&PlatformService::requestGameShutdown, "RequestGameShutdown", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, std::string(InputObject::UserInputType)> func_beginFetchFriends(&PlatformService::beginFetchFriends, "BeginFetchFriends", "gamepadId",  Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, void(void)> func_popupHelpUI(&PlatformService::popupHelpUI, "PopupHelpUI", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, void(std::string)> func_launchPlatformUri(&PlatformService::launchPlatformUri, "LaunchPlatformUri", "baseUri", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, void(InputObject::UserInputType)> func_popupPartyUI(&PlatformService::popupPartyUI, "PopupPartyUI","gamepadId",  Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, void(InputObject::UserInputType, std::string)> func_popupProfileUI(&PlatformService::popupProfileUI, "PopupProfileUI", "gamepadId", "uid", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, void(InputObject::UserInputType)> func_popupAccountPickerUI(&PlatformService::popupAccountPickerUI, "PopupAccountPickerUI", "gamepadId", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, void()> func_popupGameInviteUI(&PlatformService::popupGameInviteUI, "PopupGameInviteUI", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, shared_ptr<const Reflection::ValueArray>(void)> func_getPlatformPartyMembers(&PlatformService::beginGetPartyMembers, "GetPlatformPartyMembers", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, shared_ptr<const Reflection::ValueArray>(void)> funce_getInGamePlayers(&PlatformService::beginGetInGamePlayers, "GetInGamePlayers", Security::RobloxScript);

static Reflection::BoundYieldFuncDesc<PlatformService, int(std::string, std::string)> func_beginAccountLink(&PlatformService::beginAccountLink, "BeginAccountLink", "accountName", "password", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int(void)> func_beginUnlinkAccount(&PlatformService::beginUnlinkAccount, "BeginUnlinkAccount", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int(std::string, std::string)> func_beginSetRobloxCredentials(&PlatformService::beginSetRobloxCredentials, "BeginSetRobloxCredentials", "accountName", "password", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int()> func_beginHasLinkedAccount(&PlatformService::beginHasLinkedAccount, "BeginHasLinkedAccount", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int()> func_beginHasRobloxCredentials(&PlatformService::beginHasRobloxCredentials, "BeginHasRobloxCredentials", Security::RobloxScript);

static Reflection::BoundYieldFuncDesc<PlatformService, shared_ptr<const Reflection::ValueArray>()> func_fetchCatalogInfo(&PlatformService::beginGetCatalogInfo, "BeginGetCatalogInfo", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, shared_ptr<const Reflection::ValueArray>()> func_fetchInventoryInfo(&PlatformService::beginGetInventoryInfo, "BeginGetInventoryInfo", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int(std::string)> func_beginPlatformStorePurchase(&PlatformService::beginPlatformStorePurchase, "BeginPlatformStorePurchase", "productId",  Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int(std::string)> func_beginAwardAchievement(&PlatformService::beginAwardAchievement, "BeginAwardAchievement", "eventName", Security::RobloxScript );
static Reflection::BoundYieldFuncDesc<PlatformService, int(std::string, double)> func_beginHeroStat(&PlatformService::beginHeroStat, "BeginHeroStat", "eventName", "value", DBL_MIN, Security::RobloxScript );
static Reflection::BoundFuncDesc<PlatformService, void(double,double)> a1 (&PlatformService::changeScreenResolution, "changeScreenResolution", "px", "py", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformService, int()> func_beginGetPMPCreatorId(&PlatformService::beginGetPMPCreatorId, "BeginGetPMPCreatorId", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, int()> func_getTitleId(&PlatformService::getTitleId, "GetTitleId", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, shared_ptr<const Reflection::ValueTable>()> func_getVersionIdInfo(&PlatformService::getVersionIdInfo, "GetVersionIdInfo", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, shared_ptr<const Reflection::ValueTable>()> func_getPlatformUserInfo(&PlatformService::getPlatformUserInfo, "GetPlatformUserInfo", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, void(std::string, std::string, std::string, XboxKeyBoardType)> func_showKeyBoard(&PlatformService::showKeyBoard, "ShowKeyboard", "title", "description", "defaultText", "keyboardType", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(int)> event_GameJoined(&PlatformService::gameJoinedSignal, "GameJoined", "joinResult", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(int)> event_ViewChanged(&PlatformService::viewChanged, "ViewChanged", "viewType", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void()> event_UserAuthComplete(&PlatformService::userAuthCompleteSignal, "UserAuthComplete", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(int)> event_UserAccountChanged(&PlatformService::userAccountChangeSignal, "UserAccountChanged", "accountChangeStatus", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(int)> event_robuxAmountChanged(&PlatformService::robuxAmountChangedSignal, "RobuxAmountChanged", "robuxChangeStatus", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(std::string)> event_NetworkStatusChanged(&PlatformService::networkStatusChangedSignal, "NetworkStatusChanged", "statusJSON");
static Reflection::EventDesc<PlatformService, void(std::string)> event_KeyboardClosed(&PlatformService::keyboardClosedSignal, "KeyboardClosed", "text");

static Reflection::EventDesc<PlatformService, void(std::string)> event_GainedActiveUser(&PlatformService::gainedActiveUser, "GainedActiveUser", "userDisplayName", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(std::string)> event_LostActiveUser(&PlatformService::lostActiveUser, "LostActiveUser", "userDisplayName", Security::RobloxScript);

static Reflection::EventDesc<PlatformService, void(std::string)> event_LostUserGamepad(&PlatformService::lostUserGamepad, "LostUserGamepad", "userDisplayName", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(std::string)> event_GainedUserGamepad(&PlatformService::gainedUserGamepad, "GainedUserGamepad", "userDisplayName", Security::RobloxScript);

static Reflection::EventDesc<PlatformService, void()> event_suspend(&PlatformService::suspendSignal, "Suspended", Security::RobloxScript);

static Reflection::BoundFuncDesc<PlatformService, void(int,bool)> func_voiceChatSetMuteState (&PlatformService::voiceChatSetMuteState, "VoiceChatSetMuteState", "userID", "muted", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformService, VoiceChatState(int)> func_voiceChatGetState(&PlatformService::voiceChatGetState, "VoiceChatGetState", "userId", Security::RobloxScript );
static Reflection::EventDesc<PlatformService, void(int)> event_voiceChatUserTalkingStart(&PlatformService::voiceChatUserTalkingStartSignal , "VoiceChatUserTalkingStart", "userId", Security::RobloxScript);
static Reflection::EventDesc<PlatformService, void(int)> event_voiceChatUserTalkingEnd(&PlatformService::voiceChatUserTalkingEndSignal, "VoiceChatUserTalkingEndSignal", "userId", Security::RobloxScript);

Reflection::BoundProp<float> PlatformService::desc_Brightness("Brightness", category_Appearance, &PlatformService::brightness, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
Reflection::BoundProp<float> PlatformService::desc_Contrast("Contrast", category_Appearance, &PlatformService::contrast, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
Reflection::BoundProp<float> PlatformService::desc_GrayscaleLevel("GrayscaleLevel", category_Appearance, &PlatformService::grayscaleLevel, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
Reflection::BoundProp<float> PlatformService::desc_BlurIntensity("BlurIntensity", category_Appearance, &PlatformService::blurIntensity, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
Reflection::BoundProp<Color3> PlatformService::desc_TintColor("TintColor", category_Appearance, &PlatformService::tintColor, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
Reflection::PropDescriptor<PlatformService, int> PlatformService::prop_DatamodelType("DatamodelType", category_Data, &PlatformService::getPlatformDatamodelType, NULL, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);

namespace Reflection 
{
    template<>
    EnumDesc<XboxKeyBoardType>::EnumDesc() : EnumDescriptor("XboxKeyBoardType")
    {
        addPair(xbKeyBoard_Default, "Default");
        addPair(xbKeyBoard_EmailSmtpAddress, "EmailSmtpAddress");
        addPair(xbKeyBoard_Number, "Number");
        addPair(xbKeyBoard_Password, "Password");
        addPair(xbKeyBoard_Search, "Search");
        addPair(xbKeyBoard_TelephoneNumber, "TelephoneNumber");
        addPair(xbKeyBoard_Url, "Url");
    }

    template<>
    EnumDesc<VoiceChatState>::EnumDesc() : EnumDescriptor("VoiceChatState")
    {
        addPair(voiceChatState_Available, "Available");
        addPair(voiceChatState_Muted, "Muted");
        addPair(voiceChatState_NotInChat, "NotInChat");
        addPair(voiceChatState_UnknownUser, "UnknownUser");
        
    }

    template<>
    XboxKeyBoardType& Variant::convert<XboxKeyBoardType>(void)
    {
        return genericConvert<XboxKeyBoardType>();
    }

    template<>
    VoiceChatState& Variant::convert<VoiceChatState>(void)
    {
        return genericConvert<VoiceChatState>();
}
}

template<>
bool StringConverter<XboxKeyBoardType>::convertToValue(const std::string& text, XboxKeyBoardType& value)
{
    return Reflection::EnumDesc<XboxKeyBoardType>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool StringConverter<VoiceChatState>::convertToValue(const std::string& text, VoiceChatState& value)
{
    return Reflection::EnumDesc<VoiceChatState>::singleton().convertToValue(text.c_str(),value);
}

PlatformService::PlatformService()
    : brightness(0)
    , contrast(0)
    , grayscaleLevel(0)
    , tintColor(Color3(1,1,1))
    , blurIntensity(0)
{
	setName(sPlatformService);
    baFlag = 0;
    bnccFlag = 0;
    bsgFlag = 0;
	bffFlag = ppuiFlag = ppruiFlag = bpspFlag = bgpmFlag = 0;
	bgiiFlag = bgciFlag = bgigpFlag = 0; // xbox catalog and inventory
	alFlag = 0; // account linking related flags
    platform = 0;
	
}

template< class RF, class EF >
struct closure
{
    RF resume;
    EF error;
};

template< class RF, class EF >
closure<RF,EF>* makeClosure( RF resume, EF error )
{
    closure<RF,EF>* ptr = new closure<RF,EF>;
    ptr->resume = resume;
    ptr->error  = error;
    return ptr;
}

template< class Lambda >
void endTask( PlatformService* ps, const Lambda& t )
{
    DataModel::get(ps)->submitTask( t, DataModelJob::Write );
}
				 
void PlatformService::beginAuthorization(InputObject::UserInputType gamepadId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread worker( 
		[=]() -> void
	{
		if( InterlockedCompareExchange( &baFlag, 1, 0 ) == 0 )
		{
			
			int r = platform->performAuthorization(gamepadId, false);
			endTask( this, [=](...) { cl->resume(r); delete cl; } );

			InterlockedExchange( &baFlag, 0 );
			return;
		}
		endTask( this, [cl](...) { cl->error("Auth is already in progress"); delete cl; }  );
	} );

	worker.detach();
}
void PlatformService::beginAuthUnlinkCheck(InputObject::UserInputType gamepadId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    RBXASSERT(platform);
    auto cl = makeClosure(resumeFunction, errorFunction);
    boost::thread worker( 
        [=]() -> void
        {
            if( InterlockedCompareExchange( &baFlag, 1, 0 ) == 0 )
            {
                int r = platform->performAuthorization(gamepadId, true);
				endTask( this, [cl,r](...) { cl->resume(r); delete cl; } );
                InterlockedExchange( &baFlag, 0 );
                return;
            }
            endTask( this, [cl](...) { cl->error("Auth is already in progress"); delete cl; }  );
        } );

    worker.detach();
}

void PlatformService::beginAccountLink(std::string accountName, std::string password, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread worker( 
		[=]() -> void
	{
		if( InterlockedCompareExchange( &alFlag, 1, 0 ) == 0 )
		{
			std::string resp;
			int r = platform->performAccountLink(accountName, password, &resp);
			endTask( this, [=](...) { cl->resume(r); delete cl; } );
			InterlockedExchange( &alFlag, 0 );
			return;
		}
		endTask( this, [cl](...) { cl->error("AccountLink is already in progress"); delete cl; }  );
	} );

	worker.detach();
}
void PlatformService::beginUnlinkAccount(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread worker( 
		[=]() -> void
	{
		if( InterlockedCompareExchange( &alFlag, 1, 0 ) == 0 )
		{
			std::string resp;
			int r = platform->performUnlinkAccount(&resp);
			endTask( this, [=](...) { cl->resume(r); delete cl; } );
			InterlockedExchange( &alFlag, 0 );
			return;
		}
		endTask( this, [cl](...) { cl->error("AccountLink is already in progress"); delete cl; }  );
	} );

	worker.detach();
}

void PlatformService::beginSetRobloxCredentials(std::string accountName, std::string password, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread worker( 
		[=]() -> void
	{
		if( InterlockedCompareExchange( &alFlag, 1, 0 ) == 0 )
		{
			std::string resp;
			int r = platform->performSetRobloxCredentials(accountName, password, &resp);
			endTask( this, [=](...) { cl->resume(r); delete cl; } );
			InterlockedExchange( &alFlag, 0 );
			return;
		}
		endTask( this, [cl](...) { cl->error("AccountLink is already in progress"); delete cl; }  );
	} );

	worker.detach();
}
void PlatformService::beginHasLinkedAccount(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread worker( 
		[=]() -> void
	{
		int r = platform->performHasLinkedAccount();
		endTask( this, [=](...) { cl->resume(r); delete cl; } );
		return;
	} );

	worker.detach();
}
void PlatformService::beginHasRobloxCredentials(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread worker( 
		[=]() -> void
	{
		int r = platform->performHasRobloxCredentials();
		endTask( this, [=](...) { cl->resume(r); delete cl; } );
		return;
	} );

	worker.detach();
}

void PlatformService::beginStartGame3(int mode, int id, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    RBXASSERT(platform);
    GameJoinType joinType = (GameJoinType)mode;
    if( joinType < 0 || joinType > GameJoin__MaxCnt )
    {
        errorFunction("PlatformService::beginStartGame2(): bad 'mode' parameter");
        return;
    }

    auto cl = makeClosure(resumeFunction, errorFunction);
    boost::thread worker(
        [=]() -> void
        {
            if( InterlockedCompareExchange( &bsgFlag, 1, 0) == 0 )
            {
                int r = (int)platform->startGame3(joinType, id);
                endTask( this, [cl,r](...) { cl->resume(r); delete cl; } );
                InterlockedExchange( &bsgFlag, 0 );
                return;
            }
            endTask( this, [cl](...) { cl->error("Game start is already in progress"); delete cl; } );
        }
    );
    worker.detach();
}

void PlatformService::requestGameShutdown()
{
    RBXASSERT(platform);
    boost::thread( [this]() { platform->requestGameShutdown(false); } ).detach();
}

void PlatformService::beginFetchFriends(InputObject::UserInputType gamepadId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    RBXASSERT(platform);
    auto cl = makeClosure( resumeFunction, errorFunction );
    boost::thread(
        [=]() -> void
        {
            if( InterlockedCompareExchange( &bffFlag, 1, 0 ) == 0 )
            {
                std::string result;
                int r = platform->fetchFriends(gamepadId, &result);
                if( r>=0 )
                    endTask( this, [=](...) { cl->resume(result); delete cl; } );
                else
                    endTask( this, [=](...) { cl->error( "fetch friends failed"); delete cl; } );

                InterlockedExchange( &bffFlag, 0 );
                return;
            }
            endTask( this, [=](...) { cl->error("friend fetch is already in progress"); delete cl; } );
        }
    ).detach();
}
void PlatformService::popupAccountPickerUI(InputObject::UserInputType gamepadId)
{
	RBXASSERT(platform);
	platform->popupAccountPickerUI(gamepadId);
}
void PlatformService::popupGameInviteUI()
{
	RBXASSERT(platform);
	platform->popupGameInviteUI();
}
void PlatformService::popupHelpUI()
{
	RBXASSERT(platform);
	platform->popupHelpUI();
}
void PlatformService::launchPlatformUri(const std::string baseUri)
{
	RBXASSERT(platform);
	platform->launchPlatformUri(baseUri);
}
void PlatformService::popupPartyUI(InputObject::UserInputType gamepadId, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    RBXASSERT(platform);
    auto cl = makeClosure(resumeFunction, errorFunction);
    boost::thread(
        [=]() -> void
        {
            if( InterlockedCompareExchange( &ppuiFlag, 1, 0 ) == 0 )
            {
                dprintf("PlatformService: displaying party UI...\n");

                int r = platform->popupPartyUI(gamepadId);
                if( r==0 )
                    endTask( this, [=](...) { cl->resume(); delete cl; } );
                else
                    endTask( this, [=](...) { cl->error( "party UI failed" ); delete cl; } );

                InterlockedExchange( &ppuiFlag, 0 );
                return;
            }
            endTask( this, [=](...) { cl->error("party popup is already being invoked"); delete cl; } );
        }
    ).detach();
}

void PlatformService::showKeyBoard(std::string title, std::string description, std::string defaultText, XboxKeyBoardType keyboardType)
{
    RBXASSERT(platform);

    if (DataModel* dm = DataModel::get(this))
        platform->showKeyBoard(title, description, defaultText, keyboardType, dm);
}

void PlatformService::popupProfileUI(InputObject::UserInputType gamepadId, std::string uid, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
		{
			if (InterlockedCompareExchange(&ppruiFlag, 1, 0) == 0)
			{
				dprintf("PlatformService: displaying profile card UI..\n");

				int r = platform->popupProfileUI(gamepadId, uid);
				if (r >= 0)
				{
					endTask(this, [=](...) { cl->resume(); delete cl; } );
				}
				else
				{
					endTask(this, [=](...) { cl->error("profile card UI failed"); delete cl; } );
				}

				InterlockedExchange( &ppruiFlag, 0 );
				return;
			}
			endTask(this, [=](...) {cl->error("popupProfileUI is already being invoked"); delete cl; } );
		}
	).detach();
}

void PlatformService::beginGetPartyMembers(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
	{
		if( InterlockedCompareExchange( &bgpmFlag, 1, 0 ) == 0 )
		{
			dprintf("PlatformService: beginGetPartyMembers...\n");
			shared_ptr<Reflection::ValueArray> values(rbx::make_shared<Reflection::ValueArray>()) ;
			int r = platform->getPlatformPartyMembers(values);
			if( r==0 )
				endTask( this, [=](...) { cl->resume(values); delete cl; } );
			else
				endTask( this, [=](...) { cl->error( "beginGetPartyMembers failed" ); delete cl; } );

			InterlockedExchange( &bgpmFlag, 0 );
			return;
		}
		endTask( this, [=](...) { cl->error("beginGetPartyMembers already being invoked"); delete cl; } );
	}
	).detach();
}

void PlatformService::beginGetInGamePlayers(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
	{
		if( InterlockedCompareExchange( &bgigpFlag, 1, 0 ) == 0 )
		{
			dprintf("PlatformService: beginGetInGamePlayers...\n");
			shared_ptr<Reflection::ValueArray> values(rbx::make_shared<Reflection::ValueArray>()) ;
			int r = platform->getInGamePlayers(values);
			if( r==0 )
				endTask( this, [=](...) { cl->resume(values); delete cl; } );
			else
				endTask( this, [=](...) { cl->error( "beginGetInGamePlayers failed" ); delete cl; } );

			InterlockedExchange( &bgigpFlag, 0 );
			return;
		}
		endTask( this, [=](...) { cl->error("beginGetInGamePlayers already being invoked"); delete cl; } );
	}
	).detach();
}

void PlatformService::beginGetPMPCreatorId(boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
	{
		int id = platform->getPMPCreatorId();
		if(id >= 0 )
			endTask( this, [=](...) { cl->resume(id); delete cl;} );
		else
			endTask( this, [=](...) { cl->error("GetPMPCreatorId Timed Out"); delete cl;} );
	}
	).detach();
}

int PlatformService::getTitleId()
{
	RBXASSERT(platform);
	return platform->getTitleId();
}

shared_ptr<const Reflection::ValueTable> PlatformService::getVersionIdInfo()
{
	RBXASSERT(platform);
	return platform->getVersionIdInfo();
}

shared_ptr<const Reflection::ValueTable> PlatformService::getPlatformUserInfo()
{
	return platform->getPlatformUserInfo();
}

void PlatformService::beginGetInventoryInfo(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
	{
		if( InterlockedCompareExchange( &bgiiFlag, 1, 0 ) == 0 )
		{
			dprintf("PlatformService: beginGetInventoryInfo...\n");
			shared_ptr<Reflection::ValueArray> values(rbx::make_shared<Reflection::ValueArray>()) ;
			int r = platform->fetchInventoryInfo(values);
			if( r==0 )
				endTask( this, [=](...) { cl->resume(values); delete cl; } );
			else
				endTask( this, [=](...) { cl->error( "beginGetInventoryInfo failed" ); delete cl; } );

			InterlockedExchange( &bgiiFlag, 0 );
			return;
		}
		endTask( this, [=](...) { cl->error("beginGetInventoryInfo already being invoked"); delete cl; } );
	}
	).detach();
}
void PlatformService::beginGetCatalogInfo(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
	{
		if( InterlockedCompareExchange( &bgciFlag, 1, 0 ) == 0 )
		{
			dprintf("PlatformService: fetchingCatalogInfo...\n");
			shared_ptr<Reflection::ValueArray> values(rbx::make_shared<Reflection::ValueArray>()) ;
			int r = platform->fetchCatalogInfo(values);
			if( r==0 )
				endTask( this, [=](...) { cl->resume(values); delete cl; } );
			else
				endTask( this, [=](...) { cl->error( "fetchCatalogInfo failed" ); delete cl; } );

			InterlockedExchange( &bgciFlag, 0 );
			return;
		}
		endTask( this, [=](...) { cl->error("fetchCatalogInfo already being invoked"); delete cl; } );
	}
	).detach();
}
void PlatformService::beginPlatformStorePurchase(const std::string productId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(platform);
	auto cl = makeClosure(resumeFunction, errorFunction);
	boost::thread(
		[=]() -> void
	{
		if( InterlockedCompareExchange( &bpspFlag, 1, 0 ) == 0 )
		{
			dprintf("PlatformService: platformStorePurchase...\n");
			PlatformPurchaseResult r = platform->requestPurchase(productId);
			endTask( this, [=](...) { cl->resume((int)r); delete cl; } );
			InterlockedExchange( &bpspFlag, 0 );
			return;
		}
		endTask( this, [=](...) { cl->error("platformStorePurchase already being invoked"); delete cl; } );
	}
	).detach();
}

void PlatformService::beginAwardAchievement(std::string eventName, boost::function<void(int)> resumeFn, boost::function<void(std::string)> errorFunction)
{
    RBXASSERT(platform);
	auto cl = makeClosure(resumeFn, errorFunction);
	boost::thread(
		[=]() -> void
		{
			dprintf("PlatformService: awarding achievement..\n");
            AwardResult r = platform->awardAchievement(eventName);
            endTask(this, [=](...) { cl->resume((int)r); delete cl; } );
		}
	).detach();
}

void PlatformService::beginHeroStat(std::string eventName, double value, boost::function<void(int)> resumeFn, boost::function<void(std::string)> errorFunction)
{
    RBXASSERT(platform);
    auto cl = makeClosure(resumeFn, errorFunction);
    boost::thread(
        [=]() -> void
        {
            dprintf("PlatformService: setting hero stat..\n");
            AwardResult r = Award_Fail;
            double val = value;
            if (value != DBL_MIN)
                r = platform->setHeroStat(eventName, &val);
            else
                r = platform->setHeroStat(eventName, NULL);

            endTask(this, [=](...) { cl->resume((int)r); delete cl; } );
        }
    ).detach();
}


void PlatformService::changeScreenResolution(double px, double py)
{
    RBXASSERT(platform);
    dprintf("NOTE: PlatformService:ChangeScreenResolution() is deprecated.\n");
    platform->setScreenResolution(px, py);
}

void PlatformService::setPlatform(IPlatformAPI* iface, PlatformDatamodelType newDatamodelType)
{
    platform = iface;
	datamodelType = newDatamodelType;
}

PlatformDatamodelType PlatformService::getPlatformDatamodelType() const
{
	return datamodelType;
}

void PlatformService::voiceChatSetMuteState(int userId, bool mute)
{
    RBXASSERT(platform);
    platform->voiceChatSetMuteState(userId, mute);
}

VoiceChatState PlatformService::voiceChatGetState(int userId)
{
    RBXASSERT(platform);
    return (VoiceChatState)platform->voiceChatGetState(userId);
}

} // end RBX
