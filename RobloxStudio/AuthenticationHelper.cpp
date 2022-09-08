/**
 * AuthenticationHelper.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "AuthenticationHelper.h"

// Qt Headers
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QTimer>
#include <QCoreApplication>
#include <QTime>
#include <QtConcurrentRun>

// Roblox Headers
#include "util/standardout.h"
#include "util/Http.h"
#include "util/Statistics.h"
#include "RobloxServicesTools.h"

// Roblox Studio Headers
#include "RobloxNetworkAccessManager.h"
#include "RobloxSettings.h"
#include "QtUtilities.h"
#include "UpdateUIManager.h"
#include "RobloxNetworkReply.h"
#include "RobloxMainWindow.h"
#include "RobloxCookieJar.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "Util/MachineIdUploader.h"
#include "V8DataModel/ContentProvider.h"
#include "RobloxWebDoc.h"
#include "RobloxDocManager.h"

#include "RobloxServicesTools.h"

FASTFLAG(GoogleAnalyticsTrackingEnabled)
FASTFLAGVARIABLE(StudioInSyncWebKitAuthentication, false)
FASTFLAGVARIABLE(UseAssetGameSubdomainForGetCurrentUser, true)
FASTFLAG(UseBuildGenericGameUrl)


//////////////////////////////////////////////////////////////////////////////////////////////////////
// AuthenticationHelper
QString AuthenticationHelper::userAgentStr = "";
bool AuthenticationHelper::httpAuthenticationPending = false;
bool AuthenticationHelper::verifyUserAuthenticationPending = false;
bool AuthenticationHelper::previouslyAuthenticated = false;

QString AuthenticationHelper::getLoggedInUserUrl()
{
    QString result;
    if (FFlag::UseBuildGenericGameUrl) {
        result = QString::fromStdString(BuildGenericGameUrl(RobloxSettings::getBaseURL().toStdString(), "game/GetCurrentUser.ashx"));
    }
    else
    {
        result = RobloxSettings::getBaseURL() + "/game/GetCurrentUser.ashx";
    }
    
    if (FFlag::UseAssetGameSubdomainForGetCurrentUser)
	{
		return QString::fromStdString(
			ReplaceTopSubdomain(result.toStdString(), "assetgame"));
	}
	return result;
}

bool AuthenticationHelper::verifyUserAndAuthenticate(int timeOutTime)
{
	if (verifyUserAuthenticationPending || httpAuthenticationPending)
		return false;

	verifyUserAuthenticationPending = true;
	bool result = true;	
	try
	{
		QString	requestUserUrl = getLoggedInUserUrl();
		QUrl url(requestUserUrl);

		QNetworkRequest networkRequest(url);
		networkRequest.setRawHeader("User-Agent", userAgentStr.toAscii()); 
		RobloxNetworkReply* networkReply = RobloxNetworkAccessManager::Instance().get(networkRequest);
		
		if (!networkReply->waitForFinished(timeOutTime, 100))
		{
				verifyUserAuthenticationPending = false;
			networkReply->deleteLater();
				RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Unable to get current user. Request timed out.");
				networkReply->deleteLater();
				RobloxMainWindow::sendCounterEvent("StudioAuthenticationFailure", false);
                if (FFlag::GoogleAnalyticsTrackingEnabled)
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioAuthenticationFailure");
				return false;
			}

		// Get the logged in user Webkit
		QString userIdWebkit(networkReply->readAll());
		int statusCode = networkReply->error();
		networkReply->deleteLater();

        switch (statusCode)
        {
            case QNetworkReply::NoError:
            {
                if(userIdWebkit.toInt() > 0)
                {
                    // Get the logged in user HTTP
					QString currentUser = getLoggedInUserUrl();
                    QString userIdHttp = doHttpGetRequest(currentUser);

                    // If different authenticate the HTTP layer with the webkit user
                    if(userIdHttp != userIdWebkit)
                        result = authenticateHttpLayer();
                }
                else
                {
                    // Sync with HTTP layer: Curl will be using the saved cookie file if any, to login!
					result = authenticateWebKitFromHttp();
					if (!result)
					{
						// Error (probably user not authenticated) time to logout the user from Http
						deAuthenticateHttpLayer();
						result = false;
					}
                }

                break;
            }
            default:
            {
                // Error (probably user not authenticated) time to logout the user from Http
                deAuthenticateHttpLayer();
                result = false;
                break;
            }
        }
	}
	catch (std::exception const& e) 
	{
		result = false;
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());

		RobloxMainWindow::sendCounterEvent("StudioAuthenticationFailure", false);

        if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioAuthenticationFailure");
		}
	}

	verifyUserAuthenticationPending = false;
	return result;
}


int AuthenticationHelper::getHttpUserId()
{
	QString currentUser = getLoggedInUserUrl();
	QString userIdHttp = doHttpGetRequest(currentUser);
	
	bool ok;
	int result = userIdHttp.toInt(&ok, 10);
	if (!ok)
	{
		return 0;
	}
	return result;
}

/*
* Gets user-id of the user logged into webkit layer
* @Returns: user-id, 0 if no user is logged in
*/
int AuthenticationHelper::getWebKitUserId()
{
	QString	requestUserUrl = getLoggedInUserUrl();
	QUrl url(requestUserUrl);

	int userId = 0;

	QNetworkRequest networkRequest(url);
	networkRequest.setRawHeader("User-Agent", userAgentStr.toAscii()); 
	RobloxNetworkReply* networkReply = RobloxNetworkAccessManager::Instance().get(networkRequest);
	if (networkReply && networkReply->waitForFinished(5000, 100) && (networkReply->error() == QNetworkReply::NoError))
	{
		QString userIdWebkit(networkReply->readAll());
		userId = userIdWebkit.toInt();
		networkReply->deleteLater();
	}
	return userId;
}

