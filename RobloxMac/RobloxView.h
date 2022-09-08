#pragma once

#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "v8datamodel/game.h"
#include "Util/KeyCode.h"
#include "G3D/Vector2.h"
#include "rbx/signal.h"

namespace RBX
{
	class DataModel;
	class ViewBase;
	class FunctionMarshaller;
	namespace Tasks
	{
		class Sequence;
	}
	
	namespace Reflection
	{
		class PropertyDescriptor;
	}	
}

class UserInput;

class RobloxView
{

	boost::scoped_ptr<RBX::ViewBase> view;
	boost::shared_ptr<RBX::Game> game;
	boost::scoped_ptr<UserInput> userInput;
	boost::scoped_ptr<class LeaveGameVerb> leaveGameVerb;
	boost::scoped_ptr<RBX::Verb> fullscreenVerb;
	boost::scoped_ptr<RBX::Verb> studioVerb;
	boost::scoped_ptr<RBX::Verb> screenshotVerb;
	boost::shared_ptr<class ShutdownClientVerb> shutdownClientVerb;
	
	RBX::FunctionMarshaller* marshaller;
	
	rbx::signals::scoped_connection placeIDChangeConnection;
	
	void *viewWindow;
	void *appWindow;

	boost::shared_ptr<RBX::Tasks::Sequence> sequence;

	class RenderJob;
	class UserInputJob;
	boost::shared_ptr<RenderJob> renderJob;
	boost::shared_ptr<UserInputJob> userInputJob;

	static boost::shared_ptr<RobloxView> rbxView;

	static boost::shared_ptr<RBX::Game> startGame(std::string urlScript, const bool isApp);

	void doTeleport(std::string url, std::string ticket, std::string script);
	
	void onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc);
    
    void doUnbindWorkspace();
    
    void initializeInput();
    
public:

	// N.B.: These are clones of other Roblox defines, need to be here to get around Objective-C vs. Cpp issues.
	// The values aren't literally duplicated, they are transduced by switch statements. So this is safe-- but annoying. --TP
	enum EventType {	NO_EVENT,
						MOUSE_RIGHT_BUTTON_DOWN,
						MOUSE_RIGHT_BUTTON_UP, 
						MOUSE_LEFT_BUTTON_DOWN,
						MOUSE_LEFT_BUTTON_UP, 
						MOUSE_MOVE,
						MOUSE_DELTA,
						MOUSE_IDLE,		// during runtime, this event is guaranteed every step if no other mouse event
						MOUSE_WHEEL_FORWARD,
						MOUSE_WHEEL_BACKWARD,
						KEY_DOWN,
						KEY_UP,
						KILLFOCUS,
						SETFOCUS,
						// these can come later - for now, when moving out of a window, 
						// just make sure a mouse up comes along if the mouse was previously down...??
					//	MOUSE_DOWN_DOUBLE_CLICK, 
					//	MOUSE_MOVE_OUT_WINDOW
						MOUSE_MIDDLE_BUTTON_DOWN,
						MOUSE_MIDDLE_BUTTON_UP
					};

	RobloxView(void *viewwnd, void *appwnd, boost::shared_ptr<RBX::Game> game);
	~RobloxView(void);
	
	void handleUserMessage(EventType event, unsigned int wParam, unsigned int lParam);
	void handleFocus(bool focus);
	void handleMouseInside(bool inside);
	void handleMouse(EventType event, int x, int y, unsigned int modifiers);
	void handleKey(EventType event, RBX::KeyCode keyCode, RBX::ModCode modifiers);
	void handleScrollWheel(float delta, int x, int y);
	void processInput();

	// request a shutdown from the game
	Boolean requestShutdownClient();
	
	// Some Cocoa/windowing help (methods defined in RobloxView.mm)
	void getCursorPos(G3D::Vector2 *pPos);
    void setCursorPos(G3D::Vector2 pPos);
	void marshalTeleport(std::string url, std::string ticket, std::string script);
	
	// Game verbs
	void leaveGame();
	void handleLeaveGame();
	void shutdownClient();
	void handleShutdownClient();
	void toggleFullScreen();
	void handleToggleFullScreen();
    
    bool isFullscreenMode();
    
    void doShutdownDataModel();
    
    void stopJobs();
	
	unsigned int width;
	unsigned int height;
	void setBounds(unsigned int width, unsigned int height);

	static RobloxView *start_game(void *wnd, void *appwnd, std::string urlScript, const bool isApp);
    static RobloxView *init_game(void *wnd, void* appwnd, const bool isApp);
    
    void executeJoinScript(const std::string& urlScript, const bool isApp, const bool isFromProtocolHandler);
    void setUIMessage(const std::string& message);
	
	boost::shared_ptr<RBX::DataModel> getDataModel() { return game ? game->getDataModel() : boost::shared_ptr<RBX::DataModel>(); }
private:
	void bindToWorkspace();
	void defineConcurrencyRules();
    void setUIMessage_(const std::string& message);
};
