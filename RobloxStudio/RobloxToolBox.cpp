/**
 * RobloxToolBox.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxToolBox.h"

// Qt Headers
#include <QGridLayout>
#include <QWebElement>
#include <QWebFrame>

// Roblox Headers
#include "v8datamodel/DataModel.h"
#include "v8datamodel/InsertService.h"
#include "v8datamodel/ContentProvider.h"
#include "util/standardout.h"
#include "network/Players.h"

// Roblox Studio Headers
#include "AuthenticationHelper.h"
#include "RbxWorkspace.h"
#include "RobloxCookieJar.h"
#include "RobloxNetworkAccessManager.h"
#include "RobloxSettings.h"

FASTFLAG(WebkitLocalStorageEnabled);
FASTFLAG(WebkitDeveloperToolsEnabled);
FASTFLAG(StudioEnableWebKitPlugins);

RobloxToolBox::RobloxToolBox()
: m_pWorkspace()
, m_pWebView(NULL)
, reloadView(false)
{
	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	
    setMaximumWidth(250);

	setupWebView(this);
	layout->addWidget(m_pWebView, 0, 0);
	setLayout(dynamic_cast<QLayout*>(layout));

    setMaximumWidth(QWIDGETSIZE_MAX);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	
	setMinimumWidth(285);
}

void RobloxToolBox::setupWebView(QWidget *wrapperWidget)
{
	m_pWebView = new RobloxBrowser(wrapperWidget);
	m_pWebPage = new RobloxWebPage(wrapperWidget);
	
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

	connect(m_pWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(initJavascript())); 

	m_urlString = QString("%1/IDE/ClientToolbox.aspx").arg(RobloxSettings::getBaseURL());

	connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
}


void RobloxToolBox::setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel)
{
	if(m_pDataModel == pDataModel)
		return;

    bool firstTime = false;
    m_pDataModel = pDataModel;

    if (!m_pWorkspace)
    {
        m_pWorkspace.reset(new RbxWorkspace(this, m_pDataModel ? m_pDataModel.get() : NULL));
        firstTime = true;
    }
    else
    {
        m_pWorkspace->setDataModel(pDataModel.get());
    }

    if (!m_pDataModel)
	{
		setEnabled(false);
		return;
	}

	setEnabled(true);

    if (firstTime)
	{
        m_pWebView->load(m_urlString);
	}
	else if (reloadView)
	{
		m_pWebPage->triggerAction(QWebPage::Reload);
	}
	reloadView = false;
    
	update();
}

void RobloxToolBox::initJavascript()
{
	if(m_pWorkspace && m_pWebView->page() && m_pWebView->page()->mainFrame())
	{
		m_pWebView->page()->mainFrame()->addToJavaScriptWindowObject("external", m_pWorkspace.get() );
	}
}

QString RobloxToolBox::getTitleFromUrl(const QString &urlString)
{	
	if(m_pWebView && m_pWebView->page() && m_pWebView->page()->mainFrame())
	{
		int pos = urlString.indexOf("id=");
		if (pos > 0 && pos+3 < urlString.size())
		{
			QWebElementCollection toolboxItemElements = m_pWebView->page()->mainFrame()->findAllElements(QString("span#span_setitem_%1 a").arg(urlString.mid(pos+3)));
			Q_FOREACH(QWebElement toolboxItemElement, toolboxItemElements)
			{
				QStringList attributesList = toolboxItemElement.attributeNames();
				Q_FOREACH(QString attributeName, attributesList)
				{
					if (attributeName == "title")
						return toolboxItemElement.attribute(attributeName);
				}
			}
		}
	}
	
	return QString("");
}

void RobloxToolBox::onAuthenticationChanged(bool)
{
	if (m_pDataModel)
		m_pWebPage->triggerAction(QWebPage::Reload);
	else
		reloadView = true;
}

void RobloxToolBox::loadUrl(const QString url)
{
	setEnabled(true);
	m_urlString = url;
	m_pWebView->load(url);
}