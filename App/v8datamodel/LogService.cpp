/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/LogService.h"

#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/LuaWebService.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/ScopedAssign.h"
#include "script/ScriptContext.h"
#include "V8DataModel/Remote.h"
#include "util/Analytics.h"
#include "RobloxServicesTools.h"

#include <iostream>

DYNAMIC_FASTINTVARIABLE(MaxLogHistory, 512)
DYNAMIC_FASTINTVARIABLE(BadLogInfluxHundredthsPercentage, 0)
DYNAMIC_FASTINTVARIABLE(BadLogMask, 0)

// Do not remove the following DFFlag.  It is used in cases where an exploit gains access to devconsole.
DYNAMIC_FASTFLAGVARIABLE(DebugDisableLogServiceExecuteScript, false)

namespace {
// can also consider adding:
//    "rbxcdn%2ecom%2f", // url-encoded case
//    "hash=" // asset hash request direct form
static const size_t kNumWords = 3;
static const std::string kStartWords[kNumWords] = {
    "apikey=", // secret key param
    "accesskey=", // secret key param
    "rbxcdn.com/" // avoids asset/place stealing
    };
}

namespace RBX {

    const char *const sLogService = "LogService";

    REFLECTION_BEGIN();
    // Methods
    static Reflection::BoundFuncDesc<LogService, shared_ptr<const Reflection::ValueArray>() > func_GetLogHistory(
        &LogService::getLogHistory, "GetLogHistory", Security::None);
    static Reflection::BoundFuncDesc<LogService, void()> func_RequestServerOutput(
        &LogService::requestServerOutput, "RequestServerOutput", Security::RobloxScript);
	static Reflection::BoundFuncDesc<LogService, void(std::string)> func_executeScript(
		&LogService::executeScript, "ExecuteScript", "source", Security::RobloxScript);
    
