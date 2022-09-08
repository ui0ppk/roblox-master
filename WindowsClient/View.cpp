#include "stdafx.h"
#include <atltypes.h>

#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/GameBasicSettings.h"
#include "script/ScriptContext.h"
#include "FunctionMarshaller.h"
#include "GameVerbs.h"
#include "LogManager.h"
#include "rbx/Tasks/Coordinator.h"
#include "RenderJob.h"
#include "RenderSettingsItem.h"
#include "util/ScopedAssign.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/UserController.h"
#include "InitializationError.h"
#include "View.h"
#include "format_string.h"

#include "util/RobloxGoogleAnalytics.h"
#include "rbx/SystemUtil.h"

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)
LOGGROUP(RobloxWndInit)
FASTFLAGVARIABLE(DirectXEnable, false)
FASTFLAGVARIABLE(DirectX11Enable, false)
FASTFLAGVARIABLE(GraphicsReportingInitErrorsToGAEnabled,true)
FASTFLAGVARIABLE(UseNewAppBridgeInputWindows, false)

DYNAMIC_FASTFLAGVARIABLE(FullscreenRefocusingFix, false)


namespace {
    HWND SetFocusWrapper(HWND hwnd)
    {
        return ::SetFocus(hwnd);
    }
}

namespace RBX {

static const char* kSavedScreenSizeRegistryKey =
	"HKEY_CURRENT_USER\\Software\\ROBLOX Corporation\\Roblox\\Settings\\RobloxPlayerV4WindowSizeAndPosition";

View::View(HWND h)
	: fullscreen(false)
	, desireFullscreen(false)
	, changedResolution(false)
	, changingResolution(false)
	, hMonitor(NULL)
	, marshaller(NULL)
	, windowSettingsValid(false)
	, windowSettingsMaximized(false)
	, restoreWindowStyle(NULL)
{
	ZeroMemory(&nonFullscreenPlacement, sizeof(nonFullscreenPlacement));

    desireFullscreen = RBX::GameBasicSettings::singleton().getFullScreen();
    context.hWnd = h;
    marshaller = FunctionMarshaller::GetWindow();

    initializeView();
}

View::~View()
{
    RBXASSERT(!this->game && "Call Stop() before shutting down!");
    view.reset();

    if (marshaller)
        FunctionMarshaller::ReleaseWindow(marshaller);
}


void View::AboutToShutdown()
{
	rememberWindowSettings();
}

void View::rememberWindowSettings()
{
	// try to save window size and position
	if (HWND hWnd = GetHWnd()) 
	{
		WINDOWPLACEMENT placement;
		placement.length = sizeof(WINDOWPLACEMENT);
		
		bool foundPlacement = false;
		if (!fullscreen)
			foundPlacement = GetWindowPlacement(hWnd, &placement) != 0;
		else
		{
			foundPlacement = true;
			placement = nonFullscreenPlacement;
		}

		if (foundPlacement)
		{
			RECT rect;
			GetWindowRect(hWnd, &rect);

			RECT windowTaskRect;
			HWND taskBar = FindWindow("Shell_traywnd", NULL);
			if(taskBar && GetWindowRect(taskBar, &windowTaskRect))
			{
				// need to do some adjustment depending on where the task bar is living

				const int newLeft =  rect.left - (windowTaskRect.right - windowTaskRect.left);
				if (newLeft >= 0)
				{
					const int leftDiff = rect.left - newLeft;
					rect.left = newLeft;
					rect.right -= leftDiff;
				}

				const int newTop =  rect.top - (windowTaskRect.bottom - windowTaskRect.top);
				if (newTop >= 0)
				{
					const int topDiff = rect.top - newTop;
					rect.top = newTop;
					rect.bottom -= topDiff;
				}

				placement.rcNormalPosition = rect;
			}
			else
			{
				placement.rcNormalPosition = rect;
			}

			Vector4 lastRect(placement.rcNormalPosition.left,placement.rcNormalPosition.top, 
				placement.rcNormalPosition.right - placement.rcNormalPosition.left,
				placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);

			windowSettingsValid = true;
			windowSettingsRectangle = lastRect;
			windowSettingsMaximized = (placement.showCmd == SW_SHOWMAXIMIZED);
		}
	}
}

void View::saveWindowSettings()
{
	if (windowSettingsValid)
	{
		DataModel::LegacyLock lock(game->getDataModel(), DataModelJob::Write);

		RBX::GameBasicSettings::singleton().setStartScreenSize(windowSettingsRectangle.zw());
		RBX::GameBasicSettings::singleton().setStartScreenPos(windowSettingsRectangle.xy());

		RBX::GameBasicSettings::singleton().setStartMaximized(windowSettingsMaximized);
	}
}

void View::initializeView()
{
    ViewBase::InitPluginModules();

    LPCSTR rgLogSuffix[5];
    memset(rgLogSuffix, 0, sizeof(rgLogSuffix));
    rgLogSuffix[(size_t)CRenderSettings::Direct3D9] = "gfx_d3d9";
    rgLogSuffix[(size_t)CRenderSettings::Direct3D11] = "gfx_d3d11";
    rgLogSuffix[(size_t)CRenderSettings::OpenGL] = "gfx_gl";

    std::vector<CRenderSettings::GraphicsMode> modes;

    RBX::CRenderSettings::GraphicsMode graphicsMode = CRenderSettingsItem::singleton().getLatchedGraphicsMode();
	if (FFlag::DirectXEnable)
	{
		switch (graphicsMode)
		{
		case CRenderSettings::NoGraphics:
			break;
		case CRenderSettings::OpenGL:
		case CRenderSettings::Direct3D9:
		case CRenderSettings::Direct3D11:
			modes.push_back(graphicsMode);
			break;
		default:
			if (FFlag::DirectX11Enable)
				modes.push_back(CRenderSettings::Direct3D11);
			modes.push_back(CRenderSettings::Direct3D9);
			modes.push_back(CRenderSettings::OpenGL);
			break;
		}
	}
	else
	{
		modes.push_back(CRenderSettings::OpenGL);
	}

	std::string lastMessage;
	bool success = false;
	size_t modei = 0;
	while(!success && modei < modes.size())
	{
		graphicsMode = modes[modei];
		try
		{
			view.reset(ViewBase::CreateView(graphicsMode, &context, &CRenderSettingsItem::singleton()));
			view->initResources();
			success = true;
		}
		catch (std::exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Mode %d failed: \"%s\"", graphicsMode, e.what());
			lastMessage += e.what(); 
            lastMessage += " | ";

			modei++;
			if(modei < modes.size())
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Trying mode %d...", modes[modei]);
			}
		}
	}

	if(!success)
	{
        if (FFlag::GraphicsReportingInitErrorsToGAEnabled)
        {{{
            lastMessage += SystemUtil::getGPUMake() + " | ";
            lastMessage += SystemUtil::osVer();
            const char* label = modes.size()? "GraphicsInitError" : "GraphicsInitErrorNoModes";
            RobloxGoogleAnalytics::trackEventWithoutThrottling( GA_CATEGORY_GAME, label , lastMessage.c_str(), 0 );
        }}}

        ::WriteProfileString("Settings", "lastGFXMode", "-1");
		throw initialization_error(
			"Your graphics drivers seem to be too old for Roblox to use.\n\n"
			"Visit http://www.roblox.com/drivers for info on how to perform a driver upgrade.");
	}

    RBXASSERT( view );

	::WriteProfileString("Settings", "lastGFXMode", format_string("%d", (int)graphicsMode).c_str());

	initializeSizes();
}

