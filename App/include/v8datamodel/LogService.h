/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

#include <boost/circular_buffer.hpp>

namespace RBX
{

    namespace Network
    {
        class Player;
    }

    extern const char *const sLogService;

    class LogService
        : public DescribedCreatable<LogService, Instance, sLogService, Reflection::ClassDescriptor::INTERNAL>
        , public Service
    {
    public:
        LogService();

        void requestServerOutput();
		void executeScript(std::string source);
        shared_ptr<const Reflection::ValueArray> getLogHistory();

        rbx::signal<void(std::string, MessageType)> outputMessageSignal;
        rbx::remote_signal<void(shared_ptr<Instance>)> requestServerOutputSignal;
        rbx::remote_signal<void(std::string, MessageType, int)> serverOutputMessageSignal;
		rbx::remote_signal<void(shared_ptr<Instance>, std::string)> requestScriptExecutionSignal;
        /*override*/ void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source);
		void runCallbackIfPlayerHasConsoleAccess(shared_ptr<Network::Player> player, boost::function<void()> callback);

        static int filterSensitiveKeys(std::string& text);

    protected:
        virtual void onServiceProvider(ServiceProvider* oldSp, ServiceProvider* newSp);

    private:
        void onMessageOut(const StandardOutMessage& message);
        static void doFireEvent(StandardOutMessage message, weak_ptr<LogService> logService);

		static void handleCanManageResponse(boost::function<void()> callback, weak_ptr<LogService> weakLogService,
			std::string* responseString, std::exception* exception);

        static void maybeConnectPlayerToServerLogs(weak_ptr<LogService> weakLogService, shared_ptr<Instance> instance);
		static void connectPlayerToServerLogs(weak_ptr<LogService> weakLogService, weak_ptr<Network::Player> weakPlayer);

		void maybeExecuteServerScript(shared_ptr<Instance> requestingPlayer, std::string source);
		static void executeServerScript(weak_ptr<LogService> weakLogService, std::string source);

        bool currentlyFiringEvent;
        rbx::signals::scoped_connection messageOutConnection;
        boost::circular_buffer_space_optimized<StandardOutMessage> logHistory;
        rbx::signals::scoped_connection listenForServerLogRequestConnection;
        weak_ptr<Network::Player> playerReceivingServerLogs;
		std::list<weak_ptr<Network::Player> > playersReceivingServerLogs;
    };
}