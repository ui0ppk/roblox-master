/**
 * RibbonMainWindow.cpp
 * Copyright (c) 2013 ROBLOX Corp. All rights reserved.
 */

#include "stdafx.h"
#include "RobloxRibbonMainWindow.h"

// Qt includes
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDomDocument>
#include <QDomElement>
#include <QDir>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QtCore>
#include <QWidgetAction>
#include <QFontDatabase>

// Roblox Studio includes
#include "RobloxSettings.h"
#include "RobloxMainWindow.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxCustomWidgets.h"
#include "GalleryItemColor.h"
#include "NameValueStoreManager.h"
#include "CommonInsertWidget.h"
#include "UpdateUIManager.h"
#include "AuthoringSettings.h"
#include "RobloxToolBox.h"
#include "QtUtilities.h"
#include "RobloxUser.h"
#include "PluginAction.h"
#include "RobloxCustomWidgets.h"
#include "RobloxQuickAccessConfig.h"
#include "StudioDeviceEmulator.h"

FASTFLAGVARIABLE(SurfaceLightEnabled, false)

FASTFLAG(StudioSeparateActionByActivationMethod)

//static const char* newLayoutXmlConfigFile = "RobloxStudioRibbonRelease.xml";
static const char* updatedLayoutXmlConfigFile = "RobloxStudioRibbon.xml";

static const char* elemMainWindow      = "MainWindow";

static const char* elemQuickAccessBar  = "QuickAccessBar";
static const char* elemeSystemButton   = "SystemButton";
static const char* elemSystemPopup     = "SystemPopup";
static const char* elemRibbonBar       = "RibbonBar";

static const char* elemTabPage         = "tabpage";
static const char* elemGroup           = "group";

static const char* elemAddAction       = "addaction";
static const char* elemAddRadioButton  = "addradiobutton";
static const char* elemAddCheckBox     = "addcheckbox";
static const char* elemAddComboBox     = "addcombobox";
static const char* elemAddSeparator    = "addseparator";
static const char* elemActionGroup     = "actiongroup";
static const char* elemSubMenu         = "submenu";
static const char* elemMenuButton      = "menubutton";
static const char* elemMenuPopupButton = "menupopupbutton";
static const char* elemColorPicker     = "colorpicker";
static const char* elemButtonGroup     = "buttongroup";
static const char* elemSplitButton     = "splitbutton";
static const char* elemRibbonGallery   = "ribbongallery";
static const char* elemGalleryItem     = "galleryitem";
static const char* elemComboBoxItem    = "comboboxitem";
static const char* elemGroupOptionsMenu= "groupoptionsmenu";

static const char* attribName          = "name";
static const char* attribText          = "text";
static const char* attribStyle         = "style";
static const char* attribPopupStyle    = "popupStyle";
static const char* attribToolTip       = "tooltip";

static const char* attribIcon          = "icon";
static const char* attribIconWidth     = "width";
static const char* attribIconHeight    = "height";
static const char* attribColumns       = "columns";

static const char* attribVisible       = "visible";
static const char* attribCheckable     = "ischeckable";
static const char* attribProxyAction   = "isProxy";
static const char* attribEnabled       = "enabled";
static const char* attribRequiresAuth  = "requiresauthentication";
static const char* attribChecked       = "checked";
static const char* attribDefaultValue  = "defaultvalue";
static const char* attribIconVisibleInMenu = "iconvisibleinmenu";

static const char* attribType          = "type";
static const char* attribValue         = "value";
static const char* attribInstanceName  = "instancename";
static const char* attribMenuTrigger   = "handleMenuTrigger";
static const char* attribCommonSlot    = "iscommonslot";
static const char* attribPopup         = "popup";
static const char* attribSelectChangesText = "selectChangesText";

static const char* attribFFlag         = "fflag";
static const char* attribWinOnly       = "iswinonly";
#ifdef _WIN32
static const char* attribMacOnly       = "ismaconly";
#endif

static const char* attribMinButton     = "minimizeButton";
static const char* attribThemeOptions  = "themeoptions";
static const char* attribToolTipIcon   = "tooltipicon";
static const char* attribFont          = "font";

static const char* attribContextTitle  = "contexttitle";
static const char* attribContextColor  = "contextcolor";

static const char* attribItemWidth     = "itemwidth";
static const char* attribItemHeight    = "itemheight";
static const char* attribSetting       = "setting";

static const char* actionCheckBox      = "checkbox";
static const char* actionRadio         = "radio";
static const char* actionPlugin        = "plugin";

static const char* settingRibbonMinimized   = "rbxRibbonMinimized";
static const char* settingRecentFiles       = "rbxRecentFiles";
static const char* settingLastDirectory     = "rbxl_last_directory";

RobloxRibbonMainWindow::RobloxRibbonMainWindow(RobloxMainWindow* pMainWindow)
: Qtitan::RibbonMainWindow(NULL)
, m_LastDirectory(".")
, m_pRibbonStyle(NULL)
, m_pRibbonMinimizeAction(NULL)
, m_pQuickAccessConfigDialog(NULL)
, m_bInitialized(false)
{
	// DO NOT ADD MORE HERE
	// Try to keep most initialization logic in setupRibbonBar
}

void RobloxRibbonMainWindow::setupRibbonBar()
{
	if (m_bInitialized)
		throw RBX::runtime_error("Ribbon bar already initialized.");

	//create ribbonbar
	QString xmlFilePath(RobloxSettings::getResourcesFolder());
	xmlFilePath.append("/");
    xmlFilePath.append(updatedLayoutXmlConfigFile);

	QFile file(xmlFilePath);
	if (!file.open(QIODevice::ReadOnly))
		throw RBX::runtime_error("Failed to open configuration XML file.");

	QDomDocument doc("appRibbonBar");
	if (!doc.setContent(&file)) 
		throw RBX::runtime_error("Failed to parse configuration XML file.");
	
	QDomElement docElem = doc.documentElement();
	if(docElem.tagName() != elemMainWindow)
		throw RBX::runtime_error("Failed to parse configuration XML file.  File must start with MainWindow element!");
	
	// initialize default data and do required cleanup
	initialize();	
	
	// now create system button
	parseAndCreateSystemButton(docElem);

	// finally ribbon bar
	parseAndCreateRibbonBar(docElem);

	// quickaccess bar must be created in the last, as there can be some actions created from XML file
	parseAndCreateQuickAccessBar(docElem);
	
    ribbonBar()->setFrameThemeEnabled(false);
	ribbonBar()->setTitleBarVisible(false);
	ribbonBar()->getSystemButton()->setToolButtonStyle(Qt::ToolButtonTextOnly);

	m_bInitialized = true;
}

RibbonPage* RobloxRibbonMainWindow::createRibbonPage(const QString& objectName, const QString& pageTitle, int indexPosition)
{
	RibbonPage* pRibbonPage = ribbonBar()->findChild<RibbonPage*>(objectName);
	if (pRibbonPage)
		return pRibbonPage;

	// Pages should be tacked onto the end of the bar, unless an explicit index has been passed in.
	int insertedPosition = indexPosition >= 0 ? indexPosition : ribbonBar()->getPageCount();

	pRibbonPage = ribbonBar()->insertPage(insertedPosition, pageTitle);
	RBXASSERT(pRibbonPage);
	pRibbonPage->setObjectName(objectName);

	// update the index property of this page and all pages after
	for (int ii = insertedPosition; ii < ribbonBar()->getPageCount(); ii++)
		ribbonBar()->getPage(ii)->setProperty("index", ii);
	
	return pRibbonPage;
}

RibbonGroup* RobloxRibbonMainWindow::createRibbonGroup(RibbonPage* pParent, const QString& objectName, const QString& groupTitle)
{
	if (!pParent)
		return NULL;

	RibbonGroup* pRibbonGroup = pParent->findChild<RibbonGroup*>(objectName);
	if (pRibbonGroup)
		return pRibbonGroup;

	pRibbonGroup = pParent->addGroup(groupTitle.isEmpty() ? objectName : groupTitle);
	RBXASSERT(pRibbonGroup);
	pRibbonGroup->setObjectName(objectName);

	return pRibbonGroup;
}

void RobloxRibbonMainWindow::parseAndCreateQuickAccessBar(const QDomElement &docElem)
{
	QDomElement quickAccessBarElement = docElem.firstChildElement(elemQuickAccessBar);
	if (quickAccessBarElement.isNull())
		return;

	Qtitan::RibbonQuickAccessBar* pQuickAccessBar = ribbonBar()->getQuickAccessBar();
	if (!pQuickAccessBar)
		return;

	QString toolTip = getToolTip(quickAccessBarElement);
	if (!toolTip.isEmpty())
	{
		QAction* pAction = pQuickAccessBar->actionCustomizeButton();
		if (pAction)
			pAction->setToolTip(tr("Customize Quick Access Bar"));
	}

	QString visible = quickAccessBarElement.attribute(attribVisible);
	ribbonBar()->showQuickAccess(visible.isEmpty() || (visible != "false"));

	// load QuickAccess configuration from settings
	bool configLoaded = RobloxQuickAccessConfig::singleton().loadQuickAccessConfig();
	
	RobloxSettings settings;
	// parse data from XML 
	RobloxQuickAccessConfig::QuickAccessActionCollection defaultQuickAccessCollection;
	QDomElement addActionElement = quickAccessBarElement.firstChildElement(elemAddAction);
	while (!addActionElement.isNull()) 
	{
		QString actionName = getName(addActionElement);
		if (!actionName.isEmpty())
		{
			QAction *pChildAction = findChildAction(actionName);
			if (pChildAction)
			{
				// check if action is visible in quick access bar
				bool actVisible = false;
				if (!addActionElement.attribute(attribVisible).isEmpty())
					actVisible = addActionElement.attribute(attribVisible) == "true" ;

				// if not loaded from settings, then add actions from XML config
				if (!configLoaded)
				{
					QString key("QuickAccess_" + pChildAction->text());
					if (settings.contains(key))
						actVisible = settings.value(key).toBool(); // legacy support
					RobloxQuickAccessConfig::singleton().addToQuickAccessBar(pChildAction, actVisible);
				}

				// make a list of actions defined in XML config for restore
				defaultQuickAccessCollection.append(RobloxQuickAccessConfig::ActionData(pChildAction, actVisible, true));
			}
		}

		addActionElement = addActionElement.nextSiblingElement(elemAddAction);
	}

	// save XML config for restore
	RobloxQuickAccessConfig::singleton().storeDefaults(defaultQuickAccessCollection);
	connect(customizeQuickAccessAction, SIGNAL(triggered()), this, SLOT(customizeQuickAccess()));
}

