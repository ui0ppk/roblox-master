/**
 * RobloxPluginHost.cpp
 * Copyright (c) 2013 ROBLOX Corp. All rights reserved.
 */

#include "stdafx.h"
#include "RobloxPluginHost.h"

// Qt Headers
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QToolBar>
#include <QIODevice>

// Roblox Headers
#include "GfxBase/ViewBase.h"
#include "Script/ScriptContext.h"
#include "Util/ProtectedString.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/PluginManager.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/Workspace.h"
#include "v8datamodel/ContentProvider.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/WebParser.h"
#include "V8Xml/XmlSerializer.h"

// Roblox Studio Headers
#include "AuthoringSettings.h"
#include "RobloxCustomWidgets.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxMainWindow.h"
#include "RobloxSettings.h"
#include "RobloxUser.h"
#include "UpdateUIManager.h"
#include "RobloxQuickAccessConfig.h"
#include "RobloxContextualHelp.h"
#include "IRobloxDoc.h"
#include "QtUtilities.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "PluginAction.h"

#include "CSGOperations.h"
#include "v8datamodel/PartOperation.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8Xml/SerializerBinary.h"
#include "V8Tree/Service.h"

#include "rbx/make_shared.h"

Q_DECLARE_METATYPE(QList<int>);

FASTFLAG(StudioEnableGameAnimationsTab)

FASTFLAGVARIABLE(EnableCSGScripting, true)
FASTFLAGVARIABLE(StudioDisableToolbarUpdateOnButtonActive, false)

static const QString PluginToolbarName = "Plugins";
static const QString TerrainToolbarName = "Terrain";
static const QString TransformToolbarName = "Transform";
static const QString SmoothTerrainToolbarName = "Smooth Terrain";

int RobloxPluginHost::m_NextID = 1;
static const char* kFailedToLoadPluginIconIcon = "/textures/ui/CloseButton_dn.png";

FASTFLAG(GoogleAnalyticsTrackingEnabled)

static const char* kPluginFileName = "Plugin.rbxm";
static const char* kAssetVersionJsonKey = "AssetVersion";
static const char* kEnabledJsonKey = "Enabled";
static const char* kPluginSettingsFileName = "settings.json";

static const char* kIsChecked = "isChecked";
static const char* kIsVisible = "isVisible";

RobloxPluginHost::RobloxPluginHost(QObject* pParent)
    : QObject(pParent),
      m_PluginsToolbar(NULL),
      m_TerrainToolbar(NULL),
	  m_ConvertToSmoothID(0)
{
    RBX::PluginManager::singleton()->setStudioHost(this);

    installEventFilter(this);
}

RobloxPluginHost::~RobloxPluginHost()
{
}

void RobloxPluginHost::setNotifier(RBX::IHostNotifier *notifier)
{ 
    m_pNotifier = notifier; 
}

void* RobloxPluginHost::createToolbar(const std::string& name)
{
	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();

	if (mainWindow.isRibbonStyle())
	{
		if (name == TerrainToolbarName.toStdString())
			return (void*)m_TerrainToolbarID;
		else if (name == TransformToolbarName.toStdString())
			return (void*)m_TransformToolbarID;
	}

    int toolbarID = m_NextID++;

    QMetaObject::invokeMethod(
        this,
        "onCreateToolbar",
		Qt::QueuedConnection,
        Q_ARG(int,toolbarID),
        Q_ARG(QString,name.c_str()) );

    return (void*)toolbarID;
}

void* RobloxPluginHost::createButton(
    void*       tbId, 
    const std::string& text, 
    const std::string& tooltip, 
    const std::string& iconFilePath )
{
    int toolbarID = (int)tbId;

	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();

	if (mainWindow.isRibbonStyle() && m_TextActionMap.contains(text.c_str()))
	{
		if (toolbarID == m_TerrainToolbarID)
			return (void*)m_TextActionMap[text.c_str()];
		else if(toolbarID == m_TransformToolbarID)
			return (void*)m_TextActionMap[text.c_str()];
	}

    int actionID = m_NextID++;

    QMetaObject::invokeMethod(
        this,
        "onCreateButton",
        Qt::QueuedConnection,
        Q_ARG(int,toolbarID),
        Q_ARG(int,actionID),
        Q_ARG(QString,text.c_str()),
        Q_ARG(QString,tooltip.c_str()),
        Q_ARG(QString,QString::fromUtf8(iconFilePath.c_str())) );

    return (void*)actionID;
}

void RobloxPluginHost::setButtonActive(void *bId, bool active)
{
	QMetaObject::invokeMethod(this, "onSetButtonActive", Qt::QueuedConnection,
		Q_ARG(int, (int)(bId)), Q_ARG(bool, active));
}

void RobloxPluginHost::setButtonIcon_deprecated(void* buttonId, const std::string& iconFilePath)
{
	QMetaObject::invokeMethod(this, "setButtonIconInternal_deprecated", Qt::QueuedConnection,
		Q_ARG(int, (int)(buttonId)), Q_ARG(QString, QString(iconFilePath.c_str())));
}

void RobloxPluginHost::setButtonIcon(void* buttonId, const std::string& imageContent)
{
	QImage image;
	image.loadFromData((const uchar*)imageContent.c_str(), imageContent.size());
	if (image.isNull())
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			"Unable to load plugin icon. Image may have an invalid or unknown format.");
		buttonIconFailedToLoad(buttonId);
	}
	else
	{
		QMetaObject::invokeMethod(this, "setButtonIconInternal", Qt::QueuedConnection,
			Q_ARG(int, (int)(buttonId)), Q_ARG(QImage, image));
	}
}

void RobloxPluginHost::buttonIconFailedToLoad(void* buttonId)
{
	QImage image;
	image.load(QString(AppSettings::instance().contentFolder() + kFailedToLoadPluginIconIcon));
	QMetaObject::invokeMethod(this, "setButtonIconInternal", Qt::QueuedConnection,
		Q_ARG(int, (int)(buttonId)), Q_ARG(QImage, image));
}

