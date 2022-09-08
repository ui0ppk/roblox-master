///////////////////////////////////////////////////////////////////////////////
// XboxMultiplayer
///////////////////////////////////////////////////////////////////////////////

#include "XboxMultiplayerManager.h"
#include "XboxUtils.h"
#include "Util/Http.h"
#include "VoiceChatBase.h"
#include "VoiceChat.h"
#include "VoiceChatMaxNet.h"

#include "async.h" // xbox

#include <collection.h>
#include "xdpevents.h"
#include <ppltasks.h>
#include <map>
#include <sstream>

#include "v8datamodel/PlatformService.h"

SYNCHRONIZED_FASTFLAGVARIABLE(xboxUseMaxNet, false);

Platform::String^ partySessionTemplateString = L"PartySession";
Platform::String^ gameSessionTemplateString = L"GameSession";
Platform::String^ testSessionTemplateString = L"TestSession";

using namespace Microsoft::Xbox::Services;
using namespace Microsoft::Xbox::Services::Multiplayer;

using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::System;
using namespace Windows::Xbox::UI;
using namespace Windows::Xbox::ApplicationModel::Core;
using namespace Windows::Xbox::Multiplayer;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

using namespace Microsoft::Xbox::Services;
using namespace Microsoft::Xbox::Services::Presence;
using namespace Microsoft::Xbox::Services::Social;
using namespace Microsoft::Xbox::Services::Privacy;

using namespace Concurrency;

XboxMultiplayerManager::XboxMultiplayerManager(XboxLiveContext^ xblContext, GUID newPlayerSessionGuid, int newRobloxUserId)
{
	xboxLiveContext	= xblContext;
	stopMultiplayerListeners();
	startMultiplayerListeners();
	gameSession = nullptr;
	expectedLeaveSession = false;
	lastProcessedGameChange = 0;
	localRobloxUserId = newRobloxUserId;
	playerSessionGuid = newPlayerSessionGuid;

	gametimer.reset();
	auto clearResponse = xboxLiveContext->MultiplayerService->ClearActivityAsync(Windows::Xbox::Services::XboxLiveConfiguration::PrimaryServiceConfigId);
}
XboxMultiplayerManager::~XboxMultiplayerManager()
{
	if(gameSession){
		// clean up for gameSession
		leaveGameSession();
	}
	stopMultiplayerListeners();
	xboxLiveContext = nullptr;
}

void XboxMultiplayerManager::startMultiplayerListeners()
{
	RBXASSERT(xboxLiveContext != nullptr);

	if (!xboxLiveContext->RealTimeActivityService->MultiplayerSubscriptionsEnabled)
	{
		sessionChangeToken = xboxLiveContext->RealTimeActivityService->MultiplayerSessionChanged += ref new EventHandler<RealTimeActivity::RealTimeActivityMultiplayerSessionChangeEventArgs^>(
			[=](Platform::Object^, RealTimeActivity::RealTimeActivityMultiplayerSessionChangeEventArgs^ arg){

				dprintf("XboxMultiplayerManager: MultiplayerSessionChanged\n");
				RBX::mutex::scoped_lock lock(taskLock);

				XboxLiveContext^ xblContext = nullptr;
				{
					RBX::mutex::scoped_lock lock(stateLock);
					xblContext = xboxLiveContext;
				}

				if (!xblContext )
				{
					// This can happen in some error conditions where the manager is
					// cleaning/cleaned up and an event occurs.  Just ignore it.
					return;
				}

				MultiplayerSession^ currentSession = nullptr;
				// Retrieve the target session using the activity handle
				async(xblContext->MultiplayerService->GetCurrentSessionAsync(arg->SessionReference)).complete(
					[&currentSession](MultiplayerSession^ t){
						currentSession = t;
				}
				).except( [](Platform::Exception^ e) -> void
				{
					dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
				}
				).join();


				if (currentSession)
				{
					processSessionChange(currentSession);
				}
		});

		subscriptionLostToken = xboxLiveContext->RealTimeActivityService->MultiplayerSubscriptionsLost += ref new EventHandler<RealTimeActivity::RealTimeActivityMultiplayerSubscriptionsLostEventArgs^>(
			[=](Platform::Object^, RealTimeActivity::RealTimeActivityMultiplayerSubscriptionsLostEventArgs^ arg)
		{
			dprintf("XboxMultiplayerManager: MultiplayerSubscriptionsLost\n");

			if (expectedLeaveSession)
			{
				dprintf("Disconnect expected.\n");
				// We are expecting this event and can ignore it
				expectedLeaveSession = false;
			}
			else
			{
				dprintf("Disconnect unexpected.\n");
				//OnMultiplayerStateChanged(nullptr, SessionState::Disconnect);
			}
		});

		xboxLiveContext->RealTimeActivityService->EnableMultiplayerSubscriptions();
	}
}
void XboxMultiplayerManager::stopMultiplayerListeners()
{
	RBXASSERT(xboxLiveContext != nullptr);

	if (xboxLiveContext->RealTimeActivityService->MultiplayerSubscriptionsEnabled)
	{
		xboxLiveContext->RealTimeActivityService->MultiplayerSessionChanged -= sessionChangeToken;
		xboxLiveContext->RealTimeActivityService->MultiplayerSubscriptionsLost -= subscriptionLostToken;
		xboxLiveContext->RealTimeActivityService->DisableMultiplayerSubscriptions();
	}

}

