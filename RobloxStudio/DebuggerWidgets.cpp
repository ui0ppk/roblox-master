/**
 * DebuggerWidgets.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"

// Qt Headers
#include <QDockWidget>
#include <QToolButton>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItemV4>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QClipboard>
#include <QInputDialog>
#include <QComboBox>
#include <QLabel>
#include <QPainter>

// Roblox Headers
#include "util/StandardOut.h"
#include "util/ScopedAssign.h"
#include "util/BrickColor.h"
#include "util/UDim.h"
#include "script/Script.h"
#include "v8datamodel/DataModel.h"
#include "V8DataModel/ChangeHistory.h"
#include "reflection/Type.h"

// Roblox Studio Headers
#include "DebuggerWidgets.h"
#include "DebuggerClient.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "QtUtilities.h"
#include "RobloxSettings.h"
#include "RobloxDocManager.h"

//TODO: Move this to generic header file
static bool populateValues(QTreeWidgetItem *pParentItem, const QString &key, const RBX::Reflection::Variant varValue);
static QTreeWidgetItem* addChildren(QTreeWidgetItem* pParentItem, int iconIndex = 6);

//--------------------------------------------------------------------------------------------
// DebuggerViewsManager
//--------------------------------------------------------------------------------------------
DebuggerViewsManager& DebuggerViewsManager::Instance()
{
	static DebuggerViewsManager instance;
	return instance;
}

DebuggerViewsManager::DebuggerViewsManager()
: m_bBreakpointEncountered(false)
{
	//TODO: Move dockwidget initialization to UpdateUiManager.cpp
	RobloxMainWindow* rbxMainWindow = &UpdateUIManager::Instance().getMainWindow();

	m_DockWidgets[eDV_BREAKPOINTS] = new QDockWidget("Breakpoints", rbxMainWindow);
	m_DockWidgets[eDV_BREAKPOINTS]->setObjectName("breakpointsDockWidget");

	m_DockWidgets[eDV_CALLSTACK] = new QDockWidget("Call Stack", rbxMainWindow);
	m_DockWidgets[eDV_CALLSTACK]->setObjectName("callStackDockWidget");

	m_DockWidgets[eDV_WATCH] = new QDockWidget("Watch", rbxMainWindow);
	m_DockWidgets[eDV_WATCH]->setObjectName("watchDockWidget");

	m_ViewWidgets[eDV_BREAKPOINTS] = new BreakpointsWidget;
	m_ViewWidgets[eDV_CALLSTACK] = new CallStackWidget;
	m_ViewWidgets[eDV_WATCH] = new WatchWidget;
	
	//set widgets
	for (int dockId = 0; dockId < eDV_MAX; dockId++)
	{
		m_DockWidgets[(EDebuggerViewID)dockId]->setWidget(m_ViewWidgets[(EDebuggerViewID)dockId]);
	}

	//restore views
	RobloxSettings settings;
	for (int dockId = 0; dockId < eDV_MAX; dockId++)
	{			
		if(!rbxMainWindow->restoreDockWidget(m_DockWidgets[(EDebuggerViewID)dockId]))
		{
			if (dockId == 0)
				rbxMainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_DockWidgets[(EDebuggerViewID)dockId]);
			else
				rbxMainWindow->tabifyDockWidget(m_DockWidgets[(EDebuggerViewID)(dockId - 1)], m_DockWidgets[(EDebuggerViewID)dockId]);
		}
	}
	
	//by default hide widgets
	QAction* pToggleViewAction = NULL;
	QString icons[eDV_MAX];
	icons[eDV_BREAKPOINTS] = QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/breakpoints.png");
	icons[eDV_CALLSTACK] = QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/callStack.png");
	icons[eDV_WATCH] = QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/watch.png");
	
	for (int ii = 0; ii < eDV_MAX; ii++)
	{
		EDebuggerViewID dockId = (EDebuggerViewID)ii;

        m_DockWidgets[dockId]->setVisible(false);

		pToggleViewAction = m_DockWidgets[dockId]->toggleViewAction();
		if (pToggleViewAction)
		{
			pToggleViewAction->setIcon(QPixmap(icons[dockId]));
			pToggleViewAction->setIconVisibleInMenu(false);
			if (rbxMainWindow->isRibbonStyle())
			{
				QString actionName(m_DockWidgets[dockId]->objectName()+"Action");
				pToggleViewAction->setObjectName(actionName);
				UpdateUIManager::defaultCommands.append(" ");
				UpdateUIManager::defaultCommands.append(actionName);

				rbxMainWindow->addAction(pToggleViewAction);
			}
			else
			{
				rbxMainWindow->viewToolsToolBar->addAction(pToggleViewAction);
			}
		}
	}

	//
	m_cConnections.push_back(DebuggerClientManager::Instance().debuggingStarted.connect(boost::bind(&DebuggerViewsManager::onDebuggingStarted, this)));
	m_cConnections.push_back(DebuggerClientManager::Instance().debuggingStopped.connect(boost::bind(&DebuggerViewsManager::onDebuggingStopped, this)));

	//if debugger client get's changed (update dock widgets)
	m_cConnections.push_back(DebuggerClientManager::Instance().clientActivated.connect(boost::bind(&DebuggerViewsManager::onDebuggerClientActivated, this, _1, _2)));
	m_cConnections.push_back(DebuggerClientManager::Instance().clientDeactivated.connect(boost::bind(&DebuggerViewsManager::onDebuggerClientDeactivated, this)));

	//update dock widgets whenever exeuction gets paused or started
	m_cConnections.push_back(DebuggerClientManager::Instance().executionDataCleared.connect(boost::bind(&DebuggerViewsManager::onExecutionDataCleared, this)));
	m_cConnections.push_back(DebuggerClientManager::Instance().breakpointEncountered.connect(boost::bind(&DebuggerViewsManager::onBreakpointEncountered, this, _1, _2)));

	//clear all data
	m_cConnections.push_back(DebuggerClientManager::Instance().clearAll.connect(boost::bind(&DebuggerViewsManager::onClearAll, this)));

	//breakpoint
	m_cConnections.push_back(DebuggerClientManager::Instance().breakpointAdded.connect(boost::bind(&DebuggerViewsManager::onBreakpointAdded, this, _1)));
	m_cConnections.push_back(DebuggerClientManager::Instance().breakpointRemoved.connect(boost::bind(&DebuggerViewsManager::onBreakpointRemoved, this, _1)));

	//watch
	m_cConnections.push_back(DebuggerClientManager::Instance().watchAdded.connect(boost::bind(&DebuggerViewsManager::onWatchAdded, this, _1)));
	m_cConnections.push_back(DebuggerClientManager::Instance().watchRemoved.connect(boost::bind(&DebuggerViewsManager::onWatchRemoved, this, _1)));

	//debuggers paused with delay
	m_cConnections.push_back(DebuggerClientManager::Instance().debuggersListUpdated.connect(boost::bind(&DebuggerViewsManager::onDebuggersListUpdated, this, _1)));
}

void DebuggerViewsManager::showDockWidget(EDebuggerViewID dockId)
{
	QDockWidget* pDockWidget = m_DockWidgets[dockId];
	if (pDockWidget)
	{
		pDockWidget->show();
		pDockWidget->raise();
	}
}

void DebuggerViewsManager::restoreViews()
{
	//TODO: show only the last shown views
	for (int dockId = 1; dockId < eDV_MAX; dockId++)
	{		
		m_DockWidgets[(EDebuggerViewID)dockId]->show();
	}
}

void DebuggerViewsManager::saveViews()
{
	//TODO: save currently shown views
	for (int dockId = 1; dockId < eDV_MAX; dockId++)
	{
		m_DockWidgets[(EDebuggerViewID)dockId]->close();
	}
}

void DebuggerViewsManager::onDebuggingStarted()
{
	m_bBreakpointEncountered = false;
}

void DebuggerViewsManager::onDebuggingStopped()
{
	for (int dockId = 1; dockId < eDV_MAX; dockId++)
	{
		QMetaObject::invokeMethod(m_DockWidgets[(EDebuggerViewID)dockId], "close", Qt::QueuedConnection);
	}
}

void DebuggerViewsManager::onDebuggerClientActivated(int currentBreakpoint, RBX::Scripting::ScriptDebugger::Stack currentStack)
{
	getViewWidget<BreakpointsWidget>(eDV_BREAKPOINTS).setCurrentBreakpoint(currentBreakpoint);
	getViewWidget<WatchWidget>(eDV_WATCH).updateWatchValues(true);
}

void DebuggerViewsManager::onDebuggerClientDeactivated()
{
	// deactivate watch values
	getViewWidget<WatchWidget>(eDV_WATCH).updateWatchValues(false);
}

void DebuggerViewsManager::onExecutionDataCleared()
{
	getViewWidget<CallStackWidget>(eDV_CALLSTACK).clearDebuggers();
	getViewWidget<CallStackWidget>(eDV_CALLSTACK).clear();
	getViewWidget<BreakpointsWidget>(eDV_BREAKPOINTS).setCurrentBreakpoint(-1);
	getViewWidget<WatchWidget>(eDV_WATCH).updateWatchValues(false);
}

void DebuggerViewsManager::onBreakpointEncountered(int currentBreakpointLine, DebuggerClient* pDebuggerClient)
{
	if (!m_bBreakpointEncountered)
	{
		for (int dockId = 1; dockId < eDV_MAX; dockId++)
		{
			QMetaObject::invokeMethod(m_DockWidgets[(EDebuggerViewID)dockId], "show", Qt::QueuedConnection);
		}

		m_bBreakpointEncountered = true;
	}

	getViewWidget<CallStackWidget>(eDV_CALLSTACK).populateDebuggers(pDebuggerClient);
	getViewWidget<BreakpointsWidget>(eDV_BREAKPOINTS).setCurrentBreakpoint(currentBreakpointLine);
	getViewWidget<WatchWidget>(eDV_WATCH).updateWatchValues(true);
}

void DebuggerViewsManager::onClearAll()
{
	getViewWidget<CallStackWidget>(eDV_CALLSTACK).clearDebuggers();
	getViewWidget<CallStackWidget>(eDV_CALLSTACK).clear();
	getViewWidget<BreakpointsWidget>(eDV_BREAKPOINTS).clear();
	getViewWidget<WatchWidget>(eDV_WATCH).clear();

	for (int dockId = 1; dockId < eDV_MAX; dockId++)
	{
		m_DockWidgets[(EDebuggerViewID)dockId]->close();
	}
}

void DebuggerViewsManager::onBreakpointAdded(shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{
	getViewWidget<BreakpointsWidget>(eDV_BREAKPOINTS).addBreakpoint(spBreakpoint);
}

void DebuggerViewsManager::onBreakpointRemoved(shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{
	getViewWidget<BreakpointsWidget>(eDV_BREAKPOINTS).removeBreakpoint(spBreakpoint);
}

void DebuggerViewsManager::onWatchAdded(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	getViewWidget<WatchWidget>(eDV_WATCH).addWatch(spWatch);
}

void DebuggerViewsManager::onWatchRemoved(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	getViewWidget<WatchWidget>(eDV_WATCH).removeWatch(spWatch);
}

void DebuggerViewsManager::onDebuggersListUpdated(DebuggerClient* pDebuggerClient)
{
	getViewWidget<CallStackWidget>(eDV_CALLSTACK).populateDebuggers(pDebuggerClient);
}

//--------------------------------------------------------------------------------------------
// NoEditDelegate
//--------------------------------------------------------------------------------------------
class NoEditDelegate: public QStyledItemDelegate 
{
public:
	NoEditDelegate(QObject* parent=0):QStyledItemDelegate(parent) 
	{;}

	virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const 
	{ return NULL; }
};

//--------------------------------------------------------------------------------------------
// DebuggerTreeWidget
//--------------------------------------------------------------------------------------------
DebuggerTreeWidget::DebuggerTreeWidget(QWidget* pParent)
: QTreeWidget(pParent)
{
	setUniformRowHeights(true);
	setRootIsDecorated(false);

	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);

	setItemDelegate(new NoEditDelegate(this));

	connect(this, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(onItemActivated(QTreeWidgetItem*)));
	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemActivated(QTreeWidgetItem*)));
	connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
}

void DebuggerTreeWidget::onItemActivated(QTreeWidgetItem* pItem)
{
	DebuggerTreeWidgetItem* pDebuggerItem = dynamic_cast<DebuggerTreeWidgetItem*>(pItem);
	if(pDebuggerItem)
		pDebuggerItem->activate();
}

void DebuggerTreeWidget::onItemChanged(QTreeWidgetItem* pItem, int column)
{
	DebuggerTreeWidgetItem* pDebuggerItem = dynamic_cast<DebuggerTreeWidgetItem*>(pItem);
	if(pDebuggerItem)
		pDebuggerItem->columnModified(column);
}

void DebuggerTreeWidget::mousePressEvent(QMouseEvent *evt)
{
	if (!itemAt(evt->pos()))
		setCurrentItem(NULL);
	else
		QTreeWidget::mousePressEvent(evt);
}

void DebuggerTreeWidget::keyPressEvent(QKeyEvent * evt)
{
	if (evt->key() == Qt::Key_Delete)
	{
		Q_EMIT deleteSelectedItem();
		evt->accept();
		return;
	}

	QTreeWidget::keyPressEvent(evt);
}

//--------------------------------------------------------------------------------------------
// StackDetailsItem
//--------------------------------------------------------------------------------------------
StackDetailsItem::StackDetailsItem(const RBX::Scripting::ScriptDebugger::FunctionInfo& functionInfo, bool isFirstItem)
: m_bIsFirstItem(isFirstItem)
{
	setData(1, Qt::DisplayRole, QVariant(functionInfo.frame));
	setText(2, QString::fromStdString(functionInfo.what));
	setText(3, QString::fromStdString(functionInfo.name));
	setData(4, Qt::DisplayRole, QVariant(functionInfo.currentline));
	setText(5, QString::fromStdString(functionInfo.short_src));
	setText(6, QString::fromStdString(functionInfo.namewhat));

	if (m_bIsFirstItem)
		setIcon(0, QIcon(":/16x16/images/Studio 2.0 icons/16x16/debugger_arrow.png"));

	m_spAssociatedScript = functionInfo.script;
}

StackDetailsItem::~StackDetailsItem()
{
	DebuggerClient* pDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(m_spAssociatedScript);
	if (pDebuggerClient)
		pDebuggerClient->setMarker(getLine() - 1, "", false);
}

void StackDetailsItem::activate()
{
	updateIcon(true);
	DebuggerClient* pDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(m_spAssociatedScript);
	if (pDebuggerClient)
		pDebuggerClient->setCurrentFrame(getFrame());
}

void StackDetailsItem::updateIcon(bool isCurrent)
{
	DebuggerClient* pDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(m_spAssociatedScript);
	if (!pDebuggerClient)
		return;

	//do not update anything in case of it's the first frame
	if (m_bIsFirstItem)
	{
		pDebuggerClient->setMarker(getLine() - 1, ":/16x16/images/Studio 2.0 icons/16x16/debugger_arrow.png", true);
		pDebuggerClient->setCurrentLine(getLine() - 1);
		return;
	}

	pDebuggerClient->setMarker(getLine() - 1, isCurrent ? ":/16x16/images/Studio 2.0 icons/16x16/debugger_arrow_curve_left.png" : "", true);
	setIcon(0, QPixmap(isCurrent ? ":/16x16/images/Studio 2.0 icons/16x16/debugger_arrow_curve_left.png" : ""));
}

int StackDetailsItem::getLine()
{ return data(4, Qt::DisplayRole).toInt(); }

int StackDetailsItem::getFrame()
{ return data(1, Qt::DisplayRole).toInt(); }

//--------------------------------------------------------------------------------------------
// CallStackTreeWidget
//--------------------------------------------------------------------------------------------
CallStackTreeWidget::CallStackTreeWidget(QWidget* pParent)
: DebuggerTreeWidget(pParent)
, m_pPreviousItem(NULL)
{
	setWindowTitle("Call Stack");
	setSortingEnabled(false);

	QStringList headerLabels;
	headerLabels<<" "<<"Frame"<<"What"<<"Function Name"<<"Line No."<<"Source"<<"Function Explanation";
	setHeaderLabels(headerLabels);

	header()->setResizeMode(0,QHeaderView::Interactive);
	header()->setStretchLastSection(true);
}

CallStackTreeWidget::~CallStackTreeWidget()
{
}

void CallStackTreeWidget::setCallStack(const RBX::Scripting::ScriptDebugger::Stack& stack)
{
	m_pPreviousItem = NULL;
	clear();

	for(RBX::Scripting::ScriptDebugger::Stack::const_iterator iter = stack.begin(); iter != stack.end(); ++iter)
		addTopLevelItem(new StackDetailsItem(*iter, iter==stack.begin()));

	if (!stack.empty())
	{
		m_pPreviousItem = topLevelItem(0);
		static_cast<StackDetailsItem *>(m_pPreviousItem)->updateIcon(true);
	}

	header()->resizeSections(QHeaderView::ResizeToContents);
}

void CallStackTreeWidget::onItemActivated(QTreeWidgetItem* pItem)
{
	StackDetailsItem *pStackDetailsItem = dynamic_cast<StackDetailsItem *>(m_pPreviousItem);
	if (pStackDetailsItem)
		pStackDetailsItem->updateIcon(false);

	DebuggerTreeWidget::onItemActivated(pItem);
	m_pPreviousItem = pItem;

	setCurrentItem(NULL);
}

//--------------------------------------------------------------------------------------------
// CallStackDebuggersList
//--------------------------------------------------------------------------------------------
CallStackDebuggersList::CallStackDebuggersList(QWidget* pWidget)
: QComboBox(pWidget)
, m_bIsDirty(false)
{
}

void CallStackDebuggersList::showPopup()
{
	Q_EMIT popupAboutToShow();
	QComboBox::showPopup();
}

//--------------------------------------------------------------------------------------------
// CallStackWidget
//--------------------------------------------------------------------------------------------
CallStackWidget::CallStackWidget(QWidget* pParent)
: m_pCurrentDebuggerClient(NULL)
{
	m_pCallStackTreeWidget = new CallStackTreeWidget(this);

	QLabel* pLabel = new QLabel("Current Script: ", this);
	
	m_pScriptDebuggersList = new CallStackDebuggersList(this);
	connect(m_pScriptDebuggersList, SIGNAL(popupAboutToShow()), this, SLOT(onDebuggerListPopupShow()));
	
	//m_pScriptDebuggersList->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    m_pScriptDebuggersList->setMinimumWidth(200);
    m_pScriptDebuggersList->setMaximumWidth(QWIDGETSIZE_MAX);

	QSpacerItem *pHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	QHBoxLayout* pComboLabelLayout = new QHBoxLayout;
	pComboLabelLayout->addWidget(pLabel);
	pComboLabelLayout->addWidget(m_pScriptDebuggersList);
	pComboLabelLayout->addItem(pHorizontalSpacer);
	
	QGridLayout *pGridLayout = new QGridLayout(this);
	pGridLayout->setSpacing(4);
	pGridLayout->setContentsMargins(0, 0, 0, 0);
	pGridLayout->addLayout(pComboLabelLayout, 0, 0, 1, 1);
	pGridLayout->addWidget(m_pCallStackTreeWidget, 1, 0, 1, 1);
}

void CallStackWidget::populateDebuggers(DebuggerClient* pDebuggerClient)
{
	// if there's no change in debugger client then mark debuggers list as dirty and return
	if ( DebuggerClientManager::Instance().getCurrentDebuggerClient() && 
		 (m_pCurrentDebuggerClient == pDebuggerClient) &&
		 (m_pScriptDebuggersList->count() > 0) )
	{
		static_cast<CallStackDebuggersList*>(m_pScriptDebuggersList)->setDirty(true);
		return;
	}

	QString scriptName;
	DebuggerClient* currentDebuggerClient = NULL;
	boost::shared_ptr<RBX::Instance> script;
	bool indexSet = false;

	m_pCurrentDebuggerClient = pDebuggerClient;

	// first clear list widget
	clearDebuggers();
	clear();

	// now update combo and selection of index will update list widget
	const std::vector<DebuggerClient *>& debuggerClients = DebuggerClientManager::Instance().getDebuggerClients();
	for (unsigned int ii=0; ii < debuggerClients.size(); ++ii)
	{
		currentDebuggerClient = debuggerClients[ii];
		if (!currentDebuggerClient || !RBX::Scripting::DebuggerManager::singleton().findDebugger(currentDebuggerClient->getScript().get()))
			continue;

		script = currentDebuggerClient->getScript();
		if (!script)
			continue;

		scriptName = script->getFullName().c_str();

		const RBX::Scripting::ScriptDebugger::PausedThreads& pausedThreads = currentDebuggerClient->getPausedThreads();
		for (RBX::Scripting::ScriptDebugger::PausedThreads::const_iterator it=pausedThreads.begin(); it!=pausedThreads.end(); ++it)
		{
			m_pScriptDebuggersList->addItem(QString("%1 (%2) - %3").arg(scriptName).arg(it->first, 1, 16).arg(it->second.pausedLine), 
				QVariant::fromValue<void*>((void*)script.get()));

			if ((currentDebuggerClient == m_pCurrentDebuggerClient) && (currentDebuggerClient->getCurrentThread() == it->first))
			{
				currentDebuggerClient->setCurrentThread(currentDebuggerClient->getCurrentThread());
				m_pScriptDebuggersList->setCurrentIndex(m_pScriptDebuggersList->count() - 1);
				setCallStack(currentDebuggerClient->getCurrentCallStack());
				indexSet = true;
			}
		}
	}

	// if index is not set then make sure we set an index
	if (!indexSet && m_pScriptDebuggersList->count() > 0)
		onCurrentIndexChanged(0);

	connect(m_pScriptDebuggersList, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
}

void CallStackWidget::clearDebuggers()
{ 
	disconnect(m_pScriptDebuggersList, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
	m_pScriptDebuggersList->clear();
	static_cast<CallStackDebuggersList*>(m_pScriptDebuggersList)->setDirty(false);
}

void CallStackWidget::setCallStack(const RBX::Scripting::ScriptDebugger::Stack& stack)
{ m_pCallStackTreeWidget->setCallStack(stack); }

void CallStackWidget::clear()
{ m_pCallStackTreeWidget->clear(); }

void CallStackWidget::onCurrentIndexChanged(int index)
{
	QVariant var = m_pScriptDebuggersList->itemData(index);
	boost::shared_ptr<RBX::Script> spScript = shared_from(static_cast<RBX::Script*>(var.value<void*>()));
	if (spScript)
	{
		DebuggerClient* pDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(spScript);
		if (pDebuggerClient)
		{
			// update current thread
			QString text = m_pScriptDebuggersList->itemText(index);
			QRegExp exp("\\((.*)\\)");
			if(exp.indexIn(text) >= 0)
			{
				int threadID = exp.cap(1).toInt(0, 16);
				pDebuggerClient->setCurrentThread(threadID);
			}

			if (pDebuggerClient->updateDocument())
				setCallStack(pDebuggerClient->getCurrentCallStack()); // if document has been updated correctly, set current callstack
			else
				clear(); // else clear the previous stack details
		}
	}
}

void CallStackWidget::onDebuggerListPopupShow()
{
	// check if we need to update debuggers list
	if (!static_cast<CallStackDebuggersList*>(m_pScriptDebuggersList)->isDirty())
		return;

	// force debuggers list population!
	clearDebuggers();
	populateDebuggers(DebuggerClientManager::Instance().getCurrentDebuggerClient());
}

//--------------------------------------------------------------------------------------------
// BreakpointItem
//--------------------------------------------------------------------------------------------
BreakpointItem::BreakpointItem(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
: m_spBreakpoint(spBreakpoint)
, m_bIgnoreBreakpointModification(false)
, m_bIgnoreWaypointRequest(false)
{
	setFlags(flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
	setCheckState(0, m_spBreakpoint->isEnabled() ? Qt::Checked : Qt::Unchecked);
	
	int line = m_spBreakpoint->getLine();
	setData(1, Qt::DisplayRole, QVariant(line));
	RBX::Scripting::ScriptDebugger* spDebugger = RBX::Instance::fastDynamicCast<RBX::Scripting::ScriptDebugger>(spBreakpoint->getParent());
	if (spDebugger)
	{
		setText(2, spDebugger->getScript()->getFullName().c_str());
		if (DebuggerClient *pClient = DebuggerClientManager::Instance().getDebuggerClient(shared_from(spDebugger->getScript())))
			setText(3, pClient->getSourceCodeAtLine(line).trimmed());
	}
	//setText(4, QString::fromStdString(m_spBreakpoint->getCondition()));
	m_cPropertyChangedConnection = m_spBreakpoint->propertyChangedSignal.connect(boost::bind(&BreakpointItem::onBreakpointPropertyChanged, this, _1));
	m_cAncestoryChangedConnection = m_spBreakpoint->ancestryChangedSignal.connect(boost::bind(&BreakpointItem::onScriptAncestryChanged, this, _1));
}

BreakpointItem::~BreakpointItem()
{
	m_cPropertyChangedConnection.disconnect();
	m_cAncestoryChangedConnection.disconnect();
}

bool BreakpointItem::hasBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{ return (spBreakpoint == m_spBreakpoint); }

void BreakpointItem::columnModified(int column)
{
	RBX::ScopedAssign<bool> ignoreBreakpointModificationEvent(m_bIgnoreBreakpointModification, true);

	if (column == 0)
	{
		RBX::DataModel::LegacyLock lock(DebuggerClientManager::Instance().getDataModel(), RBX::DataModelJob::Write);
		RBX::Scripting::DebuggerBreakpoint::prop_Enabled.setValue(m_spBreakpoint.get(), checkState(column) == Qt::Checked);

		// update status in script doc
		// since this is the only way to update enabled/disabled state of a breakpoint, so we can avoid using property signal
		DebuggerClientManager::Instance().syncBreakpointState(m_spBreakpoint);
	}
	else if (column == 4)
	{
		QString modifiedCondition = text(3);
		if(modifiedCondition != QString::fromStdString(m_spBreakpoint->getCondition()))
		{
			RBX::DataModel::LegacyLock lock(DebuggerClientManager::Instance().getDataModel(), RBX::DataModelJob::Write);
			RBX::Scripting::DebuggerBreakpoint::prop_Condition.setValue(m_spBreakpoint.get(), modifiedCondition.toStdString());
		}
	}
}

void BreakpointItem::activate()
{
	//TODO: Check if we need to highlight line with a different color?
	RBX::Scripting::ScriptDebugger* pDebugger = RBX::Instance::fastDynamicCast<RBX::Scripting::ScriptDebugger>(m_spBreakpoint->getParent());
	if (pDebugger)
	{
		RobloxDocManager::Instance().openDoc(LuaSourceBuffer::fromInstance(shared_from(pDebugger->getScript())));

		DebuggerClient* pDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(shared_from(pDebugger->getScript()));
		if (pDebuggerClient)
			pDebuggerClient->setCurrentLine(m_spBreakpoint->getLine() - 1);
	}
}

void BreakpointItem::deleteBreakpoint()
{
	RBX::DataModel::LegacyLock lock(DebuggerClientManager::Instance().getDataModel(), RBX::DataModelJob::Write);
	m_spBreakpoint->setParent(NULL);
	m_spBreakpoint.reset();
}

void BreakpointItem::setBreakpointEnabled(bool state)
{
	RBX::ScopedAssign<bool> ignoreBreakpointModificationEvent(m_bIgnoreWaypointRequest, true);
	setCheckState(0, state ? Qt::Checked : Qt::Unchecked); 
}

bool BreakpointItem::isBreakpointEnabled()
{ return checkState(0) == Qt::Checked; }

void BreakpointItem::onBreakpointPropertyChanged(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	if (m_bIgnoreBreakpointModification)
		return;

	//TODO: make sure this is called from Main Thread (issues can happen if breakpoints are set from a script)
	if (*pPropertyDescriptor == RBX::Scripting::DebuggerBreakpoint::prop_Enabled)
		setBreakpointEnabled(m_spBreakpoint->isEnabled());
}

void BreakpointItem::onScriptAncestryChanged(shared_ptr<RBX::Instance> newParent)
{
	// update breakpoint name if the script has change in hierarchy
	if (newParent)
	{
		RBX::Scripting::ScriptDebugger* spDebugger = RBX::Instance::fastDynamicCast<RBX::Scripting::ScriptDebugger>(m_spBreakpoint->getParent());
		if (spDebugger)
			setText(2, spDebugger->getScript()->getFullName().c_str());
	}
}

//--------------------------------------------------------------------------------------------
// BreakpointsTreeWidget
//--------------------------------------------------------------------------------------------
BreakpointsTreeWidget::BreakpointsTreeWidget(QWidget* pParent)
: DebuggerTreeWidget(pParent)
{
	//set columns
	QStringList headerLabels;
	headerLabels<<"Enabled"<<"Line"<<"Script Name"<<"Source Line";//<<"Condition";
	setHeaderLabels(headerLabels);

	header()->setResizeMode(0,QHeaderView::Interactive);
	header()->setStretchLastSection(true);

	//setItemDelegateForColumn(3, new QStyledItemDelegate(this));

	//enable sorting
	sortByColumn(2, Qt::AscendingOrder);
	setSortingEnabled(true);
}

void BreakpointsTreeWidget::addBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{
	if(spBreakpoint && !findBreakpointItem(spBreakpoint))
		addTopLevelItem( new BreakpointItem(spBreakpoint) );
}

void BreakpointsTreeWidget::removeBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{
	if(spBreakpoint)
	{
		BreakpointItem *pBPItem = findBreakpointItem(spBreakpoint);
		if(pBPItem)
			delete pBPItem;
	}
}

void BreakpointsTreeWidget::setCurrentBreakpoint(int breakpointLine)
{
	if (breakpointLine < 0)
	{
		clearSelection();
	}
	else
	{
		BreakpointItem *pBPItem = findBreakpointItem(breakpointLine);
		if (pBPItem)
		{
			scrollToItem(pBPItem);
			setCurrentItem(pBPItem);
		}
	}
}

void BreakpointsTreeWidget::deleteSelectedBreakpoint()
{
	QList<QTreeWidgetItem*> selItems = selectedItems();
	if (selItems.length() > 0)
	{
		BreakpointItem* pBreakpointItem = dynamic_cast<BreakpointItem*>(selItems[0]);
		if (pBreakpointItem)
		{
			pBreakpointItem->deleteBreakpoint();
			delete pBreakpointItem;
		}
	}
}

void BreakpointsTreeWidget::deleteAllBreakpoints()
{
	int numChild = 	topLevelItemCount();
	if (numChild <=0)
		return;

	int currentChild = 0;
	BreakpointItem* pBreakpointItem = NULL;

	while (currentChild < numChild)
	{
		pBreakpointItem = static_cast<BreakpointItem*>(topLevelItem(currentChild));
		if (pBreakpointItem)
			pBreakpointItem->deleteBreakpoint();
		++currentChild;
	}

	clear();
}

void BreakpointsTreeWidget::disableEnableAllBreakpoints()
{
	int numChild = 	topLevelItemCount();
	if (numChild <=0)
		return;

	int currentChild = 0;
	BreakpointItem* pBreakpointItem = NULL;

	bool breakpointEnabled = hasBreakpointsEnabled();

	while (currentChild < numChild)
	{
		pBreakpointItem = static_cast<BreakpointItem*>(topLevelItem(currentChild));
		if (pBreakpointItem && (breakpointEnabled == pBreakpointItem->isBreakpointEnabled()))
			pBreakpointItem->setBreakpointEnabled(!breakpointEnabled);
		++currentChild;
	}
}

void BreakpointsTreeWidget::goToSourceCode()
{
	QList<QTreeWidgetItem*> selItems = selectedItems();
	if (selItems.length() > 0)
	{
		BreakpointItem* pBreakpointItem = static_cast<BreakpointItem*>(selItems[0]);
		if (pBreakpointItem)
			pBreakpointItem->activate();
	}	
}

BreakpointItem* BreakpointsTreeWidget::findBreakpointItem(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{
	int numChild = 	topLevelItemCount(), currentChild = 0;
	BreakpointItem* pBreakpointItem = NULL;

	while (currentChild < numChild)
	{
		//static cast should be OK as we are inserting only BreakpointItem type of items!
		pBreakpointItem = static_cast<BreakpointItem*>(topLevelItem(currentChild));
		if (pBreakpointItem && pBreakpointItem->hasBreakpoint(spBreakpoint))
			return pBreakpointItem;
		++currentChild;
	}

	return NULL;
}

BreakpointItem* BreakpointsTreeWidget::findBreakpointItem(int breakpointLine)
{
	int numChild = 	topLevelItemCount(), currentChild = 0;
	BreakpointItem* pBreakpointItem = NULL;

	while (currentChild < numChild)
	{
		//static cast should be OK as we are inserting only BreakpointItem type of items!
		pBreakpointItem = static_cast<BreakpointItem*>(topLevelItem(currentChild));
		if ( pBreakpointItem && (pBreakpointItem->data(1, Qt::DisplayRole).toInt() == breakpointLine) )
			return pBreakpointItem;
		++currentChild;
	}

	return NULL;
}

bool BreakpointsTreeWidget::hasBreakpointsEnabled()
{
	int numChild = 	topLevelItemCount(), currentChild = 0;
	BreakpointItem* pBreakpointItem = NULL;

	while (currentChild < numChild)
	{
		//static cast should be OK as we are inserting only BreakpointItem type of items!
		pBreakpointItem = static_cast<BreakpointItem*>(topLevelItem(currentChild));
		if ( pBreakpointItem && pBreakpointItem->isBreakpointEnabled() )
			return true;
		++currentChild;
	}

	return false;
}

//--------------------------------------------------------------------------------------------
// BreakpointsWidget
//--------------------------------------------------------------------------------------------
BreakpointsWidget::BreakpointsWidget(QWidget* pParent)
: QWidget(pParent)
, m_bPushButtonStateUpdateRequested(false)
, m_bIgnoreBreakpointModification(false)
{
	QSizePolicy pushButtonSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	pushButtonSizePolicy.setHorizontalStretch(0);
	pushButtonSizePolicy.setVerticalStretch(0);

	m_pDeleteBreakpointTB = new QToolButton(this);
	m_pDeleteBreakpointTB->setObjectName(QString::fromUtf8("DeleteBreakpointPB"));
	m_pDeleteBreakpointTB->setSizePolicy(pushButtonSizePolicy);
	m_pDeleteBreakpointTB->setToolTip(tr("Delete Selected Breakpoint"));
	m_pDeleteBreakpointTB->setEnabled(false);

	QIcon icon;
	icon.addFile(QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/breakpointDelete.png"), QSize(), QIcon::Normal, QIcon::Off);
	m_pDeleteBreakpointTB->setIcon(icon);

	QFrame *pLine1 = new QFrame(this);
	pLine1->setFrameShape(QFrame::VLine);
    pLine1->setFrameShadow(QFrame::Sunken);
	
	m_pDeleteAllBreakpointsTB = new QToolButton(this);
	m_pDeleteAllBreakpointsTB->setObjectName(QString::fromUtf8("DeleteAllBreakpointsPB"));
	m_pDeleteAllBreakpointsTB->setSizePolicy(pushButtonSizePolicy);
	m_pDeleteAllBreakpointsTB->setToolTip(tr("Delete All Breakpoints"));
	QIcon icon1;
	icon1.addFile(QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/deleteAllBreakpoints.png"), QSize(), QIcon::Normal, QIcon::Off);
	m_pDeleteAllBreakpointsTB->setIcon(icon1);
	m_pDeleteAllBreakpointsTB->setEnabled(false);
	
	m_pDisableEnableAllBreakpointsTB = new QToolButton(this);
	m_pDisableEnableAllBreakpointsTB->setObjectName(QString::fromUtf8("DisableEnableAllBreakpointsPB"));
	m_pDisableEnableAllBreakpointsTB->setSizePolicy(pushButtonSizePolicy);
	QIcon icon2;
	icon2.addFile(QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/enableDisableBreakpoints.png"), QSize(), QIcon::Normal, QIcon::Off);
	m_pDisableEnableAllBreakpointsTB->setIcon(icon2);
	m_pDisableEnableAllBreakpointsTB->setEnabled(false);

	QFrame *pLine2 = new QFrame(this);
	pLine2->setFrameShape(QFrame::VLine);
    pLine2->setFrameShadow(QFrame::Sunken);

	m_pGoToSourceCodeTB = new QToolButton(this);
	m_pGoToSourceCodeTB->setObjectName(QString::fromUtf8("GoToSourceCodeTB"));
	m_pGoToSourceCodeTB->setSizePolicy(pushButtonSizePolicy);
	QIcon icon3;
	icon3.addFile(QString::fromUtf8(":/16x16/images/Studio 2.0 icons/16x16/gotoSource.png"), QSize(), QIcon::Normal, QIcon::Off);
	m_pGoToSourceCodeTB->setIcon(icon3);
	m_pGoToSourceCodeTB->setToolTip(tr("Go To Source Code"));
	m_pGoToSourceCodeTB->setEnabled(false);

	QSpacerItem *pHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	QHBoxLayout *pHorizontalLayout = new QHBoxLayout();
	pHorizontalLayout->setSpacing(4);

	pHorizontalLayout->addWidget(m_pDeleteBreakpointTB);
	pHorizontalLayout->addWidget(pLine1);
	pHorizontalLayout->addWidget(m_pDeleteAllBreakpointsTB);
	pHorizontalLayout->addWidget(m_pDisableEnableAllBreakpointsTB);
	pHorizontalLayout->addWidget(pLine2);
	pHorizontalLayout->addWidget(m_pGoToSourceCodeTB);
	pHorizontalLayout->addItem(pHorizontalSpacer);
	
	m_pBreakpointTreeWidget = new BreakpointsTreeWidget(this);

	QGridLayout *pGridLayout = new QGridLayout(this);
	pGridLayout->setVerticalSpacing(4);
	pGridLayout->setContentsMargins(0, 0, 0, 0);

	pGridLayout->addLayout(pHorizontalLayout, 0, 0, 1, 1);
	pGridLayout->addWidget(m_pBreakpointTreeWidget, 1, 0, 1, 1);

	connect(m_pDeleteBreakpointTB, SIGNAL(clicked()), this, SLOT(onPushButtonClicked()));
	connect(m_pDeleteAllBreakpointsTB, SIGNAL(clicked()), this, SLOT(onPushButtonClicked()));
	connect(m_pDisableEnableAllBreakpointsTB, SIGNAL(clicked()), this, SLOT(onPushButtonClicked()));
	connect(m_pGoToSourceCodeTB, SIGNAL(clicked()), this, SLOT(onPushButtonClicked()));

	connect(UpdateUIManager::Instance().getMainWindow().toggleAllBreakpointsStateAction, SIGNAL(triggered()), m_pDisableEnableAllBreakpointsTB, SIGNAL(clicked()));

	connect(m_pBreakpointTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(requestButtonsStatusUpdate()));
	connect(m_pBreakpointTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(requestButtonsStatusUpdate()));
	connect(m_pBreakpointTreeWidget, SIGNAL(deleteSelectedItem()), this, SLOT(onDeleteSelectedItem()));
}

void BreakpointsWidget::addBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint)
{ 
	if (!m_bIgnoreBreakpointModification)
		m_pBreakpointTreeWidget->addBreakpoint(breakpoint);
	requestButtonsStatusUpdate();
}

void BreakpointsWidget::removeBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint)
{ 
	if (!m_bIgnoreBreakpointModification)
		m_pBreakpointTreeWidget->removeBreakpoint(breakpoint); 
	requestButtonsStatusUpdate();
}

void BreakpointsWidget::setCurrentBreakpoint(int breakpointLine)
{ 
	m_pBreakpointTreeWidget->setCurrentBreakpoint(breakpointLine);
	requestButtonsStatusUpdate();
}

void BreakpointsWidget::clear()
{ 
	m_pBreakpointTreeWidget->clear();
	requestButtonsStatusUpdate();
}

void BreakpointsWidget::onPushButtonClicked()
{
	QToolButton *pButtonClicked = qobject_cast<QToolButton*>(sender());
	if (!pButtonClicked)
		return;

	RBX::ScopedAssign<bool> ignorePropertyModifyEvent(m_bIgnoreBreakpointModification, true);

	if (pButtonClicked == m_pDeleteBreakpointTB)
	{
		m_pBreakpointTreeWidget->deleteSelectedBreakpoint();
	}
	else if (pButtonClicked == m_pDeleteAllBreakpointsTB)
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setText(tr("Do you want to delete all breakpoints?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if ( msgBox.exec() != QMessageBox::Yes )
			return;

		m_pBreakpointTreeWidget->deleteAllBreakpoints();
	}
	else if (pButtonClicked == m_pDisableEnableAllBreakpointsTB)
	{
		m_pBreakpointTreeWidget->disableEnableAllBreakpoints();
	}
	else if (pButtonClicked == m_pGoToSourceCodeTB)
	{
		m_pBreakpointTreeWidget->goToSourceCode();
	}

	requestButtonsStatusUpdate();
}

void BreakpointsWidget::onDeleteSelectedItem()
{
	RBX::ScopedAssign<bool> ignorePropertyModifyEvent(m_bIgnoreBreakpointModification, true);
	m_pBreakpointTreeWidget->deleteSelectedBreakpoint();
}

void BreakpointsWidget::requestButtonsStatusUpdate()
{
	if (m_bPushButtonStateUpdateRequested)
		return;

	m_bPushButtonStateUpdateRequested = true;
	QTimer::singleShot(0, this, SLOT(updateButtonsStatus()));
}

void BreakpointsWidget::updateButtonsStatus()
{
	int numItems = m_pBreakpointTreeWidget->topLevelItemCount();
	if (numItems < 1)
	{
		//disable all buttons
		m_pDeleteBreakpointTB->setEnabled(false);
		m_pDeleteAllBreakpointsTB->setEnabled(false);
		m_pDisableEnableAllBreakpointsTB->setEnabled(false);
		m_pGoToSourceCodeTB->setEnabled(false);

		//update connected action also
		UpdateUIManager::Instance().getMainWindow().toggleAllBreakpointsStateAction->setEnabled(false);
	}
	else
	{
		//enable m_pDeleteAllBreakpointsTB button if we have any item
		m_pDeleteAllBreakpointsTB->setEnabled(true);

		//if we have any enabled breakpoint then enable m_pDeleteAllBreakpointsTB
		m_pDisableEnableAllBreakpointsTB->setEnabled(true);
		if (m_pBreakpointTreeWidget->hasBreakpointsEnabled())
			m_pDisableEnableAllBreakpointsTB->setToolTip(tr("Disable All Breakpoints"));
		else
			m_pDisableEnableAllBreakpointsTB->setToolTip(tr("Enable All Breakpoints"));

		//update connected action also
		UpdateUIManager::Instance().getMainWindow().toggleAllBreakpointsStateAction->setEnabled(true);

		//if there are selected items, then enable m_pBreakpointTreeWidget and m_pGoToSourceCodeTB
		int numSelected = m_pBreakpointTreeWidget->selectedItems().length();
		m_pDeleteBreakpointTB->setEnabled(numSelected > 0);
		m_pGoToSourceCodeTB->setEnabled(numSelected > 0);
	}

	m_bPushButtonStateUpdateRequested = false;
}

//--------------------------------------------------------------------------------------------
// WatchItem
//--------------------------------------------------------------------------------------------
WatchItem::WatchItem(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
: m_spWatch(spWatch)
, m_bIsInValid(true)
{
	setFlags(flags() | Qt::ItemIsEditable);

	if (m_spWatch)
	{
		setText(0, QString::fromStdString(m_spWatch->getCondition()));
		onScriptAncestryChanged(shared_from(m_spWatch->getParent()));
		m_cPropertyChangedConnection = m_spWatch->propertyChangedSignal.connect(boost::bind(&WatchItem::onWatchPropertyChanged, this, _1));
		m_cAncestoryChangedConnection = m_spWatch->ancestryChangedSignal.connect(boost::bind(&WatchItem::onScriptAncestryChanged, this, _1));
	}
}

WatchItem::~WatchItem()
{
	m_cPropertyChangedConnection.disconnect();
	m_cAncestoryChangedConnection.disconnect();
}

void WatchItem::updateWatchValue()
{
	removeChildren();

	if (!m_spWatch)
		return;

	bool result = false;
	
	try
	{
		// TODO: Check if we need to get values from Active DebuggerClient or Current DebuggerClient?
		RBX::Reflection::Variant Value = DebuggerClientManager::Instance().getWatchValue(m_spWatch);
		if (!Value.isVoid())
			result = populateValues(this, QString::fromStdString(m_spWatch->getCondition()), Value);
	}

	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
		result = false;
	}

	setInvalid(!result);
}

bool WatchItem::hasWatch(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{ return m_spWatch == spWatch; }

void WatchItem::columnModified(int column)
{
	if (!m_spWatch)
		return;

	RBX::ScopedAssign<bool> ignoreWatchModificationEvent(m_bIgnoreWatchModification, true);
	if (column == 0)
	{
		std::string watchExpression = text(0).toStdString();
		if (watchExpression != m_spWatch->getExpression())
		{
			//TODO: Check for syntax of expression
			RBX::DataModel::LegacyLock lock(DebuggerClientManager::Instance().getDataModel(), RBX::DataModelJob::Write);
			RBX::Scripting::DebuggerWatch::prop_Expression.setValue(m_spWatch.get(), watchExpression);
			//also update the value
			updateWatchValue();
		}
	}
}

void WatchItem::deleteWatch()
{
	if (!m_spWatch)
		return;

	RBX::DataModel::LegacyLock lock(DebuggerClientManager::Instance().getDataModel(), RBX::DataModelJob::Write);
	m_spWatch->setParent(NULL);
	m_spWatch.reset();
}

void WatchItem::onWatchPropertyChanged(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
{
	if (m_bIgnoreWatchModification || !m_spWatch)
		return;

	if (*pPropertyDescriptor == RBX::Scripting::DebuggerWatch::prop_Expression)
		setText(0, QString::fromStdString(RBX::Scripting::DebuggerWatch::prop_Expression.getValue(m_spWatch.get())));
}

void WatchItem::removeChildren()
{
	QList<QTreeWidgetItem *> childItems = takeChildren();
	QList<QTreeWidgetItem *>::iterator iter = childItems.begin();

	while (iter != childItems.end())
	{
		delete *iter;
		++iter;
	}

	childItems.clear();
}

void WatchItem::setInvalid(bool state)
{ 
	m_bIsInValid = state;
	if (m_bIsInValid)
	{
		removeChildren();
		setText(1,  QString("Error symbol \"%1\" not found").arg(text(0)));
	}
}

void WatchItem::onScriptAncestryChanged(shared_ptr<RBX::Instance> newParent)
{
	// update script name if the script has change in hierarchy
	if (newParent)
	{
		QString scriptName;
		RBX::Scripting::ScriptDebugger* spDebugger = RBX::Instance::fastDynamicCast<RBX::Scripting::ScriptDebugger>(m_spWatch->getParent());
		if (spDebugger && spDebugger->getScript())
			scriptName = spDebugger->getScript()->getFullName().c_str();
		setText(2, scriptName);
	}
}

class DummyWatchItem: public WatchItem
{
public:
	DummyWatchItem()
	: WatchItem(boost::shared_ptr<RBX::Scripting::DebuggerWatch>())
	, m_bIgnoreTextModified(false)
	{
	}

	void columnModified(int column)
	{
		if (m_bIgnoreTextModified || column != 0)
			return;

		QString watchExpression = text(0);
		if (!watchExpression.isEmpty())
		{
			// add watch
			DebuggerClientManager::Instance().addWatch(watchExpression);

			// remove the text entered
			m_bIgnoreTextModified = true;
			setText(0, "");
			m_bIgnoreTextModified = false;
		}
	}

	void setInvalid(bool state) {}

private:
	bool   m_bIgnoreTextModified;
};

class WatchItemDelegate : public QStyledItemDelegate
{
public:
	WatchItemDelegate(WatchTreeWidget* parent)
	: QStyledItemDelegate(parent) 
	, m_pWatchTreeWidget(parent)
	{
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const 
	{ 
		if (index.column() != 0)
			return QStyledItemDelegate::createEditor(parent, option, index);

		// if we are in any doc other than script doc, then do not enable "DummyWatchItem"
		DummyWatchItem* dummyWatchItem = dynamic_cast<DummyWatchItem*>(m_pWatchTreeWidget->itemFromIndex(index));
		if (dummyWatchItem)
		{
			IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();
			if (pDoc && (pDoc->docType() != IRobloxDoc::SCRIPT))
			{
				RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Please open a script document to add a new watch expression");
				return NULL;
			}
		}

		// create line edit as editor
		QLineEdit *pLineEdit = new QLineEdit(parent);
		pLineEdit->setGeometry(option.rect);
		pLineEdit->setFrame(false);
		return pLineEdit;
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyledItemDelegate::paint(painter, option, index);
		
		painter->save();
        {
            painter->setPen(QPen(QColor("lightgray")));
            painter->drawLine(option.rect.right(), option.rect.y(), option.rect.right(), option.rect.bottom());
        }
		painter->restore();
	}

private:
	WatchTreeWidget*    m_pWatchTreeWidget;
};

class NoEditItemDelegate : public QStyledItemDelegate
{
public:
	NoEditItemDelegate(WatchTreeWidget* parent)
	: QStyledItemDelegate(parent) 
	{
	}

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const 
	{  return NULL; }

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyledItemDelegate::paint(painter, option, index);
		
		painter->save();
        {
            painter->setPen(QPen(QColor("lightgray")));
            painter->drawLine(option.rect.right(), option.rect.y(), option.rect.right(), option.rect.bottom());
        }
		painter->restore();
	}
};

//--------------------------------------------------------------------------------------------
// WatchTreeWidget
//--------------------------------------------------------------------------------------------
WatchTreeWidget::WatchTreeWidget(QWidget *pParent)
: DebuggerTreeWidget(pParent)
, m_pContextualMenu(NULL)
, m_pCopyAction(NULL)
, m_pAddWatchAction(NULL)
, m_pDeleteWatchAction(NULL)
, m_pDeleteAllWatchesAction(NULL)
, m_bIgnoreWatchModification(false)
{
	//set columns
	QStringList headerLabels;
	headerLabels<<"Expression"<<"Value"<<"Script Name";
	setHeaderLabels(headerLabels);

	header()->setResizeMode(0,QHeaderView::Interactive);
	header()->setResizeMode(1,QHeaderView::Interactive);
	header()->setStretchLastSection(true);
	header()->resizeSection(1, 200);

	//expressions can be modified
	setItemDelegateForColumn(0, new WatchItemDelegate(this));
	//make sure we draw columns correctly, so a no edit item is required
	setItemDelegateForColumn(1, new NoEditItemDelegate(this));

	//required to show child expansion controls (expand/collapse)
	setRootIsDecorated(true);

	//add dummywatch item, to add watch inline
	addDummyWatchItem();
}

void WatchTreeWidget::addWatch(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	if (!m_bIgnoreWatchModification)
	{
		if (spWatch && !findWatchItem(spWatch))
		{
			WatchItem* pWatchItem = new WatchItem(spWatch);
			//insert item before the last item
			insertTopLevelItem(topLevelItemCount() - 1, pWatchItem);
			//also update the value
			pWatchItem->updateWatchValue();
		}
	}
}

void WatchTreeWidget::removeWatch(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	if (!m_bIgnoreWatchModification && spWatch)
	{
		WatchItem *pWatchItem = findWatchItem(spWatch);
		if(pWatchItem)
			delete pWatchItem;
	}
}

void WatchTreeWidget::updateWatchValues(bool enable)
{
	try
	{
		int numChild = 	topLevelItemCount(), currentChild = 0;
		WatchItem* pWatchItem = NULL;

		setUpdatesEnabled(false);

		while (currentChild < numChild)
		{
			pWatchItem = static_cast<WatchItem*>(topLevelItem(currentChild));
			if (pWatchItem)
			{
				if (enable)
				{
					pWatchItem->updateWatchValue();
				}
				else
				{
					pWatchItem->setInvalid(true);
				}
			}
			++currentChild;
		}

		setUpdatesEnabled(true);
	}

	catch (std::runtime_error const& exp)
	{
		setUpdatesEnabled(true);
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	}
}

void WatchTreeWidget::deleteSelectedWatch()
{
	RBX::ScopedAssign<bool> ignoreWatchDeleteEvent(m_bIgnoreWatchModification, true);
	QList<QTreeWidgetItem*> selItems = selectedItems();
	if (selItems.length() > 0)
	{
		// make sure we do not delete DummyWatchItem
		WatchItem* pWatchItem = dynamic_cast<WatchItem*>(selItems[0]);
		DummyWatchItem* pDummyWatchItem = dynamic_cast<DummyWatchItem*>(selItems[0]);
		if (pWatchItem && !pDummyWatchItem)
		{
			pWatchItem->deleteWatch();
			delete pWatchItem;
		}
	}
}

void WatchTreeWidget::copySelectedItem()
{
	QList<QTreeWidgetItem*> selItems = selectedItems();
	if (selItems.length() > 0)
	{
		QTreeWidgetItem* pWatchItem = selItems[0];
		if (pWatchItem)
		{
			QString textToCopy;

			textToCopy.append(getFullName(pWatchItem));
			textToCopy.append("\t");
			textToCopy.append(pWatchItem->text(1));

			QClipboard *pClipboard = QApplication::clipboard();
			if (pClipboard)
				pClipboard->setText(textToCopy);
		}
	}
}

void WatchTreeWidget::addWatch()
{
	QList<QTreeWidgetItem*> selItems = selectedItems();
	QString watchExpression;
	if (selItems.length() > 0)
	{
		//add watch for child
		QTreeWidgetItem* pWatchItem = selItems[0];
		if (pWatchItem)
		{
			QString fullName = getFullName(pWatchItem);
			if (fullName != pWatchItem->text(0))
				watchExpression = fullName;
		}
	}
	
	if (!watchExpression.isEmpty())
	{
		DebuggerClientManager::Instance().addWatch(watchExpression);
	}
	else
	{
		DummyWatchItem* pDummyWatchItem = dynamic_cast<DummyWatchItem*>(topLevelItem(topLevelItemCount()-1));
		if (pDummyWatchItem)
		{
			scrollToItem(pDummyWatchItem);
			setCurrentIndex(indexFromItem(pDummyWatchItem, 0));
			editItem(pDummyWatchItem, 0);
		}
		else
		{
			// control should not come here!
			watchExpression = QInputDialog::getText(this, tr("Watch"), tr("Watch Expression:"), QLineEdit::Normal, watchExpression, NULL, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
			if (!watchExpression.isEmpty())
				DebuggerClientManager::Instance().addWatch(watchExpression);
		}
	}
}

void WatchTreeWidget::removeWatch()
{	deleteSelectedWatch(); }

void WatchTreeWidget::deleteAllWatches()
{
	int numChild = 	topLevelItemCount();
	if (numChild <=0)
		return;

	RBX::ScopedAssign<bool> ignoreWatchDeleteEvent(m_bIgnoreWatchModification, true);

	int currentChild = 0;
	WatchItem* pWatchItem = NULL;

	while (currentChild < numChild)
	{
		pWatchItem = dynamic_cast<WatchItem*>(topLevelItem(currentChild));
		if (pWatchItem)
			pWatchItem->deleteWatch();
		++currentChild;
	}

	clear();
}

void WatchTreeWidget::contextMenuEvent(QContextMenuEvent* evt)
{
	if (!m_pContextualMenu)
	{
		m_pContextualMenu = new QMenu(this);

		m_pCopyAction = m_pContextualMenu->addAction(tr("Copy"), this, SLOT(copySelectedItem()));
		m_pContextualMenu->addSeparator();
		m_pAddWatchAction = m_pContextualMenu->addAction(tr("Add Watch"), this, SLOT(addWatch()));
		m_pDeleteWatchAction = m_pContextualMenu->addAction(tr("Delete Watch"), this, SLOT(removeWatch()));
		m_pContextualMenu->addSeparator();
		m_pDeleteAllWatchesAction = m_pContextualMenu->addAction(tr("Delete All"), this, SLOT(deleteAllWatches()));
	}

	m_pDeleteAllWatchesAction->setEnabled(topLevelItemCount() > 0);
	// allow watch addition only if we are in a script doc
	IRobloxDoc *pCurrentDoc = RobloxDocManager::Instance().getCurrentDoc();
	m_pAddWatchAction->setEnabled(pCurrentDoc && (pCurrentDoc->docType() == IRobloxDoc::SCRIPT));

	m_pCopyAction->setEnabled(false);
	m_pDeleteWatchAction->setEnabled(false);

	QList<QTreeWidgetItem*> selItems = selectedItems();
	if (selItems.length() == 1 && dynamic_cast<DummyWatchItem*>(selItems[0]))
	{
		m_pDeleteAllWatchesAction->setEnabled(true);
	}
	else if (selItems.length() > 0)
	{
		WatchItem* pWatchItem = dynamic_cast<WatchItem*>(selItems[0]);
		m_pDeleteWatchAction->setEnabled(pWatchItem && !pWatchItem->parent());
		m_pCopyAction->setEnabled(true);		
	}

	m_pContextualMenu->exec(evt->globalPos());
}

void WatchTreeWidget::mousePressEvent(QMouseEvent *event)
{
	DebuggerTreeWidget::mousePressEvent(event);

	if ((event->button() == Qt::LeftButton) && (event->pos().x() + header()->offset() > 20) && (currentColumn() == 0))
	{
		QTreeWidgetItem* pTreeWidgetItem = itemAt(event->pos());
		if (pTreeWidgetItem)
		{
			setCurrentIndex(indexFromItem(pTreeWidgetItem, 0));
			editItem(pTreeWidgetItem, 0);
		}
	}
}

WatchItem* WatchTreeWidget::findWatchItem(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	int numChild = 	topLevelItemCount(), currentChild = 0;
	WatchItem* pWatchItem = NULL;

	while (currentChild < numChild)
	{
		//static cast should be OK as we are inserting only BreakpointItem type of items!
		pWatchItem = static_cast<WatchItem*>(topLevelItem(currentChild));
		if (pWatchItem && pWatchItem->hasWatch(spWatch))
			return pWatchItem;
		++currentChild;
	}

	return NULL;
}

QString WatchTreeWidget::getFullName(QTreeWidgetItem *pItem)
{
	QString fullName;
	while (pItem)
	{
		fullName.prepend(pItem->text(0));
		pItem = pItem->parent();
		if (pItem)
			fullName.prepend('.');
	}
	return fullName;
}

void WatchTreeWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// For Ribbon, it get's difficult to differentiate between enabled and disabled items, make the colors darker/lighter
	bool isRibbon = UpdateUIManager::Instance().getMainWindow().isRibbonStyle();
	QStyleOptionViewItemV4 opt = option;
	WatchItem *pItem = static_cast<WatchItem*>(itemFromIndex(index));
	if (pItem && !pItem->isInvalid())
	{
		// we do not need to do anything for Menu mode
		if (isRibbon)
		{
			opt.palette.setColor(QPalette::All, QPalette::Text, opt.palette.color(QPalette::Active, QPalette::Text).darker());
			opt.palette.setColor(QPalette::All, QPalette::Base, opt.palette.color(QPalette::Active, QPalette::Base).darker());	
		}
	}
	else
	{
		QColor disabledText(opt.palette.color(QPalette::Disabled, QPalette::Text));
		QColor disabledBase(opt.palette.color(QPalette::Disabled, QPalette::Base));

		opt.palette.setColor(QPalette::All, QPalette::Text, isRibbon ? QColor("gray") : disabledText);
		opt.palette.setColor(QPalette::All, QPalette::Base, isRibbon ? QColor("gray") : disabledBase);
	}

	QTreeWidget::drawRow(painter, opt, index);

    painter->save();
    {
        painter->setPen(QPen(QColor("lightgray")));
        painter->drawLine(opt.rect.x(), opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
    }
    painter->restore();
}

void WatchTreeWidget::clear()
{
	DebuggerTreeWidget::clear();
	// add dummy item for inline watch addition
	addDummyWatchItem();
}

void WatchTreeWidget::addDummyWatchItem()
{
	addTopLevelItem(new DummyWatchItem);
}

//--------------------------------------------------------------------------------------------
// WatchWidget
//--------------------------------------------------------------------------------------------
WatchWidget::WatchWidget(QWidget *pParent)
: QWidget(pParent)
{
	m_pWatchTreeWidget = new WatchTreeWidget(this);

	QGridLayout *pGridLayout = new QGridLayout(this);
	pGridLayout->setSpacing(0);
	pGridLayout->setContentsMargins(0, 0, 0, 0);
	pGridLayout->addWidget(m_pWatchTreeWidget, 0, 0, 1, 1);

	connect(m_pWatchTreeWidget, SIGNAL(deleteSelectedItem()), this, SLOT(onDeleteSelectedItem()));
}

void WatchWidget::addWatch(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{	m_pWatchTreeWidget->addWatch(spWatch); }

void WatchWidget::removeWatch(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{	m_pWatchTreeWidget->removeWatch(spWatch); }

void WatchWidget::updateWatchValues(bool enable)
{	QMetaObject::invokeMethod(m_pWatchTreeWidget, "updateWatchValues", Qt::AutoConnection, Q_ARG(bool, enable)); }

void WatchWidget::clear()
{ m_pWatchTreeWidget->clear(); }

void WatchWidget::onDeleteSelectedItem()
{	m_pWatchTreeWidget->deleteSelectedWatch(); }

//--------------------------------------------------------------------------------------------
// DebuggerToolTipWidget
//--------------------------------------------------------------------------------------------
DebuggerToolTipWidget::DebuggerToolTipWidget(QWidget *pParent)
: QTreeWidget(pParent)
, m_pModifiedByMouse(NULL)
{
	setWindowFlags(Qt::ToolTip);
    setFocusPolicy(Qt::NoFocus);
    setUniformRowHeights(true);
    setColumnCount(2);
	setRootIsDecorated(false);
	setMouseTracking(true);
	setStyleSheet("QTreeView::item { border-right: 1px solid black }");

	header()->hide();

	m_pExpansionTimer = new QTimer(this);
	m_pExpansionTimer->setSingleShot(true);

	connect(m_pExpansionTimer, SIGNAL(timeout()), this, SLOT(onTimeout()));
	
	connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(computeToolTipSize()));
	connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(computeToolTipSize()));
}

void DebuggerToolTipWidget::showToolTip(const QPoint& pos, const QString &key, const RBX::Reflection::Variant &value)
{
	//clear previously set data
	clear();
	setRootIsDecorated(false);

	//now again create the required items
	QTreeWidgetItem *pTreeWidgetItem = addChildren(NULL);

	try
	{
		populateValues(pTreeWidgetItem, key, value);
	}
	catch (...)
	{
		return;
	}

	m_Key = key;

	if (pTreeWidgetItem->childCount() > 0)
		setRootIsDecorated(true);

	addTopLevelItem(pTreeWidgetItem);
	
	computeToolTipSize();
	showAt(pos);

	qApp->installEventFilter(this);
}

void DebuggerToolTipWidget::hideToolTip()
{
	qApp->removeEventFilter(this);

	m_pExpansionTimer->stop();
	m_pModifiedByMouse = NULL;
	m_Key = "";

	hide();
}

void DebuggerToolTipWidget::showAt(const QPoint& pos)
{
	move(pos.x()+5, pos.y()+5);
	show();
}

void DebuggerToolTipWidget::computeToolTipSize()
{
	//calculate the required height and width
	int columnsWidth = 0;
    for (int ii = 0; ii < 2; ++ii) 
	{
        resizeColumnToContents(ii);
        columnsWidth += sizeHintForColumn(ii);
    }
	int rowsHeight = computeToolTipHeight(indexFromItem(topLevelItem(0)));
    
	//calculate available height and width
	QPoint pos(x(), y());
    QRect desktopRect = QApplication::desktop()->availableGeometry(pos);
    int maxWidth = desktopRect.right() - pos.x() - 5 - 5;
    int maxHeight = desktopRect.bottom() - pos.y() - 5 - 5;
    if (columnsWidth > maxWidth)
        rowsHeight += horizontalScrollBar()->height();

	//modify scrollbar appropriately
    if (rowsHeight > maxHeight) 
	{
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        rowsHeight = maxHeight;
        columnsWidth += verticalScrollBar()->width();
		setAutoScroll(true);
    }
    else 
	{
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setAutoScroll(false);
    }

    if (columnsWidth > maxWidth) 
	{
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        columnsWidth = maxWidth;
    }
    else 
	{
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    setMinimumSize(columnsWidth + 5, rowsHeight);
    setMaximumSize(columnsWidth + 5, rowsHeight);
}

int DebuggerToolTipWidget::computeToolTipHeight(const QModelIndex &index)
{
    int s = rowHeight(index);
    for (int ii = 0; ii < model()->rowCount(index); ++ii)
        s += computeToolTipHeight(model()->index(ii, 0, index));
    return s;
}

void DebuggerToolTipWidget::mousePressEvent(QMouseEvent * evt)
{
	QPoint position = evt->pos();
	QTreeWidgetItem *pItem = itemAt(position);
	if (pItem)
	{
		int depth = getDepth(pItem);
		if ((position.x() > (depth-1)*indentation()) && (position.x() < ((depth-1)*indentation()+20)))
		{
			m_pModifiedByMouse = pItem;
			m_pExpansionTimer->start(300);
		}
	}

	QTreeWidget::mousePressEvent(evt);
}

void DebuggerToolTipWidget::mouseMoveEvent(QMouseEvent * evt)
{
	if (isVisible())
		m_pExpansionTimer->start(300);

	QTreeWidget::mouseMoveEvent(evt);
}

bool DebuggerToolTipWidget::eventFilter(QObject *obj, QEvent *evt)	
{
	switch (evt->type()) 
	{
	case QEvent::KeyRelease:
		if (static_cast<QKeyEvent *>(evt)->key() == Qt::Key_Escape)
			hideToolTip();
		break;
	case QEvent::ApplicationDeactivate:
		hideToolTip();
		break;
	case QEvent::MouseMove:
		{
			QPoint mousePos = static_cast<QMouseEvent*>(evt)->globalPos();
			if (!rect().contains(mapFromGlobal(mousePos)) && !parentWidget()->rect().contains(parentWidget()->mapFromGlobal(mousePos)))
				hideToolTip();
			break;
		}
	case QEvent::MouseButtonPress:
		if ( (obj != this) && (obj != this->viewport()) )
			hideToolTip();
		break;
    default:
        break;
	}

	return QTreeWidget::eventFilter(obj, evt);	
}

void DebuggerToolTipWidget::wheelEvent(QWheelEvent * e)
{
	if (verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
	{
		e->accept();
		return;
	}
	
	QTreeWidget::wheelEvent(e);	
}

void DebuggerToolTipWidget::onTimeout()
{
	QPoint position = mapFromGlobal(QCursor::pos());

	QTreeWidgetItem *pItem = itemAt(position);
	if (pItem)
	{
		if (m_pModifiedByMouse != pItem)
		{
			int depth = getDepth(pItem);
			if ((position.x() > (depth-1)*indentation()) && (position.x() < ((depth-1)*indentation()+20)))
			{
				if (!pItem->isExpanded())
					expandItem(pItem);
			}

			m_pModifiedByMouse = NULL;
		}
	}
}

int DebuggerToolTipWidget::getDepth(QTreeWidgetItem *pItem)
{
	int depth = 0;
	while(pItem != 0)
	{
		depth++;
		pItem = pItem->parent();
	}

	return depth;
}

QTreeWidgetItem* addChildren(QTreeWidgetItem* pParentItem, int iconIndex)
{
	QTreeWidgetItem *pTreeWidgetItem = new QTreeWidgetItem(pParentItem);
	pTreeWidgetItem->setIcon(0, QtUtilities::getPixmap(":/images/img_classtree.bmp", iconIndex, 16, true));
	return pTreeWidgetItem;
}

bool populateValues(QTreeWidgetItem *pParentItem, const QString &key, const RBX::Reflection::Variant varValue)
{
	bool result = true;
	QString textToSet;

	if(varValue.isType<boost::shared_ptr<RBX::Instance> >())
	{
		boost::shared_ptr<RBX::Instance> pInstance = varValue.get<boost::shared_ptr<RBX::Instance> >();
		textToSet = pInstance->getName().c_str();

		for (RBX::Reflection::PropertyIterator iter = pInstance->properties_begin(); iter != pInstance->properties_end(); ++iter)
		{
			if (RBX::Reflection::RefPropertyDescriptor::isRefPropertyDescriptor((*iter).getDescriptor()))
			{
				const RBX::Reflection::RefPropertyDescriptor* desc = boost::polymorphic_downcast<const RBX::Reflection::RefPropertyDescriptor*>(&(*iter).getDescriptor());
				RBX::Reflection::DescribedBase* pBase = desc->getRefValue(pInstance.get());
				if (pBase)
				{
					RBX::Instance* instance = boost::polymorphic_downcast<RBX::Instance*>(pBase);
					if (instance)
					{
						if (!pInstance->contains(instance))
							populateValues(addChildren(pParentItem), desc->name.c_str(), RBX::shared_from(instance));
					}
					else
					{
						populateValues(addChildren(pParentItem), desc->name.c_str(), std::string("NULL"));
					}
				}
			}
			else
			{
				if ((*iter).getDescriptor().isReadOnly() || (*iter).getDescriptor().isWriteOnly())
					continue;
				if (!(*iter).getDescriptor().canXmlWrite())
					continue;
				if (!(*iter).getDescriptor().canReplicate())
					continue;

				RBX::Reflection::Variant value;
				(*iter).getDescriptor().getVariant(pInstance.get(), value);

				populateValues(addChildren(pParentItem), (*iter).getDescriptor().name.c_str(), value);
			}
		}

	}
	else if (varValue.isType<RBX::Vector2>())
	{
		//get x, y
		textToSet = RBX::StringConverter<RBX::Vector2>::convertToString(varValue.get<RBX::Vector2>()).c_str();

		RBX::Vector2 vec2 = varValue.get<RBX::Vector2>();

		populateValues(addChildren(pParentItem), "X", vec2.x);
		populateValues(addChildren(pParentItem), "Y", vec2.y);
	}
	else if (varValue.isType<RBX::Vector2int16>())
	{
		//get x, y
		textToSet = RBX::StringConverter<RBX::Vector2int16>::convertToString(varValue.get<RBX::Vector2int16>()).c_str();

		RBX::Vector2int16 vec2 = varValue.get<RBX::Vector2int16>();
		populateValues(addChildren(pParentItem), "X", int(vec2.x));
		populateValues(addChildren(pParentItem), "Y", int(vec2.y));
	}
	else if (varValue.isType<RBX::Vector3>())
	{
		textToSet = RBX::StringConverter<RBX::Vector3>::convertToString(varValue.get<RBX::Vector3>()).c_str();

		RBX::Vector3 vec3 = varValue.get<RBX::Vector3>();

		populateValues(addChildren(pParentItem), "X", vec3.x);
		populateValues(addChildren(pParentItem), "Y", vec3.y);
		populateValues(addChildren(pParentItem), "Z", vec3.z);
	}
	else if (varValue.isType<G3D::Vector3int16>())
	{
		textToSet = RBX::StringConverter<G3D::Vector3int16>::convertToString(varValue.get<G3D::Vector3int16>()).c_str();

		G3D::Vector3int16 vec3 = varValue.get<G3D::Vector3int16>();
		populateValues(addChildren(pParentItem), "X", int(vec3.x));
		populateValues(addChildren(pParentItem), "Y", int(vec3.y));
		populateValues(addChildren(pParentItem), "Z", int(vec3.z));
	}
	else if (varValue.isType<G3D::CoordinateFrame>())
	{
		textToSet = RBX::StringConverter<G3D::CoordinateFrame>::convertToString(varValue.get<G3D::CoordinateFrame>()).c_str();

		G3D::CoordinateFrame coordinateFrame = varValue.get<G3D::CoordinateFrame>();
		populateValues(addChildren(pParentItem), "p", RBX::Reflection::Variant(coordinateFrame.translation));
		populateValues(addChildren(pParentItem), "lookVector", RBX::Reflection::Variant(coordinateFrame.lookVector()));
	}
	else if (varValue.isType<RBX::Region3>())
	{
		textToSet = RBX::StringConverter<RBX::Region3>::convertToString(varValue.get<RBX::Region3>()).c_str();

		RBX::Region3 region = varValue.get<RBX::Region3>();
		populateValues(addChildren(pParentItem), "CFrame", RBX::Reflection::Variant(region.getCFrame()));
		populateValues(addChildren(pParentItem), "Size", RBX::Reflection::Variant(region.getSize()));
	}
	else if (varValue.isType<RBX::Rect2D>())
	{
		textToSet = RBX::StringConverter<RBX::Rect2D>::convertToString(varValue.get<RBX::Rect2D>()).c_str();

		RBX::Rect2D rect2D = varValue.get<RBX::Rect2D>();
		populateValues(addChildren(pParentItem), "Min", RBX::Reflection::Variant(rect2D.x0y0()));
		populateValues(addChildren(pParentItem), "Max", RBX::Reflection::Variant(rect2D.x1y1()));
		populateValues(addChildren(pParentItem), "Width", RBX::Reflection::Variant(rect2D.width()));
		populateValues(addChildren(pParentItem), "Height", RBX::Reflection::Variant(rect2D.height()));
	}
	else if (varValue.isType<RBX::Region3int16>())
	{
		textToSet = RBX::StringConverter<RBX::Region3int16>::convertToString(varValue.get<RBX::Region3int16>()).c_str();

		RBX::Region3int16 region = varValue.get<RBX::Region3int16>();
		populateValues(addChildren(pParentItem), "Min", RBX::Reflection::Variant(region.getMinPos()));
		populateValues(addChildren(pParentItem), "Max", RBX::Reflection::Variant(region.getMaxPos()));
	}
	else if (varValue.isType<RBX::BrickColor>())
	{
		textToSet = RBX::StringConverter<RBX::BrickColor>::convertToString(varValue.get<RBX::BrickColor>()).c_str();

		RBX::BrickColor brickColor = varValue.get<RBX::BrickColor>();
		populateValues(addChildren(pParentItem), "Color", RBX::Reflection::Variant(brickColor.color3()));
		populateValues(addChildren(pParentItem), "Name", RBX::Reflection::Variant(std::string(brickColor.name())));
		populateValues(addChildren(pParentItem), "Number", RBX::Reflection::Variant((int)brickColor.number));
	}
	else if (varValue.isType<G3D::Color3>())
	{
		textToSet = RBX::StringConverter<G3D::Color3>::convertToString(varValue.get<G3D::Color3>()).c_str();

		G3D::Color3 color = varValue.get<G3D::Color3>();
		populateValues(addChildren(pParentItem), "r", RBX::Reflection::Variant(color.r));
		populateValues(addChildren(pParentItem), "g", RBX::Reflection::Variant(color.g));
		populateValues(addChildren(pParentItem), "b", RBX::Reflection::Variant(color.b));
	}
	else if (varValue.isType<RBX::RbxRay>())
	{
		textToSet = RBX::StringConverter<RBX::RbxRay>::convertToString(varValue.get<RBX::RbxRay>()).c_str();

		RBX::RbxRay ray = varValue.get<RBX::RbxRay>();
		populateValues(addChildren(pParentItem), "Origin", RBX::Reflection::Variant(ray.origin()));
		populateValues(addChildren(pParentItem), "Direction", RBX::Reflection::Variant(ray.direction()));
	}
	else if (varValue.isType<RBX::UDim>())
	{
		textToSet = RBX::StringConverter<RBX::UDim>::convertToString(varValue.get<RBX::UDim>()).c_str();

		RBX::UDim dim = varValue.get<RBX::UDim>();
		populateValues(addChildren(pParentItem), "Scale", RBX::Reflection::Variant(dim.scale));
		populateValues(addChildren(pParentItem), "Offset", RBX::Reflection::Variant(int(dim.offset)));
	}
	else if (varValue.isType<RBX::UDim2>())
	{
		textToSet = RBX::StringConverter<RBX::UDim2>::convertToString(varValue.get<RBX::UDim2>()).c_str();

		RBX::UDim2 dim2 = varValue.get<RBX::UDim2>();
		populateValues(addChildren(pParentItem), "X", RBX::Reflection::Variant(dim2.x));
		populateValues(addChildren(pParentItem), "Y", RBX::Reflection::Variant(dim2.y));
	}
	else if (varValue.isType<shared_ptr<const RBX::Reflection::ValueMap> >())
	{
		textToSet = "Map";
		shared_ptr<const RBX::Reflection::ValueMap> valMap = varValue.get<shared_ptr<const RBX::Reflection::ValueMap> >();

		for (RBX::Reflection::ValueMap::const_iterator iter = valMap->begin(); iter != valMap->end(); ++iter)
		{
			populateValues(addChildren(pParentItem), QString("[%1]").arg(iter->first.c_str()), iter->second);
		}
	}
	else if (varValue.isType<shared_ptr<const RBX::Reflection::ValueTable> >())
	{
		textToSet = "Table";
		shared_ptr<const RBX::Reflection::ValueTable> valMap = varValue.get<shared_ptr<const RBX::Reflection::ValueTable> >();
		for (RBX::Reflection::ValueTable::const_iterator iter = valMap->begin(); iter != valMap->end(); ++iter)
		{
			populateValues(addChildren(pParentItem), QString("[%1]").arg(iter->first.c_str()), iter->second);
		}
	}
	else if (varValue.isType<shared_ptr<const RBX::Reflection::ValueArray> >())
	{
		textToSet = "Array";
		shared_ptr<const RBX::Reflection::ValueArray> valMap = varValue.get<shared_ptr<const RBX::Reflection::ValueArray> >();
		RBX::Reflection::ValueArray::const_iterator iter = valMap->begin();

		for (int ii=1; iter != valMap->end(); ++iter, ++ii)
		{
			populateValues(addChildren(pParentItem), QString("[%1]").arg(ii), *iter);
		}
	}
	else if (varValue.isType<shared_ptr<const RBX::Instances> >())
	{
		textToSet = "Instances";
		shared_ptr<const RBX::Instances> valMap = varValue.get<shared_ptr<const RBX::Instances> >();
		RBX::Instances::const_iterator iter = valMap->begin();

		for (int ii=0; iter != valMap->end(); ++iter, ++ii)
		{
			populateValues(addChildren(pParentItem), QString::number(ii), *iter);
		}
	}
	else if (varValue.isType<shared_ptr<const RBX::Reflection::Tuple> >())
	{
		textToSet = "Tuple";
		shared_ptr<const RBX::Reflection::Tuple> valMap = varValue.get<shared_ptr<const RBX::Reflection::Tuple> >();
		RBX::Reflection::ValueArray::const_iterator iter = valMap->values.begin();

		for (int ii=0; iter != valMap->values.end(); ++iter, ++ii)
		{
			populateValues(addChildren(pParentItem), QString::number(ii), *iter);
		}
	}
	else if (varValue.isType<float>() || varValue.isType<int>())
	{
		textToSet = QString::number(varValue.get<float>());
	}
	else if (varValue.isType<RBX::Lua::WeakFunctionRef>())
	{
		textToSet = "function";
		pParentItem->setIcon(0, QtUtilities::getPixmap(":/images/img_classtree.bmp", 4, 16, true));
	}
	else
	{
		try
		{
			textToSet = varValue.get<std::string>().c_str();
		}

		catch (...)
		{
			result = false;
		}
	}

	pParentItem->setText(0, key);
	pParentItem->setText(1, textToSet);

	return result;
}