void RobloxPluginHost::setButtonIconInternal_deprecated(int actionId, QString iconFilePath)
{
    if (!m_ActionMap.contains(actionId))
    {
        // This can happen if it takes a long time to load an icon
        return;
    }
	
	QPixmap iconPixmap;
	if (!iconPixmap.load(iconFilePath))
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			"Unable to load plugin icon. Image may have an invalid or unknown format.");
		iconPixmap.load(AppSettings::instance().contentFolder() + kFailedToLoadPluginIconIcon);
	}

	QAction* action = m_ActionMap[actionId];

	QSize scale(16, 16); // default size of icons for SystemMenu UI 

	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	if ( mainWindow.isRibbonStyle() )
	{
		// max size ribbongroup can support is 32 x 32 so scale accordingly
		if (iconPixmap.width() >= 32 || iconPixmap.height() >= 32)
		{
			scale = QSize(32, 32);
		}
		else
		{
			int min = qMin(iconPixmap.width(), iconPixmap.height());
			scale = QSize(min, min);
		}
	}

	QIcon icon(iconPixmap.scaled(scale));
	action->setIcon(icon);
}

void RobloxPluginHost::setButtonIconInternal(int actionId, QImage image)
{
	if (!m_ActionMap.contains(actionId))
    {
        // This can happen if it takes a long time to load an icon
        return;
    }
	
	QAction* action = m_ActionMap[actionId];

	QSize scale(16, 16); // default size of icons for SystemMenu UI 

	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	if ( mainWindow.isRibbonStyle() )
	{
		// max size ribbongroup can support is 32 x 32 so scale accordingly
		if (image.width() >= 32 || image.height() >= 32)
		{
			scale = QSize(32, 32);
		}
		else
		{
			int min = qMin(image.width(), image.height());
			scale = QSize(min, min);
		}
	}

	QIcon icon(QPixmap::fromImage(image).scaled(scale));
	action->setIcon(icon);
}

void RobloxPluginHost::hideToolbars(const std::vector<void*>& toolbars, bool hide)
{
    RobloxMainWindow& main_window = UpdateUIManager::Instance().getMainWindow();
    if ( !hide && main_window.getBuildMode() == BM_BASIC )
        return;

	if (toolbars.empty())
		return;

	QList<int> toolbarIds;
	for ( size_t i = 0 ; i < toolbars.size( ); ++i )
		toolbarIds.push_back((int)toolbars[i]);

	QVariant toolbarsVariant;
	toolbarsVariant.setValue(toolbarIds);

	QMetaObject::invokeMethod(
		this,
		"onShowToolbars",
		Qt::QueuedConnection,
		Q_ARG(QVariant,toolbarsVariant),
		Q_ARG(bool,!hide) );
}

void RobloxPluginHost::disableToolbars(const std::vector<void*>& toolbars, bool disable)
{
    RobloxMainWindow& main_window = UpdateUIManager::Instance().getMainWindow();
    if ( !disable && main_window.getBuildMode() == BM_BASIC )
        return;
    
    for ( size_t i = 0 ; i < toolbars.size( ); ++i )
    {
        int toolbarID = (int)toolbars[i];
        QMetaObject::invokeMethod(
                                  this,
                                  "onEnableToolbar",
                                  Qt::QueuedConnection,
                                  Q_ARG(int,toolbarID),
                                  Q_ARG(bool,!disable) );
    }
}

void RobloxPluginHost::onShowToolbar(int toolbarID, bool show)
{
    if ( !m_ToolBarMap.contains(toolbarID) && !m_RibbonGroupMap.contains(toolbarID) )
	{
		// only output an error if we're showing a toolbar that is supposed to exist
        if ( show )
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"Plugin attempting to show toolbar that does not exist - %i",toolbarID);
        return;
	}

    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	if ( mainWindow.isRibbonStyle() )
	{
		RibbonGroup* group = m_RibbonGroupMap[toolbarID];

		group->setVisible(show);
		// make sure we set the property so it can be used to correctly update the status in ribbon bar
		QList<QAction*> actions = group->actions();
		for (QList<QAction*>::iterator iter = actions.begin(); iter != actions.end(); iter++)
			(*iter)->setProperty(kIsVisible, show);
	}
	else
	{
        QToolBar* toolbar = m_ToolBarMap[toolbarID];
        toolbar->setVisible(show);
	}
}

void RobloxPluginHost::onShowToolbars(QVariant toolbarsVariant, bool show)
{
	QList<int> toolbars = toolbarsVariant.value<QList<int> >();

    QVector<bool> shows(toolbars.size(), show);

    if (show)
    {
        bool hasSmoothTerrainToolbar = false;

        for (int i = 0; i < toolbars.size(); ++i)
            if (getToolbarName(toolbars[i]) == SmoothTerrainToolbarName)
                hasSmoothTerrainToolbar = true;

        for (int i = 0; i < toolbars.size(); ++i)
            if (hasSmoothTerrainToolbar && getToolbarName(toolbars[i]) == TerrainToolbarName)
                shows[i] = false;
    }

    for (int i = 0; i < toolbars.size(); ++i)
    {
        onShowToolbar(toolbars[i], shows[i]);
    }
}