void XboxMultiplayerManager::xbEventMultiplayerRoundStart()
{
	if(gameSession && lastProcessedGameChange > 0)
	{		
		gametimer.reset();
		
		dprintf("IGMRS on START: %S\n",gameSession->MultiplayerCorrelationId->Data() ); // if this is null we can get "NULL START" results from xdi
		EventWriteMultiplayerRoundStart(
			xboxLiveContext->User->XboxUserId->Data(),	// UserId
			&IID_IUnknown,								// RoundId
			0,											// SectionId
			&playerSessionGuid,								// PlayerSessionId
			gameSession->MultiplayerCorrelationId->Data(),	//MultiplayerCorrelationId
			0,											// GameplayModelId
			5,											// MatchTypeId
			0 );											// DifficultyLevelId
	}
}
void XboxMultiplayerManager::xbEventMultiplayerRoundEnd()
{
	if(gameSession && lastProcessedGameChange > 0)
	{
		dprintf("IGMRE on END: %S\n",gameSession->MultiplayerCorrelationId->Data() ); // if this is null we can get "NULL END" results from xdi
		EventWriteMultiplayerRoundEnd(
			xboxLiveContext->User->XboxUserId->Data(),	// UserId
			&IID_IUnknown, // RoundId
			0, // SectionId
			&playerSessionGuid, // PlayerSessionId
			gameSession->MultiplayerCorrelationId->Data(), // MultiplayerSessionCorrelationId
			0, // GameplayerModeId
			5, // MatchTypeId
			0, // DifficultyLevelId
			gametimer.delta().seconds(), // TimeinSeconds
			0 ); // ExitStatusId
	}		
}

void XboxMultiplayerManager::createGameSession(int pmpCreatorId /* = 0*/)
{
    dprintf("XboxMultiplayerManager::createGameSession()\n");

    Platform::String^ gameTemplate = nullptr;
    XboxLiveContext^ xblContext = nullptr;

    {
        RBX::mutex::scoped_lock lock(stateLock);

        gameTemplate =	gameSessionTemplateString;
        xblContext = xboxLiveContext;
    }

    RBXASSERT(xblContext != nullptr);
    std::wstring wstrGameID(RBX::Http::gameID.begin(), RBX::Http::gameID.end());
    Platform::String^ sessionName= ref new Platform::String(wstrGameID.c_str());
    auto session = createNewDefaultSession(gameTemplate,sessionName, pmpCreatorId);
    {
        RBX::mutex::scoped_lock lock(stateLock);
        gameSession = session;
        lastProcessedGameChange = 0;
    }

    auto result = writeSession(xblContext, session, MultiplayerSessionWriteMode::UpdateOrCreateNew);
	
    if (gameSession && result && result->Succeeded)
    {
		// Search the gamesession for a PMPCreator
        // set this session to be the active session so				
		int creatorId = 0;
		parseFromXboxCustomJson(gameSession->SessionConstants->CustomConstantsJson, "pmpCreator", &creatorId);

		if(creatorId == 0)
			xblContext->MultiplayerService->SetActivityAsync(result->Session->SessionReference);		
        
		{// time to init voice chat
			RBX::mutex::scoped_lock lock(voiceUsersLock);
			voiceChat.reset();

			if (SFFlag::xboxUseMaxNet)
                voiceChat = RBX::VoiceChatMaxNet::create(result->Session->CurrentUser, xboxLiveContext);
			else
				voiceChat = RBX::VoiceChat::create(static_cast<unsigned char>(result->Session->CurrentUser->MemberId));
			voiceChat->addLocalUserToChannel(0, xboxLiveContext->User);
			voiceChat->addConnections(result->Session->Members);
		}

        processSessionChange(result->Session);
        xbEventMultiplayerRoundStart();
    }
}

