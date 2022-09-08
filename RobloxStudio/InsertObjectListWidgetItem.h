/**
 * InsertObjectListWidgetItem.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
	#define QT_NO_KEYWORDS
#endif

#include <QListWidgetItem>

#include "V8Tree/Instance.h"

class InsertObjectListWidgetItem : public QListWidgetItem
{
public:
	InsertObjectListWidgetItem(
		const QString&                      name, 
		const QString&                      description, 
		boost::shared_ptr<RBX::Instance>    instance, 
		std::string                         preferredParentName );

    boost::shared_ptr<RBX::Instance> getInstance() { return m_instance; }
    std::string getPreferredParent() { return m_preferredParent; }
    bool checkFilter(QString &filterString, RBX::Instance* pParent);

private:
	boost::shared_ptr<RBX::Instance>	m_instance;
	std::string		m_preferredParent;
};

