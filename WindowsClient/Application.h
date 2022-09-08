#pragma once

#include "LogManager.h"
#include "Teleporter.h"
#include "SharedLauncher.h"

#include "rbx/atomic.h"
#include "v8datamodel/FastLogSettings.h"

#include "Util/HttpAsync.h"
#include "Util/Analytics.h"

// forward declarations
namespace po = boost::program_options;

class CProcessPerfCounter;
class DumpErrorUploader;
class RbxWebView;

namespace RBX {

// more forward declarations
class Game;
class UserInput;
class RenderJob;
class LeaveGameVerb;
class RecordToggleVerb;
class ScreenshotVerb;
class ToggleFullscreenVerb;
class ProfanityFilter;
class RecordToggleVerb;
class FunctionMarshaller;
class Document;
struct StandardOutMessage;
class View;

namespace Tasks { class Sequence; }

class Application {

	enum RequestPlaceInfoResult
	{
		SUCCESS,
		FAILED,
		RETRY,
		GAME_FULL,
		USER_LEFT,
	};

	RBX::Analytics::InfluxDb::Points analyticsPoints;

public:
	Application();
	~Application();

	// Initializes the application.
	bool Initialize(HWND hWnd, HINSTANCE hInstance);

	// Load AppSettings.xml
	bool LoadAppSettings(HINSTANCE hInstance);

	// Notification that Shutdown will be happening soon
	void AboutToShutdown();

	// Free resources and perform cleanup operations before Application
	// destructor is called.
	void Shutdown();

	// Parses command line arguments.
	bool ParseArguments(const char* argv);

	// Posts messages the document or view care about (mouse, keyboard, window
	// activation, etc).
	void HandleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Resizes the viewport when the window is resized.
	void OnResize(WPARAM wParam, int cx, int cy);

	void Teleport(const std::string& authenticationUrl,
		const std::string& ticket,
		const std::string& scriptUrl);

	// Creates a crash dump which results in an upload to the server the next
	// time the process runs.
	void UploadSessionLogs();

	// Show help wiki
	void OnHelp();

	// Named event to wait for before showing the window
	std::string WaitEventName() { return waitEventName; }

	static void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

 
private:
	boost::scoped_ptr<boost::thread> launchPlaceThread;
	boost::scoped_ptr<boost::thread> reportingThread;
	void LaunchPlaceThreadImpl(const std::string& placeLauncherUrl); 
	void InferredCrashReportingThreadImpl();

	void InitializeNewGame(HWND hWnd);

	void StartNewGame(HWND hWnd, HttpFuture& scriptResult, bool isTeleport);

	void initializeLogger();
	void setWindowFrame();
	void initializeCrashReporter();
	void uploadCrashData(bool userRequested);
	void handleError(const std::exception& e);
	bool requestPlaceInfo(int placeId, std::string& authenticationUrl,
		std::string& ticket,
		std::string& scriptUrl) const;
	RequestPlaceInfoResult requestPlaceInfo(const std::string url, std::string& authenticationUrl,
		std::string& ticket,
		std::string& scriptUrl) const;
	void renewLogin(const std::string& authenticationUrl,
		const std::string& ticket) const;
	HttpFuture renewLoginAsync(const std::string& authenticationUrl,
		const std::string& ticket) const;

	HttpFuture loginAsync(const std::string& userName, const std::string& passWord) const;

	void shareHwnd(HWND hWnd);

	const char* getVRDeviceName();

	rbx::atomic<int> enteredShutdown;
	HANDLE processLocal_stopPreventMultipleJobsThread;
	HANDLE processLocal_stopWaitForVideoPrerollThread;
    HWND   mainWindow;

	HANDLE mapFileForWnd;
	LPCTSTR bufForWnd;

	LONG stopLogsCleanup; 
	ATL::CEvent clenupFinishedEvent;
	boost::scoped_ptr<boost::thread> logsCleanUpThread;
	void logsCleanUpHelper();

	// Thread to prevent multiple instances of WindowsPlayer from running
	// simultaneously
	void waitForNewPlayerProcess(HWND hWnd);
	void waitForShowWindow(int delay);
	void validateBootstrapperVersion();

	// Sends messages to the debugger for display.
	static void onMessageOut(const StandardOutMessage& message);

	// Gets version number from .rc and returns display-friendly representation
	std::string getversionNumber();

	// determines what kind of game mode we are in
	SharedLauncher::LaunchMode launchMode;

    // Application owns the views
    boost::scoped_ptr<View>        mainView;
    boost::scoped_ptr<RbxWebView>  webView;

	// Command-line arguments
	po::variables_map vm;

	// Filename and path
	std::string moduleFilename;

	// Settings (e.g. GlobalBasicSettings_10.xml)
	std::string globalBasicSettingsPath;

	// If not empty string this is the name of a named event the 
	std::string waitEventName;

	// Application owns the Document
	boost::scoped_ptr<Document> currentDocument;

	// The crash report components.
	bool crashReportEnabled, hideChat;
	MainLogManager logManager;
	boost::scoped_ptr<DumpErrorUploader> dumpErrorUploader;

	boost::shared_ptr<CProcessPerfCounter> processPerfCounter;
	boost::shared_ptr<ProfanityFilter> profanityFilter;

	boost::scoped_ptr<boost::thread> singleRunningInstance;
	boost::scoped_ptr<boost::thread> showWindowAfterEvent;
	boost::scoped_ptr<boost::thread> validateBootstrapperVersionThread;

	Teleporter teleporter;

	FunctionMarshaller* marshaller;
	bool spoofMD5; // Only used in DEBUG / NOOPT builds

    boost::scoped_ptr<ToggleFullscreenVerb> toggleFullscreenVerb;
    boost::scoped_ptr<LeaveGameVerb> leaveGameVerb;
    boost::scoped_ptr<RecordToggleVerb> recordToggleVerb;
    boost::scoped_ptr<ScreenshotVerb> screenshotVerb;

    void initVerbs();
    void shutdownVerbs();

    void openUrlInBrowserApp(const std::string url);
    void closeBrowser();
    void doOpenUrl(const std::string url);
    void doCloseBrowser();

	void onDocumentStarted(bool isTeleport);

};

}  // namespace RBX
