#pragma once
#include "Bootstrapper.h"

class BootstrapperRccService:public Bootstrapper
{
private:
	std::wstring _regPath;
	std::wstring _regSubPath;
	std::wstring _versionFileName;
	std::wstring _versionGuidName;
	std::wstring _emptyString;

	virtual std::string Type() { return "Rcc"; };

	virtual std::wstring GetRegistryPath() const;
	virtual std::wstring GetRegistrySubPath() const;
	virtual const wchar_t* GetVersionFileName() const;
	virtual const wchar_t* GetGuidName() const;
	virtual const wchar_t* GetStartAppMutexName() const;
	virtual const wchar_t* GetBootstrapperMutexName() const;
    virtual const wchar_t* GetFriendlyName() const;
	virtual const wchar_t* GetProtocolHandlerUrlScheme() const { return _T(""); };

public:
	BootstrapperRccService(HINSTANCE hInstance);
	/*override*/ bool ProcessArg(wchar_t** args, int &pos, int count);
	/*override*/ std::wstring programDirectory() const;
	/*override*/ bool EarlyInstall(bool isUninstall);

protected:
	GUID serviceGuid;

	void installService();
	void uninstallService();
};

