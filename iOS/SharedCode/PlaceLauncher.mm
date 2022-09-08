//
//  PlaceLauncher.m
//  RobloxMobile
//
//  Created by David York on 9/25/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>

#import "PlaceLauncher.h"
#import "AppDelegate.h"
#ifdef STANDALONE_PURCHASING
#import "StandaloneAppStore.h"
#endif
#import "StoreManager.h"
#import "Reachability.h"

#import "GameViewController.h"
#import "MainViewController.h"
#import "ControlView.h"
#import "RobloxWebUtility.h"
#import "UIScreen+PortraitBounds.h"
#import "RobloxNotifications.h"

#import "RobloxInfo.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxAlert.h"
#import "ObjectiveCUtilities.h"
#import "MarshallerInterface.h"
#import "RobloxMemoryManager.h"
#import "UserInfo.h"
#import "SessionReporter.h"
#import "NotificationHelper.h"
#import "CrashReporter.h"


#include "RobloxView.h"
#include "FunctionMarshaller.h"
#include "Teleporter.h"
#include "RbxFormat.h"
#include "script/ScriptContext.h"
#include "util/Http.h"
#include "util/Statistics.h"
#include "util/MemoryStats.h"
#include "util/RobloxGoogleAnalytics.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/TeleportService.h"
#include "v8datamodel/LoginService.h"
#include "V8DataModel/ContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/TextureContentProvider.h"
#include "v8datamodel/MarketplaceService.h"
#include "v8datamodel/AdService.h"
#include "v8datamodel/NotificationService.h"
#include "V8DataModel/GameBasicSettings.h"
#include "GfxBase/ViewBase.h"
#include "util/Http.h"
#include "rbx/Profiler.h"

#include "iOSSettingsService.h"

#include <rapidjson/document.h>

const std::string kStartGameStatusURL = "%sGame/PlaceLauncher.ashx?request=CheckGameJobStatus&jobId=%s";

FASTFLAG(GoogleAnalyticsTrackingEnabled)
DYNAMIC_LOGGROUP(GoogleAnalyticsTracking)
LOGGROUP(Network)
LOGGROUP(PlayerShutdownLuaTimeoutSeconds)



@implementation RBXGameLaunchParams

+(RBXGameLaunchParams*) InitDefaults
{
    RBXGameLaunchParams* params = [[RBXGameLaunchParams alloc] init];
    params.joinRequestType = JOIN_GAME_REQUEST_PLACEID;
    params.joinRequestString = @"RequestGame";
    params.targetId = -1;
    params.accessCode = @"";
    params.gameInstanceId = @"";
    
    return params;
}
+(RBXGameLaunchParams*) InitParamsForFollowUser:(int)userID
{
    RBXGameLaunchParams* params = [RBXGameLaunchParams InitDefaults];
    params.joinRequestType = JOIN_GAME_REQUEST_USERID;
    params.joinRequestString = @"RequestFollowUser";
    params.targetId = userID;
    
    return params;
}
+(RBXGameLaunchParams*) InitParamsForJoinPlace:(int)placeID
{
    RBXGameLaunchParams* params = [RBXGameLaunchParams InitDefaults];
    params.targetId = placeID;
    
    return params;
}
+(RBXGameLaunchParams*) InitParamsForJoinPrivateServer:(int)placeID withAccessCode:(NSString*)vpsAccessCode
{
    RBXGameLaunchParams* params = [RBXGameLaunchParams InitDefaults];
    params.joinRequestType = JOIN_GAME_REQUEST_PRIVATE_SERVER;
    params.joinRequestString = @"RequestPrivateGame";
    params.targetId = placeID;
    params.accessCode = vpsAccessCode;
    
    return params;
}
+(RBXGameLaunchParams*) InitParamsForJoinGameInstance:(int)placeID withInstanceID:(NSString*)instanceID
{
    RBXGameLaunchParams* params = [RBXGameLaunchParams InitDefaults];
    params.joinRequestType = JOIN_GAME_REQUEST_GAME_INSTANCE;
    params.joinRequestString = @"RequestGameJob";
    params.targetId = placeID;
    params.gameInstanceId = instanceID;
    
    return params;
}

-(std::string) getStringJoinURL
{
    const std::string kStartGameURL = "%sGame/PlaceLauncher.ashx?%s&isPartyLeader=false&gender=&isTeleport=false";
    //base - Game / PlaceLauncher.ashx ? (request=%s and other parameters) & isPartyLeader=false & gender= & isTeleport=false
    const char* base = [[RobloxInfo getBaseUrl] UTF8String];
    std::string url, formattedParameters;
    
    switch (self.joinRequestType)
    {
        case JOIN_GAME_REQUEST_USERID:
        {
            formattedParameters = RBX::format("request=%s&userId=%d", self.joinRequestString.UTF8String, self.targetId);
        } break;
            
        case JOIN_GAME_REQUEST_GAME_INSTANCE:
        {
            //note the inconsistency
            formattedParameters = RBX::format("request=%s&placeId=%d&gameId=%s", self.joinRequestString.UTF8String, self.targetId, self.gameInstanceId.UTF8String);
        } break;
            
        case JOIN_GAME_REQUEST_PRIVATE_SERVER:
        {
            formattedParameters = RBX::format("request=%s&placeId=%d&accessCode=%s", self.joinRequestString.UTF8String, self.targetId, self.accessCode.UTF8String);
        } break;
            
        default: //JOIN_GAME_REQUEST_PLACEID
        {
            formattedParameters = RBX::format("request=%s&placeId=%d", self.joinRequestString.UTF8String, self.targetId);
        } break;
    }
    
    url = RBX::format(kStartGameURL.c_str(), base, formattedParameters.c_str());
    return url;
}
@end



static bool bPlaceViewPresented = false;

