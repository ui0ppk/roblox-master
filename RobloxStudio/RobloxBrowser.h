/**
 * RobloxBrowser.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QWebView>

class RobloxBrowser : public QWebView 
{
	Q_OBJECT

public: 

	RobloxBrowser(QWidget* parent=0);
    virtual ~RobloxBrowser();

public Q_SLOTS:

	bool close();
    void resetLoadingTimer();
    
protected:
    
    virtual void paintEvent(QPaintEvent*);
 	virtual void dropEvent(QDropEvent* evt);

private Q_SLOTS:

    void loadStarted();
    void loadFinished(bool);
    
private:
    
    void drawLoadingWatermark();
    
	RobloxBrowser*  m_pPopup;
	QDialog*        m_pPopupDlg;
    QTimer*         m_loadingTimer;
    float           m_refreshIncr;
};
