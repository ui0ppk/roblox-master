/**
 * RobloxPrizeAwarder.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxPrizeAwarder.h"

// Qt headers
#include <QMutex>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QDialog>
#include <QLabel>
#include <QGridLayout>
#include <QPixmap>
#include <QCryptographicHash>

// Roblox headers
#include "V8DataModel/FastLogSettings.h"

// Roblox Studio headers
#include "RobloxNetworkAccessManager.h"
#include "RobloxSettings.h"
#include "RobloxNetworkReply.h"
#include "AuthenticationHelper.h"
#include "RobloxUser.h"

#define USER_DATA_FIELD "UserData"

RobloxPrizeAwarder::RobloxPrizeAwarder() 
	: m_initialized(false)
{

}
/*
* Fetch our prize data
*/
void RobloxPrizeAwarder::init()
{
	static QMutex initMutex;

	if (m_initialized)
		return;

	try
	{
		initMutex.lock();
		if (m_initialized)
		{
			initMutex.unlock();
			return;
		}

		// Usage:  ENUMID = ASSETID ; ENUMID = ASSETID; ENUMID = ASSETID
		// Where ENUMID correlates to the value in our PrizeType enum
		// 0 = 234512; 1 = 12999; 2 = 234234;
		QString prizesAvailable = RBX::ClientAppSettings::singleton().GetValuePrizeAssetIDs();
		QStringList prizesParsed = prizesAvailable.split(";");

		for (int i = 0; i < prizesParsed.length(); i++)
		{
			QString str = prizesParsed[i];
			QStringList strList = str.split("=");
			if (strList.length() != 3)
				continue;

			bool ok;
			int prizeTypeId = strList[0].trimmed().toInt(&ok);
			if (!ok)
				continue;

			long assetId = strList[1].trimmed().toLong(&ok);
			if (!ok || assetId <= 0)
				continue;

			QString prizeName = strList[2];

			PrizeType type = (PrizeType)prizeTypeId;
			if (type >= PRIZETYPE_MAXCOUNT || type < 0)
				continue;

			m_prizeAssetDataMap.insert(type, AssetData(assetId, prizeName));
		}
	}
	catch(std::exception)
	{
	}

	m_initialized = true;  // In case of failure, don't try to initialize again
	initMutex.unlock();
}
/*
* Prevent users from copy and pasting strings between browsers.  Isn't much more secure than that
*/
QString RobloxPrizeAwarder::generateInsecureString(long assetID, int userId)
{
	QString string("Cullen is the saltiest Programmer. %1%2");
	string = string.arg(userId).arg(assetID);
	QCryptographicHash hasher(QCryptographicHash::Sha1);
	hasher.addData(string.toStdString().c_str());
	return QString(hasher.result().toHex());
}
void RobloxPrizeAwarder::awardPrize( const PrizeType type )
{
	try
	{
		// FFlag
		const char* prizeAssetIds = RBX::ClientAppSettings::singleton().GetValuePrizeAssetIDs();
		if (!prizeAssetIds || prizeAssetIds[0] == '\0')
			return;

		// Lazy init
		init();

		// Make sure we have been initialized
		RBXASSERT(m_prizeAssetDataMap.size());
		if (!m_prizeAssetDataMap.size())
			return;

		// Make sure we have a prize for this
		long assetId = m_prizeAssetDataMap.value(type).assetId;
		if (!assetId)
			return;
		
		// Check locally to prevent massive # of requests
		if (isPrizeAwardedAlready(type))
			return;
		
		int webKitUserId = RobloxUser::singleton().getUserId();
		if (!webKitUserId)
			return;

		QString	url("%1%2?assetID=%3&token=%4");
		url = url.arg(RobloxSettings::getBaseURL())
			.arg(RBX::ClientAppSettings::singleton().GetValuePrizeAwarderURL())
			.arg(assetId)
			.arg(generateInsecureString(assetId, webKitUserId));
				
		const QNetworkRequest networkRequest(url);
		RobloxNetworkReply* networkReply = RobloxNetworkAccessManager::Instance().get(networkRequest);
		networkReply->setProperty(USER_DATA_FIELD, type);

		QObject::connect(networkReply, SIGNAL(finished()), &RobloxPrizeAwarder::singleton(), SLOT(handlePrizeAwardRequestFinished()));
	}
	catch (std::exception)
	{
	}	
}
/*
Called when our image has been fetched.  We then display the award dialog.
*/
void RobloxPrizeAwarder::handleImageRequestFinished()
{
	RobloxNetworkReply* networkReply = dynamic_cast<RobloxNetworkReply*>(sender());
	if (!networkReply)
		return;
	
	// Extract our image, and prize type, and prize name from this reply
	QByteArray imageData = networkReply->readAll();
	PrizeType prizeType = (PrizeType)networkReply->property(USER_DATA_FIELD).toInt();
	networkReply->deleteLater();
	QString prizeAwardText = m_prizeAssetDataMap.value(prizeType).prizeAwardedText;
	
	// Create our dialog
	QDialog box(0, Qt::WindowTitleHint);
	box.setWindowTitle("You found a prize!");
	box.setFixedSize(275, 275);
	QPalette palette;
	palette.setColor(QPalette::Window, QColor("white"));
	box.setPalette(palette);
	QLayout* layout = new QGridLayout();
	QLabel* label = new QLabel("");  
	label->setAlignment(Qt::AlignCenter);
	QPixmap pixmap;
	pixmap.loadFromData(imageData);
	label->setPixmap(pixmap);
	label->adjustSize();  
	layout->addWidget(label);
	QLabel* textLabel = new QLabel();
	textLabel->setWordWrap(true);
	textLabel->setTextFormat(Qt::RichText);
	textLabel->setText(QString("<span style='font-size: 12px'><b>%1</b></span>").arg(prizeAwardText.replace("\n", "<br/>")));
	textLabel->adjustSize();
	textLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(textLabel);
	box.setLayout(layout);
	box.exec();
}
/*
* Save locally to prevent massive # of requests
*/
void RobloxPrizeAwarder::setPrizeAwarded(PrizeType type, bool isPerUser) const
{
	// Persist to settings so we dont do again
	RobloxSettings settings;
	int userId = 0;
	if (isPerUser)
		userId = RobloxUser::singleton().getUserId();
	
	QString settingValue("ppa_%1_%2");
	settingValue = settingValue.arg(type).arg(userId);
	settings.setValue(settingValue, true);
}
/*
* Check locally to see if it's been awarded
*/
bool RobloxPrizeAwarder::isPrizeAwardedAlready(PrizeType type, bool isPerUser) const
{
	// TODO: Per session?
	RobloxSettings settings;
	int userId = 0;
	if (isPerUser)
		userId = RobloxUser::singleton().getUserId();
	QString settingValue("ppa_%1_%2");
	settingValue = settingValue.arg(type).arg(userId);
	return settings.value(settingValue, false).toBool();
}
/*
* Our http requests has returned telling us whether the user has received a prize or not. 
* If they have (200 status code), asynchronously fetch the image for this prize from the 
* url given to us in the response stream.  Once this image has returned, show the dialog
* with handleImageRequestFinished().
*/
void RobloxPrizeAwarder::handlePrizeAwardRequestFinished()
{
	RobloxNetworkReply* networkReply = dynamic_cast<RobloxNetworkReply*>(sender());
	if (!networkReply)
		return;

	int statusCode = networkReply->statusCode();
	PrizeType typeBeingAwarded = (PrizeType)networkReply->property(USER_DATA_FIELD).toInt();
	switch (statusCode)
	{
		case 200: // Success
		{
			QUrl imageUrl(networkReply->readAll());
			QNetworkRequest networkRequest(imageUrl);
			RobloxNetworkReply* imageNetworkReply = RobloxNetworkAccessManager::Instance().get(networkRequest);
			imageNetworkReply->setProperty(USER_DATA_FIELD, typeBeingAwarded); // Pass through any user data
			connect(imageNetworkReply, SIGNAL(finished()), this, SLOT(handleImageRequestFinished()));
			setPrizeAwarded(typeBeingAwarded); 
			break;
		}
		case 403: // Not authenticated -- should be 401 but the website automatically redirects on 401 to the login page so this is a hack
			break;
		case 500: // Who knows -- duplicate maybe?
			break;
	}
	networkReply->deleteLater();
}
