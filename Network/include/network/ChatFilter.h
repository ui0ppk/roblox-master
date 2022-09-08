#pragma once

#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"
#include "Player.h"

namespace RBX {
	namespace Network {

	class CustomChatFilter;

	extern const char *const sChatFilter;
	class ChatFilter 
		: public DescribedNonCreatable<ChatFilter, Instance, sChatFilter, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	public:
		struct Result {
			std::string whitelistFilteredMessage;
			std::string blacklistFilteredMessage;
		};
		typedef boost::function<void(const Result&)> FilteredChatMessageCallback;

        ChatFilter() {}

        bool filterMessageBase(shared_ptr<Player> sourcePlayer, shared_ptr<Instance> receiver,
            const std::string& message, const FilteredChatMessageCallback callback);

		virtual void filterMessage(shared_ptr<Player> sourcePlayer, shared_ptr<Instance> receiver,
			const std::string& message, const FilteredChatMessageCallback callback) = 0;
	};
}}
