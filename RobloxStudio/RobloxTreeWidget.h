/**
 * RobloxTreeWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QTreeWidget>
#include <QMutex>
#include <QLineEdit>

// Roblox Headers
#include "rbx/signal.h"

// Roblox Studio Headers
#include "FindDialog.h"
#include "PlayersDataManager.h"

namespace RBX {
	class Instance;
	class DataModel;
	class Selection;
	class SelectionChanged;
	namespace Reflection {
		class PropertyDescriptor;
	}
}

FASTFLAG(StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates)

class RobloxTreeWidgetItem;
class RobloxTreeWidget;
class TreeWidgetDelegate;

#define ITEM_IS_DIRTY        (1 << 1)
#define ITEM_EXPANDED_ONCE   (1 << 2)

struct DepthCompare
{
	bool operator() (RobloxTreeWidgetItem* lhs, RobloxTreeWidgetItem* rhs) const;
};

enum ItemRole
{
	ExpandRole = Qt::UserRole
};

enum SearchParams
{
	ParamAny = 0,
	ParamType = 1,
	ParamName = 2
};

typedef std::set<boost::shared_ptr<RBX::Instance> > InstanceList_SPTR;
typedef std::set<RBX::Instance*>					InstanceList_PTR;
typedef std::set<RobloxTreeWidgetItem*>             TreeWidgetItemList;
typedef std::set<RobloxTreeWidgetItem*, DepthCompare> TreeWidgetItemSortedList;

typedef	std::pair<std::string, SearchParams>		StringParamPair;
typedef std::vector<StringParamPair>				ParamList;
typedef std::deque<shared_ptr<RBX::Instance> >		FilterDeque_SPTR;

typedef std::map<const RBX::Instance*, RobloxTreeWidgetItem*> InstanceMap;

class InstanceSelectionHandler
{
public:
	virtual void onInstanceSelected(boost::shared_ptr<RBX::Instance> instance) = 0;
	virtual void onInstanceHovered(boost::shared_ptr<RBX::Instance> instance) = 0;
};

//TODO: Consider this to move a new file
class InstanceUpdateHandler
{
public:
	InstanceUpdateHandler(boost::shared_ptr<RBX::Instance> pInstance);
	virtual ~InstanceUpdateHandler();

	boost::shared_ptr<RBX::Instance> getInstance() { return m_pInstance; }

	void populateChildren(RobloxTreeWidget* pTreeWidget);

	virtual RobloxTreeWidget     *getTreeWidget() = 0;
	virtual RobloxTreeWidgetItem *getItemParent() = 0;

	virtual bool onChildAdded(shared_ptr<RBX::Instance> pChild);
	virtual bool onChildRemoved(shared_ptr<RBX::Instance> pChild);
	virtual void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

	// The update handler needs to use qt slots to marshall to the main thread,
	// but this object can't inherit QObject because that causes multiple inheritance
	// and the subclasses can use a templated mixin class because QT moc can't handle
	// templates. Use this pure virtual stubs to remind subclasses to make Q_SLOTS for
	// these events.
	virtual void childAddedSignalHandler(shared_ptr<RBX::Instance> pChild) = 0;
	virtual void childRemovedSignalHandler(shared_ptr<RBX::Instance> pChild) = 0;
	virtual void propertyChangedSignalHandler(const RBX::Reflection::PropertyDescriptor* pDescriptor) = 0;
	
	void removeFromRemovalList(RobloxTreeWidgetItem *pTreeWidgetItem);

	RobloxTreeWidgetItem* processChildAdd(shared_ptr<RBX::Instance> pInstance);
	virtual bool processChildrenAdd();
	virtual void processChildRemove();
	virtual void processPropertyChange() {}

	void createWidgetItem(shared_ptr<RBX::Instance> pInstance);

	int getTreeWidgetDepth() const { return m_treeWidgetDepth; }

protected:
	boost::shared_ptr<RBX::Instance>  m_pInstance;

	rbx::signals::scoped_connection   m_cChildAddedConnection;
	rbx::signals::scoped_connection   m_cChildRemovedConnection;
	rbx::signals::scoped_connection   m_cPropertyChangedConnection;

	InstanceList_SPTR                 m_PendingItemsToAdd;
	InstanceList_SPTR				  m_FilterItemsToAdd;
	
	TreeWidgetItemList                m_PendingItemsToRemove;

	int								  m_treeWidgetDepth;
};

class RobloxTreeWidgetItem: public QObject, public QTreeWidgetItem, public InstanceUpdateHandler
{
    Q_OBJECT
public:
	RobloxTreeWidgetItem(int index, RobloxTreeWidget *pTreeWidget, boost::shared_ptr<RBX::Instance> pInstance);	
	RobloxTreeWidgetItem(int index, RobloxTreeWidgetItem *pParentWidgetItem, boost::shared_ptr<RBX::Instance> pInstance);

	~RobloxTreeWidgetItem();

	RobloxTreeWidget     *getTreeWidget();
	RobloxTreeWidgetItem *getItemParent();
	int                   itemType() { return m_ItemType; }	

	void requestItemExpand();
	void aboutToDelete(RobloxTreeWidget* pTreeWidget);

	static bool isExplorerItem(const shared_ptr<RBX::Instance>& pInstance);
	static int  getImageIndex(const shared_ptr<RBX::Instance>& pInstance);
	static int  getItemType(const shared_ptr<RBX::Instance>& pInstance);

	void processPropertyChange() override;

	bool handleOpen();
	int getIndexToInsertAt(shared_ptr<RBX::Instance> pInstance);

	void setDirty(bool state)   { setItemInfo(ITEM_IS_DIRTY, state); }
	bool isDirty()              { return getItemInfo(ITEM_IS_DIRTY); }

	bool hasFilterChildrenToProcess() { return !m_FilterItemsToAdd.empty(); }
	bool hasChildrenToProcess();

	void setViewState();

	void updateFilterItems();

	bool isExpandedOnce()               { return getItemInfo(ITEM_EXPANDED_ONCE); }

	bool isItemPendingDeletion() { return m_queuedForDeletion; }

	bool isInTreeWidget() { return treeWidget() != NULL; }

	bool removeItemFromTreeWidget();

	void takeAllChildren();

	void setExpandedOnce(bool state)    { setItemInfo(ITEM_EXPANDED_ONCE, state); }

	void addPlayerSelection(int playerId, int color);
	void removePlayerSelection(int playerId);

protected:
	void childAddedSignalHandler(shared_ptr<RBX::Instance> pchild) override
	{
		QMetaObject::invokeMethod(this, "onChildAdded", Qt::QueuedConnection, Q_ARG(shared_ptr<RBX::Instance>, pchild));
	}
	void childRemovedSignalHandler(shared_ptr<RBX::Instance> pchild) override
	{
		QMetaObject::invokeMethod(this, "onChildRemoved", Qt::QueuedConnection, Q_ARG(shared_ptr<RBX::Instance>, pchild));
	}
	void propertyChangedSignalHandler(const RBX::Reflection::PropertyDescriptor* pDescriptor) override
	{
		QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection, Q_ARG(const RBX::Reflection::PropertyDescriptor*, pDescriptor));
	}

private:
	void setData(int column, int role, const QVariant & value) override;

	Q_INVOKABLE bool onChildAdded(shared_ptr<RBX::Instance> pChild) override;
	Q_INVOKABLE bool onChildRemoved(shared_ptr<RBX::Instance> pChild) override;
	// override just to mark as Q_SLOT
	Q_INVOKABLE void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor) override
	{
		if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_queuedForDeletion) return;
		InstanceUpdateHandler::onPropertyChanged(pDescriptor);
	}
	
	void initData();	

	bool hasChildren();

	

	void setItemInfo(int bitPosition, bool state) { m_ItemInfo = state ? m_ItemInfo | bitPosition : m_ItemInfo & ~bitPosition; }
	bool getItemInfo(int bitPosition)   { return m_ItemInfo & bitPosition; }

	int                               m_ItemInfo;
	int                               m_ItemType;

	bool							  m_queuedForDeletion;
};

class RobloxTreeWidget : public QTreeWidget, public InstanceUpdateHandler
{
	Q_OBJECT
public:
	RobloxTreeWidget(boost::shared_ptr<RBX::DataModel> pDataModel);
	virtual ~RobloxTreeWidget();

	boost::shared_ptr<RBX::DataModel> dataModel();	
	RBX::Selection*       getSelection();
	RobloxTreeWidget*     getTreeWidget();
	RobloxTreeWidgetItem* getItemParent() { return NULL; }
	QMutex* treeWidgetMutex() { return &m_treeWidgetMutex; }
	
	void addInstance(const RBX::Instance *pInstance, RobloxTreeWidgetItem *pTreeWidgetItem);
	void eraseInstance(RBX::Instance *pInstance);
	RobloxTreeWidgetItem* findItemFromInstance(const RBX::Instance *pInstance);

	void scrollToInstance(RBX::Instance *pInstance);

	int getIndexToInsertAt(shared_ptr<RBX::Instance> pInstance);

	void activate();
	void deActivate();	
	
	void addToUpdateList(RobloxTreeWidgetItem *pTreeWidgetItem);
	void removeFromUpdateList(RobloxTreeWidgetItem *pTreeWidgetItem);

	void requestDelete();

	void requestViewportUpdate();

	void updateSelectionState(RBX::Instance *pInstance);

	void processChildrenRemoval();
	void requestItemDelete(RobloxTreeWidgetItem* item);

	void removeItemFromRemovalList(RobloxTreeWidgetItem* item)
	{
		m_itemsToRemove.erase(item);
	}

	bool existsInRemovalList(RobloxTreeWidgetItem* item)
	{
		return m_itemsToRemove.find(item) != m_itemsToRemove.end();
	}
	
	bool isDeletionRequested()  { return m_bDeletionRequested;    }
		
	bool filterInstance(shared_ptr<RBX::Instance> instance);

	bool filterAncestors(shared_ptr<RBX::Instance> pInstance);

	bool filterWidgetRecursive(shared_ptr<RBX::Instance> instance, FilterDeque_SPTR& previousFilterStack, const RBX::Time& startTime);

	QString currentFilter() { return m_currentFilter; }
	bool	isFilterEmpty() { return m_currentFilter.isEmpty() || m_searchParamList.empty(); }

	void setWidgetFilter(const QString& filterString);
	
	bool isFoundItem(shared_ptr<RBX::Instance> instance);
	bool isAncestorOfFoundItem(shared_ptr<RBX::Instance> instance);

	void addFilterItemsToUpdateList();

	RobloxTreeWidgetItem* lastSelectedItem()
	{
		return m_lastSelectedItem;
	}

	void eraseLastSelectedItem()
	{
		m_lastSelectedItem = NULL;
	}

	void addItemToSelection(RBX::Instance* item)
	{
		m_selectedItems.insert(item);
	}

	void filterWidget(const QString& searchString);

	void setInstanceSelectionHandler(InstanceSelectionHandler* handler);

	void itemDeletionRequested(RobloxTreeWidgetItem* pItem);
	void markLocationToSuppressScrolling();

	void setPlayersDataManager(boost::shared_ptr<PlayersDataManager> playersDataManager);

public Q_SLOTS:
	void filterWidget();

	void requestUpdate();

protected:
    virtual void focusInEvent(QFocusEvent *event);
	void childAddedSignalHandler(shared_ptr<RBX::Instance> pchild) override
	{
		QMetaObject::invokeMethod(this, "onChildAdded", Qt::QueuedConnection, Q_ARG(shared_ptr<RBX::Instance>, pchild));
	}
	void childRemovedSignalHandler(shared_ptr<RBX::Instance> pchild) override
	{
		QMetaObject::invokeMethod(this, "onChildRemoved", Qt::QueuedConnection, Q_ARG(shared_ptr<RBX::Instance>, pchild));
	}
	void propertyChangedSignalHandler(const RBX::Reflection::PropertyDescriptor* pDescriptor) override
	{
		QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection, Q_ARG(void*, (void*)pDescriptor));
	}

private Q_SLOTS:
	void onItemSelectionChanged();
	void onCloudEditSelectionChanged(int playerId);
	void onItemCollapsed(QTreeWidgetItem* pWidgetItem);
	void onItemExpanded(QTreeWidgetItem *pWidgetItem);
	void onItemDoubleClicked(QTreeWidgetItem *pWidgetItem, int column);

	void onTreeWidgetUpdate();

    void onInsertPart();
    void onInsertObject();

	void onFilterWidgetUpdate();

	void scrollBackToLastMarkedLocation();

	bool onChildAdded(shared_ptr<RBX::Instance> pChild) override;
	bool onChildRemoved(shared_ptr<RBX::Instance> pChild) override;
	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor) override
	{
		if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_bDeletionRequested) return;
		InstanceUpdateHandler::onPropertyChanged(pDescriptor);
	}

private:	
	void onDescendantAdded(shared_ptr<RBX::Instance> pDescendant);
	void onDescendantRemoved(shared_ptr<RBX::Instance> pDescendant);

	bool event(QEvent * evt) override;
	void contextMenuEvent(QContextMenuEvent * evt) override;
	QMenu* getContextMenu();

	void keyPressEvent(QKeyEvent *event) override;
	
	void initTreeView(boost::shared_ptr<RBX::DataModel> pDataModel);
	void createTreeRoot(const boost::shared_ptr<RBX::Instance> instance);
	
	void onInstanceSelectionChanged(const RBX::SelectionChanged& evt);
	bool isValidSelection(RBX::Instance *pInstance);	
	

	void modifyItemsSelectionState(InstanceList_PTR &instancesList, bool select);	

	void mousePressEvent(QMouseEvent * evt) override;
	void mouseMoveEvent(QMouseEvent * evt) override;
	void mouseReleaseEvent(QMouseEvent * evt) override;

	bool canDragSelect(QMouseEvent * evt);
	
	void dragEnterEvent(QDragEnterEvent *evt) override;
	void dragMoveEvent(QDragMoveEvent * evt) override;
	void dropEvent(QDropEvent * evt) override;

	
	bool isSearchTimeUp(const RBX::Time& startTime, int duration);
	
	QTreeWidgetItem* getMarkerItem();
  	
	boost::shared_ptr<RBX::DataModel>        m_pDataModel;
	boost::shared_ptr<PlayersDataManager>    m_pPlayersDataManager;
	rbx::signals::scoped_connection          m_cInstanceSelectionChanged;
	rbx::signals::scoped_connection			 m_cDescendantAddedConnection;
	rbx::signals::scoped_connection			 m_cDescendantRemovedConnection;

	InstanceMap                              m_InstanceMap;

	QMutex                                   m_treeWidgetMutex;

	TreeWidgetItemSortedList				 m_itemsToUpdate;

	TreeWidgetItemList			             m_itemsToRemove;
	
	std::set<shared_ptr<RBX::Instance> >	 m_foundItems;
	std::set<shared_ptr<RBX::Instance> >	 m_ancestorsOfFoundItems;

	InstanceList_PTR                         m_selectedItems;
	InstanceList_PTR                         m_unSelectedItems;

	PlayerSelectionMap                       m_prevCloudEditSelection;
	shared_ptr<TreeWidgetDelegate>           m_TreeWidgetDelegate;

	InstanceSelectionHandler                *m_pInstanceSelectionHandler;

	QRubberBand                             *m_pRubberBand;
	QPoint                                   m_rubberBandOrigin;
	QPoint                                   m_initialOffset;

	QString									 m_currentFilter;
	std::vector<std::string>				 m_filterList;

	ParamList							     m_searchParamList;

	bool                                     m_bIgnoreInstanceSelectionChanged;
	bool                                     m_bIgnoreItemSelectionChanged;
	bool                                     m_bUpdateRequested;
	bool                                     m_bViewportUpdateRequested;

	bool                                     m_bDeletionRequested;

	bool									 m_filterRunning;

	bool									 m_isActive;

	RobloxTreeWidgetItem*					 m_lastSelectedItem;
	
	FilterDeque_SPTR						 m_currentFilterStack;

	RBX::Time								 m_filterStartTime;

	QTreeWidgetItem*                         m_savedMarkerItem;

Q_SIGNALS:
    void helpTopicChanged(const QString& topic);
    void focusGained();
	void filterSearchFinished();
	void finishedProcessing();
	void startedProcessing();
};

class RobloxExplorerWidget : public QWidget
{
	Q_OBJECT
public:
	RobloxExplorerWidget(QWidget *pParent = 0);
	
	void setCurrentWidget(RobloxTreeWidget* treeWidget);

	RobloxTreeWidget* getTreeWidget() { return m_robloxTreeWidget; }
	
	bool filterHasFocus() { return m_lineEdit->hasFocus(); }
	
Q_SIGNALS:
	void focusGained();

protected Q_SLOTS:
	void filterTreeWidget(const QString& text);

	void onProcessingStarted();
	void onProcessingFinished();

	void sendWidgetCounter();
	
private:

	RobloxTreeWidget*		m_robloxTreeWidget;

	QLineEdit*				m_lineEdit;
	
	QMovie*					m_loadingMovie;
	QLabel*					m_loadingLabel;

	bool					m_sendingCounter;
};