void RobloxPluginHost::onEnableToolbar(int toolbarID, bool enable)
{
    if ( !m_ToolBarMap.contains(toolbarID) && !m_RibbonGroupMap.contains(toolbarID) )
	{
		// only output an error if we're showing a toolbar that is supposed to exist
        if ( enable )
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"Plugin attempting to disable/enable toolbar that does not exist - %i",toolbarID);
        return;
	}
    
    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	if ( mainWindow.isRibbonStyle() )
	{
		RibbonGroup* group = m_RibbonGroupMap[toolbarID];
		group->setEnabled(enable);
		// make sure we set the property so it can be used to correctly update the status in ribbon bar
		QList<QAction*> actions = group->actions();
		for (QList<QAction*>::iterator iter = actions.begin(); iter != actions.end(); iter++)
			(*iter)->setEnabled(enable);
	}
	else
	{
        QToolBar* toolbar = m_ToolBarMap[toolbarID];
        toolbar->setEnabled(enable);
	}
}

void RobloxPluginHost::deleteToolbars(const std::vector<void*>& toolbars)
{
    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();

    for ( size_t i = 0 ; i < toolbars.size() ; ++i ) 
    {
        int toolbarID = (int)toolbars[i];
        if ( mainWindow.isRibbonStyle() && toolbarID == m_TerrainToolbarID )
		{
			handleTerrainRibbonGroupDeletion();
            continue;
		}

		QObject* parent = NULL;

		if ( mainWindow.isRibbonStyle() )
		{
			if ( !m_RibbonGroupMap.contains(toolbarID) )
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"Plugin attempting to delete toolbar that does not exist - %i",toolbarID);
                continue;
            }
    
            RibbonGroup* group = m_RibbonGroupMap[toolbarID];
            parent = group;
        }
        else
        {
            if ( !m_ToolBarMap.contains(toolbarID) )
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"Plugin attempting to delete toolbar that does not exist - %i",toolbarID);
                continue;
            }
    
            QToolBar* toolbar = m_ToolBarMap[toolbarID];
            parent = toolbar;
        }

		// delete all actions in the map that are parented to this toolbar
		removeActionsFromMap(parent);

        if ( mainWindow.isRibbonStyle() )
		{		
			if (toolbarID == m_TransformToolbarID) 
				continue;
			RibbonGroup* group = m_RibbonGroupMap[toolbarID];
            m_RibbonGroupMap.remove(toolbarID);

			qobject_cast<Qtitan::RibbonPage*>(group->parentWidget())->removeGroup(group);
		}
		else
		{
            QToolBar* toolbar = m_ToolBarMap[toolbarID];
            m_ToolBarMap.remove(toolbarID);
			QMetaObject::invokeMethod(toolbar, "deleteLater", Qt::QueuedConnection);
		}
    }
}

void RobloxPluginHost::setSetting(int assetId, const std::string& key,
	const RBX::Reflection::Variant& value)
{
    shared_ptr<RBX::Reflection::ValueTable> values(new RBX::Reflection::ValueTable());
	readPluginSettings(assetId, values);
	(*values)[key] = value;
	writePluginSettings(assetId, values);
}

void RobloxPluginHost::getSetting(int assetId, const std::string& key,
	RBX::Reflection::Variant* result)
{
	shared_ptr<RBX::Reflection::ValueTable> values(new RBX::Reflection::ValueTable());
	readPluginSettings(assetId, values);
	(*result) = (*values)[key];
}

bool RobloxPluginHost::getLoggedInUserId(int* userIdOut)
{
	int userId = RobloxUser::singleton().getUserId();
	if (userId > 0)
	{
		*userIdOut = userId;
		return true;
	}
	else
	{
		return false;
	}
}

void RobloxPluginHost::handlePluginAction()
{
    QAction* pAction = qobject_cast<QAction*>(sender());
    if ( !pAction )
        return;    

	if (RobloxIDEDoc::getIsCloudEditSession() && pAction->data().toInt() == m_ConvertToSmoothID)
	{
		QtUtilities::RBXMessageBox warning;
		warning.setText("Cannot conver to smooth during Cloud Edit session");
		warning.setStandardButtons(QMessageBox::Ok);
		warning.exec();
		return;
	}

    // revert the Qt's checked status (it will be taken care by the script code by calling "setButtonActive"
    pAction->setChecked(!pAction->isChecked());

    // action to be handled by the relevant document
    if ( RobloxDocManager::Instance().getCurrentDoc() )
    {
        int actionID = pAction->data().toInt();
        RobloxDocManager::Instance().getCurrentDoc()->handlePluginAction(m_pNotifier,(void*)(actionID));
    }

    // update toolbar status
    UpdateUIManager::Instance().updateToolBars();
}

void RobloxPluginHost::onCreateToolbar(int toolbarID,QString name)
{
    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();

	if ( mainWindow.isRibbonStyle() )
	{
		RBXASSERT(!m_RibbonGroupMap.contains(toolbarID));

		RibbonPage* page = (name == SmoothTerrainToolbarName) ? m_TerrainToolbar : m_PluginsToolbar;

		RibbonGroup* group = page->addGroup(name);

		group->setObjectName(name);
		group->setTitle(name);

		group->setVisible(false);
		m_RibbonGroupMap[toolbarID] = group;
	}
	else
	{
        RBXASSERT(!m_ToolBarMap.contains(toolbarID));
    
        RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
        QToolBar* pToolbar = mainWindow.addToolBar(name);
        RBXASSERT(pToolbar);

		pToolbar->setObjectName(name);

        pToolbar->setIconSize(QSize(16,16));
        pToolbar->setVisible(false);    
        m_ToolBarMap[toolbarID] = pToolbar;
	}
}

void RobloxPluginHost::uiAction(std::string uiActionName)
{
    QMetaObject::invokeMethod(
        this,
        "onUIAction",
        Qt::QueuedConnection,
        Q_ARG(QString,uiActionName.c_str()) );
}

void RobloxPluginHost::onUIAction(QString uiActionName)
{
    QObjectList objects = UpdateUIManager::Instance().getMainWindow().children();
    for (QObjectList::iterator iter = objects.begin();
         iter != objects.end();
         ++iter)
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if (action &&
            action->objectName() == uiActionName)
        {
            action->trigger();
            return;
        }
    }
}

