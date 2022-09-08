#include "stdafx.h"

#include "V8DataModel/HttpService.h"
#include "V8DataModel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Stats.h"
#include "V8Xml/WebParser.h"
#include "Network/Players.h"
#include "Util/Http.h"
#include "Util/RobloxGoogleAnalytics.h"

DYNAMIC_FASTINTVARIABLE(UserHttpRequestsPerMinuteLimit, 500)

namespace {
	static inline void sendHttpServiceStats(int placeId)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpService", RBX::StringConverter<int>::convertToString(placeId).c_str());
	}
}

namespace RBX {
	const char* const sHttpService = "HttpService";

    REFLECTION_BEGIN();
	static Reflection::BoundYieldFuncDesc<HttpService, std::string(std::string, bool)>  userHttpGetAsyncFunction(&HttpService::userHttpGetAsync, "GetAsync", "url", "nocache", false, Security::None);
	static Reflection::BoundYieldFuncDesc<HttpService, std::string(std::string, std::string, HttpService::HttpContentType, bool)>  userHttpPostAsyncFunction(&HttpService::userHttpPostAsync, "PostAsync", "url", "data", "content_type", HttpService::APPLICATION_JSON, "compress", false, Security::None);
	static Reflection::BoundFuncDesc<HttpService, Reflection::Variant(std::string)> decodeJSON(&HttpService::decodeJSON, "JSONDecode", "input", Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string(Reflection::Variant)> encodeJSON(&HttpService::encodeJSON, "JSONEncode", "input", Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string(std::string)> urlEncode(&HttpService::urlEncode, "UrlEncode", "input", Security::None);
	static Reflection::BoundFuncDesc<HttpService, std::string(bool)> generateGuid(&HttpService::generateGuid, "GenerateGUID", "wrapInCurlyBraces", true, Security::None);

	Reflection::BoundProp<bool> HttpService::prop_httpEnabled("HttpEnabled", category_Data, &HttpService::httpEnabled, Reflection::PropertyDescriptor::STANDARD, Security::LocalUser);
    REFLECTION_END();

	namespace Reflection {
		template<>
		EnumDesc<HttpService::HttpContentType>::EnumDesc()
			:EnumDescriptor("HttpContentType")
		{
			addPair(HttpService::APPLICATION_JSON,			"ApplicationJson");		
			addPair(HttpService::APPLICATION_XML,			"ApplicationXml");		
			addPair(HttpService::APPLICATION_URLENCODED,	"ApplicationUrlEncoded");	
			addPair(HttpService::TEXT_PLAIN,				"TextPlain");
			addPair(HttpService::TEXT_XML,					"TextXml");
		}

		template<>
		HttpService::HttpContentType& Variant::convert<HttpService::HttpContentType>(void)
		{
			return genericConvert<HttpService::HttpContentType>();
		}
	}//namespace Reflection

	template<>
	bool StringConverter<HttpService::HttpContentType>::convertToValue(const std::string& text, HttpService::HttpContentType& value)
	{
		return Reflection::EnumDesc<HttpService::HttpContentType>::singleton().convertToValue(text.c_str(),value);
	}

	HttpService::HttpService() :
		httpEnabled(false),
		throttle(&DFInt::UserHttpRequestsPerMinuteLimit)
	{
		setName(sHttpService);
	}

	bool HttpService::checkEverything(std::string& url, boost::function<void(std::string)> errorFunction)
	{
		if (url.size() == 0)
		{
			errorFunction("Empty URL");
			return false;
		}

		if (!httpEnabled)
		{
			errorFunction("Http requests are not enabled");
			return false;
		}

		if (!Network::Players::backendProcessing(this))
		{
			errorFunction("Http requests can only be executed by game server");
			return false;
		}

		if (Instance::fastDynamicCast<DataModel>(getParent()) == NULL)
		{
			errorFunction("Unrecognized HttpService");
			return false;
		}

		if (!throttle.checkLimit())
		{
			errorFunction("Number of requests exceeded limit");
			return false;
		}

		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			DataModel* dm = DataModel::get(this);
			boost::call_once(flag, boost::bind(&sendHttpServiceStats,dm->getPlaceID()));
		}

		return true;
	}

	void HttpService::addIdHeader(Http& request)
	{
		DataModel* dm = DataModel::get(this);
		request.additionalHeaders["Roblox-Id"] = RBX::StringConverter<int>::convertToString(dm->getPlaceID());
	}

	void HttpService::userHttpPostAsync(std::string url, std::string data, HttpContentType contentType, bool compress, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!checkEverything(url, errorFunction))
			return;
		 
		Http http(url);
		std::string contentTypeName;
		switch(contentType)
		{
		case APPLICATION_JSON: contentTypeName = Http::kContentTypeApplicationJson; break;
		case APPLICATION_XML: contentTypeName = Http::kContentTypeApplicationXml; break;
		case APPLICATION_URLENCODED: contentTypeName = Http::kContentTypeUrlEncoded; break;
		case TEXT_PLAIN: contentTypeName = Http::kContentTypeTextPlain; break;
		case TEXT_XML: contentTypeName = Http::kContentTypeTextXml; break;
		default:
			errorFunction("Unsupported content type");
			return; 
		}

		if(data.length() == 0)
			data = " ";
		addIdHeader(http);
		http.post(data, contentTypeName, compress, boost::bind(&DataModel::HttpHelper, _1, _2, resumeFunction, errorFunction), true);
	}

	void HttpService::userHttpGetAsync(std::string url, bool nocache, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!checkEverything(url, errorFunction))
			return;

		Http http(url);
		if(nocache)
		{
			http.additionalHeaders["Cache-Control"] = "no-cache";
			http.doNotUseCachedResponse = true;
		}

		addIdHeader(http);
		http.get(boost::bind(&DataModel::HttpHelper, _1, _2, resumeFunction, errorFunction), true);
	}

	Reflection::Variant HttpService::decodeJSON(std::string input)
	{
		Reflection::Variant value;
		if(WebParser::parseJSONObject(input, value))
		{
			return value;
		}
		else
		{
			throw std::runtime_error("Can't parse JSON");
		}
	}

	std::string HttpService::encodeJSON(Reflection::Variant obj)
	{
		std::string result;

		if(obj.isType<shared_ptr<const Reflection::ValueTable> >() || obj.isType<shared_ptr<const Reflection::ValueArray> >())
		{
			if(!WebParser::writeJSON(obj, result))
			{
				throw std::runtime_error("Can't convert to JSON");
			}
		} else
			throw std::runtime_error("Can't convert to JSON");

		return result;
	}

	std::string HttpService::urlEncode(std::string data)
	{
		return Http::urlEncode(data);
	}

	std::string HttpService::generateGuid(bool wrapInCurlyBraces)
	{
		std::string result;
		Guid::generateStandardGUID(result);

		if (!wrapInCurlyBraces)
		{
			RBXASSERT(result.size() > 2);
			result = result.substr(1, result.size() - 2);
		}

		return result;
	}
}