void RobloxRibbonMainWindow::updateInternalWidgetsState(QAction* pAction, bool enabledState,bool checkedState)
{
	// update internal widgets is required only if we have ribbon bar
	if (QAbstractButton* pCorrelaryButton = qobject_cast<QAbstractButton*>(m_mCorrelaryActionMap.value(pAction)))
	{
		if  (QThread::currentThread() == thread())
		{
			pCorrelaryButton->setEnabled(enabledState);
			pCorrelaryButton->setChecked(checkedState);
		}
		else
		{
			QMetaObject::invokeMethod(pCorrelaryButton,"setEnabled",Qt::QueuedConnection,Q_ARG(bool,enabledState));
			QMetaObject::invokeMethod(pCorrelaryButton,"setChecked",Qt::QueuedConnection,Q_ARG(bool,checkedState));
		}	
	}
}

QString RobloxRibbonMainWindow::getDefaultSavePath()
{
    QString doc_path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QDir(doc_path).mkdir("ROBLOX");
    return doc_path + "/ROBLOX";
}

QStringList RobloxRibbonMainWindow::getRecentFiles() const
{	return RobloxSettings().value(settingRecentFiles).toStringList(); }

/**
 * Sets the current path and stores it in the settings.
 *  If the path is null, the default path will be used.
 *  If the path is a filename, the filename part will be stripped off.
 * 
 * @param path  new path for current directory
 */
void RobloxRibbonMainWindow::setCurrentDirectory(const QString& path)
{
    QString newPath = path;
    QFileInfo info(newPath);

    // if it's a file, get the path
    if (info.isFile())
        newPath = info.absolutePath();
	else
		newPath = info.absoluteFilePath();

    // if empty or doesn't exist, get the default path
    if ( newPath.isEmpty() || !QDir(newPath).exists() )
        newPath = getDefaultSavePath();

	// don't change to the auto-save path
	if ( newPath == AuthoringSettings::singleton().autoSaveDir.absolutePath() )
		return;

    m_LastDirectory = newPath;
    QDir::setCurrent(m_LastDirectory);

    // save into settings
    RobloxSettings().setValue(settingLastDirectory, m_LastDirectory);
}

void RobloxRibbonMainWindow::updateRecentFile(const QString& fileName)
{
	//update the recent files
	if(!fileName.isEmpty()
       && (fileName.endsWith(".rbxl", Qt::CaseInsensitive) || fileName.endsWith(".rbxlx", Qt::CaseInsensitive))
       && !(fileName.endsWith("visit.rbxl", Qt::CaseSensitive))
       && !(fileName.endsWith("server.rbxl", Qt::CaseSensitive)))
	{
		RobloxSettings settings;
		QStringList files = getRecentFiles();
		files.removeAll(fileName);

		QFileInfo fileInfo(fileName);
		bool bFileExists = fileInfo.exists();
		if (bFileExists && !files.contains(fileName))
			files.prepend(fileName);

		// if it exceeds max limit then remove last
		while (files.size() > MAX_RECENT_FILES)
			files.removeLast();

		// update settings
		settings.setValue(settingRecentFiles, files);

		updateRecentFilesUI();

		if (bFileExists)
			setCurrentDirectory(fileName);
	}
}

void RobloxRibbonMainWindow::actionEvent(QActionEvent* evt)
{
	if (!m_bInitialized || m_mProxyActions.isEmpty() || evt->type() != QEvent::ActionAdded)
		return;

	QMap<QString, QAction*>::iterator iter = m_mProxyActions.find(getFullName(evt->action()));
	if (iter == m_mProxyActions.end())
		return;

	iter.value()->setCheckable(evt->action()->isCheckable());
	connect(iter.value(), SIGNAL(triggered()), evt->action(), SLOT(trigger()));
	connect(evt->action(), SIGNAL(changed()), this, SLOT(updateProxyAction()));
}

void RobloxRibbonMainWindow::parseAndCreateSystemButton(const QDomElement &docElem)
{
	QDomElement systemButtonElement = docElem.firstChildElement(elemeSystemButton);
	if (systemButtonElement.isNull())
		return;

	QString buttonName = getName(systemButtonElement);
	QString icon       = systemButtonElement.attribute(attribIcon);
	QString helpIcon   = systemButtonElement.attribute(attribToolTipIcon);

	QIcon iconLogo;
	if (!helpIcon.isEmpty())
	{
		// if sizes of the pixmaps added in QIcon are different then the first added pix will be used in tooltip associated with system button
		// in case of similar sizes second pixmap is used, so make sure we have size for tooltip icon different from 32x32
		QPixmap helpPixmap(helpIcon);
		if (!helpPixmap.isNull())
			iconLogo.addPixmap(helpPixmap);
	}

	if (!icon.isEmpty())
	{
		QPixmap iconPixmap(icon);
		if (!iconPixmap.isNull())
			iconLogo.addPixmap(iconPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}

	if(QAction* actionFile = ribbonBar()->addSystemButton(iconLogo, buttonName)) 
	{
		// set current theme (default theme)
		setTheme(m_pRibbonStyle->getTheme());

		QString tooltip  = getToolTip(systemButtonElement);
		actionFile->setToolTip(tooltip);

		QDomElement popupElement = systemButtonElement.firstChildElement(elemSystemPopup);
		if (!popupElement.isNull())
		{
			Qtitan::RibbonSystemPopupBar* popupBar = qobject_cast<Qtitan::RibbonSystemPopupBar*>(actionFile->menu());
			connect(popupBar, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));

			parseAndCreateChildren(popupElement, popupBar);
			
			RibbonPageSystemRecentFileList* pageRecentFile = popupBar->addPageRecentFile(tr("Recent Files"));
			pageRecentFile->setSize(MAX_RECENT_FILES);
			pageRecentFile->setObjectName("recentFileList");
			connect(pageRecentFile, SIGNAL(openRecentFile(const QString&)), this, SLOT(openRecentFile(const QString&)));
						
			QAction* pActionExit =  popupBar->addPopupBarAction(tr("Exit"));
			connect(pActionExit, SIGNAL(triggered()), this, SLOT(close()));

			//QAction* pActionOption = popupBar->addPopupBarAction(tr("Settings"));
			//connect(pActionOption, SIGNAL(triggered()), this, SLOT(openSettingsDialog()));						
		}
	}
}

void RobloxRibbonMainWindow::parseAndCreateRibbonBar(const QDomElement &docElem)
{
	QDomElement ribbonBarElement = docElem.firstChildElement(elemRibbonBar);
	if (!ribbonBarElement.isNull())
	{								
		// Add online help button
		ribbonBar()->addAction(onlineHelpAction, Qt::ToolButtonIconOnly);
		onlineHelpAction->setIcon(QIcon(":/RibbonBar/images/Studio Ribbon Icons/about.png"));
		
		// Add theme style options
		if (ribbonBarElement.attribute(attribThemeOptions) == "true")
			createThemeOptions();
		
		// Add a minimize button
		if (ribbonBarElement.attribute(attribMinButton) == "true")
		{
			m_pRibbonMinimizeAction = ribbonBar()->addAction(QIcon(":/RibbonBar/images/Studio Ribbon Icons/ribbonMinimize.png"), "Toggle ribbon visiblity", Qt::ToolButtonIconOnly);
			m_pRibbonMinimizeAction->setStatusTip(tr("Show only the tab names on the Ribbon"));
			m_pRibbonMinimizeAction->setShortcut(tr("Ctrl+F1"));
			if (FFlag::StudioSeparateActionByActivationMethod)
				m_pRibbonMinimizeAction->setObjectName("minimizeRibbonAction");
			QtUtilities::setActionShortcuts(*m_pRibbonMinimizeAction, m_pRibbonMinimizeAction->shortcuts());

			if (FFlag::StudioSeparateActionByActivationMethod)
            	connect(m_pRibbonMinimizeAction, SIGNAL(triggered()), this, SLOT(commonSlot()));
			else
				connect(m_pRibbonMinimizeAction, SIGNAL(triggered()), this, SLOT(toggleRibbonMinimize()));
			connect(ribbonBar(), SIGNAL(minimizationChanged(bool)), this, SLOT(handleRibbonMinimizationChanged(bool)));
            
			// add this to registry
            RobloxSettings settings;
			ribbonBar()->setMinimized(settings.value(settingRibbonMinimized, false).toBool());
		}	

		// Create our tab pages and their children, recursively
		parseAndCreateChildren(ribbonBarElement, ribbonBar());

        if (ribbonBarElement.hasAttribute(attribFont))
        {
			QString fontStr = ribbonBarElement.attribute(attribFont);
			if (fontStr.isEmpty())
			{
				updateFonts(m_ribbonFont);
			}
			else
			{
				QFont font;
				font.fromString(fontStr);
#ifdef Q_WS_MAC
				// On Mac we need to have bigger fonts
				font.setPointSize(font.pointSize() + 4);
#endif
				// Now update fonts
				updateFonts(font);
			}
        }
	}
}

