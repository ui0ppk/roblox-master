/**
 * RobloxTreeWidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxTreeWidget.h"

// Qt Headers
#include <QApplication>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QMenu>
#include <QScrollBar>
#include <QLineEdit>
#include <QMetaObject>
#include <QMovie>
#include <QStyledItemDelegate>

// Roblox Headers
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/ChangeHistory.h"
#include "v8datamodel/GuiObject.h"
#include "Script/ModuleScript.h"
#include "script/script.h"
#include "util/ScopedAssign.h"
#include "util/BrickColor.h"
#include "ReflectionMetaData.h"
#include "v8datamodel/PlayerGui.h"

// Roblox Studio Headers
#include "CommonInsertWidget.h"
#include "LuaSourceBuffer.h"
#include "QtUtilities.h"
#include "RobloxMainWindow.h"
#include "RobloxContextualHelp.h"
#include "RobloxCustomWidgets.h"
#include "RobloxDocManager.h" 
#include "UpdateUIManager.h"
#include "RobloxIDEDoc.h"
#include "boost/algorithm/string.hpp"
#include "util/RobloxGoogleAnalytics.h"
#include "RobloxSettings.h"

const static std::string classSearchString = "classname:";
const static std::string nameSearchString = "name:";

static int nearestWidgetItemIndex(RobloxTreeWidgetItem *pParentTWI, int first, int last, int key);
static QItemSelection mergeModelIndexes(const QList<QModelIndex> &indexes);

FASTINTVARIABLE(StudioTreeWidgetProcessingTime, 100)
FASTINTVARIABLE(StudioTreeWidgetFilterTime, 30)
FASTINTVARIABLE(StudioTreeWidgetEventProcessingTime, 10)

FASTFLAGVARIABLE(StudioDE8774CrashFixEnabled, false)
FASTFLAGVARIABLE(StudioPushTreeWidgetUpdatesToMainThread, false)
FASTFLAGVARIABLE(StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates, false)

FASTFLAG(StudioNewWiki)
FASTFLAG(StudioMimeDataContainsInstancePath)
FASTFLAG(StudioSeparateActionByActivationMethod)
FASTFLAG(TeamCreateOptimizeRemoteSelection)

LOGGROUP(Explorer)

bool DepthCompare::operator() (RobloxTreeWidgetItem* lhs, RobloxTreeWidgetItem* rhs) const
{
	if (lhs->getTreeWidgetDepth() == rhs->getTreeWidgetDepth())
		return lhs < rhs;

	return rhs->getTreeWidgetDepth() < lhs->getTreeWidgetDepth();
}

//--------------------------------------------------------------------------------------------
// TreeWidgetDelegate
//--------------------------------------------------------------------------------------------
class TreeWidgetDelegate: public QStyledItemDelegate
{
public:
	TreeWidgetDelegate(QObject* parent)
		: QStyledItemDelegate(parent)
	{

	}

	enum Role
	{
		OtherUsersHighlightingRole = Qt::UserRole+1,
	};

private:
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		return QStyledItemDelegate::sizeHint(option, index);
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		// do the normal paint
		QStyledItemDelegate::paint(painter, option, index);

		// now paint other users selection
		// TODO: once we finalize the display, get widget width and add "..."
		QRect newRect(option.rect.topRight(), QSize(15, option.rect.height()));
		PlayersColorCollection playersData = index.data(OtherUsersHighlightingRole).value<PlayersColorCollection >();
		for (PlayersColorCollection::const_iterator iter = playersData.constBegin(); iter != playersData.constEnd(); ++iter)
		{
			painter->fillRect(newRect, QtUtilities::toQColor(RBX::BrickColor(iter.value()).color3()));
			newRect.translate(15, 0);
		}
	}
};

InstanceUpdateHandler::InstanceUpdateHandler(boost::shared_ptr<RBX::Instance> pInstance)
: m_pInstance(pInstance)
{
	// Attach listeners
	if (FFlag::StudioPushTreeWidgetUpdatesToMainThread)
	{
		m_cChildAddedConnection   = m_pInstance->getOrCreateChildAddedSignal()->connect(boost::bind(&InstanceUpdateHandler::childAddedSignalHandler, this, _1));
		m_cChildRemovedConnection = m_pInstance->getOrCreateChildRemovedSignal()->connect(boost::bind(&InstanceUpdateHandler::childRemovedSignalHandler, this, _1));
		m_cPropertyChangedConnection = m_pInstance->propertyChangedSignal.connect(boost::bind(&InstanceUpdateHandler::propertyChangedSignalHandler, this, _1));
	}
	else
	{
		m_cChildAddedConnection   = m_pInstance->getOrCreateChildAddedSignal()->connect(boost::bind(&InstanceUpdateHandler::onChildAdded, this, _1));
		m_cChildRemovedConnection = m_pInstance->getOrCreateChildRemovedSignal()->connect(boost::bind(&InstanceUpdateHandler::onChildRemoved, this, _1));
		m_cPropertyChangedConnection = m_pInstance->propertyChangedSignal.connect(boost::bind(&InstanceUpdateHandler::onPropertyChanged, this, _1));
	}
}

InstanceUpdateHandler::~InstanceUpdateHandler()
{	
	m_cChildRemovedConnection.disconnect();
	m_cChildAddedConnection.disconnect();
	m_cPropertyChangedConnection.disconnect();

	m_PendingItemsToAdd.clear();
}

void InstanceUpdateHandler::populateChildren(RobloxTreeWidget* pTreeWidget)
{
	shared_ptr<RBX::Instance> inst = getInstance();

	RobloxTreeWidgetItem* pParent = getItemParent();

	for (size_t i = 0; i < inst->numChildren(); ++i)
	{
		if (RBX::Instance* pChildInst = inst->getChild(i))
		{
			RobloxTreeWidgetItem* pChildItem = pTreeWidget->findItemFromInstance(pChildInst);

			if (pChildItem)
			{
				bool currentlySelected = false;

				if (RBX::Selection* selection = RBX::ServiceProvider::find<RBX::Selection>(getInstance().get()))
				{
					for (RBX::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
					{
						if ((*iter).get() == pChildInst)
						{
							currentlySelected = true;
							break;
						}
					}
				}

				if (pParent)
				{
					int index = pParent->getIndexToInsertAt(shared_from(pChildInst));
					pParent->insertChild(index < 0 ? 0 : index, pChildItem);
				}
				else if (pTreeWidget)
				{
					int index = pTreeWidget->getIndexToInsertAt(shared_from(pChildInst));
					pTreeWidget->insertTopLevelItem(index < 0 ? 0 : index, pChildItem);
				}

				if (currentlySelected)
					pTreeWidget->addItemToSelection(pChildInst);

				pChildItem->populateChildren(pTreeWidget);
			}
		}
	}
}

bool InstanceUpdateHandler::onChildAdded(shared_ptr<RBX::Instance> pChild)
{
	FASTLOG1(FLog::Explorer, "InstanceUpdateHander::onChildAdded, child: %p", pChild.get());

	if (!pChild || !RobloxTreeWidgetItem::isExplorerItem(pChild))
		return false;

	FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

	RobloxTreeWidget *pTreeWidget = getTreeWidget();
	if (!pTreeWidget || pTreeWidget->isDeletionRequested())
		return false;

	RobloxTreeWidgetItem *pTreeWidgetParentItem = pTreeWidget->findItemFromInstance(pChild->getParent());
	
	if (pTreeWidgetParentItem)
		pTreeWidgetParentItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

	QMutexLocker lock(pTreeWidget->treeWidgetMutex());

	if (RobloxIDEDoc::getIsCloudEditSession() && RBX::Security::Context::current().identity == RBX::Security::Replicator_)
		pTreeWidget->markLocationToSuppressScrolling();

	RobloxTreeWidgetItem* pChildItem = pTreeWidget->findItemFromInstance(pChild.get());

	if (pChildItem && (pTreeWidgetParentItem || pTreeWidget))
	{
		pTreeWidget->removeItemFromRemovalList(pChildItem);
		bool currentlySelected = pChildItem->removeItemFromTreeWidget();

		if (pTreeWidgetParentItem)
		{
			int index = pTreeWidgetParentItem->getIndexToInsertAt(pChild);
			pTreeWidgetParentItem->insertChild(index < 0 ? 0 : index, pChildItem);
		}
		else if (pTreeWidget)
		{
			int index = pTreeWidget->getIndexToInsertAt(pChild);
			pTreeWidget->insertTopLevelItem(index < 0 ? 0 : index, pChildItem);
		}

		pChildItem->setExpandedOnce(false);

		if (currentlySelected)
			pTreeWidget->addItemToSelection(pChild.get());

	}
	else
	{
		m_PendingItemsToAdd.insert(pChild);
	}
	
	if (pTreeWidgetParentItem && pTreeWidgetParentItem->isExpanded())
	{
		pTreeWidgetParentItem->updateFilterItems();
	}
	pTreeWidget->addToUpdateList(pTreeWidgetParentItem);
	return true;
}

bool InstanceUpdateHandler::onChildRemoved(shared_ptr<RBX::Instance> pChild)
{
	FASTLOG1(FLog::Explorer, "InstanceUpdateHander::onChildRemoved, child: %p", pChild.get());
	if (!pChild)
		return false;

	FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

	RobloxTreeWidget *pTreeWidget = getTreeWidget();
	if (!pTreeWidget || pTreeWidget->isDeletionRequested())
		return false;

	QMutexLocker lock(pTreeWidget->treeWidgetMutex());
	m_PendingItemsToAdd.erase(pChild);
	m_FilterItemsToAdd.erase(pChild);
	RobloxTreeWidgetItem *pTreeWidgetItem = pTreeWidget->findItemFromInstance(pChild.get());
	if (pTreeWidgetItem)
	{
		pTreeWidget->requestItemDelete(pTreeWidgetItem);

		if (pTreeWidgetItem == pTreeWidget->lastSelectedItem())
			pTreeWidget->eraseLastSelectedItem();

        if (RobloxIDEDoc::getIsCloudEditSession())
		{
			if (RBX::Security::Context::current().identity == RBX::Security::Replicator_)
				pTreeWidget->markLocationToSuppressScrolling();
			pTreeWidget->itemDeletionRequested(pTreeWidgetItem);
		}
	}

	return true;
}

void InstanceUpdateHandler::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
	bool isNameChanged = (*pDescriptor==RBX::Instance::desc_Name);
	if (isNameChanged)
	{
		RobloxTreeWidget *pTreeWidget = getTreeWidget();
		if (pTreeWidget && !pTreeWidget->isDeletionRequested())
		{
			QMutexLocker lock(pTreeWidget->treeWidgetMutex());
			processPropertyChange();
		}
	}
	else if (*pDescriptor==RBX::Instance::propParent)
	{
		if (RobloxTreeWidget* pTreeWidget = getTreeWidget())
			if (!pTreeWidget->isFilterEmpty() && getInstance()->getParent())
				pTreeWidget->filterWidget();
	}
}

RobloxTreeWidgetItem* InstanceUpdateHandler::processChildAdd(shared_ptr<RBX::Instance> pInstance)
{
	RobloxTreeWidget *pTreeWidget = getTreeWidget();
	RobloxTreeWidgetItem *pWidgetItem = NULL;

	if (pTreeWidget->findItemFromInstance(pInstance.get()))
		return NULL;
	
	if (getItemParent())
	{
		int index = getItemParent()->getIndexToInsertAt(pInstance);			
		pWidgetItem = new RobloxTreeWidgetItem((index > 0) ? index : 0, getItemParent(), pInstance);			
	}
	else
	{
		int index = pTreeWidget->getIndexToInsertAt(pInstance);
		pWidgetItem = new RobloxTreeWidgetItem((index > 0) ? index : 0, pTreeWidget, pInstance);
	}

	if (pWidgetItem && pInstance->numChildren() > 0)
		pWidgetItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

	if (pWidgetItem)
		pWidgetItem->setViewState();

	pTreeWidget->updateSelectionState(pInstance.get());
	
	m_FilterItemsToAdd.erase(pInstance);
	m_PendingItemsToAdd.erase(pInstance);

	return pWidgetItem;
}

bool InstanceUpdateHandler::processChildrenAdd()
{
	RobloxTreeWidget *pTreeWidget = getTreeWidget();
	RBXASSERT(pTreeWidget);
	if (!pTreeWidget)
		return false;

	InstanceList_SPTR* instanceList = pTreeWidget->isFilterEmpty() ? &m_PendingItemsToAdd : &m_FilterItemsToAdd;

	if (instanceList->empty())
		return false;

	bool itemsAdded = false;

	//Lock for a few items
	QMutexLocker lock(pTreeWidget->treeWidgetMutex());
	// Do a few items
	for ( int i = 0 ; i < 10 ; ++i )
	{
		InstanceList_SPTR::iterator iter = instanceList->begin();
		if (iter == instanceList->end())
			return itemsAdded;
		shared_ptr<RBX::Instance> pInstance = *iter;

		processChildAdd(pInstance);

		itemsAdded = true;
	}
	return itemsAdded;
}

void InstanceUpdateHandler::removeFromRemovalList(RobloxTreeWidgetItem *pTreeWidgetItem)
{
	m_PendingItemsToRemove.erase(pTreeWidgetItem);

	int numChild = pTreeWidgetItem->childCount();
	if (!numChild)
		return;

	int currentChild = 0;
	RobloxTreeWidgetItem *pCurrentItem = NULL;

	while (currentChild < numChild)
	{		
		pCurrentItem = static_cast<RobloxTreeWidgetItem*>(pTreeWidgetItem->child(currentChild));
		if (pCurrentItem)
			removeFromRemovalList(pCurrentItem);
		++currentChild;
	}
}

void InstanceUpdateHandler::processChildRemove()
{
	if (!m_PendingItemsToRemove.size())
		return;

	RobloxTreeWidget *pTreeWidget = getTreeWidget();

	if (FFlag::StudioDE8774CrashFixEnabled)
	{
		QMutexLocker lock(pTreeWidget->treeWidgetMutex());

		while (m_PendingItemsToRemove.begin() != m_PendingItemsToRemove.end())
		{
			RobloxTreeWidgetItem *pTreeWidgetItem = *m_PendingItemsToRemove.begin();
			if (!pTreeWidgetItem)
				continue;

			pTreeWidgetItem->aboutToDelete(pTreeWidget);
			removeFromRemovalList(pTreeWidgetItem);

			delete pTreeWidgetItem;
		}
	}
	else
	{
		std::vector<RobloxTreeWidgetItem*> itemsToRemove;

		// Copy the items to a temp collection
		{
			QMutexLocker lock(pTreeWidget->treeWidgetMutex());
			for (TreeWidgetItemList::iterator iter = m_PendingItemsToRemove.begin(); iter != m_PendingItemsToRemove.end(); ++iter)
				itemsToRemove.push_back(*iter);
			m_PendingItemsToRemove.clear();
		}

		// Process them
		RobloxTreeWidgetItem *pTreeWidgetItem = NULL;
		for (std::vector<RobloxTreeWidgetItem*>::iterator iter = itemsToRemove.begin(); iter!=itemsToRemove.end(); ++iter)
		{
			pTreeWidgetItem = *iter;
			if (!pTreeWidgetItem)
				continue;
			pTreeWidgetItem->aboutToDelete(pTreeWidget);
			//if (pTreeWidgetItem->parent())
			//	pTreeWidgetItem->parent()->removeChild(pTreeWidgetItem);
			delete pTreeWidgetItem;
		}
	}
}

RobloxTreeWidgetItem::RobloxTreeWidgetItem(int index, RobloxTreeWidget *pTreeWidget, boost::shared_ptr<RBX::Instance> pInstance)
: InstanceUpdateHandler(pInstance)
, m_ItemInfo(0)
, m_queuedForDeletion(false)
{
	initData();

	m_treeWidgetDepth = 1;

	//add it to map in tree view for find
	pTreeWidget->addInstance(m_pInstance.get(), this);

	//add item to tree view
	pTreeWidget->insertTopLevelItem(index, this);
}

RobloxTreeWidgetItem::RobloxTreeWidgetItem(int index, RobloxTreeWidgetItem *pParentWidgetItem, boost::shared_ptr<RBX::Instance> pInstance)
: InstanceUpdateHandler(pInstance)
, m_ItemInfo(0)
, m_queuedForDeletion(false)
{
	initData();

	m_treeWidgetDepth = pParentWidgetItem->getTreeWidgetDepth() + 1;

	//add it to map in tree view for find (cannot use current item's getTreeWidget)
	pParentWidgetItem->getTreeWidget()->addInstance(m_pInstance.get(), this);

	//add item as child
	pParentWidgetItem->insertChild(index, this);
}

RobloxTreeWidgetItem::~RobloxTreeWidgetItem()
{
	RobloxTreeWidget* pTreeWidget = getTreeWidget();

	if (!pTreeWidget)
	{
		//May not be in treeWidget
		RobloxExplorerWidget* robloxExplorer = static_cast<RobloxExplorerWidget*>(UpdateUIManager::Instance().getExplorerWidget());
		pTreeWidget = robloxExplorer->getTreeWidget();
	}

	if (pTreeWidget)
	{
		pTreeWidget->removeItemFromRemovalList(this);
		pTreeWidget->eraseInstance(getInstance().get());
	}
}

void RobloxTreeWidgetItem::setData(int column, int role, const QVariant & value)
{
	//only editing of column 0 i.e. name change is to be handled
	if (column != 0 || role != Qt::EditRole)
	{
		QTreeWidgetItem::setData(column, role, value);
		return;
	}
	
	try
	{
		QString label = value.toString();
		if (!label.isEmpty() && label != m_pInstance->getName().c_str())
		{
			RBX::DataModel::LegacyLock lock(getTreeWidget()->dataModel(), RBX::DataModelJob::Write);
			m_pInstance->setName(label.toStdString());

            // check to see if rename failed
            if ( m_pInstance->getName() == label.toStdString() )
            {
			    //now update the tree widget data
			    QTreeWidgetItem::setData(column, role, value);
    		
			    //set waypoint
			    RBX::ChangeHistoryService::requestWaypoint("Rename", getTreeWidget()->dataModel().get());
			    getTreeWidget()->dataModel()->setDirty(true);
            }
		}
	}

	catch(std::exception& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Label edit failed : %s", exp.what());
	}
}

void RobloxTreeWidgetItem::initData()
{
	setText(0, m_pInstance->getName().c_str());
	int imageIndex = getImageIndex(m_pInstance);
	setIcon(0, QIcon(QtUtilities::getPixmap(":/images/ClassImages.PNG", imageIndex)));

	m_ItemType = getItemType(m_pInstance);

	setDirty(true);
	setExpandedOnce(false);

	setData(1, ExpandRole, QVariant(false));

	setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);

	if (hasChildren())
		setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

bool RobloxTreeWidgetItem::hasChildren()
{
	bool result = false;
	if (m_pInstance->getChildren())
	{
		RBX::Instances childInstances = *m_pInstance->getChildren();
		if (!childInstances.empty())
		{
			for (RBX::Instances::const_iterator iter = childInstances.begin(); iter != childInstances.end(); ++iter)
			{
				if (isExplorerItem(*iter))
				{
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

void RobloxTreeWidgetItem::setViewState()
{
	RobloxTreeWidget* pTreeWidget = getTreeWidget();

	if (pTreeWidget->isFilterEmpty())
	{
		setExpanded(data(1, ExpandRole).toBool());
		setTextColor(0, Qt::black);
		setHidden(false);
		setDisabled(false);
	}
	else if (pTreeWidget->isFoundItem(getInstance()))
	{
		setExpanded(true);
		setTextColor(0, Qt::black);
		setHidden(false);
		setDisabled(false);
	}
	else if (pTreeWidget->isAncestorOfFoundItem(getInstance()))
	{
		setExpanded(true);
		setTextColor(0, Qt::gray);
		setHidden(false);
		setDisabled(false);
	}
	else
	{
		setHidden(true);
		setDisabled(true);
	}
}

RobloxTreeWidget *RobloxTreeWidgetItem::getTreeWidget()
{	return dynamic_cast<RobloxTreeWidget *>(treeWidget()); }

RobloxTreeWidgetItem* RobloxTreeWidgetItem::getItemParent()
{	return this; }

void RobloxTreeWidgetItem::updateFilterItems()
{
	m_FilterItemsToAdd.clear();

	RobloxTreeWidget* pTreeWidget = getTreeWidget();
	RBXASSERT(pTreeWidget);
	if (!pTreeWidget)
		return;

	if (!pTreeWidget->isFilterEmpty())
	{
		for (std::set<shared_ptr<RBX::Instance> >::const_iterator iter = m_PendingItemsToAdd.begin(); iter != m_PendingItemsToAdd.end(); ++iter)
			if (pTreeWidget->isAncestorOfFoundItem(*iter) || pTreeWidget->isFoundItem(*iter))
				m_FilterItemsToAdd.insert(*iter);
	}
}

void RobloxTreeWidgetItem::requestItemExpand()
{
	RobloxTreeWidget *pTreeWidget = getTreeWidget();

	RBXASSERT(pTreeWidget);
	if (!pTreeWidget)
		return;

	if (pTreeWidget->isFilterEmpty())
		setData(1, ExpandRole, QVariant(true));

	if (m_pInstance->getChildren())
	{
		RBX::Instances childInstances = *m_pInstance->getChildren();
		if (childInstances.empty())
			return;

		//add child in pending queue
		if (!isExpandedOnce())
		{
			QMutexLocker lock(pTreeWidget->treeWidgetMutex());
			for (RBX::Instances::const_iterator iter = childInstances.begin(); iter != childInstances.end(); ++iter)
				if(!pTreeWidget->findItemFromInstance(iter->get()) &&RobloxTreeWidgetItem::isExplorerItem(*iter))
					m_PendingItemsToAdd.insert(*iter);
		}
	}

	setExpandedOnce(true);
	
	updateFilterItems();

	pTreeWidget->addToUpdateList(this);
}

void RobloxTreeWidgetItem::takeAllChildren()
{
	while (childCount() > 0)
	{
		QTreeWidgetItem* firstItem = child(0);
		RobloxTreeWidgetItem* treeItem = dynamic_cast<RobloxTreeWidgetItem*>(firstItem);

		if (treeItem)
			treeItem->takeAllChildren();

		this->takeChild(0);
	}
}

void RobloxTreeWidgetItem::addPlayerSelection(int playerId, int color)
{
	PlayersColorCollection playersData = data(0, TreeWidgetDelegate::OtherUsersHighlightingRole).value<PlayersColorCollection >();
	if (playersData.find(playerId) == playersData.end())
	{
		playersData[playerId] = color;
		setData(0, TreeWidgetDelegate::OtherUsersHighlightingRole, QVariant::fromValue<PlayersColorCollection >(playersData));
	}
}


void RobloxTreeWidgetItem::removePlayerSelection(int playerId)
{
	PlayersColorCollection playersData = data(0, TreeWidgetDelegate::OtherUsersHighlightingRole).value<PlayersColorCollection >();
	if (playersData.remove(playerId) > 0)
		setData(0, TreeWidgetDelegate::OtherUsersHighlightingRole, QVariant::fromValue<PlayersColorCollection >(playersData));
}

bool RobloxTreeWidgetItem::removeItemFromTreeWidget()
{
	RobloxTreeWidget *pTreeWidget = getTreeWidget();
	if (!pTreeWidget)
		return false;

	bool currentlySelected = false;

	if (RBX::Selection* selection = RBX::ServiceProvider::find<RBX::Selection>(getInstance().get()))
	{
		for (RBX::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
		{
			if (*iter == getInstance())
			{
				currentlySelected = true;
				break;
			}
		}
	}

	if (currentlySelected)
		pTreeWidget->blockSignals(true);


	if (RobloxTreeWidgetItem* previousParent = dynamic_cast<RobloxTreeWidgetItem*>(static_cast<QTreeWidgetItem*>(this)->parent()))
	{
		int childIndex = previousParent->indexOfChild(this);
		takeAllChildren();
		previousParent->takeChild(childIndex);
	}
	else
	{
		int childIndex = pTreeWidget->indexOfTopLevelItem(this);
		takeAllChildren();
		pTreeWidget->takeTopLevelItem(childIndex);
	}

	if (currentlySelected)
		pTreeWidget->blockSignals(false);

	return currentlySelected;
}

void RobloxTreeWidgetItem::aboutToDelete(RobloxTreeWidget* pTreeWidget)
{
	m_queuedForDeletion = true;

	m_cChildAddedConnection.disconnect();
	m_cChildRemovedConnection.disconnect();
	m_cPropertyChangedConnection.disconnect();
	
	pTreeWidget->eraseInstance(getInstance().get());
	pTreeWidget->removeItemFromRemovalList(this);

	pTreeWidget->removeFromUpdateList(this);
	
	int numChild = childCount();
	if (!numChild)
	{
		removeItemFromTreeWidget();
		return;
	}
	
	int currentChild = 0;
	RobloxTreeWidgetItem *pCurrentItem = NULL;
	
	while (currentChild < numChild)
	{		
		pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(currentChild));
		if (pCurrentItem)
		{
			pCurrentItem->aboutToDelete(pTreeWidget);
			pCurrentItem->deleteLater();			
		}
		++currentChild;
	}

	if (!FFlag::StudioDE8774CrashFixEnabled)
		pTreeWidget->removeFromUpdateList(this);

	removeItemFromTreeWidget();
}

bool RobloxTreeWidgetItem::onChildAdded(shared_ptr<RBX::Instance> pChild)
{
	if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_queuedForDeletion) return false;

	FASTLOG1(FLog::Explorer, "RobloxTreeWidgetItem::onChildAdded, child: %p", pChild.get());

	RobloxTreeWidget *pTreeWidget = getTreeWidget();
	if (!pTreeWidget)
		return false;

	FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

	//if not expanded once then just set dirty flag
	if (!isExpandedOnce())
	{
		if (RobloxTreeWidgetItem::isExplorerItem(pChild))
		{
			setDirty(true);	
			setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
			pTreeWidget->requestViewportUpdate();
		}
		return true;
	}

	if (!InstanceUpdateHandler::onChildAdded(pChild))
		return false;

	if (isExpandedOnce())
		pTreeWidget->addToUpdateList(this);

	return true;
}

bool RobloxTreeWidgetItem::onChildRemoved(shared_ptr<RBX::Instance> pChild)
{		
	if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_queuedForDeletion) return false;

	FASTLOG1(FLog::Explorer, "RobloxTreeWidgetItem::onChildRemoved, child: %p", pChild.get());
	//if child item has been added before then let it get removed!
	if (!InstanceUpdateHandler::onChildRemoved(pChild))
		return false;

	FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

	RobloxTreeWidget* treeWidget = getTreeWidget();

	if (!treeWidget)
		return false;

	if (!hasChildren())
	{
		setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
		getTreeWidget()->requestViewportUpdate();
	}

	//request for an update
	if (isExpandedOnce())
		getTreeWidget()->addToUpdateList(this);

	return true;
}

bool RobloxTreeWidgetItem::hasChildrenToProcess()
{
	if (RobloxTreeWidget* pWidget = getTreeWidget())
		if (!pWidget->isFilterEmpty())
			return !m_FilterItemsToAdd.empty();

	return !m_PendingItemsToAdd.empty();
}

void RobloxTreeWidgetItem::processPropertyChange()
{
	setText(0, m_pInstance->getName().c_str());
}

bool RobloxTreeWidgetItem::handleOpen()
{
	if(!m_pInstance)
		return false;

	// Open script document
	return RobloxDocManager::Instance().openDoc(LuaSourceBuffer::fromInstance(m_pInstance));
}

int RobloxTreeWidgetItem::getIndexToInsertAt(shared_ptr<RBX::Instance> pInstance)
{
	int numChild = childCount();
	if (!numChild)
		return -1;

	int itemTypeToAdd = RobloxTreeWidgetItem::getItemType(pInstance);
	int nearestIndex = nearestWidgetItemIndex(this, 0, childCount()-1, itemTypeToAdd);

	RobloxTreeWidgetItem *pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(nearestIndex));
	if (!pCurrentItem || (itemTypeToAdd != pCurrentItem->itemType()))
		return nearestIndex;

	//if the type is same then we need to insert in the order of item's name
	if ((pInstance->getName() < pCurrentItem->getInstance()->getName()))
	{		
		while (nearestIndex >= 0)
		{
			pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(nearestIndex));
			if (!pCurrentItem)
				break;

			if((itemTypeToAdd != pCurrentItem->itemType()) || (pInstance->getName() > pCurrentItem->getInstance()->getName()))
			{
				nearestIndex++;
				break;
			}

			--nearestIndex;
		}
	}
	else
	{
		while (nearestIndex < childCount())
		{
			pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(nearestIndex));
			if (!pCurrentItem || (itemTypeToAdd != pCurrentItem->itemType()) || (pInstance->getName() < pCurrentItem->getInstance()->getName()))
				break;
			++nearestIndex;
		}
	}

	return nearestIndex;
}

bool RobloxTreeWidgetItem::isExplorerItem(const shared_ptr<RBX::Instance>& pInstance)
{
    // Assume everything that isn't a direct child of data model should be shown in explorer
    if (pInstance->getParent() != RBX::DataModel::get(pInstance.get()))
	{
		RBX::CoreGuiService* cgs = RBX::ServiceProvider::find<RBX::CoreGuiService>(pInstance.get());
		if (cgs && pInstance->isDescendantOf(cgs))
			return false;
		
		return true;
    }

    // For everything else, check the class metadata
	boost::shared_ptr<const RBX::Reflection::Metadata::Reflection> pReflection = RBX::Reflection::Metadata::Reflection::singleton();
	RBX::Reflection::Metadata::Class* pClassData = pReflection ? pReflection->get(pInstance->getDescriptor(), true) : NULL;
	if(pClassData && pClassData->isExplorerItem())
		return true;

	return false;
}

int RobloxTreeWidgetItem::getImageIndex(const shared_ptr<RBX::Instance>& pInstance)
{
	boost::shared_ptr<const RBX::Reflection::Metadata::Reflection> pReflection = RBX::Reflection::Metadata::Reflection::singleton();
	RBX::Reflection::Metadata::Class* pClassData = pReflection ? pReflection->get(pInstance->getDescriptor(), true) : NULL;
	if(pClassData)
		return pClassData->getExplorerImageIndex();

	return 0;
}

int RobloxTreeWidgetItem::getItemType(const shared_ptr<RBX::Instance>& pInstance)
{
	boost::shared_ptr<const RBX::Reflection::Metadata::Reflection> pReflection = RBX::Reflection::Metadata::Reflection::singleton();
	RBX::Reflection::Metadata::Class* pClassData = pReflection->get(pInstance->getDescriptor(), true);
	return pClassData ? pClassData->getExplorerOrder() : 0;
}

RobloxTreeWidget::RobloxTreeWidget(boost::shared_ptr<RBX::DataModel> pDataModel)
: InstanceUpdateHandler(pDataModel)
, m_pDataModel(pDataModel)
, m_treeWidgetMutex(QMutex::Recursive)
, m_pInstanceSelectionHandler(NULL)
, m_pRubberBand(NULL)
, m_bIgnoreInstanceSelectionChanged(false)
, m_bIgnoreItemSelectionChanged(false)
, m_bUpdateRequested(false)
, m_bViewportUpdateRequested(false)
, m_bDeletionRequested(false)
, m_currentFilter("")
, m_filterRunning(false)
, m_lastSelectedItem(NULL)
, m_isActive(false)
, m_savedMarkerItem(NULL)
{
	setHeaderHidden(true);
	setUniformRowHeights(true);

	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::DragDrop);
	setDropIndicatorShown(true);
    setAutoScroll(true);
    setAutoScrollMargin(32);

	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSelectionBehavior(QAbstractItemView::SelectItems);

    header()->setStretchLastSection(false);
    header()->setResizeMode(0,QHeaderView::ResizeToContents);
    
    // Set edit trigger only for SelectionClicked and EditKeyPressed 
	setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

	if (!UpdateUIManager::Instance().getMainWindow().isRibbonStyle())
	{
		setStyleSheet(STRINGIFY(
			QTreeView 
			{
				 show-decoration-selected: 1;
				 selection-color: white;
			}

			QTreeView::item 
			{
				border: 1px solid transparent;
			}

			QTreeView::item:hover 
			{
				border: 1px solid #BFCDE4;
				background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #E7EFFD, stop: 1 #CBDAF1);
			}

			QTreeView::item:selected 
			{
				border: 1px solid #567DBC;
			}

			QTreeView::item:selected:active
			{
				background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6EA1F1, stop: 1 #567DBC);
			}

			QTreeView::item:selected:!active 
			{
				background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6B9BE8, stop: 1 #577FBF);
			}
		) );
	}

	initTreeView(pDataModel);
    
    connect(this,                                      SIGNAL(helpTopicChanged(const QString&)),
            &RobloxContextualHelpService::singleton(), SLOT(onHelpTopicChanged(const QString&)));
		
	m_cDescendantAddedConnection = pDataModel->getWorkspace()->getOrCreateDescendantAddedSignal()->connect(boost::bind(&RobloxTreeWidget::onDescendantAdded, this, _1));
}	

RobloxTreeWidget::~RobloxTreeWidget()
{	
	deActivate();

	m_InstanceMap.clear();
	m_itemsToUpdate.clear();
	m_selectedItems.clear();
	m_unSelectedItems.clear();
}

void RobloxTreeWidget::addInstance(const RBX::Instance *pInstance, RobloxTreeWidgetItem *pTreeWidgetItem) 
{
	m_InstanceMap[pInstance] = pTreeWidgetItem;
}

void RobloxTreeWidget::initTreeView(boost::shared_ptr<RBX::DataModel> pDataModel)
{
	if (!pDataModel->getChildren())
		return;

	RBX::Instances instances;
	instances = *pDataModel->getChildren();

	std::for_each(instances.begin(), instances.end(), boost::bind(&RobloxTreeWidget::createTreeRoot, this, _1));
}

void RobloxTreeWidget::focusInEvent(QFocusEvent *event)
{
	eraseLastSelectedItem();
    QTreeWidget::focusInEvent(event);
    Q_EMIT focusGained();

    UpdateUIManager::Instance().updateToolBars();
}

void RobloxTreeWidget::createTreeRoot(const boost::shared_ptr<RBX::Instance> pInstance)
{
	if (RobloxTreeWidgetItem::isExplorerItem(pInstance))
	{
		int index = getIndexToInsertAt(pInstance);
		RobloxTreeWidgetItem* item = new RobloxTreeWidgetItem((index > 0) ? index : 0, this, pInstance);
	}
}

RobloxTreeWidget* RobloxTreeWidget::getTreeWidget()
{	return this; }

RBX::Selection* RobloxTreeWidget::getSelection()
{	return RBX::ServiceProvider::create<RBX::Selection>(m_pDataModel.get()); }

boost::shared_ptr<RBX::DataModel> RobloxTreeWidget::dataModel()
{	return m_pDataModel; }

void RobloxTreeWidget::eraseInstance(RBX::Instance *pInstance)
{ 
    QMutexLocker lock(treeWidgetMutex());

	m_selectedItems.erase(pInstance);
	m_unSelectedItems.erase(pInstance);

	InstanceMap::iterator iter = m_InstanceMap.find(pInstance); 
	if (iter != m_InstanceMap.end())
	{
		if (m_lastSelectedItem == iter->second)
			eraseLastSelectedItem();

		removeFromUpdateList(iter->second);
		m_InstanceMap.erase(iter);
	}
}

RobloxTreeWidgetItem* RobloxTreeWidget::findItemFromInstance(const RBX::Instance *pInstance)
{
	InstanceMap::iterator iter = m_InstanceMap.find(pInstance);
	if (iter != m_InstanceMap.end())	
		return iter->second;
	return NULL;
}

void RobloxTreeWidget::scrollToInstance(RBX::Instance *pInstance)
{
	if(!pInstance)
		return;

	if(isValidSelection(pInstance))
	{
		RobloxTreeWidgetItem* item = findItemFromInstance(pInstance);
		if(item)
			scrollToItem(item);
		else if (RobloxTreeWidgetItem* parentItem = findItemFromInstance(pInstance->getParent()))
			parentItem->setExpanded(true);

	}
}

int RobloxTreeWidget::getIndexToInsertAt(shared_ptr<RBX::Instance> pInstance)
{
	int numChild = 	topLevelItemCount();
	if (!numChild)
		return -1;

	int itemTypeToAdd = RobloxTreeWidgetItem::getItemType(pInstance), currentChild = 0;
	RobloxTreeWidgetItem *pCurrentItem = NULL;

	while (currentChild < numChild)
	{		
		pCurrentItem = static_cast<RobloxTreeWidgetItem*>(topLevelItem(currentChild));
		if (itemTypeToAdd < pCurrentItem->itemType())
			break;

		if (itemTypeToAdd == pCurrentItem->itemType())
		{
			if (pInstance->getName() < pCurrentItem->getInstance()->getName())
				break;			
		}

		++currentChild;
	}

	return currentChild;
}

void RobloxTreeWidget::activate()
{
	if (m_isActive)
		return;
	m_isActive = true;

	connect((QTreeWidget *)this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
	connect((QTreeWidget *)this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onItemExpanded(QTreeWidgetItem*)));
	connect((QTreeWidget *)this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
	connect((QTreeWidget *)this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));

	if (m_pDataModel)
	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
		RBX::Selection* pSelection = m_pDataModel->create<RBX::Selection>();
		if (pSelection)
			m_cInstanceSelectionChanged = pSelection->selectionChanged.connect(boost::bind(&RobloxTreeWidget::onInstanceSelectionChanged, this, _1));
	}
}

void RobloxTreeWidget::deActivate()
{
	if (!m_isActive)
		m_isActive = false;

	disconnect((QTreeWidget *)this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
	m_cInstanceSelectionChanged.disconnect();

	disconnect((QTreeWidget *)this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onItemExpanded(QTreeWidgetItem*)));
	disconnect((QTreeWidget *)this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
	disconnect((QTreeWidget *)this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));
}

bool RobloxTreeWidget::onChildAdded(shared_ptr<RBX::Instance> pChild)
{
	if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_bDeletionRequested) return false;

	if (!InstanceUpdateHandler::onChildAdded(pChild))
		return false;
	requestUpdate();
	return true;
}

bool RobloxTreeWidget::onChildRemoved(shared_ptr<RBX::Instance> pChild)
{
	if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_bDeletionRequested) return false;

	if (!InstanceUpdateHandler::onChildRemoved(pChild))
		return false;
	requestUpdate();
	return true;
}

void RobloxTreeWidget::onDescendantAdded(shared_ptr<RBX::Instance> pDescendant)
{
	if (!isFilterEmpty())
	{
		if (filterInstance(pDescendant) && filterAncestors(pDescendant))
			requestUpdate();
	}
}

void RobloxTreeWidget::contextMenuEvent(QContextMenuEvent * evt)
{
    QMenu menu;

	QList<QAction*> commonActions;
	UpdateUIManager::Instance().commonContextMenuActions(commonActions);

	QAction *pAction = NULL;
	for (int ii=0; ii<commonActions.size(); ++ii)
	{
		pAction = commonActions.at(ii);
		if (pAction)
        {
			// hack to add sub menu for inserting basic objects
			if ( pAction == UpdateUIManager::Instance().getMainWindow().insertIntoFileAction )
			{
				RBX::Instance* parent = NULL;
				if ( currentItem() )
				{
					RobloxTreeWidgetItem* item = static_cast<RobloxTreeWidgetItem*>(currentItem());
					parent = item->getInstance().get();
				}

				if (!parent)
					parent = m_pDataModel->getWorkspace();

				menu.addAction(
					QIcon(QtUtilities::getPixmap(":/images/ClassImages.PNG",1)),    // part icon 
					"Insert Part",
					this,
					SLOT(onInsertPart()) );
				menu.addMenu(InsertObjectWidget::createMenu(parent,this,SLOT(onInsertObject())));
			}           
			menu.addAction(pAction);
        }
		else
			menu.addSeparator();
	}

	menu.addSeparator();
	menu.addAction(UpdateUIManager::Instance().getMainWindow().launchHelpForSelectionAction);
	
	//TODO: Add other tree view actions
	connect(&menu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));
	connect(&menu, SIGNAL(aboutToHide()), &UpdateUIManager::Instance(), SLOT(onMenuHide()));
	
    menu.exec(evt->globalPos());
}

/**
 * Callback for user clicking on insert part in context menu menu.
 */
