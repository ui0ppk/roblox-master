#include "stdafx.h"
#include "UserInput.h"

#include "resource.h"
#include "Util/Rect.h"
#include "Util/Standardout.h"
#include "Util/NavKeys.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/Datamodel.h"
#include "v8datamodel/DataModelJob.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/UserInputService.h"
#include "View.h"
#include "Humanoid/Humanoid.h"
#include "RbxG3D/RbxTime.h"
#include "G3D/System.h"
#include "G3D/Vector2.h"
#include "RenderSettingsItem.h"
#include "FastLog.h"

#define DXINPUT_TRACE  __noop

LOGGROUP(UserInputProfile)
DYNAMIC_FASTFLAGVARIABLE(MouseDeltaWhenNotMouseLocked, false)
DYNAMIC_FASTFLAGVARIABLE(UserInputViewportSizeFixWindows, true)

FASTFLAG(UserAllCamerasInLua)

#define RBX_DI_BUFFER_SIZE 2048

namespace RBX {

class RobloxCritSecLoc
{
	RobloxCriticalSection& robloxCriticalSection;
	CCritSecLock lock;

public:
	RobloxCritSecLoc( RobloxCriticalSection& cs, const std::string& location)
		: robloxCriticalSection(cs)
		, lock(cs.diSection)
	{
// should compile away the string manipulations
#ifdef __RBX_NOT_RELEASE
		// Show THESE ASSERTS TO David/Erik - they imply multiple entry points
		// to User Input Object - trying to track down
		RBXASSERT(robloxCriticalSection.callers == 0);
		robloxCriticalSection.callers++;
		robloxCriticalSection.olderCaller = robloxCriticalSection.recentCaller;
		robloxCriticalSection.recentCaller = location;
#endif
	}

	~RobloxCritSecLoc()
	{
// should compile away the string manipulations
#ifdef __RBX_NOT_RELEASE
		RBXASSERT(robloxCriticalSection.callers == 1);
		robloxCriticalSection.callers--;
		robloxCriticalSection.recentCaller = "";
		robloxCriticalSection.olderCaller = "";
#endif
	}
};

UserInput::UserInput(HWND wnd, shared_ptr<RBX::Game> game, View* view)
	: externallyForcedKeyDown(0)
	, isMouseCaptured(false)
	, isMouseAcquired(false)
	, isKeyboardAcquired(false)
	, desireKeyboardAcquired(false)
	, isMouseInside(false)
	, wnd(wnd)
	, wrapping(false)
	, posToWrapTo(0, 0)
	, leftMouseButtonDown(false)
	, rightMouseDown(false)
	, autoMouseMove(true)
	, mouseButtonSwap(false)
	, parentView(view)
{
#ifdef _DEBUG
	hArrow = LoadCursor(NULL, IDC_CROSS);
	hInvisibleCursor = LoadCursor(GetModuleHandle(NULL), IDC_CROSS);
#else
	hArrow = LoadCursor(NULL, IDC_ARROW);
	hInvisibleCursor = LoadCursor(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDR_INVISIBLECURSOR));
#endif

	setGame(game);

	memset(&diKeys, 0, sizeof(diKeys));

	HRESULT hr;
	if( FAILED( hr = DirectInput8Create(GetModuleHandle(NULL),
										DIRECTINPUT_VERSION,
										IID_IDirectInput8,
										(VOID**)&diPtr,
										NULL ) ) )
	{
		MessageBoxA(NULL, "ROBLOX could not initialize input. Please make sure you have a mouse and keyboard plugged in.", "ROBLOX", MB_OK);
		return;
	}

	// check to see if user has swapped their primary and secondary mouse buttons (left hand mouse)
	CRegKey mouseKey;
	if (mouseKey.Open(HKEY_CURRENT_USER, "Control Panel\\Mouse", KEY_READ) == ERROR_SUCCESS) // ERROR_SUCCESS? Really, Microsoft?
	{
		TCHAR szBuffer[256];
		ULONG cchBuffer = 256;
		if(mouseKey.QueryStringValue(_T("SwapMouseButtons"), szBuffer, &cchBuffer) == ERROR_SUCCESS)
			mouseButtonSwap = ( szBuffer[0] == '1' );
	}

	createMouse();
	createKeyboard();
	createAccelerators();

	layout = GetKeyboardLayout(0);
}

