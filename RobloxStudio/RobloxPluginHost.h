/**
 * RobloxPluginHost.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once 

// QT Headers
#include <QDir>
#include <QImage>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QList>
#include <QVariant>

// Boost Headers
#include <boost/function.hpp>

// Roblox headers
#include "v8datamodel/StudioPluginHost.h"
#include "reflection/reflection.h"

class QToolBar;
class QAction;

namespace RBX
{
	class DataModel;
}

namespace Qtitan
{
    class RibbonGroup;
    class RibbonPage;
}

class RobloxPluginHost: public QObject, public RBX::IStudioPluginHost
{
    Q_OBJECT

public:

    RobloxPluginHost(QObject *pParent);
    virtual ~RobloxPluginHost();

	void init();

    /**
     * RBX::IStudioPluginHost
     */
    void setNotifier(RBX::IHostNotifier *notifier);
    void* createToolbar(const std::string& name);
    void* createButton(void *tbId, const std::string& text, const std::string& tooltip, const std::string& iconFilePath);
    void setButtonActive(void *bId, bool active);
	void setButtonIcon_deprecated(void* buttonId, const std::string& iconFilePath);
	void setButtonIcon(void* butonId, const std::string& iconImage);
	void buttonIconFailedToLoad(void* buttonId);
    void hideToolbars(const std::vector<void*> &toolbars, bool hide);
    void disableToolbars(const std::vector<void*> &toolbars, bool disable);
    void deleteToolbars(const std::vector<void*> &toolbars);
    // Finds a setting in the plugin settings file, and changes it to the
    // specified value, or adds it if no setting is found.
    void setSetting(int assetId, const std::string& key, const RBX::Reflection::Variant& value);
    // Returns a setting value from the plugin settings file, or an empty value
    // if the setitng does not exist.
	void getSetting(int assetId, const std::string& key, RBX::Reflection::Variant* result);
	bool getLoggedInUserId(int* userIdOut);
    void uiAction(std::string uiActionName);
	// Returns action state if it is valid
	bool actionState(const QString &actionID, bool &enableState, bool &checkedState);

	void exportPlace(std::string filePath, RBX::ExporterSaveType saveType);

	void promptForExistingAssetId(const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

    shared_ptr<RBX::Instance> csgUnion(shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel);
    shared_ptr<const RBX::Instances> csgNegate(shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel);
    shared_ptr<const RBX::Instances> csgSeparate(shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel);

	// Other methods
    static void loadPlugins(RBX::DataModel *dataModel);

    static QString builtInPluginPath();
    static QString userPluginPath();
	static QString modelPluginsPath();

	static void processInstallPluginFromWebsite(int assetId, int assetVersion,
		std::string* result, std::exception* error,
		boost::function<void(bool)> resultCallback);
	static QString getPluginMetadataJson();
	static void setPluginEnabled(int assetId, bool enabled);
	static void uninstallPlugin(int assetId);

	void openScriptDoc(shared_ptr<RBX::Instance> instance, int lineNumber);
	void openWikiPage(const std::string& url);

	void handleDragEnterEvent(RBX::DataModel* dataModel, shared_ptr<const RBX::Instances> instances, RBX::Vector2 location);

private Q_SLOTS:

    void handlePluginAction();

    void onCreateToolbar(int toolbarID,QString name);
    void onCreateButton(int toolbarID,int actionID,QString text,QString tooltip,QString iconName);
	void onSetButtonActive(int buttonID, bool active);
    void onShowToolbar(int toolbarID,bool show);
    void onShowToolbars(QVariant toolbarID,bool show);
    void onEnableToolbar(int toolbarID,bool enable);
	void setButtonIconInternal_deprecated(int buttonId, QString iconName);
	void setButtonIconInternal(int buttonId, QImage icon);
    void onUIAction(QString uiActionName);

private:

    virtual bool event(QEvent * evt);

    static void loadPluginsFromPath(RBX::DataModel *dataModel, const QString &srcPath);
    void loadPluginsInternal(RBX::DataModel *dataModel, const QString &srcPath);
    static void doLoadPlugin(RBX::DataModel *dataModel, QString filename);

	static void findAndLoadModelPlugins(RBX::DataModel* dataModel, const QString& srcPath);
	static void loadInstalledModelPlugins(RBX::DataModel* dataModel);
	// Caller is responsible for ensuring that filename is a valid file path
	// to an openable file.
	static void doLoadModelPlugin(RBX::DataModel* dataModel, const QString& filename, int assetId);

	void removeActionsFromMap(QObject* parent);
	void handleTerrainRibbonGroupDeletion();

    QString getToolbarName(int id) const;

	static QDir getSpecificPluginFolder(int assetId);
	static QString getPluginMetadataFilePath();
	static bool readPluginMetadata(boost::shared_ptr<RBX::Reflection::ValueTable>& metadata);
	static void writePluginMetadata(const boost::shared_ptr<RBX::Reflection::ValueTable>& metadata);
	static void readPluginSettings(int assetId, boost::shared_ptr<RBX::Reflection::ValueTable>& settings);
	static void writePluginSettings(int assetId, const boost::shared_ptr<RBX::Reflection::ValueTable>& settings);
    RBX::IHostNotifier*     		m_pNotifier;

    QMap<int,QToolBar*>             m_ToolBarMap;
    QMap<int,QAction*>              m_ActionMap;
    QMap<int,Qtitan::RibbonGroup*>  m_RibbonGroupMap;

	Qtitan::RibbonPage*			    m_PluginsToolbar;
	Qtitan::RibbonPage*			    m_TerrainToolbar;       // Hack for terrain until these become native
    int                             m_TerrainToolbarID;
	int								m_TransformToolbarID;
	int                             m_ConvertToSmoothID;
    QMap<QString,int>               m_TextActionMap;
	
    static int              		m_NextID;
};