void RobloxTreeWidget::onInsertPart()
{
    InsertObjectWidget::InsertObject("Part",m_pDataModel,InsertObjectWidget::InsertMode_TreeWidget);
	if (FFlag::StudioSeparateActionByActivationMethod)
		UpdateUIManager::Instance().getMainWindow().sendActionCounterEvent("insertPartMainContext");
}

/**
 * Callback for user clicking on insert basic object in context menu sub menu.
 */
void RobloxTreeWidget::onInsertObject()
{
    QAction* action = static_cast<QAction*>(sender());
    QString className = action->text();
    InsertObjectWidget::InsertObject(className,m_pDataModel,InsertObjectWidget::InsertMode_TreeWidget);
	if (FFlag::StudioSeparateActionByActivationMethod)
		UpdateUIManager::Instance().getMainWindow().sendActionCounterEvent(QString("insert%1Context").arg(className));
}

void RobloxTreeWidget::keyPressEvent(QKeyEvent *evt)
{	
	if ( (state() != EditingState) && ((evt->key() == Qt::Key_Enter) || (evt->key() == Qt::Key_Return)) )
	{
		RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(currentItem());
		//handle appropriate document open
		if (pTreeWidgetItem && pTreeWidgetItem->handleOpen())
		{
			//accept event
			evt->accept();
			return;
		}
	}

    if (QKeySequence(evt->key() | evt->modifiers()) == UpdateUIManager::Instance().getMainWindow().zoomExtentsAction->shortcut())
    {
        UpdateUIManager::Instance().getMainWindow().zoomExtentsAction->activate(QAction::Trigger);
        evt->accept();
        return;
    }

	QTreeWidget::keyPressEvent(evt);
}

