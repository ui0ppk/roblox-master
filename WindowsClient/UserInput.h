#pragma once

#include "stdafx.h"
#include "UserInputUtil.h"
#include "Util/UserInputBase.h"
#include "util/standardout.h"
#include "SDLGameController.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxerr9.lib")
#pragma comment(lib, "dinput8.lib")

namespace RBX {

class Game;
class DataModel;
class RunService;
class View;

class RobloxCriticalSection
{
public:
	int callers;
	std::string recentCaller;
	std::string olderCaller;
	ATL::CCriticalSection diSection;

	RobloxCriticalSection() : callers(0)
	{
	}
};

class UserInput : public UserInputBase
{
	enum MouseButtonType
    {
	    MBUTTON_LEFT	= 0,
	    MBUTTON_RIGHT	= 1,
		MBUTTON_MIDDLE  = 2
    };

	rbx::signals::scoped_connection steppedConnection;

	mutable RobloxCriticalSection diSection;

	// Mouse Stuff
	bool isMouseCaptured;			// poor man's tracker of button state
	Vector2 wrapMousePosition;		// in normalized coordinates (center is 0,0. radius is getWrapRadius)
	bool wrapping;
	bool rightMouseDown;
	bool autoMouseMove;
	bool mouseButtonSwap;			// used for left hand mouse

	// Keyboard Stuff
	BYTE diKeys[256];					// regular key state
	int externallyForcedKeyDown;		// + this from external source (like a gui button)
	HKL layout;
	BYTE keyboardState[256];
	std::vector<ACCEL> accelerators;

	// DirectInput fields
	HWND wnd;
	CComPtr<IDirectInput8> diPtr;
	CComPtr<IDirectInputDevice8> diMousePtr;
	CComPtr<IDirectInputDevice8> diKeyboardPtr;

	// InputObject stuff
	boost::unordered_map<RBX::InputObject::UserInputType,shared_ptr<RBX::InputObject> > inputObjectMap;

	// Gamepad stuff
	shared_ptr<SDLGameController> sdlGameController;

	shared_ptr<RunService> runService;

	View* parentView;

	G3D::Vector2 posToWrapTo;

	G3D::Vector2 previousCursorPosFraction;

	bool leftMouseButtonDown;

	HCURSOR hArrow;

	////////////////////////////////
	//
	// Events
	void sendEvent(shared_ptr<InputObject> event);
	void sendMouseEvent(InputObject::UserInputType mouseEventType,
						InputObject::UserInputState mouseEventState,
						Vector3& position,
						Vector3& delta);
	// helper for mouse events
	void processMouseButtonEvent(DIDEVICEOBJECTDATA mouseData,
		MouseButtonType mouseButton, bool &leftMouseUp); 

	////////////////////////////////////
	//
	// Keyboard Mouse
	bool isMouseInside;
	bool desireKeyboardAcquired;

	bool isMouseAcquired;			// goes false if data read fails
	bool isKeyboardAcquired;		// goes false if data read fails

	void createMouse();
	void createKeyboard();
	void createAccelerators();

	void updateMouse();
	void updateKeyboard();

	void acquireMouseInternal();
	void acquireMouseInternalBase(const Vector2& pos);
	void acquireKeyboard();

	void unAcquireMouse();
	void unAcquireKeyboard();

	bool readBufferedData(LPDIDEVICEOBJECTDATA didod, DWORD& dwElements,
		IDirectInputDevice8* device);

	void readBufferedMouseData();
	void readBufferedKeyboardData();

	// window stuff
	Vector2int16 getWindowSize() const;
	G3D::Rect2D getWindowRect() const;
	bool isFullScreenMode() const;
	bool movementKeysDown();
	bool keyDownInternal(KeyCode code) const;

	void doWrapMouse(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta);
	void setKeyboardDesiredInternal(bool set);			// keyboard set by having focus;

	// todo: no longer used remove this
	void doWrapHybrid(bool cursorMoved, bool leftMouseUp, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, G3D::Vector2& posToWrapTo);

	G3D::Vector2 getGameCursorPositionInternal();
	G3D::Vector2 getGameCursorPositionExpandedInternal();	// prevent hysteresis
	G3D::Vector2 getWindowsCursorPositionInternal();
	Vector2 getCursorPositionInternal();

	void doDiagnostics();

	void postProcessUserInput(bool cursorMoved, bool leftMouseUp, RBX::Vector2 wrapMouseDelta, RBX::Vector2 mouseDelta);

public:
	shared_ptr<RBX::Game> game;
	const static int WM_CALL_SETFOCUS = WM_USER + 187;
	HCURSOR hInvisibleCursor;

	////////////////////////////////
	//
	// UserInputBase

	/*implement*/ Vector2 getCursorPosition();

	/*implement*/ bool keyDown(KeyCode code) const;
	/*implement*/ void setKeyState(RBX::KeyCode code, RBX::ModCode modCode, char modifiedKey, bool isDown);

	/*implement*/ void centerCursor() { wrapMousePosition = Vector2::zero(); }
	/*override*/ TextureProxyBaseRef getGameCursor(Adorn* adorn);

	void postUserInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// Call this only within a DataModel lock:
	void processUserInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	UserInput(HWND wnd, shared_ptr<RBX::Game> game, View* view);
	~UserInput();

	void setGame(shared_ptr<RBX::Game> game);
	void setKeyboardDesired(bool set);			// keyboard set by having focus;

	void onMouseInside();
	void onMouseLeave();

	bool getIsKeyboardAcquired() const { return isKeyboardAcquired; }
	bool getIsMouseAcquired() const { return isMouseAcquired; }
	void reacquireKeyboard();

	void processInput();
};

}  // namespace RBX