static std::string ReadStringValue(std::string &data, std::string name)
{
	std::string result;
	int pos = data.find(name);
	if (pos != -1)
	{
		int start = pos + name.length() + 3;
		int end = data.find(",", start);
		result = data.substr(start, end - start - 1);
	}
    
	pos = result.find("\\/"); // find first space
	while (pos != std::string::npos)
	{
		result.replace(pos, 2, "/");
		pos = result.find("\\/", pos + 1);
	}
	return result;
}

// binds ogre views to our iOS views (so we can control segues and such)
static void bindOgreViews(RobloxView* rbxView, GameViewController* gameController)
{
    if (MainViewController* mainViewController = [MainViewController sharedInstance])
    {
        UIView* mUIView = gameController.view;
        UIWindow* mUIWindow = mUIView.window;
        
        [mainViewController setOgreView:mUIView];
            
        mUIView.multipleTouchEnabled = YES;
        
        ControlView* controlView = [[ControlView alloc] init:[[UIScreen mainScreen] portraitBounds] withGame:rbxView->getGame()];
        [gameController addControlView:controlView];
            
        [mainViewController setOgreWindow:mUIWindow];
    }
}

// if we have all of our view controls set up properly, this function will present
// our current game view over our current view
static void presentGameView()
{
    // make sure we are in main thread to show controller
    dispatch_async(dispatch_get_main_queue(), ^{
        MainViewController* mainController = [MainViewController sharedInstance];
        if(!mainController)
            return;
        
        UIViewController* ogreViewController = [mainController getOgreViewController];
        if(!ogreViewController)
            return;
        
        UIViewController* lastNonGameController = [mainController getLastNonGameController];
        if(!lastNonGameController)
            return;
    
        if ([lastNonGameController respondsToSelector:@selector(presentedViewController)]) {
            if(lastNonGameController.presentedViewController != ogreViewController) {
                   // present the game view on top of whatever non game controller we are using
                   // this way, when we leave the game we will go back to what we were doing
                   [lastNonGameController presentViewController:ogreViewController animated:NO completion:^{
                       if(lastNonGameController && [lastNonGameController respondsToSelector:@selector(handleStartGameSuccess)])
                           [lastNonGameController performSelector:@selector(handleStartGameSuccess)];
                       
                       bPlaceViewPresented = true;
                   }];
            }
        }
    });
}

static void initControlViewHelper(RobloxView* rbxView, BOOL presentGameAutomatically, GameViewController* gameController)
{
    bindOgreViews(rbxView, gameController);
    
    if(presentGameAutomatically)
        presentGameView();
    
    dispatch_async(dispatch_get_main_queue(),^{
        PlaceLauncher* placeLauncher = [PlaceLauncher sharedInstance];
        [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_BOUND_VIEWS object:placeLauncher userInfo:nil];
    });
}

static void initControlView(RobloxView* rbxView, BOOL presentGameAutomatically, GameViewController* gameController, RBX::FunctionMarshaller* marshaller)
{
    // This needs to be execute, because this task is holding the write lock needed
    // to complete the work in initControlViewHelper
    marshaller->Execute(boost::bind(&initControlViewHelper, rbxView, presentGameAutomatically, gameController), NULL);
}


static void finishTeleportHelper(RobloxView* rbxView, shared_ptr<RBX::Game> game)
{
    if(MainViewController* mainViewController = [MainViewController sharedInstance])
    {
        GameViewController* gameController = [mainViewController getOgreViewController];
        
        for(ControlView* control in gameController.view.subviews)
        {
            [control setGame:game];
            break;
        }
    }
}

static void finishTeleport(RobloxView* rbxView, shared_ptr<RBX::Game> game, RBX::FunctionMarshaller* marshaller)
{
    // This needs to be execute, because this task is holding the write lock needed
    // to complete the work in finishTeleportImpl
    marshaller->Execute(boost::bind(&finishTeleportHelper, rbxView, game), NULL);
}

static RBX::ProtectedString fetchAndValidateScript(RBX::DataModel* dataModel, const std::string& urlScript)
{
    RBX::Security::Impersonator impersonate(RBX::Security::COM);
	
	std::ostringstream data;
	if (RBX::ContentProvider::isUrl(urlScript))
	{
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
		std::auto_ptr<std::istream> stream(RBX::ServiceProvider::create<RBX::ContentProvider>(dataModel)->getContent(RBX::ContentId(urlScript.c_str())));
		boost::iostreams::copy(*stream, data);
	}
	else
		return RBX::ProtectedString();
    
    RBX::ProtectedString verifiedSource;
    
    try
    {
        verifiedSource = RBX::ProtectedString::fromTrustedSource(data.str());
        RBX::ContentProvider::verifyScriptSignature(verifiedSource, true);
    }
	catch(std::bad_alloc& e)
	{
		throw;
	}
	catch(RBX::base_exception& e)
	{
		return RBX::ProtectedString();
	}
    
    return verifiedSource;
}

static void executeUrlJoinScript(boost::shared_ptr<RBX::Game> game, const std::string& urlScript)
{
    RBXASSERT(urlScript.find("join.ashx"));
    
    boost::shared_ptr<RBX::DataModel> dataModel = game->getDataModel();

    RBX::ProtectedString verifiedSource = fetchAndValidateScript(dataModel.get(), urlScript);
    if (verifiedSource.empty())
        return;
    
    RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
	
	if (dataModel->isClosed())
		return;
    
    // new join script
    std::string dataString = verifiedSource.getSource();
    int firstNewLineIndex = dataString.find("\r\n");
    if (dataString[firstNewLineIndex+2] == '{')
    {
        game->configurePlayer(RBX::Security::COM, dataString.substr(firstNewLineIndex+2));
        return;
    }
    
    // old join script
	RBX::ScriptContext* context = dataModel->create<RBX::ScriptContext>();
	context->executeInNewThread(RBX::Security::COM, verifiedSource, "Start Script");
}

