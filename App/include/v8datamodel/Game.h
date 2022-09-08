#pragma once

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "rbx/signal.h"
#include <vector>
#include <string>
#include "security/SecurityContext.h"

namespace RBX {

	class Verb;
	class CommonVerbs;
	class DataModel;
	class GameConfigurer;

	// Encapsulates the creation of a DataModel used by client apps
	class Game : boost::noncopyable
	{
	protected:
		Game(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen = false);

		bool hasShutdown;

		shared_ptr<GameConfigurer> gameConfigurer;

		boost::shared_ptr<DataModel> dataModel;

	public:
		static void globalInit(bool isStudio);
		static void globalExit();

		std::vector<Verb*> verbs;
		boost::shared_ptr<CommonVerbs> commonVerbs;
        
        boost::shared_ptr<DataModel> getDataModel() const { return dataModel; }

		void shutdown();

		virtual ~Game(void);

		void setupDataModel(const std::string& baseUrl);
      
		bool getSuppressNavKeys();

		void configurePlayer(RBX::Security::Identities identity, const std::string& params, int launchMode = -1, const char* vrDevice = 0);
    private:
        void doClearVerbs();
		void clearVerbs(bool needsLock = true);
	};

	class SecurePlayerGame : public Game
	{
	public:
		SecurePlayerGame(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen = true);
	};

	class UnsecuredStudioGame : public Game
	{
	public:
		UnsecuredStudioGame(Verb* lockVerb, const char* baseUrl, bool isNetworked = false, bool showLoadingScreen = false);
	};

}  // namespace RBX
