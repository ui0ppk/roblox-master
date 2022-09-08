#include "ChatFilter.h"

#include "Util/http.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

DYNAMIC_LOGGROUP(WebChatFiltering)

using namespace RBX;
using namespace RBX::Network;

const char* const RBX::Network::sChatFilter = "ChatFilter";

bool ChatFilter::filterMessageBase(shared_ptr<Player> sourcePlayer, shared_ptr<Instance> receiver,
		const std::string& message,
		const ChatFilter::FilteredChatMessageCallback callback) {

	// Safe chats (chats from fixed safe chat list) are chatted as "/sc [0-9]+".
	// Always pass safe chats along without filtering.
	if (boost::starts_with(message, "/sc ")) {
		Result result;
		result.whitelistFilteredMessage = message;
		result.blacklistFilteredMessage = message;
		boost::thread t(boost::bind(callback, result));
		return true;
	}

    return false;
}