void RobloxRibbonMainWindow::parseAndCreateChildren(const QDomElement& tabPageElement, QObject* pParent)
{
	for (QDomElement domElementIterator = tabPageElement.firstChildElement(); !domElementIterator.isNull(); domElementIterator = domElementIterator.nextSiblingElement()) 
	{		
#ifdef Q_WS_MAC
        if (domElementIterator.attribute(attribWinOnly) == "true")
            continue;
#else
        if (domElementIterator.attribute(attribMacOnly) == "true")
            continue;
#endif
		//check if we need to enable any element via fflag?
		QString flagName = domElementIterator.attribute(attribFFlag);
		if (!flagName.isEmpty())
		{
			try
			{
				bool negate = flagName.startsWith("!");
				if (negate)
					flagName = flagName.mid(1);

				bool keepItem = RBX::GlobalAdvancedSettings::singleton()->getFFlag(flagName.toStdString());
				if (negate)
					keepItem = !keepItem;

				if (!keepItem)
					continue;
			}
			catch (const std::runtime_error&)
			{
				RBXASSERT(false);
			}
		}
		
		QObject* pCreatedElement   = NULL;
		QString  domElementTagName = domElementIterator.tagName().toLower();

		if (domElementTagName == elemTabPage)
		{
			pCreatedElement = parseAndCreateRibbonPage(domElementIterator);
		}
		else if (domElementTagName == elemGroup)
		{
			pCreatedElement = parseAndCreateRibbonGroup(domElementIterator, pParent);	
		}
		else if (domElementTagName == elemColorPicker)
		{
			pCreatedElement = parseAndCreateColorPicker(domElementIterator, pParent);
		}
		else if (domElementTagName == elemMenuPopupButton)
		{
			pCreatedElement = parseAndCreateMenuPopupButton(domElementIterator, pParent);
		}
		else if (domElementTagName == elemMenuButton)
		{
			pCreatedElement = parseAndCreateMenu(domElementIterator, pParent);			
		}
		else if (domElementTagName == elemSplitButton)
		{
			pCreatedElement = parseAndCreateSplitButton(domElementIterator, pParent);
		}
		else if (domElementTagName == elemRibbonGallery)
		{
			pCreatedElement = parseAndCreateRibbonGallery(domElementIterator, pParent);			
		}		
		else if (domElementTagName == elemAddAction)
		{
			QString actionType = domElementIterator.attribute(attribType);
			if (actionType == actionCheckBox)
			{			
				pCreatedElement = parseAndCreateCheckBox(domElementIterator, pParent);				
			}			
			else if (actionType == actionRadio)
			{
				pCreatedElement = parseAndCreateRadioButton(domElementIterator, pParent);				
			}
            else if ((actionType == actionPlugin) || actionType.isNull())
			{
                pCreatedElement = parseAndCreateRibbonAction(domElementIterator, pParent);
			}
			else
			{
				throw RBX::runtime_error("Invalid action type: %s", actionType.toStdString().c_str());
			}
		}
		else if (domElementTagName == elemAddSeparator)
		{
			pCreatedElement = parseAndCreateSeparator(domElementIterator, pParent);			
		}		
		else if (domElementTagName == elemActionGroup)
		{
			pCreatedElement = parseAndCreateActionGroup(domElementIterator, pParent);
		}
		else if (domElementTagName == elemButtonGroup)
		{
			pCreatedElement = parseAnCreateButtonGroup(domElementIterator, dynamic_cast<QWidget*>(pParent));
		}
		else if (domElementTagName == elemAddRadioButton)
		{
			pCreatedElement = parseAndCreateRadioButton(domElementIterator, pParent);
		}
		else if (domElementTagName == elemGalleryItem)
		{
			parseAndCreateGalleryItem(domElementIterator, pParent);
		}
		else if (domElementTagName == elemAddCheckBox)
		{
			pCreatedElement = parseAndCreateCheckBox(domElementIterator, pParent, false);
		}
		else if (domElementTagName == elemAddComboBox)
		{
			pCreatedElement = parseAndCreateComboBox(domElementIterator, pParent);
		}
		else if (domElementTagName == elemGroupOptionsMenu)
		{
			pCreatedElement = parseAndCreateGroupOptionsMenu(domElementIterator, pParent);
		}
		else if (domElementTagName == elemSubMenu)
		{
			pCreatedElement = parseAndCreateSubMenu(domElementIterator, pParent);
		}

		// add dynamic properties if any
		if (pCreatedElement)
		{	
			if (!domElementIterator.attribute(attribValue).isEmpty())
				pCreatedElement->setProperty("user_value", domElementIterator.attribute(attribValue));
            
			if (!domElementIterator.attribute(attribVisible).isEmpty())
				pCreatedElement->setProperty(attribVisible, domElementIterator.attribute(attribVisible) == "true");

			if (!domElementIterator.attribute(attribEnabled).isEmpty())
				pCreatedElement->setProperty(attribEnabled, domElementIterator.attribute(attribEnabled) == "true");

			if (!domElementIterator.attribute(attribRequiresAuth).isEmpty())
				pCreatedElement->setProperty(attribRequiresAuth, domElementIterator.attribute(attribRequiresAuth) == "true");
		}
	}
}

///// Create ribbon element functions /////
RibbonPage* RobloxRibbonMainWindow::parseAndCreateRibbonPage(const QDomElement& domElement)
{
	// See if it exists first
	QString objectName = getName(domElement);
	RBXASSERT(!objectName.isEmpty());

	QString pageTitle = getText(domElement);
	if (pageTitle.isNull())
		pageTitle = objectName;

	RibbonPage* pRibbonPage = createRibbonPage(objectName, pageTitle);

	// Check to see if this is a context tab 
	QString contextTitle = domElement.attribute(attribContextTitle); 
	if (!contextTitle.isEmpty())
	{
		pRibbonPage->setContextTitle(contextTitle);
		//(http://www.devmachines.com/productdocs/ribbon/qtitan.html#ContextColor-enum)
		if (!domElement.attribute(attribContextColor).isEmpty())
			pRibbonPage->setContextColor((Qtitan::RibbonPage::ContextColor)domElement.attribute(attribContextColor).toInt());
	}

	//now recursively create child elements
	parseAndCreateChildren(domElement, pRibbonPage);

	return pRibbonPage;
}

Qtitan::RibbonGroup* RobloxRibbonMainWindow::parseAndCreateRibbonGroup(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonPage* pParentPage = dynamic_cast<Qtitan::RibbonPage*>(pParent);
	if (!pParentPage)
		throw RBX::runtime_error("Group parent must be a RibbonPage!");

	Qtitan::RibbonGroup* pRibbonGroup = createRibbonGroup(pParentPage, getName(domElement));
	RBXASSERT(pRibbonGroup);
	
	pRibbonGroup->setBaseSize(getSize(domElement, QSize(100, 50)));
	pRibbonGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	if (domElement.attribute("options") == "true")
		pRibbonGroup->setOptionButtonVisible();

	//now recursively create child elements
	parseAndCreateChildren(domElement, pRibbonGroup);

	return pRibbonGroup;
}

QWidget* RobloxRibbonMainWindow::parseAndCreateColorPicker(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("Menu buttons must have a ribbon group parent element!");

	QString text = getText(domElement);
	QIcon icon   = getIcon(domElement);
	if (icon.isNull() && text.isEmpty())
		throw RBX::runtime_error("Colorpicker must have an icon or text!");

	QMenu* pDummyMenu = new QMenu;
	connect(pDummyMenu, SIGNAL(aboutToShow ()), this, SLOT(handleColorPickMenu()));
        
    QAction* pAction = getAction(domElement);
    if (!pAction)
        throw RBX::runtime_error("Failed to create action: %s", getName(domElement).toStdString().c_str());
	QAction* pAddedAction = pRibbonGroup->addAction(pAction, getStyle(domElement), pDummyMenu);
    pAddedAction->setIcon(icon);
    pAddedAction->setText(text);
	pAddedAction->setObjectName(getName(domElement));
	pAddedAction->setToolTip(getToolTip(domElement));

	QLayoutItem* pLayoutItem = pRibbonGroup->layout()->itemAt(pRibbonGroup->layout()->count()-1);
    QToolButton* pToolButton = qobject_cast<QToolButton*>(pLayoutItem ? pLayoutItem->widget() : NULL);
	if (pToolButton)
	{
		if (!domElement.attribute(attribCheckable).isEmpty() && (domElement.attribute(attribCheckable) == "true"))
		{
			pToolButton->defaultAction()->setObjectName(getName(domElement));
			pToolButton->defaultAction()->setCheckable(true);
			pToolButton->setCheckable(true);
		}
		pDummyMenu->setParent(pToolButton);
		pToolButton->setObjectName(getName(domElement));
		connect(pToolButton, SIGNAL(clicked()), this, SLOT(handleToolButtonClicked()));
	}
    
	return pToolButton;
}

