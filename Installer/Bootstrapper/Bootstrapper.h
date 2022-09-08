#pragma once

#include "commonresourceconstants.h"
#include "Maindialog.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "boost/thread.hpp"
#include <fstream>
#include <vector>
#include "FileSystem.h"
#include "format_string.h"
#include <atlsync.h>
#include "Progress.h"
#include "SharedHelpers.h"
#include "FileDeployer.h"
#include "ComModule.h"
#include "InfluxDbHelper.h"

class Bootstrapper : public boost::enable_shared_from_this<Bootstrapper>, public IInstallerSite
{
	boost::condition_variable done;
	boost::mutex mut;

	CHandle latestProcess;		// A shared file that contains the PID of the latest instance of this module
	const DWORD mainThreadId;

	int windowDelay;
	int cancelDelay;
	bool editMode;
	bool service;
	bool force;
	bool fullCheck;
	bool isUninstall;
	bool isFailIfNotUpToDate;
	bool debug;
	bool windowed;
	bool dontStartApp;
	bool perUser;
	bool adminInstallDetected;
	bool userCancelled;
	bool isNop;
	bool requestedInstall; // indicates that the -install command line flag was used
	bool performedNewInstall;
	bool performedUpdateInstall;
	bool performedBootstrapperUpdateAndQuit;
	time_t startTime;
	time_t userCancelledTime;
	DWORD startTickCounter;
	int stage;
	std::vector<std::string> errors;
	unsigned long reportStatGuid;

	std::string moduleVersionNumber;		// The version number of this module

	bool silentMode;
	// only used if silentMode is true, which means we don't show dialog at all
	CEvent robloxReady;
	// this mean that bootstrapper should not run anything except preDeploy
	bool noRun;
	bool waitOnStart;

	virtual std::string Type() = 0;
	virtual std::string BinaryType() { return ""; }
	std::string baseHost;
	std::string installHost;		// HTTP host for getting installation components
	std::string installVersion;		// version string of latest installer
	std::string browserTrackerId;	// passed in from website
	int exitCode;

	//will be used to track all versions of this component to do proper clean up
	std::list<std::string> prevVersions;

	int influxdbLottery;

	std::wstring prevVersionRegKey();
	std::wstring prevVersionValueKey(int index);
	void loadPrevVersions();
	void storePrevVersions();

	void postData(std::fstream &data);
	void LogMACaddress();

	void showWindowAfterDelay()
	{
		{
			boost::unique_lock<boost::mutex> lock(mut);
			if (done.timed_wait(lock, boost::get_system_time() + boost::posix_time::time_duration(0, 0, windowDelay)))
				return;
		}

		LOG_ENTRY1("Bootstrapper::showWindowAfterDelay showing window, thread id = %d", ::GetCurrentThreadId());
		if (!this->isSilentMode())
		{
			dialog->ShowWindow(CMainDialog::WindowTimeDelayShow);
		}
	}

	void showCancelAfterDelay()
	{
		{
			boost::unique_lock<boost::mutex> lock(mut);
			if (done.timed_wait(lock, boost::get_system_time() + boost::posix_time::time_duration(0, 0, cancelDelay)))
				return;
		}
		dialog->ShowCancelButton(CMainDialog::CancelTimeDelayShow);
	}

	
	void userCancel();
	void checkCancel();

	bool isIEUpToDate();
	void checkIEPrerequisit();
	void checkDirectXPrerequisit();
	bool isDirectXUpToDate();
	void checkOSPrerequisit();
	void checkCPUPrerequisit();

	void parseCmdLine();
	void run();
	void install();
	void uninstall();
	void uninstall(bool isPerUser);

	bool checkBootstrapperVersion();
	void shutdownRobloxApp(std::wstring appExeName);
	void checkDiskSpace();
	bool shutdownProgress(int& time, int pos, int max);
	void deleteVersionFolder(std::string version);

	void setLatestProcess();
	bool isLatestProcess();

	void deleteVersionsDirectoryContents();
	void forceUninstallMFCStudio();
	void forceMFCStudioCleanup();

	// If size is set to 0, size will not be included in the log.
	// Reporting eventually boils down to http requests, which will be made
	// with normal strings (not unicode strings), so this method also uses
	// normal strings to that the caller will consider and handle unicode as needed.
	void reportDurationAndSizeEvent(const char* category, const char* result, DWORD duration, DWORD size);
	void reportFinishingHelper(const char* category, const char* result);