    // Events and properties
    static Reflection::EventDesc<LogService, void(std::string, MessageType)> event_OutputMessage(
        &LogService::outputMessageSignal, "MessageOut", "message", "messageType");
    static Reflection::RemoteEventDesc<LogService, void(std::string, MessageType, int)> event_ServerOutputMessage(
        &LogService::serverOutputMessageSignal, "ServerMessageOut", "message", "messageType", "timestamp",
        Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
    static Reflection::RemoteEventDesc<LogService, void(shared_ptr<Instance>)> event_RequestServerOutputSignal(
        &LogService::requestServerOutputSignal, "RequestServerOutputSignal", "requestingPlayer",
        Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::RemoteEventDesc<LogService, void(shared_ptr<Instance>, std::string)> event_requestScriptExecutionSignal(
		&LogService::requestScriptExecutionSignal, "RequestScriptExecutionSignal", "requestingPlayer", "source",
		Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
    REFLECTION_END();

    static void raiseServerEventFromMessage(shared_ptr<LogService> logService,
        const StandardOutMessage& message, const SystemAddress& systemAddress)
    {
        Reflection::EventArguments args(3);
        
        args[0] = message.message;
        args[1] = message.type;
        args[2] = static_cast<int>(message.time);

        logService->raiseEventInvocation(event_ServerOutputMessage, args, &systemAddress);
    }

    LogService::LogService()
        : currentlyFiringEvent(false)
        , logHistory(DFInt::MaxLogHistory)
    {
        setName("LogService");

		messageOutConnection = StandardOut::singleton()->messageOut.connect(
			boost::bind(&LogService::onMessageOut, this, _1));
    }

    void LogService::requestServerOutput()
    {
        if (Network::Players::clientIsPresent(this))
        {
            if (Instance* player = Network::Players::findLocalPlayer(this))
            {
                Reflection::EventArguments args(1);
                args[0] = shared_from(player);
                raiseEventInvocation(event_RequestServerOutputSignal, args, NULL);
            }
        }
    }

    shared_ptr<const Reflection::ValueArray> LogService::getLogHistory()
    {
        RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "GetLogHistory", 
            format("%d", DataModel::get(this)->getPlaceID()).c_str());

        shared_ptr<Reflection::ValueArray> result = rbx::make_shared<Reflection::ValueArray>();

        for (size_t i = 0; i < logHistory.size(); ++i)
        {
            shared_ptr<Reflection::ValueTable> logTable = rbx::make_shared<Reflection::ValueTable>();
            (*logTable)["timestamp"] = static_cast<int>(logHistory[i].time);
            (*logTable)["message"] = logHistory[i].message;
            (*logTable)["messageType"] = logHistory[i].type;
            result->push_back(shared_ptr<const Reflection::ValueTable>(logTable));
        }

        return result;
    }

    void LogService::onServiceProvider(ServiceProvider* oldSp, ServiceProvider* newSp)
    {
        if (newSp)
        {
            listenForServerLogRequestConnection = requestServerOutputSignal.connect(
                boost::bind(&LogService::maybeConnectPlayerToServerLogs, weak_from(this), _1));
			
			requestScriptExecutionSignal.connect(boost::bind(&LogService::maybeExecuteServerScript, this, _1, _2));
        }
    }

	void LogService::executeScript(std::string source) {
		if (Network::Players::clientIsPresent(this))
		{
			if (Network::Player* player = Network::Players::findLocalPlayer(this))
			{
				Reflection::EventArguments args(2);
				Instance* instance = player;
				args[0] = shared_from(instance);
				args[1] = source;
				raiseEventInvocation(event_requestScriptExecutionSignal, args, NULL);
			}
		}
	}

    void LogService::processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source)
    {
        if ((&descriptor == &event_requestScriptExecutionSignal) || (&descriptor == &event_RequestServerOutputSignal))
        {
            shared_ptr<Instance> player = args[0].cast<shared_ptr<Instance> >();
            
            // Validate player object; ignore invalid calls for security reasons
            if (isPlayerValid(player.get(), source))
            {
                descriptor.fireEvent(this, args);
            }
        }
        else
        {
            descriptor.fireEvent(this, args);
        }
    }

    void LogService::onMessageOut(const StandardOutMessage& message)
    {
        if (currentlyFiringEvent || message.type == MESSAGE_SENSITIVE)
            return;

        if (DataModel* dm = DataModel::get(this))
            dm->submitTask(boost::bind(&LogService::doFireEvent, message, weak_from(this)), DataModelJob::Write);
    }


    int LogService::filterSensitiveKeys(std::string& text)
    {
        int changeMask = 0;
        std::string::size_type offset = 0;
        std::string::size_type offsetSaved = 0;

        // prefilter the string and exit if http and (roblox or rbxcdn) is not found.
        std::string lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), tolower);
        const std::string& lower = lowerText;

        offset = lower.find("http", offset);
        if (offset == std::string::npos)
            return changeMask;

        offsetSaved = offset;
        offset = lower.find("roblox", offset);
        if (offset == std::string::npos)
        {
            offset = lower.find("rbxcdn", offsetSaved);
            if (offset == std::string::npos)
                return changeMask;
        }

