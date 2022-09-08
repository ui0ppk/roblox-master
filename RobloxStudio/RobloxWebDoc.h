/**
 * RobloxWebDoc.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QObject>

#include <boost/shared_ptr.hpp>

// Roblox Studio Headers
#include "RobloxBasicDoc.h"

class QComboBox;
class QNetworkReply;
class QUrl;
class QToolBar;
class QSslError;

class RobloxBrowser;
class RbxWorkspace;

class RobloxWebDoc: public QObject, public RobloxBasicDoc
{
	Q_OBJECT
public:

	RobloxWebDoc(const QString& displayName, const QString& keyName);
	~RobloxWebDoc();
	
	bool open(RobloxMainWindow *pMainWindow, const QString &fileName);
	
	IRobloxDoc::RBXCloseRequest requestClose() { return IRobloxDoc::NO_SAVE_NEEDED; }

	IRobloxDoc::RBXDocType docType() { return IRobloxDoc::BROWSER; }

	QString fileName() const        { return ""; }
	QString displayName() const     { return m_displayName; }
	QString keyName() const         { return m_keyName; }
			
	bool save(){ return false; }
	bool saveAs(const QString &){ return false; }

	QString saveFileFilters() { return ""; }
	
	QWidget* getViewer();
	
	bool isModified(){ return false; }
	
	void activate();
	void deActivate() { m_bActive = false; }
		
	bool actionState(const QString &actionID, bool &enableState, bool &checkedState);

	bool handlePluginAction(void *, void *) { return false; }

	void handleScriptCommand(const QString &) { return; }	

	bool supportsZeroPlaneGrid()        { return false; }

	boost::shared_ptr<RbxWorkspace> getWorkspace() { return m_pWorkspace; }

public Q_SLOTS:
	void refreshPage();
	
private Q_SLOTS:
	void updateAddressBar(const QUrl&);
	void sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & errlist);
	void initJavascript();
	void handleHomeClicked();
	void navigateUrl(const QString& url);
	void onAuthenticationChanged(bool);
	void updateTitle(QString title);

private:
	virtual bool doClose();
	void setupWebView(QWidget*);
	QToolBar* setupAddressToolBar(QWidget*);
	

	RobloxBrowser                       *m_pWebView;
	QWidget                             *m_pWrapperWidget;
	boost::shared_ptr<RbxWorkspace>      m_pWorkspace;
	QComboBox                           *m_pAddrInputComboBox;
	QString                              m_displayName; //to be used if file name is empty
	QString                              m_currentUrl;
	QString                              m_homeUrl;
	QString                              m_keyName;
};