void RobloxTreeWidget::processChildrenRemoval()
{
	TreeWidgetItemList::const_iterator iter = m_itemsToRemove.begin();
	while (iter != m_itemsToRemove.end())
	{
		if (RobloxTreeWidgetItem* pTreeWidgetItem = *iter)
		{
			pTreeWidgetItem->aboutToDelete(this);
			pTreeWidgetItem->deleteLater();
		}
		else
		{
			m_itemsToRemove.erase(iter);
		}

		iter = m_itemsToRemove.begin();
	}
}

void RobloxTreeWidget::requestItemDelete(RobloxTreeWidgetItem* item)
{
	for (int i = 0; i < item->childCount(); ++i)
		if (RobloxTreeWidgetItem* childItem = dynamic_cast<RobloxTreeWidgetItem*>((item)->child(i)))
			requestItemDelete(childItem);

	m_itemsToRemove.insert(item);
	removeFromUpdateList(item);
}

void RobloxTreeWidget::onInstanceSelectionChanged(const RBX::SelectionChanged& evt)
{
	if (evt.addedItem != NULL)
		if(RBX::Instance *pInstance = evt.addedItem.get())
			if(RBX::GuiObject* guiObject = dynamic_cast<RBX::GuiObject*>(pInstance))
				guiObject->setSelectionBox(true);

	if (evt.removedItem != NULL)
		if(RBX::Instance *pInstance = evt.removedItem.get())
			if(RBX::GuiObject* guiObject = dynamic_cast<RBX::GuiObject*>(pInstance))
				guiObject->setSelectionBox(false);


	if (m_bIgnoreInstanceSelectionChanged)
		return;
	
	RBX::ScopedAssign<bool> ignoreSelectionEvent(m_bIgnoreItemSelectionChanged, true);

	QMutexLocker lock(treeWidgetMutex());
	bool isModificationRequired = false;

	if (evt.addedItem != NULL)
	{
		RBX::Instance* pInstance = evt.addedItem.get();
		if (isValidSelection(pInstance))
		{
			m_unSelectedItems.erase(pInstance);
			m_selectedItems.insert(pInstance);

			isModificationRequired = true;
		}
	}

	if (evt.removedItem != NULL)
	{
		RBX::Instance* pInstance = evt.removedItem.get();
		
		m_selectedItems.erase(pInstance);
		m_unSelectedItems.insert(pInstance);
		isModificationRequired = true;
	}

	if (isModificationRequired)
		requestUpdate();
}

