#pragma once
#include "Bootstrapper.h"
#include "CountersClient.h"
#include "WindowsStudioInstallerSettings.h"
#include "SharedLauncher.h"

class BootstrapperQTStudio:
	public Bootstrapper
{
private:
	
    SharedLauncher::EditArgs editArgs;

	CEvent syncGameStartEvent;

	std::wstring _versionFileName;
	std::wstring _versionGuidName;

	std::wstring _protocolHandlerScheme;

	WindowsStudioInstallerSettings settings;
	boost::scoped_ptr<CountersClient> counters;

	void registerFileTypes();
	void unregisterFileTypes();
	void unregisterFileTypes(HKEY ckey);

	void deleteLegacyShortcuts();
	bool hasLegacyStudioDesktopShortcut();

	virtual std::string Type() { return "QTStudio"; };
	virtual std::string BinaryType() { return "WindowsStudio"; }
	virtual void initialize();
	virtual void createDialog();
	virtual void DoInstallApp();
	virtual void DoUninstallApp(CRegKey &hk);

	virtual std::wstring GetRegistryPath() const;
	virtual std::wstring GetRegistrySubPath() const;
	virtual const wchar_t* GetProtocolHandlerUrlScheme() const;
	virtual const wchar_t *GetVersionFileName() const;
	virtual const wchar_t *GetGuidName() const;
	virtual std::wstring GetBootstrapperFileName() const;
	virtual std::wstring GetRobloxAppFileName() const;
	virtual std::wstring GetProductCodeKey() const;
	virtual const wchar_t* GetStartAppMutexName() const;
	virtual const wchar_t* GetBootstrapperMutexName() const;
    virtual const wchar_t* GetFriendlyName() const;

	virtual void DeployComponents(bool isUpdating, bool commitData);
	virtual bool ProcessArg(wchar_t** args, int &pos, int count);
	virtual bool ProcessProtocolHandlerArgs(const std::map<std::wstring, std::wstring>& argMap);

	virtual bool GetLogChromeProtocolFix() { return settings.GetValueLogChromeProtocolFix(); }
protected:
	virtual void LoadSettings();

	virtual std::string InfluxDbUrl() { return settings.GetValueInfluxUrl(); }
	virtual std::string InfluxDbDatabase() { return settings.GetValueInfluxDatabase(); }
	virtual std::string InfluxDbUser() { return settings.GetValueInfluxUser(); }
	virtual std::string InfluxDbPassword() { return settings.GetValueInfluxPassword(); }
	virtual int InfluxDbReportHundredthsPercentage() { return settings.GetValueInfluxHundredthsPercentage(); }
	virtual bool GetUseNewVersionFetch() { return settings.GetValueUseNewVersionFetch(); }
	virtual bool GetUseDataDomain() { return settings.GetValueUseDataDomain(); }
	virtual bool CreateEdgeRegistry() { return settings.GetValueCreateEdgeRegistry(); }
	virtual bool DeleteEdgeRegistry() { return settings.GetValueDeleteEdgeRegistry(); }

public:
	BootstrapperQTStudio(HINSTANCE hInstance);

	virtual void RegisterEvent(const TCHAR *eventName);
	virtual void FlushEvents();
	virtual bool PerModeLoggingEnabled();

	virtual void StartRobloxApp(bool fromInstall);
};
