#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
namespace RBX {

	extern const char* const sContentFilter;
	class ContentFilter
		: public DescribedNonCreatable<ContentFilter, Instance, sContentFilter, Reflection::ClassDescriptor::RUNTIME_LOCAL>
		, public Service

	{
	public:
		typedef enum { Waiting, Succeeded, Failed } FilterResult;
		static const unsigned MAX_CONTENT_FILTER_SIZE;
	private:
		struct ResultEntry
		{
			bool result;
			int usageCount;
			ResultEntry(bool result=false)
				:result(result),usageCount(0)
			{}
		};
		typedef std::map<std::string, ResultEntry> ResultsDictionary;
		typedef std::set<std::string> RequestSet;

		ResultsDictionary resultsDictionary;
		RequestSet requestSet;

		std::string url;
		unsigned maxOutstandingRequests;
		unsigned maxTableSize;

		static void truncateString(std::string& text);

		//Returns false if it doesn't know yet, may truncate the string
		bool isContentFilterReady(const std::string& value);
		bool isStringSafe(std::string& value);

		void cleanTable();
	public:
		ContentFilter();
		~ContentFilter();


		FilterResult getStringState(std::string& value);
		
		void setFilterUrl(std::string);
		void setFilterLimits(int,int);

		void doFilterRequest(std::string value);
		void saveFilterResult(std::string value, bool result);
	};

}
