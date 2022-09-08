/**
 * RobloxWebDoc.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxWebDoc.h"

// Qt Headers
#include <QToolBar>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDesktopServices>
#include <QGridLayout>
#include <QWebSettings>
#include <QWebFrame>
#include <QNetworkReply>
#include <QDebug>
#include <QSslError>

// Roblox Headers
#include "util/Statistics.h"
#include "util/Http.h"
#include "v8datamodel/FastLogSettings.h"

// Roblox Studio Headers
#include "RbxWorkspace.h"
#include "RobloxBrowser.h"
#include "RobloxMainWindow.h"
#include "RobloxWebPage.h"
#include "UpdateUIManager.h"
#include "QtUtilities.h"
#include "AuthenticationHelper.h"
#include "RobloxCookieJar.h"
#include "RobloxDocManager.h"

FASTFLAG(StudioInSyncWebKitAuthentication)
FASTFLAG(WebkitLocalStorageEnabled);
FASTFLAG(WebkitDeveloperToolsEnabled);
FASTFLAG(StudioEnableWebKitPlugins);

RobloxWebDoc::RobloxWebDoc(const QString& displayName, const QString& keyName)
: m_pWebView(NULL)
, m_pWrapperWidget(NULL)
, m_pWorkspace(new RbxWorkspace(this, NULL))
, m_pAddrInputComboBox(NULL)
, m_displayName(displayName)
, m_currentUrl("")
, m_homeUrl("")
, m_keyName(keyName)
{
}

RobloxWebDoc::~RobloxWebDoc()
{
	if (m_pWrapperWidget)
		m_pWrapperWidget->deleteLater();
}

bool RobloxWebDoc::open(RobloxMainWindow *pMainWindow, const QString& url)
{
	bool success = true;
	
	try
	{
		if(!m_pWrapperWidget)
		{
			//create parent widget
			m_pWrapperWidget = new QWidget(pMainWindow);
			m_homeUrl = url;

			//create required widgets for the web view
			setupWebView(m_pWrapperWidget);
			
			//setup layout
			QGridLayout *layout = new QGridLayout(m_pWrapperWidget);
			layout->setContentsMargins(0, 0, 0, 0);
			layout->setSpacing(0);
			
			if (RBX::ClientAppSettings::singleton().GetValueWebDocAddressBarEnabled())
			{
				QToolBar *addrToolBar = setupAddressToolBar(m_pWrapperWidget);
			    layout->addWidget(addrToolBar, 0, 0);
			}

			layout->addWidget(m_pWebView, 1, 0);
			m_pWrapperWidget->setLayout(layout);
		}

		navigateUrl(url);
	}
	
	catch(...)
	{
		success =  false;
	}
	
	return success;
}

bool RobloxWebDoc::doClose()
{
	return true;
}

QWidget* RobloxWebDoc::getViewer() 
{ 
	return m_pWrapperWidget; 
}

void RobloxWebDoc::activate()
{
	if (m_bActive)
		return;

	//update toobars
	UpdateUIManager::Instance().updateToolBars();

	m_bActive = true;
}

bool RobloxWebDoc::actionState(const QString &actionID, bool &enableState, bool &checkedState)
{
	static QString webActiveActions("fileCloseAction");
	if (webActiveActions.contains(actionID))
	{
		enableState = true;
	}
	else if (UpdateUIManager::Instance().getDockActionNames().contains(actionID))
	{
		enableState = true;
		checkedState = UpdateUIManager::Instance().getAction(actionID)->isChecked();
	}
	else if (actionID == "axisWidgetAction" || actionID == "toggleGridAction")
	{
		enableState = false;
		checkedState = UpdateUIManager::Instance().getAction(actionID)->isChecked();
	}
	else
	{
		enableState = false;
		checkedState = false;
	}

	return true;	
}

void RobloxWebDoc::setupWebView(QWidget *wrapperWidget)
{
	m_pWebView = new RobloxBrowser(wrapperWidget);
	m_pWebView->setPage(new RobloxWebPage(wrapperWidget));

	// Reload page if authentication changes, required for webpages other than start page
	if (FFlag::StudioInSyncWebKitAuthentication && m_keyName != "StartPage")
		connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
	
	connect(m_pWebView->page()->networkAccessManager(),
			SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )),
			this,
			SLOT(sslErrorHandler(QNetworkReply*, const QList<QSslError> & )));
	
	QWebSettings *globalSetting = QWebSettings::globalSettings();
	
	globalSetting->setAttribute(QWebSettings::AutoLoadImages, true);
	
	globalSetting->setAttribute(QWebSettings::JavascriptEnabled, true);
	globalSetting->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
	globalSetting->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
	
#ifdef _WIN32
    if (FFlag::StudioEnableWebKitPlugins)
        globalSetting->setAttribute(QWebSettings::PluginsEnabled, true);
    else
        globalSetting->setAttribute(QWebSettings::PluginsEnabled, false);
#endif
	
	/// Keep all this for now, later on we should remove it depending on bare minimum required.
	globalSetting->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
	globalSetting->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);

	if(FFlag::WebkitLocalStorageEnabled)
		globalSetting->setAttribute(QWebSettings::LocalStorageEnabled, true);

	if(FFlag::WebkitDeveloperToolsEnabled)
		globalSetting->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	
	connect(m_pWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(initJavascript()));

	// update title only for StartPage
	if (m_keyName == "StartPage")
		connect(m_pWebView, SIGNAL(titleChanged(QString)), SLOT(updateTitle(QString)));
}

void RobloxWebDoc::handleHomeClicked()
{
	navigateUrl(m_homeUrl);
}

QToolBar* RobloxWebDoc::setupAddressToolBar(QWidget *wrapperWidget)
{
	QToolBar *pToolBar = new QToolBar(wrapperWidget);
	
	//set the layout for the address toolbar.
	QLabel *pLabel = new QLabel(pToolBar);
	pLabel->setText(QString("Address"));
	pLabel->setMinimumSize(10, 0);

	m_pAddrInputComboBox = new QComboBox(pToolBar);
	m_pAddrInputComboBox->setEditable(true);
	m_pAddrInputComboBox->setSizePolicy(QLineEdit().sizePolicy());

	//QT has implementations for the back, forward, stop, and reload actions already.
	QAction* pAction = m_pWebView->pageAction(QWebPage::Back);
	pAction->setStatusTip("Go Back");
	QtUtilities::setActionShortcuts(*pAction,QKeySequence::keyBindings(QKeySequence::Back));
	pAction->setShortcutContext(Qt::WidgetShortcut);
	pToolBar->addAction(pAction);

	pAction = m_pWebView->pageAction(QWebPage::Forward);
	pAction->setStatusTip("Go Forward");
	QtUtilities::setActionShortcuts(*pAction,QKeySequence::keyBindings(QKeySequence::Forward));
	pAction->setShortcutContext(Qt::WidgetShortcut);
	pToolBar->addAction(pAction);

	pAction = m_pWebView->pageAction(QWebPage::Stop);
	pAction->setStatusTip("Stop");
	pToolBar->addAction(pAction);

	pAction = m_pWebView->pageAction(QWebPage::Reload);
	pAction->setStatusTip("Reload");
    QtUtilities::setActionShortcuts(*pAction,QKeySequence::keyBindings(QKeySequence::Refresh));
	pAction->setShortcutContext(Qt::WidgetShortcut);
	pToolBar->addAction(pAction);

	pAction = new QAction("Home", pToolBar);
	pAction->setIcon(QIcon(":/images/home_button.png"));
	pAction->setToolTip("Home");
	pAction->setStatusTip("Home");
	pAction->setObjectName("actionNavigateHome");

    QList<QKeySequence> sequences;
#ifdef Q_WS_WIN32
    sequences.append(QKeySequence("Alt+Home"));
#else
    sequences.append(QKeySequence("Ctrl+Home"));
#endif
    QtUtilities::setActionShortcuts(*pAction,sequences);
	pAction->setShortcutContext(Qt::WidgetShortcut);
	pToolBar->addAction(pAction);
	connect(pAction, SIGNAL(triggered()), this, SLOT(handleHomeClicked()));

	pToolBar->addWidget(pLabel);
	pToolBar->addWidget(m_pAddrInputComboBox);
	
	pToolBar->setIconSize(QSize(16, 16));

	//setup the connection for address toolbar. (home/ comboBox edit line)
	connect(m_pAddrInputComboBox, SIGNAL(activated(const QString& )), this, SLOT(navigateUrl(const QString& )));
	connect(m_pWebView, SIGNAL(urlChanged(const QUrl&)), this, SLOT(updateAddressBar(const QUrl&)));
	
	return pToolBar;
}

void RobloxWebDoc::navigateUrl(const QString& urlString)
{
	if (urlString.isEmpty()) 
		return;
	
	QString urlStringMod(urlString);

	//append http if it's not there already
	if (!urlStringMod.contains("://"))
		urlStringMod.prepend("http://");

	QByteArray ba = urlStringMod.toAscii();
	const char *c_str = ba.data();

	if (RBX::Http::trustCheckBrowser(c_str))
	{
		m_pWebView->load(urlStringMod);
	}
	else 
	{
		QDesktopServices::openUrl(urlStringMod);
		urlStringMod = "";
	}

	updateAddressBar(QUrl(urlStringMod));
}

void RobloxWebDoc::updateAddressBar(const QUrl& url)
{ 
	QString urlStr = url.toString();
	if (m_currentUrl == urlStr)
		return;

	m_currentUrl = urlStr;

	if (m_pAddrInputComboBox)
		m_pAddrInputComboBox->setEditText(m_currentUrl);
}

void RobloxWebDoc::initJavascript()
{
	if (m_pWebView->page() && m_pWebView->page()->mainFrame())
	{
		// remove all slots connected to workspace before adding it again
		// or else we can have multiple slots getting called from web page, resulting in dangling function calls.
		m_pWorkspace->disconnect();
		m_pWebView->page()->mainFrame()->addToJavaScriptWindowObject("external", m_pWorkspace.get() );
	}
}

void RobloxWebDoc::refreshPage()
{
	if (m_pWebView)
	{
		m_pWebView->reload();
	}
}

void RobloxWebDoc::sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & errlist)
{
#ifdef _DEBUG
  qDebug() << "---RobloxWebDoc::sslErrorHandler: ";
  // show list of all ssl errors
  Q_FOREACH (QSslError err,errlist)
    qDebug() << "ssl error: " << err;
#endif
 
   qnr->ignoreSslErrors();
}

void RobloxWebDoc::onAuthenticationChanged(bool)
{
	//make sure reload action is enabled (to avoid circular loop)
	QAction* pReloadAction = m_pWebView->page()->action(QWebPage::Reload);
	if (pReloadAction && pReloadAction->isEnabled())
		QTimer::singleShot(0, pReloadAction, SLOT(trigger()));
}

void RobloxWebDoc::updateTitle(QString title)
{
	if (!title.isEmpty())
	{
		const int maxTitleWidth = 130;
		m_displayName = title;
		RobloxDocManager::Instance().setDocTitle(*this, qApp->fontMetrics().elidedText(title, Qt::ElideRight, maxTitleWidth), title);
	}
}