void UserInput::setGame(shared_ptr<RBX::Game> game)
{
	this->game = game;

	runService = shared_from(ServiceProvider::create<RunService>(game->getDataModel().get()));

	shared_ptr<DataModel> dataModel = game->getDataModel();

	sdlGameController = shared_ptr<SDLGameController>(new SDLGameController(dataModel));

	steppedConnection = runService->earlyRenderSignal.connect(boost::bind(&UserInput::processInput, this));
}
UserInput::~UserInput()
{
	sdlGameController.reset();

	RBXASSERT(diMousePtr);
	RBXASSERT(diKeyboardPtr);
	RBXASSERT(diPtr);

	if (diMousePtr)
	{
		diMousePtr->Unacquire();
		diMousePtr = 0;
	}

	if (diKeyboardPtr)
	{
		diKeyboardPtr->Unacquire();
		diKeyboardPtr = 0;
	}

	if (diPtr)
		diPtr = 0;			// is this the same as release() -> Yes!
}

void getDiProp(DIPROPDWORD& dipdw)
{
	dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj        = 0;
	dipdw.diph.dwHow        = DIPH_DEVICE;
	dipdw.dwData            = RBX_DI_BUFFER_SIZE; // Arbitrary buffer size
}

void UserInput::createMouse()
{
	HRESULT hr;
	if (FAILED(hr = diPtr->CreateDevice( GUID_SysMouse, &diMousePtr, NULL)))
	{
		MessageBoxA(NULL, "Could not find a mouse! ROBLOX requires a mouse.", "ROBLOX", MB_OK);
		return;
	}

	if (FAILED(hr = diMousePtr->SetDataFormat(&c_dfDIMouse2)))
	{
		MessageBoxA(NULL, "Could not find a mouse (data format failed)! ROBLOX requires a mouse.", "ROBLOX", MB_OK);
		return;
	}

	DIPROPDWORD dipdw;
	getDiProp(dipdw);
	if (FAILED(hr = diMousePtr->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
	{
		MessageBoxA(NULL, "Could not find a mouse (set property failed)! ROBLOX requires a mouse.", "ROBLOX", MB_OK);
		return;
	}
}

void UserInput::createKeyboard()
{
	HRESULT hr;

	if (FAILED(hr = diPtr->CreateDevice(GUID_SysKeyboard, &diKeyboardPtr, NULL)))
	{
		MessageBoxA(NULL, "Could not find a keyboard! ROBLOX requires a keyboard.", "ROBLOX", MB_OK);
		return;
	}

	if (FAILED(hr = diKeyboardPtr->SetDataFormat(&c_dfDIKeyboard)))
	{
		MessageBoxA(NULL, "Could not find a keyboard (data format failed)! ROBLOX requires a keyboard.", "ROBLOX", MB_OK);
		return;
	}

	DIPROPDWORD dipdw;
	getDiProp(dipdw);
	if (FAILED(hr = diKeyboardPtr->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
	{
		MessageBoxA(NULL, "Could not find a keyboard (set property failed)! ROBLOX requires a keyboard.", "ROBLOX", MB_OK);
		return;
	}
}

void UserInput::createAccelerators()
{
	HACCEL hAccel = LoadAccelerators(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_GAME_ACCELERATOR)); 
	if (NULL != hAccel) {
		int numAccelerators = CopyAcceleratorTable(hAccel, NULL, 0);
		accelerators.resize(numAccelerators);
		if (numAccelerators != 0) {
			CopyAcceleratorTable(hAccel, &accelerators[0], numAccelerators);
		}
	}
}

void UserInput::setKeyboardDesiredInternal(bool set)
{
	desireKeyboardAcquired = set;
	updateKeyboard();
}

void UserInput::setKeyboardDesired(bool set)
{
	RobloxCritSecLoc lock(diSection, "setKeyboardDesired");
	setKeyboardDesiredInternal(set);
}

void UserInput::updateMouse()
{
	if (!diMousePtr || diMousePtr.p == NULL)
	{
		isMouseAcquired = false;
		return;
	}

	if (isMouseAcquired == isMouseInside)
		return;

	if (isMouseInside)
		acquireMouseInternal();
	else
		unAcquireMouse();
}

void UserInput::acquireMouseInternal()
{
	Vector2 cp = getCursorPositionInternal();
	acquireMouseInternalBase(cp);
}

void UserInput::acquireMouseInternalBase(const Vector2& pos)
{
	RBXASSERT(!isMouseAcquired);

	if (diMousePtr)
	{
		diMousePtr->Unacquire();	// for good measure -

		HRESULT hr = diMousePtr->SetCooperativeLevel(wnd,
			DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		if (hr != DI_OK)
		{
			DXINPUT_TRACE("diMousePtr->SetCooperativeLevel() Failed %d\n", hr);
		}
		else
		{
			DXINPUT_TRACE("diMousePtr->SetCooperativeLevel() Succeeded %d\n", hr);
			hr = diMousePtr->Acquire();
			if (hr != DI_OK)
			{
				DXINPUT_TRACE("diMousePtr->Acquire() Failed %d\n", hr);
			}
			else
			{
				wrapMousePosition = Math::expandVector2(pos - getWindowRect().center(), -10);	// i.e. - pull towards the origin - remove chatter
				DXINPUT_TRACE("diMousePtr->Acquire() Succeeded %d\n", hr);
				isMouseAcquired = true;
				isMouseCaptured = false;		// reset here
				SetClassLong(wnd, GCL_HCURSOR, (long)hInvisibleCursor);
			}
		}
	}
}

void UserInput::unAcquireMouse()
{
	RBXASSERT(isMouseAcquired);

	HRESULT hr = diMousePtr->Unacquire();
	if (hr != DI_OK)
		DXINPUT_TRACE("diMousePtr->UnAcquire() Failed %d\n", hr);
	else
		DXINPUT_TRACE("diMousePtr->UnAcquire() Succeeded \n");

	// Move the Windows cursor into position
	Vector2 pos = getGameCursorPositionExpandedInternal();
	POINT p;
	p.x = static_cast<int>(pos.x);
	p.y = static_cast<int>(pos.y);
	ClientToScreen(wnd, &p);

	SetCursorPos(p.x, p.y);

	SetClassLong(wnd, GCL_HCURSOR, (long)hArrow);
	isMouseAcquired = false;
	isMouseCaptured = false;		// reset here
}

void UserInput::updateKeyboard()
{
	if (diKeyboardPtr.p == NULL)
		return;

	if (isKeyboardAcquired == desireKeyboardAcquired)
		return;

	if (desireKeyboardAcquired) {
		DXINPUT_TRACE("updateKeyboard believes keyboard should be acquired\n");
		acquireKeyboard();
	} else {
		DXINPUT_TRACE("updateKeyboard believes keyboard should be unacquired\n");
		unAcquireKeyboard();
	}
}

void UserInput::acquireKeyboard()
{
	RBXASSERT(!isKeyboardAcquired);

	diKeyboardPtr->Unacquire();		// for good measure

	HRESULT hr = diKeyboardPtr->SetCooperativeLevel(wnd,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (hr != DI_OK)
	{
		DXINPUT_TRACE("diKeyboardPtr->SetCooperativeLevel failed %d\n", hr);
	}
	else
	{
		DXINPUT_TRACE("diKeyboardPtr->SetCooperativeLevel succeeded %d\n", hr);
		if (FAILED(hr = diKeyboardPtr->Acquire()))
		{
			DXINPUT_TRACE("Keyboard Acquisition failed %d\n", hr);
		}
		else
		{
			isKeyboardAcquired = true;
			DXINPUT_TRACE("Keyboard Acquisition succeeded %d\n", hr);
		}
	}
}

void UserInput::unAcquireKeyboard()
{
	diKeyboardPtr->Unacquire();
	DXINPUT_TRACE("Keyboard Acquisition given up intentionally\n");
	isKeyboardAcquired = false;
}


void UserInput::postUserInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_MOUSEMOVE)
		if (shared_ptr<DataModel> dataModel = game->getDataModel())
			dataModel->mouseStats.osMouseMove.sample();

	processUserInputMessage(uMsg, wParam, lParam);
}

void UserInput::processUserInputMessage(UINT uMsg, WPARAM wParam,
										LPARAM lParam)
{
	RobloxCritSecLoc lock(diSection, "ProcessUserInputMessage");

	if (uMsg == WM_MOUSEMOVE)
	{
		onMouseInside();

		if(!isMouseAcquired)
		{
			G3D::Vector3 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam),0);
			sendMouseEvent(UserInputUtil::msgToEventType(uMsg),UserInputUtil::msgToEventState(uMsg), pos, Vector3(0,0,0));
		}
		updateMouse(); // correct potential failure to acquire. (this happens on mouse over when app is not foreground)

		if (uMsg == WM_LBUTTONDOWN)
			isMouseCaptured = true;
		if (uMsg == WM_LBUTTONUP)
			isMouseCaptured = false;

		return;
	}

	if (uMsg == WM_MOUSELEAVE)
	{
		onMouseLeave();
		return;
	}

	if (uMsg == WM_MOUSEWHEEL)
	{
		if(!isMouseAcquired)
		{
			int zDelta = (int) wParam; // wheel rotation
			G3D::Vector3 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam),zDelta);
			sendMouseEvent(InputObject::TYPE_MOUSEWHEEL,InputObject::INPUT_STATE_CHANGE,pos, Vector3(0,0,0));
		}
		return;
	}

	if (uMsg == WM_SETFOCUS)
	{
		setKeyboardDesiredInternal(true);

		DXINPUT_TRACE(std::string("UserInput::WM_SETFOCUS\n").c_str());
		shared_ptr<InputObject> event = Creatable<Instance>::create<InputObject>(InputObject::TYPE_FOCUS, InputObject::INPUT_STATE_BEGIN, game->getDataModel().get());
		sendEvent(event);
		return;
	}

	if (uMsg == WM_KILLFOCUS)
	{
		DXINPUT_TRACE(std::string("UserInput::WM_KILLFOCUS\n").c_str());
		shared_ptr<InputObject> event = Creatable<Instance>::create<InputObject>(InputObject::TYPE_FOCUS, InputObject::INPUT_STATE_END, game->getDataModel().get());
		sendEvent(event);
	}
}

