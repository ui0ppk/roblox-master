/**
 * AuthenticationHelper.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

// Roblox Headers
#include "util/HttpAux.h"
#include "RobloxNetworkAccessManager.h"

class AuthenticationHelper : public QObject
{
Q_OBJECT

public:
	static AuthenticationHelper& Instance() 
	{	
		static AuthenticationHelper instance;
		return instance;
	}
	static QString userAgentStr;
	
	// synchronus authentication
	QString authenticateWinINetFromUrlWithTicket(const QString& url, const QString& ticket);

	static bool validateMachine();

	void authenticateUserAsync(const QString& url, const QString& ticket);
	void authenticateHttpSessionAsync(bool state);
	
	void waitForHttpAuthentication();
	void waitForQtWebkitAuthentication();
	
	static QString getLoggedInUserUrl();

Q_SIGNALS:
	void authenticationDone(bool authSuccess);
    void authenticationChanged(bool isAuthenticated) const;

public Q_SLOTS:
	bool authenticateHttpLayer();
	void deAuthenticateHttpLayer();
	bool verifyUserAndAuthenticate(int timeOutTime = 10000);
    
private Q_SLOTS:
    void onSSLErrors(QNetworkReply* networkReply, const QList<QSslError> & errlist);
	void httpSessionAuthenticationDone();

private:
	AuthenticationHelper();

	// Helper functions
	QString generateAuthenticationUrl(const QString& authUrl, const QString& ticket);

	int getHttpUserId();
	int getWebKitUserId();
	
	// WebKit auth
	int authenticateWebKitFromHttp();
	int authenticateWebKitFromUrlWithTicket(const QString& url, const QString& ticket);
	int doWebKitAuthentication(const QString& url);
	void deAuthenticateWebKitLayer();

	// WinINet auth
	QString doHttpGetRequest(const QString& url, const RBX::HttpAux::AdditionalHeaders& externalHeaders = RBX::HttpAux::AdditionalHeaders());

	/*override*/bool eventFilter(QObject * watched, QEvent * evt);

	void authenticateQtWebkitSessionAsync();

	bool authenticateHttpSession(const QString& authenticationURL);
	bool authenticateHttpSession();
	
	bool authenticateQtWebkitSession(const QString& authenticationURL);
	bool authenticateQtWebkitSession();

	bool deAuthenticateHttpSession();

	QString getHttpRequest(const QString& urlString, const RBX::HttpAux::AdditionalHeaders& externalHeaders = RBX::HttpAux::AdditionalHeaders());

	QFuture<bool>             m_HttpAuthenticationFuture;
	QFutureWatcher<bool>      m_HttpAuthenticationFutureWatcher;

	QFuture<bool>             m_QtWebkitAuthenticationFuture;
	QFutureWatcher<bool>      m_QtWebkitAuthenticationFutureWatcher;

	int                       m_CurrentHttpRequestState;
	bool                      m_bHttpSessionAuthenticationState;
	bool                      m_bIgnoreFinishedSignal;

	// Member variables
	QString savedCookieKey;

	static bool httpAuthenticationPending;
	static bool verifyUserAuthenticationPending;
    static bool previouslyAuthenticated;
};