bool AuthenticationHelper::authenticateHttpLayer()
{	
	Q_EMIT authenticationChanged(true);
	Q_EMIT authenticationDone(true);
	return true;
}

void AuthenticationHelper::deAuthenticateHttpLayer()
{	
    QString logoutUrl;
    if (FFlag::UseBuildGenericGameUrl)
    {
        logoutUrl = QString::fromStdString(BuildGenericGameUrl(RobloxSettings::getBaseURL().toStdString(),"game/logout.aspx"));
    }
    else
    {
        logoutUrl = RobloxSettings::getBaseURL() + "/game/logout.aspx";
    }
    //
	doHttpGetRequest(logoutUrl);

	Q_EMIT authenticationChanged(false);
}


QString AuthenticationHelper::generateAuthenticationUrl(const QString& authUrl, const QString& ticket)
{	
	QString compoundStr(authUrl);
	if (!ticket.isEmpty()) 
	{
		compoundStr.append("?suggest=");
		compoundStr.append(ticket);
	}
	return compoundStr;
}

int AuthenticationHelper::authenticateWebKitFromUrlWithTicket(const QString& url, const QString& ticket)
{
	if(!url.isEmpty() && !ticket.isEmpty())
	{
		return doWebKitAuthentication(generateAuthenticationUrl(url, ticket));
	}

	return 0;
}

/*
* Authenticate the webkit layer by making a request to this url.  Follows redirects recursively.
* @Returns: status code of response
*/
int AuthenticationHelper::doWebKitAuthentication(const QString& url)
{
	QNetworkRequest networkRequest(url);
	networkRequest.setRawHeader("RBXAuthenticationNegotiation:", QByteArray(GetBaseURL().c_str(), GetBaseURL().length()));
	networkRequest.setRawHeader("User-Agent", userAgentStr.toAscii()); 
	RobloxNetworkReply *networkReply = RobloxNetworkAccessManager::Instance().get(networkRequest);

	if (!networkReply->waitForFinished(5000, 100)) 
		return 0; // timed out
    
    networkReply->deleteLater();
			
	return networkReply->statusCode();
}