void RobloxPluginHost::onCreateButton(int toolbarID,int actionID,QString text,QString tooltip,QString iconName)
{
     if ( !m_RibbonGroupMap.contains(toolbarID) && !m_ToolBarMap.contains(toolbarID) )
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"Plugin attempting to add button to toolbar that does not exist - %i, %i",toolbarID,actionID);
        return;
    }

    QAction* action = NULL;

    if ( text.isEmpty() )
        text = tooltip;

    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	if ( mainWindow.isRibbonStyle() )
	{
		Qtitan::RibbonGroup* pGroup = m_RibbonGroupMap[toolbarID];
        action = new PluginAction(text,pGroup);
		if (mainWindow.ribbonBar())
			action->setFont(mainWindow.ribbonBar()->font());
		// by default an action is visible (unless it is explicitly set to invisible)
		action->setProperty(kIsVisible, true);
		pGroup->addAction(action,Qt::ToolButtonTextUnderIcon);
	}
	else
	{
	    QToolBar* pToolbar = m_ToolBarMap[toolbarID];
        action = new PluginAction(text,pToolbar);
	    pToolbar->addAction(action);
	}

    action->setObjectName(text);
    connect(action,SIGNAL(triggered()),this,SLOT(handlePluginAction()));

    if ( text.isEmpty() )
        action->setPriority(QAction::LowPriority);
    else
        action->setText(text);

    action->setToolTip(tooltip);
    action->setCheckable(true);
    action->setData(actionID);

    m_ActionMap[actionID] = action;

	if ( !iconName.isEmpty() )
	{ 
#ifdef Q_WS_MAC
		iconName.replace('\\',QDir::separator());
#endif
		// set icon for action (safe to call this method as we should be in MainThread)
		setButtonIconInternal_deprecated(actionID, iconName);
	}

	// make sure we enable plugin actions for shortcut customization
	mainWindow.addAction(action);
	// for ribbon bar mode, check if we need to add created action in quick access bar
	if (mainWindow.isRibbonStyle())
	{
		RobloxQuickAccessConfig& quickAccessConfig = RobloxQuickAccessConfig::singleton();
		if (QAction* mappedAction = quickAccessConfig.mapProxyAction(action))
			quickAccessConfig.addToQuickAccessBar(mappedAction, quickAccessConfig.visibleInQuickAccessBar(action));

		// QTitanRibbon doesn't recalculate group width, make sure we force the width recalculation
		Qtitan::RibbonGroup* pGroup = m_RibbonGroupMap[toolbarID];
		if (pGroup)
		{
			pGroup->recalcWidths();
			QEvent evt(QEvent::LayoutRequest);
			QApplication::sendEvent(pGroup->parentWidget(), &evt);
		}
	}
}

void RobloxPluginHost::onSetButtonActive(int buttonID, bool active)
{
	if ( !m_ActionMap.contains(buttonID) )
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,
			"Plugin attempting to set active state for missing button ID - %i", buttonID);
		return;
	}

	QAction *pAction = m_ActionMap[buttonID];
	pAction->setChecked(active);
	//make sure we set the property so it can be used to correctly status in ribbon bar
	pAction->setProperty(kIsChecked, active);

	if (!FFlag::StudioDisableToolbarUpdateOnButtonActive)
		QTimer::singleShot(1, &UpdateUIManager::Instance(), SLOT(updateToolBars()));
}

bool RobloxPluginHost::event(QEvent* evt)
{
    if ( evt->type() != PLUGIN_LOAD )
        return QObject::event(evt);

    RobloxCustomEventWithArg *pCustomEvent = dynamic_cast<RobloxCustomEventWithArg*>(evt);
    if ( pCustomEvent )
    {
        boost::function<void()>* pFunctionObj = pCustomEvent->m_pEventArg;
        if ( pFunctionObj )
            (*pFunctionObj)();        
    }

    return true;
}

void RobloxPluginHost::loadPlugins(RBX::DataModel* dataModel)
{
    if ( !dataModel )
        return;

	// Old style plugins
    loadPluginsFromPath(dataModel,RobloxPluginHost::builtInPluginPath());
    loadPluginsFromPath(dataModel,RobloxPluginHost::userPluginPath());

	findAndLoadModelPlugins(dataModel, builtInPluginPath());
	loadInstalledModelPlugins(dataModel);
	findAndLoadModelPlugins(dataModel, userPluginPath());

	RBX::PluginManager::singleton()->startModelPluginScripts(dataModel);
}

void RobloxPluginHost::loadPluginsFromPath(RBX::DataModel* dataModel,const QString &srcPath)
{
    if ( !dataModel || srcPath.isEmpty() )
        return;
    UpdateUIManager::Instance().getMainWindow().pluginHost().loadPluginsInternal(dataModel,srcPath);
}

void RobloxPluginHost::loadPluginsInternal(RBX::DataModel* dataModel,const QString& srcPath)
{
    QDir srcDir(srcPath);    
    QFileInfoList fileList = srcDir.entryInfoList(QStringList("*.lua"),QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    QFileInfo fileInfo;
    for ( int ii = 0 ; ii < fileList.size() ; ++ii )    
    {        
        fileInfo = fileList.at(ii);

        if ( fileInfo.isSymLink() )
            continue;

        if ( fileInfo.isDir() )        
            loadPluginsInternal(dataModel, fileInfo.absoluteFilePath());
        else
            QApplication::postEvent(this, new RobloxCustomEventWithArg(PLUGIN_LOAD,boost::bind(&RobloxPluginHost::doLoadPlugin,dataModel,fileInfo.absoluteFilePath())));
    }
}

void RobloxPluginHost::doLoadPlugin(RBX::DataModel* dataModel, QString filename)
{
    boost::mutex::scoped_lock lock(RBX::PluginManager::singleton()->mutex);

    QFile file(filename);
    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
        return;

    QTextStream inStream(&file);
    QString toExecute = inStream.readAll();
    if ( toExecute.isEmpty() )
        return;

    QByteArray baToExe = toExecute.toAscii();
    const char* c_strToExe = baToExe.data();

    QByteArray baFileName = filename.toAscii();
    const char* c_strFileName = baFileName.data();

    RBX::PluginManager::singleton()->setLastPath(QFileInfo(filename).absoluteDir().absolutePath().toStdString());

    try
    {        
        RBX::DataModel::LegacyLock legacylock(dataModel, RBX::DataModelJob::Write);
        boost::shared_ptr<RBX::ScriptContext> pContext = RBX::shared_from(dataModel->create<RBX::ScriptContext>());
        if (pContext)    
            pContext->executeInNewThread(RBX::Security::StudioPlugin,
                RBX::ProtectedString::fromTrustedSource(c_strToExe), c_strFileName);
    }
    catch (std::exception &e)
    {
        RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,e.what());
    }
}