QMenu* RobloxRibbonMainWindow::parseAndCreateMenuPopupButton(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("Menu buttons must have a ribbon group parent element!");

    QString text = getText(domElement);
	QIcon icon = getIcon(domElement);
	if (icon.isNull() && text.isEmpty())
		throw RBX::runtime_error("Drop downs must have an icon or text!");

	Qt::ToolButtonStyle style = getStyle(domElement);
    QToolButton::ToolButtonPopupMode popupMode = getPopupStyle(domElement);

	QString objectName = getName(domElement);
	ToolButtonProxyMenu* menu = new ToolButtonProxyMenu(objectName, pRibbonGroup);
    QAction* action = menu->menuAction();
    action->setText(text);

    if (menu->selectChangesText && 
        !domElement.attribute(attribSelectChangesText).isEmpty() &&
        domElement.attribute(attribSelectChangesText) == "true")
    {
        menu->selectChangesText = true;
    }

	action->setIcon(icon);
	action->setToolTip(getToolTip(domElement));
	
    pRibbonGroup->addAction(action);

    QString settingName;
    
    if (domElement.attribute(attribSetting) == "true")
    {
        settingName = objectName + "_RibbonSetting";
    }
    
    QLayoutItem* item = pRibbonGroup->layout()->itemAt(pRibbonGroup->layout()->count()-1);
    if (QToolButton* widgetAction = qobject_cast<QToolButton*>(item ? item->widget() : Q_NULL)) 
    {
		if (!domElement.attribute(attribCheckable).isEmpty() && (domElement.attribute(attribCheckable) == "true"))
		{
			widgetAction->defaultAction()->setObjectName(getName(domElement));
			widgetAction->defaultAction()->setCheckable(true);
			widgetAction->setCheckable(true);
		}
		widgetAction->setToolButtonStyle(style);
        widgetAction->setPopupMode(popupMode);
		connect(widgetAction, SIGNAL(clicked()), this, SLOT(handleToolButtonClicked()));
		widgetAction->setObjectName(objectName);
        menu->toolButton = widgetAction;
        menu->settingName = settingName;
    }

	menu->setObjectName(objectName);
	menu->menuAction()->setObjectName(objectName);
	
	// check if we need to handle menu execution at top level
	if (!domElement.attribute(attribMenuTrigger).isEmpty() && domElement.attribute(attribMenuTrigger) == "true")
		connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(handleActionSelected(QAction*)));

	// now recursively create child elements
	parseAndCreateChildren(domElement, menu);
    
	QString value = domElement.attribute(attribValue);
    
    if (!settingName.isEmpty())
    {
        value = RobloxSettings().value(settingName, value).toString();
    }

    action->setProperty("selectedItem", value);
    
    QList<QAction*> actions = menu->actions();
    for (QList<QAction*>::iterator iter = actions.begin();
         iter != actions.end();
         iter++)
    {
        if ((*iter)->objectName() == value)
        {
            menu->toolButton->connect(menu->toolButton, SIGNAL(clicked()), *iter, SLOT(trigger()));
            menu->connect(*iter, SIGNAL(changed()), menu, SLOT(onChanged()));
            menu->boundAction = *iter;

            QString iconPath = (*iter)->property("iconPath").toString();
            QSize iconSize = menu->toolButton->icon().availableSizes().first();
            
            menu->toolButton->defaultAction()->setIcon(QIcon(QPixmap(iconPath).scaled(iconSize)));

            if (!settingName.isEmpty())
            {
                RobloxSettings().setValue(settingName, value);
            }
        }
    }
    
	return menu;
}

void ToolButtonProxyMenu::onChanged()
{
    QAction* action = qobject_cast<QAction*>(sender());
    
    if (action->isEnabled() != toolButton->isEnabled())
        toolButton->setEnabled(action->isEnabled());
}

QMenu* RobloxRibbonMainWindow::parseAndCreateMenu(const QDomElement& domElement, QObject* pParent)
{	
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("Menu buttons must have a ribbon group parent element!");

	QString text = getText(domElement);
	QIcon icon   = getIcon(domElement);
	if ( icon.isNull() && text.isEmpty() )
		throw RBX::runtime_error("Drop downs must have an icon or text!");

	QMenu* pMenu = pRibbonGroup->addMenu(icon, text, getStyle(domElement));
	connect(pMenu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));

	RBXASSERT(pMenu);
	RBXASSERT(pMenu->menuAction());

	QString objectName = getName(domElement);
	pMenu->setObjectName(objectName);
	pMenu->menuAction()->setToolTip(getToolTip(domElement));
	pMenu->menuAction()->setObjectName(objectName);
	
	// check if we need to handle menu execution at top level
	if (!domElement.attribute(attribMenuTrigger).isEmpty() && domElement.attribute(attribMenuTrigger) == "true")
		connect(pMenu, SIGNAL(triggered(QAction*)), this, SLOT(handleMenuActionTrigger(QAction*)));

	// now recursively create child elements
	if (objectName == "emulateDeviceAction")
        StudioDeviceEmulator::Instance().addEmulationChildren(this, pMenu);
	
	parseAndCreateChildren(domElement, pMenu);

	if ( !domElement.attribute(attribCheckable).isEmpty() )
		pMenu->menuAction()->setCheckable(domElement.attribute(attribCheckable) == "true");

	if (domElement.attribute(attribIconVisibleInMenu) == "false")
	{
		QList<QAction*> actions = pMenu->actions();
		for (int ii = 0; ii < actions.size(); ii++)
			actions[ii]->setIconVisibleInMenu(false);
	}

	return pMenu;
}

Qtitan::RibbonGallery* RobloxRibbonMainWindow::parseAndCreateRibbonGallery(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("Gallery parent must be a ribbon group!");

	Qtitan::RibbonGallery* pGallery = new Qtitan::RibbonGallery;
	pGallery->setObjectName(getName(domElement));
	pGallery->setScrollBarPolicy(Qt::ScrollBarAsNeeded);
	
    Qtitan::RibbonGalleryGroup* pGalleryGroup = Qtitan::RibbonGalleryGroup::createGalleryGroup();
	pGalleryGroup->setClipItems(false);

	// Set up the size of each color spot and populate colors
	QSize itemSize(domElement.attribute(attribItemWidth).toInt(), domElement.attribute(attribItemHeight).toInt());
	pGalleryGroup->setSize(itemSize.isNull() ? QSize(16, 16) : itemSize);

    pGallery->setGalleryGroup(pGalleryGroup);

	// check if we need to create color gallery (special case)
	if (domElement.attribute(attribType) == "color")
	{
		GalleryItemColor::addStandardColors(pGalleryGroup);
		pGallery->setCheckedIndex(12);
	}
	else
	{
		parseAndCreateChildren(domElement, pGalleryGroup);
		pGallery->setCheckedIndex(0);
	}

	RBXASSERT(pGalleryGroup->getItemCount());

	int numColumns = domElement.attribute(attribColumns).toInt();
	if (!numColumns)
		numColumns = 5;

	int galleryWidth  = (itemSize.width() * numColumns);
	int galleryHeight = (qCeil(pGalleryGroup->getItemCount()/(float)numColumns)*itemSize.height());
	
	if (!domElement.attribute(attribPopup).isEmpty() && domElement.attribute(attribPopup).toLower() == "false")
	{
		if (galleryHeight > 64)
		{
			galleryHeight = 64;
			galleryWidth  = galleryWidth + qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

			pGallery->setScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		}

		pGallery->setBaseSize(QSize(galleryWidth, galleryHeight));
		pRibbonGroup->addWidget(pGallery);
	}
	else
	{
		pGallery->setBaseSize(QSize(galleryWidth+4, galleryHeight+2));
		
		OfficePopupMenu* pPopupMenu = OfficePopupMenu::createPopupMenu(ribbonBar());	
		pPopupMenu->addWidget(pGallery);

		QAction* pAction = pRibbonGroup->addAction(getIcon(domElement), getText(domElement), getStyle(domElement), pPopupMenu, QToolButton::InstantPopup);
		pAction->setObjectName(getName(domElement));
	}

	connect(pGallery, SIGNAL(itemClicked(RibbonGalleryItem*)), this, SLOT(handleBasicObjectsGalleryItemClicked(RibbonGalleryItem*)));

	return pGallery;
}

Qtitan::RibbonGalleryItem* RobloxRibbonMainWindow::parseAndCreateGalleryItem(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGalleryGroup* pGalleryGroup = qobject_cast<Qtitan::RibbonGalleryGroup*>(pParent);
	if (!pGalleryGroup)
		throw RBX::runtime_error("galleryitem must be beneath a gallery group");

	Qtitan::RibbonGalleryItem* pGalleryItem = new StudioGalleryItem;

	pGalleryItem->setCaption(getText(domElement));
	pGalleryItem->setToolTip(getToolTip(domElement));
	pGalleryItem->setSizeHint(pGalleryGroup->getSize());
	pGalleryItem->setData(GALLERY_ITEM_USER_DATA, domElement.attribute(attribValue));

	QIcon icon = getIcon(domElement);
	if (!icon.isNull())
		pGalleryItem->setIcon(icon);

	pGalleryGroup->appendItem(pGalleryItem);

	return pGalleryItem;
}

QActionGroup* RobloxRibbonMainWindow::parseAndCreateActionGroup(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	QMenu* pMenu                = qobject_cast<QMenu*>(pParent);
	if (!pGroup && !pMenu)
		throw RBX::runtime_error("actiongroup elements must be beneath a group or menu");

	// Qt doesn't let us add same actions in multiple action groups, so we must reuse the already created action group
	QString objectName = getName(domElement);
	QActionGroup* pActionGroup = findChild<QActionGroup*>(objectName);

	if (!pActionGroup)
	{
		pActionGroup = new QActionGroup(this);
		pActionGroup->setObjectName(objectName);

		bool isSingleSelect = domElement.attribute(attribType) == "singleselect";
		pActionGroup->setExclusive(!isSingleSelect);
		pActionGroup->setProperty("singleselect", isSingleSelect);

		connect(pActionGroup, SIGNAL(selected(QAction*)), this, SLOT(handleActionGroupSelected(QAction*)));
	}

	// we will be parsing actions to a dummy action group and later add the created actions to the original action group
	QActionGroup dummyActionGroup(this);
	//now recursively create child elements 
	parseAndCreateChildren(domElement, &dummyActionGroup);

	bool isCheckable = domElement.attribute(attribCheckable) == "true";

	QAction* pAction = NULL;
	QList<QAction*> actions = dummyActionGroup.actions();
	for (int ii = 0; ii < actions.size(); ii++)
	{
		pAction = actions.at(ii);
		pAction->setCheckable(isCheckable);
		// add action to correct parent i.e. Ribbon Group or Menu
		if (pGroup)
			pGroup->addAction(pAction, getStyle(domElement));
		else
			pMenu->addAction(pAction);
		// add actions to the original group (NOTE: dummy action group will not be having these actions anymore after this)
		pActionGroup->addAction(pAction);
	}

	return pActionGroup;
}

