/**
 * InsertObjectListWidgetItem.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "InsertObjectListWidgetItem.h"

// Qt Headers
#include <QIcon>

// ROBLOX Headers
#include "QtUtilities.h"
#include "RobloxTreeWidget.h"


InsertObjectListWidgetItem::InsertObjectListWidgetItem(
	const QString&                      name, 
	const QString&                      description, 
	boost::shared_ptr<RBX::Instance>    instance, 
	std::string                         preferredParentName )
	: m_instance(instance)
	, m_preferredParent(preferredParentName)
{
	setIcon(QIcon(QtUtilities::getPixmap(":/images/ClassImages.PNG", RobloxTreeWidgetItem::getImageIndex(m_instance))));
	setText(name);

	QString desc(description);
	setToolTip(QtUtilities::wrapText(desc.replace(QRegExp("<a href.*(\\/a>|\\/>)"), ""), 80));
}

bool InsertObjectListWidgetItem::checkFilter(QString &filterString, RBX::Instance* pParent)
{
    return pParent && m_instance && m_instance->canSetParent(pParent) && (filterString.isEmpty() || text().contains(filterString, Qt::CaseInsensitive));
}