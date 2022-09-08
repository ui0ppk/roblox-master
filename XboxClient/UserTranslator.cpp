#include "UserTranslator.h"
#include "util/Http.h"
#include "v8xml/WebParser.h"
#include "rbx/make_shared.h"

extern void dprintf( const char* fmt, ... );

    static std::string constructJsonRequest(const UserIDTranslator::GametagSet& tags)
    {
        if (!tags.empty())
        {
            std::string requestIds = "{\"ids\":[\"";
            bool noComma = true;
            for(UserIDTranslator::GametagSet::const_iterator inputIt = tags.begin(); inputIt != tags.end(); ++inputIt)
            {
                if (noComma)
                    noComma = false;
                else
                    requestIds += "\",\"";
                requestIds += *inputIt;
            }
            requestIds += "\"]}";

            return requestIds;
        }
        return "";
    }

    UserIDTranslator::~UserIDTranslator()
    {
        for(EntryMap::iterator it = entryMap.begin(); it != entryMap.end(); ++it)
        {
            delete it->second;
        }
        entryMap.clear();
    }

    void UserIDTranslator::get( OutputVector& result, const InputVector& inputs )
    {
        GametagSet gamerTagsToConvert;

        if (1)
        {
            RBX::mutex::scoped_lock lock(requestMutex);
            result.clear();

            for(InputVector::const_iterator inputIt = inputs.begin(); inputIt != inputs.end(); ++inputIt)
            {
                std::string xuid = std::string(*inputIt);
                Entry* entry = entryMap[xuid];

                if (!entry)
                {
                    entry = entryMap[xuid] = new Entry(*inputIt);
                    gamerTagsToConvert.insert(xuid);
                }
                else
                {
                    switch (entry->getState())
                    {
                        case UserIDTranslator::UnknownUser:     // lets try again
                        case UserIDTranslator::Failed:             
                            gamerTagsToConvert.insert(xuid); 
                            entry->state.store((int)Waiting);
                            break;
                        case UserIDTranslator::Ready:           // Horray! do nothing
                        case UserIDTranslator::Waiting:         // Still waiting for response, do notning
                            break;
                    }
                }

                result.push_back(entry);
            }
        }

        if (!gamerTagsToConvert.empty())
        {
            std::string request = constructJsonRequest(gamerTagsToConvert);

            RBX::Http( apiUrl ).post(request, RBX::Http::kContentTypeApplicationJson, false, 
                
                [this, gamerTagsToConvert](std::string* response, std::exception* exception) mutable
                {
                    if (exception)
                        dprintf("%s\n", exception->what());

                    if (response)
                    {
                        shared_ptr<const RBX::Reflection::ValueTable> jsonResult(rbx::make_shared<const RBX::Reflection::ValueTable>());
                        bool parseResult = RBX::WebParser::parseJSONTable(*response, jsonResult);

                        RBX::Reflection::ValueTable::const_iterator itUsers = jsonResult->find("Users");
                        if (itUsers != jsonResult->end() && itUsers->second.isType<shared_ptr<const RBX::Reflection::ValueArray> >())
                        {
                            shared_ptr<const RBX::Reflection::ValueArray> entries = itUsers->second.cast<shared_ptr<const RBX::Reflection::ValueArray> >();

                            for(RBX::Reflection::ValueArray::const_iterator it = entries->begin(); it != entries->end(); ++it)
                            {
                                if(!it->isType<shared_ptr<const RBX::Reflection::ValueTable> >()) 
                                    continue;

                                shared_ptr<const RBX::Reflection::ValueTable> keyValueEntry = it->cast<shared_ptr<const RBX::Reflection::ValueTable> >();

                                RBX::Reflection::ValueTable::const_iterator itGamertag = keyValueEntry->find("Id");
                                RBX::Reflection::ValueTable::const_iterator itUserId = keyValueEntry->find("UserId");

                                if(itGamertag == keyValueEntry->end() || itUserId == keyValueEntry->end())
                                {
                                    dprintf("UserTranslator: problem decoding part of JSON response\n");
                                    continue;
                                }

                                std::string gamertag = "";
                                if (itGamertag->second.isString())
                                {
                                    gamertag = itGamertag->second.cast<std::string>();
                                    gamerTagsToConvert.erase(gamertag);

                                    EntryMap::const_iterator it = entryMap.find(gamertag);
                                    if( it != entryMap.end() )
                                    {
                                        Entry* entry = it->second;
                                        if (!itUserId->second.isNumber())
                                        {
                                            entry->state.store(UnknownUser);
                                        }
                                        else
                                        {
                                            entry->robloxUid = itUserId->second.cast<int>();
                                            entry->state.store(Ready);
                                        }
                                    }
                                }
                            }
                        }  
                    }

                    // set values of gamer tags that were expected to come, but was not send or received
                    for(boost::unordered_set<std::string>::const_iterator inputIt = gamerTagsToConvert.begin(); inputIt != gamerTagsToConvert.end(); ++inputIt)
                    {
                        entryMap[*inputIt]->state.store(Failed);
                    }

                }, false);
        }
    }


    void UserIDTranslator::waitForUIDs( const std::vector< const UserIDTranslator::Entry* >& uids )
    {
        for (int k=0;;k=0)
        {
            for (int j=0; j<uids.size(); ++j)
            {
                auto st = uids[j]->getState();
                k +=  st != UserIDTranslator::Waiting;
            }

            if( k == uids.size() ) return;
            Sleep(5);
        }
    }