void XboxMultiplayerManager::joinGameSession(Platform::String^ gameSessionUri)
{
	dprintf("XboxMultiplayerManager::JoinGameSessionAsync()\n");

	create_async([this, gameSessionUri]()
	{
		XboxLiveContext^ xblContext = xboxLiveContext;

		assert(xblContext != nullptr);

		MultiplayerSession^ session = nullptr;
		Platform::Exception^ exception = nullptr;

		// Retrieve the target session using the activity handle
		async(xblContext->MultiplayerService->GetCurrentSessionAsync(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference::ParseFromUriPath(gameSessionUri))).complete(
			[&session](MultiplayerSession^ t){
				session = t;
		}
		).except( [&exception](Platform::Exception^ e) -> void
		{
			dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
			exception = e;
		}
		).join();


		// Failure to get the session is fatal
		if (exception )
		{
			dprintf("Failed to get session by handle: %S\n", exception);
			throw ref new Platform::FailureException("Failure to get the session is fatal");
		}

		// Check if the session is full
		if (session->Members->Size >= session->SessionConstants->MaxMembersInSession)
		{
			dprintf("Session is full.\n");
			throw ref new Platform::FailureException("Session is full");
		}

		// Join the session and mark ourselves as active
		session->Join(nullptr);
		session->SetCurrentUserStatus(MultiplayerSessionMemberStatus::Active);
		session->SetCurrentUserSecureDeviceAddressBase64(Windows::Xbox::Networking::SecureDeviceAddress::GetLocal()->GetBase64String());

		// Set RTA event types to be notified of
		session->SetSessionChangeSubscription(
			MultiplayerSessionChangeTypes::SessionJoinabilityChange |
			MultiplayerSessionChangeTypes::MemberListChange |
			MultiplayerSessionChangeTypes::MatchmakingStatusChange |
			MultiplayerSessionChangeTypes::InitializationStateChange |
			MultiplayerSessionChangeTypes::HostDeviceTokenChange
			);

		{
			RBX::mutex::scoped_lock lock(stateLock);
			gameSession = session;
		}

		// Submit it to MPSD
		auto result = updateAndProcessSession(
			xblContext,
			session,
			MultiplayerSessionWriteMode::UpdateExisting
			);

		// Failure to join the session is fatal
		if (result->Succeeded == false)
		{
			throw ref new Platform::FailureException(result->Details);
		}

		// Notify listeners that state has been established
		//OnMultiplayerStateChanged(result->Session, SessionState::None);
	});
	return;
}

void XboxMultiplayerManager::leaveGameSession()
{
    if (voiceChat.get())
    {
		RBX::mutex::scoped_lock lock(voiceUsersLock);
        voiceChat->removeConnections(gameSession->Members);
        voiceChat->removeLocalUserFromChannel();
        voiceChat.reset();
    }
    
	if(!gameSession) return;

	xbEventMultiplayerRoundEnd();

	MultiplayerSession^ game = nullptr;
	XboxLiveContext^ xblContext = nullptr;

	{
		RBX::mutex::scoped_lock lock(stateLock);

		xblContext = xboxLiveContext;
		game = gameSession;
		gameSession = nullptr;
	}

	if (game && game->ChangeNumber > 0 && xblContext)
	{
		expectedLeaveSession = true;

        try
        {
            // Leave the session and update MPSD
            game->Leave();

            // Ignore the results since we've left as far as we're concerned
		    async(xblContext->MultiplayerService->TryWriteSessionAsync(game,MultiplayerSessionWriteMode::UpdateExisting)).detach();
        }
        catch( Platform::Exception^ e )
        {
            dprintf( "leaveGameSession exception: 0x%x %S\n", (unsigned)e->HResult, e->Message->Data() );
        }
	}
}

void XboxMultiplayerManager::setTeamChannel(unsigned char channelId)
{
    if (voiceChat)
    {
        voiceChat->addLocalUserToChannel(channelId, xboxLiveContext->User);
    }
}

