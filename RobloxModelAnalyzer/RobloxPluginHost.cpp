#include "RobloxPluginHost.h"


#include "v8datamodel\PluginManager.h"
#include "script\ScriptContext.h"
#include "v8datamodel\Stats.h"
#include "v8xml\WebParser.h"


RobloxPluginHost::RobloxPluginHost(const std::string& location)
{
	RBX::PluginManager::singleton()->setStudioHost(this);

	m_modelPluginsDir = location;
}

std::string RobloxPluginHost::getSpecificPluginFolderLocation()
{
	return m_modelPluginsDir + "\\settings.json";
}

void RobloxPluginHost::readPluginSettings(boost::shared_ptr<RBX::Reflection::ValueTable>& settings)
{
	std::ifstream inFile;
	inFile.open(getSpecificPluginFolderLocation(), std::ios::in);

	std::stringstream stream;

	if (inFile.fail())
		return;

	stream << inFile.rdbuf();

	shared_ptr<const RBX::Reflection::ValueTable> readValues = rbx::make_shared<const RBX::Reflection::ValueTable>();

	if (  RBX::WebParser::parseJSONTable(stream.str(), readValues) )
	{
		*settings = *readValues;
	}
	else
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Unable to parse settings file for plugin");
	}
}


void RobloxPluginHost::writePluginSettings(const boost::shared_ptr<RBX::Reflection::ValueTable>& settings)
{
	std::stringstream stream;
	bool needComma = false;
	stream << "{";
	RBX::Stats::JsonWriter writer(stream, needComma);
	writer.writeTableEntries(*settings.get());
	stream << "}";


	std::ofstream outFile;
	outFile.open(getSpecificPluginFolderLocation(), std::ios::out);

	if (!outFile.fail())
	{
		outFile << stream.rdbuf();
		outFile.close();
	}
}


void RobloxPluginHost::setSetting(int assetId, const std::string& key, const RBX::Reflection::Variant& value)
{
	shared_ptr<RBX::Reflection::ValueTable> values(new RBX::Reflection::ValueTable());
	readPluginSettings(values);
	(*values)[key] = value;
	writePluginSettings(values);
}

void RobloxPluginHost::getSetting(int assetId, const std::string& key, RBX::Reflection::Variant* result)
{
	shared_ptr<RBX::Reflection::ValueTable> values(new RBX::Reflection::ValueTable());
	readPluginSettings(values);
	(*result) = (*values)[key];
}