void RobloxPluginHost::findAndLoadModelPlugins(RBX::DataModel* dataModel, const QString& srcPath)
{
	QStringList extensions;
	extensions += "*.rbxm";
	extensions += "*.rbxmx";

	QDir srcDir(srcPath);    
	QFileInfoList fileList = srcDir.entryInfoList(extensions, QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

	QFileInfo fileInfo;
	for (int ii = 0 ; ii < fileList.size() ; ++ii)
	{        
		fileInfo = fileList.at(ii);
		if (!fileInfo.isDir())
		{
			doLoadModelPlugin(dataModel, fileInfo.absoluteFilePath(), -1);
		}
	}
}

void RobloxPluginHost::loadInstalledModelPlugins(RBX::DataModel* dataModel)
{
	shared_ptr<const RBX::Reflection::ValueTable> values(new RBX::Reflection::ValueTable);
	QString pluginMetadataString = getPluginMetadataJson();
    std::stringstream jsonStream(pluginMetadataString.toUtf8().constData());
	if (!pluginMetadataString.isEmpty() && !RBX::WebParser::parseJSONTable(pluginMetadataString.toUtf8().constData(), values))
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			qPrintable(tr("Unable to load metadata for installed plugins. "
			"Installed plugins will not be loaded.")));
		return;
	}

	QDir basePluginFolder(modelPluginsPath());
	for (RBX::Reflection::ValueTable::const_iterator itr = values->begin();
		itr != values->end(); ++itr)
	{
		// the key in the map should be asset id
		QString assetId = QString::fromStdString(itr->first);
		bool intConversionWorked = true;
		int intAssetId = assetId.toInt(&intConversionWorked);

		if (!intConversionWorked)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,
				qPrintable(tr("Error parsing plugin asset id for plugin: %s")),
				qPrintable(assetId));
			continue;
		}

		// skip the plugin if it marked as not enabled
		shared_ptr<const RBX::Reflection::ValueTable> valueTable =
			itr->second.cast<shared_ptr<const RBX::Reflection::ValueTable> >();
		RBX::Reflection::ValueTable::const_iterator enabledData = valueTable->find(kEnabledJsonKey);
		if (enabledData != valueTable->end() &&
			enabledData->second.cast<bool>() == false)
		{
			continue;
		}

		QDir subFolder = basePluginFolder;
		QString assetFile = QString(kPluginFileName);
		if (!subFolder.cd(assetId) || !subFolder.exists(assetFile))
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,
				qPrintable(tr("Unable to find plugin with the following id: %s")),
				qPrintable(assetId));
			continue;
		}

		// everything is good! load the plugin
		doLoadModelPlugin(dataModel, subFolder.absoluteFilePath(assetFile),
			intAssetId);
	}
}

void RobloxPluginHost::doLoadModelPlugin(RBX::DataModel* dataModel,
	const QString& filename, int assetId)
{	
	std::ifstream stream(qPrintable(filename), std::ios_base::in | std::ios_base::binary);

	if (!stream.is_open())
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,
			qPrintable(tr("Error while trying to open plugin file: %s")),
			qPrintable(filename));
		return;
	}

	try
	{
		// Read the items
		RBX::Instances instances;
		Serializer().loadInstances(stream, instances);
		if (RBX::ContentProvider* cp = dataModel->create<RBX::ContentProvider>())
		{
			RBX::LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, instances);
		}

		RBX::PluginManager::singleton()->addModelPlugin(dataModel, instances, assetId);
	}
	catch (const std::runtime_error&) // xml parse can throw
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			qPrintable(tr("Unable to open plugin file (bad file format): ")));
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,
			"%s", qPrintable(filename));
	}
}

QString RobloxPluginHost::builtInPluginPath()
{
    return RobloxSettings::getBuiltInPluginsFolder();
}

QString RobloxPluginHost::userPluginPath()
{
	return AuthoringSettings::singleton().pluginsDir.absolutePath();
}

QString RobloxPluginHost::modelPluginsPath()
{
	return AuthoringSettings::singleton().modelPluginsDir.absolutePath();
}