/////////////////////////////////////////////////////////////////////
// Event Handling
//

bool UserInput::readBufferedData(LPDIDEVICEOBJECTDATA didod,
								   DWORD& dwElements,
								   IDirectInputDevice8* device)
{
	HRESULT hr = device->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), didod, &dwElements, 0 );

	if (hr == DI_OK)
	{
		return true;
	}
	else if (hr == DI_BUFFEROVERFLOW)
	{
		std::string devString = (device == diKeyboardPtr) ? "Keyboard " : "Mouse ";
		std::string error = devString + "DI_BUFFEROVERFLOW!\n";
		StandardOut::singleton()->print(MESSAGE_ERROR, error);
		DXINPUT_TRACE(("UserInput::readBufferedData: " + error).c_str());
		return true;
	}

	std::string devString = (device == diKeyboardPtr) ? "Keyboard " : "Mouse ";
	DXINPUT_TRACE(("UserInput::readBufferedData: " + devString + "Acquisition was lost\n").c_str());
	return false;
}

void UserInput::readBufferedKeyboardData()
{
	RBXASSERT(isKeyboardAcquired);

	DIDEVICEOBJECTDATA didod[RBX_DI_BUFFER_SIZE];  // Receives buffered data
	DWORD              dwElements = RBX_DI_BUFFER_SIZE;

	if (!readBufferedData(didod, dwElements, diKeyboardPtr))
	{
		isKeyboardAcquired = false;
		return;
	}

	FASTLOG1(FLog::UserInputProfile, "Read %u keyboard events", dwElements);


	// Study each of the buffer elements and process them.
	for (DWORD i = 0; i < dwElements; ++i)
	{
		DWORD diKey = didod[ i ].dwOfs;
		const bool down = (didod[i].dwData & 0x80) != 0;

		ModCode modCode = UserInputUtil::createModCode(diKeys);
		KeyCode keyCode = UserInputUtil::directInputToKeyCode(diKey);

		if (down)
		{
			// suppress is to prevent keyboard shortcuts (brute-force attempt 
			// to prevent back-doors)
			bool suppress = false;
			DWORD vKey = MapVirtualKeyEx(diKey, 1, layout);
			if (vKey == 0)
				vKey = UserInputUtil::keyCodeToVK(keyCode);

			// We have a whitelist of approved keys.  Any key not in this list
			// will not be processed as an accelerator.  Note that Lua GUI
			// controls will still receive keyboard input.
			switch (vKey) {
				case VK_F4:
					if (modCode & (RBX::KMOD_LALT | RBX::KMOD_RALT)) {
						PostMessage(wnd, WM_CLOSE, 0, 0);
						continue;
					}
					break;
				case VK_F1:
				case VK_F8:
				case VK_F11:
				case VK_SNAPSHOT:
					break;
				default:
					suppress = true;
					break;
			}

			// Find if key corresponds to accelerator, and if so post
			// associated WM_COMMAND message
			if (!suppress) {
				int iCharCode = MapVirtualKeyEx(vKey, 2, layout);
				char charCode = 0xFF & iCharCode;
				DXINPUT_TRACE("diKey=0x%x, vKey=0x%x, iCharCode=0x%x", diKey, vKey, iCharCode);
				if (iCharCode!=0)
					DXINPUT_TRACE(", charCode='%c'\n", charCode);
				else
					DXINPUT_TRACE("\n");

				DWORD modKeys = modCode & (RBX::KMOD_LCTRL | RBX::KMOD_RCTRL) ? FCONTROL : 0;
				modKeys |= modCode & (RBX::KMOD_LALT | RBX::KMOD_RALT) ? FALT : 0;
				modKeys |= modCode & (RBX::KMOD_LSHIFT | RBX::KMOD_RSHIFT) ? FSHIFT : 0;

				for (size_t i = 0; i<accelerators.size(); ++i)
				{
					if (accelerators[i].fVirt & FVIRTKEY)
					{
						if (accelerators[i].key != vKey)
							continue;
					}
					else
					{
						if (accelerators[i].key != charCode)
							continue;
					}
					if (modKeys != (accelerators[i].fVirt & (FALT | FCONTROL | FSHIFT)))
						continue;

					PostMessage(wnd, WM_COMMAND, 0x10000 | accelerators[i].cmd, 0);
					continue;
					break;
				}
			}
		}

		GetKeyboardState(keyboardState);

		DWORD virtKey = MapVirtualKeyEx(diKey, 1, layout);
		unsigned short asciiKey;
		int result = ToAsciiEx(virtKey, diKey, keyboardState, &asciiKey,
			0, layout);

		char key = UserInputService::getModifiedKey(keyCode, modCode);

		if (result == 1)
			key = asciiKey & 0xff;

		if( RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(game->getDataModel().get()) )
		{
			userInputService->setKeyState(keyCode, modCode, key, down);
		}

		InputObject::UserInputState eventState = down ? InputObject::INPUT_STATE_BEGIN : InputObject::INPUT_STATE_END;

		shared_ptr<RBX::InputObject> keyInput = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(InputObject::TYPE_KEYBOARD, eventState, keyCode, modCode, key, game->getDataModel().get());
		sendEvent(keyInput);
	}
}

