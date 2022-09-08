#pragma once

#include <boost/function.hpp>
#include "format_string.h"
#include "BootstrapperSite.h"

namespace HttpTools
{
	static int httpboostPostTimeout = 0;

	std::string httpGetString(const std::string& url);

	int httpGet(IInstallerSite *site, std::string host, std::string path, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log = true);
	int httpPost(IInstallerSite *site, std::string host, std::string path, std::istream& input, const char* contentType, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log = true);
	int httpGetCdn(IInstallerSite *site, std::string secondaryHost, std::string path, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress);
	const std::string getPrimaryCdnHost(IInstallerSite *site);
	const std::string getCdnHost(IInstallerSite *site);
}