	void ReportToInfluxDbImpl(const std::string& json);

	virtual bool EarlyInstall(bool isUninstall) { return false; }

protected:

	HINSTANCE hInstance;
	HWND launchedAppHwnd;

	boost::scoped_ptr<CMainDialog> dialog;
	boost::scoped_ptr<FileDeployer> deployer;
	CRegKey classesKey;

	//TODO move this into client part
	std::wstring robloxAppArgs;	// If empty, then don't launch RobloxApp.exe

	InfluxDb influxDb;

	Bootstrapper(HINSTANCE hInstance);

	static void dummyProgress(int pos, int max) {}
	static void createDirectory(const TCHAR* dir);

	void setStage(int s) { stage = s; }
	bool isPerUser() const { return perUser; }
	bool isDontStartApp() { return dontStartApp; }
	CMainDialog *Dialog() { return dialog.get(); }
	void setInstallVersion(std::string version) { installVersion = version; }

	std::wstring GetSelfName() const;

	virtual void initialize();
	virtual void createDialog() {} ;

	// lets make sure that this guy will be 100% client only (RCC won't run this code)
	virtual HRESULT SheduleRobloxUpdater() { return S_OK; };
	virtual HRESULT UninstallRobloxUpdater() { return S_OK; };
	// will load settings from server
	virtual void LoadSettings() {}
	virtual bool CanRunBgTask() { return true; }

	virtual void SetupInfluxDb();
	virtual std::string InfluxDbUrl() { return ""; }
	virtual std::string InfluxDbDatabase() { return ""; }
	virtual std::string InfluxDbUser() { return ""; }
	virtual std::string InfluxDbPassword() { return ""; }
	virtual int InfluxDbReportHundredthsPercentage() { return 0; }
	virtual int InfluxDbInstallHundredthsPercentage() { return 0; }
	void ReportToInfluxDb(int throttle, const std::string& json, bool blocking = false);
	void ReportInstallMetricsToInfluxDb(bool isUpdate, const std::string& result, const std::string& message);

	virtual void SetupGoogleAnalytics() {}
	virtual bool ValidateInstalledExeVersion() { return false; }
	virtual bool IsInstalledVersionUptodate() { return true; }

	virtual bool IsPreDeployOn() { return false; }
	virtual bool NeedPreDeployRun() { return false; }
	virtual void RunPreDeploy() {}
	virtual bool ForceNoDialogMode() { return false; }
	virtual bool GetLogChromeProtocolFix() { return false; }
	virtual bool GetUseNewVersionFetch() { return false; }
	virtual bool GetUseDataDomain() { return true; }
	virtual bool CreateEdgeRegistry() { return true; }
	virtual bool DeleteEdgeRegistry() { return false; }

	std::string fetchVersionGuid(std::string product);
	virtual std::string fetchVersionGuidFromWeb(std::string product);
	void writeAppSettings();

	void CreateProcess(const TCHAR* module, const TCHAR* args, PROCESS_INFORMATION& pi);
	void RegisterElevationPolicy(const TCHAR* appName, const TCHAR* appPath);
	void RegisterUninstall(const TCHAR *productName);
	void RegisterProtocolHandler(const std::wstring& protocolScheme, const std::wstring& exePath, const std::wstring& installKeyRegPath);
	void UnregisterProtocolHandler(const std::wstring& protocolScheme);
	bool isInstalled(std::wstring productKey);
	void CreateShortcuts(bool forceDesktopIconCreation, const TCHAR *linkName, const TCHAR *exeName, const TCHAR *params);
	void DeleteOldVersionsDirectories();
	void DoDeployComponents(const std::vector<std::pair<std::wstring, std::wstring> > &files, bool isUpdating, bool commitData);

	bool deleteDirectory(const std::wstring &refcstrRootDirectory, bool childrenOnly = false);
	std::string queryInstalledInstallHost();

	std::string queryInstalledVersion(const std::wstring& RegistryPath);
	std::string queryInstalledVersion() { return queryInstalledVersion(GetRegistryPath()); }

	std::wstring getInstalledBootstrapperPath(const std::wstring& RegistryPath);
	std::wstring getInstalledBootstrapperPath() { return getInstalledBootstrapperPath(GetRegistryPath()); }

	void deploySelf(bool commitData);
	void message(const std::string& message);

