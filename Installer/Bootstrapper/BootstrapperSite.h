#pragma once

#include <string>
#include "format_string.h"

class IInstallerSite
{
public:
	virtual std::string BaseHost() = 0;
	virtual std::string InstallHost() = 0;
	virtual std::string InstallVersion() = 0;
	virtual simple_logger<wchar_t> &Logger() = 0;
	virtual void progressBar(int pos, int max) = 0;
	virtual void CheckCancel() = 0;
	virtual void Error(const std::exception& e) = 0;
	virtual std::wstring ProgramDirectory() = 0;
	virtual bool UseNewCdn() { return false; }
	virtual bool ReplaceCdnTxt() { return false; }
};