static void joinLocalGame(int portID, const std::string& ipAddress, shared_ptr<RBX::Game> game, int userId)
{
    try
	{
        const char* base = [[RobloxInfo getBaseUrl] UTF8String];
        std::string scriptString = RBX::format("%sGame/Join.ashx?userID=%i&serverPort=%i&server=%s",
                                               base, userId, portID, ipAddress.c_str());
        
        executeUrlJoinScript(game, scriptString);
    }
	catch (const RBX::base_exception& e)
	{
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't join local place (%s,%i), because %s\n", ipAddress.c_str(), portID, e.what());
        [[PlaceLauncher sharedInstance] handleStartGameFailure];
	}
}

static void joinGameWithJoinScript(const std::string& joinScript, shared_ptr<RBX::Game> game)
{
    try
    {
        executeUrlJoinScript(game, joinScript);
    }
    catch (const RBX::base_exception& e)
	{
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't join game with joinScript because %s\n", e.what());
        [[PlaceLauncher sharedInstance] handleStartGameFailure];
    }
}

static void joinGameWithParameters(shared_ptr<RBX::Game> game, RBXGameLaunchParams* params)
{
    try
	{        
		int retrys = 5;
        
        
        // const char* base = [[RobloxInfo getBaseUrl] UTF8String];
        
        NSDictionary *dictionary;
        dictionary = [NSDictionary dictionaryWithObjectsAndKeys: [RobloxInfo getUserAgentString], @"UserAgent", nil];
        [[NSUserDefaults standardUserDefaults] registerDefaults:dictionary];
        
        std::string response;
        bool found = false;
        int status = -1;
        
        std::string url = [params getStringJoinURL];
        std::string jobId;
        while (retrys >= 0)
        {
            bool retryUsed = true;
            response = "";
            RBX::Http(url).get(response);
            
            rapidjson::Document doc;
            doc.Parse<rapidjson::kParseDefaultFlags>(response.c_str());
            
            RBXASSERT(doc.HasMember("status"));
            status = doc["status"].GetInt();
            
            if (2 == status)
            {
                // not most efficient way to check status,
                // but we have web calls so strstr is not a big deal
                found = true;
                break;
            }
            else if (0 == status || 1 == status)
            {
                // 0 or 1 is not an error - it is a sign that we should wait
                retryUsed = false;
                RBXASSERT(doc.HasMember("jobId"));
                jobId = doc["jobId"].GetString();
            }
            
            int sleepTime = 250*1000;

            if (retryUsed)
            {
                --retrys;
                sleepTime = 999*1000;
            }
            
            ::usleep(sleepTime);
        }
        
        bool bAppActive = [[PlaceLauncher sharedInstance] appActive];
        
        if (found && bAppActive)
        {

            NSString *label = [NSString stringWithFormat:@"%s - %d", params.joinRequestString.UTF8String, params.targetId];
            [RobloxGoogleAnalytics setEventTracking:@"PlaySuccess" withAction:@"Success" withLabel:label withValue:1];
            [[CrashReporter sharedInstance] logIntKeyValue:label withValue: params.targetId];
            
            executeUrlJoinScript(game, ReadStringValue(response, "joinScriptUrl"));
            
            [[PlaceLauncher sharedInstance] setLastPlaceId:params.targetId];
            
            [[SessionReporter sharedInstance] reportSessionFor:ENTER_GAME PlaceId:params.targetId];
            [RobloxGoogleAnalytics setPageViewTracking:@"Visit/Success/Join"];
            
            dispatch_async(dispatch_get_main_queue(), ^{
                [[RobloxMemoryManager sharedInstance] startMaxMemoryTracker];
            });
        }
        else
        {
            if (!bAppActive)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Cannot connect to place %d.  App is not active.", params.targetId);
            if (params.joinRequestType == JOIN_GAME_REQUEST_PLACEID)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Cannot connect to place %d, return from GetPlaceLauncher.ashx = %s", params.targetId, response.c_str());
            else if (params.joinRequestType == JOIN_GAME_REQUEST_GAME_INSTANCE)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Cannot connect to place %d with game instance id - %s", params.targetId, params.gameInstanceId.UTF8String);
            else if (params.joinRequestType == JOIN_GAME_REQUEST_PRIVATE_SERVER)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Cannot connect to private server %d with access code - %s", params.targetId, params.accessCode.UTF8String);
            else
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Cannot follow user %d, return from GetPlaceLauncher.ashx = %s", params.targetId, response.c_str());
            
            /* Place join status results
             Waiting = 0,
             Loading = 1,
             Joining = 2,
             Disabled = 3,
             Error = 4,
             GameEnded = 5,
             GameFull = 6,
             UserLeft = 10
             Restricted = 11
             */
            
            NSString *statusMessage = @"UnknownError";
            if (5 == status)
                statusMessage = @"ConnectionErrorGameEnded";
            else if (6 == status)
                statusMessage = @"ConnectionErrorGameFull";
            else if (3 == status)
                statusMessage = @"ConnectionErrorGameDisabled";
            else if (4 == status)
                statusMessage = @"GeneralGameStartError";
            else if (10 == status)
                statusMessage = @"ConnectionErrorGameUserLeft";
            else if (11 == status)
                statusMessage = @"ConnectionErrorGameRestricted";
            
            NSString *label = [NSString stringWithFormat:@"%s - %d", params.joinRequestString.UTF8String, params.targetId];
            [RobloxGoogleAnalytics setEventTracking:@"PlayErrors" withAction:statusMessage withLabel:label withValue:1];
            
            if (bAppActive)
                [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(statusMessage, nil)];
            
            [[PlaceLauncher sharedInstance] handleStartGameFailure];
        }
    }
	catch (const RBX::base_exception& e)
	{
        if (params.joinRequestType == JOIN_GAME_REQUEST_PLACEID)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't join place %i, because %s\n", params.targetId, e.what());
        else if (params.joinRequestType == JOIN_GAME_REQUEST_GAME_INSTANCE)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't join place %i with game instance %s, because %s\n", params.targetId, params.gameInstanceId.UTF8String, e.what());
        else if (params.joinRequestType == JOIN_GAME_REQUEST_PRIVATE_SERVER)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't join private server of place %i with access code %s, because %s\n", params.targetId, params.accessCode.UTF8String, e.what());
        else
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't follow user %i, because %s\n", params.targetId, e.what());
        
        [[PlaceLauncher sharedInstance] handleStartGameFailure];
	}
}

