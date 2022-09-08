#pragma once
// in this file...
// - fast flag fetching helpers
// - join script fetching/parsing helpers
// - unicode conversion
#include <string>
#include <locale>
#include <functional>
#include "rbx/Boost.hpp"

namespace RBX 
{ 
    class DataModel;
    class DataModelJob; 
}


void dprintf( const char* fmt, ... );

bool loadLocalFFlags();

bool fetchFFlags(const char* baseUrl);

// status codes
enum PlaceLauncherResult
{
    PlaceLaunch_Waiting = 0, // wait
    PlaceLaunch_Loading = 1, // wait
    PlaceLaunch_Joining = 2, // *the* success code!
    PlaceLaunch_Disabled = 3,
    PlaceLaunch_Error = 4,
    PlaceLaunch_GameEnded = 5,
    PlaceLaunch_GameFull = 6,
    PlaceLaunch_UserLeft = 10,
    PlaceLaunch_Restricted = 11,
    PlaceLaunch_Unauthorized = 12,

    PlaceLaunch_SomethingReallyBad = 1000, // not a real status
};


PlaceLauncherResult requestPlaceInfo(const std::string url, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl);

bool isStringEqual(Platform::String^ val1,Platform::String^ val2);
bool isStringEqualCaseInsensitive(Platform::String^ val1,Platform::String^ val2);

std::string ws2s(const wchar_t* data);
std::wstring s2ws(const std::string* data);

template< class M >
inline typename M::mapped_type* mapget( M& m, const typename M::key_type& k )
{
    M::iterator it = m.find(k);
    if( it == m.end() ) return 0;
    return &it->second;
}

shared_ptr< RBX::DataModelJob > addGenericDataModelJob( const char* name, int type, shared_ptr< RBX::DataModel> dm, double period, std::function<void()> step );

void removeGenericDataModelJob( shared_ptr< RBX::DataModelJob >& job, std::function<void()> spinfunc );