void View::resetScheduler()
{
	TaskScheduler& taskScheduler = TaskScheduler::singleton();
	taskScheduler.add(renderJob);
}

void View::HandleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_ACTIVATE == uMsg)
	{
		// We become active, need to restore window if fullscreen
		if ((fullscreen || (!fullscreen && desireFullscreen)) && !changingResolution && (WA_ACTIVE == wParam || WA_CLICKACTIVE == wParam)) 
		{
			changeResolution();
			HWND hWnd = GetHWnd();
			::ShowWindow(hWnd, SW_RESTORE);
			SetFocus(hWnd);
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		}
		// We become inactive, need to minimize if fullscreen
		else if ( fullscreen && !changingResolution && WA_INACTIVE == wParam 
			&& ::GetParent((HWND)lParam) != GetHWnd() ) 
		{
			HWND hWnd = GetHWnd();
			SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX| WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
			SetFocus(hWnd);
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		}
	}
	// Other messages that made it to here are meant for UserInput
	else if (!FFlag::UseNewAppBridgeInputWindows && userInput)
	{
		userInput->postUserInputMessage(uMsg, wParam, lParam);
	}
}

void View::initializeJobs()
{
	shared_ptr<DataModel> dataModel = game->getDataModel();
	renderJob.reset(new RenderJob(this, marshaller, dataModel));
}