static void joinGameTeleport(std::string url, std::string ticket, std::string script, NSObject* controller, shared_ptr<RBX::Game> game)
{
    try
    {
        // get authentication URL
        std::string compound = url;

        if (!ticket.empty())
        {
            compound += "?suggest=";
            compound += ticket;
        }

        // issue an authentication request
        std::string result;

        try
        {
            RBX::Http http(compound.c_str());
            http.setAuthDomain(GetBaseURL().c_str());
            http.get(result);
        }
        catch (const RBX::base_exception& e)
        {
        }

        // run game
        executeUrlJoinScript(game, script);
        if ([controller respondsToSelector:@selector(handleStartGameSuccess)])
            [controller performSelector:@selector(handleStartGameSuccess)];
    }
	catch (const RBX::base_exception& e)
	{
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Teleport failed: %s\n", e.what());
        [[PlaceLauncher sharedInstance] handleStartGameFailure];
	}
}

/////////////////////////////////////////////////////////////////////////////////
//
// PlaceLauncher implementation
//
/////////////////////////////////////////////////////////////////////////////////

// private methods
@interface PlaceLauncher()
-(BOOL) startGame:(boost::function0<void>) scriptFunction controller:(UIViewController*) lastNonGameController preloadedGame:(shared_ptr<RBX::Game>) preloadedGame presentGameAutomatically:(BOOL) presentGameAutomatically;

-(shared_ptr<RBX::Game>) setupPreloadedGameWithNonGameController:(UIViewController*) lastNonGameController needsOnline:(BOOL) needsOnline;
-(shared_ptr<RBX::Game>) setupPreloadedGameWithNonGameController:(UIViewController*) lastNonGameController unsecuredGame:(BOOL) unsecuredGame needsOnline:(BOOL) needsOnline;

-(shared_ptr<RBX::Game>) setupGame:(UIViewController*)lastNonGameController needsOnline:(BOOL) needsOnline;
-(shared_ptr<RBX::Game>) setupGame:(UIViewController*)lastNonGameController unsecuredGame:(BOOL) unsecuredGame needsOnline:(BOOL) needsOnline;

-(void) setupDatamodelConnections:(shared_ptr<RBX::DataModel>) dataModel;
@end

@implementation PlaceLauncher

-(id)init
{
    if(self = [super init])
    {
        rbxView = NULL;
        hasReceivedMemoryWarning = NO;
        isCurrentlyPlayingGame = NO;
        lastPlaceId = 0;

        teleporter.reset(new Teleporter(self, RBX::FunctionMarshaller::GetWindow()));
        RBX::TeleportService::SetCallback(teleporter.get());
    }

    return self;
}

-(void)dealloc
{
    RBX::TeleportService::SetCallback(NULL);
    teleporter.reset();
}

+ (id)sharedInstance
{
    static dispatch_once_t placeLauncherPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&placeLauncherPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(BOOL) getIsCurrentlyPlayingGame
{
    return isCurrentlyPlayingGame;
}

-(void) handleStartGameFailure
{
    [[PlaceLauncher sharedInstance] leaveGame];
    
    if (![[MainViewController sharedInstance] getLastNonGameController])
            return;
    
    dispatch_async(dispatch_get_main_queue(), ^
    {
        UIViewController* lastNonGameController = [[MainViewController sharedInstance] getLastNonGameController];
        if (lastNonGameController && [lastNonGameController respondsToSelector:@selector(handleStartGameFailure)])
            [lastNonGameController performSelector:@selector(handleStartGameFailure)];

        isCurrentlyPlayingGame = NO;
    });
}

-(bool)prepareGame:(BOOL) needsOnline
{   
    // Global static variable setup
    NSString* exePath = [[NSBundle mainBundle] resourcePath];
    exePath = [exePath stringByAppendingString:@"/content"];
    
    RBX::ContentProvider::setAssetFolder([exePath UTF8String]);
    RBX::Http::rbxUserAgent = [[RobloxInfo getUserAgentString] UTF8String];
    
    {
        bool useCurl = rand() % 100 < RBX::ClientAppSettings::singleton().GetValueHttpUseCurlPercentageMacClient();
        FASTLOG1(FLog::Network, "Use CURL = %d", useCurl);
        RBX::Http::SetUseCurl(useCurl);
        
        RBX::Http::SetUseStatistics(true);
    }
    
    {
		int lottery = rand() % 100;
		FASTLOG1(DFLog::GoogleAnalyticsTracking, "Google analytics lottery number = %d", lottery);
		// initialize google analytics
		if (FFlag::GoogleAnalyticsTrackingEnabled && (lottery < RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsLoadPlayer()))
		{
			RBX::RobloxGoogleAnalytics::setCanUseAnalytics();
            
			RBX::RobloxGoogleAnalytics::init(RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyIDPlayer(),
                                             RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());
		}
	}
    
    RBX::TeleportService::SetBaseUrl(GetBaseURL().c_str());
    
    if(needsOnline)
    {
        Reachability* reachability = [Reachability reachabilityForInternetConnection];
        NetworkStatus remoteStatus = [reachability currentReachabilityStatus];

        if(remoteStatus == NotReachable)
        {
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: No Network Connection available");
            [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"ConnectionError", nil)];
            return false;
        }
        else if(remoteStatus == ReachableViaWWAN)
        {
            // if user preference is for wifi only, don't allow this
            NSUserDefaults *userDefaults =[NSUserDefaults standardUserDefaults];
            if([[userDefaults stringForKey:@"wifionly_preference"] boolValue])
            {
                [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"WiFiOnlyError", nil)];
                return false;
            }
        }
    }
    
    std::string ios = "i";
    ios += "o";
    ios += "s";
    ios = ios + "," + ios; // slight obfuscation
    
    RBX::DataModel::hash = ios;
    
    {
        RBX::Security::Impersonator impersonate(RBX::Security::RobloxGameScript_);
        RBX::GlobalBasicSettings::singleton()->loadState("");
    }

	RBX::Profiler::onThreadCreate("Main");
    
	RBX::TaskScheduler::singleton().setThreadCount(RBX::TaskSchedulerSettings::singleton().getThreadPoolConfig());

    return true;
}

