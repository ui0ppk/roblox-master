/**
 * RobloxReportView.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QTreeWidget>
#include <QStyledItemDelegate>

class RobloxCategoryItemDelegate : public QStyledItemDelegate
{
public:
	RobloxCategoryItemDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const;
};

class RobloxCategoryItem: public QTreeWidgetItem
{
public:
	RobloxCategoryItem();
};

class RobloxReportView : public QTreeWidget
{
public:
	RobloxReportView(); 
	virtual ~RobloxReportView();

	void addCategoryItem(RobloxCategoryItem* pItem);
	bool isCategoryItem(const QModelIndex &index);
	RobloxCategoryItem* findCategoryItem(const QString& category);
	bool addToCategory(const QString& category, QTreeWidgetItem* pItem);
		
private:
	/*override*/void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	/*override*/void drawBranches(QPainter * painter, const QRect & rect, const QModelIndex & index ) const;
};