void View::initializeInput()
{
	userInput.reset(new UserInput(GetHWnd(), game, this));

	if (userInput)
	{
		DataModel::LegacyLock lock(game->getDataModel(), DataModelJob::Write);

		ControllerService* service =
			ServiceProvider::create<ControllerService>(game->getDataModel().get());
		service->setHardwareDevice(userInput.get());
	}
}

void View::RemoveJobs()
{
	if (renderJob)
	{
		boost::function<void()> callback =
			boost::bind(&FunctionMarshaller::ProcessMessages, marshaller);
		TaskScheduler::singleton().removeBlocking(renderJob, callback);
	}

    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    marshaller->ProcessMessages();

    // All render processing is complete; it's safe to reset job pointers now
    renderJob.reset();
}

shared_ptr<DataModel> View::getDataModel() 
{  
	return game ? game->getDataModel() : shared_ptr<DataModel>(); 
}

CRenderSettings::GraphicsMode View::GetLatchedGraphicsMode()
{
	return CRenderSettingsItem::singleton().getLatchedGraphicsMode();
}

bool View::IsFullscreen()
{
	return fullscreen;
}

bool View::findBestMonitorMatch(LPCTSTR szDevice, int desiredX, int desiredY, bool resolutionAuto, DEVMODE& dmBest)
{
	DEVMODE dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);

	LONG result = EnumDisplaySettingsEx(szDevice, ENUM_CURRENT_SETTINGS, &dm, 0);
	if (result==0)
	{
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
			RBX::format("In view::findBestMonitorMatch EnumDisplaySettings failed. "
			"GetLastError() == %d", GetLastError()).c_str());
		return false;
	}

	FASTLOG2(FLog::RobloxWndInit, "Current screen resolution - %d x %d", dm.dmPelsWidth, dm.dmPelsHeight);

	// If current user mode is smaller than the desired resolution, just keep it
	if (dm.dmPelsHeight <= (DWORD)desiredY && resolutionAuto)
		return true;

	int desiredPixelCount = desiredX * desiredY;

	const double currentAspectRatio = double(dm.dmPelsWidth)/double(dm.dmPelsHeight);

	dmBest = dm;
	bool match = true;

	DWORD iModeNum = 0;
	while (EnumDisplaySettingsEx(szDevice, iModeNum++, &dm, 0)!=0)
	{
		const double aspectRatio = double(dm.dmPelsWidth)/double(dm.dmPelsHeight);
		if (std::abs(currentAspectRatio-aspectRatio) / currentAspectRatio <= 0.1)
		{
			if (dmBest.dmBitsPerPel==dm.dmBitsPerPel && dmBest.dmDisplayFrequency==dm.dmDisplayFrequency)
			{
				int a = desiredPixelCount-dm.dmPelsWidth*dm.dmPelsHeight;
				int b = desiredPixelCount-dmBest.dmPelsWidth*dmBest.dmPelsHeight;
				if (std::abs(a) < std::abs(b))
				{
					dmBest = dm;
					match = false;
				}
			}
		}
	}

	// Only change resolution
	dmBest.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

	return match;
}