void UserInput::processMouseButtonEvent(DIDEVICEOBJECTDATA mouseData, MouseButtonType mouseButton, bool &leftMouseUp)
{
	if(mouseButtonSwap) // we need to send events to each button reversed
	{
		if(mouseButton == MBUTTON_LEFT)
			mouseButton = MBUTTON_RIGHT;
		else
			mouseButton = MBUTTON_LEFT;
	}

	switch(mouseButton)
	{
	case MBUTTON_LEFT:
		DXINPUT_TRACE("wnd.PostMessage(WM_CALL_SETFOCUS)\n");
		::PostMessage(wnd, WM_CALL_SETFOCUS, 0, 0);
		sendMouseEvent( RBX::InputObject::TYPE_MOUSEBUTTON1, 
						(int)mouseData.dwData==0x80 
							? RBX::InputObject::INPUT_STATE_BEGIN 
							: RBX::InputObject::INPUT_STATE_END,
						Vector3(getCursorPositionInternal(),0), Vector3(0,0,0));

		leftMouseUp = ((int)mouseData.dwData != 0x80);
		autoMouseMove = leftMouseUp;
		leftMouseButtonDown = ((int)mouseData.dwData == 0x80);
		break;
	case MBUTTON_RIGHT:
		DXINPUT_TRACE("wnd.PostMessage(WM_CALL_SETFOCUS)\n");
		::PostMessage(wnd, WM_CALL_SETFOCUS, 0, 0);
		sendMouseEvent( RBX::InputObject::TYPE_MOUSEBUTTON2, 
						(int)mouseData.dwData==0x80 
							? RBX::InputObject::INPUT_STATE_BEGIN 
							: RBX::InputObject::INPUT_STATE_END,
						Vector3(getCursorPositionInternal(),0), Vector3(0,0,0) );

		rightMouseDown = ((int)mouseData.dwData == 0x80);
		break;
	case MBUTTON_MIDDLE:
		DXINPUT_TRACE("wnd.PostMessage(WM_CALL_SETFOCUS)\n");
		::PostMessage(wnd, WM_CALL_SETFOCUS, 0, 0);
		sendMouseEvent( RBX::InputObject::TYPE_MOUSEBUTTON3, 
						(int)mouseData.dwData==0x80 
							? RBX::InputObject::INPUT_STATE_BEGIN 
							: RBX::InputObject::INPUT_STATE_END,
						Vector3(getCursorPositionInternal(),0), Vector3(0,0,0) );
		break;
	default:
		break;
	}
}

