#pragma once

#include "UserInputUtil.h"

#include "Util/UserInputBase.h"
#include "Util/RunStateOwner.h"
#include "Util/Rect.h"

#include "SDLGameController.h"

#include "GfxBase/TextureProxyBase.h"

namespace RBX {
	class Game;
}

class RobloxCriticalSection
{
private:
public:
	int callers;
	std::string recentCaller;
	std::string olderCaller;
	RobloxCriticalSection() 
		: callers(0)
	{}
};

class RobloxCritSecLoc
{
private:
	RobloxCriticalSection& robloxCriticalSection;

public:
	RobloxCritSecLoc( RobloxCriticalSection& cs, const std::string& location) 
		: robloxCriticalSection(cs)
	{
	}
	
	~RobloxCritSecLoc()
	{
	}
};

#define NKEYSTATES 512

struct BufferedEvent
{
    RBX::InputObject::UserInputType userInputType;
    RBX::InputObject::UserInputState userInputState;
    unsigned int wParam;
    unsigned int lParam;
};

class UserInputJob;
class UserInput 
	: public RBX::UserInputBase
{
private:
	shared_ptr<UserInputJob> job;

	mutable RobloxCriticalSection diSection;
    
    rbx::signals::scoped_connection renderStepConnection;

	// Mouse Stuff
	bool isMouseCaptured;			// poor man's tracker of button state
	bool leftMouseButtonDown;
	G3D::Vector2 wrapMousePosition;		// in normalized coordinates (center is 0,0. radius is getWrapRadius)
	bool wrapping;
	bool rightMouseDown;
	bool autoMouseMove;

	// Keyboard Stuff
	int externallyForcedKeyDown;		// + this from exteranal source (like a gui button)

	class RobloxView * wnd;
    
    boost::mutex bufferMutex;
    std::vector<BufferedEvent> bufferedEvents;
    
    // InputObject stuff
	boost::unordered_map<RBX::InputObject::UserInputType,shared_ptr<RBX::InputObject> > inputObjectMap;
    boost::unordered_map<RBX::KeyCode,shared_ptr<RBX::InputObject> > keyboardInputObjects;
    
	////////////////////////////////
	//
	// Events
	void sendEvent(const shared_ptr<RBX::InputObject>& event);
	void sendMouseEvent(RBX::InputObject::UserInputType mouseEventType, RBX::InputObject::UserInputState mouseEventState, const RBX::Vector3& position, const RBX::Vector3& delta);

	////////////////////////////////////
	// 
	// Keyboard Mouse
	bool isMouseInside;
	
	G3D::Vector2 posToWrapTo;
    G3D::Vector2 accumulatedFractionalMouseDelta;
    
    
    ////////////////////////////////////
    // Gamepad Stuff
    shared_ptr<SDLGameController> sdlGameController;

	// window stuff
	RBX::Vector2int16 getWindowSize() const;
	G3D::Rect2D getWindowRect() const;
	bool isFullScreenMode() const;
	bool movementKeysDown();
	bool keyDownInternal(RBX::KeyCode code) const;

	void doWrapMouse(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta);
	
	G3D::Vector2 getGameCursorPositionInternal();
	G3D::Vector2 getGameCursorPositionExpandedInternal();	// prevent hysteresis
	G3D::Vector2 getWindowsCursorPositionInternal();
	RBX::Vector2 getCursorPositionInternal();
    
    void processInput();

public:
    shared_ptr<RBX::Game> game;

	////////////////////////////////
	//
	// UserInputBase

	/*implement*/ RBX::Vector2 getCursorPosition();
	
	void doWrapHybrid(bool cursorMoved, bool leftMouseUp, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, G3D::Vector2& posToWrapTo);

	/*implement*/ bool keyDown(RBX::KeyCode code) const;
    /*implement*/ void setKeyState(RBX::KeyCode code, RBX::ModCode modCode, char modifiedKey, bool isDown);

	/*implement*/ void centerCursor();

	/*override*/ RBX::TextureProxyBaseRef getGameCursor(RBX::Adorn* adorn);

	void PostUserInputMessage(RBX::InputObject::UserInputType eventType, RBX::InputObject::UserInputState eventState, unsigned int wParam, unsigned int lParam);
	// Call this only within a DataModel lock:
	void ProcessUserInputMessage(RBX::InputObject::UserInputType eventType, RBX::InputObject::UserInputState eventState, unsigned int wParam, unsigned int lParam);

	UserInput(RobloxView *wnd, shared_ptr<RBX::Game> newGame);
	~UserInput();
    
    void setGame(shared_ptr<RBX::Game> newGame);

	void onMouseInside();
	void onMouseLeave();
};
