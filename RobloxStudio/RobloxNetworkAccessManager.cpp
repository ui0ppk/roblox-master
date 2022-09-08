/**
 * RobloxNetworkAccessManager.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxNetworkAccessManager.h"

// Roblox Headers
#include "QtUtilities.h"
#include "RobloxCookieJar.h"
#include "RobloxWebPage.h"
#include "RobloxSettings.h"
#include "RobloxNetworkReply.h"

RobloxNetworkAccessManager::RobloxNetworkAccessManager()
: QNetworkAccessManager()
{
	// When we enable the Cookie Persistence, logging into mail.roblox.com stops working, but works fine for www.roblox.com with persistence
	setCookieJar(new RobloxCookieJar());
}
void RobloxNetworkAccessManager::initUserAgent()
{
	// Set up our useragent
	RobloxWebPage dummy;
	userAgent = dummy.getDefaultUserAgent();
	
	// Only on Mac webkit does not provide the OSX version, add it, will do only for Mac
	QString macOSXVersion = QtUtilities::getMacOSXVersion();
	userAgent = userAgent.replace("Mac OS X", macOSXVersion);
}
QString RobloxNetworkAccessManager::getUserAgent()
{
	if (userAgent.isEmpty())
		initUserAgent();
	return userAgent;
}

RobloxNetworkReply* RobloxNetworkAccessManager::get(const QNetworkRequest& request, bool followRedirection /*= true*/ )
{
	QNetworkReply* reply = QNetworkAccessManager::get(request);
	RobloxNetworkReply* robloxreply = new RobloxNetworkReply(reply, followRedirection);
	return robloxreply;
}

RobloxCookieJar* RobloxNetworkAccessManager::cookieJar() const
{
	return dynamic_cast<RobloxCookieJar*>(QNetworkAccessManager::cookieJar());
}