        std::string::size_type keyPos[kNumWords] = {};
        size_t amountRedacted = 0;
        while (offset < lower.length())
        {
            // It is unlikely multiple keywords will be in the same string.
            // This caches the find result to avoid needless rescanning of the input.
            std::string::size_type bestKeyPos = std::string::npos;
            size_t bestKeyIdx = 0;
            for (size_t i = 0; i < kNumWords; ++i)
            {
                if (keyPos[i] < offset && keyPos[i] != std::string::npos)
                {
                    keyPos[i] = lower.find(kStartWords[i], offset);
                }
                if (bestKeyPos == std::string::npos || (keyPos[i] < bestKeyPos && keyPos[i] != std::string::npos))
                {
                    bestKeyPos = keyPos[i];
                    bestKeyIdx = i;
                }
            }

            if (bestKeyPos == std::string::npos)
                return changeMask;

            // Remove the text after the keyword.  The lowercase version is not modified.
            // The amount removed is used to get the corresponding indicies in the original.
            std::string::size_type keyBegin = bestKeyPos + kStartWords[bestKeyIdx].size();
            std::string::size_type keyEnd = keyBegin;

            while (keyEnd < lower.length() && (lower[keyEnd] != '&' && lower[keyEnd] != ' ' && lower[keyEnd] != ':'))
                keyEnd++;

            text.erase(text.begin() + (keyBegin - amountRedacted), text.begin() + (keyEnd - amountRedacted));

            offset = keyEnd;
            amountRedacted += keyEnd - keyBegin;
            changeMask |=  (1 << bestKeyIdx);
        }
        return changeMask;
    }

	static bool playerNoLongerActive(weak_ptr<Network::Player> p)
	{
		if (shared_ptr<Network::Player> player = p.lock())
		{
			if (player->getParent() != NULL)
			{
				return false;
			}
		}
		return true;
	}

    void LogService::doFireEvent(StandardOutMessage message, weak_ptr<LogService> logService)
    {
        if (shared_ptr<LogService> sharedLogService = logService.lock())
        {
            ScopedAssign<bool> scopedAssign(sharedLogService->currentlyFiringEvent, true);

			// Attempt to filter messages before we emit them to lua.
			// If the message has something that "looks like" a sensitive api key, filter
			// the part of the message that "looks" sensitive.
            int filterResults = filterSensitiveKeys(message.message);
            if (filterResults & DFInt::BadLogMask)
            {
                // send message to influx
                RBX::Analytics::InfluxDb::Points analyticsPoints;
                analyticsPoints.addPoint("msg", message.message.c_str());
                analyticsPoints.addPoint("mask", filterResults);
                analyticsPoints.report("BadLog", DFInt::BadLogInfluxHundredthsPercentage);
            }

			event_OutputMessage.fireEvent(sharedLogService.get(), message.message.c_str(), message.type);

            if (DFInt::MaxLogHistory != sharedLogService->logHistory.capacity())
            {
                // grow/shrink, keeping the backmost (most recent) elements
                sharedLogService->logHistory.rset_capacity(DFInt::MaxLogHistory);
            }
            sharedLogService->logHistory.push_back(message);

			std::list<weak_ptr<Network::Player> >& playerList = sharedLogService->playersReceivingServerLogs;
			playerList.erase(std::remove_if(playerList.begin(), playerList.end(), &playerNoLongerActive),
				playerList.end());

			for (std::list<weak_ptr<Network::Player> >::iterator itr = playerList.begin();
				itr != playerList.end(); ++itr)
			{
				if (shared_ptr<Network::Player> player = itr->lock())
				{
					raiseServerEventFromMessage(sharedLogService, message,
						player->getRemoteAddressAsRbxAddress());
				}
			}
		}
    }

	void LogService::runCallbackIfPlayerHasConsoleAccess(shared_ptr<Network::Player> p,
		boost::function<void()> callback)
	{
		if (DataModel* dm = DataModel::get(this))
		{
			if (ContentProvider* cp = dm->find<ContentProvider>())
			{
				Http http(format("%s/users/%d/canmanage/%d",
					::trim_trailing_slashes(cp->getApiBaseUrl()).c_str(), p->getUserID(), dm->getPlaceID()));
				http.get(boost::bind(&LogService::handleCanManageResponse,
					callback, weak_from(this), _1, _2));
			}
		}
	}

	static void wrap(DataModel* dm, boost::function<void()> callback)
	{
		callback();
	}

	void LogService::handleCanManageResponse(boost::function<void()> callback,
		weak_ptr<LogService> weakLogService, std::string* responseString, std::exception* exception)
	{
		if (shared_ptr<LogService> logService = weakLogService.lock())
		{
			shared_ptr<const Reflection::ValueTable> valueTable;
			std::string status;
			if (LuaWebService::parseWebJSONResponseHelper(responseString, exception, valueTable, status))
			{
				Reflection::ValueTable::const_iterator canManageItr = valueTable->find("CanManage");
				if (canManageItr != valueTable->end() && canManageItr->second.isType<bool>() &&
					canManageItr->second.cast<bool>())
				{
					if (DataModel* dm = DataModel::get(logService.get()))
					{
						dm->submitTask(boost::bind(&wrap, _1, callback), DataModelJob::Write);
					}
				}
			}
		}
	}

    void LogService::maybeConnectPlayerToServerLogs(weak_ptr<LogService> weakLogService, shared_ptr<Instance> instance)
    {
        if (shared_ptr<LogService> logService = weakLogService.lock())
        {
            if (Network::Players::serverIsPresent(logService.get()))
            {
                if (shared_ptr<Network::Player> p = Instance::fastSharedDynamicCast<Network::Player>(instance))
                {
					logService->runCallbackIfPlayerHasConsoleAccess(p,
						boost::bind(&LogService::connectPlayerToServerLogs, weakLogService,
							weak_ptr<Network::Player>(p)));
                }
            }
        }
    }

	void LogService::connectPlayerToServerLogs(weak_ptr<LogService> weakLogService,
		weak_ptr<Network::Player> weakPlayer)
	{
		if (shared_ptr<LogService> logService = weakLogService.lock())
		{
			if (shared_ptr<Network::Player> p = weakPlayer.lock())
            {
                RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "SendingServerLogHistory", 
                    format("%d", DataModel::get(logService.get())->getPlaceID()).c_str());
                RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CreatorReadingServerLogs",
                    format("%d", p->getUserID()).c_str());

                logService->playerReceivingServerLogs = p;
				logService->playersReceivingServerLogs.push_back(p);

                // send backlog
                const SystemAddress& target = p->getRemoteAddressAsRbxAddress();
                for (size_t i = 0; i < logService->logHistory.size(); ++i)
                {
                    raiseServerEventFromMessage(logService, logService->logHistory[i], target);
                }
            }
        }
    }

	void LogService::maybeExecuteServerScript(shared_ptr<Instance> requestingPlayer, std::string source)
	{
		RBXASSERT(Network::Players::serverIsPresent(this));
		if (Network::Players::serverIsPresent(this))
		{
			if (shared_ptr<Network::Player> p = Instance::fastSharedDynamicCast<Network::Player>(requestingPlayer))
			{
				runCallbackIfPlayerHasConsoleAccess(p,
					boost::bind(&LogService::executeServerScript, weak_from(this), source));
			}
		}
	}

	void LogService::executeServerScript(weak_ptr<LogService> weakLogService, std::string source)
	{
        // Do not remove the following DFFlag.  It is used in cases where an exploit gains access to devconsole.
		if (DFFlag::DebugDisableLogServiceExecuteScript)
			return;

		if (shared_ptr<LogService> logService = weakLogService.lock())
		{
			if (ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(logService.get()))
			{
				StandardOut::singleton()->printf(MESSAGE_OUTPUT, "> %s", source.c_str());
				try
				{
					scriptContext->executeInNewThread(Security::GameScript_,
						ProtectedString::fromTrustedSource(source),
						"console"
					);
				}
				catch (const RBX::base_exception& ex)
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "%s", ex.what());
				}
			}
		}
	}

    namespace Reflection {
        template<>
        EnumDesc<MessageType>::EnumDesc()
            :EnumDescriptor("MessageType")
        {
            addPair(MESSAGE_OUTPUT,         "MessageOutput");
            addPair(MESSAGE_INFO,           "MessageInfo");
            addPair(MESSAGE_WARNING,        "MessageWarning");
            addPair(MESSAGE_ERROR,          "MessageError");
        }

        template<>
        MessageType& Variant::convert<MessageType>(void)
        {
            return genericConvert<MessageType>();
        }
    }

    template<>
    bool StringConverter<MessageType>::convertToValue(const std::string& text, MessageType& value)
    {
        return Reflection::EnumDesc<MessageType>::singleton().convertToValue(text.c_str(),value);
    }
}
