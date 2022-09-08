#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX {

	extern const char* const sGamePassService;
	class GamePassService
		: public DescribedNonCreatable<GamePassService, Instance, sGamePassService>
		, public Service

	{
	public:
		GamePassService();

		void setPlayerHasPassUrl(std::string);

		void playerHasPass(shared_ptr<Instance> playerInstance, int gamePassId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		
	private:
		template<typename ResultType>
		void dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction);
		std::string playerHasPassUrl;
	};

}