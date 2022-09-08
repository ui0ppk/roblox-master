/**
 * RobloxCookieJar.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */


#include "stdafx.h"
#include "RobloxCookieJar.h"

// Qt headers
#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QTimer>

// Roblox headers
#include "AuthenticationHelper.h"
#include "Util/Http.h"
#include "FastLog.h"

typedef NetworkCookieList::const_iterator CookieListIterator;
typedef CookieMap::const_iterator CookieMapIterator;
typedef NameToCookieMap::const_iterator NameToCookieMapIterator;

CookieMap RobloxCookieJar::m_cookieMap;
CookieReadStatusMap RobloxCookieJar::m_cookieReadMap;

static const int kExpirationDateStringLength = 20;

// Lazy Evaluation for faster browser loading
// Do not Load any Cookie from QSettings Persistence to Memory in ctor

RobloxCookieJar::RobloxCookieJar()
{
    QTimer::singleShot(1000, this, SLOT(lazyInitialization()));
}

// Store the Cookie from Memory into QSettings Persistence
// This has the downside that if the application crash, the settings will not be persisted
RobloxCookieJar::~RobloxCookieJar()
{
	saveCookiesToDisk();
}
void RobloxCookieJar::saveCookiesToDisk()
{
	// Do Not mix the RobloxStudio Settings with Cookies, create a separate plist file for StudioBrowser
	QSettings settings("Roblox", "RobloxStudioBrowser");

	for (CookieMapIterator outerIter = m_cookieMap.constBegin(); outerIter != m_cookieMap.constEnd(); ++outerIter) 
	{
		// Begin a Settings Group with URL
		settings.beginGroup(outerIter.key());
		NameToCookieMap nameToCookieMap = outerIter.value();
		for (NameToCookieMapIterator innerIter = nameToCookieMap.begin(); innerIter != nameToCookieMap.end(); ++innerIter)
		{
			// Construct the Value Part with following syntax:
			// SEC::<YES>,EXP::<YYYY-MM-DDTHH:MM:SS>,COOK::<COOKIE>
			QNetworkCookie cookie = innerIter.value();

			// Do not store Session Cookie
			if (!cookie.isSessionCookie())
			{
				QDateTime cookieExpTime = cookie.expirationDate().toUTC();
				if (cookieExpTime.isValid() && cookieExpTime > QDateTime::currentDateTimeUtc())
				{
					QString cookieValue;
					if(cookie.isSecure()) cookieValue+="SEC::<YES>,";
					cookieValue+="EXP::<" + cookieExpTime.toString(Qt::ISODate) + ">,";
					cookieValue+="COOK::<" + QString(cookie.value()) + ">";
					settings.setValue(innerIter.key(), cookieValue);
				}
			}
		}
		// End the Setting Group for URL
		settings.endGroup();
		settings.sync();
	}
}

bool RobloxCookieJar::setCookiesFromUrl(const NetworkCookieList& cookieList, const QUrl & url) 
{	
    if(!url.isValid())
        return false;

	QString hostName = url.host().mid(url.host().indexOf(QString(".")) + 1);

	// Lazy Eval
	loadCookiesFromDisk(hostName);
	NameToCookieMap nameToCookieMap;

	// See if we already have a map entry for the url passed in
	CookieMapIterator foundUrl = m_cookieMap.constFind(hostName);
	if(foundUrl != m_cookieMap.constEnd())
		nameToCookieMap = foundUrl.value();

	bool authenticationChanged = false;
	bool newAuthenticationValue = false;

	// Get the Cookies, Prepare the NameToCookieMap & assign it to the Main Cookie Map
	for (CookieListIterator iter = cookieList.constBegin(); iter != cookieList.constEnd(); ++iter) 
	{
		QNetworkCookie cookie = *iter;	
		// Check if the Cookie passed in has expired
		if (!cookie.isSessionCookie())
		{
			QByteArray ba = iter->name();
			QString str(ba);

			/// This will be a case when the user logs out
			QDateTime cookieExpTime = cookie.expirationDate().toUTC();
			if (!cookieExpTime.isValid() || cookieExpTime < QDateTime::currentDateTimeUtc())
			{
				// Logout the user from HTTP session
				if (str == ".ROBLOSECURITY")
				{
					authenticationChanged = true;
					newAuthenticationValue = false;
				}

				// If expired, we will need to delete the cookie from memory
				nameToCookieMap.erase(nameToCookieMap.find((*iter).name()));

				// Also delete immedietly from persistence for cookies that have expired
				QSettings settings("Roblox", "RobloxStudioBrowser");
				settings.beginGroup(hostName);
				settings.remove((*iter).name());
				settings.endGroup();
				settings.sync();
				continue;

			}
			// user logged in successfully
			else if (str == ".ROBLOSECURITY")
			{
				authenticationChanged = true;
				newAuthenticationValue = true;
			}
		}
		// If all is good then save the QNetworkCookie in the NameToCookieMap
		nameToCookieMap[(*iter).name()] = cookie;

		transferCookieToNativeHttpLayer(cookie.domain(), cookie);
	}

	// Finally assign the NameToCookieMap to our Main CookieMap
	m_cookieMap[hostName] = nameToCookieMap;
	m_cookieReadMap[hostName] = true;

	if (authenticationChanged)
	{
		authenticateHttpLayer(newAuthenticationValue);

		saveCookiesToDisk(); // Persist
	}

	return true;
}