QButtonGroup* RobloxRibbonMainWindow::parseAnCreateButtonGroup(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("buttongroup must have a ribbon group parent element!");

	QButtonGroup* pButtonGroup = new QButtonGroup(this);
	pButtonGroup->setObjectName(getName(domElement));

	parseAndCreateChildren(domElement, pButtonGroup);

	// add created buttons to ribbon group
	QList<QAbstractButton*> buttons = pButtonGroup->buttons();
	if (buttons.size() > 0)
	{
		for (int ii = 0; ii < buttons.size(); ++ii)
		{
			// populate default value if any
			if (buttons.at(ii)->isChecked())
				populateNameValueStore(pButtonGroup->objectName(), buttons.at(ii));
			// add widget to ribbon group
			pRibbonGroup->addWidget(buttons.at(ii));
		}

		connect(pButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(handleButtonGroupSelected(QAbstractButton*)));
	}

	return pButtonGroup;
}

QAction* RobloxRibbonMainWindow::parseAndCreateSplitButton(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("splitbutton must have a ribbon group parent element!");

	QMenu* pMenu = new QMenu(this);
	parseAndCreateChildren(domElement, pMenu);

	QAction* pAction = pRibbonGroup->addAction(getIcon(domElement), getText(domElement), getStyle(domElement), pMenu);
	pAction->setObjectName(getName(domElement));
	if (!FFlag::StudioSeparateActionByActivationMethod)
		connect(pAction, SIGNAL(triggered(bool)), this, SLOT(commonSlot(bool)));

	return pAction;
}

QAction* RobloxRibbonMainWindow::parseAndCreateSeparator(const QDomElement& domElement, QObject* pParent)
{
	if (QMenu* pParentMenu = qobject_cast<QMenu*>(pParent))
		return pParentMenu->addSeparator();

	if (Qtitan::RibbonGroup* pParentGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent))
		return pParentGroup->addSeparator();
	
	//none of the above - throw
	throw RBX::runtime_error("addseparator elements must be beneath a menu or group!");
}

QRadioButton* RobloxRibbonMainWindow::parseAndCreateRadioButton(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	QButtonGroup*  pButtonGroup       = qobject_cast<QButtonGroup*>(pParent);
	if (!pRibbonGroup && !pButtonGroup)
		throw RBX::runtime_error("radio buttons must be beneath a button/ribbon group");

	QString objectName = getName(domElement);
	if (objectName.isEmpty())
		throw RBX::runtime_error("radio button name can not be empty!");

	// Create the button
	QRadioButton* pRadioButton = new QRadioButton;
	pRadioButton->setObjectName(objectName);
	pRadioButton->setToolTip(getToolTip(domElement));
	pRadioButton->setText(getText(domElement));
	pRadioButton->setChecked(domElement.attribute(attribChecked) == "true");

	if (pRibbonGroup)
		addWidgetToRibbonGroup(pRibbonGroup, pRadioButton);
	else
		addButtonToButtonGroup(pButtonGroup, pRadioButton);

    QAction* pAction = getAction(domElement);
    if (!pAction)
        throw RBX::runtime_error("Failed to create action: %s", getName(domElement).toStdString().c_str());
    
    connect(pRadioButton, SIGNAL(clicked()), pAction, SLOT(trigger()));
	if (!FFlag::StudioSeparateActionByActivationMethod)
        connect(pAction, SIGNAL(triggered()), pRadioButton, SLOT(toggle()));

	return pRadioButton;
}

QCheckBox* RobloxRibbonMainWindow::parseAndCreateCheckBox(const QDomElement& domElement, QObject* pParent, bool actionMode)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("checkbox must be beneath a ribbon group");

	QString objectName = getName(domElement);
	if (objectName.isEmpty())
		throw RBX::runtime_error("checkbox name element can not be empty!");

	// Create button
	QCheckBox* pCheckBox = new QCheckBox;
	pCheckBox->setObjectName(getName(domElement));
	pCheckBox->setText(getText(domElement));
	pCheckBox->setToolTip(getToolTip(domElement));
	pCheckBox->setChecked(domElement.attribute(attribChecked) == "true");

	// add to ribbon group
	addWidgetToRibbonGroup(pRibbonGroup, pCheckBox);

	// if no action is associated then connect with clicked signal to populate valuestore
	if (!actionMode)
		connect(pCheckBox, SIGNAL(clicked(bool)), this, SLOT(handleCheckBoxClicked(bool)));
	
	return pCheckBox;
}

QComboBox* RobloxRibbonMainWindow::parseAndCreateComboBox(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("checkbox must be beneath a ribbon group");

	QString objectName = getName(domElement);
	if (objectName.isEmpty())
		throw RBX::runtime_error("checkbox name element can not be empty!");

	// Create button
	QComboBox* pComboBox = new QComboBox;
	pComboBox->setObjectName(getName(domElement));

	// add combobox items
	for (QDomElement domElementIterator = domElement.firstChildElement(); !domElementIterator.isNull(); domElementIterator = domElementIterator.nextSiblingElement()) 
	{
		if (domElementIterator.tagName().toLower() == elemComboBoxItem)
		{
			pComboBox->addItem(getText(domElementIterator));
			// check if there's any user value
			if (!domElementIterator.attribute(attribValue).isEmpty())
				pComboBox->setItemData(pComboBox->count() - 1, domElementIterator.attribute(attribValue));
			// if this index is default
			if (domElementIterator.attribute(attribDefaultValue) == "true")
				pComboBox->setCurrentIndex(pComboBox->count() - 1);
		}
	}

	// add to ribbon group
	addWidgetToRibbonGroup(pRibbonGroup, pComboBox);

	connect(pComboBox, SIGNAL(activated(int)), this, SLOT(handleComboBoxActivated(int)));
	
	return pComboBox;
}

QAction* RobloxRibbonMainWindow::parseAndCreateRibbonAction(const QDomElement& domElement, QObject* pParent)
{
	if (!pParent)
		throw RBX::runtime_error("Failed to create action: %s.  No parent element provided!", getName(domElement).toStdString().c_str());

	QAction* pAction = getAction(domElement);
	if (!pAction)
		throw RBX::runtime_error("Failed to create action: %s", getName(domElement).toStdString().c_str());

	addActionToParent(pAction, pParent, domElement);	

	// If we have verbs that handle these actions, we just need to connect these actions to the common slot,
	// which will invoke the associated verb.  In the future we should consider making the verb list in the common 
	// slot just a list of strings instead of actual QAction*s.  That way we can add these actions to that list
	// instead of marking them as "iscommonslot=true" in the RobloxStudioRibbon.xml file.
	if (!FFlag::StudioSeparateActionByActivationMethod && domElement.attribute(attribCommonSlot) == "true")
		connect(pAction, SIGNAL(triggered(bool)), this, SLOT(commonSlot(bool)));
	
	return pAction;
}

QMenu* RobloxRibbonMainWindow::parseAndCreateGroupOptionsMenu(const QDomElement& domElement, QObject* pParent)
{
	Qtitan::RibbonGroup* pRibbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent);
	if (!pRibbonGroup)
		throw RBX::runtime_error("optionsMenu must be beneath a ribbon group");

	QAction* optionButtonAction = pRibbonGroup->getOptionButtonAction();
	if (!optionButtonAction)
		throw RBX::runtime_error("optionsMenu must has an action to associate");

	QMenu* pMenu = new QMenu(pRibbonGroup);
	pMenu->setObjectName(QString("__%1Group-Menu_").arg(pRibbonGroup->objectName()));
	parseAndCreateChildren(domElement, pMenu);

	if (!pMenu->isEmpty())
	{
		pMenu->setProperty("useActionName", true);
		connect(pMenu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));

		optionButtonAction->setToolTip(getToolTip(domElement));
		connect(optionButtonAction, SIGNAL(triggered()), this, SLOT(showGroupOptionsMenu()));

		// check if we need to handle menu execution at top level
		if (!domElement.attribute(attribMenuTrigger).isEmpty() && domElement.attribute(attribMenuTrigger) == "true")
			connect(pMenu, SIGNAL(triggered(QAction*)), this, SLOT(handleMenuActionTrigger(QAction*)));
	}

	return pMenu;
}

QMenu* RobloxRibbonMainWindow::parseAndCreateSubMenu(const QDomElement& domElement, QObject* parent)
{
	QMenu* menuParent = qobject_cast<QMenu*>(parent);
	if (!menuParent)
		throw RBX::runtime_error("submenu must be beneath a QMenu");
	
	QMenu* submenu = new QMenu(domElement.attribute("name"));
	parseAndCreateChildren(domElement, submenu);
	menuParent->addMenu(submenu);
	connect(submenu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));
	return submenu;
}

void RobloxRibbonMainWindow::showGroupOptionsMenu()
{
	QAction* pSender = qobject_cast<QAction*>(sender());
	if (pSender)
	{
		QWidget* pWidget = pSender->parentWidget();
		if (pWidget)
		{
			QMenu* pMenu = pWidget->findChild<QMenu*>(QString("__%1Group-Menu_").arg(pWidget->objectName()));
			if (pMenu)
				pMenu->exec(QCursor::pos());
		}
	}
}

void RobloxRibbonMainWindow::updateProxyAction()
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	QMap<QString, QAction*>::iterator iter = m_mProxyActions.find(getFullName(pAction));
	if (iter == m_mProxyActions.end())
		return;

	iter.value()->setEnabled(pAction->isEnabled());
	iter.value()->setChecked(pAction->isChecked());
	iter.value()->setVisible(pAction->isVisible());
	iter.value()->setIcon(pAction->icon());
}