void UserInput::readBufferedMouseData()
{
	RBXASSERT(isMouseAcquired);

	DIDEVICEOBJECTDATA didod[RBX_DI_BUFFER_SIZE];  // Receives buffered data
	DWORD              dwElements = RBX_DI_BUFFER_SIZE;

	if (!readBufferedData(didod, dwElements, diMousePtr))
	{
		isMouseAcquired = false;
		return;
	}

	FASTLOG1(FLog::UserInputProfile, "Read %u mouse events", dwElements);

	bool cursorMoved = false;
	bool leftMouseUp = false;
	G3D::Vector2 wrapMouseDelta;
	G3D::Vector2 mouseDelta;

	// Study each of the buffer elements and process them.
	for(DWORD i = 0; i < dwElements; ++i )
	{
		switch( didod[ i ].dwOfs )
		{
		case DIMOFS_BUTTON0:
			FASTLOG(FLog::UserInputProfile, "Processing mouse button0");
			processMouseButtonEvent(didod[i], MBUTTON_LEFT, leftMouseUp);
			break;
		case DIMOFS_BUTTON1:
			processMouseButtonEvent(didod[i], MBUTTON_RIGHT, leftMouseUp);
			break;
		case DIMOFS_BUTTON2:
			processMouseButtonEvent(didod[i], MBUTTON_MIDDLE, leftMouseUp);
			break;
		case DIMOFS_X:
		case DIMOFS_Y:
			{
				mouseDelta = UserInputUtil::didodToVector2(didod[i]);
				previousCursorPosFraction += mouseDelta * RBX::GameBasicSettings::singleton().getMouseSensitivity();
				mouseDelta.x = (int) previousCursorPosFraction.x;
				mouseDelta.y = (int) previousCursorPosFraction.y;
				previousCursorPosFraction -= mouseDelta;
				cursorMoved = true;
				doWrapMouse(mouseDelta,	wrapMouseDelta);
			}
			break;
		case DIMOFS_Z:
			float delta = float((int)didod[ i ].dwData) / float(WHEEL_DELTA);

			if (DFFlag::MouseDeltaWhenNotMouseLocked)
			{
				sendMouseEvent( InputObject::TYPE_MOUSEWHEEL,
					InputObject::INPUT_STATE_CHANGE,
					Vector3(getGameCursorPositionInternal(), delta), Vector3(mouseDelta.x, mouseDelta.y, 0) );
			}
			else
			{
				sendMouseEvent( InputObject::TYPE_MOUSEWHEEL,
								InputObject::INPUT_STATE_CHANGE,
								Vector3(getGameCursorPositionInternal(), delta), Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0) );
			}
			break;
		}
	}

	/////////// DONE PROCESSING ////////////////
	postProcessUserInput(cursorMoved, leftMouseUp, wrapMouseDelta, mouseDelta);
}

