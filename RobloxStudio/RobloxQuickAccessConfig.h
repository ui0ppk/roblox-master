/**
 * QuickAccessConfigDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QTreeWidget>
#include <QPointer>
#include <QStack>
#include <QAction>

#include "RobloxSavingStateDialog.h"

class RobloxMainWindow;
class QAbstractButton;
class QDialogButtonBox;
namespace Qtitan {
	class RibbonQuickAccessBar;
};

enum eAllActionsColumns
{
	AC_ICON = 0,
	AC_NAME
};

enum eQuickAccessActionsColumns
{
	QAC_VISIBILITY = 0,
	QAC_ICON,
	QAC_NAME
};

class ActionItem : public QTreeWidgetItem
{
public:
	ActionItem(QAction* pAction, Qt::ItemFlags flags = Qt::ItemIsEnabled|Qt::ItemIsSelectable);

	/*override*/ QVariant data(int column, int role) const;
	
	void applyFilter(const QString& filter, int column);

	bool canBeShown();
	QAction* getAction() { return m_pAction; }
	bool hasAction(QAction* pAction);

protected:
	QPointer<QAction>    m_pAction;
};

class QuickAccessActionItem: public ActionItem
{
public:
	QuickAccessActionItem(QAction* pAction, bool isVisible, Qt::ItemFlags flags = Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);

	/*override*/ QVariant data(int column, int role) const;
	/*override*/ void setData(int column, int role, const QVariant & value);
	bool hasAction(QAction* pAction);

	void setQuickAccess(Qt::CheckState state) { m_isQuickAccessCheckState = state; }
	bool hasQuickAccessOverride() const { return m_isQuickAccessCheckState != m_originalCheckState; }

	void removeQuickAccessOverride();
	void commitQuickAccessOverride() { m_originalCheckState = m_isQuickAccessCheckState;}
	
private:
	Qt::CheckState       m_isQuickAccessCheckState, m_originalCheckState;
};

class BaseListWidget : public QTreeWidget
{
public:
	BaseListWidget(QWidget* parent);

	bool isActionAvailable(QAction* pAction);
	void selectItem(int row);

	void addAction(QAction* action);
	void removeAction(QAction* action);

protected:
	virtual ActionItem* createItem(QAction* action) = 0;
	virtual bool isValid(QAction* action);
	void setFilter(const QString& filter, int column);
	ActionItem* getItem(QAction* action);

private:
	void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class AllActionsListWidget : public BaseListWidget
{
public:
	AllActionsListWidget(QWidget* parent);
	ActionItem* createItem(QAction* action);

	void setFilter(const QString& filter);

	void selectNextItem(ActionItem* item);
	void sort();

	bool isValid(QAction* action);
};

class QuickAccessActionsListWidget : public BaseListWidget
{
public:
	QuickAccessActionsListWidget(QWidget* parent);
	void initialize();

	ActionItem* createItem(QAction* action);

	void moveItem(QTreeWidgetItem* itemToAdd, bool moveUp);
	void removeItem(QTreeWidgetItem* itemToRemove);

	void accept();
};

class QuickAccessConfigDialog : public RobloxSavingStateDialog<QDialog>
{
    Q_OBJECT

public:
    QuickAccessConfigDialog(QWidget* parent);
    
private Q_SLOTS:
    void accept();
    void cancel();
    void restoreDefaults(QAbstractButton* button);
	void onDefaultsRestored();
    void onQuickAccessListDataChanged();

	void onAddButtonClicked();
	void onRemoveButtonClicked();
	void onMoveButtonClicked();

	void setFilter(QString filter);
    
private:
	bool eventFilter(QObject* watched, QEvent* evt);
	void showEvent(QShowEvent* evt);
	void doLayout();

    QDialogButtonBox             *m_dialogButtonBox;
  
    AllActionsListWidget         *m_pAllActionListWidget;
	QuickAccessActionsListWidget *m_pQuickAccessActionsListWidget;

    QPushButton                  *m_pAddButton;
    QPushButton                  *m_pRemoveButton;

	QPushButton                  *m_pMoveUpButton;
	QPushButton                  *m_pMoveDownButton;

	QLineEdit                    *m_pFilterEdit;

    bool                          m_dataChanged;
};

class RobloxQuickAccessConfig : public QObject
{
	Q_OBJECT
public:

	static RobloxQuickAccessConfig& singleton();

	bool loadQuickAccessConfig();
	void saveQuickAccessConfig();

	void setPluginHostInitialized(bool state) { m_pluginHostInitialized = state; }

	struct ActionData
	{
		QPointer<QAction> action;

		bool isVisible;
		bool isInitialized;

		ActionData(QAction* act, bool state, bool initialized = false)
		: action(act), isVisible(state), isInitialized(initialized)
		{
		}

		bool isProxyAction() const;
	};

	typedef QList<ActionData> QuickAccessActionCollection;
	
	void restoreDefaults();
	void storeDefaults(const QuickAccessActionCollection& quickAccessBarActions);

	void recreateQuickAccessBar(const QuickAccessActionCollection& quickAccessBarActions);
	QuickAccessActionCollection& getQuickAccessBarActions() { return m_currentQuickAccessBarActions; }

	void addToQuickAccessBar(QAction* action, bool visibility);
	void removeFromQuickAccessBar(QAction* action);

	QAction* mapProxyAction(QAction* action, bool force = false);
	QAction* resetProxyAction(QAction* action);
	bool updateProxyAction(QAction* action);
	QAction* getProxyAction(QAction* action);

	bool visibleInQuickAccessBar(QAction* action);
	bool availableInQuickAccessBar(QAction* action);

Q_SIGNALS:
	void actionAdded(QAction* action);
	void actionRemoved(QAction* action);
	void defaultsRestored();

private Q_SLOTS:
	void updateQuickAccessMenu(QMenu* pMenu);
	void updateActionVisiblity(QAction* action);

	void onExtensionMenuShow();
	void onExtensionMenuHide();

private:
	RobloxQuickAccessConfig();

	RobloxMainWindow*              m_pMainWindow;
	Qtitan::RibbonQuickAccessBar*  m_pQuickAccessBar;

	QuickAccessActionCollection    m_defaultQuickAccessBarActions;
	QuickAccessActionCollection    m_currentQuickAccessBarActions;

	QPointer<QAction>              m_pSeparationAction;

	bool                           m_pluginHostInitialized;
};

class QuickAccessBarProxyAction : public QAction
{
	Q_OBJECT
public:
	QuickAccessBarProxyAction(QObject* parent);
	void setSourceAction(QAction* pSourceAction);

	bool hasAction(QAction* action);

private Q_SLOTS:
	void onChanged();
	void onSourceActionDestroyed(QObject*);
	void onActionTriggered();

private:
	QStack<QAction*>     m_pSourceActions;
	QAction*             m_pCurrentSourceAction;
};