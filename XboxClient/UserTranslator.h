#pragma once

#include <atomic>
#include <vector>
#include "boost\unordered_map.hpp"
#include "boost/unordered/unordered_set.hpp"
#include "rbx/threadsafe.h"

class UserIDTranslator
{
public:

    enum ResultState
    {
        Waiting = 0,
        Ready,
        Failed,
        UnknownUser
    };

    struct Entry
    {
        friend class UserIDTranslator;

        std::string gamertag;
        int         robloxUid;

        ResultState getState() const { return (ResultState)state.load(); }
        
    private:
        std::atomic<int> state;
        Entry():
            robloxUid(0),
            state(Waiting){}

        Entry(const std::string& gamertag)
            : gamertag(gamertag)
            , state(Waiting) {}
    };

    typedef std::vector<const Entry*> OutputVector;
    typedef std::vector<std::string> InputVector;
    typedef boost::unordered_set<std::string> GametagSet;

    UserIDTranslator(const std::string& apiUrl_): apiUrl(apiUrl_) {} 
    ~UserIDTranslator();

    void get( OutputVector& result, const InputVector& inputs ); 

    static void waitForUIDs( const std::vector< const UserIDTranslator::Entry* >& uids );

private:
        
    std::string apiUrl; // https://api.roblox.com/xbox/translate
    typedef boost::unordered_map<std::string, Entry*> EntryMap;
    EntryMap entryMap;
    RBX::mutex requestMutex; // taken only during get()
};
