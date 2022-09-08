#pragma  once

#include <string>
#include <boost/scoped_ptr.hpp>
#include "reflection/Type.h"
#include "reflection/Property.h"
#include "rbx/signal.h"
#include "security/SecurityContext.h"
#include "util/Analytics.h"

namespace boost
{
	namespace filesystem
	{
		class path;
	}
}


namespace RBX { 
	
	class DataModel;
	class Instance;
	class GamePerfMonitor;
	class ProtectedString;

	namespace Network
	{
		class Replicator;
	}

	class GameConfigurer
	{
	public:
		GameConfigurer() {};
		virtual ~GameConfigurer() {}

	protected:
		DataModel* dataModel;
		boost::shared_ptr<const Reflection::ValueTable> parameters;

		void parseArgs(const std::string& args);
		int getParamInt(const std::string& key);
		std::string getParamString(const std::string& key);
		bool getParamBool(const std::string& key);
		void registerPlay(const std::string& key, int userId, int placeId);

		void setupUrls();

	public:
		
		virtual void configure(RBX::Security::Identities identity, DataModel* dm, const std::string& args, int launchMode = -1, const char* vrDevice = 0) = 0;
	};

	class PlayerConfigurer : public GameConfigurer
	{
		bool testing;
		bool logAnalytics;
		bool isTouchDevice;
		bool connectResolved;
		bool connectionFailed;
		bool loadResolved;
		bool joinResolved;
		bool playResolved;
		G3D::RealTime startTime;
		G3D::RealTime playStartTime;
		bool waitingForCharacter;
		int launchMode;

		RBX::Analytics::InfluxDb::Points analyticsPoints;
		boost::shared_ptr<GamePerfMonitor> gamePerfMonitor;

		rbx::signals::scoped_connection playerChangedConnection;
        
        std::vector<rbx::signals::connection> connections;

		void ifSeleniumThenSetCookie(const std::string& key, const std::string& value);
		void showErrorWindow(const std::string& message, const std::string& errorType, const std::string& errorCategory);
		
		void reportError(const std::string& error, const std::string& msg);
		void reportCounter(const std::string& counterNamesCSV, bool blocking);
		void reportStats(const std::string& category, float value);
		void reportDuration(const std::string& category, const std::string& result, double duration, bool blocking);

		void requestCharacter(boost::shared_ptr<Network::Replicator> replicator, boost::shared_ptr<bool> isWaiting);

		void onGameClose();
		void onDisconnection(const std::string& peer, bool lostConnection);
		void onConnectionAccepted(std::string url, boost::shared_ptr<Instance> replicator);
		void onConnectionFailed(const std::string& remoteAddress, int errorCode, const std::string& errorMsg);
		void onConnectionRejected();
		void onReceivedGlobals();
		void onGameLoaded(boost::shared_ptr<bool> isWaiting);
		void onPlayerIdled(double time);
		void onPlayerChanged(const Reflection::PropertyDescriptor* propertyDescriptor);

		void setMessage(const std::string& msg);

	public:
		PlayerConfigurer();
		~PlayerConfigurer();

		/*override*/ void configure(RBX::Security::Identities identity, DataModel* dm, const std::string& args, int launchMode = -1, const char* vrDevice = 0);

	};

	class StudioConfigurer : public GameConfigurer
	{
	private:
		bool findModulesAndLoad(const std::string& baseModulePath, const boost::filesystem::path& dir_path, boost::unordered_map<std::string, ProtectedString>& coreModules);
		void loadCoreModules();
	public:
		StudioConfigurer() {}
		~StudioConfigurer() {}

		std::string starterScript;
		/*override*/ void configure(RBX::Security::Identities identity, DataModel* dm, const std::string& args, int launchMode = -1, const char* vrDevice = 0);

	};


}