void RobloxPluginHost::processInstallPluginFromWebsite(int assetId, int assetVersion,
	std::string* serialized, std::exception* error, boost::function<void(bool)> resultCallback)
{
	QString assetIdString = QString("%1").arg(assetId);
	// Track an event for each attempt to install a plugin
	if (FFlag::GoogleAnalyticsTrackingEnabled)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "PluginInstall", assetIdString.toStdString().c_str());
	}

	if (error || !serialized || serialized->empty() || userPluginPath().isEmpty())
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			qPrintable(tr("Unable to download plugin at this time. Try again later.")));
		if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "PluginInstallFailed", assetIdString.toStdString().c_str());
		}
		resultCallback(false);
		return;
	}

	QDir pluginFolder = getSpecificPluginFolder(assetId);
	QFile pluginFile(pluginFolder.absoluteFilePath(kPluginFileName));
	if (!pluginFile.open(QIODevice::Truncate | QIODevice::WriteOnly) ||
		!pluginFile.write(QByteArray(serialized->c_str(), serialized->size())))
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			qPrintable(tr("Unable to create plugin file.")));
		if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "PluginInstallFailed", assetIdString.toStdString().c_str());
		}
		resultCallback(false);
		return;
	}
	pluginFile.flush();

	// Update Metadata
	{
		shared_ptr<RBX::Reflection::ValueTable> metadata(new RBX::Reflection::ValueTable);
		if (readPluginMetadata(metadata))
		{
			std::string assetIdStdString(qPrintable(assetIdString));
		
			RBX::Reflection::ValueTable::iterator pluginData = metadata->find(assetIdStdString);
			shared_ptr<RBX::Reflection::ValueTable> newData;
			if (pluginData == metadata->end())
			{
				newData.reset(new RBX::Reflection::ValueTable);
				(*newData)[kEnabledJsonKey] = true;
			}
			else
			{
				if (FFlag::GoogleAnalyticsTrackingEnabled)
				{
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "PluginReInstall",
						assetIdString.toStdString().c_str());
				}

				newData.reset(new RBX::Reflection::ValueTable(
					*(pluginData->second.cast<shared_ptr<const RBX::Reflection::ValueTable> >())));
			}
			(*newData)[kAssetVersionJsonKey] = assetVersion;
			(*metadata)[assetIdStdString] = shared_ptr<const RBX::Reflection::ValueTable>(newData);

			writePluginMetadata(metadata);
		}
	}

	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO,
		qPrintable(tr("Finished installing plugin. Open a new window to start using it.")));
	resultCallback(true);
}

QString RobloxPluginHost::getPluginMetadataJson()
{
	QFile pluginMetadataFile(getPluginMetadataFilePath());

	if (!pluginMetadataFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return QString();
	}

	return QTextStream(&pluginMetadataFile).readAll();
}

void RobloxPluginHost::setPluginEnabled(int assetId, bool enabled)
{
	std::string assetIdString(qPrintable(QString("%1").arg(assetId)));

	if (FFlag::GoogleAnalyticsTrackingEnabled)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION,
			enabled ? "EnablePlugin" : "DisablePlugin", QString::fromUtf8(assetIdString.c_str()).toStdString().c_str());
	}

	shared_ptr<RBX::Reflection::ValueTable> metadata(new RBX::Reflection::ValueTable);
	if (readPluginMetadata(metadata))
	{
		RBX::Reflection::ValueTable::iterator itr = metadata->find(assetIdString);
		if (itr == metadata->end())
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,
				qPrintable(tr("Unable to set enabled/disabled state for plugin: %d")),
				assetId);
			return;
		}

		shared_ptr<RBX::Reflection::ValueTable> newData(
			new RBX::Reflection::ValueTable(
				*(itr->second.cast<shared_ptr<const RBX::Reflection::ValueTable> >())));
		(*newData)[kEnabledJsonKey] = enabled;
		(*metadata)[assetIdString] = shared_ptr<const RBX::Reflection::ValueTable>(newData);

		writePluginMetadata(metadata);
	}
}

void RobloxPluginHost::uninstallPlugin(int assetId)
{
	if (modelPluginsPath().isEmpty())
	{
		// If we don't know where the model plugins should be, we can't delete.
		return;
	}

	QString assetIdString = QString("%1").arg(assetId);
	std::string assetIdStdString(qPrintable(assetIdString));

	if (FFlag::GoogleAnalyticsTrackingEnabled)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "UninstallPlugin", assetIdString.toStdString().c_str());
	}

	{
		shared_ptr<RBX::Reflection::ValueTable> metadata(new RBX::Reflection::ValueTable);
		if (readPluginMetadata(metadata))
		{
			metadata->erase(assetIdStdString);
			writePluginMetadata(metadata);
		}
	}

	QDir removingPluginPath(getSpecificPluginFolder(assetId));
	
	bool cleanPluginFileSucceeded = 
		!removingPluginPath.exists(kPluginFileName) ||
		removingPluginPath.remove(kPluginFileName);
	bool cleanPluginSettingsSucceeded = 
		!removingPluginPath.exists(kPluginSettingsFileName) ||
		removingPluginPath.remove(kPluginSettingsFileName);
	bool removeDirectorySucceeded = false;
	removeDirectorySucceeded =
		!removingPluginPath.exists() ||
		(removingPluginPath.cdUp() && removingPluginPath.rmdir(assetIdString));

	if (!cleanPluginFileSucceeded || !cleanPluginSettingsSucceeded || !removeDirectorySucceeded)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,
			qPrintable(tr("Unable to remove all of the plugin files in directory %s. "
			"Please clean this location manually")),
			qPrintable(removingPluginPath.absolutePath()));
	}
}

QDir RobloxPluginHost::getSpecificPluginFolder(int assetId)
{
	QString assetIdString = QString("%1").arg(assetId);
	QDir result(modelPluginsPath());
	result.mkpath(QString("%1").arg(assetIdString));
	result.cd(assetIdString);
	return result;
}

QString RobloxPluginHost::getPluginMetadataFilePath()
{
	static const char* kPluginMetadataFilename = "PluginMetadata.json";
	return QDir(modelPluginsPath()).absoluteFilePath(kPluginMetadataFilename);
}