void UserInput::postProcessUserInput(bool cursorMoved, bool leftMouseUp, RBX::Vector2 wrapMouseDelta, RBX::Vector2 mouseDelta)
{
	UserInputService* userInputService = ServiceProvider::find<UserInputService>(game->getDataModel().get());

	// kind of a hack to allows us to pan when mouse isn't moving
	if (userInputService->getMouseWrapMode() == UserInputService::WRAP_HYBRID && !game->getDataModel()->getMouseOverGui())
	{
		doWrapHybrid(cursorMoved,
			leftMouseUp,
			wrapMouseDelta,
			wrapMousePosition,
			posToWrapTo);
	}
	else if (GameBasicSettings::singleton().inMousepanMode())
	{
		if (leftMouseButtonDown && GameBasicSettings::singleton().getCanMousePan())
			userInputService->setMouseWrapMode(UserInputService::WRAP_CENTER);
		else if (!rightMouseDown)
			userInputService->setMouseWrapMode(UserInputService::WRAP_AUTO);
	}

	if (wrapMouseDelta != Vector2::zero())
	{
        if (RBX::Workspace* workspace = game->getDataModel()->getWorkspace())
        {
            if (Camera* camera = workspace->getCamera())
            {
                if (!(FFlag::UserAllCamerasInLua && camera->hasClientPlayer()))
                {
                    if ( camera->getCameraType() != Camera::CUSTOM_CAMERA )
                    {
                        workspace->onWrapMouse(wrapMouseDelta);
                    }
                }
            }
        }

		sendMouseEvent(InputObject::TYPE_MOUSEDELTA, InputObject::INPUT_STATE_CHANGE, Vector3(getCursorPositionInternal(),0), Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0) );
		sendMouseEvent(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE, Vector3(getCursorPositionInternal(),0), Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0) );

	}
	else if (cursorMoved)
	{
		if (DFFlag::MouseDeltaWhenNotMouseLocked)
		{
			sendMouseEvent(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE, Vector3(getCursorPositionInternal(),0), Vector3(mouseDelta.x,mouseDelta.y,0) );
		}
		else
		{
			sendMouseEvent(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE, Vector3(getCursorPositionInternal(),0), Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0) );
		}
		
	}
}

