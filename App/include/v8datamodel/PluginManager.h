#pragma once

#include "Util/AsyncHttpQueue.h"
#include "V8Tree/Instance.h"
#include "V8DataModel/PluginMouse.h"
#include "V8Tree/Service.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "StudioPluginHost.h"
#include "Tool/ToolsArrow.h"

LOGGROUP(Plugins);

namespace RBX {
    class PluginManager;
	class Toolbar;

	extern const char *const sButton;
	class Button 
		: public DescribedNonCreatable<Button, Instance, sButton>
	{
		void *id;
		IStudioPluginHost *host;
		Toolbar *toolbar;
		bool isDeleted;
		bool active;
	public:
		Button();

		void SetId(void *id) { this->id = id; }
		void setToolbar(Toolbar *toolbar);
		void setHost(IStudioPluginHost *host) { this->host = host; }
		void markDeleted();
		void processIconLoaded(ContentId id, AsyncHttpQueue::RequestResult result,
			std::istream* stream, shared_ptr<const std::string> response);
		
		void setActive(bool value);

		rbx::signal<void()> clickSignal;
	};

	extern const char *const sToolbar;
	class Toolbar 
		: public DescribedNonCreatable<Toolbar, Instance, sToolbar>
	{
		IStudioPluginHost *host;
		DataModel* dataModel;
		void *id;
		bool isDeleted;
		
		typedef std::map<void*, shared_ptr<Button> >::iterator buttonsIter;
		std::map<void*, shared_ptr<Button> > buttonsMap;
	public:
		Toolbar();

		void SetId(void *id) { this->id = id; }
		void *getId() const { return id; }
		void setHost(IStudioPluginHost *host) { this->host = host; }
		void setDataModel(DataModel* dataModel) { this->dataModel = dataModel; }
		DataModel* getDataModel() const { return dataModel; }
		void reset();
		void markDeleted();

		shared_ptr<Instance> createButton(std::string text, std::string tooltip, std::string iconName);
		Button *getButton(void *id);
	};

	extern const char *const sPlugin;
	class Plugin
		: public DescribedNonCreatable<Plugin, Instance, sPlugin>
	{
	private:
		PluginManager *pluginManager;
		DataModel *dataModel;
		shared_ptr<Instance> luaMouse;
		PluginMouse *mouse;
		bool active;
		bool tool;
		int assetId;
	public:
		Plugin();
		~Plugin();

		void setPluginManager(PluginManager *pluginManager_) { pluginManager = pluginManager_; }
		void setDataModel(DataModel *dataModel);
		DataModel *getDataModel() const { return dataModel; }
		bool isTool() const { return tool; };

		PluginMouse *getMouse() const { return mouse; }
		shared_ptr<Instance> getMouseLua();
		shared_ptr<Instance> createToolbar(std::string name);
        void saveSelectedToRoblox();

        void setSetting(std::string key, Reflection::Variant value);
        Reflection::Variant getSetting(std::string key);
		int getStudioUserId();
		bool isCollisionOn() const;
		float getGridSize() const;
		AdvArrowToolBase::JointCreationMode getJoinMode();

		void activate(bool exclusiveMouse);
		void setActive(bool state);
		rbx::signal<void()> deactivationSignal;

		void setAssetId(int assetId);

		void openScriptDoc(shared_ptr<RBX::Instance> script, int lineNumber);
		void openWikiPage(std::string url);

        shared_ptr<Instance> csgUnion( shared_ptr<const Instances> instances );
        shared_ptr<const Instances> csgNegate( shared_ptr<const Instances> instances );
        shared_ptr<const Instances> csgSeparate( shared_ptr<const Instances> instances );

		void promptForExistingAssetId(std::string assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
	};

	extern const char *const sPluginManager;
	class PluginManager
		: public DescribedNonCreatable<PluginManager, Instance, sPluginManager>,
		IHostNotifier
	{
	private:
		std::string lastPluginPath;
		
		// this dataModel refers to the currently selected dataModel (as in the selected tab in studio)
		DataModel *currentDataModel;
		IStudioPluginHost *studioHost;

		struct StateDataEntry
		{
			typedef std::map<std::string, shared_ptr<Toolbar> >::iterator toolbarsIter;

			StateDataEntry() : active(NULL), hidden(false) {}

			std::map<std::string, shared_ptr<Toolbar> > toolbars;
			std::list<shared_ptr<Plugin> > plugins;
			Plugin *active;
			bool hidden;

			void addPlugin(shared_ptr<Plugin> p) { plugins.push_back(p); }
			void setActivePlugin(Plugin *p) { active = p; p->setActive(true); }
			shared_ptr<Instance> getToolbar(std::string name, IStudioPluginHost *host, DataModel* dataModel);
			void fireButtonClick(void *id);
			void disableStudioUI(bool hide, IStudioPluginHost *host);
			void hideStudioUI(bool hide, IStudioPluginHost *host);
			void deleteStudioUI(IStudioPluginHost *host);
		};

		typedef std::map<DataModel*, StateDataEntry>::iterator stateIter;
		typedef std::list<shared_ptr<Plugin> > pluginsList;
		typedef std::list<shared_ptr<Plugin> >::iterator pluginsIter;
		typedef std::pair<DataModel*, StateDataEntry> statePair;
		std::map<DataModel*, StateDataEntry> state;
	public:
		PluginManager();
		static shared_ptr<PluginManager> singleton();
		boost::mutex mutex;
		rbx::signal<void()> allPluginsDeactivatedSignal;

		virtual shared_ptr<Instance> createToolbar(Plugin *plugin, std::string name);
		virtual void activate(Plugin *plugin, DataModel *dataModel);
		virtual void DeactivatePlugins();      

		virtual void buttonClick(DataModel *dataModel, void *id);

		void setStudioHost(IStudioPluginHost *host) { studioHost = host; host->setNotifier(this); }
		IStudioPluginHost* getStudioHost() { return studioHost; }
		void setCurrentDataModel(DataModel *dataModel) { currentDataModel = dataModel; }

		std::string getLastPath() const { return lastPluginPath; }
		void setLastPath(std::string path) { lastPluginPath = path; }
		void addModelPlugin(DataModel* dataModel, Instances instances, int assetId);
		void startModelPluginScripts(DataModel* dataModel);

		int createPlugin(lua_State* L);
		void deletePlugins(DataModel *dataModel);
		void deleteStudioUI(DataModel *dataModel);

		Plugin *getActivePlugin(DataModel *dataModel);

		void hideStudioUI(DataModel *dataModel, bool hide);
		void disableStudioUI(DataModel *dataModel, bool hide);

		void exportPlace(std::string filePath);
		void exportSelection(std::string filePath);

		virtual void fireDragEnterEvent(DataModel* dataModel, shared_ptr<const RBX::Instances> instances, Vector2 location);
	};
};
		