-(void) setLastPlaceId:(int) lastId;
{
    lastPlaceId = lastId;
}

-(void) checkPlacePartCount
{
    // only show these alerts if the user preference says to
    NSUserDefaults *userDefaults =[NSUserDefaults standardUserDefaults];
    if(![[userDefaults stringForKey:@"warnings_preference"] boolValue])
        return;
    
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        // check num of parts in the level, warn user if over our limits
        iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
        
        int maxIdealParts = iosSettings->GetValueiPad2_MaximumIdealParts();
        if (maxIdealParts > 0)
        {
            RBX::World* world = NULL;
            RBX::Workspace* workspace = NULL;
            
            if (rbxView && rbxView->getDataModel() != NULL)
                workspace = rbxView->getDataModel()->getWorkspace();
            if (workspace != NULL)
                world = workspace->getWorld();
            
            if (world != NULL)
            {
                int numPrimitives = world->getNumPrimitives();
                if (maxIdealParts < numPrimitives)
                {
                    NSString* warningMsg = NSLocalizedString(@"WarnPlaceIsNotIdeal", nil);
                    
                    NSString* detailMsg = [NSString stringWithFormat:NSLocalizedString(@"WarnTooManyParts", nil), numPrimitives, maxIdealParts];
                    NSString* fullMsg = [NSString stringWithFormat:@"%@\n\n%@", warningMsg, detailMsg];
                    NSString* placeId = [NSString stringWithFormat:@"%d", lastPlaceId];
                    [RobloxGoogleAnalytics setEventTracking:@"PlayErrors" withAction:@"TooManyParts" withLabel:placeId withValue:0];
                    [RobloxAlert RobloxAlertWithMessage:fullMsg];
                }
            }
        }
    });
}

-(void)placeDidFinishLoading
{
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_FINISHED_LOADING object:self userInfo:nil];
    [self checkPlacePartCount];
}

-(void) deleteRobloxView:(BOOL) resetCurrentGame
{
    if(resetCurrentGame)
    {
        // remove all shared ptr to games so it can properly be destroyed
        currentGame.reset();
    }
    
    // do actual destruction of the Roblox View, only if it isn't NULL
    if (rbxView)
    {
        // Destroy the view synchronously
        // Take care to zero-out view pointer beforehand to support reentrancy
        RobloxView* view = rbxView;
        rbxView = NULL;
        delete view;
        
        [[RobloxMemoryManager sharedInstance] stopFreeMemoryChecker];
    }
}

-(void) finishGameSetup: (boost::shared_ptr<RBX::Game>)game gameViewController:(GameViewController*) gameController
{ 
    CGSize screenSize = [[UIScreen mainScreen] portraitBounds].size;

    // View dimensions are rotated since we're rendering in landscape
    int viewWidth = screenSize.height;
    int viewHeight = screenSize.width;
    
    RBX::GameBasicSettings::singleton().setFullScreen(true);

    rbxView = RobloxView::create_view(game, (__bridge void*)gameController.view, viewWidth, viewHeight);
    
    rbxView->getDataModel()->submitTask(boost::bind(boostFuncFromSelector(@selector(finishGameSetupWithLock), self)), RBX::DataModelJob::Write);
}

-(void) finishGameSetupWithLock
{
    if(rbxView->getDataModel()->getIsGameLoaded())
    {
        [self placeDidFinishLoading];
    }
    else
    {
        rbxView->getDataModel()->gameLoadedSignal.connect(boostFuncFromSelector(@selector(placeDidFinishLoading),self));
    }
    
    [self setupDatamodelConnections:rbxView->getDataModel()];
}

-(void) setupDatamodelConnections:(shared_ptr<RBX::DataModel>) dataModel
{
    // connect to events to show a web page in game
    if(RBX::GuiService* guiService = dataModel->find<RBX::GuiService>())
        guiService->openUrlWindow.connect( boostFuncFromSelector_1<std::string>(@selector(openUrlWindow:), [[MainViewController sharedInstance] getOgreViewController]) );
    
    if(RBX::AdService* adService = dataModel->find<RBX::AdService>())
        adService->playVideoAdSignal.connect( boostFuncFromSelector(@selector(playVideoAd), [[MainViewController sharedInstance] getOgreViewController]) );

#ifdef STANDALONE_NOTIFICATION
    id notificationHelper = [NotificationHelper sharedInstance];
    if(RBX::NotificationService* notificationService = dataModel->find<RBX::NotificationService>())
    {
        notificationService->scheduleNotificationSignal.connect( boostFuncFromSelector_4<int, int, std::string, int>(@selector(scheduleLocalNotification:), notificationHelper) );
        notificationService->cancelNotificationSignal.connect( boostFuncFromSelector_2<int, int>(@selector(cancelLocalNotification:), notificationHelper) );
        notificationService->cancelAllNotificationSignal.connect( boostFuncFromSelector_1<int>(@selector(cancelAllNotification:), notificationHelper) );
        notificationService->getScheduledNotificationsSignal.connect( boostFuncFromSelector_3<int, boost::function<void(shared_ptr<const RBX::Reflection::ValueArray>)>, boost::function<void(std::string)> >(@selector(getScheduledNotifications:), notificationHelper) );
    }
#endif
    
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [[RobloxMemoryManager sharedInstance] startFreeMemoryChecker];
    });
    
    RBX::Network::Players* players = rbxView->getDataModel()->find<RBX::Network::Players>();
    childConnection = players->onDemandWrite()->childAddedSignal.connect(boostFuncFromSelector_1< shared_ptr<RBX::Instance> >(@selector(childAdded:), self));

	// connect to events to show signup in game
    if( RBX::LoginService* loginService =  RBX::ServiceProvider::create<RBX::LoginService>(dataModel.get()) )
        loginService->promptLoginSignal.connect(boostFuncFromSelector(@selector(handlePromptLoginSignal), [[MainViewController sharedInstance] getOgreViewController]));
    
    if (RBX::MarketplaceService* marketplaceService = RBX::ServiceProvider::create<RBX::MarketplaceService>(dataModel.get()) )
    {
        id storeManager = GetStoreMgr;
        
#ifdef STANDALONE_PURCHASING
        if ([storeManager isKindOfClass:[StandaloneAppStore class]])
        {
            StandaloneAppStore* appStore = (StandaloneAppStore*) storeManager;
            marketplaceService->promptThirdPartyPurchaseRequested.connect(boostFuncFromSelector_2<shared_ptr<RBX::Instance>, std::string>(@selector(promptThirdPartyPurchase:productId:), appStore));
        }
#else
        if([storeManager isKindOfClass:[StoreManager class]])
        {
            StoreManager* appStore = (StoreManager*) storeManager;
            marketplaceService->promptNativePurchaseRequested.connect(boostFuncFromSelector_2<shared_ptr<RBX::Instance>, std::string>(@selector(promptNativePurchase:productId:), appStore));
        }
#endif
    }
    
}

