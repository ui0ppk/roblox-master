/**
 * DebuggerWidgets.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
#define QT_NO_KEYWORDS
#endif

// standard C/C++ Headers
#include <map>

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

// Roblox Headers
#include "script/DebuggerManager.h"

// Qt Headers
#include <QObject>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QComboBox>

// Roblox Studio Headers

namespace RBX {
	namespace Reflection {
		class PropertyDescriptor;
	}
}

class QDockWidget;
class QToolButton;
class BreakpointsWidget;
class CallStackWidget;
class DebuggerClient;

class DebuggerViewsManager
{
public:
	static DebuggerViewsManager& Instance();
	
	enum EDebuggerViewID
	{
		eDV_BREAKPOINTS,
		eDV_CALLSTACK,
		eDV_WATCH,
		eDV_MAX
	};

	template<class T> 
	T& getViewWidget(EDebuggerViewID id) const
	{
		RBXASSERT(m_ViewWidgets.contains(id));
		RBXASSERT(m_ViewWidgets[id]);
		QWidget* widget = m_ViewWidgets[id];
		RBXASSERT(dynamic_cast<T*>(widget));
		return static_cast<T&>(*widget);
	}

	void showDockWidget(EDebuggerViewID dockId);

private:
	DebuggerViewsManager();

	void restoreViews();
	void saveViews();

	void onDebuggingStarted();
	void onDebuggingStopped();

	void onDebuggerClientActivated(int currentBreakpoint, RBX::Scripting::ScriptDebugger::Stack currentStack);
	void onDebuggerClientDeactivated();

	void onExecutionDataCleared();
	void onBreakpointEncountered(int currentBreakpoint, DebuggerClient* pDebuggerClient);

	void onClearAll();

	void onBreakpointAdded(shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint);
	void onBreakpointRemoved(shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint);
	
	void onWatchAdded(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	void onWatchRemoved(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);

	void onDebuggersListUpdated(DebuggerClient* pDebuggerClient);

	QMap<EDebuggerViewID,QDockWidget*>    m_DockWidgets;
	QMap<EDebuggerViewID,QWidget*>        m_ViewWidgets;

	std::list<rbx::signals::connection>	  m_cConnections;
	bool                                  m_bBreakpointEncountered;
};

class DebuggerTreeWidgetItem : public QTreeWidgetItem
{
public:
	DebuggerTreeWidgetItem() {}

	//no-op functions
	virtual void activate() {}
	virtual void columnModified(int column) {}
};

class DebuggerTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	DebuggerTreeWidget(QWidget* pParent);

Q_SIGNALS:
	void deleteSelectedItem();
	
protected Q_SLOTS:
	virtual void onItemActivated(QTreeWidgetItem*);
	virtual void onItemChanged(QTreeWidgetItem*, int);

protected:
	virtual void mousePressEvent(QMouseEvent *evt);
	virtual void keyPressEvent(QKeyEvent * evt);
};

class StackDetailsItem : public DebuggerTreeWidgetItem
{
public:
	StackDetailsItem(const RBX::Scripting::ScriptDebugger::FunctionInfo& funcInfo, bool isFirstItem);
	~StackDetailsItem();
	void activate();
	void updateIcon(bool isCurrent);

private:
	int getLine();
	int getFrame();

	boost::shared_ptr<RBX::Instance> m_spAssociatedScript;
	bool                             m_bIsFirstItem;
};

class CallStackTreeWidget : public DebuggerTreeWidget
{
	Q_OBJECT

public:
	CallStackTreeWidget(QWidget* pParent=0);
	~CallStackTreeWidget();

	void setCallStack(const RBX::Scripting::ScriptDebugger::Stack& stack);

private:
	void onItemActivated(QTreeWidgetItem* pItem);

	QTreeWidgetItem*      m_pPreviousItem;
};

class CallStackDebuggersList: public QComboBox
{
	Q_OBJECT
public:
	CallStackDebuggersList(QWidget* pWidget);
	/*override*/ void showPopup();

	bool isDirty() { return m_bIsDirty;}
	void setDirty(bool state) { m_bIsDirty = state;}

Q_SIGNALS:
	void popupAboutToShow();

private:
	bool                   m_bIsDirty;
};

class CallStackWidget : public QWidget
{
	Q_OBJECT
public:
	CallStackWidget(QWidget* pParent=0);

	void populateDebuggers(DebuggerClient* pDebuggerClient);
	void clearDebuggers();

	void setCallStack(const RBX::Scripting::ScriptDebugger::Stack& stack);
	void clear();

private Q_SLOTS:
	void onCurrentIndexChanged(int index);
	void onDebuggerListPopupShow();

private:
	CallStackTreeWidget *m_pCallStackTreeWidget;
	QComboBox           *m_pScriptDebuggersList;

	DebuggerClient      *m_pCurrentDebuggerClient;
};

class BreakpointItem : public DebuggerTreeWidgetItem
{
public:
	BreakpointItem(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	~BreakpointItem();

	bool hasBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	void columnModified(int column);
	void activate();

	void setBreakpointEnabled(bool state);
	bool isBreakpointEnabled();
	void deleteBreakpoint();
	
private:
	void onBreakpointPropertyChanged(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);
	void onScriptAncestryChanged(shared_ptr<RBX::Instance> newParent);

	boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint>		m_spBreakpoint;
	rbx::signals::scoped_connection                             m_cPropertyChangedConnection;
	rbx::signals::scoped_connection                             m_cAncestoryChangedConnection;
	bool                                                        m_bIgnoreBreakpointModification;
	bool                                                        m_bIgnoreWaypointRequest;
};

class BreakpointsTreeWidget : public DebuggerTreeWidget
{
public:
	BreakpointsTreeWidget(QWidget* pParent=0);
	