void View::restoreResolution()
{
	fullscreen = false;
	FASTLOG(FLog::RobloxWndInit, "Start View::restoreResolution");

	RBX::ScopedAssign<bool> assign(changingResolution, true);

	if (hMonitor == NULL) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, "Unable to restore resolution, no handle to monitor");
		return;
	}

	if (changedResolution) {
		MONITORINFOEX mi;
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfo(hMonitor, &mi)) {
			// For some reason we need to hide the window before restoring the resolution,
			// otherwise ChangeDisplaySettings will return DISP_CHANGE_SUCCESSFUL
			::ShowWindow(GetHWnd(), SW_HIDE);

			LONG result = ChangeDisplaySettingsEx(mi.szDevice, NULL, NULL, 0, NULL);

			if (result != DISP_CHANGE_SUCCESSFUL) {
				LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
					RBX::format("Unable to restore resolution, ChangeDisplaySettingsEx returned %d", result).c_str());
			}

			::ShowWindow(GetHWnd(), SW_SHOWNORMAL);
		}
		changedResolution = false;
	}

	if (!fullscreen)
	{
		if (restoreWindowStyle) {
			SetWindowLongPtr(GetHWnd(), GWL_STYLE, restoreWindowStyle);
		} else {
			SetWindowLongPtr(GetHWnd(), GWL_STYLE, WS_OVERLAPPEDWINDOW);
		}
	}

	SetWindowPlacement(GetHWnd(), &nonFullscreenPlacement);
	FASTLOG(FLog::RobloxWndInit, "Done Vew::restoreResolution");
}

void View::SetFullscreen(bool value)
{
	if (fullscreen != value) 
	{
		if (value)
		{
			restoreWindowStyle = GetWindowLong(GetHWnd(), GWL_STYLE);
			changeResolution();
		} 
		else
			restoreResolution();
	}
	desireFullscreen = value;
	RBX::GameBasicSettings::singleton().setFullScreen(value);
}

void View::initializeSizes()
{
	if (GetHWnd() == NULL) {
		LogManager::ReportEvent(EVENTLOG_WARNING_TYPE, "Attempt to initialize monitor sizes without valid HWND");
		return;
	}
	G3D::Vector2int16 currentDisplaySize = getCurrentDesktopResolution();

	G3D::Vector2int16 fullscreenSize, windowSize;
   
	RBX::CRenderSettings::ResolutionPreset preference = CRenderSettingsItem::singleton().getResolutionPreference();
	if (preference == RBX::CRenderSettings::ResolutionAuto) {
		fullscreenSize = currentDisplaySize;
	} else {
		const RBX::CRenderSettings::RESOLUTIONENTRY& res = 
			CRenderSettingsItem::singleton().getResolutionPreset(preference);
		fullscreenSize.x = res.width;
		fullscreenSize.y = res.height;
	}

	// validate mode
    windowSize = fullscreenSize;
	windowSize.x = std::min((int)windowSize.x, (int)currentDisplaySize.x);
	windowSize.y = std::min((int)windowSize.y, (int)currentDisplaySize.y);

	CRenderSettingsItem::singleton().setWindowSize(windowSize);
	CRenderSettingsItem::singleton().setFullscreenSize(fullscreenSize);
}

void View::modifyWindow(DWORD argMask, const RECT& area)
{
	SetWindowLongPtr(GetHWnd(), GWL_STYLE, argMask);
	SetWindowPos(GetHWnd(), NULL, area.left, area.top, area.right - area.left, area.bottom - area.top, 0);
}

