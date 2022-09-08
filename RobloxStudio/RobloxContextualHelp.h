/**
 * RobloxContextualHelp.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include "RobloxBrowser.h"
#include "RobloxWebPage.h"

#include <QWidget>

class RobloxContextualHelpService : public QObject
{
    Q_OBJECT
public:
    RobloxContextualHelpService();
    
    static RobloxContextualHelpService& singleton();

public Q_SLOTS:
    void onHelpTopicChanged(const QString& helpTopic);

Q_SIGNALS:
    void helpTopicChanged(const QString&);

private:
    QString m_helpTopic;
};

class RobloxContextualHelp;

class RobloxHelpWebView : public RobloxBrowser
{
    Q_OBJECT
public:
    RobloxHelpWebView(RobloxContextualHelp* widget);

protected:
    void paintEvent(QPaintEvent* event);

private Q_SLOTS:
    void loadProgress(int);

private:
    RobloxContextualHelp*    m_contextualHelpWidget;
};

class RobloxContextualHelp : public QWidget
{
	Q_OBJECT
	
public:
    RobloxContextualHelp();

    void updateURL();

public Q_SLOTS:
    void onAuthenticationChanged(bool isAuthenticated);
    void onHelpTopicChanged(const QString& helpTopic);
    
private Q_SLOTS:
	void linkClicked(const QUrl& url);
    
private:
    void setupWebView();
   
    RobloxHelpWebView*      m_pWebView;
    RobloxWebPage*          m_pWebPage;
    QString                 m_urlString;
    bool                    m_urlDirty;
};