bool RobloxPluginHost::readPluginMetadata(shared_ptr<RBX::Reflection::ValueTable>& values)
{
	if (userPluginPath().isEmpty())
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING,
			qPrintable(tr("Warning: unable to add new plugin to metadata file.")));
		return false;
	}

	QString pluginMetadataString = getPluginMetadataJson();
    std::stringstream jsonStream(pluginMetadataString.toUtf8().constData());
	if (!pluginMetadataString.isEmpty())
	{
		shared_ptr<const RBX::Reflection::ValueTable> readValues;
		if (!RBX::WebParser::parseJSONTable(pluginMetadataString.toUtf8().constData(), readValues) )
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING,
				qPrintable(tr("Plugin metadata file is corrupted. "
				"Version information for your plugins may have been lost.")));
			
			return false;
		}

		*values = *readValues;
	}

	return true;
}

void RobloxPluginHost::writePluginMetadata(const shared_ptr<RBX::Reflection::ValueTable>& values)
{
	std::stringstream outputBuffer;
	bool needComma = false;
	outputBuffer << "{";
	RBX::Stats::JsonWriter writer(outputBuffer, needComma);
	writer.writeTableEntries(*values);
	outputBuffer << "}";

	QFile pluginMetadataFile(getPluginMetadataFilePath());

	if (!pluginMetadataFile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text)
		|| !pluginMetadataFile.write(outputBuffer.str().c_str()))
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
			qPrintable(tr("Unable to write plugin metadata. "
			"Version information for your plugins may be lost.")));
	}
}

void RobloxPluginHost::readPluginSettings(int assetId,
	boost::shared_ptr<RBX::Reflection::ValueTable>& settings)
{
	QDir pluginFolder = getSpecificPluginFolder(assetId);
	QString settingsFileContents;
	{
		QFile settingsFile(pluginFolder.absoluteFilePath(kPluginSettingsFileName));
		if (settingsFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			settingsFileContents = settingsFile.readAll();
		}
	}

	// initialize readValues to an empty table, in case the settings cannot be read from file.
	shared_ptr<const RBX::Reflection::ValueTable> readValues =
		rbx::make_shared<const RBX::Reflection::ValueTable>();

	if (!settingsFileContents.isEmpty())
	{
		std::stringstream jsonStream(settingsFileContents.toStdString());
		if (  RBX::WebParser::parseJSONTable(settingsFileContents.toStdString(), readValues) )
		{
			*settings = *readValues;
		}
		else
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,
				qPrintable(tr("Unable to parse settings file for plugin")));
			if (FFlag::GoogleAnalyticsTrackingEnabled)
			{
				RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR,
					"ParsePluginSettingsFailed", QString("%1").arg(assetId).toStdString().c_str());
			}
		}
	}
}

void RobloxPluginHost::writePluginSettings(int assetId,
	const boost::shared_ptr<RBX::Reflection::ValueTable>& settings)
{
	std::stringstream stream;
    bool needComma = false;
	stream << "{";
    RBX::Stats::JsonWriter writer(stream, needComma);
    writer.writeTableEntries(*settings.get());
	stream << "}";

	QDir pluginFolder = getSpecificPluginFolder(assetId);
	QFile settingsFile(pluginFolder.absoluteFilePath(kPluginSettingsFileName));
    
	if (settingsFile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text))
    {
        settingsFile.write(stream.str().c_str());
    }
}
void RobloxPluginHost::init()
{
	// All must be done on the main thread
	// Create our Add-ons and Terrain special pages for Ribbon
	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	if ( mainWindow.isRibbonStyle() )
	{
		RobloxQuickAccessConfig::singleton().setPluginHostInitialized(true);

		if ( !m_PluginsToolbar )
			m_PluginsToolbar = mainWindow.ribbonBar()->findChild<RibbonPage*>(PluginToolbarName);

		if ( !m_TerrainToolbar )
		{
            const int pagesCount = mainWindow.ribbonBar()->getPageCount();
			for ( int i = 0 ; i < pagesCount ; ++i )
			{
                RibbonPage* page = mainWindow.ribbonBar()->getPage(i);
                if ( page->objectName() != TerrainToolbarName )
                    continue;

                m_TerrainToolbar = page;

				const QList<RibbonGroup*> ribbonGroups = mainWindow.ribbonBar()->getPage(i)->findChildren<RibbonGroup*>();
                for (int j = 0; j < ribbonGroups.count(); j++)
			    {
                    RibbonGroup* group = ribbonGroups[j];
                    if ( group->objectName() != TerrainToolbarName )
                        continue;

                    m_TerrainToolbarID = m_NextID++;
                    m_RibbonGroupMap[m_TerrainToolbarID] = group;

                    const QList<QAction*>& actions = group->actions();
                    for ( int k = 0 ; k < actions.size() ; ++k )
                    {                        
                        QAction* action = actions[k];
                        const int actionID = m_NextID++;

                        m_ActionMap[actionID] = action;
                        m_TextActionMap[action->text()] = actionID;

                        action->setCheckable(true);
                        action->setData(actionID);

						if (action->text() == "Convert To Smooth")
						{
							m_ConvertToSmoothID = actionID;
						}

                        connect(action,SIGNAL(triggered()),this,SLOT(handlePluginAction()));
                    }
                }
            }

			if (RibbonPage* page = mainWindow.ribbonBar()->findChild<RibbonPage*>("Model"))
			{
				if (RibbonGroup* group = page->findChild<RibbonGroup*>("Tools"))
				{
					m_TransformToolbarID = m_NextID++;
					m_RibbonGroupMap[m_TransformToolbarID] = group;
					
					const QList<QAction*>& actions = group->actions();
					for (int k = 0; k < actions.size(); ++k)
					{
						QAction* action = actions[k];
						if (action->objectName() != TransformToolbarName)
							continue;

						const int actionID = m_NextID++;

						m_ActionMap[actionID] = action;
						m_TextActionMap[action->text()] = actionID;

						action->setCheckable(true);
						action->setData(actionID);

						connect(action, SIGNAL(triggered()),this,SLOT(handlePluginAction()));
						break;
					}
				}
			}
		}
	}
}