void View::changeResolution()
{
	// Save current rect so we can restore to it, but only if we're not already fullscreen
	if (!DFFlag::FullscreenRefocusingFix || !fullscreen)
	{
		ZeroMemory(&nonFullscreenPlacement, sizeof(nonFullscreenPlacement));
		nonFullscreenPlacement.length = sizeof(nonFullscreenPlacement);
		GetWindowPlacement(GetHWnd(), &nonFullscreenPlacement);
	}

	fullscreen = true;

	// Get monitor information
	hMonitor = MonitorFromWindow(GetHWnd(), MONITOR_DEFAULTTONEAREST);
	if (hMonitor == NULL) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, "Cannot changeResolution, MonitorFromWindow returned NULL");
		return;
	}

	// We need to initialize sizes every time we toggle fullscreen in case user
	// has modified desktop settings during program run
	initializeSizes();

	RBX::ScopedAssign<bool> assign(changingResolution, true);

	MONITORINFOEX mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, "Cannot changeResolution, GetMonitorInfo failed");
		return;
	}

	G3D::Vector2int16 size = CRenderSettingsItem::singleton().getFullscreenSize();
	DEVMODE dm;
	bool matches = findBestMonitorMatch(mi.szDevice, size.x, size.y, 
		CRenderSettingsItem::singleton().getResolutionPreference() == RBX::CRenderSettings::ResolutionAuto, dm);

	LONG result = matches ? DISP_CHANGE_SUCCESSFUL : ChangeDisplaySettingsEx(mi.szDevice, &dm, 
		NULL, CDS_FULLSCREEN, NULL);

	if (result == DISP_CHANGE_SUCCESSFUL) {
		if (!changedResolution)
			changedResolution = !matches;

		if (!matches)
			FASTLOG2(FLog::RobloxWndInit, "Changed screen resolution to %d x %d", dm.dmPelsWidth, dm.dmPelsHeight);

		// Now resize the window to the monitor's (potentially) new resolution
		MONITORINFOEX monitorInfo;
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (!GetMonitorInfo(hMonitor, &monitorInfo)) {
			LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, "Cannot changeResolution, GetMonitorInfo failed");
			return;
		}

		int cxBorder = ::GetSystemMetrics(SM_CXBORDER);
		int cyBorder = ::GetSystemMetrics(SM_CYBORDER);
		cxBorder = cyBorder = 0;

		G3D::Vector2int16 newResolution = getCurrentDesktopResolution();

		RECT area = monitorInfo.rcMonitor;
		area.left -= cxBorder;
		area.right += cxBorder * 2;
		area.top -= cyBorder;
		area.bottom += cyBorder * 2;

		modifyWindow(WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, area);
	} else {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, RBX::format("ChangeDisplaySettings returned %d", result).c_str());
	}
	FASTLOG(FLog::RobloxWndInit, "Done View::changeResolution");
}

G3D::Vector2int16 View::calcDefaultResolution(float aspect_XdivY)
{
	// It's ok to return approximate fullscreen size since fullscreen transition 
	// code will filter it through the mode list
	int numlines = 600;
	int videomem = RBX::DebugSettings::singleton().videoMemory();

	std::string videocard = RBX::DebugSettings::singleton().gfxcard();
	if (videocard.find("Intel") != std::string::npos) {
		// Intel videocards report random amount of video memory, so use patterns to find good resolution

		// 828xx cards mean 810/815, no shaders, really slow
		if (videocard.find("828") != std::string::npos) {
			numlines = 600;
		} else {
			numlines = 1200;
		}
	} else if (videomem <= 32) {
		numlines = 600;
	} else if (videomem <= 64) {
		numlines = 1024;
	} else if (videomem <= 128) {
		numlines = 1200;
	} else if (videomem <= 256) {
		numlines = 1280;
	} else {
		numlines = 1600;
	}

	int numrows = (int)(numlines * aspect_XdivY);

	return G3D::Vector2int16(numrows, numlines);
}