void RobloxTreeWidget::onCloudEditSelectionChanged(int playerId)
{
	RBXASSERT(QThread::currentThread() == qApp->thread());
	if (!m_pPlayersDataManager)
	{
		RBXASSERT(false);
		return;
	}

	bool enabled = updatesEnabled();
	setUpdatesEnabled(false);

	if (FFlag::TeamCreateOptimizeRemoteSelection)
	{
		shared_ptr<const RBX::Instances> oldSelection;
		PlayerSelectionMap::iterator iter = m_prevCloudEditSelection.find(playerId);
		if (iter != m_prevCloudEditSelection.end())
			oldSelection = iter->second;
		else
			oldSelection.reset(new RBX::Instances());

		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
		RBX::Instances::const_iterator findIter;
		shared_ptr<const RBX::Instances> currentSelection = m_pPlayersDataManager->getLastKnownSelection(playerId);

		bool blockSignals = model()->blockSignals(true);

		for (auto& selectedItem : *currentSelection)
		{
			findIter = std::find(oldSelection->begin(), oldSelection->end(), selectedItem);
			if (findIter == oldSelection->end())
			{
				if (RobloxTreeWidgetItem* item = findItemFromInstance(selectedItem.get()))
					item->addPlayerSelection(playerId, m_pPlayersDataManager->getPlayerColor(playerId));
			}
		}

		for (auto& oldSelectedItem : *oldSelection)
		{
			findIter = std::find(currentSelection->begin(), currentSelection->end(), oldSelectedItem);
			if (findIter == currentSelection->end())
			{
				if (RobloxTreeWidgetItem* item = findItemFromInstance(oldSelectedItem.get()))
					item->removePlayerSelection(playerId);
			}
		}

		model()->blockSignals(blockSignals);

		// TODO: check if we need to break it into smaller chunks to clear main thread asap?

		// save current selection so we can update unselected instances in next change
		m_prevCloudEditSelection[playerId] = currentSelection;
		setUpdatesEnabled(enabled);
	}
	else
	{
		RBX::Instances itemsToUnselect;
		PlayerSelectionMap::iterator iter = m_prevCloudEditSelection.find(playerId);
		if (iter != m_prevCloudEditSelection.end())
			itemsToUnselect = *(iter->second);

		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

		// TODO: check if we need to break it into smaller chunks to clear main thread asap?
		RBX::Instances::iterator findIter;
		shared_ptr<const RBX::Instances> currentSelection = m_pPlayersDataManager->getLastKnownSelection(playerId);
		for (auto& selectedItem : *currentSelection)
		{
			findIter = std::find(itemsToUnselect.begin(), itemsToUnselect.end(), selectedItem);
			if (findIter == itemsToUnselect.end())
			{
				if (RobloxTreeWidgetItem* item = findItemFromInstance(selectedItem.get()))
					item->addPlayerSelection(playerId, m_pPlayersDataManager->getPlayerColor(playerId));
			}
			else
			{
				itemsToUnselect.erase(findIter);
			}
		}

		for (auto& unselectedItem : itemsToUnselect)
		{
			if (RobloxTreeWidgetItem* item = findItemFromInstance(unselectedItem.get()))
				item->removePlayerSelection(playerId);
		}

		// save current selection so we can update unselected instances in next change
		m_prevCloudEditSelection[playerId] = currentSelection;
		setUpdatesEnabled(enabled);
	}
}