	void addBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	void removeBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	void setCurrentBreakpoint(int breakpointLine);

	void deleteSelectedBreakpoint();
	void deleteAllBreakpoints();
	void disableEnableAllBreakpoints();
	void goToSourceCode();

	bool hasBreakpointsEnabled();

private:
	BreakpointItem* findBreakpointItem(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	BreakpointItem* findBreakpointItem(int breakpointLine);
};

class BreakpointsWidget: public QWidget
{
	Q_OBJECT
public:
	BreakpointsWidget(QWidget* pParent=0);

	//delegate functions
	void addBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	void removeBreakpoint(boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint);
	void setCurrentBreakpoint(int breakpointLine);

	void clear();

	int numBreakpoints() { return m_pBreakpointTreeWidget ? m_pBreakpointTreeWidget->topLevelItemCount() : 0; }

private Q_SLOTS:
	void onPushButtonClicked();
	void onDeleteSelectedItem();
	void requestButtonsStatusUpdate();
	void updateButtonsStatus();

private:
	BreakpointsTreeWidget*   m_pBreakpointTreeWidget;

	QToolButton*             m_pDeleteBreakpointTB;
	QToolButton*			 m_pDeleteAllBreakpointsTB;
	QToolButton*			 m_pDisableEnableAllBreakpointsTB;
	QToolButton*			 m_pGoToSourceCodeTB;

	bool                     m_bPushButtonStateUpdateRequested;
	bool                     m_bIgnoreBreakpointModification;
};

class WatchItem : public DebuggerTreeWidgetItem
{
public:
	WatchItem(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	~WatchItem();

	void updateWatchValue();
	bool hasWatch(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);

	void columnModified(int column);
	void deleteWatch();

	virtual void setInvalid(bool state);
	bool isInvalid() { return m_bIsInValid; }

private:
	void onWatchPropertyChanged(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);
	void onScriptAncestryChanged(shared_ptr<RBX::Instance> newParent);
	void removeChildren();

	boost::shared_ptr<RBX::Scripting::DebuggerWatch>            m_spWatch;
	rbx::signals::scoped_connection                             m_cPropertyChangedConnection;
	rbx::signals::scoped_connection                             m_cAncestoryChangedConnection;
	bool                                                        m_bIgnoreWatchModification;
	bool                                                        m_bIsInValid;
};

class WatchTreeWidget : public DebuggerTreeWidget
{
	Q_OBJECT
public:
	WatchTreeWidget(QWidget *pParent);
	
	void addWatch(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	void removeWatch(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	void deleteSelectedWatch();

	QTreeWidgetItem* itemFromIndex(QModelIndex index) const { return QTreeWidget::itemFromIndex(index); }
	void clear();
public Q_SLOTS:
		void updateWatchValues(bool enable);

private Q_SLOTS:
	void copySelectedItem();
	void addWatch();
	void removeWatch();
	void deleteAllWatches();

private:
	virtual void contextMenuEvent(QContextMenuEvent* evt);
	virtual void mousePressEvent(QMouseEvent *event);
	WatchItem* findWatchItem(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	QString getFullName(QTreeWidgetItem *pItem);
	void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void addDummyWatchItem();

	QMenu*                   m_pContextualMenu;
	QAction*                 m_pCopyAction;
	QAction*                 m_pAddWatchAction;
	QAction*                 m_pDeleteWatchAction;
	QAction*                 m_pDeleteAllWatchesAction;

	bool                     m_bIgnoreWatchModification;
};

class WatchWidget : public QWidget
{
	Q_OBJECT
public:
	WatchWidget(QWidget *pParent=0);

	void addWatch(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	void removeWatch(shared_ptr<RBX::Scripting::DebuggerWatch> spWatch);
	void updateWatchValues(bool enable = false);

	void clear();

private Q_SLOTS:
	void onDeleteSelectedItem();

private:
	WatchTreeWidget*         m_pWatchTreeWidget;
};

class DebuggerToolTipWidget: public QTreeWidget
{
	Q_OBJECT
public:
	DebuggerToolTipWidget(QWidget *pParent);

	void showToolTip(const QPoint& pos, const QString &key, const RBX::Reflection::Variant &value);
	void hideToolTip();

	bool isKeyValid(const QString& key) { return key == m_Key; }
	void showAt(const QPoint& pos);

private Q_SLOTS:
	void computeToolTipSize();
	void onTimeout();

private:
	void mousePressEvent(QMouseEvent * evt);
	void mouseMoveEvent(QMouseEvent * evt);
	bool eventFilter(QObject *obj, QEvent *evt);
	void wheelEvent(QWheelEvent * e);

	int getDepth(QTreeWidgetItem *pItem);
	int computeToolTipHeight(const QModelIndex &index);

	QTimer*                         m_pExpansionTimer;
	QTreeWidgetItem*                m_pModifiedByMouse;
	QString                         m_Key;
	RBX::Reflection::Variant        m_Value;
};