void XboxMultiplayerManager::sendGameInvite()
{

	dprintf("SessionName: %S\n", gameSession->SessionReference->SessionName->Data());	
	dprintf("ServiceConfigurationId: %S\n", gameSession->SessionReference->ServiceConfigurationId->Data());
	dprintf("SessionTemplateName: %S\n", gameSession->SessionReference->SessionTemplateName->Data());
	Windows::Xbox::Multiplayer::MultiplayerSessionReference^ multiRef = ref new Windows::Xbox::Multiplayer::MultiplayerSessionReference(
		gameSession->SessionReference->SessionName, 
		gameSession->SessionReference->ServiceConfigurationId, 
		gameSession->SessionReference->SessionTemplateName
		);

	std::wstring wstrGameID(std::to_wstring(localRobloxUserId));
	Platform::String^ gameId = ref new Platform::String(wstrGameID.c_str());

	Windows::Xbox::UI::SystemUI::ShowSendGameInvitesAsync(xboxLiveContext->User, multiRef, gameId);
}
void XboxMultiplayerManager::recentPlayersList()
{
	Windows::Xbox::UI::SystemUI::LaunchRecentPlayersAsync(xboxLiveContext->User);
}
void XboxMultiplayerManager::getInGamePlayers(shared_ptr<RBX::Reflection::ValueArray> values)
{
	if(!xboxLiveContext || !gameSession ) return;
	MultiplayerSession^ session = nullptr;
	
	{
		RBX::mutex::scoped_lock lock(stateLock);
		session = gameSession;		
	}
	dprintf("PRINTING OUT ALL MEMBERS IN GAMESESSION\n");

	for(int i = 0; i < session->Members->Size; i++){
		MultiplayerSessionMember^ player = session->Members->GetAt(i);
		int robloxId = 0;
		parseFromXboxCustomJson(player->MemberCustomConstantsJson,"userid", &robloxId);
		dprintf("\t GamerTag:%S ,XUID:%S ,RobloxId:%d\n", player->Gamertag->Data(), player->XboxUserId->Data(), robloxId );

		shared_ptr<RBX::Reflection::ValueTable> temp(rbx::make_shared<RBX::Reflection::ValueTable>());
		temp->insert(std::pair<std::string, std::string>("xuid", ws2s(player->XboxUserId->Data()) ));
		temp->insert(std::pair<std::string, int>("robloxuid", robloxId ));		

		RBX::Reflection::Variant tempValue = shared_ptr<const RBX::Reflection::ValueTable>(temp);
		values->push_back(tempValue);
	}
}

MultiplayerSession^ XboxMultiplayerManager::createNewDefaultSession( Platform::String^ templateName, Platform::String^ sessionName, int pmpCreatorId /*=0*/ )
{
	XboxLiveContext^ xblContext = nullptr;

	{
		RBX::mutex::scoped_lock lock(stateLock);
		xblContext = xboxLiveContext;
	}

	assert(xblContext != nullptr);

	Platform::String^ serviceConfig = Windows::Xbox::Services::XboxLiveConfiguration::PrimaryServiceConfigId;

	// Create new Session Reference
	auto sessionRef  = ref new Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference(
		serviceConfig,	// Service Config ID of title from XDP
		templateName,		// MPSD Template Name
		sessionName 		// Generated session name
		);

	std::wstring wcustomConst = L"{\"pmpCreator\": \"" + std::to_wstring(pmpCreatorId) + L"\"}";
	Platform::String^ customConst = ref new Platform::String(wcustomConst.c_str()); 	
	// Create a new Multiplayer Session object
	auto session = ref new MultiplayerSession(
		xblContext,                         // XBL Context
		sessionRef,                         // Session reference
		0,                                // Max members; 0 = template value
		false,                              // Not supported, always false
		MultiplayerSessionVisibility::Open, // Allow others to join
		nullptr,                            // Initial users
		customConst                        // Custom constants
		);

	
	// store our Roblox ID in the MPSD so we can do game follow
	// str needs to be formated as a json or else it will throw an undocumented hresult	
	// {
	//     "userId":1234	
	// }
	//

	std::wstring wstrUserID = L"{\"userId\": \"" + std::to_wstring(localRobloxUserId) + L"\"}"; 	

	Platform::String^ customInfo = ref new Platform::String(wstrUserID.c_str());
	session->Join(customInfo, true);

	session->SetCurrentUserStatus(MultiplayerSessionMemberStatus::Active);
	session->SetCurrentUserSecureDeviceAddressBase64(Windows::Xbox::Networking::SecureDeviceAddress::GetLocal()->GetBase64String());

	// Set RTA event types to be notified of
	session->SetSessionChangeSubscription(
		MultiplayerSessionChangeTypes::SessionJoinabilityChange |
		MultiplayerSessionChangeTypes::MemberListChange |
		MultiplayerSessionChangeTypes::MatchmakingStatusChange |
		MultiplayerSessionChangeTypes::InitializationStateChange |
		MultiplayerSessionChangeTypes::HostDeviceTokenChange
		);
	return session;
}


