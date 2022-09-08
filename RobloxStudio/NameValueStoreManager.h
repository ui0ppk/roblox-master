/**
 *  SimulationManager.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QMap>

class QVariant;

class NameValueStoreManager
{
public:
	typedef QMap<QString, QVariant>  Properties;
	typedef QMap<QString, Properties> Stores;

	static NameValueStoreManager& singleton()
	{
		static NameValueStoreManager manager;
		return manager;
	}

	void setValue(const QString& storeID, const QString& key, const QVariant &value);
	QVariant getValue(const QString&storeID, const QString& key, bool *isOK = NULL);

	bool clear(const QString& storeID);
	void clearAll();

private:
	Stores store;
};