-(void) setLastNonGameController:(UIViewController*)lastNonGameController needsOnline:(BOOL) needsOnline
{
    [[MainViewController sharedInstance] setLastNonGameController:lastNonGameController];
    
    if (lastNonGameController != nil && ![self prepareGame:needsOnline])
        [self handleStartGameFailure];
}

-(void) createGame:(shared_ptr<RBX::Game>)game presentGameAutomatically:(BOOL) presentGameAutomatically
{
    hasReceivedMemoryWarning = NO;
    
    // make sure we have no other views going
    [self deleteRobloxView:NO];
    
    UIViewController* lastController = [[MainViewController sharedInstance] getLastNonGameController];
    if(lastController)
    {
        GameViewController* gameController = [[GameViewController alloc] init];
        gameController.modalPresentationStyle = UIModalPresentationFullScreen;
        [[MainViewController sharedInstance] setOgreViewController:gameController];
        
        [self finishGameSetup:game gameViewController:gameController];
        
        rbxView->getDataModel()->submitTask(boost::bind(initControlView, rbxView, presentGameAutomatically, gameController, RBX::FunctionMarshaller::GetWindow()), RBX::DataModelJob::Write);
    }
}

-(shared_ptr<RBX::Game>) setupGame:(UIViewController*)lastNonGameController needsOnline:(BOOL) needsOnline
{
    return [self setupGame:lastNonGameController unsecuredGame:NO needsOnline:needsOnline];
}

-(shared_ptr<RBX::Game>) setupGame:(UIViewController*)lastNonGameController unsecuredGame:(BOOL) unsecuredGame needsOnline:(BOOL) needsOnline
{
    if(isCurrentlyPlayingGame)
        return shared_ptr<RBX::Game>();
        
    [[RobloxWebUtility sharedInstance] updateAllClientSettingsWithCompletion:nil];
    
    // don't allow screen to dim while playing game
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    
    isCurrentlyPlayingGame = YES;
    
    [self setLastNonGameController:lastNonGameController needsOnline:needsOnline];
    
    currentGame = unsecuredGame ? shared_ptr<RBX::Game>( new RBX::UnsecuredStudioGame(NULL, GetBaseURL().c_str()) ) :
                                  shared_ptr<RBX::Game>( new RBX::SecurePlayerGame(NULL, GetBaseURL().c_str()) );
    
    if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
        currentGame->getDataModel()->create<RBX::ScriptContext>();
    
    return currentGame;
}

-(void) presentGameViewController
{
    presentGameView();
}

-(shared_ptr<RBX::Game>) setupPreloadedGameWithNonGameController:(UIViewController*) lastNonGameController unsecuredGame:(BOOL) unsecuredGame needsOnline:(BOOL) needsOnline
{
    return [self setupGame:lastNonGameController unsecuredGame:unsecuredGame needsOnline:needsOnline];
}
-(shared_ptr<RBX::Game>) setupPreloadedGameWithNonGameController:(UIViewController*) lastNonGameController needsOnline:(BOOL) needsOnline
{
    return [self setupGame:lastNonGameController needsOnline:needsOnline];
}

-(void) injectJoinScript:(NSString*)joinUrlScript
{
    // Start the script thread!
    boost::thread(RBX::thread_wrapper(boost::bind(&joinGameWithJoinScript, [joinUrlScript UTF8String], rbxView->getGame()), "InjectStartScript"));
}

-(BOOL) startGameLocal:(int)portId ipAddress:(NSString*)ipAddress controller:(UIViewController*)lastNonGameController presentGameAutomatically:(BOOL)presentGameAutomatically userId:(int)userId
{
    shared_ptr<RBX::Game> game = [self setupPreloadedGameWithNonGameController:lastNonGameController unsecuredGame:YES needsOnline:YES];
    if( game != shared_ptr<RBX::Game>())
        return [self startGame:boost::bind(&joinLocalGame, portId, [ipAddress UTF8String], game, userId) controller:lastNonGameController preloadedGame:game presentGameAutomatically:presentGameAutomatically];
    
    return false;
}

-(BOOL) startGame:(RBXGameLaunchParams*)params controller:(UIViewController*)lastNonGameController presentGameAutomatically:(BOOL)presentGameAutomatically
{
    shared_ptr<RBX::Game> game = [self setupPreloadedGameWithNonGameController:lastNonGameController needsOnline:YES];
    if( game != shared_ptr<RBX::Game>())
        return [self startGame:boost::bind(&joinGameWithParameters, game, params)
                    controller:lastNonGameController
                 preloadedGame:game
      presentGameAutomatically:presentGameAutomatically];
    
    return false;
}