void XboxMultiplayerManager::processSessionChange( MultiplayerSession^ updatedSession )
{
	MultiplayerSession^ currentSession = updatedSession;	
	MultiplayerSession^ previousSession = nullptr;	
	unsigned long long lastProcessedChange = 0;

	{
		RBX::mutex::scoped_lock lock(processLock);

		dprintf("ProcessSessionChange( %u )\n", updatedSession->ChangeNumber);

		XboxLiveContext^ xblContext = nullptr;
		MultiplayerSession^ session = nullptr;

		{
			RBX::mutex::scoped_lock lock(stateLock);

			xblContext = xboxLiveContext;
			session = gameSession;
		}

		if ( !xblContext )
		{
			// This can happen in some error conditions where the manager is
			// cleaning/cleaned up and an event occurs.  Just ignore it.
			return;
		}
		if (session &&
			isStringEqualCaseInsensitive(	updatedSession->SessionReference->SessionName,
			session->SessionReference->SessionName))
		{
			dprintf("GameSession changed.\n");
			RBX::mutex::scoped_lock lock(stateLock);

			previousSession = session;
			lastProcessedChange = lastProcessedGameChange;

			if (!gameSession || lastProcessedGameChange > currentSession->ChangeNumber)
			{
				// The matched session is no longer valid so abort processing
				dprintf("GameSession is no longer valid.\n");
				return;
			}

			gameSession = currentSession;
			lastProcessedGameChange = currentSession->ChangeNumber;
		}
		else
		{
			dprintf("Unable to match session changed event with an existing session.\n");
		}

		if (!currentSession  || !previousSession )
		{
			dprintf("An expected session is null and has probably been left.\n");
			return;
		}
	}

    processSessionDeltas(currentSession, previousSession, lastProcessedChange);
}


WriteSessionResult^ XboxMultiplayerManager::updateAndProcessSession( XboxLiveContext^ context, MultiplayerSession^ session, MultiplayerSessionWriteMode writeMode )
{
	auto result = writeSession(context, session, writeMode);

	if (result && result->Succeeded)
	{
		processSessionChange(result->Session);
	}

	return result;
}

WriteSessionResult^ XboxMultiplayerManager::createPartySession()
{
	dprintf("XboxMultiplayerManager::CreateLobbySession()\n");

	Platform::String^ partyTemplate = nullptr;
	XboxLiveContext^ xblContext = nullptr;

	{
		RBX::mutex::scoped_lock lock(stateLock);

		partyTemplate =	partySessionTemplateString;
		xblContext = xboxLiveContext;
	}

	RBXASSERT(xblContext != nullptr);

	std::string partySessionId;
	RBX::Guid::generateStandardGUID(partySessionId);
	std::wstring wstrPartyID(partySessionId.begin(), partySessionId.end());
	Platform::String^ sessionName= ref new Platform::String(wstrPartyID.c_str());
	auto session = createNewDefaultSession(partyTemplate,sessionName);
	{
		RBX::mutex::scoped_lock lock(stateLock);
		partySession = session;
	}

	// Submit it to MPSD
	return updateAndProcessSession(
		xblContext,
		session,
		MultiplayerSessionWriteMode::UpdateOrCreateNew
		);
}

void XboxMultiplayerManager::joinPartySession(Platform::String^ partySessionUri)
{
	create_async([this, partySessionUri]()
	{
		XboxLiveContext^ xblContext = xboxLiveContext;

		assert(xblContext != nullptr);

		MultiplayerSession^ session = nullptr;
		Platform::Exception^ exception = nullptr;

		// Retrieve the target session using the activity handle
		async(xblContext->MultiplayerService->GetCurrentSessionAsync(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference::ParseFromUriPath(partySessionUri))).complete(
			[&session](MultiplayerSession^ t){
				session = t;
		}
		).except( [&exception](Platform::Exception^ e) -> void
		{
			dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
			exception = e;
		}
		).join();


		// Failure to get the session is fatal
		if (exception )
		{
			dprintf("Failed to get session by handle: %S\n", exception);
			throw ref new Platform::FailureException("Failure to get the session is fatal");
		}

		// Check if the session is full
		if (session->Members->Size >= session->SessionConstants->MaxMembersInSession)
		{
			dprintf("Session is full.\n");
			throw ref new Platform::FailureException("Session is full");
		}

		// Join the session and mark ourselves as active
		session->Join(nullptr);
		session->SetCurrentUserStatus(MultiplayerSessionMemberStatus::Active);
		session->SetCurrentUserSecureDeviceAddressBase64(Windows::Xbox::Networking::SecureDeviceAddress::GetLocal()->GetBase64String());

		// Set RTA event types to be notified of
		session->SetSessionChangeSubscription(
			MultiplayerSessionChangeTypes::SessionJoinabilityChange |
			MultiplayerSessionChangeTypes::MemberListChange |
			MultiplayerSessionChangeTypes::MatchmakingStatusChange |
			MultiplayerSessionChangeTypes::InitializationStateChange |
			MultiplayerSessionChangeTypes::HostDeviceTokenChange
			);

		{
			RBX::mutex::scoped_lock lock(stateLock);
			partySession = session;
		}

		// Submit it to MPSD
		auto result = updateAndProcessSession(
			xblContext,
			session,
			MultiplayerSessionWriteMode::UpdateExisting
			);

		// Failure to join the session is fatal
		if (result->Succeeded == false)
		{
			throw ref new Platform::FailureException(result->Details);
		}

		// Notify listeners that state has been established
		//OnMultiplayerStateChanged(result->Session, SessionState::None);
	});
	return;
}
void XboxMultiplayerManager::leavePartySession()
{
	MultiplayerSession^ party = nullptr;
	XboxLiveContext^ xblContext = nullptr;

	{
		RBX::mutex::scoped_lock lock(stateLock);

		xblContext = xboxLiveContext;
		party = partySession;
		partySession = nullptr;
	}

	if (party && xblContext)
	{
		expectedLeaveSession = true;

		// Leave the session and update MPSD
		party->Leave();

		// Ignore the results since we've left as far as we're concerned
		writeSession(
			xblContext,
			party,
			MultiplayerSessionWriteMode::UpdateExisting
			);
	}
}

