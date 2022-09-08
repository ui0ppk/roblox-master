/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/GlobalSettings.h"
#include "FastLog.h"
#include "V8Xml/XmlSerializer.h"
#include "V8Xml/Serializer.h"
#include "Util/FileSystem.h"
#include "V8DataModel/Selection.h"

#include "util/standardout.h"

#include <boost/algorithm/string.hpp>

#include "StringConv.h"

FASTFLAGVARIABLE(DisableGlobalSettingsParentChange, true)

const char* const RBX::sGlobalAdvancedSettings		= "GlobalSettings";
const char* const RBX::sGlobalBasicSettings			= "UserSettings";
const char* const RBX::sSettings					= "GenericSettings";
const char* const RBX::sSettingsItem				= NULL;

using namespace RBX;

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<GlobalAdvancedSettings, std::string(std::string)> fun_getFastVariable(&GlobalAdvancedSettings::getFVariable, "GetFVariable", "name", Security::None);
static Reflection::BoundFuncDesc<GlobalAdvancedSettings, shared_ptr<const RBX::Reflection::ValueTable>()> fun_getFastVariables(&GlobalAdvancedSettings::getFVariables, "GetFVariables", Security::RobloxScript);
static Reflection::BoundFuncDesc<GlobalAdvancedSettings, bool(std::string)> func_getFFlag(&GlobalAdvancedSettings::getFFlag, "GetFFlag", "name", Security::None);
REFLECTION_END();

Settings::Settings(const std::string& settingsFile)
	:settingsFile(settingsFile)
	,settingsErased(false)
{
}

Settings::InvalidDescendentDetector::InvalidDescendentDetector() : anyInvalid(false) {}
Settings::InvalidDescendentCollector::InvalidDescendentCollector() {}

bool Settings::InvalidDescendentDetector::invalid(const Instance* instance) {
	return (dynamic_cast<const GlobalBasicSettings::Item*>(instance) == 0 &&
		dynamic_cast<const GlobalAdvancedSettings::Item*>(instance) == 0 &&
		dynamic_cast<const Selection*>(instance) == 0);
}

void Settings::InvalidDescendentDetector::operator()(shared_ptr<Instance> descendant) {
	anyInvalid |= invalid(descendant.get());
}

void Settings::InvalidDescendentCollector::operator()(shared_ptr<Instance> descendant) {
	if (InvalidDescendentDetector::invalid(descendant.get()))
		invalidInstances.push_back(descendant);
}

void Settings::eraseSettingsStore()
{
	settingsErased = true;
#ifdef _WIN32
	::DeleteFile(settingsFile.c_str());
#else
	std::remove(settingsFile.c_str());
#endif
}

void Settings::loadState(const std::string& optGlobalSettingsFile)
{
	std::string filePath = (optGlobalSettingsFile.empty()) ? settingsFile : optGlobalSettingsFile;
	if (filePath.size()==0)
		return;

	try
	{
		std::ifstream stream(utf8_decode(filePath).c_str(), std::ios_base::in | std::ios_base::binary);
		if (stream)
		{
			TextXmlParser machine(stream.rdbuf());
			std::auto_ptr<XmlElement> root(machine.parse());

			MergeBinder binder;
			readChildren(root.get(), binder, EngineCreator);
			binder.resolveRefs();
		}
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception thrown in loadState %s", e.what());
	}
}

void Settings::saveState()
{
	InvalidDescendentDetector invalidDetector;
	visitDescendants(invalidDetector);
	if(!settingsErased && !invalidDetector.anyInvalid){
		std::string filePath = settingsFile;
		if (filePath.size()==0)
			return;

		boost::scoped_ptr<XmlElement> root(Serializer::newRootElement());
		writeChildren(root.get(), EngineCreator);
		std::ofstream stream(filePath.c_str(), std::ios_base::out | std::ios_base::binary);
		TextXmlWriter machine(stream);
		machine.serialize(root.get());
	}
}

void Settings::removeInvalidChildren()
{
	InvalidDescendentCollector invalidCollector;
	visitDescendants(invalidCollector);

	for (unsigned int i = 0; i < invalidCollector.invalidInstances.size(); i++)
		invalidCollector.invalidInstances[i]->setLockedParent(NULL);
}

void Settings::verifyAddDescendant(const Instance* newParent, const Instance* instanceGettingNewParent) const {
	bool invalid = InvalidDescendentDetector::invalid(instanceGettingNewParent);
	if (!invalid) {
		InvalidDescendentDetector invalidDetector;
		instanceGettingNewParent->visitDescendants(invalidDetector);
		invalid = invalidDetector.anyInvalid;
	}
	if (invalid) {
		throw RBX::runtime_error("Not allowed to add that under settings");
	}
	Super::verifyAddDescendant(newParent, instanceGettingNewParent);
}

static std::string globalAdvancedSettingsFile()
{
	boost::filesystem::path file = RBX::FileSystem::getUserDirectory(true, RBX::DirAppData);
	if (!file.string().empty())
		file /= "GlobalSettings_13.xml";
	return file.string();
}

GlobalAdvancedSettings* g_sing = 0;

