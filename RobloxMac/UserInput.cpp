#include "Util/Rect.h"
#include "Util/NavKeys.h"
#include "Util/Standardout.h"
#include "Util/NavKeys.h"
#include "v8datamodel/ContentProvider.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/UserInputService.h"
#include "RbxG3D/RbxTime.h"
#include "G3D/System.h"
#include "UserInput.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/GameBasicSettings.h"
#include "RobloxView.h"
#include "V8DataModel/SleepingJob.h"
#include "V8DataModel/DataModelJob.h"

#define DXINPUT_TRACE  __noop

DYNAMIC_FASTFLAGVARIABLE(MouseDeltaWhenNotMouseLocked, false)

FASTFLAG(UserAllCamerasInLua)

namespace RBX
{
	class DataModel;
}

class UserInputJob : public RBX::SleepingJob
{
private:
	UserInput* const userInput;
    
    void processQueue()
    {
        Event event;
        while(queue.pop_if_present(event))
		{
            userInput->ProcessUserInputMessage(event.eventType, event.eventState, event.wParam, event.lParam);
        }
    }

public:
	struct Event
	{
		RBX::InputObject::UserInputType eventType;
        RBX::InputObject::UserInputState eventState;
		unsigned int wParam;
		unsigned int lParam;
	};
	rbx::safe_queue<Event> queue;

	UserInputJob(UserInput* userInput, shared_ptr<RBX::DataModelArbiter> arbiter)
        :RBX::SleepingJob("UserInput", RBX::DataModelJob::Write, false, arbiter, RBX::Time::Interval(0.01), 60)
		,userInput(userInput)
	{}
							 
	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
        RBX::DataModel::scoped_write_request request( userInput->game->getDataModel().get() );
        processQueue();
    
		sleep();
		
		return RBX::TaskScheduler::Stepped;
	}
};



UserInput::UserInput(RobloxView *wnd, shared_ptr<RBX::Game> newGame)
	:	externallyForcedKeyDown(0),
		isMouseCaptured(false),
		isMouseInside(false),
		wnd(wnd),
		leftMouseButtonDown(false),
		rightMouseDown(false),
		autoMouseMove(false),
		posToWrapTo(0,0),
		wrapping(false)
{
    setGame(newGame);
}


void UserInput::setGame(shared_ptr<RBX::Game> newGame)
{
	game = newGame;
    if(shared_ptr<RBX::DataModel> dataModel = game->getDataModel())
    {
        renderStepConnection.disconnect();
        
        job.reset(new UserInputJob(this, dataModel));
        RBX::TaskScheduler::singleton().add(job);
        
        sdlGameController = shared_ptr<SDLGameController>(new SDLGameController(dataModel));
        
        if (RBX::RunService* runService = RBX::ServiceProvider::create<RBX::RunService>(dataModel.get()))
        {
                renderStepConnection = runService->earlyRenderSignal.connect(boost::bind(&UserInput::processInput, this));
        }
    }
}

UserInput::~UserInput()
{
    sdlGameController.reset();
    
    RBX::TaskScheduler::singleton().removeBlocking(job);
}

void UserInput::PostUserInputMessage(RBX::InputObject::UserInputType eventType, RBX::InputObject::UserInputState eventState, unsigned int wParam, unsigned int lParam)
{
    BufferedEvent bufferedEvent;
    bufferedEvent.userInputType = eventType;
    bufferedEvent.userInputState = eventState;
    bufferedEvent.wParam = wParam;
    bufferedEvent.lParam = lParam;
    
    boost::mutex::scoped_lock lock(bufferMutex);

    bufferedEvents.push_back(bufferedEvent);
}

void UserInput::processInput()
{
    std::vector<BufferedEvent> tempVec;
    {
        boost::mutex::scoped_lock lock(bufferMutex);
        bufferedEvents.swap(tempVec);
    }
    
    for (std::vector<BufferedEvent>::iterator iter = tempVec.begin(); iter != tempVec.end(); ++iter)
    {
        ProcessUserInputMessage(iter->userInputType, iter->userInputState, iter->wParam, iter->lParam);
    }
}

extern void MacWriteFastLogDump();

