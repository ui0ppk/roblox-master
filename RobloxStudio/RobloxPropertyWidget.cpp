/**
 * RobloxPropertyWidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxPropertyWidget.h"

#undef check

// Qt Headers
#include <QApplication>
#include <QPainter>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QSettings>
#include <QDockWidget>
#include <QObject>

// Roblox Headers
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"
#include "rbx/CEvent.h"
#include "ReflectionMetaData.h"
#include "util/RobloxGoogleAnalytics.h"

// Roblox Studio Headers
#include "RobloxContextualHelp.h"
#include "RobloxCustomWidgets.h"
#include "RobloxSettings.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"

FASTFLAG(StudioNewWiki)

FASTFLAGVARIABLE(StudioPropertySearchDisabled, false)
FASTFLAGVARIABLE(StudioPropertyNameBarDisabled, false)
FASTFLAGVARIABLE(StudioPropertyCompareOriginalVals, true)

//utility function to create icons
static QIcon getIndicatorIcon(const QPalette &palette, QStyle *style);

//--------------------------------------------------------------------------------------------
// CategoryItem -- for showing property items in a category
//--------------------------------------------------------------------------------------------
CategoryItem::CategoryItem(const QString &name)
{
	setHidden(true);
	setFlags(Qt::ItemIsEnabled);

	QFont boldFont(font(0));
	boldFont.setBold(true);
	setFont(0, boldFont);

	setText(0, name);
}

void CategoryItem::update(bool storeCollapsedState)
{
	int index = 0, numChildModified = 0, numChild = childCount();	
	PropertyItem *pPropertyItem = NULL;

	while (index < numChild)
	{
		pPropertyItem = static_cast<PropertyItem *>(child(index));
		if (pPropertyItem && pPropertyItem->update())
			++numChildModified;
		++index;
	}

	setHidden(!numChildModified);

	if (storeCollapsedState)
	{
		if (!isHidden() && isExpanded())
			setExpanded(numChildModified);
	}
	else
	{
		setExpanded(numChildModified);
	}
}

void CategoryItem::applyFilter(const QString &filterString)
{
	int index = 0, numChildModified = 0, numChild = childCount();	
	PropertyItem *pPropertyItem = NULL;

	while (index < numChild)
	{
		pPropertyItem = static_cast<PropertyItem *>(child(index));
		if (pPropertyItem && pPropertyItem->applyFilter(filterString))
			++numChildModified;
		++index;
	}

	setHidden(!numChildModified);
}

//--------------------------------------------------------------------------------------------
// PropertyItemDelegate -- for editing/customizing property items
//--------------------------------------------------------------------------------------------
class PropertyItemDelegate: public QStyledItemDelegate
{    
private:
	PropertyTreeWidget    *m_pTreeWidget;

public:
	PropertyItemDelegate(PropertyTreeWidget* parent)
	: QStyledItemDelegate(parent) 
	, m_pTreeWidget(parent)
	{}      

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const 
	{ 
		if (index.column() == 1)
		{
			PropertyItem *pItem = m_pTreeWidget->propertyItemFromIndex(index);
			if (pItem)
				return pItem->createEditor(parent, option);			
		}

		return NULL;
	}

	void setEditorData(QWidget * editor, const QModelIndex & index) const
	{
		if (!editor)
			return;

		//set value in editor
		PropertyItem *pItem = m_pTreeWidget->propertyItemFromIndex(index);
		if (pItem)
			pItem->setEditorData(editor);
	}

	void setModelData(QWidget *editor, QAbstractItemModel*, const QModelIndex &index) const
	{
		if (!editor)
			return;

		//set value from editor
		PropertyItem *pItem = m_pTreeWidget->propertyItemFromIndex(index);
		if (pItem)
			pItem->setModelData(editor);
	}

	bool editorEvent(QEvent * evt, QAbstractItemModel*, const QStyleOptionViewItem&, const QModelIndex & index )
	{
		if (index.column() == 1)
		{
			//set value from editor
			PropertyItem *pItem = m_pTreeWidget->propertyItemFromIndex(index);
			if (pItem)
				return pItem->editorEvent(evt);
		}

		return false;
	}

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
	{	editor->setGeometry(option.rect); }	

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{	return QStyledItemDelegate::sizeHint(option, index) + QSize(3, 4); }

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyleOptionViewItemV4 opt = option;
		bool hasError = false;
		if (PropertyItem* pItem = m_pTreeWidget->propertyItemFromIndex(index))
		{
			opt.textElideMode = pItem->getTextElideMode();
			hasError = pItem->hasError();
		}
	
		QStyledItemDelegate::paint(painter, opt, index);
		opt.palette.setCurrentColorGroup(QPalette::Active);
		QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
		painter->save();
        {
			if (hasError && index.column() == 1)
			{
				painter->setPen(QPen(QColor("red")));				
				painter->drawRect(opt.rect.adjusted(2, 2, -2, -2));
			}

			painter->setPen(QPen(color));
			painter->drawLine(opt.rect.right(), opt.rect.y(), opt.rect.right(), opt.rect.bottom());			
        }
		painter->restore();
	}
};

//--------------------------------------------------------------------------------------------
// PropertyItemUpdateJob -- for updating properties of selected items
//--------------------------------------------------------------------------------------------
class PropertyItemUpdateJob : public RBX::DataModelJob
{	
public:
	typedef enum { SentUpdateMessage, Updating, Waiting } UpdateState;

private:
	PropertyTreeWidget    *m_pPropertyWidget;	
	volatile bool          m_bIsGridInvalid;
	volatile UpdateState   m_state;
	double                 m_waitingPhaseStep;
	double                 m_timeSinceLastRender;

public:
	
	RBX::CEvent update;		// Set when the UI thread may update
	RBX::CEvent updated;	// Set when the UI thread has updated

	
	PropertyItemUpdateJob(PropertyTreeWidget *pPropertyWidget, shared_ptr<RBX::DataModel> dataModel)
	: RBX::DataModelJob("PropGrid", RBX::DataModelJob::Read, false, dataModel, RBX::Time::Interval(0))
	, m_pPropertyWidget(pPropertyWidget)
	, m_bIsGridInvalid(true)
	, m_state(PropertyItemUpdateJob::Waiting)
	, update(false)
	, updated(false)
	{}

	
	void requestUpdate()
	{		
		if (!m_bIsGridInvalid)
		{
			m_bIsGridInvalid = true;
			RBX::TaskScheduler::singleton().reschedule(shared_from_this());	
		}
	}

	void setState(PropertyItemUpdateJob::UpdateState state)
	{ m_state = state; }

	bool isGridInvalid() { return m_bIsGridInvalid; }

	virtual RBX::Time::Interval sleepTime(const Stats&)
	{
		switch (m_state)
		{
		default:
		case Waiting:
			{
				if (m_bIsGridInvalid)
					return RBX::Time::Interval::zero();
				else
					return RBX::Time::Interval::max();
			}

		case Updating:
			return RBX::Time::Interval::zero();

		case SentUpdateMessage:
			return RBX::Time::Interval::max();
		}
	}

	virtual Job::Error error(const Stats& stats)
	{
		switch (m_state)
		{
		default:
		case Waiting:
			{				
				m_waitingPhaseStep = stats.timespanSinceLastStep.seconds();
				Job::Error result;
				result.error = m_waitingPhaseStep;
				return result;
			}

		case Updating:
			{
				Job::Error result;
			    // The error is the sum of the previous phase plus this phase
				result.error = m_waitingPhaseStep + stats.timespanSinceLastStep.seconds();	
				result.urgent = true;
				return result;
			}

		case SentUpdateMessage:
			RBXASSERT(false);
			return Job::Error();
		}
	}

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		m_timeSinceLastRender = m_waitingPhaseStep + stats.timespanOfLastStep.seconds();

		switch (m_state)
		{
		case Waiting:
			m_state = SentUpdateMessage;
			m_pPropertyWidget->requestUpdate();
			break;

		case SentUpdateMessage:
			//RBXASSERT(false);
			break;

		case Updating:
			update.Set();
			updated.Wait();		
			m_bIsGridInvalid = false;
			m_state = Waiting;
			break;
		}

		return RBX::TaskScheduler::Stepped;
	}
};

//--------------------------------------------------------------------------------------------
// PropertyTreeWidget -- collection of property items
//--------------------------------------------------------------------------------------------
PropertyTreeWidget::PropertyTreeWidget(QWidget *pParent)
: QTreeWidget(pParent)
, m_bIsPropertyChanged(false)
, m_bIgnorePropertyChanged(false)
, m_bUpdateRequested(false)
, m_bStoreCollapasedState(false)
, m_bSameOriginalValues(true)
, m_sendingCounter(false)
{
	setUniformRowHeights(true);
	setAlternatingRowColors(true);
	setRootIsDecorated(false);
	setColumnCount(2);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    // set up headers

	QStringList headerLabels;
	headerLabels << "Property" << "Value";
	setHeaderLabels(headerLabels);
    setHeaderHidden(true);

	header()->setStretchLastSection(true);
    header()->setResizeMode(0,QHeaderView::ResizeToContents);

	m_expandIcon = getIndicatorIcon(palette(), style());

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setItemDelegate(new PropertyItemDelegate(this));

	setUpdatesEnabled(false);

	//sort items
	CategoryItems::const_iterator curIter = m_categoryItems.begin();	
	while (curIter != m_categoryItems.end())
	{
		if (curIter->second)
			curIter->second->sortChildren(0, Qt::AscendingOrder);
		++curIter;
	}

    setUpdatesEnabled(true);

	connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(onItemExpanded(QTreeWidgetItem *)));
	connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(onItemCollapsed(QTreeWidgetItem *)));

    connect(this,                                      SIGNAL(helpTopicChanged(const QString&)),
            &RobloxContextualHelpService::singleton(), SLOT(onHelpTopicChanged(const QString&)));
}

PropertyTreeWidget::~PropertyTreeWidget()
{
}

void PropertyTreeWidget::reinitialize()
{
	for (Connections::iterator iter = m_cPropertyChanged.begin(); iter!=m_cPropertyChanged.end(); ++iter)
		iter->second.disconnect();

	m_cPropertyChanged.clear();
	m_invalidItems.clear();
	m_selectionData.clear();
}

void PropertyTreeWidget::setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel)
{
   if ( m_pDataModel == pDataModel )
       return;

	removeUpdateJob();

	//clear selection data and update tree
	m_selectionData.clear();
	m_invalidItems.clear();
	m_bIsPropertyChanged = false;
	m_bIgnorePropertyChanged = false;
	QCoreApplication::removePostedEvents(this);

	m_pDataModel = pDataModel;

	createUpdateJob();
	
	//make sure if the document is deleted then we don't keep showing it's properties
	//WARNING: do not post event as it leads to a thread lock in case user is switching documents
	//calling update should be safe as setDataModel will be called from main thread only
	if (!m_pUpdateItemsJob)
		update();
}

void PropertyTreeWidget::createUpdateJob()
{
	if (!m_pDataModel)
		return;

	m_pUpdateItemsJob.reset(new PropertyItemUpdateJob(this,m_pDataModel));
	RBX::TaskScheduler::singleton().add(m_pUpdateItemsJob);
}

void PropertyTreeWidget::removeUpdateJob()
{
	if (m_pUpdateItemsJob)
	{
		for (Connections::iterator iter = m_cPropertyChanged.begin(); iter!=m_cPropertyChanged.end(); ++iter)
			iter->second.disconnect();
		m_cPropertyChanged.clear();

		m_pUpdateItemsJob->updated.Set();
		RBX::TaskScheduler::singleton().removeBlocking(m_pUpdateItemsJob);
		m_pUpdateItemsJob.reset();		
	}
}

void PropertyTreeWidget::restoreOriginalValues(PropertyItem* item)
{
	if (!item || !item->propertyDescriptor() )
		return;

	RBX::DataModel::LegacyLock lock(getDataModel(), RBX::DataModelJob::Write);

	for (boost::unordered_map<shared_ptr<RBX::Instance>, RBX::Reflection::Variant>::const_iterator iter = m_originalValues.begin(); iter != m_originalValues.end(); ++iter)
		if (iter->first && item->propertyDescriptor()->isMemberOf(iter->first.get()))
			item->propertyDescriptor()->setVariant(iter->first.get(), iter->second);
}

void PropertyTreeWidget::storeOriginalValues(PropertyItem* item)
{
	if (!item || !item->propertyDescriptor() )
		return;

	m_originalValues.clear();
	m_bSameOriginalValues = true;

	if (RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(getDataModel().get()))
	{
		RBX::Instance* prevInstance = NULL;
		for (RBX::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
		{
			if (item->propertyDescriptor()->isMemberOf((*iter).get()))
			{
				RBX::Reflection::Variant value;
				item->propertyDescriptor()->getVariant((*iter).get(), value);

				m_originalValues.insert(std::make_pair(*iter, value));

				if (FFlag::StudioPropertyCompareOriginalVals)
				{
					if (prevInstance && m_bSameOriginalValues)
						m_bSameOriginalValues = item->propertyDescriptor()->equalValues(prevInstance, (*iter).get());
					prevInstance = (*iter).get();
				}
			}
		}
	}

}

void PropertyTreeWidget::addInstance(RBX::Instance *pInstance)
{
	//first remove (to be sure we don't put in duplicate values)
	m_selectionData[&pInstance->getDescriptor()].remove(pInstance);
	//now add
	m_selectionData[&pInstance->getDescriptor()].push_back(pInstance);
	
	m_cPropertyChanged[pInstance] = pInstance->propertyChangedSignal.connect(boost::bind(&PropertyTreeWidget::onPropertyChanged, this, _1));

	m_bIgnorePropertyChanged = true;
	
	//update property view
	if (m_pUpdateItemsJob)
		m_pUpdateItemsJob->requestUpdate();
	else
		requestUpdate();
}

void PropertyTreeWidget::removeInstance(RBX::Instance *pInstance)
{	
	//remove connection
	Connections::iterator conIter = m_cPropertyChanged.find(pInstance);
	if (conIter != m_cPropertyChanged.end())
	{
		conIter->second.disconnect();
		m_cPropertyChanged.erase(conIter);
	}

	//remove instance
	ClassDescriptorInstanceMap::iterator iter = m_selectionData.find(&pInstance->getDescriptor());
	if (iter != m_selectionData.end())
	{
		iter->second.remove(pInstance);
		if (iter->second.empty())
			m_selectionData.erase(iter);
	}

	m_bIgnorePropertyChanged = true;

	//update property view
	if (m_pUpdateItemsJob)
		m_pUpdateItemsJob->requestUpdate();
	else
		requestUpdate();
}

void PropertyTreeWidget::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	QMutexLocker lock(&m_propModifyMutex);

	if (m_bIgnorePropertyChanged)
		return;
	
	PropertyItemMap::const_iterator iter = m_itemMap.find(pPropertyDescriptor);
	if (iter != m_itemMap.end())
	{
		m_invalidItems.insert(iter->second);
		m_bIsPropertyChanged = true;
		//update property view
		if (m_pUpdateItemsJob)
		{
			//data model job will take care of avoiding sending duplicate events (so no need to check for update requested)
			m_pUpdateItemsJob->requestUpdate();
		}
		else
		{
			QMutexLocker lock(&m_propUpdateMutex);
			if (!m_bUpdateRequested)
			{	
				requestUpdate();
				m_bUpdateRequested = true;
			}
		}
	}
}

void PropertyTreeWidget::requestUpdate()
{
	//qt will take ownership of the event so no need to delete
	QApplication::postEvent(this, new RobloxCustomEvent(PROPERTY_WIDGET_UPDATE));
}

bool PropertyTreeWidget::event(QEvent* evt)
{
    bool retVal = false;

	if ( evt->type() == PROPERTY_WIDGET_UPDATE )
    {
		update();
		QMutexLocker lock(&m_propUpdateMutex);
		m_bUpdateRequested = false;
        retVal = true;
    }
    else if ( evt->type() == QEvent::ShortcutOverride )
    {
        retVal = QTreeWidget::event(evt);
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
        QChar key = keyEvent->key();

        // capture single key alpha numerical presses - we want these to be considered
        //  as shortcuts in the tree to jumped to items starting with that key, not global shortcuts
        if ( !(keyEvent->key() >= Qt::Key_F1 && keyEvent->key() <= Qt::Key_F35) && 
             key.isLetterOrNumber() && 
             keyEvent->modifiers() == Qt::NoModifier )
        {
            keyEvent->accept();
            retVal = true;
        }
    }
    else
    {
        retVal = QTreeWidget::event(evt);
    }

	return retVal;
}

void PropertyTreeWidget::setPropertyCallback(const QObject* sender, const char* signal, boost::function<void(QColor)> callback)
{
	m_callback = callback;
	connect(sender, signal, this, SLOT(updatePropertySlot(QColor)));
}

void PropertyTreeWidget::resetPropertyCallback()
{
	disconnect(this, SLOT(updatePropertySlot(QColor)));
	m_callback.clear();
}

void PropertyTreeWidget::updatePropertySlot(QColor objectValue)
{
	m_callback(objectValue);
}

void PropertyTreeWidget::update()
{
	if (m_pUpdateItemsJob)
	{
		if (!m_pUpdateItemsJob->isGridInvalid())
			return;

		m_pUpdateItemsJob->setState(PropertyItemUpdateJob::Updating);
		RBX::TaskScheduler::singleton().reschedule(m_pUpdateItemsJob);
		m_pUpdateItemsJob->update.Wait();		
	}

	setUpdatesEnabled(false);

	if (m_itemMap.empty()) 
	{
		visitClassProperties(&RBX::Reflection::ClassDescriptor::rootDescriptor());

        // create categories for items (if we don't precreate the loop below has O(N^2) layout?)
		for (auto& item: m_items)
			getCategoryItem(item);

        // add items
		for (auto& item: m_itemMap)
		{
			CategoryItem* pCategoryItem = getCategoryItem(item.first);

			if (pCategoryItem)
				pCategoryItem->addChild(item.second);
		}

		// sort items
		CategoryItems::const_iterator curIter = m_categoryItems.begin();	
		while (curIter != m_categoryItems.end())
		{
			if (curIter->second)
				curIter->second->sortChildren(0, Qt::AscendingOrder);
			++curIter;
		}
	}
	
	if (m_bIsPropertyChanged && !m_bIgnorePropertyChanged)
	{
		std::set<PropertyItem*>  tempInvalidItems = m_invalidItems;
		m_invalidItems.clear();
		std::for_each(tempInvalidItems.begin(), 
			          tempInvalidItems.end(), 
			          boost::bind(&PropertyItem::update, _1));
		m_bIsPropertyChanged = false;
	}
	else
	{
		CategoryItems::const_iterator curIter = m_categoryItems.begin();
		CategoryItems::const_iterator endIter = m_categoryItems.end();

		while (curIter != endIter)
		{
			curIter->second->update(m_bStoreCollapasedState);
			++curIter;
		}

		m_bIgnorePropertyChanged = false;
	}

	setUpdatesEnabled(true);	

	if (m_pUpdateItemsJob)
		m_pUpdateItemsJob->updated.Set();	
}

void PropertyTreeWidget::visitClassProperties(const RBX::Reflection::ClassDescriptor* pClassDescriptor)
{
	//visit classes
	std::for_each(pClassDescriptor->derivedClasses_begin(), 
		          pClassDescriptor->derivedClasses_end(), 
				  boost::bind(&PropertyTreeWidget::visitClassProperties, this, _1));

	//initialize properties
	std::for_each(pClassDescriptor->begin<RBX::Reflection::PropertyDescriptor>(), 
		          pClassDescriptor->end<RBX::Reflection::PropertyDescriptor>(), 
				  boost::bind(&PropertyTreeWidget::initProperty, this, pClassDescriptor, _1));
}

void PropertyTreeWidget::initProperty(const RBX::Reflection::ClassDescriptor* pClassDescriptor, 
										  const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	if (!pPropertyDescriptor || !pClassDescriptor)
		return;

	// Ignore inherited properties
	if (pPropertyDescriptor->owner!=*pClassDescriptor)
		return;

	RBX::Reflection::Metadata::Member* pMember = RBX::Reflection::Metadata::Reflection::singleton()->get(*pPropertyDescriptor);

    // only show public, editable properties
    if ( !pPropertyDescriptor->isPublic() || 
         !pPropertyDescriptor->isEditable() || 
			(pPropertyDescriptor->security != RBX::Security::None && 
				!(RBX::Reflection::Metadata::Item::isBrowsable(pMember, *pPropertyDescriptor)) 
			)
		)
	{
		return;
    }

	if(RBX::Reflection::Metadata::Item::isDeprecated(pMember, *pPropertyDescriptor))
		return;
	PropertyItem *pItem = PropertyItem::createItem(pPropertyDescriptor);
	if (!pItem)
		return;

    m_items.push_back(pPropertyDescriptor);
	m_itemMap[pPropertyDescriptor] = pItem;
}

void PropertyTreeWidget::closeEditor(QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{	QTreeWidget::closeEditor(editor, hint); }

PropertyItem* PropertyTreeWidget::propertyItemFromIndex(const QModelIndex &index) const
{	return dynamic_cast<PropertyItem *>(itemFromIndex(index)); }

CategoryItem* PropertyTreeWidget::getCategoryItem(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	QString categoryName = pPropertyDescriptor->category.toString().c_str();
	CategoryItems::const_iterator iter = m_categoryItems.find(categoryName);
	if (iter != m_categoryItems.end())
		return iter->second;

	CategoryItem *pItem = new CategoryItem(categoryName);
	addTopLevelItem(pItem);
	setFirstItemColumnSpanned(pItem, true);
	pItem->setIcon(0, m_expandIcon);
	static QString collapsedCategoryItems("Surface Inputs"); //this will collapse both Surface and Surface Inputs
	pItem->setExpanded(m_bStoreCollapasedState ? (!collapsedCategoryItems.contains(categoryName) && RobloxSettings().value(getRegistryKey(pItem), true).toBool()) : true);

	m_categoryItems[categoryName] = pItem;

	return pItem;
}

void PropertyTreeWidget::sendPropertySearchCounter()
{
	m_sendingCounter = false;
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "StudioUserAction", "PropertySearchUsed");
}

void PropertyTreeWidget::setFilter(QString filter)
{
	setUpdatesEnabled(false);

	CategoryItems::const_iterator curIter = m_categoryItems.begin();
	CategoryItems::const_iterator endIter = m_categoryItems.end();

	while (curIter != endIter)
	{
		if (curIter->second)
			curIter->second->applyFilter(filter);
		++curIter;
	}

	setUpdatesEnabled(true);

	if (!m_sendingCounter)
	{
		m_sendingCounter = true;
		QTimer::singleShot(1000, this, SLOT(sendPropertySearchCounter()));
	}
}

void PropertyTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QTreeWidget::mousePressEvent(event);

    QTreeWidgetItem* treeWidgetItem = itemAt(event->pos());
    
    QTreeWidgetItem* categoryItem = dynamic_cast<CategoryItem *>(treeWidgetItem);
	if ( categoryItem ) 
	{
	    if ( event->pos().x() + header()->offset() < 20 )
		    categoryItem->setExpanded(!categoryItem->isExpanded());
    }
    else
    {
        if (treeWidgetItem)
		{
			if(FFlag::StudioNewWiki)
			{
				QTreeWidgetItem* curr = treeWidgetItem;
				QString wikiQString;
				bool done = false;

				while (!done) {
					if(curr == invisibleRootItem()) {
						done = true;
					}
					PropertyItem* pi = dynamic_cast<PropertyItem*>(curr);
					const RBX::Reflection::PropertyDescriptor* desc = pi->propertyDescriptor();
					if(desc) {
						done = true;
						std::string wikiString = "API:Class/";
						wikiString += desc->owner.name.str + "/";
						wikiQString = pi->text(0).prepend(wikiString.c_str());
					} else {
						curr = curr->parent();
					}
				}
				if (!wikiQString.isEmpty()) {
					Q_EMIT(helpTopicChanged(wikiQString));
				}
			}
			else
			{
				Q_EMIT(helpTopicChanged(treeWidgetItem->text(0) + "_(Property)"));
			}
		}

		bool handled = false;
		if (PropertyItem* propertyItem = dynamic_cast<PropertyItem*>(treeWidgetItem))
		{
			handled = propertyItem->customLaunchEditorHook(event);
		}

		if (!handled)
		{
			setCurrentIndex(indexFromItem(treeWidgetItem, 1));
			editItem(treeWidgetItem,1);
		}
	}
}

void PropertyTreeWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;

	PropertyItem *pItem = propertyItemFromIndex(index);
	if (!pItem)
	{
		const QColor c = QColor(233, 233, 233);
        painter->fillRect(option.rect, c);
        opt.palette.setColor(QPalette::AlternateBase, c);
	}
	else if (!(pItem->flags() & Qt::ItemIsEditable))
	{
		opt.palette.setColor(QPalette::Text, QColor("gray"));
	}

	QTreeWidget::drawRow(painter, opt, index);
	QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));

    painter->save();
    {
        painter->setPen(QPen(color));
        painter->drawLine(opt.rect.x(), opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    }
    painter->restore();
}

void PropertyTreeWidget::onItemExpanded(QTreeWidgetItem * pItem)
{
	if (m_bStoreCollapasedState)
	{
		//don't store unnecessary entires, on expansion remove since it's the default value
		CategoryItem *pCategoryItem = dynamic_cast<CategoryItem *>(pItem);
		if (pCategoryItem)
			RobloxSettings().remove(getRegistryKey(pCategoryItem));
	}
}

void PropertyTreeWidget::onItemCollapsed(QTreeWidgetItem * pItem)
{
	if (m_bStoreCollapasedState)
	{
		CategoryItem *pCategoryItem = dynamic_cast<CategoryItem *>(pItem);
		if (pCategoryItem)
			RobloxSettings().setValue(getRegistryKey(pCategoryItem), false);
	}
}

QString PropertyTreeWidget::getRegistryKey(CategoryItem* pCategoryItem)
{
	if (pCategoryItem)
		return parent()->objectName() + "_" + pCategoryItem->text(0);
	return QString("");
}

//--------------------------------------------------------------------------------------------
// RobloxPropertyWidget -- main widget for showing properties of selected items
//--------------------------------------------------------------------------------------------
RobloxPropertyWidget::RobloxPropertyWidget(QWidget *pParent)
: QWidget(pParent)
, m_bUpdateRequested(false)
{
	setObjectName("RobloxPropertyWidget");
	QGridLayout *pMainGridLayout = new QGridLayout(this);
	pMainGridLayout->setVerticalSpacing(4);
	pMainGridLayout->setHorizontalSpacing(0);
    pMainGridLayout->setContentsMargins(0, 0, 0, 0);

	QFrame *pFrame = new QFrame(this);
	pFrame->setFrameShape(QFrame::Box);
    pFrame->setFrameShadow(QFrame::Raised);
	pFrame->setVisible(!FFlag::StudioPropertyNameBarDisabled);
	m_pSelectionInfoLabel = new QLabel(pFrame);
	m_pSelectionInfoLabel->setTextFormat(Qt::PlainText);
	QSpacerItem *pHorizontalSpacer = new QSpacerItem(20, 18, QSizePolicy::Expanding, QSizePolicy::Minimum);
	QGridLayout *pFrameGridLayout = new QGridLayout(pFrame);
	QMargins margins = pFrameGridLayout->contentsMargins();
	margins.setTop(0);
	margins.setBottom(0);
    pFrameGridLayout->setContentsMargins(margins);
	pFrameGridLayout->setHorizontalSpacing(4);
	pFrameGridLayout->addWidget(m_pSelectionInfoLabel, 0, 0, 1, 1);
	pFrameGridLayout->addItem(pHorizontalSpacer, 0, 1, 1, 1);

	pMainGridLayout->addWidget(pFrame, 0, 0, 1, 3);

	/*
	m_pSortItems = new QPushButton(this);
	m_pSortItems->setText("Sort");
	m_pSortItems->setCheckable(true);
	*/
	m_pFilterEdit = new QLineEdit(this);
	m_pFilterEdit->setPlaceholderText("Search Properties");	
	m_pFilterEdit->setVisible(!FFlag::StudioPropertySearchDisabled);
	pMainGridLayout->addWidget(m_pFilterEdit, 1, 0, 1, 1);
	
	m_pTreeWidget = new PropertyTreeWidget(this);
	m_pTreeWidget->setStoreCollapsedState(true);
	pMainGridLayout->addWidget(m_pTreeWidget, 2, 0, 1, 3);

	connect(m_pFilterEdit, SIGNAL(textChanged(QString)), m_pTreeWidget, SLOT(setFilter(QString)));
}

