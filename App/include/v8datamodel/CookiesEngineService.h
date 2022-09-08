#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX
{
	extern const char* const sCookiesService;
	class CookiesService
		: public DescribedNonCreatable<CookiesService, Instance, sCookiesService>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<CookiesService, Instance, sCookiesService> Super;
		std::string path;
	public:
		CookiesService();

		void SetValue(std::string key, std::string value);
		std::string GetValue(std::string key);
		void DeleteValue(std::string key);
	};
}