/*
* De-authenticate the webkit layer.
*/
void AuthenticationHelper::deAuthenticateWebKitLayer()
{	
    QString logoutUrl;
    if (FFlag::UseBuildGenericGameUrl)
    {
        logoutUrl = QString::fromStdString(BuildGenericGameUrl(RobloxSettings::getBaseURL().toStdString(), "game/logout.aspx"));
    }
    else
    {
        logoutUrl = RobloxSettings::getBaseURL() + "/game/logout.aspx";
    }
	QNetworkRequest networkRequest(logoutUrl);
	networkRequest.setRawHeader("User-Agent", userAgentStr.toAscii()); 
	RobloxNetworkReply* networkReply = RobloxNetworkAccessManager::Instance().get(networkRequest);
	if (networkReply)
	{
		networkReply->waitForFinished(5000, 100);
		networkReply->deleteLater();
	}
}

/*
* Authenticate the WinINet layer by making a request to this url.
* @Returns: userId of authenticated user, empty if failed
*/
QString AuthenticationHelper::authenticateWinINetFromUrlWithTicket(const QString& url, const QString& ticket)
{
	if(!url.isEmpty() && !ticket.isEmpty())
	{
		QString compoundStr = generateAuthenticationUrl(url, ticket);
		return doHttpGetRequest(compoundStr);
	}

	return "";
}
QString AuthenticationHelper::doHttpGetRequest(const QString& str, const RBX::HttpAux::AdditionalHeaders& externalHeaders)
{
	std::string result = "";
	try 
	{
		QByteArray ba = str.toAscii();
		const char *c_str = ba.data();

		RBX::Http http(c_str);
#ifdef Q_WS_MAC
			http.setAuthDomain(::GetBaseURL());
#else
			http.additionalHeaders["RBXAuthenticationNegotiation:"] = ::GetBaseURL();
#endif
		http.additionalHeaders.insert(externalHeaders.begin(), externalHeaders.end());
		http.get(result);
	}
	catch (std::exception& e) 
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		return "";
	}

	return QString::fromStdString(result);
}

AuthenticationHelper::AuthenticationHelper()
: m_CurrentHttpRequestState(-1)
, m_bHttpSessionAuthenticationState(false)
, m_bIgnoreFinishedSignal(false)
{
	userAgentStr = RobloxNetworkAccessManager::Instance().getUserAgent();
	if (FFlag::StudioInSyncWebKitAuthentication)
		qApp->installEventFilter(this);
    
	connect(&RobloxNetworkAccessManager::Instance(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )),
			this, SLOT(onSSLErrors(QNetworkReply*, const QList<QSslError> & )));

	connect(&m_HttpAuthenticationFutureWatcher, SIGNAL(finished()), this, SLOT(httpSessionAuthenticationDone()));
}

bool AuthenticationHelper::validateMachine()
{
	return RBX::MachineIdUploader::uploadMachineId(qPrintable(RobloxSettings::getBaseURL()))
		!= RBX::MachineIdUploader::RESULT_MachineBanned;
}