void UserInput::ProcessUserInputMessage(RBX::InputObject::UserInputType eventType, RBX::InputObject::UserInputState eventState, unsigned int wParam, unsigned int lParam)
{
	RobloxCritSecLoc lock(diSection, "ProcessUserInputMessage");

	bool cursorMoved = false;
	bool leftMouseUp = false;
	RBX::Vector2 wrapMouseDelta;
	G3D::Vector2 mouseDelta;

	switch (eventType)
	{
        case RBX::InputObject::TYPE_KEYBOARD:
        {
            if (wParam == RBX::SDLK_F8 && lParam == RBX::KMOD_NONE)
				MacWriteFastLogDump();
            
            const RBX::ModCode modCode = (RBX::ModCode) lParam;
            const bool keyDown = (eventState == RBX::InputObject::INPUT_STATE_BEGIN);
            const char key = RBX::UserInputService::getModifiedKey((RBX::KeyCode) wParam,(RBX::ModCode) lParam);
            
            if(shared_ptr<RBX::DataModel> dataModel = game->getDataModel())
                if(RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(dataModel.get()))
                    userInputService->setKeyState((RBX::KeyCode) wParam, modCode, key, keyDown);
            
            RBX::KeyCode keyCode = (RBX::KeyCode) wParam;
            
			if(eventState == RBX::InputObject::INPUT_STATE_END && keyboardInputObjects.find(keyCode) != keyboardInputObjects.end())
			{
				shared_ptr<RBX::InputObject> keyInput = keyboardInputObjects[keyCode];
				keyInput->setInputState(RBX::InputObject::INPUT_STATE_END);
				keyInput->mod = (RBX::ModCode) lParam;
				keyInput->modifiedKey = key;
                
                sendEvent(keyInput);
				keyboardInputObjects.erase(keyCode);
			}
			else
			{
				shared_ptr<RBX::InputObject> keyInput = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(eventType, eventState, keyCode, (RBX::ModCode) lParam, key, game->getDataModel().get());
				keyboardInputObjects[keyCode] = keyInput;
                sendEvent(keyInput);
			}
            
            break;
        }
        case RBX::InputObject::TYPE_MOUSEBUTTON1:
        case RBX::InputObject::TYPE_MOUSEBUTTON2: 
		case RBX::InputObject::TYPE_MOUSEBUTTON3: // Intentional fall thru (todo: lets clean this up)
        {
        	RBX::InputObject event(eventType, eventState, RBX::Vector3(getCursorPosition(),0), RBX::Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0), game->getDataModel().get());
        	RBXASSERT(event.isLeftMouseDownEvent() || event.isLeftMouseUpEvent() || event.isRightMouseDownEvent() || event.isRightMouseUpEvent() || event.isMiddleMouseDownEvent() || event.isMiddleMouseUpEvent());

            G3D::Vector2 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
			sendMouseEvent(eventType, eventState, RBX::Vector3(getCursorPosition(),0), RBX::Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0));
            
			if ( event.isLeftMouseDownEvent() )
			{
				leftMouseButtonDown = true;
				leftMouseUp = false;
				isMouseCaptured = true;
			}
			if ( event.isLeftMouseUpEvent() )
			{
				leftMouseButtonDown = false;
				leftMouseUp = true;
				isMouseCaptured = false;
			}
			if ( event.isRightMouseDownEvent() )
				rightMouseDown = true;
			if ( event.isRightMouseUpEvent() )
				rightMouseDown = false;
            
            break;
        }
        case RBX::InputObject::TYPE_MOUSEMOVEMENT:
        {
            mouseDelta = G3D::Vector2((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
            
            autoMouseMove = false;
            cursorMoved = true;
            
            accumulatedFractionalMouseDelta += mouseDelta * RBX::GameBasicSettings::singleton().getMouseSensitivity();
            mouseDelta.x = (int) accumulatedFractionalMouseDelta.x;
            mouseDelta.y = (int) accumulatedFractionalMouseDelta.y;
            accumulatedFractionalMouseDelta -= mouseDelta;
  
            doWrapMouse(mouseDelta, wrapMouseDelta);

            break;
        }
        case RBX::InputObject::TYPE_MOUSEWHEEL:
        {
            int zDelta = (int) wParam; // wheel rotation
			G3D::Vector2 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
			sendMouseEvent(RBX::InputObject::TYPE_MOUSEWHEEL,
                           RBX::InputObject::INPUT_STATE_CHANGE,
                           RBX::Vector3(pos.x,pos.y,zDelta),
                           RBX::Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0));
            break;
        }
        case RBX::InputObject::TYPE_FOCUS:
        {
            if(eventState == RBX::InputObject::INPUT_STATE_BEGIN)
                DXINPUT_TRACE(std::string("UserInput::WM_SETFOCUS\n").c_str());
            else
                DXINPUT_TRACE(std::string("UserInput::WM_KILLFOCUS\n").c_str());
            
            shared_ptr<RBX::InputObject> event = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_FOCUS,eventState,game->getDataModel().get());
            sendEvent(event);
            break;
        }
        default:
        {
            break;
        }
	}
	
	/////////// DONE PROCESSING ////////////////
    shared_ptr<RBX::DataModel> dataModel = game->getDataModel();
	
	if (wrapMouseDelta != RBX::Vector2::zero())
	{
        if (RBX::Workspace* workspace = dataModel->getWorkspace())
        {
            if (!FFlag::UserAllCamerasInLua || !workspace->getCamera()->hasClientPlayer())
            {
                if ( workspace->getCamera()->getCameraType() != RBX::Camera::CUSTOM_CAMERA )
                {
                    workspace->onWrapMouse(wrapMouseDelta);
                }
            }
        }
        
        sendMouseEvent(RBX::InputObject::TYPE_MOUSEDELTA, RBX::InputObject::INPUT_STATE_CHANGE, RBX::Vector3(getCursorPosition(),0), RBX::Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0));
        sendMouseEvent(RBX::InputObject::TYPE_MOUSEMOVEMENT, RBX::InputObject::INPUT_STATE_CHANGE, RBX::Vector3(getCursorPosition(),0), RBX::Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0));
	}
	else if (cursorMoved)
    {
		if (DFFlag::MouseDeltaWhenNotMouseLocked)
		{
			sendMouseEvent(RBX::InputObject::TYPE_MOUSEMOVEMENT, RBX::InputObject::INPUT_STATE_CHANGE, RBX::Vector3(getCursorPosition(),0), RBX::Vector3(mouseDelta.x, mouseDelta.y, 0));
		}
		else
		{
			sendMouseEvent(RBX::InputObject::TYPE_MOUSEMOVEMENT, RBX::InputObject::INPUT_STATE_CHANGE, RBX::Vector3(getCursorPosition(),0), RBX::Vector3(wrapMouseDelta.x,wrapMouseDelta.y,0));
		}
    }
}

