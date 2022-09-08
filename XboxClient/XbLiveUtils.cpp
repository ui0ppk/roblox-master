
#include "XbLiveUtils.h"


#include <collection.h>
#include "xdpevents.h"
#include <ppltasks.h>
#include <map>

#include "Util/Http.h"
#include "XboxUtils.h"
#include "UserTranslator.h"
#include "async.h"
#include "rbx/make_shared.h"

using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::System;
using namespace Windows::Xbox::UI;
using namespace Windows::Xbox::ApplicationModel::Core;
using namespace Windows::Xbox::Multiplayer;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation;

using namespace Microsoft::Xbox::Services;
using namespace Microsoft::Xbox::Services::Presence;
using namespace Microsoft::Xbox::Services::Social;
using namespace Microsoft::Xbox::Services::Privacy;

#define TITLE_ID 0x1465F7BC
#include <map>


int getFriends(std::string* ret, UserIDTranslator* translator, Microsoft::Xbox::Services::XboxLiveContext^ xblContext)
{
    bool comma = false;

    if( !xblContext || !xblContext->User->IsSignedIn)
    {
        dprintf("User not signed in, friends are not available.\n");
        return -1;
    }

	XboxLiveContext^ xboxLiveContext = xblContext;

    try{

    std::vector< Platform::String^ > friendXuids;
    IVectorView< XboxUserProfile^ >^ friendProfiles = ref new Platform::Collections::VectorView< XboxUserProfile^ >(); // make sure we it's not null if an exception occurs
    IVectorView< PresenceRecord^ >^ friendPresence  = ref new Platform::Collections::VectorView< PresenceRecord^ >();
    std::vector< std::string > friendGamertags;
    std::vector< const UserIDTranslator::Entry* > friendRobloxUids;

    try {

        // get all favorite people from player's list
        async( xboxLiveContext->SocialService->GetSocialRelationshipsAsync()).complete(
            [&friendXuids]( XboxSocialRelationshipResult^ result ) -> void
            {
                for ( auto r : result->Items )
                {
                    if( r->IsFollowingCaller )
                        friendXuids.push_back( r->XboxUserId );
                }
            } 
        ).except( [](Platform::Exception^ e) -> void
            {
                dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
            }
        ).join();

		if( friendXuids.size() == 0)
		{
			dprintf("User has no friends.\n");
			return -1;
		}

        // get those people's profiles
        auto& a1 = async( xboxLiveContext->ProfileService->GetUserProfilesAsync( ref new Platform::Collections::VectorView< Platform::String^ > ( friendXuids ) ) ).complete(
            [&friendProfiles, &friendGamertags]( IVectorView< XboxUserProfile^ > ^ profiles ) -> void
            {
                friendProfiles = profiles;
                for (auto pr : profiles)
                {
                    friendGamertags.push_back(ws2s(pr->Gamertag->Data()));
                }
            }
        );

        // get those people's presence
        auto& a2 = async( xboxLiveContext->PresenceService->GetPresenceForMultipleUsersAsync( ref new Platform::Collections::VectorView< Platform::String^ >( friendXuids ) ) ).complete(
            [&friendPresence]( IVectorView< PresenceRecord^ >^ pr )
            {
                friendPresence = pr;
            }
        );
		
        a1.join();
        a2.join();

        translator->get( friendRobloxUids, friendGamertags );
        UserIDTranslator::waitForUIDs(friendRobloxUids); // we've got nothing better to do
    }
    catch( Platform::Exception^ e )
    {
        dprintf( "%s exception [0x%x] '%S'. Not an error, but the returned data might be incomplete.\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
        // The function should still generate a well-formed json response string. 
        // Talk to Max if you're hitting this and the string is not well-formed.
    }

    // filter out unresolved roblox ids
    std::map< std::string, int > rbidLookup;
    for( auto e : friendRobloxUids )
    {
        auto st = e->getState();
        if( st == UserIDTranslator::Ready )
            rbidLookup[e->gamertag] = e->robloxUid;
    }

    // not so sure about the order, so...
    std::map< std::wstring, std::wstringstream > combined; // xuid -> user info
    
    
    for ( auto xuid : friendXuids )
        combined[xuid->Data()] << L"    {\n        \"xuid\" : \"" << xuid->Data() << L"\", ";

    for ( auto profile : friendProfiles )
    {
        if ( auto str = mapget( combined, profile->XboxUserId->Data() ) )
        {
			// Yes, we repeat the gamertag data; this is a quick fix to show gamertag in app instead of display name due to us not wanting
			// to fail cert for having unsupported unicode characters.
            (*str) << L"\"gamertag\" : \"" << profile->Gamertag->Data() << L"\", " << L"\"display\" : \"" << profile->Gamertag->Data() << L"\", ";
            (*str) << L"\"robloxuid\" : ";

            if( int robloxuid = rbidLookup[ ws2s(profile->Gamertag->Data()) ] )
                (*str) << robloxuid <<", ";
            else
                (*str) << L"null, ";
        }
        else
        {
            dprintf("Warn: got profile for %S, didn\"t ask for.\n", profile->XboxUserId->Data() );
        }
    }

    for ( auto presence : friendPresence )
    {
        if ( auto str = mapget( combined, presence->XboxUserId->Data() ) )
        {
            (*str) << L"\"status\" : \"" << presence->UserState.ToString()->Data() << L"\", ";
            (*str) << L"\"rich\":\n        [\n";

            comma = false;
            for ( auto r: presence->PresenceTitleRecords )
            {
                if( comma ) (*str) << ",\n";
                (*str) << L"            { \"timestamp\" : \"" << r->LastModifiedDate.UniversalTime.ToString()->Data() << "\", \"device\" : \"" << r->DeviceType.ToString()->Data() << "\",  \"title\" : \"" << r->TitleName->Data() << "\", \"titleId\" : \"" << r->TitleId.ToString()->Data() <<  "\" , \"playing\" : \"" << r->IsTitleActive.ToString()->Data() << "\", \"presence\" : \"" << r->Presence->Data() << "\"}";
                comma = true;
            }
            (*str) << L"\n        ]\n";
        }
        else
        {
            dprintf("Warn: got presence for %S, didn't ask for.\n", presence->XboxUserId->Data() );
        }
    }

    *ret = "{ \"friends\": [\n";
    comma = false;
    for ( auto& it : combined )
    {
        it.second << L"    }";
        if( comma ) *ret += ",\n";
        *ret += ws2s( it.second.str().c_str() );
        comma = true;
    }
    *ret += "\n]\n }\n";

    return 0;

    } catch( Platform::Exception^ e )
    {
        dprintf( "Friends exception: 0x%x %S\n", e->HResult, e->Message->Data() );
    }
    return -1;
}

int getParty(std::vector<std::string>* result, bool filter, UserIDTranslator* translator, XboxLiveContext^ xblContext )
{

	if(!xblContext||!xblContext->User->IsSignedIn){ 
		dprintf("User not signed in, friends are not available.\n");
		return 1;
	}
	XboxLiveContext^ xboxLiveContext = xblContext;
    struct PartyUserInfo
    {
        Platform::String^ xuid;
        Platform::String^ gamertag;
        int               robloxuid;
        int               online;
    };

    std::map< std::string, PartyUserInfo > partyMembers; // xuid->party member info
    std::map< std::string, PartyUserInfo* > gtlookup; // gamertag->same party member info
    std::vector< Platform::String^ > partyMemberXuids; // list of xbox party members
    std::vector< std::string > allgamertags; // list of xbox party members
    result->clear();

    try
    {
        // get Xuids of party members
        async( PartyChat::GetPartyChatViewAsync() ).complete(
            [&]( IPartyChatView^ pv ) -> void
            {
                if( !pv || !pv->Members ) 
                    return;
                for( auto mem : pv->Members )
                {
                    std::string xuid = ws2s(mem->XboxUserId->Data());
                    partyMemberXuids.push_back(mem->XboxUserId);

                    PartyUserInfo info = {};
                    info.xuid = mem->XboxUserId;
                    bool b = partyMembers.insert( std::make_pair(xuid, info) ).second;
                    if( !b )
                    {
                        dprintf("getParty(): got a duplicate xuid [%s] from XBL.\n", xuid.c_str());
                    }
                }
            }
        ).except( 
            [=](Platform::Exception^ e)
            {
                dprintf("getParty(): exception 0x%x %S\n", e->HResult, e->Message->Data() );
            }
        ).join();

        if( partyMembers.empty() )
        {
            dprintf("NOTE: getParty(): no party members.\n");
            return 1; // no party
        }

        // look up user profiles to get their gamertags
        async( xboxLiveContext->ProfileService->GetUserProfilesAsync( ref new Platform::Collections::VectorView< Platform::String^ > ( partyMemberXuids ) ) ).complete(
            [&]( IVectorView< XboxUserProfile^ > ^ profiles ) -> void
            {
                for( auto pr : profiles )
                {
                    std::string xuid = ws2s( pr->XboxUserId->Data() );
                    if( PartyUserInfo* info = mapget(partyMembers, xuid) )
                    {
                        std::string gamertag = ws2s( pr->Gamertag->Data() );
                        allgamertags.push_back( gamertag );
                        info->gamertag = pr->Gamertag;
                        gtlookup[gamertag] = info;
                    }
                    else
                    {
                        dprintf("getParty(): got gamertag for [%s], didn't ask for.\n", xuid.c_str() );
                    }
                }
            }
        ).join();

        if( !filter )
        {
            result->swap(allgamertags);
            return 0; // done
        }

        // check that these players are online and have roblox ids
        std::vector< const UserIDTranslator::Entry* > robloxuids;
        translator->get( robloxuids, allgamertags );

        // in parallel, fetch friends' online status
        async( xboxLiveContext->PresenceService->GetPresenceForMultipleUsersAsync( ref new Platform::Collections::VectorView< Platform::String^ >( partyMemberXuids ) ) ).complete(
            [&]( IVectorView<PresenceRecord^>^ friends )
            {
                for( auto rec : friends )
                {
                    std::string xuid = ws2s(rec->XboxUserId->Data());
                    if( PartyUserInfo* info = mapget(partyMembers, xuid) )
                    {
                        info->online = rec->UserState == UserPresenceState::Online;
                    }
                    else
                    {
                        dprintf("getParty(): got presence for [%s], didn't ask for.\n", xuid.c_str());
                    }
                }
            }
        ).join();

        UserIDTranslator::waitForUIDs(robloxuids);

        // finally, fill out roblox userids
        for( int j=0, e=robloxuids.size(); j<e; ++j )
        {
            auto st = robloxuids[j]->getState();
            if( st == UserIDTranslator::Ready )
            {
                if( PartyUserInfo* info = gtlookup[ robloxuids[j]->gamertag ] )
                {
                    info->robloxuid = robloxuids[j]->robloxUid;
                }
                else
            {
                    dprintf("getParty(): got Roblox UserId for [%s], didn't ask for.\n", robloxuids[j]->gamertag.c_str() );
                }
            }
        }

        // now fill out the result
        for( auto& it : partyMembers )
        {
            auto& info = it.second;
            if( info.online && info.robloxuid > 0 )
            {
                result->push_back(ws2s(info.gamertag->Data()));
            }
        }

        if( result->empty() )
        {
            dprintf("NOTE: getParty(): all players were filtered out.");
            return 1;
        }

        return 0;
    }
    catch( Platform::Exception^ e )
    {
        dprintf( "ERROR: getParty() exception: 0x%x %s\n", e->HResult, e->Message->Data() );
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
// Xbox live events (used for achievements)

void xboxEvents_init()
{
    EventRegisterRBLX_1465F7BC();
}

void xboxEvents_shutdown()
{
    EventUnregisterRBLX_1465F7BC();
}

static ULONG sendEvent( const ETX_EVENT_DESCRIPTOR* desc, const wchar_t* userId, const GUID* playerSessionId )
{
    enum { kNArgs = 3 };
    EVENT_DATA_DESCRIPTOR EventData[kNArgs];
    UINT8 scratch[64];

    EtxFillCommonFields_v7(&EventData[0], scratch, 64);

    EventDataDescCreate(&EventData[1], (userId != NULL) ? userId : L"", (userId != NULL) ? (ULONG)((wcslen(userId) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L""));
    EventDataDescCreate(&EventData[2], playerSessionId, sizeof(GUID));

	try{
		return EtxEventWrite(desc, &RBLX_1465F7BCProvider, RBLX_1465F7BCHandle, kNArgs, EventData);
	}catch(Platform::Exception^ e){
		dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult,  e->Message->Data() );
		return 1;
	}
}

static ULONG sendEvent( const ETX_EVENT_DESCRIPTOR* desc, const wchar_t* userId, const GUID* playerSessionId, double value)
{
    enum { kNArgs = 4 };
    EVENT_DATA_DESCRIPTOR EventData[kNArgs];
    UINT8 scratch[64];

    EtxFillCommonFields_v7(&EventData[0], scratch, 64);

    EventDataDescCreate(&EventData[1], (userId != NULL) ? userId : L"", (userId != NULL) ? (ULONG)((wcslen(userId) + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L""));
    EventDataDescCreate(&EventData[2], playerSessionId, sizeof(GUID));

    float valFlt = 0;
    int valInt = 0;

    switch (desc->FieldDescriptors[3].Type)
    {
    case EtxFieldType_Int32: 
        {
            valInt = (int)value;
            EventDataDescCreate(&EventData[3], &valInt, sizeof(int));
            break;
        }
    case EtxFieldType_Float:
        {
            valFlt = (float)value;
            EventDataDescCreate(&EventData[3], &valFlt, sizeof(float));
            break;
        }
    case EtxFieldType_Double:
        {
            EventDataDescCreate(&EventData[3], &value, sizeof(double));
            break;
        }
    default: RBXASSERT(false); // unsupported type
    }

    
	try{
		return EtxEventWrite(desc, &RBLX_1465F7BCProvider, RBLX_1465F7BCHandle, kNArgs, EventData);
	}catch(Platform::Exception^ e){
		dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult,  e->Message->Data() );
		return 1;
	}
}

static const ETX_EVENT_DESCRIPTOR* findEvent( const char* name )
{
    const ETX_EVENT_DESCRIPTOR* const table = RBLX_1465F7BCEvents;
    enum { kCount = sizeof(RBLX_1465F7BCEvents) / sizeof(RBLX_1465F7BCEvents[0]) };

    for( int j=0; j<kCount; ++j )
    {
        if( strcmp( table[j].Name, name ) == 0 )
            return &table[j];
    }
    return 0;
}

RBX::AwardResult xboxEvents_send( Microsoft::Xbox::Services::XboxLiveContext^ xboxLiveContext, const char* evtName, double *value /*= NULL*/)
{
    if( !xboxLiveContext ) return RBX::Award_NoUser;

    auto e = findEvent(evtName);
    if( !e ) return RBX::Award_NotFound;

    ULONG r = 1;
    if (!value)
        r = sendEvent( e, xboxLiveContext->User->XboxUserId->Data(), &IID_IUnknown);
    else
        r = sendEvent( e, xboxLiveContext->User->XboxUserId->Data(), &IID_IUnknown, *value);

    return r == 0 ?  RBX::Award_OK : RBX::Award_Fail;
}


