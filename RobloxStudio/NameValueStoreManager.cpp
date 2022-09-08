/**
 *  SimulationManager.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "NameValueStoreManager.h"

// Qt Headers
#include <QVariant>

void NameValueStoreManager::setValue(const QString& storeID, const QString& key, const QVariant &value)
{
	Stores::iterator iter = store.find(storeID);
	if (iter != store.end())
	{
		iter.value().insert(key, value);
	}
	else
	{
		Properties prop;
		prop.insert(key, value);

		store.insert(storeID, prop);
	}
}

QVariant NameValueStoreManager::getValue(const QString&storeID, const QString& key, bool *isOK)
{
	if (isOK) *isOK = false;

	Stores::iterator iter = store.find(storeID);
	if (iter != store.end())
	{
		Properties prop = iter.value();
		Properties::iterator prop_iter = prop.find(key);
		if (prop_iter != prop.end())
		{
			if (isOK) *isOK = true;
			return prop_iter.value();
		}
	}

	return QVariant();
}

bool NameValueStoreManager::clear(const QString& storeID)
{ return store.remove(storeID); }

void NameValueStoreManager::clearAll()
{ store.clear(); }