static shared_ptr<GlobalAdvancedSettings> doAdvancedSingleton()
{
	static shared_ptr<GlobalAdvancedSettings> sing = Creatable<Instance>::create<GlobalAdvancedSettings>();
	g_sing = sing.get();
	return sing;
}

void initAdvancedSingleton()
{
	doAdvancedSingleton();
}

shared_ptr<GlobalAdvancedSettings> GlobalAdvancedSettings::singleton()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initAdvancedSingleton, flag);
	return doAdvancedSingleton();
}

GlobalAdvancedSettings* GlobalAdvancedSettings::raw_singleton()
{
	RBXASSERT(g_sing);
	return g_sing;
}

GlobalAdvancedSettings::GlobalAdvancedSettings()
	:Super(globalAdvancedSettingsFile())
{
	setName("Global Settings");
	
	// During the unit tests (but only on Macs) the construction order (and consequently the
	// destruction order) needs to be done in the correct order.  Without the following line
	// the selection will be created after the static shared_ptr<GlobalAdvancedSettings> 
	// meaning the Selection gets destroyed first resulting in the destruction of 
	// GlobalAdvancedSettings raising SIGABRT as the Selection has already been destroyed
	// but the GlobalAdvancedSettings destruction accesses it.
	create<Selection>();
}

GlobalAdvancedSettings::~GlobalAdvancedSettings()
{
}

/*override*/ void GlobalAdvancedSettings::verifySetParent(const Instance* instance) const
{
	if (FFlag::DisableGlobalSettingsParentChange && (RBX::Security::Context::current().identity != RBX::Security::Anonymous))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set settings parent");
		}
		catch (RBX::base_exception& e)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set settings parent");
			throw e;
		}
	}
	Super::verifySetParent(instance);
}

static std::string globalBasicSettingsFile()
{
	boost::filesystem::path file = RBX::FileSystem::getUserDirectory(true, RBX::DirAppData);
	if (!file.string().empty())
		file /= "GlobalBasicSettings_13.xml";
	return file.string();
}

static shared_ptr<GlobalBasicSettings> doBasicSingleton()
{
	static shared_ptr<GlobalBasicSettings> sing = Creatable<Instance>::create<GlobalBasicSettings>();
	return sing;
}

void initBasicSingleton()
{
	doBasicSingleton();
}

shared_ptr<GlobalBasicSettings> GlobalBasicSettings::singleton()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initBasicSingleton, flag);
	return doBasicSingleton();
}

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<GlobalBasicSettings, void()> func_reset(&GlobalBasicSettings::reset, "Reset", Security::None);
static Reflection::BoundFuncDesc<GlobalBasicSettings, bool(std::string)> func_isUserFeatureEnabled(&GlobalBasicSettings::isUserFeatureEnabled, "IsUserFeatureEnabled", "name", Security::None);
REFLECTION_END();

GlobalBasicSettings::GlobalBasicSettings()
	:Super(globalBasicSettingsFile())
{
	setName("GlobalBasicSettings");
}

static void resetChild(shared_ptr<Instance> instance)
{
	if(GlobalBasicSettings::Item* item = Instance::fastDynamicCast<GlobalBasicSettings::Item>(instance.get())){
		item->reset();
	}
}
void GlobalBasicSettings::reset()
{
	visitChildren(&resetChild);
}

bool RBX::GlobalBasicSettings::isUserFeatureEnabled( std::string name )
{
	std::string user("User");

	if (name.compare(0, user.length(), user) != 0)
	{
		throw std::runtime_error(RBX::format("Flag %s does not exist", name.c_str()));
	}

	std::string result;
	if (FLog::GetValue(name.c_str(), result, true))
	{
		return boost::iequals(result, "true");
	}
	throw std::runtime_error(RBX::format("Flag %s does not exist", name.c_str()));
}

/*override*/ void GlobalBasicSettings::verifySetParent(const Instance* instance) const
{
	if (RBX::Security::Context::current().identity != RBX::Security::Anonymous) 
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set UserSettings parent");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set UserSettings parent");
			throw e;
		}
	}
	Super::verifySetParent(instance);
}


static void visit(const std::string& name, const std::string& value, void* context)
{
	RBX::Reflection::ValueTable* table = reinterpret_cast<RBX::Reflection::ValueTable*>(context);

	(*table)[name] = value;
}

shared_ptr<const RBX::Reflection::ValueTable> RBX::GlobalAdvancedSettings::getFVariables()
{
	shared_ptr<Reflection::ValueTable> values(rbx::make_shared<Reflection::ValueTable>());
    FLog::ForEachVariable(&visit, values.get(), FASTVARTYPE_ANY);
	return values;
}

bool RBX::GlobalAdvancedSettings::getFFlag( std::string name )
{
	std::string result;
	if (FLog::GetValue(name.c_str(), result, true))
	{
		return boost::iequals(result, "true");
	}
	throw std::runtime_error(RBX::format("Flag %s does not exist", name.c_str()));
}


std::string RBX::GlobalAdvancedSettings::getFVariable( std::string name )
{
	std::string result;
	if (FLog::GetValue(name.c_str(), result))
		return result;
	throw std::runtime_error(RBX::format("Flag %s does not exist", name.c_str()));
}
