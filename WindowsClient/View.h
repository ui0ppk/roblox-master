#pragma once

#include "GfxBase/ViewBase.h"
#include "UserInput.h"
namespace RBX {

// Forward declarations
class FunctionMarshaller;
class Game;
struct OSContext;
class RenderJob;
class ViewBase;

namespace Tasks { class Sequence; }

// Class responsible for the game view
class View
{
public:
	View(HWND h);
	~View();

	void AboutToShutdown();

	void Start(const shared_ptr<Game>& game);
    void Stop();

    void OnResize(WPARAM wParam, int cx, int cy);
    void ShowWindow();

    void CloseWindow();
	void HandleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND GetHWnd() const { return static_cast<HWND>(context.hWnd); }

	// TODO: refactor verbs so this isn't needed
	ViewBase* GetGfxView() const { return view.get(); }

	CRenderSettings::GraphicsMode GetLatchedGraphicsMode();

	bool IsFullscreen();
	void SetFullscreen(bool value);

	shared_ptr<DataModel> getDataModel();

private:
	// View references, but doesn't own the game
	shared_ptr<Game> game;

	// The OS context used by the view.
	OSContext context;

	// Are we currently fullscreen?
	bool fullscreen;

	// Do we want to become fullscreen at first available opportunity?
	bool desireFullscreen;
	
	// When fullscreen this is true if and only if an actual screen resolution change occured
	bool changedResolution;

	// Enumerated fullscreen sizes.  Prone to change as user drags window from
	// player to player
	std::vector<G3D::Vector2int16> fullScreenSizes;

	// The area of the window when not fullscreen (used for exit fullscreen).
	WINDOWPLACEMENT nonFullscreenPlacement;

	// The style of the window before it went fullscreen
	DWORD restoreWindowStyle;

	// Are we currently changing resolution?
	bool changingResolution;

	// Handle to monitor game is running on
	HMONITOR hMonitor;

	// The view into the game world.
	boost::scoped_ptr<RBX::ViewBase> view;

	FunctionMarshaller* marshaller;
	boost::scoped_ptr<UserInput> userInput;
	boost::shared_ptr<Tasks::Sequence> sequence;
	boost::shared_ptr<RenderJob> renderJob;

	// Window settings
	bool windowSettingsValid;
	Vector4 windowSettingsRectangle;
	bool windowSettingsMaximized;

	// Used to enable toggling fullscreen support
	void modifyWindow(DWORD argMask, const RECT& area);
	bool findBestMonitorMatch(LPCTSTR szDevice, int desiredX, int desiredY, bool resolutionAuto, DEVMODE& dmBest);
	void changeResolution();
	void restoreResolution();
	G3D::Vector2int16 calcDefaultResolution(float aspect_XdivY);
	G3D::Vector2int16 getCurrentDesktopResolution();
	void initializeSizes();

	void bindWorkspace();
	void unbindWorkspace();

	void initializeView();
	void initializeInput();
	void resetScheduler();

    void initializeJobs();
    void RemoveJobs();

	void rememberWindowSettings();
	void saveWindowSettings();
};

}  // namespace RBX