void XboxMultiplayerManager::joinMultiplayerSession(Platform::String^ handleID, Platform::String^ xuid)
{

	create_async([this, handleID, xuid]()
	{
		XboxLiveContext^ xblContext = xboxLiveContext;

		MultiplayerSession^ session = nullptr;
		Platform::Exception^ exception = nullptr;

		// Retrieve the lobby session using the activity handle
		create_task(
			xblContext->MultiplayerService->GetCurrentSessionByHandleAsync(
			handleID
			)
			).then([&session, &exception] (task<MultiplayerSession^> t)
		{
			try
			{
				session = t.get();
			}
			catch(Platform::Exception^ ex)
			{
				exception = ex;
				// Fall through...
			}
		}
		).wait();

		// Failure to get the session is fatal
		if (exception )
		{
			dprintf("Failed to get session by handle: %S\n", exception);
			throw ref new Platform::FailureException("Failure to get the session is fatal");
		}

		// Check if the session is full
		if (session->Members->Size >= session->SessionConstants->MaxMembersInSession)
		{
			dprintf("Session is full.\n");
			throw ref new Platform::FailureException("Session is full");
		}


		// Join the session and mark ourselves as active
		session->Join(nullptr);
		session->SetCurrentUserStatus(MultiplayerSessionMemberStatus::Active);
		session->SetCurrentUserSecureDeviceAddressBase64(Windows::Xbox::Networking::SecureDeviceAddress::GetLocal()->GetBase64String());

		// Set RTA event types to be notified of
		session->SetSessionChangeSubscription(
			MultiplayerSessionChangeTypes::SessionJoinabilityChange |
			MultiplayerSessionChangeTypes::MemberListChange |
			MultiplayerSessionChangeTypes::MatchmakingStatusChange |
			MultiplayerSessionChangeTypes::InitializationStateChange |
			MultiplayerSessionChangeTypes::HostDeviceTokenChange
			);
		{
			RBX::mutex::scoped_lock lock(stateLock);
			partySession = session;
		}


		// Submit it to MPSD
		auto result = updateAndProcessSession(
			xblContext,
			session,
			MultiplayerSessionWriteMode::UpdateExisting
			);

		// Failure to join the lobby session is fatal
		if (result->Succeeded == false)
		{
			throw ref new Platform::FailureException(result->Details);
		}

		// Publish the lobby session as the user's activity
		xblContext->MultiplayerService->SetActivityAsync(result->Session->SessionReference);

		// Notify listeners that state has been established
		//OnMultiplayerStateChanged(result->Session, SessionState::None);
	});
}

WriteSessionResult^ XboxMultiplayerManager::writeSession( XboxLiveContext^ context, MultiplayerSession^ session, MultiplayerSessionWriteMode writeMode )
{
	dprintf("MultiplayerManager::WriteSession()\n");
	WriteSessionResult^ result = nullptr;
	int counter = 0;
	while(counter < 5)
	{
		try{
			async(context->MultiplayerService->TryWriteSessionAsync(session,writeMode)).complete(
				[&result, &counter](WriteSessionResult^ response)
			{
				result = response;
				counter = 5;
			}
			).except(
				[](Platform::Exception^ e)
			{
				dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
			}
			).join();
		}catch(Platform::COMException^ e){
				dprintf("FAILED UserAuth COMException: [0x%x] (%S)\n", e->HResult, e->Message->Data());	
			if(e->HResult == 0x80190194)
			{
				counter = 5;
			}
		}
		counter++;
	}

	if(result)
	{
		// 0 = Unknown
		// 1 = AccessDenied
		// 2 = Created
		// 3 = Conflicted
		// 4 = HandleNotFound
		// 5 = OutOfSync
		// 6 = SessionDeleted
		// 7 = Updated
		dprintf("WriteSessionStatus: %d\n", result->Status);
	}

	return result;
}

