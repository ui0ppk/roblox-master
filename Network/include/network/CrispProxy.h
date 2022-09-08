#pragma once

#include <string>
#include <map>
#include <vector>

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

namespace Crisp {
	namespace RMF3 {
		typedef std::map<std::string, std::string> Data;
	}
}

namespace RBX {
	// This very closely mirrors Crisp::RMF3::Response, except we expose the fields directly
	// and remove the async-specific bits.
	struct CrispResponse {
		const std::string                                    id;
		const int                                            result;
		const std::string                                    originalContent;
		const std::string                                    filteredContent;
		const int                                            errorCode;
		const std::string                                    errorDescription;

		CrispResponse(
			const std::string &id_,
			int result_,
			const std::string &originalContent_,
			const std::string &filteredContent_,
			int errorCode_,
			const std::string &errorDescription_) :
		id(id_), result(result_),
			originalContent(originalContent_), filteredContent(filteredContent_), errorCode(errorCode_),
			errorDescription(errorDescription_)
		{}

		inline bool succeeded() const
		{
			return 0 == errorCode;
		}
	};

	typedef boost::shared_ptr<const CrispResponse> CrispResponsePtr;

// An interface of Crisp::RMF3 services.  The intention is for subclasses
// to implement the behavior so that not all projects need the crisprmf3.dll.
class CrispProxy {
public:
	typedef boost::function<void (CrispResponsePtr)> ResponseHandler;

	virtual boost::thread checkContent(
		const std::string& id,
		const std::string& sender,
		const std::string& receiver,
		const std::string& content,
		const Crisp::RMF3::Data* pData,
		const std::string& policy,
		ResponseHandler &callback) const = 0;

	virtual boost::thread sendEvent(
		const std::string& id,
		const std::string& sender,
		const std::string& receiver,
		const std::string& event,
		const Crisp::RMF3::Data *pData,
		ResponseHandler &callback) const = 0;
};

}