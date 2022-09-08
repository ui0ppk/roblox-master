#pragma once

#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"

namespace RBX {
	class PartInstance;

	extern const char *const sChatService ;

	class ChatService 
		: public DescribedCreatable<ChatService, Instance, sChatService, Reflection::ClassDescriptor::INTERNAL>
		, public Service
	{
	private:
		void gotFilteredStringSuccess(std::string response, Network::Player* player, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void gotFilterStringError(std::string error, boost::function<void(std::string)> errorFunction);
	public:
		enum ChatColor
		{
			CHAT_BLUE,
			CHAT_GREEN,
			CHAT_RED
		};

		ChatService();

		void chat(shared_ptr<Instance> instance, std::string message, ChatService::ChatColor chatColor);

		void filterStringForPlayer(std::string stringToFilter, shared_ptr<Instance> playerToFilterFor, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		rbx::remote_signal<void(shared_ptr<Instance>, std::string, ChatService::ChatColor)> chattedSignal;
	};
}