//  Performs a diff between two sessions and preforms actions based on the
//  changed values.
void XboxMultiplayerManager::processSessionDeltas( MultiplayerSession^ currentSession, MultiplayerSession^ previousSession, unsigned long long lastChange )
{
	dprintf("MultiplayerManager::ProcessSessionDeltas()\n");

	// Make sure we haven't seen this change before
	if (currentSession->ChangeNumber < lastChange)
	{
		dprintf(
			"Change %u has already been processed for session %S, most recent: %u\n",
			currentSession->ChangeNumber,
			currentSession->SessionReference->SessionName->Data(),
			lastChange );

		return;
	}

	dprintf(
		"Diffing sessions { id: %S, num: %d } vs. { id: %S, num: %d }\n",
		previousSession->SessionReference->SessionName->Data(),
		previousSession->ChangeNumber,
		currentSession->SessionReference->SessionName->Data(),
		currentSession->ChangeNumber );

	// Perform a session diff
	auto diff = MultiplayerSession::CompareMultiplayerSessions(
		currentSession,
		previousSession
		);

	if (isMultiplayerSessionChangeTypeSet(diff, MultiplayerSessionChangeTypes::MemberListChange))
	{
		dprintf(
			"ProcessSessionDeltas: MemberListChanged from: %d, %d\n",
			previousSession->Members->Size,
			currentSession->Members->Size );

        if (voiceChat)
        {
            // find users that were removed
            for (MultiplayerSessionMember^ memberOld : previousSession->Members)
            {
                bool found = false;
                for (MultiplayerSessionMember^ memberNew : currentSession->Members)
                {
                    if (memberNew->XboxUserId == memberOld->XboxUserId)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    voiceChat->multiplayerUserRemoved(memberOld);

                    int rbxId;
                    if (parseFromXboxCustomJson(memberOld->MemberCustomConstantsJson,"userid", &rbxId))
                    {
                        RBX::mutex::scoped_lock lock(voiceUsersLock);
                        rbxIdToSessionId.erase(rbxId);
                    }
            }
            }

            // find new users 
            for (MultiplayerSessionMember^ memberNew : currentSession->Members)     
            {
                bool found = false;
                for (MultiplayerSessionMember^ memberOld : previousSession->Members)
                {
                    if (memberNew->XboxUserId == memberOld->XboxUserId)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    voiceChat->multiplayerUserAdded(memberNew);
                    
                    int rbxId;
                    if (parseFromXboxCustomJson(memberNew->MemberCustomConstantsJson, "userid", &rbxId))
                    {
                        RBX::mutex::scoped_lock lock(voiceUsersLock);
                        rbxIdToSessionId[rbxId] = memberNew->MemberId;
                        
                        if (mutedUsersRbxIds.find(rbxId) != mutedUsersRbxIds.end())
                            voiceChat->muteUser(memberNew->MemberId);
            }
            }
        }   
	}
    }

	if (isMultiplayerSessionChangeTypeSet(diff, MultiplayerSessionChangeTypes::SessionJoinabilityChange))
	{
		dprintf("ProcessSessionDeltas: SessionJoinabilityChange from %d, %d\n", previousSession->SessionProperties->JoinRestriction, currentSession->SessionProperties->JoinRestriction );
	}

	if(isMultiplayerSessionChangeTypeSet(diff, MultiplayerSessionChangeTypes::HostDeviceTokenChange))
	{
		dprintf("ProcessSessionDeltas: HostDeviceTokenChange\n");
	}
}

int XboxMultiplayerManager::getPMPCreatorId()
{	
	int creatorId = -1;
	RBX::Timer<RBX::Time::Fast> timer;
	timer.reset();

	while(creatorId < 0 && timer.delta().seconds() < 30) // 5 min timeout
	{
		MultiplayerSession^ session = nullptr;
		{
			RBX::mutex::scoped_lock lock(stateLock);	
			session = gameSession;		
		}
		if(session)
		{		
			parseFromXboxCustomJson(session->SessionConstants->CustomConstantsJson, "pmpCreator", &creatorId);
			break;
		}
		Sleep(500);
	}
	return creatorId;
}

