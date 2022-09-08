#pragma once 
#include "Reflection/Reflection.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "rbx/RunningAverage.h"


namespace RBX {
	class Http;

	extern const char* const sHttpService;
	class HttpService
		:public DescribedCreatable<HttpService, Instance, sHttpService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
		,public Service
	{
		ThrottlingHelper throttle;

		bool httpEnabled;
		static Reflection::BoundProp<bool> prop_httpEnabled;
		bool checkEverything(std::string& url, boost::function<void(std::string)> errorFunction);

		void addIdHeader(Http& request);

	public:	
		HttpService();

		enum HttpContentType
		{	
			APPLICATION_JSON = 0,
			APPLICATION_XML = 1,
			APPLICATION_URLENCODED = 2,
			TEXT_PLAIN = 3, 
			TEXT_XML = 4, 
		};


		void userHttpGetAsync(std::string url, bool noCache, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void userHttpPostAsync(std::string url, std::string data, HttpContentType content, bool compress, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		Reflection::Variant decodeJSON(std::string input);
		std::string encodeJSON(Reflection::Variant obj);
		std::string urlEncode(std::string data);
		std::string generateGuid(bool wrapInCurlyBraces);
	};
}
