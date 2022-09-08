#pragma once
#include "rbx/signal.h"
#include "SharedLauncher.h"
#include "RbxWebView.h"
#include "util/HttpAsync.h"
namespace RBX {

// Forward declarations
class FunctionMarshaller;
class Game;
class View;
class PlayerConfigurer;

// Class responsible for the game state
class Document
{
public:

	rbx::signal<void(bool)> startedSignal;

	Document();
	~Document();

	void Initialize(HWND hWnd, bool useChat);
	void Start(HttpFuture& scriptResult, const SharedLauncher::LaunchMode launchMode, bool isTelport, const char* vrDevice);
	void Shutdown();
	void SetUiMessage(const std::string& message);
    void PrepareShutdown(); // call before destroying the view

	FunctionMarshaller* GetMarshaller() const;
	std::string GetSEOStr() const;

	boost::shared_ptr<Game> getGame()
    {
        return game;
    }

private:
	FunctionMarshaller* marshaller;

	// The game to run.
	boost::shared_ptr<Game> game;

	// Executes the 'script' as part of the game initialization.
	void executeScript(HttpFuture& scriptResult, const SharedLauncher::LaunchMode launchMode, const char* vrDevice) const;

	void configureDataModelServices(bool useChat, RBX::DataModel* dataModel);

	void dataModelDidRestart();
	void dataModelWillShutdown();
	void gameIsLoaded();
};

}  // namespace RBX