bool RobloxTreeWidget::isSearchTimeUp(const RBX::Time& startTime, int duration)
{
	return (RBX::Time::now<RBX::Time::Fast>() - startTime).msec() > duration;
}

void RobloxTreeWidget::addFilterItemsToUpdateList()
{
	for (std::set<shared_ptr<RBX::Instance> >::const_iterator iter = m_ancestorsOfFoundItems.begin(); iter != m_ancestorsOfFoundItems.end(); ++iter)
		if (RobloxTreeWidgetItem* item = findItemFromInstance(iter->get()))
			addToUpdateList(item);

	for (std::set<shared_ptr<RBX::Instance> >::const_iterator iter = m_foundItems.begin(); iter != m_foundItems.end(); ++iter)
		if (RobloxTreeWidgetItem* item = findItemFromInstance(iter->get()))
			addToUpdateList(item);
}

void RobloxTreeWidget::onFilterWidgetUpdate()
{
	if (isHidden())
		return;

	Q_EMIT startedProcessing();

	FilterDeque_SPTR previousFilterStack = FilterDeque_SPTR(m_currentFilterStack.begin(), m_currentFilterStack.end());
	m_currentFilterStack.clear();

	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Read);
		filterWidgetRecursive(dataModel(), previousFilterStack, RBX::Time::now<RBX::Time::Fast>());
	}
	
	if (!m_currentFilterStack.empty())
	{
		QTimer::singleShot(FInt::StudioTreeWidgetEventProcessingTime, this, SLOT(onFilterWidgetUpdate()));
	}
	else
	{
		m_filterRunning = false;
		addFilterItemsToUpdateList();
		requestUpdate();
		Q_EMIT filterSearchFinished();
	}
}

bool RobloxTreeWidget::filterAncestors(shared_ptr<RBX::Instance> instance)
{
	if (!instance->getParent())
		return true;

	if (filterAncestors(shared_from(instance->getParent())))
	{
		if (!RobloxTreeWidgetItem::isExplorerItem(instance))
			return false;

		if (filterInstance(instance))
			m_foundItems.insert(instance);
		else
			m_ancestorsOfFoundItems.insert(instance);

		if (RobloxTreeWidgetItem* item = findItemFromInstance(instance.get()))
			addToUpdateList(item);

		return true;
	}
	return false;
}

void RobloxTreeWidget::filterWidget(const QString& searchString)
{
	setWidgetFilter(searchString);
	filterWidget();
}

void RobloxTreeWidget::filterWidget()
{
	m_currentFilterStack.clear();
	m_foundItems.clear();
	m_ancestorsOfFoundItems.clear();
	m_itemsToUpdate.clear();

	for (int i = 0; i < invisibleRootItem()->childCount(); ++i)
	{
		invisibleRootItem()->child(i)->setDisabled(true);
		invisibleRootItem()->child(i)->setHidden(true);
	}

	if (!m_filterRunning)
	{
		m_filterRunning = true;
		QTimer::singleShot(0, this, SLOT(onFilterWidgetUpdate()));
	}
}

bool RobloxTreeWidget::filterInstance(shared_ptr<RBX::Instance> instance)
{
	for (ParamList::const_iterator iter = m_searchParamList.begin(); iter != m_searchParamList.end(); ++iter)
	{
		switch (iter->second)
		{
		case ParamAny:
			if (!boost::icontains(instance->getName(), iter->first) && !boost::icontains(instance->getClassNameStr(), iter->first))
				return false;
			break;
		case ParamName:
			if (!boost::icontains(instance->getName(), iter->first))
				return false;
			break;
		case ParamType:
			if (!boost::icontains(instance->getClassNameStr(), iter->first))
				return false;
			break;
		}
	}
	return true;
}