bool XboxMultiplayerManager::parseFromXboxCustomJson(Platform::String^ jsonStr,std::string str, int* out)
{
	std::string userIdStr = ws2s(jsonStr->Data());
	int ind1 = userIdStr.find(str);
	int ind2 = userIdStr.find(":", ind1+1);
	ind2 = userIdStr.find("\"", ind2+1);
	int ind3 = userIdStr.find("\"", ind2+1);
	std::string id = userIdStr.substr(ind2+1 , ind3-ind2-1);
	std::stringstream convert(id); 
	if(convert >> *out)
		return true;
	return false;
}

void XboxMultiplayerManager::updatePlayer(unsigned int rbxId, float distance, RBX::VoiceChatBase::TalkingDelta* talkingDeltaOut)
{
    RBX::mutex::scoped_lock lock(voiceUsersLock);
    RbxIdToSessionIdMap::const_iterator it = rbxIdToSessionId.find(rbxId);
    *talkingDeltaOut = RBX::VoiceChatBase::TalkingChange_NoChange;

    if (voiceChat && it != rbxIdToSessionId.end())
    {
        voiceChat->updateMember(it->second, distance, talkingDeltaOut);
    }
}

void XboxMultiplayerManager::muteVoiceChatPlayer(unsigned rbxId)
{
    RBX::mutex::scoped_lock lock(voiceUsersLock);
    RbxIdToSessionIdMap::const_iterator it = rbxIdToSessionId.find(rbxId);

    if (voiceChat && it != rbxIdToSessionId.end())
    {
        mutedUsersRbxIds.insert(it->second);
        voiceChat->muteUser(it->second);
    }
}

void XboxMultiplayerManager::unmuteVoiceChatPlayer(unsigned rbxId)
{
    RBX::mutex::scoped_lock lock(voiceUsersLock);
    RbxIdToSessionIdMap::const_iterator it = rbxIdToSessionId.find(rbxId);

    if (voiceChat && it != rbxIdToSessionId.end())
    {
        mutedUsersRbxIds.erase(it->second);
        voiceChat->unmuteUser(it->second);
    }
}

unsigned XboxMultiplayerManager::voiceChatGetState(int rbxId)
{
    RbxIdToSessionIdMap::const_iterator it = rbxIdToSessionId.find(rbxId);

    if (voiceChat && it != rbxIdToSessionId.end())
        return voiceChat->getMuteState(it->second);

    return (unsigned)RBX::voiceChatState_UnknownUser;
}

int XboxMultiplayerManager::requestJoinParty()
{
	// Request 10 friends to display in the people picker
	auto getSocialRelationshipsTask = create_task(xboxLiveContext->SocialService->GetSocialRelationshipsAsync(
		Social::SocialRelationship::All, 0, 10));

	getSocialRelationshipsTask.then([this](task<XboxSocialRelationshipResult^> t)
	{
		auto socialRelationshipResult = t.get();
		if (socialRelationshipResult->Items->Size == 0)
		{
			dprintf("Error, no results in social list\n");
			return;
		}

		auto userList = ref new Platform::Collections::Vector<Platform::String^>();
		for (auto relationship : socialRelationshipResult->Items)
		{
			userList->Append(relationship->XboxUserId);
		}

		auto selectPeopleTask = create_task(Windows::Xbox::UI::SystemUI::ShowPeoplePickerAsync(xboxLiveContext->User,	// the Requesting User
			L"PICK YOUR PARTY MEMBERS", 
			userList->GetView(),
			nullptr, 1, userList->Size));

		selectPeopleTask.then([this](task<IVectorView<Platform::String^>^> t){
			auto selectedPlayers = t.get();
			for(int i = 0; i < selectedPlayers->Size;i++){
				dprintf("%S\n", selectedPlayers->GetAt(i)->Data());
			};

			Windows::Xbox::Multiplayer::PartyChatView^ view;

			async( PartyChat::GetPartyChatViewAsync() ).complete(
				[&]( PartyChatView^ pcv )
			{
				view = pcv;
			}
			).join();

			//Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionReference^ mpsr = view;
			//Microsoft::Xbox::Services::Multiplayer::MultiplayerService::SendInvitesAsync(mpsr, selectedPlayers, TITLE_ID);

			Party::InviteToPartyAsync(xboxLiveContext->User, selectedPlayers);

			//IPartyChatStatics.GetPartyChatViewAsync()
		});
	});

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// XboxMultiplayer::End
///////////////////////////////////////////////////////////////////////////////
