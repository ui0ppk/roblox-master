#pragma once

#include "GfxBase/ViewBase.h"
#include "Reflection/reflection.h"
#include "v8tree/Instance.h"
#include <vector>

namespace RBX {
	class DataModel;
    //class Instance;
    //class Instances;

	class IHostNotifier
	{
	public:
		virtual void buttonClick(DataModel *dataModel, void *id) = 0;

		virtual void fireDragEnterEvent(DataModel* dataModel, shared_ptr<const RBX::Instances> instances, Vector2 location) = 0;
	};

	class IStudioPluginHost
	{
	public:
		virtual void setNotifier(IHostNotifier *notifier) = 0;

		virtual void *createToolbar(const std::string& name) = 0;
		virtual void *createButton(void *tbId, const std::string& text, const std::string& tooltip, const std::string& iconFilePath) = 0;
		virtual void setButtonActive(void *bId, bool active) = 0;
		virtual void setButtonIcon_deprecated(void *buttonId, const std::string& iconFilePath) = 0;
		virtual void setButtonIcon(void* butonId, const std::string& iconImage) = 0;
		virtual void buttonIconFailedToLoad(void* buttonId) = 0;
		virtual void hideToolbars(const std::vector<void*> &toolbars, bool hide) = 0;
		virtual void disableToolbars(const std::vector<void*> &toolbars, bool hide) = 0;
		virtual void deleteToolbars(const std::vector<void*> &toolbars) = 0;

        virtual void setSetting(int assetId, const std::string& key, const RBX::Reflection::Variant& value) = 0;
		virtual void getSetting(int assetId, const std::string& key, RBX::Reflection::Variant* result) = 0;
		virtual bool getLoggedInUserId(int* userIdOut) = 0;
        virtual void uiAction(std::string uiActionName) = 0;

		virtual void openScriptDoc(shared_ptr<Instance> script, int lineNumber) = 0;
		virtual void exportPlace(std::string filePath, RBX::ExporterSaveType exportType) = 0;

		virtual void openWikiPage(const std::string& url) = 0;

        virtual shared_ptr<Instance> csgUnion( shared_ptr<const Instances> instances, DataModel* dataModel ) = 0;
        virtual shared_ptr<const Instances> csgNegate( shared_ptr<const Instances> instances, DataModel* dataModel ) = 0;
        virtual shared_ptr<const Instances> csgSeparate( shared_ptr<const Instances> instances, DataModel* dataModel ) = 0;

		virtual void promptForExistingAssetId(const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction) = 0;
	};
}