RobloxPropertyWidget::~RobloxPropertyWidget()
{
}

QSize RobloxPropertyWidget::sizeHint() const 
{
	return QSize(295, UpdateUIManager::Instance().getMainWindow().size().height() / 2);
}

void RobloxPropertyWidget::setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel)
{
	if ( m_pDataModel == pDataModel )
       return;

	//first disconnect the connection
	m_cInstanceSelectionChanged.disconnect();
	m_selectedInstaces.clear();

	m_pDataModel = pDataModel;
	m_pTreeWidget->setDataModel(m_pDataModel);
	
	if (m_pDataModel)
	{
		//check if we need to connect again
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
		RBX::Selection* pSelection = m_pDataModel->create<RBX::Selection>();
		if (pSelection)
		{
			std::for_each(pSelection->begin(), pSelection->end(), boost::bind(&RobloxPropertyWidget::addInstance, this, _1));
			m_cInstanceSelectionChanged = pSelection->selectionChanged.connect(boost::bind(&RobloxPropertyWidget::onInstanceSelectionChanged, this, _1));
		}
	}

	requestInfoLabelUpdate();
}

void RobloxPropertyWidget::addInstance(boost::shared_ptr<RBX::Instance> pInstance)
{
	if (!pInstance)
		return;
	
	//RBX::readwrite_concurrency_catcher::scoped_write_request lock(m_selectedInstancesGuard);
	m_selectedInstaces.push_back(pInstance.get());
	m_pTreeWidget->addInstance(pInstance.get());
}