bool RobloxTreeWidget::filterWidgetRecursive(shared_ptr<RBX::Instance> instance, FilterDeque_SPTR& previousFilterStack, const RBX::Time& startTime)
{
	if (!RobloxTreeWidgetItem::isExplorerItem(instance))
		return false;

	bool parentOfShownItem = false;

	if (instance->getChildren())
	{
		shared_ptr<RBX::Instance> startingChild;
		bool stackFrontFound = true;

		//using previousFilterStack to start from last search location
		if (!previousFilterStack.empty())
		{
			stackFrontFound = false;
			
			startingChild = previousFilterStack.front();
			previousFilterStack.pop_front();
			
			//if stack is broken, we continue through all children
			if (startingChild->getParent() != instance.get())
			{
				previousFilterStack.clear();
				stackFrontFound = true;
			}
		}

		for (RBX::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
		{
			if (!stackFrontFound && *iter == startingChild)
				stackFrontFound = true;

			m_currentFilterStack.push_back(*iter);

			if (stackFrontFound && filterWidgetRecursive(*iter, previousFilterStack, startTime))
				parentOfShownItem = true;

			if (isSearchTimeUp(startTime, FInt::StudioTreeWidgetFilterTime))
				break;

			m_currentFilterStack.pop_back();
		}
	}
	else
	{
		previousFilterStack.clear();
	}

	//This is a check to see if filter is empty or instance is the datamodel
	if (isFilterEmpty() || !instance->getParent())
	{
		//Restore item to it's original state
		if (RobloxTreeWidgetItem* item = findItemFromInstance(instance.get()))
			item->setViewState();
		return false;
	}

	if (filterInstance(instance))
	{
		m_foundItems.insert(instance);
		return true;
	}
	else if (parentOfShownItem)
	{
		m_ancestorsOfFoundItems.insert(instance);
		return true;
	}
	else
	{
		//This is hiding the item if there is a filter
		if (RobloxTreeWidgetItem* item = findItemFromInstance(instance.get()))
			item->setViewState();
	}

	return false;
}

void RobloxTreeWidget::setWidgetFilter(const QString& filterString)
{
	m_currentFilter = filterString;

	QStringList filterList = filterString.split(" ", QString::SkipEmptyParts);

	bool nextIsName = false;
	bool nextIsType = false;
	m_searchParamList.clear();
	
	for (QStringList::const_iterator iter = filterList.begin(); iter != filterList.end(); ++iter)
	{
		if (nextIsName)
		{
			nextIsName = false;
			m_searchParamList.push_back(StringParamPair(iter->toStdString(), ParamName));
		}
		else if (nextIsType)
		{
			nextIsType = false;
			m_searchParamList.push_back(StringParamPair(iter->toStdString(), ParamType));
		}
		else if (iter->startsWith(nameSearchString.c_str(), Qt::CaseInsensitive))
		{
			QString rightSide = iter->right(iter->size() - nameSearchString.size());

			if (rightSide.isEmpty())
				nextIsName = true;
			else
				m_searchParamList.push_back(StringParamPair(rightSide.toStdString(), ParamName));
		}
		else if (iter->startsWith(classSearchString.c_str(), Qt::CaseInsensitive))
		{
			QString rightSide = iter->right(iter->size() - classSearchString.size());

			if (rightSide.isEmpty())
				nextIsType = true;
			else
				m_searchParamList.push_back(StringParamPair(rightSide.toStdString(), ParamType));
		}
		else
		{
			m_searchParamList.push_back(StringParamPair(iter->toStdString(), ParamAny));
		}
	}
}

bool RobloxTreeWidget::isFoundItem(shared_ptr<RBX::Instance> instance)
{
	return m_foundItems.count(instance) > 0;
}

bool RobloxTreeWidget::isAncestorOfFoundItem(shared_ptr<RBX::Instance> instance)
{
	return m_ancestorsOfFoundItems.count(instance) > 0;
}

bool RobloxTreeWidget::isValidSelection(RBX::Instance *pInstance)
{
	if (!pInstance)
		return false;

	if (!pInstance->getParent())
		return true;

	if (isValidSelection(pInstance->getParent()))
	{
		if (!RobloxTreeWidgetItem::isExplorerItem(shared_from(pInstance)))
			return false;

		return true;
	}

	return false;
}

void RobloxTreeWidget::onItemSelectionChanged()
{
	if (m_bIgnoreItemSelectionChanged)
		return;

	RBX::Selection* pSelection(getSelection());
	if (!pSelection)
		return;

	if (m_pInstanceSelectionHandler)
	{
		boost::shared_ptr<RBX::Instance> selectedInstance;
		QList<QTreeWidgetItem *> selectedItems = this->selectedItems();
		if (selectedItems.size() > 0)
		{
			RobloxTreeWidgetItem *pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(selectedItems.at(0));
			if (pTreeWidgetItem)
				selectedInstance = pTreeWidgetItem->getInstance();
		}
		m_pInstanceSelectionHandler->onInstanceSelected(selectedInstance);
		return;
	}

	RBX::ScopedAssign<bool> ignoreSelectionEvent(m_bIgnoreInstanceSelectionChanged, true);
	RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

	QList<QTreeWidgetItem *> selectedItems = this->selectedItems();

	shared_ptr<RBX::Instances> instances(new RBX::Instances());
	for (QList<QTreeWidgetItem*>::const_iterator iter = selectedItems.begin(); iter != selectedItems.end(); ++iter)
	{
		if (RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(*iter))
		{
			if (shared_ptr<RBX::Instance> instance = pTreeWidgetItem->getInstance())
				instances->push_back(instance);
		}
	}

	pSelection->setSelection(instances);
	
	if (currentItem())
	{
		RobloxTreeWidgetItem *pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(currentItem());
		QString className = pTreeWidgetItem->getInstance()->getClassName().c_str();
		if (FFlag::StudioNewWiki) 
			className.prepend("API:Class/");
		Q_EMIT(helpTopicChanged(className));
	}
	
	//HACK: to get toolbars updated for object selection
	UpdateUIManager::Instance().updateToolBars();
}

void RobloxTreeWidget::modifyItemsSelectionState(InstanceList_PTR &instancesList, bool select)
{
	QList<QModelIndex> modelIndexes;
	
	for (InstanceList_PTR::const_iterator iter = instancesList.begin(); iter != instancesList.end(); )
	{
		if (RobloxTreeWidgetItem *pTreeWidgetItem = findItemFromInstance(*iter))
		{
			modelIndexes.push_back(indexFromItem(pTreeWidgetItem));

			if (select)
			{
				m_lastSelectedItem = pTreeWidgetItem;
				instancesList.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
		else
		{
			++iter;
		}
	}

	qStableSort(modelIndexes.begin(), modelIndexes.end());	
	selectionModel()->select(mergeModelIndexes(modelIndexes), select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

void RobloxTreeWidget::onItemExpanded(QTreeWidgetItem *pItem)
{
	if (!pItem)
		return;

	RobloxTreeWidgetItem* robloxTreeWidgetItem = static_cast<RobloxTreeWidgetItem*>(pItem);
	robloxTreeWidgetItem->requestItemExpand();
}

void RobloxTreeWidget::onItemCollapsed(QTreeWidgetItem *pItem)
{
	if (!pItem)
		return;

	if (isFilterEmpty())
		pItem->setData(1, ExpandRole, QVariant(false));
}

void RobloxTreeWidget::onItemDoubleClicked(QTreeWidgetItem *pWidgetItem, int)
{
	if (!pWidgetItem)
		return;
	
	static_cast<RobloxTreeWidgetItem*>(pWidgetItem)->handleOpen();
}

void RobloxTreeWidget::addToUpdateList(RobloxTreeWidgetItem *pTreeWidgetItem)
{
	if (!pTreeWidgetItem)
		return;

	QMutexLocker lock(treeWidgetMutex());

	if (pTreeWidgetItem->isItemPendingDeletion())
		return;
	pTreeWidgetItem->setViewState();
	if (existsInRemovalList(pTreeWidgetItem) || !pTreeWidgetItem->isInTreeWidget())
		return;

	std::pair<TreeWidgetItemList::iterator, bool> retIter = m_itemsToUpdate.insert(pTreeWidgetItem);
	if (retIter.second)
		requestUpdate();
}

void RobloxTreeWidget::removeFromUpdateList(RobloxTreeWidgetItem *pTreeWidgetItem)
{
	QMutexLocker lock(treeWidgetMutex());
	m_itemsToUpdate.erase(pTreeWidgetItem);
}

void RobloxTreeWidget::requestUpdate()
{
	if (m_bUpdateRequested)
		return;
	//qt will take ownership of the event so no need to delete
	QApplication::postEvent(this, new RobloxCustomEvent(TREE_WIDGET_UPDATE));
		
	m_bUpdateRequested = true;
}

void RobloxTreeWidget::requestViewportUpdate()
{
	if (m_bViewportUpdateRequested || m_bUpdateRequested)
		return;
	//qt will take ownership of the event so no need to delete
	QApplication::postEvent(this, new RobloxCustomEvent(TREE_WIDGET_VIEWPORT_UPDATE));
	m_bViewportUpdateRequested = true;
}

void RobloxTreeWidget::updateSelectionState(RBX::Instance *pInstance)
{
	RBX::Selection* pSelection(getSelection());
	if (pSelection && pSelection->isSelected(pInstance))
	{
		QMutexLocker lock(treeWidgetMutex());
		m_unSelectedItems.erase(pInstance);
		m_selectedItems.insert(pInstance);			
	}

	// If items are added after updating the highlighting state for CloudEdit mode, update now
	RBX::Instances::const_iterator selectedIter;
	for (auto& playersSelection : m_prevCloudEditSelection)
	{
		selectedIter = std::find(playersSelection.second->begin(), playersSelection.second->end(), shared_from(pInstance));
		if (selectedIter != playersSelection.second->end())
		{
			if (RobloxTreeWidgetItem* pTreeWidgetItem = findItemFromInstance(pInstance))
				pTreeWidgetItem->addPlayerSelection(playersSelection.first, m_pPlayersDataManager->getPlayerColor(playersSelection.first));
		}
	}
}

bool RobloxTreeWidget::event(QEvent* evt)
{
	bool retVal = false;
	if(evt->type() == TREE_WIDGET_UPDATE)
	{
		onTreeWidgetUpdate();
		m_bUpdateRequested = false;
		retVal = true;
	}
	else if(evt->type() == TREE_WIDGET_FILTER_UPDATE)
	{
		onFilterWidgetUpdate();
	}
	else if(evt->type() == TREE_SCROLL_TO_INSTANCE)
	{
		RobloxCustomEventWithArg *pCustomEvent = dynamic_cast<RobloxCustomEventWithArg*>(evt);
		if (pCustomEvent)
		{
			boost::function<void()>* pFunctionObj = pCustomEvent->m_pEventArg;
			if (pFunctionObj)
				(*pFunctionObj)();		
		}
		retVal = true;
	}
	else if (evt->type() == TREE_WIDGET_VIEWPORT_UPDATE)
	{
		m_bViewportUpdateRequested = false;
		viewport()->update();
		
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
		retVal = QTreeWidget::event(evt);

	return retVal;
}

void RobloxTreeWidget::onTreeWidgetUpdate()
{
	if (isHidden() || m_filterRunning)
		return;
		
	QCoreApplication::removePostedEvents(this, TREE_WIDGET_VIEWPORT_UPDATE);

	//prevents screen flicker
	setUpdatesEnabled(false);
	RBX::ScopedAssign<bool> ignoreSelectionEvent(m_bIgnoreItemSelectionChanged, true);

	QMutexLocker treeLock(treeWidgetMutex());

	if (!m_itemsToRemove.empty())
		processChildrenRemoval();
		
	int selectionSize = m_selectedItems.size();
	//modify selection first, items not removed from list and items that have yet been populated
	{
		blockSignals(true);
		QMutexLocker lock(treeWidgetMutex());

		if (!m_unSelectedItems.empty())
		{
			modifyItemsSelectionState(m_unSelectedItems, false);
			m_unSelectedItems.clear();
		}

		if (!m_selectedItems.empty())
			modifyItemsSelectionState(m_selectedItems, true);	

		selectionSize = m_selectedItems.size();
		blockSignals(false);
	}

	bool itemsAdded = false;
		
	InstanceList_PTR itemsToSelect;

	RBX::Time startTime = RBX::Time::now<RBX::Time::Fast>();

	std::vector<RobloxTreeWidgetItem*> recentlyAddedItems;

	//Processing Selection
	while (!m_selectedItems.empty() && isFilterEmpty() && !isSearchTimeUp(startTime, FInt::StudioTreeWidgetProcessingTime))
	{
		RBX::Instance* pInstance = *m_selectedItems.begin();

		RBX::Instance* pChild = pInstance;
		while (pChild->getParent() && !findItemFromInstance(pChild->getParent()))
			pChild = pChild->getParent();

		RobloxTreeWidgetItem* newChildNode = NULL;

		if (RobloxTreeWidgetItem* pParentItem = findItemFromInstance(pChild->getParent()))
		{
			pParentItem->requestItemExpand();
			newChildNode = pParentItem->processChildAdd(shared_from(pChild));
		}
		else
		{
			m_selectedItems.erase(pInstance);
		}

		if (pChild == pInstance && newChildNode)
		{
			recentlyAddedItems.push_back(newChildNode);
			m_lastSelectedItem = newChildNode;
			m_selectedItems.erase(pInstance);
		}
	}

	//Processing items requesting update
	while (!m_itemsToUpdate.empty() && !isSearchTimeUp(startTime, FInt::StudioTreeWidgetProcessingTime))
	{
		RobloxTreeWidgetItem* itemNode = *m_itemsToUpdate.begin();

		if (itemNode)
		{
			itemNode->updateFilterItems();
			if (itemNode->processChildrenAdd())
				itemsAdded = true;

			if (!itemNode->hasChildrenToProcess())
			{
				removeFromUpdateList(itemNode);
				itemNode->setDirty(false);
			}
		}
	}

	//recently added items
	if (!recentlyAddedItems.empty())
	{
		QList<QModelIndex> modelIndexes;

		for (std::vector<RobloxTreeWidgetItem*>::const_iterator iter = recentlyAddedItems.begin(); iter != recentlyAddedItems.end(); ++iter)
			modelIndexes.push_back(indexFromItem(*iter));

		qStableSort(modelIndexes.begin(), modelIndexes.end());
		selectionModel()->select(mergeModelIndexes(modelIndexes), QItemSelectionModel::Select);
	}
		
	processChildrenAdd();

	setUpdatesEnabled(true);

	m_bUpdateRequested = false;

	//again request for an update
	if (!m_itemsToRemove.empty() || !m_itemsToUpdate.empty() || itemsAdded || (!m_selectedItems.empty() && isFilterEmpty()))
	{
		QTimer::singleShot(0, this, SLOT(requestUpdate()));
	}
	else
	{
		if (m_lastSelectedItem)
		{
			scrollToItem(m_lastSelectedItem);
			eraseLastSelectedItem();
		}

		if (m_savedMarkerItem)
			QMetaObject::invokeMethod(this, "scrollBackToLastMarkedLocation");

		Q_EMIT finishedProcessing();
	}
}

void RobloxTreeWidget::requestDelete()
{
	m_bDeletionRequested = true;
	
	deActivate();

	m_pInstance.reset();
	m_pDataModel.reset();
	setPlayersDataManager(boost::shared_ptr<PlayersDataManager>());

	deleteLater();
}

void RobloxTreeWidget::mousePressEvent(QMouseEvent * evt)
{
	QTreeWidget::mousePressEvent(evt);

    RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos()));
    
    if (pTreeWidgetItem)
    {
        QString className = pTreeWidgetItem->getInstance()->getClassName().c_str();
		if (FFlag::StudioNewWiki) 
			className.prepend("API:Class/");
        Q_EMIT(helpTopicChanged(className));
    }
    
	// Check if ContextMenu was opened in the middle of dragging.
	if (m_pRubberBand)
	{    
		m_pRubberBand->hide();
		m_pRubberBand->deleteLater();
		m_pRubberBand = NULL;
	}
	else
	{
		//although it will be set to NULL in mouseReleaseEvent but just to be sure setting it to NULL again
		m_pRubberBand = NULL;
	}


	//if we cannot drag select then continue with Qt event propagation
	if (!canDragSelect(evt))
	{
		if (FFlag::StudioMimeDataContainsInstancePath)
		{
			QDrag *drag = new QDrag(this);
			QMimeData *mimeData = new QMimeData;
			mimeData->setText(("Game." + pTreeWidgetItem->getInstance()->getFullName()).c_str());
			drag->setMimeData(mimeData);
			drag->exec();
		}

		return;
	}

	//begin drag select
	m_initialOffset = QPoint(horizontalOffset(), verticalOffset());
	m_rubberBandOrigin = evt->pos();

	m_pRubberBand = new QRubberBand(QRubberBand::Rectangle, this);
	m_pRubberBand->setGeometry(QRect(m_rubberBandOrigin, QSize()));
	m_pRubberBand->show();
}

void RobloxTreeWidget::mouseMoveEvent(QMouseEvent * evt)
{
	if (!m_pRubberBand)
	{
        QTreeWidget::mouseMoveEvent(evt);
        if (m_pInstanceSelectionHandler)
        {
            boost::shared_ptr<RBX::Instance> hoveredInstance;
            RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos()));
            if (pTreeWidgetItem)
                hoveredInstance = pTreeWidgetItem->getInstance();
            m_pInstanceSelectionHandler->onInstanceHovered(hoveredInstance);
        }

		return;
	}

	//--- drag selection is ON (do the required updates)
	if (verticalScrollBar())
	{
		if (evt->y() <= 1)
			verticalScrollBar()->setValue(verticalScrollBar()->value() + evt->y() - 1);
		else if (evt->y() >= (viewport()->height() - 1))
			verticalScrollBar()->setValue(verticalScrollBar()->value() + evt->y() - viewport()->height() - 1);
	}

	if (horizontalScrollBar())
	{
		if (evt->x() <= 1)
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() + evt->x() - 1);
		else if (evt->y() >= (viewport()->width() - 1))
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() + evt->x() - viewport()->width() - 1);
	}

	QPoint offset = QPoint(horizontalOffset() - m_initialOffset.x(), verticalOffset() - m_initialOffset.y());
	QRect rubberBandRect = QRect(m_rubberBandOrigin - offset, evt->pos()).normalized();

	m_pRubberBand->setGeometry(rubberBandRect);
	setSelection(QRect(rubberBandRect.bottomLeft(), rubberBandRect.topLeft()), QItemSelectionModel::ClearAndSelect);
}

