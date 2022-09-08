#pragma once

#include "network/ChatFilter.h"
#include "Network/Player.h"
#include "Util/Http.h"
#include "util/HttpAux.h"

#include <boost/shared_ptr.hpp>

static const std::string kWebChatWhiteListPolicy = "white";
static const std::string kWebChatBlackListPolicy = "black";

namespace RBX
{
namespace Network
{

class WebChatFilter : public RBX::Network::ChatFilter
{
public:
	/*override*/
	virtual void filterMessage(
		shared_ptr<RBX::Network::Player> sourcePlayer,
		shared_ptr<RBX::Instance> receiver,
		const std::string& message,
		const RBX::Network::ChatFilter::FilteredChatMessageCallback callback);
}; // class WebChatFilter

void ConstructModerationFilterTextParamsAndHeaders(
	std::string text,
	int userID,
	int placeID,
	std::string gameInstanceID,

	std::stringstream &outParams,
	RBX::HttpAux::AdditionalHeaders &outHeaders
);

} // namespace Network
} // namespace RBX