bool AuthenticationHelper::eventFilter(QObject * watched, QEvent * evt)
{
	QEvent::Type eventType = evt->type();

	if (eventType == QEvent::ApplicationDeactivate)
	{
        // save webkit cookies
		savedCookieKey = RobloxNetworkAccessManager::Instance().cookieJar()->getCookieValue(RobloxSettings::getBaseURL(), ".ROBLOSECURITY");
	}
	else if (eventType == QEvent::ApplicationActivate)
	{
		QString currentCookieKey = RobloxNetworkAccessManager::Instance().cookieJar()->getCookieValue(RobloxSettings::getBaseURL(), ".ROBLOSECURITY");
		if (QString::compare(savedCookieKey, currentCookieKey, Qt::CaseSensitive) != 0)	
		{
			// if we've some cookies saved, then authenticate studio with the saved cookies
			if (!currentCookieKey.isEmpty())
			{
				RobloxNetworkAccessManager::Instance().cookieJar()->reloadCookies(true);
			}
			// if no cookies, just deauthenticate http layer too
			else
			{
				deAuthenticateWebKitLayer();
			}

			// to avoid multiple reload of start page, we must call refresh here
			RobloxWebDoc* startPage = dynamic_cast<RobloxWebDoc*>(RobloxDocManager::Instance().getStartPageDoc());
			if (startPage)
				startPage->refreshPage();

			// http user and webkit user must be in sync
			RBXASSERT(getHttpUserId() == getWebKitUserId());
		}
	}
	
	return QObject::eventFilter(watched, evt);
}

int AuthenticationHelper::authenticateWebKitFromHttp()
{
	// first verify is we've an user id for http, if not then just return
	if (getHttpUserId() <=0 )
		return 0;

	QString requestURL = RobloxSettings::getBaseURL() + "/login/RequestAuth.ashx";
	requestURL.replace("http", "https");
	// need to add user agent, to make sure we get authorization URL
	RBX::HttpAux::AdditionalHeaders externalHeaders;
	externalHeaders["User-Agent"] =  userAgentStr.toStdString();
	// got the URL, now we can authorize webkit
    QString returnURL = doHttpGetRequest(requestURL, externalHeaders);
	if (!returnURL.isEmpty())
		return doWebKitAuthentication(returnURL);

	return 0;
}

void AuthenticationHelper::onSSLErrors(QNetworkReply* networkReply, const QList<QSslError> & errlist)
{
    QString urlString = networkReply->url().toString();
    if (urlString.contains(".robloxlabs.com"))
        networkReply->ignoreSslErrors();
}

void AuthenticationHelper::authenticateUserAsync(const QString& url, const QString& ticket)
{
	if (!url.isEmpty() && !ticket.isEmpty())
	{
        // make sure network manager instance get's created in Main thread
        RobloxNetworkAccessManager::Instance();
        
		QString authenticationUrl = generateAuthenticationUrl(url, ticket);

		m_CurrentHttpRequestState = true;

		m_HttpAuthenticationFuture = QtConcurrent::run(this, &AuthenticationHelper::authenticateHttpSession, authenticationUrl);
		m_HttpAuthenticationFutureWatcher.setFuture(m_HttpAuthenticationFuture);

		m_QtWebkitAuthenticationFuture = QtConcurrent::run(this, &AuthenticationHelper::authenticateQtWebkitSession, authenticationUrl);
		m_QtWebkitAuthenticationFutureWatcher.setFuture(m_QtWebkitAuthenticationFuture);
	}
	else
	{
		// if we do not have security cookie, then try to authentication using Http session
		QString secCookie = RobloxNetworkAccessManager::Instance().cookieJar()->getCookieValue(RobloxSettings::getBaseURL(), ".ROBLOSECURITY");
		if (secCookie.isEmpty())
		{
			m_QtWebkitAuthenticationFuture = QtConcurrent::run(this, &AuthenticationHelper::authenticateQtWebkitSession);
			m_QtWebkitAuthenticationFutureWatcher.setFuture(m_QtWebkitAuthenticationFuture);
		}
		else
		{
			m_CurrentHttpRequestState = true;

			m_HttpAuthenticationFuture = QtConcurrent::run(this, &AuthenticationHelper::authenticateHttpSession);
			m_HttpAuthenticationFutureWatcher.setFuture(m_HttpAuthenticationFuture);
		}
	}
}