///// Utility functions /////
QString RobloxRibbonMainWindow::getName(const QDomElement& domElement) const
{	return domElement.attribute(attribName); }

QString RobloxRibbonMainWindow::getToolTip(const QDomElement& domElement) const
{	
	// if we've tool tip then return 
	if (!domElement.attribute(attribToolTip).isEmpty())
		return domElement.attribute(attribToolTip); 

	// return "text"(i.e. title)
	if (!domElement.attribute(attribText).isEmpty())
		return domElement.attribute(attribText); 

	// nothing, return name
	return getName(domElement);
}

QString RobloxRibbonMainWindow::getText(const QDomElement& domElement) const
{	
	if (!domElement.attribute(attribText).isEmpty())
		return domElement.attribute(attribText); 

	return domElement.attribute(attribName);
}

QSize RobloxRibbonMainWindow::getSize(const QDomElement& domElement, const QSize& defaultSize) const
{
	QSize size(defaultSize);

	int width = domElement.attribute(attribIconWidth).toInt();
	if (width > 0)
		size.setWidth(width);

    int height = domElement.attribute(attribIconHeight).toInt();
	if (height > 0)
		size.setHeight(height);

	return size;
}

QIcon RobloxRibbonMainWindow::getIcon(const QDomElement& domElement, const QSize& defaultSize) const
{
	QSize size(getSize(domElement, defaultSize));

	QPixmap pixmap(domElement.attribute(attribIcon));
	if (!pixmap.isNull())
		pixmap = pixmap.scaled(size);

	return QIcon(pixmap);
}

Qt::ToolButtonStyle RobloxRibbonMainWindow::getStyle( const QDomElement& domElement ) const
{
	QString style = domElement.attribute(attribStyle);
	if (style == "icononly")
		return Qt::ToolButtonIconOnly;
	if (style == "textonly")
		return Qt::ToolButtonTextOnly;
	if (style == "textbesideicon")
		return Qt::ToolButtonTextBesideIcon;
	if (style == "textundericon")
		return Qt::ToolButtonTextUnderIcon;
	return Qt::ToolButtonFollowStyle;
}

enum QToolButton::ToolButtonPopupMode RobloxRibbonMainWindow::getPopupStyle(const QDomElement& domElement) const
{
	QString style = domElement.attribute(attribPopupStyle);
	if (style == "delayed")
		return QToolButton::DelayedPopup;
	if (style == "menubutton")
		return QToolButton::MenuButtonPopup;
	if (style == "instant")
		return QToolButton::InstantPopup;
    
	return QToolButton::InstantPopup;
}


QAction* RobloxRibbonMainWindow::getAction(const QDomElement& domElement)
{
	QString objectName = getName(domElement);
	if (objectName.isEmpty())
		return NULL;

	QAction* pAction = findChildAction(objectName);
	if (!pAction)
	{
		if (domElement.attribute(attribType) == "plugin")
		{
			pAction = new PluginAction(getText(domElement), this);
		}
		else
		{
			pAction = new QAction(getText(domElement), this);
		}

		// add newly created actions to mainwindow, so it is available for customization
		addAction(pAction);

		pAction->setObjectName(objectName);
		pAction->setEnabled(true);
		pAction->setVisible(true);
	}

	if ( !domElement.attribute(attribText).isEmpty() )
		pAction->setText(domElement.attribute(attribText));

    if ( !domElement.attribute(attribIcon).isEmpty())
        pAction->setProperty("iconPath", domElement.attribute(attribIcon));
    
	QIcon icon = getIcon(domElement);
	if (!icon.isNull())
		pAction->setIcon(icon);

	if ( !domElement.attribute(attribToolTip).isEmpty() )
		pAction->setToolTip(domElement.attribute(attribToolTip));

	if ( !domElement.attribute(attribInstanceName).isEmpty() )
		pAction->setData(domElement.attribute(attribInstanceName));

	if ( !domElement.attribute(attribCheckable).isEmpty() )
		pAction->setCheckable(domElement.attribute(attribCheckable) == "true");

	if (domElement.attribute(attribType) == "plugin")
	{
		RobloxQuickAccessConfig& quickAccessConfig = RobloxQuickAccessConfig::singleton();
		if (QAction* mappedAction = quickAccessConfig.mapProxyAction(pAction))
			quickAccessConfig.addToQuickAccessBar(mappedAction, quickAccessConfig.visibleInQuickAccessBar(pAction));
	}
	
	if (domElement.attribute(attribProxyAction) == "true")
	{
		// since this action is a proxy action, it's source action will be added to quick access dialog
		pAction->setProperty("disableForQuickAccess", true);
		m_mProxyActions.insert(objectName, pAction);
	}

	return pAction;
}

void RobloxRibbonMainWindow::addWidgetToRibbonGroup(Qtitan::RibbonGroup* pRibbonGroup, QWidget* pWidget)
{
	if (!pRibbonGroup || !pWidget)
		return;

	QAction* pAction = pRibbonGroup->addWidget(pWidget);
	pAction->setObjectName(pWidget->objectName());
	pAction->setCheckable(true);

	QAction* pMainWindowAction = findChildAction(pWidget->objectName());
	if (pMainWindowAction)
	{
		/*
		* This sets up bidirectional trigger handling to make sure menu actions are in sync with existing action.
		* This is because we have to have separate actions for displaying some buttons on the ribbon bar than
		* the existing action that actually handles the execution.
		*/
		m_mCorrelaryActionMap.insert(pMainWindowAction, pWidget);

		//execute action
		if (qobject_cast<QAbstractButton*>(pWidget))
			connect(pWidget, SIGNAL(clicked(bool)), pMainWindowAction, SLOT(trigger()));
	}
}

void RobloxRibbonMainWindow::addButtonToButtonGroup(QButtonGroup* pButtonGroup, QAbstractButton* pButton)
{	
	// add button the ribbon group
	pButtonGroup->addButton(pButton);		
}

void RobloxRibbonMainWindow::addActionToParent(QAction* pAction, QObject* pParent, const QDomElement& domElement)
{
	if ( Qtitan::RibbonGallery* pRibbonGallery = qobject_cast<Qtitan::RibbonGallery*>(pParent) )
	{
        RibbonGalleryGroup* pRibbonGalleryGroup = pRibbonGallery->galleryGroup();
		if ( !pRibbonGalleryGroup )
			throw RBX::runtime_error("Failed to create action: %s. RibbonGallery must have RibbonGalleryGroup!", qPrintable(pAction->objectName()));

		RibbonGalleryItem* pGalleryItem = pRibbonGalleryGroup->addItem(pAction->text(), pAction->icon().pixmap(getSize(domElement)));
		pGalleryItem->setData(0,pAction->data());
		pGalleryItem->setToolTip(pAction->text());
        pGalleryItem->setStatusTip(pAction->text());
	}
	else if (Qtitan::RibbonGroup* ribbonGroup = qobject_cast<Qtitan::RibbonGroup*>(pParent))
	{
		ribbonGroup->addAction(pAction, getStyle(domElement));
	}
	else if (QWidget* pParentWidget = qobject_cast<QWidget*>(pParent))
	{
		pParentWidget->addAction(pAction);
	}
	else if (QActionGroup* pParentActionGroup = qobject_cast<QActionGroup*>(pParent))
	{
		pParentActionGroup->addAction(pAction);
	}
	else
	{
		pAction->setParent(pParent);
	}
}

///// Slots /////
void RobloxRibbonMainWindow::handleActionGroupSelected(QAction *pAction)
{
	const QActionGroup* pActionGroup = qobject_cast<const QActionGroup*>(sender());
	if (!pActionGroup)
		return;

	//make sure we have only one action is selected
	if (pActionGroup->property("singleselect").toBool())
	{
		QList<QAction*> actions = pActionGroup->actions();
		for (int i = 0; i < actions.size(); i++)
		{

			if (actions.at(i) != pAction)
				actions.at(i)->setChecked(false);
		}
	}

	//now execute the action
	if (RobloxDocManager::Instance().getCurrentDoc())
	{
		//set the dynamic properties for later use only if the action is checked
		if (!pAction->isCheckable() || (pAction->isCheckable() && pAction->isChecked()))
			populateNameValueStore(pActionGroup->objectName(), pAction);
		else
			NameValueStoreManager::singleton().clear(pActionGroup->objectName());

		RobloxDocManager::Instance().getCurrentDoc()->handleAction(pActionGroup->objectName(), pAction->isChecked());
	}
}

void RobloxRibbonMainWindow::handleButtonGroupSelected(QAbstractButton* pButton)
{
	const QButtonGroup* pButtonGroup = qobject_cast<const QButtonGroup*>(sender());
	if (!pButtonGroup)
		return;

	//now execute the button
	if (RobloxDocManager::Instance().getCurrentDoc())
	{		
		populateNameValueStore(pButtonGroup->objectName(), pButton);
		RobloxDocManager::Instance().getCurrentDoc()->handleAction(pButtonGroup->objectName(), pButton->isChecked());
	}
}

