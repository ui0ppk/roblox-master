/**
 * InsertObjectListWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
	#define QT_NO_KEYWORDS
#endif

#include <QListWidget>

#include "rbx/signal.h"
#include "Reflection/Property.h"

class QListWidgetItem;

class InsertObjectListWidget: public QListWidget
{
	Q_OBJECT
public:
	InsertObjectListWidget(QWidget *pParent);
	virtual ~InsertObjectListWidget();
	void InsertObject(QListWidgetItem* item);
	virtual void keyPressEvent(QKeyEvent *event);
    virtual void sortItems(const QHash<QString, QVariant> &itemWeights = (QHash<QString, QVariant>()), QString filter = QString(), Qt::SortOrder order = Qt::AscendingOrder);

protected:
    virtual bool event(QEvent *e);

Q_SIGNALS:
	void enterKeyPressed(QListWidgetItem *pItem);
	void itemInserted();
    void helpTopicChanged(const QString& topic);
    
public Q_SLOTS:
	void onItemInsertRequested(QListWidgetItem* item = NULL);

private Q_SLOTS:
    void onSelectionChanged();
    
private:
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

    void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

	QPoint  m_dragStartPosition;
	rbx::signals::scoped_connection m_PropertyChangedConnection;
};
