/**
 * RobloxContextualHelp.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"

#include "RobloxContextualHelp.h"

#include <QGridLayout>
#include <QString>
#include <QPainter>
#include <QTimer>
#include <QWebElement>
#include <QWebFrame>
#include <QWebPage>

#include "util/standardout.h"

#include "AuthenticationHelper.h"
#include "RobloxCookieJar.h"
#include "RobloxNetworkAccessManager.h"
#include "RobloxSettings.h"

FASTFLAG(WebkitLocalStorageEnabled)
FASTFLAG(WebkitDeveloperToolsEnabled)

FASTFLAGVARIABLE(StudioNewWiki, false)
FASTFLAGVARIABLE(StudioEnableWebKitPlugins, false)

RobloxContextualHelpService::RobloxContextualHelpService()
    : m_helpTopic("Studio")
{
}

RobloxContextualHelpService& RobloxContextualHelpService::singleton()
{
    static RobloxContextualHelpService* helpService = new RobloxContextualHelpService();
    return *helpService;
}

void RobloxContextualHelpService::onHelpTopicChanged(const QString& helpTopic)
{
    if (m_helpTopic != helpTopic)
    {
        m_helpTopic = helpTopic;
        Q_EMIT(helpTopicChanged(helpTopic));
    }
}

RobloxContextualHelp::RobloxContextualHelp()
    : m_pWebView(NULL)
    , m_urlDirty(false)
{
	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	
	setupWebView();
	layout->addWidget(m_pWebView, 0, 0);
	setLayout(dynamic_cast<QLayout*>(layout));
    
    connect(&RobloxContextualHelpService::singleton(), SIGNAL(helpTopicChanged(const QString&)), this, SLOT(onHelpTopicChanged(const QString&)));
}

void RobloxContextualHelp::setupWebView()
{
	m_pWebView = new RobloxHelpWebView(this);
	m_pWebPage = new RobloxWebPage(this);
	
	m_pWebView->setPage(m_pWebPage);

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
    
    connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
    
	if(FFlag::StudioNewWiki)
	{
		m_pWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
		connect(m_pWebView->page(), SIGNAL(linkClicked(const QUrl&)), this, SLOT(linkClicked(const QUrl&)));
	}
	m_urlString = QString("http://wiki.roblox.com/index.php/Studio");

	if (FFlag::StudioNewWiki)
	{
		m_urlString += "?studiomode=true";
	}
    
    m_urlDirty = true;
    
    setEnabled(true);
	update();
}

void RobloxContextualHelp::linkClicked(const QUrl& url)
{
	QUrl studioModeUrl = url;
	studioModeUrl.addQueryItem("studiomode", "true");
	m_pWebView->load(studioModeUrl);
}

void RobloxContextualHelp::onAuthenticationChanged(bool)
{
	m_pWebPage->triggerAction(QWebPage::Reload);
}

void RobloxContextualHelp::onHelpTopicChanged(const QString& helpTopic)
{
	QString prevURLString(m_urlString);
    m_urlString = QString("http://wiki.roblox.com/index.php/") + helpTopic;
 
    // url with anchor tag and query - http://wiki.roblox.com/index.php/Script_Analysis?studiomode=true#W003

	if (FFlag::StudioNewWiki)
	{
		int index = m_urlString.indexOf('#');
		index > 0 ? m_urlString.insert(index, "?studiomode=true") : (m_urlString += "?studiomode=true");
    }

	// if URL has only anchor tag change then do not reset timer as 'loadFinished' signal does not get emitted to remove the watermark
	// instead directly load URL
	if (prevURLString.mid(0, prevURLString.indexOf('#')) != m_urlString.mid(0, m_urlString.indexOf('#')))
	{
		m_pWebView->resetLoadingTimer();
		m_urlDirty = true;
	}
	else
	{
		m_urlDirty = true;
		updateURL();
	}

	update();
}

void RobloxContextualHelp::updateURL()
{
    if (m_urlDirty)
    {
        m_pWebView->load(m_urlString);
        m_urlDirty = false;
    }
}

RobloxHelpWebView::RobloxHelpWebView(RobloxContextualHelp* widget)
: RobloxBrowser(widget)
, m_contextualHelpWidget(widget)
{
    connect(this, SIGNAL(loadProgress(int)), this, SLOT(loadProgress(int)));
}

void RobloxHelpWebView::loadProgress(int)
{
    // Run some special code to hide the sidebars and turn off the background.
    // This is temporary until the wiki has Studio friendly view.
    ((QWebView*)sender())->page()->mainFrame()->evaluateJavaScript("content = document.getElementById('column-content'); content.style.float = 'none'; content.style.margin = '-3em 0 0 -12.2em'; content.style.width = 'auto'; columnOne = document.getElementById('column-one'); columnOne.style.display = 'none'; document.body.style.background = 'white';");
    update();
}


void RobloxHelpWebView::paintEvent(QPaintEvent* event)
{
    m_contextualHelpWidget->updateURL();
    
    RobloxBrowser::paintEvent(event);
}