	void deleteLegacyShortcuts();

	void validateAndFixChromeState();

	HWND GetHwndFromPID(DWORD pid);

public:
	simple_logger<wchar_t> logger;

	virtual ~Bootstrapper(void);
	
	static boost::shared_ptr<Bootstrapper> Create(HINSTANCE hInstance, Bootstrapper*(*newBootstrapper)(HINSTANCE));
	static bool hasSse2();
	
	virtual std::wstring programDirectory() const;			  // Local location for installing components to
	std::wstring programDirectory(bool isPerUser) const;   // Local location for installing components to
	std::wstring baseProgramDirectory(bool isPerUser) const; //place where all installed versions are
	bool hasReturnValue() const { return isFailIfNotUpToDate; }
	bool hasLegacyStudioDesktopShortcut();
	bool isSilentMode() const { return silentMode; }
	bool isWindowed() const { return windowed; }
	bool WaitForCompletion() { DWORD result = WaitForSingleObject(robloxReady, 30*60*1000); return (result == WAIT_TIMEOUT); }
	int ExitCode() const { return exitCode; }
	void cleanKey(const std::wstring regPathToDelete, CRegKey &hk);
	std::string BrowserTrackerId() { return browserTrackerId; }
	HWND getLaunchedAppHwnd() { return launchedAppHwnd; }

	// IInstallerSite
	virtual std::string BaseHost() { return baseHost; }
	virtual std::string InstallHost() { return installHost; }
	virtual std::string InstallVersion() { return installVersion; }
	virtual simple_logger<wchar_t> &Logger() { return logger; }
	virtual void progressBar(int pos, int max);
	virtual void CheckCancel() { checkCancel(); }
	virtual void Error(const std::exception& e) { error(e); }
	virtual std::wstring ProgramDirectory() { return programDirectory(); }
	virtual bool UseNewCdn() { return false; }
	virtual bool ReplaceCdnTxt() { return false; }

	void Close();
	void postLogFile(bool uploadApp);
	bool hasErrors() const { return errors.size()>0; }
	bool hasLongUserCancel() const { return userCancelled && ((userCancelledTime - startTime) > 60); } 
	bool hasUserCancel() const { return userCancelled; }
	bool stopRequested() const { return userCancelled || hasErrors(); }
	void error(const std::exception& e);

	DWORD GetElapsedTime();
	void ReportElapsedEvent(bool startingGameClient);
	virtual void RegisterEvent(const TCHAR *eventName) {};
	virtual void FlushEvents() {};
	// Returns true if we want to enhance remote event reporting with
	// information needed to separate different bootstrapper use cases
	// (e.g. to be able to separate installs vs plays vs check-if-up-to-date).
	virtual bool PerModeLoggingEnabled() { return false; }
	void ReportFinishing(const char* status);

	virtual std::wstring GetRegistryPath() const = 0;
	virtual std::wstring GetRegistrySubPath() const = 0;
	virtual const wchar_t* GetVersionFileName() const = 0;
	virtual const wchar_t* GetGuidName() const = 0;
	virtual const wchar_t* GetStartAppMutexName() const = 0;
	virtual const wchar_t* GetBootstrapperMutexName() const = 0;
	virtual const wchar_t* GetFriendlyName() const = 0;
	virtual const wchar_t* GetProtocolHandlerUrlScheme() const = 0;

	virtual void DoInstallApp() {};
	virtual void DoUninstallApp(CRegKey &hk) {}; //component specific uninstall actions
	virtual std::wstring GetRobloxAppFileName() const { return std::wstring(_T("")); }
	virtual std::wstring GetBootstrapperFileName() const { return std::wstring(_T("")); }
	virtual std::wstring GetProductCodeKey() const { return std::wstring(_T("")); }
	virtual void DeployComponents(bool isUpdating, bool commitData) {};
	virtual void StartRobloxApp(bool fromInstall) {}
	virtual bool IsPlayMode() { return false; }
	virtual bool HasUnhideGuid() {return false; }
	virtual bool ProcessProtocolHandlerArgs(const std::map<std::wstring, std::wstring>& args) { return false; }
	virtual bool ProcessArg(wchar_t** args, int &pos, int count) { return false; }

};

int BootstrapperMain(HINSTANCE hInstance, int nCmdShow, Bootstrapper*(*newBootstrapper)(HINSTANCE));