void UserInput::sendMouseEvent(RBX::InputObject::UserInputType mouseEventType, RBX::InputObject::UserInputState mouseEventState, const RBX::Vector3& position, const RBX::Vector3& delta)
{
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

void UserInput::sendEvent(const shared_ptr<RBX::InputObject>& event)
{
    if (shared_ptr<RBX::DataModel> dataModel = game->getDataModel())
    {
        if ( RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(dataModel.get()) )
        {
            inputService->fireInputEvent(event, NULL);
        }
    }
}


bool UserInput::isFullScreenMode() const
{
	return wnd->isFullscreenMode();
}

G3D::Rect2D UserInput::getWindowRect() const			// { return windowRect; }
{
	G3D::Rect2D answer(G3D::Vector2(wnd->width, wnd->height));
	return answer;
}

G3D::Vector2int16 UserInput::getWindowSize() const
{
	G3D::Rect2D windowRect = getWindowRect();
	return G3D::Vector2int16((G3D::int16) windowRect.width(), (G3D::int16) windowRect.height());
}

RBX::TextureProxyBaseRef UserInput::getGameCursor(RBX::Adorn* adorn)
{
    return UserInputBase::getGameCursor(adorn);
}

void UserInput::doWrapHybrid(bool cursorMoved, bool leftMouseUp, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, G3D::Vector2& posToWrapTo)
{
}

bool UserInput::movementKeysDown()
{
	return (	keyDownInternal(RBX::SDLK_w)	|| keyDownInternal(RBX::SDLK_s) || keyDownInternal(RBX::SDLK_a) || keyDownInternal(RBX::SDLK_d)
			||	keyDownInternal(RBX::SDLK_UP)|| keyDownInternal(RBX::SDLK_DOWN) || keyDownInternal(RBX::SDLK_LEFT) || keyDownInternal(RBX::SDLK_RIGHT) );
}

// will be called repeatedly. ignore repeated calls.
void UserInput::onMouseInside()
{
	G3D::Vector2 pos;
	wnd->getCursorPos(&pos);	
	wrapMousePosition = pos - getWindowRect().center();	// i.e. - pull towards the origin - remove chatter

	if(isMouseInside)
		return; // we know. ignore.
	
	RobloxCritSecLoc lock(diSection, "onMouseEnter");
	
	isMouseInside = true;

};

void UserInput::onMouseLeave() 
{ 
	if (!isMouseInside)
		return;

	RobloxCritSecLoc lock(diSection, "onMouseLeave");

	isMouseInside = false;
};

void UserInput::centerCursor()
{
    wrapMousePosition = RBX::Vector2::zero();
    wnd->setCursorPos(getWindowRect().center());
}

G3D::Vector2 UserInput::getCursorPosition()
{
	RobloxCritSecLoc lock(diSection, "getCursorPosition");
    
    G3D::Vector2 pt;
	wnd->getCursorPos(&pt);
	return pt;
}


bool UserInput::keyDownInternal(RBX::KeyCode code) const
{
	if (externallyForcedKeyDown && (code == externallyForcedKeyDown))
    {
		return true;
	}
    
    boost::unordered_map<RBX::KeyCode,shared_ptr<RBX::InputObject> >::const_iterator iter = keyboardInputObjects.find(code);
    if ( iter != keyboardInputObjects.end() )
    {
        if ( iter->second && iter->second.get() )
        {
            return (iter->second->getUserInputState() != RBX::InputObject::INPUT_STATE_END);
        }
    }

	return false;
}


bool UserInput::keyDown(RBX::KeyCode code) const
{
	return keyDownInternal(code);
}

void UserInput::setKeyState(RBX::KeyCode code, RBX::ModCode modCode, char modifiedKey, bool isDown)
{
	RobloxCritSecLoc lock(diSection, "setKeyState");

	externallyForcedKeyDown = isDown ? code : 0;
}


bool preventWrapMouse()
{
	// unlikely to get here
	return true;
}

void UserInput::doWrapMouse(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta)
{
    RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(game->getDataModel().get());
    
    if (!userInputService)
    {
        return;
    }
    
	switch (userInputService->getMouseWrapMode())
	{
    case RBX::UserInputService::WRAP_NONEANDCENTER:
    {
        centerCursor();
    }
    case  RBX::UserInputService::WRAP_NONE: // intentional fall thru
    case  RBX::UserInputService::WRAP_CENTER:
    {
        UserInputUtil::wrapMouseCenter(delta,
            wrapMouseDelta,
            this->wrapMousePosition);
        break;
    }
	case RBX::UserInputService::WRAP_HYBRID:
		{
			UserInputUtil::wrapMousePos(delta,
										wrapMouseDelta,
										wrapMousePosition,
										getWindowSize(),
										posToWrapTo,
										autoMouseMove);
			break;
		}
	case RBX::UserInputService::WRAP_AUTO:
		{
            // 1. If movement keys are down - keep the mouse in the window
            // 2. If the left mouse button is down (we are dragging) - keep in the window
			if (movementKeysDown() || isMouseCaptured)
            {
				UserInputUtil::wrapMouseBorderLock(delta,			
					wrapMouseDelta, 
					this->wrapMousePosition, 
					getWindowSize());
			}
            // 4. OK - we're in PlayMode (i.e. character)
			else if (isFullScreenMode())
            {
				UserInputUtil::wrapFullScreen(delta,
					wrapMouseDelta, 
					this->wrapMousePosition, 
					getWindowSize());
			}
            // We no longer want mouse wrap and camera auto-pan at the horizontal window extents.
			else
            {
				UserInputUtil::wrapMouseNone(delta, 
					wrapMouseDelta,
					this->wrapMousePosition);
			}
			break;
		}
	}
    
    wnd->setCursorPos(getWindowRect().center() + wrapMousePosition);
}