NetworkCookieList RobloxCookieJar::cookiesForUrl(const QUrl & url) const 
{
    if(!url.isValid())
        return NetworkCookieList();

	QString hostName = url.host().mid(url.host().indexOf(QString(".")) + 1);
	// Lazy Eval
	loadCookiesFromDisk(hostName);

	NetworkCookieList cookieList;
	NameToCookieMap nameToCookieMap = m_cookieMap[hostName];

	for (NameToCookieMapIterator iter = nameToCookieMap.constBegin(); iter != nameToCookieMap.constEnd(); ++iter) 
	{
		// if found pump in the QNetworkCookie into the NetworkCookieList
		cookieList.push_back(iter.value());
	}
	m_cookieReadMap[hostName] = true;
	return cookieList;
}

// This fn is responsible for loading cookies from persistence to memory
void RobloxCookieJar::loadCookiesFromDisk(const QString &url, bool authenticate) const
{	
	/// Get the NameToCookieMap from the Main Cookie Map
	NameToCookieMap nameToCookieMap = m_cookieMap[url];

	if(m_cookieReadMap[url] != true)
	{
		// Do Not mix the RobloxStudio Settings with Cookies, create a separate plist file for StudioBrowser
		QSettings settings("Roblox", "RobloxStudioBrowser");
		QString cookieName, valueStr, cookieStr, expDateStr;

		settings.beginGroup(url);
		QStringList childKeys = settings.childKeys();
		bool foundBadFormat = false;

		for (int j = 0; j < childKeys.size(); ++j) 
		{
			cookieName = childKeys.at(j);			
			valueStr = settings.value(cookieName).toString();

			int indexExpDate = valueStr.indexOf("EXP::<");
			int indexCookie = valueStr.indexOf("COOK::<");
			if (indexExpDate == -1 || indexCookie == -1)
			{
				foundBadFormat = true;
				break;
			}
			expDateStr = indexExpDate != -1 ? valueStr.mid(indexExpDate + 6, kExpirationDateStringLength) : "";
			cookieStr = indexCookie != -1 ? valueStr.right(valueStr.length() - indexCookie - 7) : "";
			cookieStr = cookieStr.left(cookieStr.length() - 1);

			if (!expDateStr.isEmpty()) 
			{
				QDateTime cookieExpTime = QDateTime::fromString(expDateStr, Qt::ISODate);
				if (cookieExpTime > QDateTime::currentDateTimeUtc()) 
				{
					QNetworkCookie cookie(childKeys.at(j).toLocal8Bit(), cookieStr.toLocal8Bit());
					cookie.setExpirationDate(cookieExpTime);
					nameToCookieMap[childKeys.at(j)] = cookie;

					// User is logged in from previous Roblox Studio session
                    if (cookieName == ".ROBLOSECURITY" && authenticate)
						authenticateHttpLayer(true);

					transferCookieToNativeHttpLayer(url, cookie);
				}
			}
		}
		settings.endGroup();
		if (foundBadFormat)
		{
			settings.clear();
		}

		m_cookieMap[url] = nameToCookieMap;
		m_cookieReadMap[url] = true;
	}	
}

void RobloxCookieJar::transferCookieToNativeHttpLayer(const QString& domain, const QNetworkCookie& cookie)
{
	RBX::Http::setCookiesForDomain(domain.toStdString(),
		QString("%1=%2").arg(QString(cookie.name())).arg(QString(cookie.value())).toStdString());
}

/// Establish the channel for authentication by calling requestAuth
void RobloxCookieJar::authenticateHttpLayer(bool isAuthenticated) const
{
	AuthenticationHelper::Instance().authenticateHttpSessionAsync(isAuthenticated);
}

void RobloxCookieJar::lazyInitialization()
{
	connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(saveCookiesToDisk()));
}

void RobloxCookieJar::reloadCookies(bool authenticate)
{
	for (CookieReadStatusMap::iterator cookiesReadMapIter = m_cookieReadMap.begin(); cookiesReadMapIter != m_cookieReadMap.end(); ++cookiesReadMapIter) 
	{
		//just set the read status to false to reloaded again from disk
		m_cookieReadMap[cookiesReadMapIter.key()] = false;
		//reload cookies for the URL (we do not want any authentication while loading cookies)
		loadCookiesFromDisk(cookiesReadMapIter.key(), authenticate);
	}
}

QString RobloxCookieJar::getCookieValue(const QString &hostURL, const QString& cookieKey)
{	
	QUrl url(hostURL);
	if (!url.isValid())
		return QString();

	QString result;
	QString hostName = url.host().mid(url.host().indexOf(QString(".")) + 1);

	QSettings settings("Roblox", "RobloxStudioBrowser");
	settings.beginGroup(hostName);
	QStringList childKeys = settings.childKeys();

	for (int j = 0; j < childKeys.size(); ++j) 
	{
		if (childKeys.at(j) == cookieKey)
		{
			QString valueStr = settings.value(cookieKey).toString();

			int indexExpDate = valueStr.indexOf("EXP::<");
			QString expDateStr = indexExpDate != -1 ? valueStr.mid(indexExpDate + 6, kExpirationDateStringLength) : "";

			int indexCookie = valueStr.indexOf("COOK::<");
			QString cookieStr = indexCookie != -1 ? valueStr.right(valueStr.length() - indexCookie - 7) : "";
			cookieStr = cookieStr.left(cookieStr.length() - 1);

			if (!cookieStr.isEmpty() && !expDateStr.isEmpty())
			{
				QDateTime cookieExpTime = QDateTime::fromString(expDateStr, Qt::ISODate);
				if (cookieExpTime > QDateTime::currentDateTimeUtc()) 
					result = cookieStr;
			}
			else
			{
				result = cookieStr;
			}

			break;
		}
	}

	 return result;
}
