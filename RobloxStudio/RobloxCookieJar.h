/**
 * RobloxCookieJar.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QNetworkCookie>
#include <QNetworkCookieJar>


typedef QList<QNetworkCookie> NetworkCookieList;
typedef QMap<QString, QNetworkCookie> NameToCookieMap;
typedef QMap<QString, NameToCookieMap> CookieMap;
typedef QMap<QString, bool> CookieReadStatusMap;

class RobloxCookieJar : public QNetworkCookieJar
{
	Q_OBJECT
public:
	RobloxCookieJar();
	virtual ~RobloxCookieJar();
	/*override*/bool setCookiesFromUrl(const NetworkCookieList& cookieList, const QUrl & url);
	/*override*/NetworkCookieList cookiesForUrl (const QUrl & url) const;

	void reloadCookies(bool authenticate);
	QString getCookieValue(const QString &url, const QString& cookieKey);

public Q_SLOTS:
	void saveCookiesToDisk();
    void lazyInitialization();

private:
	static void transferCookieToNativeHttpLayer(const QString& domain, const QNetworkCookie& cookie);
	void loadCookiesFromDisk(const QString &url, bool authenticate = true) const;
	void authenticateHttpLayer(bool isAuthenticated) const;

	static CookieMap m_cookieMap;
	static CookieReadStatusMap m_cookieReadMap;
};
