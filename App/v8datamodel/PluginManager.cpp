/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PluginManager.h"

#include "Script/Script.h"
#include "Script/ScriptContext.h"
#include "Util/FileSystem.h"
#include "Util/standardout.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/DataModel.h"
#include "V8Xml/SerializerV2.h"
#include "V8Xml/Serializer.h"
#include "Tool/ToolsArrow.h"

#include <luaconf.h>
#include "script/LuaInstanceBridge.h"

const char* const RBX::sPluginManager = "PluginManager";
const char* const RBX::sPlugin = "Plugin";
const char *const RBX::sToolbar = "Toolbar";
const char *const RBX::sButton = "Button";

using namespace RBX;

REFLECTION_BEGIN();
// Reflections
static Reflection::CustomBoundFuncDesc<PluginManager, shared_ptr<Instance>()> func_createPlugin(&PluginManager::createPlugin, "CreatePlugin", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, void(std::string)> func_openWikiPage(&Plugin::openWikiPage, "OpenWikiPage", "url", Security::Plugin);

static Reflection::BoundFuncDesc<Plugin, shared_ptr<Instance>()> func_getMouseLua(&Plugin::getMouseLua, "GetMouse", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, void(bool)> func_activate(&Plugin::activate, "Activate", "exclusiveMouse", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, shared_ptr<Instance>(std::string)> func_createToolbar(&Plugin::createToolbar, "CreateToolbar", "name", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, void(std::string, Reflection::Variant)> func_setSetting(&Plugin::setSetting, "SetSetting",
    "key", "value", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, Reflection::Variant(std::string)> func_getSetting(&Plugin::getSetting, "GetSetting",
    "key", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, int()> func_getStudioUserId(&Plugin::getStudioUserId, "GetStudioUserId", Security::Plugin);
static Reflection::EventDesc<Plugin, void() > desc_Plugin_Deactivation(&Plugin::deactivationSignal, "Deactivation", Security::Plugin);
static Reflection::BoundFuncDesc<Plugin, void(void)> func_saveSelectedToRoblox(&Plugin::saveSelectedToRoblox, "SaveSelectedToRoblox", Security::Plugin);

//CSG
static Reflection::BoundFuncDesc<Plugin, shared_ptr<Instance>( shared_ptr<const Instances> )> plugin_Union( &Plugin::csgUnion, "Union", "objects", Security::Plugin );
static Reflection::BoundFuncDesc<Plugin, shared_ptr<const Instances>( shared_ptr<const Instances> )> plugin_Negate( &Plugin::csgNegate, "Negate", "objects", Security::Plugin );
static Reflection::BoundFuncDesc<Plugin, shared_ptr<const Instances>( shared_ptr<const Instances> )> plugin_Separate( &Plugin::csgSeparate, "Separate", "objects", Security::Plugin );

static Reflection::BoundYieldFuncDesc<Plugin, int(std::string) > func_promptForExistingAssetId(&Plugin::promptForExistingAssetId, "PromptForExistingAssetId", "assetType", Security::Plugin);

static Reflection::BoundFuncDesc<Toolbar, shared_ptr<Instance>(std::string, std::string, std::string)> func_createButton(&Toolbar::createButton, "CreateButton", "text", "tooltip", "iconname", Security::Plugin);

static Reflection::BoundFuncDesc<Button, void(bool)> func_setButtonActive(&Button::setActive, "SetActive", "active", Security::Plugin);
static Reflection::EventDesc<Button, void()>  desc_buttonClick(&Button::clickSignal, "Click", Security::Plugin);

static Reflection::BoundFuncDesc<Plugin, void(shared_ptr<RBX::Instance>, int)>	func_openScript(&Plugin::openScriptDoc, "OpenScript", "script", "lineNumber", 0, Security::Plugin);

static Reflection::BoundFuncDesc<PluginManager, void(std::string) > func_exportPlace(&PluginManager::exportPlace, "ExportPlace", "filePath", "", Security::Plugin);
static Reflection::BoundFuncDesc<PluginManager, void(std::string) > func_exportSelection(&PluginManager::exportSelection, "ExportSelection", "filePath", "", Security::Plugin);

static Reflection::PropDescriptor<Plugin, bool> prop_CollisionActive("CollisionEnabled", category_Data, &Plugin::isCollisionOn, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Plugin, float> prop_GridSize("GridSize", category_Data, &Plugin::getGridSize, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::BoundFuncDesc<Plugin, AdvArrowTool::JointCreationMode(void)> func_getJoinMode(&Plugin::getJoinMode, "GetJoinMode", Security::Plugin);
REFLECTION_END();

namespace
{
	void runAllPluginScriptsHelper(ScriptContext* scriptContext, shared_ptr<Plugin> plugin,
		shared_ptr<Instance> descendant)
	{
		shared_ptr<Script> script = Instance::fastSharedDynamicCast<Script>(descendant);
		if (script != NULL && !script->isDisabled())
		{
			std::map<std::string, shared_ptr<Instance> > extraGlobals;
			extraGlobals["script"] = script;
			extraGlobals["plugin"] = plugin;

#if defined(RBX_STUDIO_BUILD)
			try {
				scriptContext->executeInNewThreadWithExtraGlobals(
                    Security::StudioPlugin,
					script->getEmbeddedCodeSafe(),
					script->getFullName().c_str(),
					extraGlobals);
			}
			catch (std::exception &e)
			{
				StandardOut::singleton()->print(MESSAGE_ERROR, e.what());
			}
#endif
		}
	}
}

Button::Button() : isDeleted(false),
	active(false)
{

}

void Button::setToolbar(Toolbar *toolbar)
{
	this->toolbar = toolbar;
}

void Button::setActive(bool value)
{
	if (isDeleted) 
		return;

	if (value)
		toolbar->reset();

	if(!value && active && PluginManager::singleton()->getActivePlugin(toolbar->getDataModel()))
		PluginManager::singleton()->DeactivatePlugins();

	active = value;
	host->setButtonActive(id, value);
}

void Button::markDeleted()
{
	isDeleted = true;
}

void Button::processIconLoaded(ContentId contentId, AsyncHttpQueue::RequestResult result,
	std::istream* stream, shared_ptr<const std::string> unused)
{
	if (isDeleted)
	{
		return;
	}

	std::string responseContent;
	if (stream)
	{
		responseContent = std::string(std::istreambuf_iterator<char>(*stream), std::istreambuf_iterator<char>());
	}

	if (result != AsyncHttpQueue::Succeeded || responseContent.empty())
	{
		host->buttonIconFailedToLoad(id);
		// TODO: better error message
		StandardOut::singleton()->print(MESSAGE_ERROR, "Unable to load plugin icon.");
		return;
	}

	host->setButtonIcon(id, responseContent);
}

Toolbar::Toolbar()
	: host(NULL)
	, dataModel(NULL)
	, id(NULL)
	, isDeleted(false)
{
}

shared_ptr<Instance> Toolbar::createButton(std::string text, std::string tooltip, std::string iconName)
{
	shared_ptr<Button> b = Creatable<Instance>::create<Button>();

	std::string path;
	bool fetchIconFromAssetUrl = ContentProvider::isUrl(iconName);
	if (!iconName.empty() && !fetchIconFromAssetUrl)
	{
		path = PluginManager::singleton()->getLastPath();
		if (!path.empty())
		{
			path += "\\";
			path += iconName;
		}
	}
	if (fetchIconFromAssetUrl)
	{
		// set loading button
		path = ContentProvider::getAssetFile("textures/chatBubble_bot_notifyGray_dotDotDot.png");
	}

	void *buttonId = host->createButton(id, text, tooltip, path);
	b->SetId(buttonId);
	b->setToolbar(this);
	b->setHost(host);

	buttonsMap[buttonId] = b;

	if (fetchIconFromAssetUrl)
	{
		ContentProvider* contentProvider = dataModel->create<ContentProvider>();
		try
		{
			ContentId contentId(iconName);
			contentProvider->getContent(contentId, ContentProvider::PRIORITY_GUI,
				boost::bind(&Button::processIconLoaded, b,
					contentId, _1, _2, _3), AsyncHttpQueue::AsyncWrite);
		}
		catch (const std::runtime_error&)
		{
			host->buttonIconFailedToLoad(buttonId);
			// ContentProvider can throw when we try to fetch the icon. Inform the user.
			StandardOut::singleton()->printf(MESSAGE_ERROR,
				"Encountered error while loading plugin icon %s", iconName.c_str());
		}
	}

	return b;
}

Button *Toolbar::getButton(void *id)
{
	buttonsIter i = buttonsMap.find(id);
	if (i != buttonsMap.end())
	{
		return i->second.get();
	}

	return NULL;
}

void Toolbar::reset()
{
	for (buttonsIter i = buttonsMap.begin(); i != buttonsMap.end();i ++)
	{
		i->second->setActive(false);
	}
}

void Toolbar::markDeleted()
{
	for (buttonsIter i = buttonsMap.begin(); i != buttonsMap.end();i ++)
	{
		i->second->markDeleted();
	}
	isDeleted = true;
}

// Plugin
Plugin::Plugin()
	: tool(false)
	, assetId(0)
{
}

Plugin::~Plugin()
{
}

void Plugin::setDataModel(DataModel *dm)
{
	dataModel = dm;
	luaMouse = Creatable<Instance>::create<PluginMouse>();
	mouse = static_cast<PluginMouse*>(luaMouse.get());
	mouse->setWorkspace(dm->getWorkspace());
}

shared_ptr<Instance> Plugin::createToolbar(std::string name)
{
	return pluginManager->createToolbar(this, name);
}

void Plugin::saveSelectedToRoblox()
{
    pluginManager->getStudioHost()->uiAction("saveToRobloxAction");
}

shared_ptr<Instance> Plugin::getMouseLua()
{
	return luaMouse;
}

void Plugin::setSetting(std::string key, Reflection::Variant variant)
{
    pluginManager->getStudioHost()->setSetting(assetId, key, variant);
}

Reflection::Variant Plugin::getSetting(std::string key)
{
	Reflection::Variant variant;
	pluginManager->getStudioHost()->getSetting(assetId, key, &variant);
	return variant;
}

int Plugin::getStudioUserId()
{
	int userId;
	if (pluginManager->getStudioHost()->getLoggedInUserId(&userId))
	{
		return userId;
	}
	else
	{
		return 0;
	}
}

bool Plugin::isCollisionOn() const
{
	return AdvArrowToolBase::advCollisionCheckMode;
}

float Plugin::getGridSize() const
{
	return DragUtilities::getGrid().x;
}

AdvArrowToolBase::JointCreationMode Plugin::getJoinMode()
{
	return RBX::AdvArrowTool::getJointCreationMode();
}

void Plugin::activate(bool exclusiveMouse)
{
	pluginManager->activate(this, dataModel);
	setActive(true);
    
	if (exclusiveMouse)
	{
		tool = exclusiveMouse;
		Workspace *ws = dataModel->getWorkspace();
		ws->setNullMouseCommand();
	}
}

void Plugin::setActive(bool state) 
{ 
	active = state;
	if (!state)
	{
		tool = false;
	}
}

void Plugin::setAssetId(int assetId)
{
	this->assetId = assetId;
}

void Plugin::openScriptDoc(shared_ptr<RBX::Instance> script, int lineNumber)
{
	pluginManager->getStudioHost()->openScriptDoc(script, lineNumber);
}

// PluginManager
PluginManager::PluginManager()
	: currentDataModel(NULL)
{
}

static shared_ptr<PluginManager> doPluginManagerSingleton()
{
	static shared_ptr<PluginManager> sing = Creatable<Instance>::create<PluginManager>();
	return sing;
}

void initPluginManagerSingleton()
{
	doPluginManagerSingleton();
}

shared_ptr<PluginManager> PluginManager::singleton()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initPluginManagerSingleton, flag);
	return doPluginManagerSingleton();
}

void PluginManager::addModelPlugin(DataModel* dataModel, Instances instances, int assetId)
{
	boost::mutex::scoped_lock lock(mutex);

	StateDataEntry& entry = state[dataModel];

	shared_ptr<Plugin> pluginContainer = Creatable<Instance>::create<Plugin>();
	pluginContainer->setPluginManager(this);
	pluginContainer->setDataModel(dataModel);
	pluginContainer->setActive(false);
	pluginContainer->setName(format("Plugin_%d", assetId));
	pluginContainer->setAssetId(assetId);

	for (size_t i = 0; i < instances.size(); ++i)
	{
		instances[i]->setParent(pluginContainer.get());
	}

	entry.addPlugin(pluginContainer);
}

void PluginManager::startModelPluginScripts(DataModel* dataModel)
{
	boost::mutex::scoped_lock lock(mutex);

	// if there are no plugins it is possible for the state map to
	// be empty for this dataModel
	stateIter iter = state.find(dataModel);
	if (iter != state.end())
	{
		StateDataEntry& entry = iter->second;
		DataModel::LegacyLock legacylock(dataModel, DataModelJob::Write);

		if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
		{
			for (pluginsIter pluginIter = entry.plugins.begin();
				pluginIter != entry.plugins.end(); ++pluginIter)
			{
				(*pluginIter)->visitDescendants(
					boost::bind(&runAllPluginScriptsHelper, scriptContext, *pluginIter, _1));
			}
		}
	}
}

int PluginManager::createPlugin(lua_State* L)
{
	DataModel* dm = DataModel::get(RobloxExtraSpace::get(L)->context());
	RBXASSERT(dm);
	shared_ptr<Plugin> p = Creatable<Instance>::create<Plugin>();
	p->setPluginManager(this);
	p->setDataModel(dm);
	p->setActive(false);

	stateIter iter = state.find(dm);
	if (iter == state.end()) {
		state.insert(statePair(dm, StateDataEntry()));
		iter = state.find(dm);
	}

	iter->second.addPlugin(p);
	RBX::Lua::ObjectBridge::push(L, p);

	return 1;
}

void PluginManager::deletePlugins(DataModel *dataModel)
{
	RBX::Plugin *activePlugin = getActivePlugin(dataModel);
	if (activePlugin) 
	{
		activate(NULL, dataModel);
	}

	deleteStudioUI(dataModel);
	
	state.erase(dataModel);
}

void PluginManager::deleteStudioUI(DataModel *dataModel)
{
	stateIter iter = state.find(dataModel);
	if (iter != state.end())
	{
		iter->second.deleteStudioUI(studioHost);
	}
}

Plugin *PluginManager::getActivePlugin(DataModel *dataModel) 
{
	stateIter iter = state.find(dataModel);
	if (iter != state.end())
	{
		return iter->second.active;
	} 
	else
	{
		return NULL;
	}
}


void PluginManager::DeactivatePlugins()
{
	activate(NULL, currentDataModel);
	allPluginsDeactivatedSignal();
}

void PluginManager::activate(Plugin *plugin, DataModel *dataModel)
{
	if (plugin != NULL)
	{
		stateIter iter = state.find(plugin->getDataModel());
		RBXASSERT(iter != state.end());

		for (StateDataEntry::toolbarsIter i = iter->second.toolbars.begin();i != iter->second.toolbars.end();i ++)
		{
			i->second->reset();
		}

		pluginsList pluginlist = iter->second.plugins;
		for (pluginsIter i = pluginlist.begin(); i != pluginlist.end(); ++i)
		{
			(*i)->setActive(false);
			(*i)->deactivationSignal();		
		}

		iter->second.setActivePlugin(plugin);
	}
	else
	{
		Plugin *active = getActivePlugin(dataModel);
		if (active)
		{
			stateIter iter = state.find(dataModel);
			if (iter != state.end())
			{
				iter->second.active = NULL;
			} 

			active->setActive(false);
			active->deactivationSignal();
		}
	}
}

shared_ptr<Instance> PluginManager::StateDataEntry::getToolbar(std::string name, IStudioPluginHost *host, DataModel* dataModel)
{
	toolbarsIter iter = toolbars.find(name);
	if (iter == toolbars.end())
	{
		RBXASSERT(dataModel);
		shared_ptr<Toolbar> t = Creatable<Instance>::create<Toolbar>();
		t->SetId(host->createToolbar(name));
		t->setHost(host);
		t->setDataModel(dataModel);
		toolbars[name] = t;

		hideStudioUI(false, host);
	}

	iter = toolbars.find(name);
	RBXASSERT(iter != toolbars.end());

	return iter->second;
}

void PluginManager::StateDataEntry::fireButtonClick(void *id)
{
	for (toolbarsIter i = toolbars.begin();i != toolbars.end(); i++)
	{
		if (i->second.get() == NULL)
		{
			continue;
		}

		Button *btn = i->second->getButton(id);
		if (btn) 
		{
			btn->clickSignal();
			break;
		}
	}
}

void PluginManager::StateDataEntry::disableStudioUI(bool hide, IStudioPluginHost *host)
{
	FASTLOG(FLog::Plugins, "PluginManager::StateDataEntry::disableStudioUI");
	std::vector<void*> t;
	for (toolbarsIter i = toolbars.begin();i != toolbars.end(); i++)
	{
		t.push_back(i->second->getId());
		FASTLOG1(FLog::Plugins, "PluginManager::StateDataEntry::disableStudioUI - Toolbar %p", i->second->getId());
	}
	host->disableToolbars(t, hide);
	hidden = hide;
}

void PluginManager::StateDataEntry::hideStudioUI(bool hide, IStudioPluginHost *host)
{
	FASTLOG(FLog::Plugins, "PluginManager::StateDataEntry::hideStudioUI");
	std::vector<void*> t;
	for (toolbarsIter i = toolbars.begin();i != toolbars.end(); i++)
	{
		t.push_back(i->second->getId());
		FASTLOG1(FLog::Plugins, "PluginManager::StateDataEntry::hideStudioUI - Toolbar %p", i->second->getId());
	}
	host->hideToolbars(t, hide);
	hidden = hide;
}

void PluginManager::StateDataEntry::deleteStudioUI(IStudioPluginHost *host)
{
	std::vector<void*> t;
	for (toolbarsIter i = toolbars.begin();i != toolbars.end(); i++)
	{
		t.push_back(i->second->getId());
	}
	if (t.empty())
	{
		return;
	}
	host->deleteToolbars(t);
	for (toolbarsIter i = toolbars.begin();i != toolbars.end(); i++)
	{
		i->second->markDeleted();
	}
	toolbars.erase(toolbars.begin(), toolbars.end());
}

shared_ptr<Instance> PluginManager::createToolbar(Plugin *plugin, std::string name)
{
	stateIter iter = state.find(plugin->getDataModel());	
	RBXASSERT(iter != state.end());
	return iter->second.getToolbar(name, studioHost, plugin->getDataModel());
}

void PluginManager::buttonClick(DataModel *dataModel, void *id)
{
	stateIter iter = state.find(dataModel);	
	RBXASSERT(iter != state.end());

	// The click event is exposed in LUA, which in turn may try to change the
	// datamodel. Grab a write lock before propagating the click.
	if (iter != state.end()) 
	{
		RBX::DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		iter->second.fireButtonClick(id);
	}
}

void PluginManager::disableStudioUI(DataModel *dataModel, bool hide)
{
	stateIter iter = state.find(dataModel);
	if (iter != state.end())
	{
        iter->second.disableStudioUI(hide, studioHost);
	}
}

void PluginManager::hideStudioUI(DataModel *dataModel, bool hide)
{
	stateIter iter = state.find(dataModel);
	if (iter != state.end())
	{
		if (iter->second.hidden != hide)
		{
			iter->second.hideStudioUI(hide, studioHost);
		}
	}
}

void PluginManager::exportPlace(std::string filePath)
{
	getStudioHost()->exportPlace(filePath, RBX::ExporterSaveType_Everything);
}

void PluginManager::exportSelection(std::string filePath)
{
	getStudioHost()->exportPlace(filePath, RBX::ExporterSaveType_Selection);
}

void Plugin::openWikiPage(std::string url)
{
	pluginManager->getStudioHost()->openWikiPage(url);
}

shared_ptr<Instance> Plugin::csgUnion(shared_ptr<const Instances> instances)
{
	return pluginManager->getStudioHost()->csgUnion(instances, dataModel);
}

shared_ptr<const Instances> Plugin::csgNegate(shared_ptr<const Instances> instances)
{
	return pluginManager->getStudioHost()->csgNegate(instances, dataModel);
}

shared_ptr<const Instances> Plugin::csgSeparate(shared_ptr<const Instances> instances)
{
	return pluginManager->getStudioHost()->csgSeparate(instances, dataModel);
}

void Plugin::promptForExistingAssetId(std::string assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	pluginManager->getStudioHost()->promptForExistingAssetId(assetType, resumeFunction, errorFunction);
}

void PluginManager::fireDragEnterEvent(DataModel* dataModel, shared_ptr<const RBX::Instances> instances, Vector2 location)
{
	if (!dataModel)
		return;
	
	if (RBX::Plugin* activePlugin = getActivePlugin(dataModel))
	{
		RBX::DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		shared_ptr<InputObject> input = Creatable<Instance>::create<InputObject>(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE, Vector3(location.x, location.y, 0), Vector3::zero(), dataModel);
		ServiceProvider::find<UserInputService>(dataModel)->setCurrentMousePosition(input);
		activePlugin->getMouse()->fireDragEnterEvent(instances, input);
	}			
}