void AuthenticationHelper::authenticateHttpSessionAsync(bool state)
{
	// if the authentication state is same as the requested then do nothing
	if (state && m_bHttpSessionAuthenticationState)
		return;

	// if either of the concurrent jobs are running then we will have the authentication done
	if (((int)state == m_CurrentHttpRequestState) && (m_HttpAuthenticationFutureWatcher.isRunning() || m_QtWebkitAuthenticationFuture.isRunning()))
		return;

	// make sure there's no other http authentication request is running
	waitForHttpAuthentication();
	m_CurrentHttpRequestState = state;

	if (state)
	{
		m_HttpAuthenticationFuture = QtConcurrent::run(this, &AuthenticationHelper::authenticateHttpSession);
	}
	else
	{
		m_HttpAuthenticationFuture = QtConcurrent::run(this, &AuthenticationHelper::deAuthenticateHttpSession);
	}

	m_HttpAuthenticationFutureWatcher.setFuture(m_HttpAuthenticationFuture);
}

bool AuthenticationHelper::authenticateHttpSession(const QString& authenticationURL)
{
	return getHttpRequest(authenticationURL).isEmpty();
}

bool AuthenticationHelper::authenticateHttpSession()
{
	bool result = false;
	QString baseURL = RobloxSettings::getBaseURL();

	QString authRequest = baseURL + "/login/RequestAuth.ashx";
	authRequest.replace("http", "https");

	QNetworkRequest networkRequest(authRequest);
	networkRequest.setRawHeader("User-Agent", userAgentStr.toAscii()); 

	// we need to create new AccessManager as we are in a different thread, also set cookies loaded from disk to new cookiejar
	QNetworkCookieJar* cookieJar = new QNetworkCookieJar;
	cookieJar->setCookiesFromUrl(RobloxNetworkAccessManager::Instance().cookieJar()->cookiesForUrl(baseURL), baseURL);

	QNetworkAccessManager* accessManager = new QNetworkAccessManager;
	accessManager->setCookieJar(cookieJar);

	QNetworkReply* networkReply = accessManager->get(networkRequest);
	if (baseURL.contains(".robloxlabs.com"))
		networkReply->ignoreSslErrors();

	RobloxNetworkReply* rbxNetworkReply = new RobloxNetworkReply(networkReply, true);
	if (rbxNetworkReply->waitForFinished(10000, 100))
	{
		if (rbxNetworkReply->error() == QNetworkReply::NoError)
		{
			result = getHttpRequest(rbxNetworkReply->readAll()).isEmpty();
		}
		else
		{
			m_CurrentHttpRequestState = false;
			result = deAuthenticateHttpSession();
		}
	}
	else
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Unable to authenticate user. Request timed out.");
	}

	rbxNetworkReply->deleteLater();
	accessManager->deleteLater();

	return result;
}

bool AuthenticationHelper::deAuthenticateHttpSession()
{
    QString logoutUrl;
    if (FFlag::UseBuildGenericGameUrl)
    {
        logoutUrl = QString::fromStdString(BuildGenericGameUrl(RobloxSettings::getBaseURL().toStdString(), "game/logout.aspx"));
    }
    else
    {
        logoutUrl = RobloxSettings::getBaseURL() + "/game/logout.aspx";
    }
    getHttpRequest(logoutUrl);
	return true;
}

