#include "stdafx.h"
#include "resource.h"

#include "Application.h"
#include "InitializationError.h"
#include "util/ProgramMemoryChecker.h"
#include "v8datamodel/ContentProvider.h"

#define MAX_LOADSTRING 100
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

RBX::Application* appPtr;

LOGGROUP(HangDetection)
LOGGROUP(RobloxWndInit)

//  Processes messages for the main window.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_TIMER:
		// Notify log manager that main thread is still responsive
		if (MainLogManager* manager = LogManager::getMainLogManager()) {
			FASTLOG(FLog::HangDetection, "WindowsPlayer: timer event fired");
			manager->NotifyFGThreadAlive();
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_UPLOADSESSIONLOGS:
			appPtr->UploadSessionLogs();
			break;
		case ID_LOADWIKI:
			appPtr->OnHelp();
			break;
		}
		break;
	case WM_GETMINMAXINFO:
		RBX::Application::OnGetMinMaxInfo((MINMAXINFO*)lParam);
		break;
	case WM_KEYDOWN:
	case WM_MOUSEMOVE:
	case WM_MOUSELEAVE:
	case WM_MOUSEWHEEL:
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
	case WM_CHAR:
	case WM_INPUT:
		appPtr->HandleWindowsMessage(message, wParam, lParam);
		break;
	case WM_DESTROY:
		appPtr->AboutToShutdown();
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		appPtr->OnResize(wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//  Registers the window class.
ATOM RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOW_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WINDOWSCLIENT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_WINDOW_ICON));

	return RegisterClassEx(&wcex);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					   LPTSTR lpCmdLine, int nCmdShow)
{
    // This was compiled with sse2 support.  This should be first as any floats
    // will cause issues.
    if (!G3D::System::hasSSE2())
    {
        MessageBoxA(NULL, "This platform lacks SSE2 support.", "ROBLOX", MB_OK);
        return false;
    }

	UNREFERENCED_PARAMETER(hPrevInstance);

	RBX::Application app;
	appPtr = &app;
	CComModule comModule; // Needed for ActiveX hosting of IWebBrowser2

	if (!app.LoadAppSettings(hInstance))
		return FALSE;
	if (!app.ParseArguments(lpCmdLine))
		return FALSE;

	HWND hWnd = NULL;

	// need client settings here before we create window
	std::string clientSettingsString;
	FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &clientSettingsString);
	// Apply client settings
	LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, clientSettingsString, &RBX::ClientAppSettings::singleton());

	TCHAR szTitle[MAX_LOADSTRING];

	// initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WINDOWSCLIENT, szWindowClass, MAX_LOADSTRING);

	RegisterWindowClass(hInstance);

	// perform application initialization:
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance,
		NULL);

	if (!hWnd)
		return FALSE;

	try 
	{
        if (!app.Initialize(hWnd, hInstance))
            return FALSE;
	} 
	catch (const RBX::initialization_error& e) 
	{
		const char* const errorMessage = e.what();
		FASTLOGS(FLog::RobloxWndInit, "Error during initialization. User message = %s", errorMessage);
		MessageBoxA(hWnd, errorMessage, "ROBLOX", MB_OK);
		app.AboutToShutdown();
		app.Shutdown();
		return FALSE;
	}

	// set a "keep alive" timer that periodically puts a message in the queue
	// see message handler for WM_TIMER
	SetTimer(hWnd, NULL, 10 * 1000 /*once every ten seconds*/, NULL);

	// Only show the window if there isn't a named object to wait for before
	// displaying it
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);
	
	MSG msg;

	// main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    // at this point, threads will still be around.
    DWORD unused;
    VirtualProtect(reinterpret_cast<void*>(RBX::Security::rbxVmpBase), RBX::Security::rbxVmpSize, PAGE_EXECUTE_READWRITE, &unused);

	app.Shutdown();

	return static_cast<int>(msg.wParam);
}
