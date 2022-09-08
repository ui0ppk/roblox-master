
#include <functional>

// Roblox headers
#include "v8datamodel/StudioPluginHost.h"


class RobloxPluginHost : public RBX::IStudioPluginHost
{
public:
	RobloxPluginHost(const std::string& location);
	RobloxPluginHost() {}
	virtual ~RobloxPluginHost() {}

	virtual void setNotifier(RBX::IHostNotifier *notifier) {}

	std::string getSpecificPluginFolderLocation();

	void readPluginSettings(boost::shared_ptr<RBX::Reflection::ValueTable>& settings);
	void writePluginSettings(const boost::shared_ptr<RBX::Reflection::ValueTable>& settings);

	virtual void *createToolbar(const std::string& name) { return NULL; }
	virtual void *createButton(void *tbId, const std::string& text, const std::string& tooltip, const std::string& iconFilePath) { return NULL; }
	virtual void setButtonActive(void *bId, bool active) {}
	virtual void setButtonIcon_deprecated(void *buttonId, const std::string& iconFilePath) {}
	virtual void setButtonIcon(void* butonId, const std::string& iconImage) {}
	virtual void buttonIconFailedToLoad(void* buttonId) {}
	virtual void hideToolbars(const std::vector<void*> &toolbars, bool hide) {}
	virtual void disableToolbars(const std::vector<void*> &toolbars, bool hide) {}
	virtual void deleteToolbars(const std::vector<void*> &toolbars) {}

	virtual void setSetting(int assetId, const std::string& key, const RBX::Reflection::Variant& value);
	virtual void getSetting(int assetId, const std::string& key, RBX::Reflection::Variant* result);
	virtual bool getLoggedInUserId(int* userIdOut) { return false; }
	virtual void uiAction(std::string uiActionName) {}

	virtual void openScriptDoc(shared_ptr<RBX::Instance> script, int lineNumber) {}
	virtual void exportPlace(std::string filePath, RBX::ExporterSaveType exportType) {}

	virtual void openWikiPage(const std::string& url) {}
	
	virtual shared_ptr<RBX::Instance> csgUnion( shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel) { return shared_ptr<RBX::Instance>(); }
	virtual shared_ptr<const RBX::Instances> csgNegate( shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel) { return shared_ptr<RBX::Instances>(); }
	virtual shared_ptr<const RBX::Instances> csgSeparate( shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel) { return shared_ptr<RBX::Instances>(); }

	virtual void promptForExistingAssetId(const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction) {} 

private:

	std::string m_modelPluginsDir;
};