void RobloxPluginHost::removeActionsFromMap(QObject* parent)
{
	QMap<int,QAction*>::iterator iter = m_ActionMap.begin();
	while ( iter != m_ActionMap.end() )
	{
		if ( iter.value()->parent() == parent )
			iter = m_ActionMap.erase(iter);
		else
			++iter;
	}
}

void RobloxPluginHost::handleTerrainRibbonGroupDeletion()
{
	RibbonGroup* group = m_RibbonGroupMap[m_TerrainToolbarID];
	if (!group)
		return;

	const QList<QAction*>& actions = group->actions();
	for ( int k = 0 ; k < actions.size() ; ++k )
	{  
		if (!m_TextActionMap.contains(actions.at(k)->text()))
			group->removeAction(actions.at(k));
	}

	removeActionsFromMap(group);

	group->setVisible(true);
}

QString RobloxPluginHost::getToolbarName(int id) const
{
    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();

	if (mainWindow.isRibbonStyle())
	{
		RibbonGroup* group = m_RibbonGroupMap[id];

        if (group)
            return group->objectName();
    }
    else
    {
        QToolBar* toolbar = m_ToolBarMap[id];

        if (toolbar)
            return toolbar->objectName();
	}

    return QString();
}

bool RobloxPluginHost::actionState(const QString &actionID, bool &enableState, bool &checkedState)
{
	QAction* pAction = NULL;
	QMapIterator<int,QAction*> iter(m_ActionMap);

	while (iter.hasNext()) 
	{
		iter.next();

		pAction = iter.value();
		if (pAction && ((pAction->objectName() == actionID) || (pAction->objectName() + "_proxyAct" == actionID)) && pAction->property(kIsVisible).toBool())
		{
			enableState = true;
			checkedState = pAction->property(kIsChecked).toBool();
			return true;
		}		
	}

	return false;
}

void RobloxPluginHost::openScriptDoc(shared_ptr<RBX::Instance> instance, int lineNumber)
{
	if (RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance) ||
		RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(instance))
		QMetaObject::invokeMethod(&RobloxDocManager::Instance(), "openDoc", Qt::QueuedConnection, Q_ARG(shared_ptr<RBX::Instance>, instance), Q_ARG(int, lineNumber));
}

void RobloxPluginHost::openWikiPage(const std::string& url)
{
	UpdateUIManager::Instance().setDockVisibility(eDW_CONTEXTUAL_HELP, true);

	QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxContextualHelp>(eDW_CONTEXTUAL_HELP), "onHelpTopicChanged", Q_ARG(QString, QString::fromStdString(url)));
}

void RobloxPluginHost::exportPlace(std::string filePath, RBX::ExporterSaveType exportType)
{
	RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
	if (!playDoc)
		return;

	playDoc->exportSelection(filePath, exportType);
}

void RobloxPluginHost::promptForExistingAssetId(const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (FFlag::StudioEnableGameAnimationsTab)
	{
		RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
		if (playDoc)
			playDoc->promptForExistingAssetId(assetType, resumeFunction, errorFunction);
	}
	else
	{
		errorFunction("Requested functionality is not available.");
	}
}

void operationFailed(const char* title, const char* msg)
{
    throw RBX::runtime_error("%s %s", title, msg);
}

shared_ptr<RBX::Instance> RobloxPluginHost::csgUnion(shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel)
{
    if (!FFlag::EnableCSGScripting)
        throw std::runtime_error("Scripted CSG operations are disabled");

	if (instances->size() == 0)
		throw std::runtime_error("Union function requires instances; no instances passed.");

    RBX::CSGOperations csgOps( dataModel, operationFailed );
    shared_ptr<RBX::PartOperation> partOperation;
    csgOps.doUnion( instances->begin(), instances->end(), partOperation );
    return partOperation;
}

shared_ptr<const RBX::Instances> RobloxPluginHost::csgNegate(shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel)
{
    if (!FFlag::EnableCSGScripting)
        throw std::runtime_error("Scripted CSG operations are disabled");

	if (instances->size() == 0)
		throw std::runtime_error("Negate function requires instances; no instances passed.");

    RBX::CSGOperations csgOps( dataModel, operationFailed );

    std::vector<shared_ptr<RBX::Instance> > toSelect;
    csgOps.doNegate( instances->begin(), instances->end(), toSelect );
    shared_ptr<RBX::Instances> result = rbx::make_shared<RBX::Instances>(toSelect.size());
    int i = 0;
    for ( std::vector<shared_ptr<RBX::Instance> >::const_iterator iter = toSelect.begin(); iter != toSelect.end(); iter++, i++ )
        (*result)[i] = (*iter);
    return result;
}

shared_ptr<const RBX::Instances> RobloxPluginHost::csgSeparate(shared_ptr<const RBX::Instances> instances, RBX::DataModel* dataModel)
{
    if (!FFlag::EnableCSGScripting)
        throw std::runtime_error("Scripted CSG operations are disabled");

	if (instances->size() == 0)
		throw std::runtime_error("Separate function requires instances; no instances passed.");

    RBX::CSGOperations csgOps( dataModel, operationFailed );

    std::vector<shared_ptr<RBX::Instance> > ungroupedItems;
    csgOps.doSeparate( instances->begin(), instances->end(), ungroupedItems );
    shared_ptr<RBX::Instances> result = rbx::make_shared<RBX::Instances>(ungroupedItems.size());
    int i = 0;
    for ( std::vector<shared_ptr<RBX::Instance> >::const_iterator iter = ungroupedItems.begin(); iter != ungroupedItems.end(); iter++, i++ )
        (*result)[i] = (*iter);
    return result;
}

void RobloxPluginHost::handleDragEnterEvent(RBX::DataModel* dataModel, shared_ptr<const RBX::Instances> instances, RBX::Vector2 location)
{
	if (!m_pNotifier)
		return;
	m_pNotifier->fireDragEnterEvent(dataModel, instances, location);
}