void UserInput::sendMouseEvent(InputObject::UserInputType mouseEventType, 
							   InputObject::UserInputState mouseEventState,
							   Vector3& position,
							   Vector3& delta)
{
	FASTLOG1(FLog::UserInputProfile, "Processing mouse event from the queue, event: %u", mouseEventType);

	shared_ptr<RBX::InputObject> mouseEventObject;
	if( inputObjectMap.find(mouseEventType) == inputObjectMap.end() )
	{
		mouseEventObject = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(mouseEventType, mouseEventState, position, delta, game->getDataModel().get());
		inputObjectMap[mouseEventType] = mouseEventObject;
	}
	else
	{
		mouseEventObject = inputObjectMap[mouseEventType];
		mouseEventObject->setInputState(mouseEventState);
		mouseEventObject->setPosition(position);
		mouseEventObject->setDelta(delta);
		inputObjectMap[mouseEventType] = mouseEventObject;
	}
	
	sendEvent(mouseEventObject);
}

void UserInput::sendEvent(shared_ptr<InputObject> event)
{
	if( RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(game->getDataModel().get()) )
		userInputService->fireInputEvent(event, NULL);
}

bool UserInput::isFullScreenMode() const
{
	return parentView->IsFullscreen();
}

G3D::Rect2D UserInput::getWindowRect() const
{
	if (!DFFlag::UserInputViewportSizeFixWindows) {
		RECT rect;
		GetClientRect(wnd, &rect);
		return Rect2D::xyxy(rect.left, rect.top, rect.right, rect.bottom);
	}
	else 
	{
		RBX::Camera* cam = game->getDataModel().get()->getWorkspace()->getCamera();
		return Rect2D::xywh(0.0f, 0.0f, cam->getViewportWidth(), cam->getViewportHeight());
	}
}

G3D::Vector2int16 UserInput::getWindowSize() const
{
	G3D::Rect2D windowRect = getWindowRect();
	return G3D::Vector2int16((G3D::int16) windowRect.width(), (G3D::int16) windowRect.height());
}

TextureProxyBaseRef UserInput::getGameCursor(Adorn* adorn)
{
	if (isMouseAcquired)
		return __super::getGameCursor(adorn);
	else
		return TextureProxyBaseRef();
}

void UserInput::doWrapHybrid(bool cursorMoved, bool leftMouseUp, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, G3D::Vector2& posToWrapTo)
{
}

bool UserInput::movementKeysDown()
{
	return (	keyDownInternal(SDLK_w)	|| keyDownInternal(SDLK_s) || keyDownInternal(SDLK_a) || keyDownInternal(SDLK_d)
			||	keyDownInternal(SDLK_UP)|| keyDownInternal(SDLK_DOWN) || keyDownInternal(SDLK_LEFT) || keyDownInternal(SDLK_RIGHT) );
}

G3D::Vector2 UserInput::getGameCursorPositionInternal()
{
	RBXASSERT(isMouseAcquired);
	return getWindowRect().center() + wrapMousePosition;
}

G3D::Vector2 UserInput::getGameCursorPositionExpandedInternal()
{
	RBXASSERT(isMouseAcquired);
	return getWindowRect().center() + Math::expandVector2(wrapMousePosition, 10);
}

// will be called repeatedly. ignore repeated calls.
void UserInput::onMouseInside()
{
	if(isMouseInside)
		return; // we know. ignore.

	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = wnd;

	if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(game->getDataModel().get()))
	{
		if (userInputService->getMouseWrapMode() == RBX::UserInputService::WRAP_NONEANDCENTER)
		{
			centerCursor();
		}
	}

	if (!TrackMouseEvent(&tme))
		return;

	isMouseInside = true;

	if (!isMouseCaptured)
		acquireMouseInternal();
	else
		RBXASSERT(isMouseAcquired); // isMouseCaptured should imply isMouseAquired.
}

void UserInput::onMouseLeave()
{
	if (!isMouseInside)
		return;

	isMouseInside = false;

	if(!isMouseCaptured)
	{
		// acquire could have failed... we could be already in
		// an unacquired state.
		if(isMouseAcquired)
			unAcquireMouse();
	}
}

G3D::Vector2 UserInput::getWindowsCursorPositionInternal()
{
	RBXASSERT(!isMouseAcquired);

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(wnd, &p);
	return G3D::Vector2(static_cast<float>(p.x), static_cast<float>(p.y));
}

G3D::Vector2 UserInput::getCursorPosition()
{
	RobloxCritSecLoc lock(diSection, "getCursorPosition");
	return getCursorPositionInternal();
}

