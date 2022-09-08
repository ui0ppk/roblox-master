/**
 * RobloxWebPage.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxWebPage.h"

// Qt Headers
#include <QDesktopServices>
#include <QNetworkReply>
#include <QWebFrame>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QWebHitTestResult>

// Roblox Headers
#include "AuthenticationHelper.h"
#include "FastLog.h"
#include "RobloxBrowser.h"
#include "RobloxNetworkAccessManager.h"
#include "RobloxSettings.h"
#include "QtUtilities.h"
#include "Util/RobloxGoogleAnalytics.h"

#include <QWebElement>

QString RobloxWebPage::userAgentForUrl(const QUrl &url) const 
{	
	return RobloxNetworkAccessManager::Instance().getUserAgent();
}

RobloxWebPage::RobloxWebPage(QWidget* parent) 
    : QWebPage(parent)
{
	setNetworkAccessManager(&RobloxNetworkAccessManager::Instance());
	connect(&RobloxNetworkAccessManager::Instance(), SIGNAL(finished(QNetworkReply*)), this, SLOT(handleFinished(QNetworkReply*)));
}

void RobloxWebPage::handleFinished(QNetworkReply* reply)
{
	QString status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
	if (status.toInt() != 200 || !reply->hasRawHeader("refresh"))
		return;

	QString acceptHeader = reply->request().rawHeader("accept").data();
	if (!acceptHeader.startsWith("application"))
		return;

	QString url = QString(reply->rawHeader("refresh").data());
	this->mainFrame()->load(url.mid(url.indexOf("=")+1));
}

bool RobloxWebPage::acceptNavigationRequest ( QWebFrame * frame, const QNetworkRequest & request, NavigationType type )
{
    RBX::RobloxGoogleAnalytics::trackEvent("Web", "URLRequest", request.url().toString().toStdString().c_str());

	if (!frame) // This is a new window request, open in default browser
	{
		QDesktopServices::openUrl(request.url());
		return true;
	}
	else
		return QWebPage::acceptNavigationRequest(frame, request, type);
}

QString RobloxWebPage::getDefaultUserAgent() const
{
	return QWebPage::userAgentForUrl(QUrl(""));
}

// If we have an override for the file upload element on this page, use that instead 
// This allows us to inject files into file input tags <input type="file" /> on the page
QString RobloxWebPage::chooseFile( QWebFrame *originatingFrame, const QString& oldFile )
{
	if (m_overideUploadFile.isEmpty())
		return QWebPage::chooseFile(originatingFrame, oldFile);
	else
		return m_overideUploadFile;
}

void RobloxWebPage::setUploadFile(QString selector, QString fileName)
{
	// Store for uploading
	m_overideUploadFile = fileName; 
	
	// Invoke an "file selection" behind the scenes.  This will automatically put the file into
	// the pages file input tag.
	QWebElement button = this->mainFrame()->findFirstElement(selector);
	button.setFocus();
	QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
	QCoreApplication::postEvent (this->view(), event);
}

void RobloxWebPage::triggerAction(QWebPage::WebAction action, bool checked)
{
	if ((action == QWebPage::OpenLinkInNewWindow) && mainFrame() && !m_contextPos.isNull())
	{
		QWebHitTestResult hitTestResult = mainFrame()->hitTestContent(m_contextPos);
		if (!hitTestResult.isNull())
		{
			QUrl url = hitTestResult.linkUrl();
			if (url.isValid() && !url.host().isEmpty())
				QDesktopServices::openUrl(url);
		}
		m_contextPos = QPoint();
	}
	else
	{
		QWebPage::triggerAction(action, checked);
	}
}

bool RobloxWebPage::event(QEvent *evt)
{
	if (evt->type() == QEvent::ContextMenu)
       m_contextPos = static_cast<QContextMenuEvent*>(evt)->pos();

	return QWebPage::event(evt);
}