-(BOOL) startGameWithJoinScript:(NSString*)joinScript controller:(UIViewController*)lastNonGameController presentGameAutomatically:(BOOL)presentGameAutomatically
{
    shared_ptr<RBX::Game> game = [self setupPreloadedGameWithNonGameController:lastNonGameController needsOnline:YES];
    if( game != shared_ptr<RBX::Game>())
        return [self startGame:boost::bind(&joinGameWithJoinScript, [joinScript UTF8String], game) controller:lastNonGameController preloadedGame:game presentGameAutomatically:presentGameAutomatically];
    
    return false;
}

-(BOOL) startGame:(boost::function0<void>)scriptFunction controller:(UIViewController*)lastNonGameController preloadedGame:(shared_ptr<RBX::Game>)preloadedGame presentGameAutomatically:(BOOL)presentGameAutomatically
{    
    // Start the script thread!
    boost::thread(RBX::thread_wrapper(scriptFunction, "GameStartScript"));
    
    [self createGame:preloadedGame presentGameAutomatically:presentGameAutomatically];
    return YES;
}

-(BOOL) appActive
{
    NSString * appState = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxAppState"];
    
    NSLog(@"PlacerLauncher::appActive - %@", appState);
    
    if (appState)
    {
        if ([appState isEqualToString:@"inApp"] || [appState isEqualToString:@"tryForeground"])
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

-(void) leaveGameShutdownWithObject:(NSObject*) object
{
    if( [object isKindOfClass:[NSNumber class]] )
    {
        NSNumber* numberVal = (NSNumber*)object;
        [self leaveGameShutdown:[numberVal boolValue]];
    }
}

-(void)leaveGameShutdown:(BOOL) userRequestedLeave
{
    if (!bPlaceViewPresented)
    {
        [self performSelectorOnMainThread:@selector(leaveGameShutdownWithObject:) withObject:[NSNumber numberWithBool:userRequestedLeave] waitUntilDone:FALSE];
        
        return;
    }
    
    bPlaceViewPresented = false;
    
    MainViewController* mainViewController = [MainViewController sharedInstance];
    
    NSString* didUserRequestLeave = userRequestedLeave ? @"TRUE" : @"FALSE";
    NSDictionary *dict = [[NSDictionary alloc] initWithObjectsAndKeys:didUserRequestLeave,@"UserRequestedLeave", nil];
    
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_START_LEAVING object:self userInfo:dict];
    
    [[mainViewController getOgreViewController] dismissViewControllerAnimated:false completion:^(void)
     {
         // these will change, so we should remove them from being used
         [mainViewController setOgreViewController:nil];
         [mainViewController setOgreView:nil];
         [mainViewController setOgreWindow:nil];
         
         [self deleteRobloxView:YES];
         
         hasReceivedMemoryWarning = NO;
         isCurrentlyPlayingGame = NO;
         
         [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_DID_LEAVE object:self userInfo:dict];
         
         [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxGameState"];
         [[NSUserDefaults standardUserDefaults] synchronize];
         
         isLeavingGame = false;
         
         dispatch_async(dispatch_get_main_queue(), ^{
             [[RobloxWebUtility sharedInstance] writeUpdatedSettings];
         });
         
         // end backgroundTask since we are done
         AppDelegate * appDelegate = (AppDelegate*)[UIApplication sharedApplication].delegate;
         //NSLog(@"Task [%d] Time remaining: %f", appDelegate.bgTask, [[UIApplication sharedApplication] backgroundTimeRemaining]);
         [[UIApplication sharedApplication] endBackgroundTask:appDelegate.bgTask];
         appDelegate.bgTask = UIBackgroundTaskInvalid;
     }];
}

-(void)leaveGame
{
    [self leaveGame:NO];
}

-(void)leaveGame:(BOOL) userRequestedLeave
{
    GameViewController *gcv = [[MainViewController sharedInstance] getOgreViewController];
    NSLog(@"isCurrentlyPlayingGame: %d", isCurrentlyPlayingGame);
    NSLog(@"isLeavingGame: %d", isLeavingGame);
    NSLog(@"[[MainViewController sharedInstance] getOgreViewController]: %p", gcv);
    
    if(!isCurrentlyPlayingGame || isLeavingGame || ([[MainViewController sharedInstance] getOgreViewController] == nil))
        return;
    
    isLeavingGame = true;
    NSLog(@"PlaceLauncher::leaveGame");
    
    // allow screen to dim
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    
    [[NSUserDefaults standardUserDefaults] setObject:@"leaveGame" forKey:@"RobloxGameState"];
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self closeChildConnections];
    
    [[SessionReporter sharedInstance] reportSessionFor:EXIT_GAME];
    [RobloxGoogleAnalytics setPageViewTracking:@"Visit/Success/LeaveGame"];
    
    // set up a background task to give time to finish in application has gone into background to prevent termination
    AppDelegate * appDelegate = (AppDelegate*)[[UIApplication sharedApplication] delegate];
    appDelegate.bgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^
                          {
                              isLeavingGame = false;
                              
                              // stopped or ending the task outright.
                              [[UIApplication sharedApplication] endBackgroundTask:appDelegate.bgTask];
                              appDelegate.bgTask = UIBackgroundTaskInvalid;
                          }];
    
    [[RobloxMemoryManager sharedInstance] stopMaxMemoryTracker];

    dispatch_async(dispatch_get_main_queue(),^{
        [self leaveGameShutdown:userRequestedLeave];
    });
}

-(void)disableViewBecauseGoingToBackground
{
    if (rbxView)
        rbxView->requestStopRenderingForBackgroundMode();
}

-(void)enableViewBecauseGoingToForeground
{
    if (rbxView)
        rbxView->requestResumeRendering();
}

