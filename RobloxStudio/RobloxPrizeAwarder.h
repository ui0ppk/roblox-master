/**
 * RobloxPrizeAwarder.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QMap>
#include <QObject>

class RobloxNetworkReply;

class RobloxPrizeAwarder : public QObject
{
	Q_OBJECT
public:
	// NOTE: Do not change enum values -- these are correlated to assetids via a client app setting value
	enum PrizeType
	{
		PRIZETYPE_MAXCOUNT
	};
	struct AssetData
	{
		AssetData(long assetIdData, QString prizeAwardText) 
		{ 
			assetId = assetIdData; 
			prizeAwardedText = prizeAwardText;
		}
		AssetData(){}
		
		long assetId;
		QString prizeAwardedText;
	};
	static RobloxPrizeAwarder& singleton()
	{
		static RobloxPrizeAwarder instance;
		return instance;
	}
	
	void awardPrize(const PrizeType type);

public Q_SLOTS:
	void handlePrizeAwardRequestFinished();

private Q_SLOTS:
	void handleImageRequestFinished();

private:
	RobloxPrizeAwarder();
	void init();
	QString generateInsecureString(long assetID, int userId);
	bool isPrizeAwardedAlready(PrizeType type, bool isPerUser = true) const;
	void setPrizeAwarded(PrizeType type, bool isPerUser = true) const;

	QMap<PrizeType, AssetData> m_prizeAssetDataMap;
	bool m_initialized;
};
