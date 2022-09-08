#pragma once

#include "format_string.h"
#include "Progress.h"
#include "BootstrapperSite.h"

//TODO make this guy like bootstrapper public boost::enable_shared_from_this<Bootstrapper> and use shared pointer
class FileDeployer
{
	IInstallerSite *_site;
	bool _perUser;
	DWORD _bytesDownloaded;
public:
	FileDeployer(IInstallerSite *site, bool perUser);

	void deployVersionedFile(const TCHAR* name, const TCHAR* subfolder, Progress& progress, bool commitData);

	DWORD GetBytesDownloaded() const { return _bytesDownloaded; }
private:
	std::wstring downloadVersionedFile(const TCHAR* name, Progress& progress, bool usePrimaryCdn);
	void cleanupVersionedFile(const std::wstring& name, const std::wstring& path, const std::wstring& fileName);
	void deployVersionedFileInternal(const TCHAR* name, const TCHAR* subfolder, Progress& progress, bool usePrimaryCdn,
		/*output*/ std::wstring& path, /*output*/ std::wstring& fileName, bool commitData);
};