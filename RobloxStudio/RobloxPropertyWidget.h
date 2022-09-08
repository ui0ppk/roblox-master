/**
 * RobloxPropertyWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Standard C/C++ Headers
#include <map>
#include <list>

// Qt Headers
#include <QMutex>

// Roblox Headers
#include "rbx/signal.h"

// Roblox Studio Headers
#include "PropertyItems.h"

namespace RBX {
	class DataModel;
	class SelectionChanged;
	class Instance;
	namespace Reflection
	{
		class ClassDescriptor;
		class PropertyDescriptor;
		class Variant;
	}
}

class QLabel;
class QLineEdit;
class CategoryItem;
class PropertyItem;
class PropertyItemUpdateJob;

typedef std::list<PropertyItem *>                                           PropertyItems;
typedef std::map<QString, CategoryItem *>                                   CategoryItems;
typedef std::map<const RBX::Reflection::PropertyDescriptor*, PropertyItem*> PropertyItemMap;
typedef std::map<const RBX::Reflection::ClassDescriptor*, InstanceList>     ClassDescriptorInstanceMap;
typedef std::map<RBX::Instance*, rbx::signals::connection>                  Connections;

class CategoryItem: public QTreeWidgetItem
{
public:
	CategoryItem(const QString &name);
	
	void update(bool storeCollapsedState);
	void applyFilter(const QString &filterString);
};

class PropertyTreeWidget: public QTreeWidget
{
	Q_OBJECT
public:
	PropertyTreeWidget(QWidget *pParent);
	virtual ~PropertyTreeWidget();

	void setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel);
	boost::shared_ptr<RBX::DataModel> getDataModel() { return m_pDataModel; }

	void addInstance(RBX::Instance* pInstance);
	void removeInstance(RBX::Instance* pInstance);

	ClassDescriptorInstanceMap& getSelectionData() { return m_selectionData; }

	PropertyItem* propertyItemFromIndex(const QModelIndex &index) const;

	void closeEditor(QWidget * editor, QAbstractItemDelegate::EndEditHint hint);

	virtual void requestUpdate();	
	void reinitialize();

	void setStoreCollapsedState(bool state) { m_bStoreCollapasedState = state; }
	
	void setPropertyCallback(const QObject* sender, const char* signal, boost::function<void(QColor)> callback);
	void resetPropertyCallback();

	void storeOriginalValues(PropertyItem* item);
	void restoreOriginalValues(PropertyItem* item);
	bool hasOriginalValues() { return !m_originalValues.empty(); }
	void resetOriginalValues() { m_originalValues.clear(); }
	bool hasSameOriginalValues() { return m_bSameOriginalValues; }

protected:
	virtual void initProperty(const RBX::Reflection::ClassDescriptor* pClassDescriptor, 
		              const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);

Q_SIGNALS:
    void helpTopicChanged(const QString& topic);

private Q_SLOTS:
	void setFilter(QString filter);
	void update();
	void onItemExpanded(QTreeWidgetItem * pItem);
	void onItemCollapsed(QTreeWidgetItem * pItem);
	void updatePropertySlot(QColor objectValue); //If other types are needed, use QVariant with QSignalMapper
	void sendPropertySearchCounter();
    
private:
	void visitClassProperties(const RBX::Reflection::ClassDescriptor* pClassDescriptor);
	CategoryItem* getCategoryItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);
	QString getRegistryKey(CategoryItem* pCategoryItem);

	void applyFilter();
	void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void mousePressEvent(QMouseEvent *event);

	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);

	void createUpdateJob();
	void removeUpdateJob();

	boost::unordered_map<shared_ptr<RBX::Instance>, RBX::Reflection::Variant>	m_originalValues;

	/*override*/bool event(QEvent * evt);

	//static data
	CategoryItems                            m_categoryItems;
	PropertyItemMap                          m_itemMap;
    std::vector<const RBX::Reflection::PropertyDescriptor*> m_items;

	//dynamic data
	std::set<PropertyItem*>                  m_invalidItems;
	ClassDescriptorInstanceMap               m_selectionData;	
	Connections                              m_cPropertyChanged;

	boost::shared_ptr<PropertyItemUpdateJob> m_pUpdateItemsJob;
	QIcon                                    m_expandIcon;	
	QString                                  m_filterString;

	QMutex                                   m_propModifyMutex;
	QMutex                                   m_propUpdateMutex;

	boost::shared_ptr<RBX::DataModel>        m_pDataModel;

	bool                                     m_bIsPropertyChanged;
	bool                                     m_bIgnorePropertyChanged;
	bool                                     m_bUpdateRequested;
	bool                                     m_bStoreCollapasedState;
	bool                                     m_bSameOriginalValues;
	bool									 m_sendingCounter;

	boost::function<void(QColor)>			 m_callback;

	friend class RobloxPropertyWidget;
};

class RobloxPropertyWidget: public QWidget
{
	Q_OBJECT
public:
	RobloxPropertyWidget(QWidget *pParent);
	virtual ~RobloxPropertyWidget();

	void setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel);
	void requestInfoLabelUpdate();

	virtual QSize sizeHint() const;

private Q_SLOTS:
		void updateSelectionInfoLabel();	

private:
	void onInstanceSelectionChanged(const RBX::SelectionChanged& evt);	
	void addInstance(boost::shared_ptr<RBX::Instance> pInstance);
	void removeInstance(boost::shared_ptr<RBX::Instance> pInstance);
	
	/*override*/bool event(QEvent * evt);
	
	boost::shared_ptr<RBX::DataModel>        m_pDataModel;
	rbx::signals::scoped_connection          m_cInstanceSelectionChanged;

	QMutex                                   m_propertyWidgetMutex;

	InstanceList                             m_selectedInstaces;
	//RBX::readwrite_concurrency_catcher       m_selectedInstancesGuard;	

	PropertyTreeWidget                      *m_pTreeWidget;
	QLabel                                  *m_pSelectionInfoLabel;
	QLineEdit                               *m_pFilterEdit;	

	bool                                     m_bUpdateRequested;
};


