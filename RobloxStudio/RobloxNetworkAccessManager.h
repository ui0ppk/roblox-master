/**
 * RobloxNetworkAccessManager.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QNetworkAccessManager>

class RobloxNetworkReply;
class RobloxCookieJar;

class RobloxNetworkAccessManager : public QNetworkAccessManager
{
public:
	static RobloxNetworkAccessManager& Instance()
	{
		// TODO: RobloxMainWindow should be the parent of this, but it's not a singleton or QObject
		// We instead set the parent during initialization.  Would be better to pass into the constructor
		static RobloxNetworkAccessManager singleInstance;
		return singleInstance;
	}

	QString getUserAgent();
	RobloxNetworkReply* get(const QNetworkRequest& request, bool followRedirection = true);
	RobloxCookieJar* cookieJar() const;

private:
	RobloxNetworkAccessManager();
	void initUserAgent();

	QString userAgent;
};