void RobloxPropertyWidget::removeInstance(boost::shared_ptr<RBX::Instance> pInstance)
{
	if (!pInstance)
		return;
	
	//RBX::readwrite_concurrency_catcher::scoped_write_request lock(m_selectedInstancesGuard);
	InstanceList::iterator iter = std::find(m_selectedInstaces.begin(), m_selectedInstaces.end(), pInstance.get());
	if (iter != m_selectedInstaces.end())
		m_selectedInstaces.erase(iter);
	m_pTreeWidget->removeInstance(pInstance.get());
}

void RobloxPropertyWidget::onInstanceSelectionChanged(const RBX::SelectionChanged& evt)
{
	QMutexLocker lock(&m_propertyWidgetMutex);
	if (evt.addedItem)
	{
		addInstance(evt.addedItem);
	}

	if (evt.removedItem)
	{
		removeInstance(evt.removedItem);
	}

	//remove the filter on selection change
	if (!m_pFilterEdit->text().isEmpty())
		m_pFilterEdit->clear();

	requestInfoLabelUpdate();
}

void RobloxPropertyWidget::requestInfoLabelUpdate()
{
	if (m_bUpdateRequested)
		return;
	//qt will take ownership of the event so no need to delete
	QApplication::postEvent(this, new RobloxCustomEvent(PROPERTY_LABEL_UPDATE));
	m_bUpdateRequested = true;
}

