/**
 * RobloxToolBox.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <boost/shared_ptr.hpp>

// Roblox Headers
#include "reflection/Type.h"

// Roblox Studio Headers
#include "RobloxBrowser.h"
#include "RobloxWebPage.h"

namespace RBX
{
	class DataModel;
}

class RbxWorkspace;


class RobloxToolBox : public QWidget
{
	Q_OBJECT
	
public:
	RobloxToolBox();
	void setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel);
	QString getTitleFromUrl(const QString &urlString);
	
public Q_SLOTS:
	void onAuthenticationChanged(bool isAuthenticated);
	void loadUrl(const QString url);

private:
	RobloxBrowser *m_pWebView;
	RobloxWebPage *m_pWebPage;
	boost::shared_ptr<RBX::DataModel>	m_pDataModel;
	
	QString m_urlString;
	
	boost::shared_ptr<RbxWorkspace> m_pWorkspace;
	
	void setupWebView(QWidget *wrapperWidget);
	bool reloadView;
	
private Q_SLOTS:
	void initJavascript();
	
};