bool AuthenticationHelper::authenticateQtWebkitSession(const QString& authenticationUrl)
{
	bool result = false;
	QString baseURL = RobloxSettings::getBaseURL();

	QNetworkRequest networkRequest(authenticationUrl);
	networkRequest.setRawHeader("RBXAuthenticationNegotiation:", baseURL.toAscii());
	networkRequest.setRawHeader("User-Agent", userAgentStr.toAscii());

	// we need to create new AccessManager as we are in a different thread
	QNetworkAccessManager* accessManager = new QNetworkAccessManager;
	QNetworkReply* networkReply = accessManager->get(networkRequest);
	if (baseURL.contains(".robloxlabs.com"))
		networkReply->ignoreSslErrors();
	RobloxNetworkReply*  rbxNetworkReply = new RobloxNetworkReply(networkReply, true);

	if (rbxNetworkReply->waitForFinished(10000, 100))
	{
		if (rbxNetworkReply->statusCode() == 200)
		{
			// pass on the cookies from authenticated session to the static network access manager's cookiejar
			RobloxNetworkAccessManager::Instance().cookieJar()->setCookiesFromUrl(accessManager->cookieJar()->cookiesForUrl(baseURL), baseURL);
			result = true;
		}
	}
	else
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Unable to authenticate user. Request timed out.");
	}

	rbxNetworkReply->deleteLater();
	accessManager->deleteLater();

	return result;
}

bool AuthenticationHelper::authenticateQtWebkitSession()
{
	// check if already authenticated
	QString result = getHttpRequest(getLoggedInUserUrl());
    int httpUserId = result.toInt();
    // if authenticated then authenticate webkit session too
	if (httpUserId > 0)
	{
		QString requestURL = RobloxSettings::getBaseURL() + "/login/RequestAuth.ashx";
		requestURL.replace("http", "https");

		// need to add user agent, to make sure we get authorization URL
		RBX::HttpAux::AdditionalHeaders externalHeaders;
		externalHeaders["User-Agent"] =  userAgentStr.toStdString();

		// get negotiate URL and authenticate QtWebkit using this
		QString negotiateURL = getHttpRequest(requestURL, externalHeaders);
		if (!negotiateURL.isEmpty())
			return authenticateQtWebkitSession(negotiateURL);
	}

	return false;
}

QString AuthenticationHelper::getHttpRequest(const QString& urlString, const RBX::HttpAux::AdditionalHeaders& externalHeaders)
{
	static QMutex httpRequestMutex;
	QMutexLocker lock(&httpRequestMutex);

	RBX::HttpOptions options;
	options.addHeader("RBXAuthenticationNegotiation:", ::GetBaseURL());

	for (RBX::HttpAux::AdditionalHeaders::const_iterator iter = externalHeaders.begin(); iter != externalHeaders.end(); ++iter)
		options.addHeader(iter->first, iter->second);
	
	RBX::HttpFuture request = RBX::HttpAsync::get(urlString.toStdString(), options);
	std::string result;

	try
	{
		result = request.get();
	}
	catch (const RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error while authenticating user: %s", e.what());
	}

	return QString::fromStdString(result);
}

void AuthenticationHelper::httpSessionAuthenticationDone()
{
	if (m_bIgnoreFinishedSignal)
	{
		m_bIgnoreFinishedSignal = false;
		return;
	}

	RBXASSERT(m_CurrentHttpRequestState != -1);

	bool requestedState = (bool)m_CurrentHttpRequestState;
	m_CurrentHttpRequestState = -1;

	if (m_HttpAuthenticationFutureWatcher.result())
	{
		m_bHttpSessionAuthenticationState = requestedState;
		Q_EMIT authenticationChanged(requestedState);
	}
	else
	{
		m_bHttpSessionAuthenticationState = false;
	}
}

void AuthenticationHelper::waitForHttpAuthentication() 
{
	if (m_HttpAuthenticationFutureWatcher.isRunning())
	{
		m_HttpAuthenticationFutureWatcher.waitForFinished();
		// if a function is waiting for authentication to complete then call 'httpSessionAuthenticationDone' here itself
		httpSessionAuthenticationDone();
		// make sure we do not calling 'httpSessionAuthenticationDone' again called on 'finished' signal
		m_bIgnoreFinishedSignal = true;
	}
}

void AuthenticationHelper::waitForQtWebkitAuthentication() 
{
	if (m_QtWebkitAuthenticationFutureWatcher.isRunning())
		m_QtWebkitAuthenticationFutureWatcher.waitForFinished();
}