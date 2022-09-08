#include "stdafx.h"

#include "Document.h"
#include "FunctionMarshaller.h"
#include "GameVerbs.h"
#include "LogManager.h"
#include "resource.h"
#include "script/ScriptContext.h"
#include "util/Statistics.h"
#include "util/RobloxGoogleAnalytics.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/Game.h"
#include "V8DataModel/GuiService.h"
#include "V8DataModel/HackDefines.h"
#include "V8DataModel/UserInputService.h"
#include "V8DataModel/UserController.h"
#include "Network/Api.h"
#include "InitializationError.h"
#include "View.h"
#include "RbxWebView.h"

#include "VMProtect/VMProtectSDK.h"

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)

namespace RBX {

Document::Document() : marshaller(NULL)
{
}

Document::~Document()
{
}

void Document::Start(HttpFuture& scriptResult, const SharedLauncher::LaunchMode launchMode, bool isTeleport, const char* vrDevice)
{
	startedSignal(isTeleport);

	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Starting script");
	SetUiMessage(""); // clear UI message

	// TODO: VMProtect this section 
	executeScript(scriptResult, launchMode, vrDevice);

	if (!isTeleport)
		RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, GA_CLIENT_START, Time::nowFast().timestampSeconds() * 1000, "Join script executed");
}

static void setUiMessageImpl(shared_ptr<DataModel> dm, const std::string& message)
{
	if (message.length() > 0)
	{
		dm->setUiMessage(message);
	}
	else
	{
		dm->clearUiMessage();
	}

	if (GuiService*gs = dm->create<GuiService>())
		gs->setUiMessage(GuiService::UIMESSAGE_INFO, message);
}

void Document::SetUiMessage(const std::string& message)
{
	if (shared_ptr<DataModel> dm = game->getDataModel())
	{
		dm->submitTask(boost::bind(setUiMessageImpl, dm, message), DataModelJob::Write);
	}
}

void Document::PrepareShutdown()
{
    // give scripts a deadline to finish
    if (game && game->getDataModel()) 
        if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
            if (ScriptContext* scriptContext = game->getDataModel()->find<ScriptContext>())
                scriptContext->setTimeout(FLog::PlayerShutdownLuaTimeoutSeconds);
}


void Document::Shutdown()
{
	if (marshaller)
		FunctionMarshaller::ReleaseWindow(marshaller);

    if (game)
    {
        game->shutdown();
        game.reset();
    }
}



void Document::configureDataModelServices(bool useChat, RBX::DataModel* dataModel)
{
	if(!dataModel)
		return;

	DataModel::LegacyLock lock(dataModel, DataModelJob::Write);


	// Inform the UserInputService what kind of input we are providing (this may have to change if we use this with windows 8 touch devices)
	if(UserInputService* userInputService = dataModel->find<UserInputService>())
	{
		userInputService->setKeyboardEnabled(true);
		userInputService->setMouseEnabled(true);
	}
}

void Document::Initialize(HWND hWnd, bool useChat)
{
	marshaller = FunctionMarshaller::GetWindow();
	game.reset(new RBX::SecurePlayerGame(NULL, GetBaseURL().c_str()));

	configureDataModelServices(useChat, game->getDataModel().get());

	DataModel::LegacyLock lock(game->getDataModel().get(), DataModelJob::Write);

	if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
		game->getDataModel()->create<ScriptContext>();

	game->getDataModel().get()->gameLoadedSignal.connect(boost::bind(&Document::gameIsLoaded,this));
}

void Document::gameIsLoaded()
{
	MainLogManager::getMainLogManager()->setGameLoaded();
}

// Executes the 'script' as part of the game initialization.
void Document::executeScript(HttpFuture& scriptResult, const SharedLauncher::LaunchMode launchMode, const char* vrDevice) const
{
	shared_ptr<RBX::DataModel> dataModel = game->getDataModel();

#if !defined(LOVE_ALL_ACCESS) && !defined(_NOOPT) && !defined(_DEBUG) && !defined(RBX_STUDIO_BUILD)
	dataModel->addHackFlag(HATE_DEBUGGER *
		VMProtectIsDebuggerPresent(true /*check for kernel debuggers too*/));
#endif

	Security::Impersonator impersonate(Security::COM);
	std::string data;

    try
	{
        data = scriptResult.get();
    }
	catch(const std::exception& e)
	{
		std::string err = RBX::format("Exception occured in Document::executeScript: %s", e.what());
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, err.c_str());

		if (GuiService*gs = dataModel->create<GuiService>())
			gs->setUiMessage(GuiService::UIMESSAGE_INFO, "Unable to join game. Please try again later.");

		return;
	}

#if !defined(LOVE_ALL_ACCESS) && !defined(_NOOPT) && !defined(_DEBUG) && !defined(RBX_STUDIO_BUILD)
	dataModel->addHackFlag(HATE_DEBUGGER *
		VMProtectIsDebuggerPresent(true /*check for kernel debuggers too*/));
#endif
	ProtectedString verifiedSource;
	try
	{
        verifiedSource = ProtectedString::fromTrustedSource(data);
        ContentProvider::verifyScriptSignature(verifiedSource, true);
	}
	catch(std::bad_alloc& e)
	{
		std::string err = RBX::format("Exception occured in Document::executeScript: %s", e.what());
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, err.c_str());
		throw;
	}
	catch(std::exception& e)
	{
		std::string err = RBX::format("Exception occured in Document::executeScript: %s", e.what());
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, err.c_str());
		return;
	}

	if (dataModel->isClosed())
		return;

	int firstNewLineIndex = data.find("\r\n");
	if (data[firstNewLineIndex+2] == '{')
	{
		game->configurePlayer(Security::COM, data.substr(firstNewLineIndex+2), launchMode, vrDevice);
	}
	else
	{
		ScriptContext* context = dataModel->create<ScriptContext>();
		context->executeInNewThread(Security::COM, verifiedSource, "Start Game");
	}
}

FunctionMarshaller* Document::GetMarshaller() const
{
	return marshaller;
}

std::string Document::GetSEOStr() const
{
	boost::shared_ptr<DataModel> dataModel = game->getDataModel();
	std::string seo;
	if (dataModel)
	{
		std::string seo = dataModel->getScreenshotSEOInfo();
		if (!seo.empty())
			return seo;
	}

	const int MAX_LOAD_STRING = 100;
	char defaultImageInfo[MAX_LOAD_STRING];
	LoadStringA(GetModuleHandle(NULL), IDS_DEFAULT_IMAGE_INFO, defaultImageInfo, MAX_LOAD_STRING);
	return std::string(defaultImageInfo);
}

}  // namespace RBX