void RobloxTreeWidget::mouseReleaseEvent(QMouseEvent * evt)
{
	if (!m_pRubberBand)
	{
		QTreeWidget::mouseReleaseEvent(evt);
		return;
	}

	//clean up rubberband
    m_pRubberBand->hide();
	m_pRubberBand->deleteLater();
	m_pRubberBand = NULL;
}

bool RobloxTreeWidget::canDragSelect(QMouseEvent * evt)
{
	if (evt->buttons() != Qt::LeftButton || evt->modifiers() != Qt::NoModifier || m_pInstanceSelectionHandler)
		return false;

	QPoint mousePos = evt->pos();
	QTreeWidgetItem *pTreeWidgetItem = itemAt(mousePos);
	if (pTreeWidgetItem)
	{
		// - make sure if the mouse is clicked outside the item label we must be able to drag select
		// - we must not activate drag selection if mouse click is on the left of the label (to ensure the expand keys work fine)
		// - item drag/drop will work when user clicks on the item label
		QRect itemRect = visualItemRect(pTreeWidgetItem);
		if ( !itemRect.contains(mousePos) ||
			 (itemRect.contains(mousePos) && (mousePos.x() < (itemRect.bottomLeft().x() + 28 + fontMetrics().width(pTreeWidgetItem->text(0))))) )
			return false;
	}

	//-- we can do a drag select --

	// item under mouse is already selected and the only selected item, we don't proceed further
	if (pTreeWidgetItem && pTreeWidgetItem->isSelected() && (selectedItems().size() == 1))
		return true;

	//clear previously selected items
	clearSelection();
	//if we have an item then select it
	if (pTreeWidgetItem)
		pTreeWidgetItem->setSelected(true);

	return true;
}

