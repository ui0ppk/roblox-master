#include "stdafx.h"

#include "v8datamodel/Game.h"
#include "v8datamodel/factoryregistration.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/commonverbs.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/FastLogSettings.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/UserInputService.h"
#include "v8world/World.h"
#include "v8datamodel/Teams.h"
#include "v8datamodel/SpawnLocation.h"
#include "V8World/ContactManager.h"
#include "v8datamodel/ChangeHistory.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/Test.h"
#include "v8datamodel/JointsService.h"
#include "v8datamodel/HttpRbxApiService.h"

#include "v8world/Block.h"

#include "Util/ScriptInformationProvider.h"
#include "Util/Profiling.h"

#include "Gui/ProfanityFilter.h"
#include "script/LuaSettings.h"
#include "network/api.h"
#include "network/GameConfigurer.h"

#include "v8datamodel/TextButton.h"
#include "v8datamodel/ImageButton.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/UserController.h"
#include "script/ScriptContext.h"
#include "FastLog.h"

DYNAMIC_FASTFLAGVARIABLE(PersistenceCurlCookies, false)

namespace RBX {

	static boost::shared_ptr<ProfanityFilter> s_profanityFilter;

	SecurePlayerGame::SecurePlayerGame(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen)
		:Game(lockVerb, baseUrl, shouldShowLoadingScreen)
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
#if 1
		boost::call_once(&Network::initWithPlayerSecurity, flag);
#else
		// For testing security. Don't ship with this!
		boost::call_once(&Network::initWithServerSecurity, flag);
#endif
	}

	UnsecuredStudioGame::UnsecuredStudioGame(Verb* lockVerb, const char* baseUrl, bool isNetworked, bool showLoadingScreen)
		:Game(lockVerb, baseUrl, showLoadingScreen)
	{
        if (isNetworked)
		{
            static boost::once_flag flag = BOOST_ONCE_INIT;
		    boost::call_once(&Network::initWithoutSecurity, flag);
        }
	}

	void Game::globalInit(bool isStudio)
	{
		RBX::Http::CookieSharingPolicy cookieSharingPolicy;
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
        cookieSharingPolicy = RBX::Http::CookieSharingSingleProcessMultipleThreads;
#elif defined(RBX_PLATFORM_DURANGO)
		cookieSharingPolicy = RBX::Http::CookieSharingSingleProcessMultipleThreads;
#elif defined(_WIN32) || defined(__APPLE__)
	if (DFFlag::PersistenceCurlCookies)
	{
        cookieSharingPolicy = RBX::Http::CookieSharingMultipleProcessesWrite;
        if (isStudio)
        {
            cookieSharingPolicy |= RBX::Http::CookieSharingMultipleProcessesRead;
        }
	}
	else
		cookieSharingPolicy = RBX::Http::CookieSharingSingleProcessMultipleThreads;


#else
#error Unsupported platform.
#endif

#if defined(RBX_PLATFORM_DURANGO)
		Http::init(Http::XboxHttp, cookieSharingPolicy);
#elif defined(_WIN32)
		Http::init(Http::WinInet, cookieSharingPolicy);
#else
		Http::init(Http::WinHttp, cookieSharingPolicy);
#endif

		Profiling::init(false);
		static FactoryRegistrator registerFactoryObjects;

		Analytics::InfluxDb::init(); // initialize the random number used for throttle

		s_profanityFilter = ProfanityFilter::getInstance();

		GlobalAdvancedSettings::singleton();
		GameSettings::singleton();
		LuaSettings::singleton();
		DebugSettings::singleton();
		PhysicsSettings::singleton();

		// Initialize Block's static data as soon as possible
		// to make sure it doesn't get destroyed before other static
		// objects try to use it.
		Block::init();
	}

	void Game::globalExit()
	{
		s_profanityFilter.reset();
	}
    
	void Game::setupDataModel(const std::string& baseUrl)
    {        
		dataModel->create<ScriptInformationProvider>()->setAssetUrl(baseUrl + "/asset/");
		dataModel->create<ContentProvider>()->setBaseUrl(baseUrl);

		dataModel->setGame(this);
        
        commonVerbs.reset(new CommonVerbs(dataModel.get()));
        
		// verb container will destroy
		verbs.push_back(new CameraPanLeftCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraPanRightCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraTiltUpCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraTiltDownCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraZoomInCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraZoomOutCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraCenterCommand(dataModel->getWorkspace()));
		verbs.push_back(new CameraZoomExtentsCommand(dataModel->getWorkspace()));
		verbs.push_back(new ToggleViewMode(dataModel.get()));

		// record settings
		UserInputService *uiService = dataModel->create<UserInputService>();
		bool touchEnabled = false;
		if (uiService) {
			switch (uiService->getPlatform()) {
				case UserInputService::PLATFORM_XBOXONE:
				case UserInputService::PLATFORM_WINDOWS:
				case UserInputService::PLATFORM_OSX:
				case UserInputService::PLATFORM_NONE:
				default:
					touchEnabled = false;
					break;
				case UserInputService::PLATFORM_IOS:
				case UserInputService::PLATFORM_ANDROID:
					touchEnabled = true;
					break;
			}
		}
		RBX::GameBasicSettings::singleton().recordSettingsInGA(touchEnabled);
    }

	Game::Game(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen) :
		hasShutdown(false)
	{
	    dataModel = DataModel::createDataModel(true, lockVerb, shouldShowLoadingScreen);
		dataModel->submitTask(boost::bind(&Game::setupDataModel, this, std::string(baseUrl)), DataModelJob::Write);
	}

	Game::~Game(void)
	{
		shutdown();
	}
    
    void Game::doClearVerbs()
    {
        if (!dataModel)
			return;
        
        Security::Impersonator impersonate(Security::COM);
		{
			commonVerbs.reset();
            
			std::vector<Verb*>::iterator iter = verbs.begin();
			std::vector<Verb*>::iterator end = verbs.end();
			while (iter!=end)
			{
				delete *iter;
				++iter;
			}
			verbs.clear();
		}
    }

	void Game::clearVerbs(bool needsLock)
	{
		if(needsLock)
		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			doClearVerbs();
		}
		else
			doClearVerbs();
	}

	template<class T> void shutdownDM(shared_ptr<T>& dataModelToShutdown)
	{
		if(!dataModelToShutdown)
			return;

		{
			// Ensure that all content is wiped out
			DataModel::closeDataModel(dataModelToShutdown);
		}

		// Now release the DataModel
		dataModelToShutdown.reset();
	}

	void Game::shutdown()
	{
		if(hasShutdown)
			return;

		hasShutdown = true;

		clearVerbs(true);

		shutdownDM(dataModel);
	}

	bool Game::getSuppressNavKeys()
	{
		bool suppress = false;

		if (dataModel)
			suppress = dataModel->getSuppressNavKeys();

		return suppress;
	}

	void Game::configurePlayer(RBX::Security::Identities identity, const std::string& params, int launchMode, const char* vrDevice)
	{
		gameConfigurer.reset(new PlayerConfigurer());
		gameConfigurer->configure(identity, dataModel.get(), params, launchMode, vrDevice);
	}
}  // namespace RBX
