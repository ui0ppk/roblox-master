/**
 * CommonInsertWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QWidget>
#include <QListWidgetItem>
#include <QDialog>
#include <QCheckBox>
#include <QSet>

// Roblox Headers
#include "rbx/signal.h"
#include "reflection/Property.h"

namespace RBX {
	class DataModel;
	class Instance;
	class SelectionChanged;
	namespace Reflection {
		class ClassDescriptor; 
	}
}

class QTabWidget;
class QListWidget;
class QLineEdit;
class QPushButton;
class RobloxToolBox;
class InsertObjectWidget;
class InsertObjectListWidget;
class InsertObjectListWidgetItem;

class InsertObjectWidget : public QWidget
{
	Q_OBJECT 

public:

	InsertObjectWidget(QWidget *pParentWidget);
	virtual ~InsertObjectWidget();
	
	void setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel);

	enum InsertMode
	{
		InsertMode_InsertWidget,
		InsertMode_TreeWidget,
		InsertMode_RibbonAction,
		InsertMode_ContextMenu
	};

	static void InsertObject(
        boost::shared_ptr<RBX::Instance>    pObjectToInsert,
        boost::shared_ptr<RBX::DataModel>   pDataModel,
		InsertMode insertMode = InsertMode_InsertWidget,
		QPoint* mousePosition = NULL);
    static void InsertObject(
        const QString&                      className,
        boost::shared_ptr<RBX::DataModel>   pDataModel,
		InsertMode insertMode = InsertMode_InsertWidget,
		QPoint* mousePosition = NULL);

    static void createWidgets(bool services, InsertObjectListWidget* listWidget, const RBX::Reflection::ClassDescriptor* pDescriptor, shared_ptr<RBX::DataModel> pDataModel);

    static void populateList(bool services, QList<InsertObjectListWidgetItem*> *itemList, const RBX::Reflection::ClassDescriptor* pDescriptor, shared_ptr<RBX::DataModel> pDataModel);
    static void populateListHelper(bool services, QList<InsertObjectListWidgetItem*> *itemList, const RBX::Reflection::ClassDescriptor& pDescriptor, shared_ptr<RBX::DataModel> pDataModel);

    static QMenu* createMenu(
        RBX::Instance*  parent,
        const QObject*  receiver, 
        const char*     member );    

	void setFilter(QString filter = QString(""));

    bool isSelectObject() const         { return m_pCheckBox->isChecked(); }

    void startingQuickInsert(QWidget* widget, bool closeDockWhenDoneWithQuickInsert);

	static void SetObjectDefaultValues(boost::shared_ptr<RBX::Instance> pObjectToInsert);

	static RBX::Vector3 getInsertLocation(boost::shared_ptr<RBX::DataModel> pDataModel, QPoint* insertPosition, bool* isOnPart = NULL);

	virtual QSize sizeHint() const;

public Q_SLOTS:

	void updateWidget(bool state);
	virtual void setVisible(bool visible);

private Q_SLOTS:

	void onFilter(QString filter);
	void redrawDialog();
	void onSelectOnInsertClicked(bool checked);
    void onItemInsertRequested();
	void onPreviousWidgetDestroyed() { m_quickInsertPreviousWidget = NULL; }

protected:
	virtual bool event(QEvent* e);
    virtual bool eventFilter(QObject* watched, QEvent* evt);

private:	
	void onInstanceSelectionChanged(const RBX::SelectionChanged& evt);
	void connectSelectionChangeEvent(bool connectSignal);

	void requestDialogRedraw();

    static bool instanceExistsInDataModel(shared_ptr<RBX::Instance> instance, shared_ptr<RBX::DataModel> pDataModel);
	
	static RBX::Instance* getTargetInstance(RBX::Instance* pInstance, boost::shared_ptr<RBX::DataModel> pDataModel);
	RBX::Instance* getSelectionFrontPart();

    typedef QMap<QString,boost::shared_ptr<RBX::Instance> > tInstanceMap;
    static void generateInstanceMap(
        const RBX::Reflection::ClassDescriptor* pDescriptor,
        tInstanceMap*                           instanceMap,
        RBX::Instance*                          parent );

	boost::shared_ptr<RBX::DataModel>	            m_pDataModel;
		
	InsertObjectListWidget*                         m_pInsertObjectListWidget;
	QLineEdit*                                      m_pFilterEdit;
	QCheckBox*										m_pCheckBox;
	
	rbx::signals::connection                        m_cSelectionChanged;
	
	bool                                            m_bInitializationRequired;
	bool											m_bRedrawRequested;

    static QHash<QString, QVariant>                  m_objectWeights;

    static QSet<QString>                            m_sObjectExceptions;

    QWidget*                                        m_quickInsertPreviousWidget;
	bool                                            m_closeDockWhenDoneWithQuickInsert;

    QList<InsertObjectListWidgetItem*>*             m_ItemList;

	static RBX::Vector3                             m_sCachedInsertLocation;
	static RBX::CoordinateFrame                     m_sCameraCFrame;
	static bool                                     m_sIsInsertLocationValid;
};