-(void)teleport:(NSString*)ticket withAuthentication:(NSString*)url withScript:(NSString*)script
{
    if (isLeavingGame) return;
    if (!rbxView) return;
    
    //RobloxPageViewController* lastController = (RobloxPageViewController*)[[MainViewController sharedInstance] getLastNonGameController];
    //RBXASSERT([lastController isKindOfClass:[RobloxPageViewController class]]);
    
    UIViewController* lastController = [[MainViewController sharedInstance] getLastNonGameController];
    
    [[PlaceLauncher sharedInstance] setLastNonGameController:lastController needsOnline:YES];
    
    [self deleteRobloxView:NO];
    
    // remove reference to game from controlView (otherwise won't shut down)
    if(MainViewController* mainViewController = [MainViewController sharedInstance])
    {
        GameViewController* gameController = [mainViewController getOgreViewController];
        for(ControlView* control in gameController.view.subviews)
        {
            [control setGame:boost::shared_ptr<RBX::Game>()];
            break;
        }
    }
    
    // first do an empty reset so serviceprovider oldProvider is called first
    currentGame.reset();
    currentGame.reset(new RBX::SecurePlayerGame(NULL, GetBaseURL().c_str()));
    
    // Don't use submit task on data model for this thread, the script fetched in
    // the spawned thread does data model setup.
    // joinGameTeleport will take ownership of the game.
    boost::thread joinScriptThread(boost::bind(&joinGameTeleport, std::string([url UTF8String]), std::string([ticket UTF8String]), std::string([script UTF8String]), lastController, currentGame));
    
    if (isLeavingGame) return;
    if (!isCurrentlyPlayingGame) return;

    [self finishGameSetup:currentGame gameViewController:[[MainViewController sharedInstance] getOgreViewController]];
    rbxView->getDataModel()->submitTask(boost::bind(&finishTeleport, rbxView, currentGame, RBX::FunctionMarshaller::GetWindow()), RBX::DataModelJob::Write);
}

-(BOOL)isCurrentlyPlayingGame
{
    return isCurrentlyPlayingGame;
}

-(void) applicationDidReceiveMemoryWarning
{
    if ([self isCurrentlyPlayingGame] == YES)
    {
        NSLog(@"applicationDidReceiveMemoryWarning - FREE mem: %ld", (long)RBX::MemoryStats::freeMemoryBytes());
        print_free_memory();
        
        // log out of memory
        bool bJoining = (childConnection.connected() || playerConnection.connected());
        
        NSString* placeId = [NSString stringWithFormat:@"%d", lastPlaceId];
        if (bJoining)
        {
            [RobloxGoogleAnalytics setEventTracking:@"PlayErrors" withAction:@"OutOfMemory_EarlyExit" withLabel:placeId withValue:0];
            [[SessionReporter sharedInstance] reportSessionFor:OUT_MEMORY_ON_LOAD PlaceId:lastPlaceId];
        }
        else
        {
            [RobloxGoogleAnalytics setEventTracking:@"PlayErrors" withAction:@"OutOfMemory" withLabel:placeId withValue:0];
            [[SessionReporter sharedInstance] reportSessionFor:OUT_MEMORY_IN_GAME PlaceId:lastPlaceId];
        }
        
        [self clearCachedContent];
    }
    else
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,"PlaceLauncher: applicationDidReceiveMemoryWarning receive while out of game, ignoring");
}

-(void) clearCachedContent
{
    if ([self isCurrentlyPlayingGame] == YES)
    {
        if (rbxView == 0)
        {
            return;
        }
        
        uint64_t preClearBytes = RBX::MemoryStats::usedMemoryBytes();

        rbxView->getView()->garbageCollect();

        uint64_t postClearBytes = RBX::MemoryStats::usedMemoryBytes();

        NSLog(@"PlaceLauncher::clearCachedContent: %.02fMB", (preClearBytes - postClearBytes)/(1048576.0));
    }
}

-(void)childAdded:(shared_ptr<RBX::Instance>) child
{
    NSLog(@"PlaceLauncher::childAdded");
    
    // see if we are the local player
    if (rbxView == 0)
    {
        NSLog(@"robloxView == 0");
        [self closeChildConnections];
        return;
    }
    
    boost::shared_ptr<RBX::DataModel> dataModel = rbxView->getDataModel();
    if (dataModel == 0)
    {
        NSLog(@"dataModel == 0");
        [self closeChildConnections];
        return;
    }
    
    RBX::Network::Players* players = dataModel->find<RBX::Network::Players>();
    
    if (players == 0)
    {
        NSLog(@"players == 0");
        [self closeChildConnections];
        return;
    }
    
    RBX::Network::Player * player = players->getLocalPlayer();
    
    if (player == 0)
    {
        NSLog(@"player == 0");
        [self closeChildConnections];
        return;
    }
    
    if (player == child.get())
    {
        // HOOK INTO OnCharacterAdded
        NSLog(@"player == child");
        playerConnection = player->characterAddedSignal.connect(boostFuncFromSelector_1< shared_ptr<RBX::Instance> >(@selector(playerLoaded:),self));
        
        childConnection.disconnect();
    }
    else
    {
        NSLog(@"player not child but valid");
        playerConnection = player->characterAddedSignal.connect(boostFuncFromSelector_1< shared_ptr<RBX::Instance> >(@selector(playerLoaded:),self));
        
        childConnection.disconnect();
    }
}

-(void) playerLoaded:(shared_ptr<RBX::Instance>) character
{
    NSLog(@"PlaceLauncher::playerLoaded - disabling heartbeat");
    playerConnection.disconnect();
    
    [self closeChildConnections];
    
    [[NSUserDefaults standardUserDefaults] setObject:@"inGame" forKey:@"RobloxGameState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

-(void) closeChildConnections
{
    if (childConnection.connected())
    {
        childConnection.disconnect();
    }
    
    if (playerConnection.connected())
    {
        playerConnection.disconnect();
    }
    
    [[RobloxMemoryManager sharedInstance] stopFreeMemoryChecker];
}

-(shared_ptr<RBX::Game>) getCurrentGame
{
    return currentGame;
}
@end