G3D::Vector2int16 View::getCurrentDesktopResolution()
{
	G3D::Vector2int16 defaultr(800,600);

	if (NULL == hMonitor) {
		hMonitor = MonitorFromWindow(GetHWnd(), MONITOR_DEFAULTTONEAREST);
		if (!hMonitor)
			return defaultr;
	}

	MONITORINFOEX mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
			RBX::format("view::geCurrentDesctopResolution GetMonitorInfo failed, GetLastError returned %d", 
			GetLastError()).c_str());
		return defaultr;
	}

	DEVMODE dm;
	ZeroMemory(&dm, sizeof(dm));
	dm.dmSize = sizeof(dm);

	DWORD iModeNum = 0;
	if (EnumDisplaySettingsEx(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm, 0)!=0) {		
		return G3D::Vector2int16(dm.dmPelsWidth, dm.dmPelsHeight);
	} else {
		return defaultr;
	}
}

void View::OnResize(WPARAM wParam, int cx, int cy)
{
	view->onResize(cx, cy);
}

void View::ShowWindow()
{
	// This needs to be done fairly late in initialization. It assumes that it
	// is safe to trigger a window resize event, for example.

	HWND hWnd = GetHWnd();
	if (RBX::GameBasicSettings::singleton().getStartMaximized())
	{
		::ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	}
	else
	{
		// Read window info from xml, if present.
		Vector4 startScreenRect(RBX::GameBasicSettings::singleton().getStartScreenPos(), RBX::GameBasicSettings::singleton().getStartScreenSize());

		::ShowWindow(hWnd, SW_SHOWNORMAL);

		if ( !fullscreen && (startScreenRect != Vector4::zero()) )
		{
			WINDOWPLACEMENT p = nonFullscreenPlacement;
			RECT normalRect;
			normalRect.left = startScreenRect.x;
			normalRect.top = startScreenRect.y;
			normalRect.right = startScreenRect.x + startScreenRect.z;
			normalRect.bottom = startScreenRect.y + startScreenRect.w;

			p.rcNormalPosition = normalRect;
			p.showCmd = SW_SHOWNOACTIVATE;
			p.length = sizeof(WINDOWPLACEMENT);

			SetWindowPlacement(hWnd, &p);
		}	
	}

	// Bring window to foreground and give it focus.
    // SetFocus can only do this from specific threads.
    marshaller->Submit(boost::bind(&::SetFocusWrapper,hWnd));

	// Enable then immediately disable the "always on top" bit to bring
	// this window to the foreground (this is more reliable than HWND_TOP).
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

	if (RBX::GameBasicSettings::singleton().getFullScreen()) 
	{
		SetFullscreen(true);
	}
}

void View::unbindWorkspace()
{
	shared_ptr<DataModel> dm = getDataModel();
	DataModel::LegacyLock lock( dm, DataModelJob::Write);
	view->bindWorkspace(boost::shared_ptr<DataModel>());
}

void View::bindWorkspace()
{
	shared_ptr<DataModel> dm = getDataModel();
	DataModel::LegacyLock lock( dm, DataModelJob::Write);
	view->bindWorkspace(game->getDataModel());
	view->buildGui();
}

void View::Start(const shared_ptr<Game>& game)
{
    RBXASSERT(!this->game);
    this->game = game;

    bindWorkspace();
    initializeJobs();
    initializeInput(); // NOTE: have to do this here, Input requires datamodel access
	resetScheduler();

	// ensure keyboard is in focus (DE6272)
	if(userInput)
		userInput->setKeyboardDesired(true);
}

void View::Stop()
{
    RBXASSERT(this->game);
    this->RemoveJobs();

    if (game && game->getDataModel())
        if (RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(game->getDataModel().get()))
            service->setHardwareDevice(NULL);

	if (userInput)
	{
		userInput->removeJobs();
		userInput.reset();
	}

    unbindWorkspace();

	saveWindowSettings();

    game.reset();
}

void View::CloseWindow()
{
	PostMessage(GetHWnd(), WM_CLOSE, 0, 0);
}


}  // namespace RBX