bool RobloxPropertyWidget::event(QEvent * evt)
{
	if (evt->type() != PROPERTY_LABEL_UPDATE)
		return QWidget::event(evt);

	updateSelectionInfoLabel();
	m_bUpdateRequested = false;

	return true;
}

void RobloxPropertyWidget::updateSelectionInfoLabel()
{
	QMutexLocker mutexLock(&m_propertyWidgetMutex);

	//if no selection just clear the label
	if (m_selectedInstaces.empty())
	{
		if (FFlag::StudioPropertyNameBarDisabled)
			dynamic_cast<QDockWidget *>(this->parent())->setWindowTitle("Properties");
		m_pSelectionInfoLabel->setText("");	
		return;
	}
	
	int numInstances = m_selectedInstaces.size();
	InstanceList::const_iterator curIter = m_selectedInstaces.begin();
	InstanceList::const_iterator endIter = m_selectedInstaces.end();

	//RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Read);

	std::string instanceName   =  (*curIter)->getName();
	const RBX::Name* className = &(*curIter)->getClassName();

	QString labelText;
	if (numInstances == 1)
	{
		labelText = QString("%1 \"%2\"").arg(className ? className->toString().c_str() : "").arg(instanceName.c_str());
	}	
	else
	{
		while (++curIter != endIter)
		{
			if ((*curIter)->getName() != instanceName)
			{
				instanceName.clear();
				if (className==NULL)
					break;
			}

			if ((*curIter)->getClassName() != *className)
			{
				className = NULL;
				if (instanceName.empty())
					break;
			}
		}

		//modify label appropriately
		if (className != NULL && !instanceName.empty())
			labelText = QString("%1 \"%2\" - %3 items").arg(className->toString().c_str()).arg(instanceName.c_str()).arg(numInstances);
		else if (className != NULL)
			labelText = QString("%1 - %2 items").arg(className->toString().c_str()).arg(numInstances);
		else
			labelText = QString("%1 items").arg(numInstances);
	}

	//elide text if required
	QFontMetrics fm = fontMetrics();
	m_pSelectionInfoLabel->setText( fm.elidedText(labelText, Qt::ElideRight, width()-24) );

	if (FFlag::StudioPropertyNameBarDisabled)
		dynamic_cast<QDockWidget *>(this->parent())->setWindowTitle(QString("Properties - %1").arg(labelText));
}

// Draw an icon indicating open/closed branches
static QIcon getIndicatorIcon(const QPalette &palette, QStyle *style)
{
    QPixmap pix(14, 14);
    pix.fill(Qt::transparent);
    QStyleOptionViewItemV4 branchOption;
    QRect r(QPoint(0, 0), pix.size());
    branchOption.rect = QRect(2, 2, 9, 9);
    branchOption.palette = palette;
    branchOption.state = QStyle::State_Children;

    QPainter p;
    // closed state
    p.begin(&pix);
    style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p);
    p.end();
    QIcon rc = pix;
    rc.addPixmap(pix, QIcon::Selected, QIcon::Off);
    // open state
    branchOption.state |= QStyle::State_Open;
    pix.fill(Qt::transparent);
    p.begin(&pix);
    style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p);
    p.end();

    rc.addPixmap(pix, QIcon::Normal, QIcon::On);
    rc.addPixmap(pix, QIcon::Selected, QIcon::On);
    return rc;
}