void RobloxTreeWidget::dragEnterEvent(QDragEnterEvent *evt)
{
	const QMimeData *mime = evt->mimeData();
	QString data = mime->text();
	QTreeWidget::dragEnterEvent(evt);
}

void RobloxTreeWidget::dragMoveEvent(QDragMoveEvent * evt)
{
	// call base class to handle auto scrolling
    QTreeWidget::dragMoveEvent(evt);

	evt->ignore();

	if (RobloxTreeWidgetItem *pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos())))
	{
		if (shared_ptr<RBX::Instance> pInstance = pTreeWidgetItem->getInstance())
		{
			RBX::Selection* selection = getSelection();
			for (RBX::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
				if (pInstance == *iter || (*iter)->isAncestorOf2(pInstance) || (*iter)->getIsParentLocked())
					return;

			evt->acceptProposedAction();
		}
	}
}

static void setChildren(RBX::Instance* pParent, shared_ptr<RBX::Instance> pChild);

void RobloxTreeWidget::dropEvent(QDropEvent * evt)
{
	try
	{
		RobloxTreeWidgetItem *pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos()));
		if (pTreeWidgetItem)
		{
			boost::shared_ptr<RBX::Instance> pInstance = pTreeWidgetItem->getInstance();
			if (pInstance)
			{
				RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
				pTreeWidgetItem->requestItemExpand();
		
				std::for_each(getSelection()->begin(), getSelection()->end(), boost::bind(&setChildren, pInstance.get(), _1));

				//add in history for undo/redo
				RBX::ChangeHistoryService::requestWaypoint("Drop", m_pDataModel.get());
				m_pDataModel->setDirty(true);
			}
		}
	}
	catch (std::exception& e)
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setText(e.what());
		msgBox.exec();
	}

	evt->setDropAction(Qt::IgnoreAction);
    setState(NoState);
}

static void setChildren(RBX::Instance* pParent, shared_ptr<RBX::Instance> pChild)
{
	//self drag case (ideally qt should have already taken care of this)
	if (!pChild || pChild->getParent() == pParent)
		return;

	pChild->setParent(pParent);
}

static int nearestWidgetItemIndex(RobloxTreeWidgetItem *pParentWidgetItem, int first, int last, int itemTypeToAdd) 
{
	while (first <= last) 
	{
		int mid = (first + last) / 2;
		RobloxTreeWidgetItem *pCurrentItem = static_cast<RobloxTreeWidgetItem*>(pParentWidgetItem->child(mid));
		RBXASSERT(pCurrentItem != NULL);

		if (itemTypeToAdd == pCurrentItem->itemType()) 
			return mid; 
		else if (itemTypeToAdd > pCurrentItem->itemType()) 
			first = mid+1;
		else
			last = mid-1;
	}

	return first;
}

static QItemSelection mergeModelIndexes(const QList<QModelIndex> &indexes)
{
    QItemSelection colSpans;
    // merge columns
    int i = 0;
    while (i < indexes.count()) {
        QModelIndex tl = indexes.at(i);
        QModelIndex br = tl;
        while (++i < indexes.count()) {
            QModelIndex next = indexes.at(i);
            if ((next.parent() == br.parent())
                 && (next.row() == br.row())
                 && (next.column() == br.column() + 1))
                br = next;
            else
                break;
        }
        colSpans.append(QItemSelectionRange(tl, br));
    }
    // merge rows
    QItemSelection rowSpans;
    i = 0;
    while (i < colSpans.count()) {
        QModelIndex tl = colSpans.at(i).topLeft();
        QModelIndex br = colSpans.at(i).bottomRight();
        QModelIndex prevTl = tl;
        while (++i < colSpans.count()) {
            QModelIndex nextTl = colSpans.at(i).topLeft();
            QModelIndex nextBr = colSpans.at(i).bottomRight();

            if (nextTl.parent() != tl.parent())
                break;

            if ((nextTl.column() == prevTl.column()) && (nextBr.column() == br.column())
                && (nextTl.row() == prevTl.row() + 1) && (nextBr.row() == br.row() + 1)) {
                br = nextBr;
                prevTl = nextTl;
            } else {
                break;
            }
        }
        rowSpans.append(QItemSelectionRange(tl, br));
    }
    return rowSpans;
}

void RobloxTreeWidget::setInstanceSelectionHandler(InstanceSelectionHandler* handler)
{
	setMouseTracking(handler);
	m_pInstanceSelectionHandler = handler;
}

void RobloxTreeWidget::markLocationToSuppressScrolling()
{
	if (m_savedMarkerItem == NULL)
	{
		m_savedMarkerItem = getMarkerItem();
	}
}

void RobloxTreeWidget::scrollBackToLastMarkedLocation()
{
	if (m_savedMarkerItem)
	{
		QTreeWidgetItem* currenMarkerItem = getMarkerItem();
		if (currenMarkerItem != m_savedMarkerItem)
		{
			int savedRow = indexFromItem(m_savedMarkerItem).row();
			int currentRow = indexFromItem(currenMarkerItem).row();

			verticalScrollBar()->setValue(verticalScrollBar()->value() + (savedRow - currentRow) * verticalScrollBar()->singleStep());
		}
		m_savedMarkerItem = NULL;
	}
}

QTreeWidgetItem* RobloxTreeWidget::getMarkerItem()
{
	QList<QTreeWidgetItem*> selection = selectedItems();
	// if we've a single selection and the selected item is not hidden, the use it as benchmark
	if (selection.count() == 1 && !selection.at(0)->isHidden())
		return selection.at(0);
	// or else use the top item as benchmark
	return itemAt(rect().topLeft());
}

void RobloxTreeWidget::setPlayersDataManager(boost::shared_ptr<PlayersDataManager> playersDataManager)
{
	if (m_pPlayersDataManager == playersDataManager)
		return;

	if (m_pPlayersDataManager)
		disconnect(m_pPlayersDataManager.get(), SIGNAL(cloudEditSelectionChanged(int)), this, SLOT(onCloudEditSelectionChanged(int)));
	m_TreeWidgetDelegate.reset();
	m_pPlayersDataManager = playersDataManager;

	if (m_pPlayersDataManager)
	{
		m_TreeWidgetDelegate.reset(new TreeWidgetDelegate(this));
		setItemDelegate(m_TreeWidgetDelegate.get());
		connect(m_pPlayersDataManager.get(), SIGNAL(cloudEditSelectionChanged(int)), this, SLOT(onCloudEditSelectionChanged(int)));
	}
}

// TODO: Merge this with eraseLastSelectedItem
void RobloxTreeWidget::itemDeletionRequested(RobloxTreeWidgetItem* pItem)
{
	if (pItem == m_savedMarkerItem)
		m_savedMarkerItem = NULL;
}

RobloxExplorerWidget::RobloxExplorerWidget(QWidget *pParent)
: QWidget(pParent)
, m_robloxTreeWidget(NULL)
, m_sendingCounter(false)
{
	m_lineEdit = new QLineEdit();
	m_lineEdit->setPlaceholderText(QString("Filter workspace (%1)").arg(UpdateUIManager::Instance().getMainWindow().explorerFilterAction->shortcut().toString()));
	m_lineEdit->hide();

	m_loadingMovie = new QMovie(":/images/loading.gif");
	m_loadingLabel = new QLabel(m_lineEdit);
	m_loadingLabel->setMovie(m_loadingMovie);
	
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch();
	layout->addWidget(m_loadingLabel);
	layout->addSpacing(5);

	m_lineEdit->setLayout(layout);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->setSpacing(4);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(m_lineEdit);
	setLayout(mainLayout);

	setFocusProxy(m_lineEdit);
}

void RobloxExplorerWidget::setCurrentWidget(RobloxTreeWidget* treeWidget)
{
	if (m_robloxTreeWidget == treeWidget)
		return;

	QLayout* mainLayout = layout();
	
	if (m_robloxTreeWidget)
	{
		m_lineEdit->hide();

		onProcessingFinished();
		
		m_robloxTreeWidget->hide();
		mainLayout->removeWidget(m_robloxTreeWidget);

		disconnect(m_robloxTreeWidget, SIGNAL(finishedProcessing()), this, SLOT(onProcessingFinished()));
		disconnect(m_robloxTreeWidget, SIGNAL(startedProcessing()), this, SLOT(onProcessingStarted()));
		disconnect(m_robloxTreeWidget, SIGNAL(focusGained()), this, SIGNAL(focusGained()));
		disconnect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterTreeWidget(QString)));
	}

	m_robloxTreeWidget = treeWidget;
	
	if (m_robloxTreeWidget)
	{
		mainLayout->addWidget(m_robloxTreeWidget);
		m_robloxTreeWidget->show();
		m_lineEdit->show();

		m_lineEdit->setText(m_robloxTreeWidget->currentFilter());

		connect(m_robloxTreeWidget, SIGNAL(finishedProcessing()), this, SLOT(onProcessingFinished()));
		connect(m_robloxTreeWidget, SIGNAL(startedProcessing()), this, SLOT(onProcessingStarted()));
		connect(m_robloxTreeWidget, SIGNAL(focusGained()), this, SIGNAL(focusGained()));
		connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterTreeWidget(QString)));
		
		if (!m_robloxTreeWidget->isFilterEmpty())
		{
			onProcessingStarted();
			m_robloxTreeWidget->filterWidget();
		}
	}
}

void RobloxExplorerWidget::onProcessingStarted()
{
	m_loadingMovie->start();
	m_loadingLabel->show();
}

void RobloxExplorerWidget::onProcessingFinished()
{
	m_loadingLabel->hide();
	m_loadingMovie->stop();
}

void RobloxExplorerWidget::filterTreeWidget(const QString& text)
{
	if (m_robloxTreeWidget)
	{
		if (!m_sendingCounter)
		{
			m_sendingCounter = true;
			QTimer::singleShot(1000, this, SLOT(sendWidgetCounter()));
		}
		
		m_robloxTreeWidget->filterWidget(text);
	}	
}

void RobloxExplorerWidget::sendWidgetCounter()
{
	m_sendingCounter = false;

	RobloxSettings settings;
	bool previouslyFiltered = settings.value("ExplorerWidgetFilterUsed", false).toBool();
	settings.setValue("ExplorerWidgetFilterUsed", true);

	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, previouslyFiltered ? "ExplorerWidgetFilterUsed" : "ExplorerWidgetFilterUsed_FTU");
}