G3D::Vector2 UserInput::getCursorPositionInternal()
{
	return isMouseAcquired ? getGameCursorPositionInternal() : getWindowsCursorPositionInternal();
}

bool UserInput::keyDownInternal(KeyCode code) const
{
	if (externallyForcedKeyDown && (code == externallyForcedKeyDown))
		return true;

	if (diKeys[UserInputUtil::keyCodeToDirectInput(code)] & 0x80)
		return true;

	return false;
}

bool UserInput::keyDown(KeyCode code) const
{
	return keyDownInternal(code);
}

void UserInput::setKeyState(RBX::KeyCode code, RBX::ModCode modCode, char modifiedKey, bool isDown)
{
	RobloxCritSecLoc lock(diSection, "setKeyState");
	externallyForcedKeyDown = isDown ? code : 0;
}

void UserInput::reacquireKeyboard()
{
	DXINPUT_TRACE("UserInput::reaquireKeyboard begin\n");
	unAcquireKeyboard();
	setKeyboardDesiredInternal(true);
	acquireKeyboard();
	DXINPUT_TRACE("UserInput::reaquireKeyboard end\n");
}

void UserInput::processInput()
{
	RobloxCritSecLoc lock(diSection, "processInput");
	updateKeyboard();

	if (isMouseAcquired)
		readBufferedMouseData();

	if (diKeyboardPtr!=NULL)
		diKeyboardPtr->GetDeviceState(256, diKeys);

	if (isKeyboardAcquired)
		readBufferedKeyboardData();
}

void UserInput::doWrapMouse(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta)
{
	RBXASSERT(isMouseAcquired);

	UserInputService* userInputService = ServiceProvider::find<UserInputService>(game->getDataModel().get());

	switch (userInputService->getMouseWrapMode())
	{
	case UserInputService::WRAP_NONEANDCENTER: // intentional fall thru
			centerCursor();
	case RBX::UserInputService::WRAP_NONE: // intentional fall thru
	case UserInputService::WRAP_CENTER:
            UserInputUtil::wrapMouseCenter(delta,
				wrapMouseDelta,
				this->wrapMousePosition);
			break;
	case UserInputService::WRAP_HYBRID:
			UserInputUtil::wrapMousePos(delta,
				wrapMouseDelta,
				wrapMousePosition,
				getWindowSize(),
				posToWrapTo,
				autoMouseMove);
			break;
	case UserInputService::WRAP_AUTO:
			if (!GameBasicSettings::singleton().inMousepanMode() && (movementKeysDown() || isMouseCaptured))		// 1. If movement keys are down - keep the mouse in the window
			{
				UserInputUtil::wrapMouseBorderLock(delta,			// 2. If the left mouse button is down (we are dragging) - keep in the window
					wrapMouseDelta,
					this->wrapMousePosition,
					getWindowSize());
			}
			else if (isFullScreenMode())							// 3. OK - we're in PlayMode (i.e. character)
			{
				UserInputUtil::wrapFullScreen(delta,
					wrapMouseDelta,
					this->wrapMousePosition,
					getWindowSize());
			}
			else
			{
				// We no longer want mouse wrap and camera auto-pan at the horizontal window extents.
				UserInputUtil::wrapMouseNone(delta, wrapMouseDelta,
					this->wrapMousePosition);
			}
			break;
	}

	Vector2 pos = getGameCursorPositionInternal();

	POINT p;
	p.x = static_cast<int>(pos.x);
	p.y = static_cast<int>(pos.y);

	ClientToScreen(wnd, &p);
	SetCursorPos(p.x, p.y);
}

void UserInput::doDiagnostics()
{
	// Diagnostics
	static int errorCount = 0;
	static bool haveFailed = false;
	if (isMouseAcquired)
	{
		if (haveFailed)
		{
			if (errorCount>0)
			{
				StandardOut::singleton()->printf(MESSAGE_WARNING, "UserInput::attemptOwnMouse Failed to acquire mouse exclusively %d time(s)", errorCount);
				errorCount = 0;
			}
			haveFailed = false;
		}
	}
	else
	{
		static G3D::RealTime nextMessage = G3D::System::time();
		++errorCount;
		haveFailed = true;
		if (nextMessage <= G3D::System::time())
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING, "UserInput::attemptOwnMouse Failed to acquire mouse exclusively %d time(s)", errorCount);
			errorCount = 0;
			nextMessage = G3D::System::time() + 10.0f;		// wait 10 seconds so as not to slow things down with logging
		}
	}
}

}  // namespace RBX
