#include "network/WebChatFilter.h"

#include <algorithm>

#include "RobloxServicesTools.h"

#include "rbx/rbxTime.h"
#include "v8datamodel/Stats.h"
#include "V8DataModel/DataModel.h"
#include "util/Guid.h"
#include "util/Http.h"
#include "util/HttpAsync.h"
#include "util/standardout.h"
#include "reflection/Type.h"
#include "util/Statistics.h"
#include "util/RobloxGoogleAnalytics.h"

#include <boost/lexical_cast.hpp>
#include <boost/thread/once.hpp>
#include <rapidjson/document.h>

DYNAMIC_LOGVARIABLE(WebChatFiltering, 0)

DYNAMIC_FASTINTVARIABLE(WebChatFilterHttpTimeoutSeconds, 60)
DYNAMIC_FASTFLAGVARIABLE(UseComSiftUpdatedWebChatFilterParamsAndHeader, true)
DYNAMIC_FASTFLAGVARIABLE(ConstructModerationFilterTextParamsAndHeadersUseLegacyFilterParams, true)

using namespace RBX;
using namespace RBX::Network;

namespace
{
static Timer<RBX::Time::Precise> lastFailureTime;

void initFailureTimer()
{
	lastFailureTime.reset();
}

inline void logContent(const std::string &varName, const std::string &msg)
{
	if (DFLog::WebChatFiltering)
	{
		std::stringstream ss;
		ss << varName << " (" << msg.length() << " bytes) " << msg;
		FASTLOGS(DFLog::WebChatFiltering, "%s", ss.str().c_str());
	}
}

inline void logResponseData(
	const std::string& unfiltered,
	const ChatFilter::Result& result,
	const RBX::Time::Interval& webChatFilterResponseDelta)
{
    FASTLOG1F(DFLog::WebChatFiltering, "ResponseLatencyMillis: %f", webChatFilterResponseDelta.msec());
    logContent("FilteredContent (blacklist)", result.blacklistFilteredMessage);
    logContent("FilteredContent (whitelist)", result.whitelistFilteredMessage);
}
} // namespace

void filterMessageHelper(
	const std::string message,
	const RBX::Network::ChatFilter::FilteredChatMessageCallback filteredCallback,
	shared_ptr<RBX::Network::Player> playerFilter)
{
	logContent("Unfiltered", message);

	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(&initFailureTimer, flag);
	}

	std::stringstream params;
	RBX::HttpAux::AdditionalHeaders headers;
	if (DFFlag::UseComSiftUpdatedWebChatFilterParamsAndHeader)
	{
		DataModel* dataModel = DataModel::get(playerFilter.get());

		ConstructModerationFilterTextParamsAndHeaders(
			message,
			playerFilter->getUserID(), 
			dataModel->getPlaceID(), 
			dataModel->getGameInstanceID(), 
			/*out*/ params, 
			/*out*/ headers);
	}
	else
	{
		params << "filters=" << kWebChatWhiteListPolicy;
		params << "&filters=" << kWebChatBlackListPolicy;
		params << "&text=" << Http::urlEncode(message);
	}


	RBX::Timer<RBX::Time::Precise> timer;

	std::string response;
	Time::Interval responseDelta = timer.delta();
	std::string failureReason;

	ChatFilter::Result result;
	try
	{
        Http http(GetWebChatFilterURL(GetBaseURL()));
        http.setResponseTimeout(DFInt::WebChatFilterHttpTimeoutSeconds * 1000);
		if (DFFlag::UseComSiftUpdatedWebChatFilterParamsAndHeader)
		{
			http.applyAdditionalHeaders(headers);
		}
        http.post(params, Http::kContentTypeUrlEncoded, false, response);

        using namespace rapidjson;
        Document doc;
        doc.Parse<kParseDefaultFlags>(response.c_str());

        RBXASSERT(doc.HasMember("data"));
        Value& data = doc["data"];
        RBXASSERT(data.HasMember(kWebChatWhiteListPolicy.c_str()));
        RBXASSERT(data.HasMember(kWebChatBlackListPolicy.c_str()));

        result.whitelistFilteredMessage = data[kWebChatWhiteListPolicy.c_str()].GetString();
        result.blacklistFilteredMessage = data[kWebChatBlackListPolicy.c_str()].GetString();

        filteredCallback(result);
	}
	catch (const std::exception& e)
	{
		FASTLOGS(DFLog::WebChatFiltering, "Web chat failed: %s", e.what());
        failureReason = e.what();
        RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ChatFailure", failureReason.c_str());
	}

	logResponseData(message, result, responseDelta);
}

void WebChatFilter::filterMessage(
	shared_ptr<RBX::Network::Player> sender,
	shared_ptr<RBX::Instance> receiver,
	const std::string& message,
	const RBX::Network::ChatFilter::FilteredChatMessageCallback filteredCallback)
{
    if (filterMessageBase(sender, receiver, message, filteredCallback))
    {
        return;
    }

	RBXASSERT(sender);
	boost::function0<void> f = boost::bind(&filterMessageHelper, message, filteredCallback, sender);
	boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, RBX::MESSAGE_ERROR, false);
	boost::thread(thread_wrapper(g, "rbx_webChatFilterHttpPost"));
}

void RBX::Network::ConstructModerationFilterTextParamsAndHeaders(
	std::string text,
	int userID,
	int placeID,
	std::string gameInstanceID,

	std::stringstream &outParams,
	RBX::HttpAux::AdditionalHeaders &outHeaders
)
{
    outParams << "text=" << Http::urlEncode(text);
    if (DFFlag::ConstructModerationFilterTextParamsAndHeadersUseLegacyFilterParams) {
        outParams << "&filters=" << kWebChatWhiteListPolicy;
        outParams << "&filters=" << kWebChatBlackListPolicy;
    }
	outParams << "&userId=" << userID;

    outHeaders["placeId"] = boost::lexical_cast<std::string>(placeID);
	outHeaders["gameInstanceID"] = gameInstanceID;
}