void RobloxRibbonMainWindow::handleActionSelected(QAction* pAction)
{
	ToolButtonProxyMenu* pMenu = qobject_cast<ToolButtonProxyMenu*>(sender());
    
	if (!pMenu)
		return;
    
    QToolButton* toolButton = pMenu->toolButton;
    
    if (pMenu->boundAction)
    {       
        toolButton->disconnect(toolButton, SIGNAL(clicked()), pMenu->boundAction, SLOT(trigger()));
        pMenu->disconnect(pMenu->boundAction, SIGNAL(changed()), pMenu, SLOT(onChanged()));
    }
    
    pMenu->toolButton->connect(pMenu->toolButton, SIGNAL(clicked()), pAction, SLOT(trigger()));
    pMenu->connect(pAction, SIGNAL(changed()), pMenu, SLOT(onChanged()));
    
    pMenu->boundAction = pAction;
    
    if (!pMenu->settingName.isEmpty())
    {
        RobloxSettings().setValue(pMenu->settingName, pAction->objectName());
    }

    QString iconPath = pAction->property("iconPath").toString();

    if (!pMenu->toolButton->icon().isNull())
    {
        QSize iconSize = pMenu->toolButton->icon().availableSizes().first();
        pMenu->toolButton->defaultAction()->setIcon(QIcon(QPixmap(iconPath).scaled(iconSize)));
    }

    if (pMenu->selectChangesText)
        pMenu->toolButton->setText(pAction->text());

	//now execute the button
	if (RobloxDocManager::Instance().getCurrentDoc())
	{
		populateNameValueStore(pMenu->objectName(), pAction);
		RobloxDocManager::Instance().getCurrentDoc()->handleAction(pMenu->objectName(), pAction->isChecked());
		UpdateUIManager::Instance().updateToolBars();
	}
}

void RobloxRibbonMainWindow::handleMenuActionTrigger(QAction* pAction)
{
	const QMenu* pMenu = qobject_cast<const QMenu*>(sender());
	if (!pMenu)
		return;

	//now execute the button
	if (RobloxDocManager::Instance().getCurrentDoc())
	{		
		if (pMenu->property("useActionName").toBool())
		{
			RobloxDocManager::Instance().getCurrentDoc()->handleAction(pAction->objectName(), pAction->isChecked());
			UpdateUIManager::Instance().updateToolBars();
		}
		else
		{
			populateNameValueStore(pMenu->objectName(), pAction);
			RobloxDocManager::Instance().getCurrentDoc()->handleAction(pMenu->objectName(), pAction->isChecked());
		}
	}
}

void RobloxRibbonMainWindow::handleBasicObjectsGalleryItemClicked(RibbonGalleryItem* item)
{
	Qtitan::RibbonGallery* pGallery = qobject_cast<Qtitan::RibbonGallery*>(sender());
	if (!pGallery)
		return;

	pGallery->setCheckedItem(item);

	//now execute the action
	if (RobloxDocManager::Instance().getCurrentDoc())
	{		
		NameValueStoreManager::singleton().setValue(pGallery->objectName(), "user_value", item->data(100));
		RobloxDocManager::Instance().getCurrentDoc()->handleAction(pGallery->objectName(), false);
	}
}

void RobloxRibbonMainWindow::handleCheckBoxClicked(bool state)
{
	const QCheckBox* pCheckBox = qobject_cast<const QCheckBox*>(sender());
	if (!pCheckBox)
		return;

	if (RobloxDocManager::Instance().getCurrentDoc())
	{
		NameValueStoreManager::singleton().setValue(pCheckBox->objectName(), "enabled", pCheckBox->isEnabled());
		NameValueStoreManager::singleton().setValue(pCheckBox->objectName(), "checked", pCheckBox->isChecked());
		RobloxDocManager::Instance().getCurrentDoc()->handleAction(pCheckBox->objectName(), false);
	}
}

void RobloxRibbonMainWindow::handleComboBoxActivated(int index)
{
	const QComboBox* pComboBox = qobject_cast<const QComboBox*>(sender());
	if (!pComboBox)
		return;

	if (RobloxDocManager::Instance().getCurrentDoc())
	{
		if (index < 0)
		{
			NameValueStoreManager::singleton().clear(pComboBox->objectName());
		}
		else
		{
			NameValueStoreManager::singleton().setValue(pComboBox->objectName(), "index", index);
			NameValueStoreManager::singleton().setValue(pComboBox->objectName(), "user_value", pComboBox->itemData(index));
		}
	}
}

void RobloxRibbonMainWindow::handleItemSelected(const QString& selectedItem)
{
	QToolButton* item = qobject_cast<QToolButton*>(sender());

	if (!item || !item->defaultAction())
        return;

	// Now execute the action.
	if (RobloxDocManager::Instance().getCurrentDoc())
	{
        NameValueStoreManager::singleton().setValue(item->objectName(), "user_value", selectedItem);
        item->defaultAction()->setProperty("selectedItem", selectedItem);
		RobloxDocManager::Instance().getCurrentDoc()->handleAction(item->objectName(), false);
		UpdateUIManager::Instance().updateToolBars();
	}
}

void RobloxRibbonMainWindow::handleToolButtonClicked()
{
	QToolButton* item = qobject_cast<QToolButton*>(sender());

	if (!item || !item->defaultAction())
        return;

    handleItemSelected(item->defaultAction()->property("selectedItem").toString());
}

void RobloxRibbonMainWindow::handleColorPickMenu()
{
	QMenu* pMenu = qobject_cast<QMenu*>(sender());
	QToolButton* pButton = pMenu ? qobject_cast<QToolButton*>(pMenu->parent()) : NULL;
	if (!pMenu || !pButton)
		return;

	// create colorpicker frame and it show at appropriate location
	ColorPickerFrame *pPickerFrame = new ColorPickerFrame(this);
	QVariant var = pButton->defaultAction()->property("selectedIndex");
	if (!var.isNull() && var.isValid())
		pPickerFrame->setSelectedIndex(var.toInt());
	pPickerFrame->move(pButton->mapToGlobal(pButton->rect().bottomLeft()));
	pPickerFrame->show();

	// create an event loop
	QEventLoop* eventLoop = new QEventLoop(pPickerFrame);
	pPickerFrame->setEventLoop(eventLoop);		
	eventLoop->exec();

	// get selected color
	int selectedColor = pPickerFrame->getSelectedItem();

	// let document handle the selection
	if (selectedColor != -1 && RobloxDocManager::Instance().getCurrentDoc())
	{
		NameValueStoreManager::singleton().setValue(pButton->objectName(), "user_value", selectedColor);
		pButton->defaultAction()->setProperty("selectedItem", selectedColor);
		pButton->defaultAction()->setProperty("selectedIndex", pPickerFrame->getSelectedIndex());
		RobloxDocManager::Instance().getCurrentDoc()->handleAction(pButton->objectName(), false);
		UpdateUIManager::Instance().updateToolBars();
	}
	
	// cleanup
	delete eventLoop;
	delete pPickerFrame;
	pMenu->hide();
	pButton->setDown(false);
}

void RobloxRibbonMainWindow::handleRibbonMinimizationChanged(bool minimized)
{
    m_pRibbonMinimizeAction->setChecked(minimized);

    m_pRibbonMinimizeAction->setIcon(minimized ? QIcon(":/RibbonBar/images/Studio Ribbon Icons/ribbonMaximize.png") 
											   : QIcon(":/RibbonBar/images/Studio Ribbon Icons/ribbonMinimize.png"));	

	RobloxSettings().setValue(settingRibbonMinimized, minimized);	
}

void RobloxRibbonMainWindow::toggleRibbonMinimize()
{	
	ribbonBar()->setMinimized(!ribbonBar()->isMinimized()); 
}

void RobloxRibbonMainWindow::handleStyleChange(QAction* pAction)
{
    if (!m_pRibbonStyle)
		return;

	Qtitan::RibbonStyle::Theme themeId = Qtitan::RibbonStyle::Office2010Black;
	if (pAction->objectName() == "Office2007Blue")
		themeId = Qtitan::RibbonStyle::Office2007Blue;
	else if (pAction->objectName() == "Office2007Black")
		themeId = Qtitan::RibbonStyle::Office2007Black;
	else if (pAction->objectName() == "Office2007Silver")
		themeId = Qtitan::RibbonStyle::Office2007Silver;
	else if (pAction->objectName() == "Office2007Aqua")
		themeId = Qtitan::RibbonStyle::Office2007Aqua;
	else if (pAction->objectName() == "Windows7Scenic")
		themeId = Qtitan::OfficeStyle::Windows7Scenic;
	else if (pAction->objectName() == "Office2010Blue")
		themeId = Qtitan::OfficeStyle::Office2010Blue;
	else if (pAction->objectName() == "Office2010Silver")
		themeId = Qtitan::OfficeStyle::Office2010Silver;
	else if (pAction->objectName() == "Office2010Black")
		themeId = Qtitan::OfficeStyle::Office2010Black;
	else if (pAction->objectName() == "Office2013White")
		themeId = Qtitan::OfficeStyle::Office2013White;
	else if (pAction->objectName() == "Office2013Gray")
		themeId = Qtitan::OfficeStyle::Office2013Gray;

	setTheme(themeId);	
}

void RobloxRibbonMainWindow::initialize()
{
	cleanupToolBars();

	// Add all previously created actions to the ribbon so we have accelerators (even though they may not be visible)
	QList<QAction*> childActions = this->findChildren<QAction*>();
	for (int i = 0; i < childActions.length(); i++)
		this->addAction(childActions.at(i));

	// we do no want mainwindow to steal focus
    setFocusPolicy(Qt::NoFocus);

	QByteArray fontData = QtUtilities::getResourceFileBytes(":/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf");
	int fontIndex = QFontDatabase::addApplicationFontFromData(fontData);

	QStringList loadedFontFamilies = QFontDatabase::applicationFontFamilies(fontIndex);
	RBXASSERT(!loadedFontFamilies.empty());

	m_ribbonFont = QFont(loadedFontFamilies.at(0), 9);

#ifdef Q_WS_MAC
	// On Mac we need to have bigger fonts
	m_ribbonFont.setPointSize(m_ribbonFont.pointSize() + 4);

	QApplication::setFont(m_ribbonFont);
	QApplication::setFont(m_ribbonFont, "QListWidget");
#else
	QApplication::setFont(m_ribbonFont);
#endif
	
	// create default style
	m_pRibbonStyle = new Qtitan::RibbonStyle();
	m_pRibbonStyle->setTheme(Qtitan::OfficeStyle::Office2013White);
	m_pRibbonStyle->setAnimationEnabled(false);
	qApp->setStyle(m_pRibbonStyle);

	//  ribbon bar object (MUST)
	setRibbonBar(new RibbonBar(this));

	ribbonBar()->setFont(m_ribbonFont);
}

