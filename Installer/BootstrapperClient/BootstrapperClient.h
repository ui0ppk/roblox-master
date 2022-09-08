#pragma once
#include "Bootstrapper.h"
#include "ClientInstallerSettings.h"
#include "CountersClient.h"
#include "SharedLauncher.h"


class BootstrapperClient:
	public Bootstrapper
{
private:
	struct PlayArgs
	{
		std::string authUrl;
		std::string authTicket;
		std::string script;
		std::string guidName;
		std::string hiddenStartEventName;
		SharedLauncher::LaunchMode launchMode;
		std::string browserTrackerId;
	};
	boost::scoped_ptr<PlayArgs> playArgs;

	std::wstring _regSubPath;
	std::wstring _regPath;
	std::wstring _versionFileName;
	std::wstring _versionGuidName;

	std::wstring _protocolHandlerScheme;

	int startHour;
	int startYear, startMonth, startDay;
	int endYear, endMonth, endDay;

	ClientInstallerSettings settings;
	boost::scoped_ptr<CountersClient> counters;
	ComModule proxyModule;
	ComModule proxyModule64;

	//install steps of Bootstrapper client
	void DeployBootstrapper();
	void RegisterInstallHost();
	void DeployRobloxProxys();
	void InstallPlayer();

	std::string BuildDateTime(int y, int m, int d);
	HRESULT SheduleTaskWinVista();
	HRESULT SheduleTaskWinXP();
	virtual std::string Type() { return "Client"; };
	virtual std::string BinaryType() { return "WindowsPlayer"; }

	void deployExtraStudioBootstrapper(std::string exeName, TCHAR *linkName, std::wstring componentId, bool forceDesktopIconCreation, const TCHAR *registryPath);
	void deployStudioBetaBootstrapper(bool forceDesktopIconCreation);
	void deployRobloxProxy(bool commitData);
	void deployNPRobloxProxy(bool commitData);
	void registerFirefoxPlugin(const TCHAR* id, bool is64Bits);
	void unregisterFirefoxPlugin(const TCHAR* id, bool is64Bits);

	virtual std::wstring GetRegistryPath() const;
	virtual std::wstring GetRegistrySubPath() const;
	virtual const wchar_t* GetProtocolHandlerUrlScheme() const;
	virtual const wchar_t* GetVersionFileName() const;
	virtual const wchar_t* GetGuidName() const;
	virtual const wchar_t* GetStartAppMutexName() const;
	virtual const wchar_t* GetBootstrapperMutexName() const;
    virtual const wchar_t* GetFriendlyName() const;
	std::string GetExpectedExeVersion()const;

	virtual void DoInstallApp();
	virtual void DoUninstallApp(CRegKey &hk);
	virtual std::wstring GetRobloxAppFileName() const;
	virtual std::wstring GetBootstrapperFileName() const;
	virtual std::wstring GetProductCodeKey() const;
	virtual void DeployComponents(bool isUpdating, bool commitData);
	virtual void StartRobloxApp(bool fromInstall);
	virtual bool IsPlayMode() { return (playArgs != NULL); }
	virtual bool HasUnhideGuid() { return (playArgs != NULL && !playArgs->guidName.empty()); }
	virtual bool ProcessArg(wchar_t** args, int &pos, int count);
	virtual bool ProcessProtocolHandlerArgs(const std::map<std::wstring, std::wstring>& argMap);
protected:
	virtual void initialize();
	virtual void createDialog();

	virtual HRESULT SheduleRobloxUpdater();
	virtual HRESULT UninstallRobloxUpdater();
	
	virtual void SetupGoogleAnalytics();
	virtual bool ValidateInstalledExeVersion();
	virtual bool IsInstalledVersionUptodate();

	virtual std::string InfluxDbUrl() { return settings.GetValueInfluxUrl(); }
	virtual std::string InfluxDbDatabase() { return settings.GetValueInfluxDatabase(); }
	virtual std::string InfluxDbUser() { return settings.GetValueInfluxUser(); }
	virtual std::string InfluxDbPassword() { return settings.GetValueInfluxPassword(); }
	virtual int InfluxDbReportHundredthsPercentage() { return settings.GetValueInfluxHundredthsPercentage(); }
	virtual int InfluxDbInstallHundredthsPercentage() { return settings.GetValueInfluxInstallHundredthsPercentage(); }

	virtual void LoadSettings();
	virtual bool CanRunBgTask() { return settings.GetValueNeedRunBgTask(); }
	virtual bool NeedPreDeployRun();
	virtual bool IsPreDeployOn();
	virtual void RunPreDeploy();
	virtual bool ForceNoDialogMode() { return settings.GetValueForceSilentMode(); }
	virtual bool GetLogChromeProtocolFix() { return settings.GetValueLogChromeProtocolFix(); }
	virtual bool GetUseNewVersionFetch() { return settings.GetValueUseNewVersionFetch(); }
	virtual bool GetUseDataDomain() { return settings.GetValueUseDataDomain(); }
	virtual bool CreateEdgeRegistry() { return settings.GetValueCreateEdgeRegistry(); }
	virtual bool DeleteEdgeRegistry() { return settings.GetValueDeleteEdgeRegistry(); }
	virtual bool ReplaceCdnTxt() { return settings.GetValueReplaceCdnTxt(); }


	bool UpdaterActive() { return settings.GetValueNeedInstallBgTask(); }

    void checkStudioForUninstall(CRegKey &hk);
	void installStudioLauncher(bool isUpdating);
public:
	BootstrapperClient(HINSTANCE hInstance);

	virtual void RegisterEvent(const TCHAR *eventName);
	virtual void FlushEvents();
	virtual bool PerModeLoggingEnabled();
	virtual bool UseNewCdn();
};