void RobloxRibbonMainWindow::cleanupToolBars()
{
	QList<QToolBar*> toolbars = findChildren<QToolBar*>();
	for (int ii = 0; ii < toolbars.count(); ++ii)
	{
		if (toolbars.at(ii) != commandToolBar)
		{
			removeToolBar(toolbars.at(ii));
			delete toolbars.at(ii);			
		}
	}
}

void RobloxRibbonMainWindow::updateFonts(const QFont& font)
{
	if (font == m_ribbonFont)
	{
		QFont smallerFont(font);
		smallerFont.setPointSize(font.pointSize() - 1);

		const QList<QWidget*> ribbonWidgets = ribbonBar()->findChildren<QWidget*>();
		for (int ii = 0; ii < ribbonWidgets.count(); ii++)
		{
			if (ribbonWidgets[ii] && ribbonWidgets[ii]->metaObject())
			{
#ifdef Q_WS_MAC
                // Mac requires setting of fonts to both i.e. Tab as well as other children
                ribbonWidgets[ii]->setFont(ribbonWidgets[ii]->metaObject()->className() == QString("Qtitan::RibbonTab") ? smallerFont : font);
#else
                if (ribbonWidgets[ii]->metaObject()->className() == QString("Qtitan::RibbonTab"))
                    ribbonWidgets[ii]->setFont(smallerFont);
#endif
			}
		}

		RibbonSystemButton* pSystemButton = ribbonBar()->getSystemButton();
		if (pSystemButton && pSystemButton->defaultAction() && pSystemButton->defaultAction()->menu())
		{
			pSystemButton->defaultAction()->menu()->setFont(font);
			const QList<QAction*>& systemMenuActions = pSystemButton->defaultAction()->menu()->actions();
			for (int ii = 0; ii < systemMenuActions.count(); ii++)
			{
				if (systemMenuActions[ii])
					systemMenuActions[ii]->setFont(font);
			}
		}
	}
	else
	{
		QApplication::setFont(font);
		// on mac we need explicit font update
#ifdef Q_WS_MAC
		QApplication::setFont(font, "QListWidget");
#endif

		QFont smallerFont(font);
		smallerFont.setPointSize(font.pointSize() - 1);

		ribbonBar()->setFont(font);
		const QList<QWidget*> ribbonWidgets = ribbonBar()->findChildren<QWidget*>();
		for (int ii = 0; ii < ribbonWidgets.count(); ii++)
		{
			if (ribbonWidgets[ii] && ribbonWidgets[ii]->metaObject())
				ribbonWidgets[ii]->setFont(ribbonWidgets[ii]->metaObject()->className() == QString("Qtitan::RibbonTab") ? smallerFont : font);
		}

		RibbonSystemButton* pSystemButton = ribbonBar()->getSystemButton();
		if (pSystemButton && pSystemButton->defaultAction() && pSystemButton->defaultAction()->menu())
		{
			pSystemButton->defaultAction()->menu()->setFont(font);
			const QList<QAction*>& systemMenuActions = pSystemButton->defaultAction()->menu()->actions();
			for (int ii = 0; ii < systemMenuActions.count(); ii++)
			{
				if (systemMenuActions[ii])
					systemMenuActions[ii]->setFont(font);
			}
		}
	}
}

void RobloxRibbonMainWindow::customizeQuickAccess()
{
	if (!m_pQuickAccessConfigDialog)
		m_pQuickAccessConfigDialog = new QuickAccessConfigDialog(this);

	// toggle visibility
	if (m_pQuickAccessConfigDialog->isVisible())
	{
		m_pQuickAccessConfigDialog->hide();
	}
	else
	{
		m_pQuickAccessConfigDialog->show();
		m_pQuickAccessConfigDialog->raise();
	}
}

void RobloxRibbonMainWindow::createThemeOptions()
{
	Qtitan::RibbonStyle::Theme themeId = Qtitan::RibbonStyle::Office2007Blue;
	if (m_pRibbonStyle)
		themeId = m_pRibbonStyle->getTheme();

	QMenu* menu = ribbonBar()->addMenu(tr("Theme Options"));
	QAction* actionStyle = menu->addAction(tr(attribStyle));

	QMenu* menuStyle = new QMenu(ribbonBar());
	QActionGroup* styleActions = new QActionGroup(this);	

	QAction* actionBlue = menuStyle->addAction(tr("Office 2007 Blue"));
    actionBlue->setObjectName("Office2007Blue");
	actionBlue->setChecked(themeId == Qtitan::RibbonStyle::Office2007Blue);
	
	QAction* actionBlack = menuStyle->addAction(tr("Office 2007 Black"));
	actionBlack->setObjectName("Office2007Black");
	actionBlack->setChecked(themeId == Qtitan::RibbonStyle::Office2007Black);

	QAction* actionSilver = menuStyle->addAction(tr("Office 2007 Silver"));
	actionSilver->setObjectName("Office2007Silver");
	actionSilver->setChecked(themeId == Qtitan::RibbonStyle::Office2007Silver);

	QAction* actionAqua = menuStyle->addAction(tr("Office 2007 Aqua"));
	actionAqua->setObjectName("Office2007Aqua");
	actionAqua->setChecked(themeId == Qtitan::RibbonStyle::Office2007Aqua);

	QAction* actionScenic = menuStyle->addAction(tr("Windows 7 Scenic"));
	actionScenic->setObjectName("Windows7Scenic");
	actionScenic->setChecked(themeId == Qtitan::RibbonStyle::Windows7Scenic);

	QAction* action2010Blue = menuStyle->addAction(tr("Office 2010 Blue"));
	action2010Blue->setObjectName("Office2010Blue");
	action2010Blue->setChecked(themeId == Qtitan::RibbonStyle::Office2010Blue);

	QAction* action2010Silver = menuStyle->addAction(tr("Office 2010 Silver"));
	action2010Silver->setObjectName("Office2010Silver");
	action2010Silver->setChecked(themeId == Qtitan::RibbonStyle::Office2010Silver);

	QAction* action2010Black = menuStyle->addAction(tr("Office 2010 Black"));
	action2010Black->setObjectName("Office2010Black");
	action2010Black->setChecked(themeId == Qtitan::RibbonStyle::Office2010Black);

	QAction* action2013White = menuStyle->addAction(tr("Office 2013 White"));
	action2013White->setObjectName("Office2013White");
	action2013White->setChecked(themeId == Qtitan::RibbonStyle::Office2013White);

	QAction* action2013Gray = menuStyle->addAction(tr("Office 2013 Gray"));
	action2013Gray->setObjectName("Office2013Gray");
	action2013Gray->setChecked(themeId == Qtitan::RibbonStyle::Office2013Gray);

	styleActions->addAction(actionBlue);
	styleActions->addAction(actionBlack);
	styleActions->addAction(actionSilver);
	styleActions->addAction(actionAqua);
	styleActions->addAction(actionScenic);
	styleActions->addAction(action2010Blue);
	styleActions->addAction(action2010Silver);
	styleActions->addAction(action2010Black);
	styleActions->addAction(action2013White);
	styleActions->addAction(action2013Gray);
	styleActions->setEnabled(true);

	QList<QAction*> styleActionsList= styleActions->actions();
	for(int i = 0 ; i < styleActionsList.size(); i++)
	{
		styleActionsList.at(i)->setCheckable(true);
	}
	
	actionStyle->setMenu(menuStyle);
	connect(styleActions, SIGNAL(triggered(QAction*)), this, SLOT(handleStyleChange(QAction*)));
}

void RobloxRibbonMainWindow::setTheme(Qtitan::RibbonStyle::Theme themeId)
{
	if (!m_pRibbonStyle)
		return;

	if (QToolButton* pButton = ribbonBar()->getSystemButton())
	{
		if (themeId == Qtitan::OfficeStyle::Windows7Scenic || 
			themeId == Qtitan::OfficeStyle::Office2010Blue ||
			themeId == Qtitan::OfficeStyle::Office2010Silver ||
			themeId == Qtitan::OfficeStyle::Office2010Black ||
			themeId == Qtitan::OfficeStyle::Office2013White ||
			themeId == Qtitan::OfficeStyle::Office2013Gray)
		{
			pButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
		}
		else
		{
			pButton->setToolButtonStyle(Qt::ToolButtonFollowStyle);
		}
	}

	m_pRibbonStyle->setTheme(themeId);	
}

QAction* RobloxRibbonMainWindow::findChildAction(const QString& actionName) const
{	return findChild<QAction*>(actionName); }

void RobloxRibbonMainWindow::populateNameValueStore(const QString& storeID, const QObject *pObject)
{
	//first clear the store
	NameValueStoreManager::singleton().clear(storeID);

	if (!pObject || !pObject->dynamicPropertyNames().count())
		return;

	//now we are ready to populate
	NameValueStoreManager::singleton().setValue(storeID, "objectName", pObject->objectName());
	//set the dynamic properties for later use
	for (int ii = 0; ii < pObject->dynamicPropertyNames().count(); ++ii) 
	{
		QString propName = pObject->dynamicPropertyNames().at(ii);
		if (!propName.isEmpty())
			NameValueStoreManager::singleton().setValue(storeID, propName, pObject->property(qPrintable(propName)));
	}	
}

QString RobloxRibbonMainWindow::getFullName(QObject* pObject)
{
	QString fullName;
	while (pObject && (pObject != this)) 
	{
		if (!fullName.isEmpty())
			fullName.append('.');
		fullName.append(pObject->objectName());
		pObject = pObject->parent();	
	}
	return fullName;
}