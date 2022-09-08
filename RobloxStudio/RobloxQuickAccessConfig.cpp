/**
 * RobloxQuickAccessConfig.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxQuickAccessConfig.h"

#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QPainter>

#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "PluginAction.h"
#include "QtUtilities.h"

FASTFLAG(StudioSeparateActionByActivationMethod)

static const QString QuickAccessConfigKey         = "Roblox Quick Access Config";
static const QString QuickAccessActionNameKey     = "name";
static const QString QuickAccessActionTextKey     = "text";
static const QString QuickAccessActionVisibleKey  = "visible";
static const QString QuickAccessActionShortcutKey = "shortcut";
static const QString QuickAccessProxyActionExt    = "_proxyAct";

static const char* sProp_DisableForQuickAccess    = "disableForQuickAccess";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActionItemDelegate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ActionItemDelegate: public QStyledItemDelegate
{    
public:
	ActionItemDelegate(QWidget* parent)
	: QStyledItemDelegate(parent) 
	{}    

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{  return QStyledItemDelegate::sizeHint(option, index) + QSize(2, 2); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ActionItem
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ActionItem::ActionItem(QAction* pAction, Qt::ItemFlags flags)
: m_pAction(pAction)
{
	setFlags(flags);
}

QVariant ActionItem::data(int column, int role) const
{
	if (m_pAction)
	{
		if (role == Qt::ToolTipRole)
			return m_pAction->statusTip();

		if ((column == AC_NAME) && (role == Qt::DisplayRole))
			return m_pAction->text();

		if ((column == AC_ICON) && (role == Qt::DecorationRole))
			return m_pAction->icon();
	}

    return QVariant();
}

bool ActionItem::hasAction(QAction* action)
{
	if (m_pAction == action)
		return true;

	QuickAccessBarProxyAction* pProxyAction = qobject_cast<QuickAccessBarProxyAction*>(m_pAction);
	if (pProxyAction)
		return (pProxyAction->hasAction(action));

	return false;
}

void ActionItem::applyFilter(const QString& filter, int column)
{
	bool filterStringFound = filter.isEmpty() || text(column).contains(filter, Qt::CaseInsensitive);
	bool itemShown = filterStringFound && canBeShown();
	setHidden(!itemShown);	
}

bool ActionItem::canBeShown()
{
	if (dynamic_cast<PluginAction*>(m_pAction.data()) && m_pAction->parentWidget())
		return !m_pAction->parentWidget()->isHidden();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QuickAccessActionItem
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QuickAccessActionItem::QuickAccessActionItem(QAction* pAction, bool isVisible, Qt::ItemFlags flags)
: ActionItem(pAction, flags)
, m_isQuickAccessCheckState(isVisible ? Qt::Checked : Qt::Unchecked)
{
}

QVariant QuickAccessActionItem::data(int column, int role) const
{
	if (m_pAction)
	{
		if (role == Qt::ToolTipRole)
			return m_pAction->statusTip();

		if ((column == QAC_NAME) && (role == Qt::DisplayRole))
			return m_pAction->text();

		if ((column == QAC_ICON) && (role == Qt::DecorationRole))
			return m_pAction->icon();

		if ((column == QAC_VISIBILITY) && (role == Qt::CheckStateRole))
			return m_isQuickAccessCheckState;
	}

    return QVariant();
}

void QuickAccessActionItem::setData(int column, int role, const QVariant & value)
{
	if ((role == Qt::CheckStateRole) && (column == QAC_VISIBILITY))
	{
		m_isQuickAccessCheckState = (value == Qt::Checked) ? Qt::Checked : Qt::Unchecked;
		emitDataChanged();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BaseListWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseListWidget::BaseListWidget(QWidget* parent)
{
	setAlternatingRowColors(true);
	setRootIsDecorated(false);
	setUniformRowHeights(true);
	setAllColumnsShowFocus(true);
	setIconSize(QSize(16, 16));
	setSelectionMode(QAbstractItemView::SingleSelection);

	header()->setVisible(false);
	header()->setResizeMode(QHeaderView::ResizeToContents);
	header()->setStretchLastSection(true);

	setItemDelegate(new ActionItemDelegate(this));

	setStyleSheet(STRINGIFY(
		QTreeView::item:selected:!active { background: lightgray;}
		QTreeView::item { border: none; }
		));
}

void BaseListWidget::addAction(QAction* action)
{
	// check if action is valid and not added already
	if (isValid(action) && !getItem(action))
	{
		ActionItem* pItem = createItem(action);
		if (pItem)
			addTopLevelItem(pItem);
	}
}

void BaseListWidget::removeAction(QAction* action)
{
	ActionItem* pItem = getItem(action);
	if (pItem)
		delete pItem;
}

ActionItem* BaseListWidget::getItem(QAction* action)
{
	int numChild = 	topLevelItemCount(), currentChild = 0;
	ActionItem *pCurrentItem = NULL;
	while (currentChild < numChild)
	{		
		pCurrentItem = static_cast<ActionItem*>(topLevelItem(currentChild++));
		if (pCurrentItem && pCurrentItem->hasAction(action))
			return pCurrentItem;
	}

	return NULL;
}

void BaseListWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItemV4 opt = option;

	QTreeWidgetItem *pItem = itemFromIndex(index);
	if (!pItem)
	{
		const QColor c = QColor(233, 233, 233);
        painter->fillRect(option.rect, c);
        opt.palette.setColor(QPalette::AlternateBase, c);
	}

	QTreeWidget::drawRow(painter, opt, index);
}

bool BaseListWidget::isValid(QAction* action)
{
	return (action && !action->text().isEmpty() && !action->text().startsWith("&") && !action->objectName().isEmpty() && action->isVisible());
}

bool BaseListWidget::isActionAvailable(QAction* pAction)
{
	int numChild = 	topLevelItemCount(), currentChild = 0;
	ActionItem* pCurrentItem = NULL;
	while (currentChild < numChild)
	{		
		pCurrentItem = static_cast<ActionItem*>(topLevelItem(currentChild++));
		if (pCurrentItem && pCurrentItem->hasAction(pAction))
			return true;
	}
	return false;
}

void BaseListWidget::selectItem(int row)
{
	QTreeWidgetItem* itemToSelect = NULL;
	while (row > 0 && row < topLevelItemCount())
	{
		itemToSelect = topLevelItem(row++);
		if (!itemToSelect->isHidden())
		{
			setCurrentItem(itemToSelect);
			break;
		}
	}
		
}

void BaseListWidget::setFilter(const QString& filter, int column)
{
	setUpdatesEnabled(false);

	QString text;
	int numChild = 	topLevelItemCount(), currentChild = 0;
	ActionItem *pCurrentItem = NULL;
	while (currentChild < numChild)
	{
		pCurrentItem = static_cast<ActionItem*>(topLevelItem(currentChild++));
		if (pCurrentItem)
			pCurrentItem->applyFilter(filter, column);
	}

	setUpdatesEnabled(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BaseListWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AllActionsListWidget::AllActionsListWidget(QWidget* parent)
: BaseListWidget(parent)
{
	setColumnCount(2);

	QAction* pAction = NULL;
	QVariant propVal;

	QList<QAction*> mainWindowActions = UpdateUIManager::Instance().getMainWindow().actions();
	for (QList<QAction*>::const_iterator iter = mainWindowActions.begin(); iter != mainWindowActions.end() ; ++iter)
	{
		pAction = *iter;
		if (!isValid(pAction))
			continue;
		// check if property "disableForQuickAccess" is available or set to true
		propVal = pAction->property(sProp_DisableForQuickAccess);
		if (propVal.isValid() && propVal.toBool())
			continue;
		// action is valid and can be added into quickaccess bar
		addAction(pAction);
	}

	sort();
}

ActionItem* AllActionsListWidget::createItem(QAction* action)
{ return new ActionItem(action); }

void AllActionsListWidget::sort()
{ sortByColumn(AC_NAME, Qt::AscendingOrder); }

bool AllActionsListWidget::isValid(QAction* action)
{ return (BaseListWidget::isValid(action) && !qobject_cast<QuickAccessBarProxyAction*>(action)); }

void AllActionsListWidget::setFilter(const QString& filter)
{ BaseListWidget::setFilter(filter, AC_NAME); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BaseListWidget
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QuickAccessActionsListWidget::QuickAccessActionsListWidget(QWidget* parent)
: BaseListWidget(parent)
{
	setColumnCount(3);
	initialize();
}

void QuickAccessActionsListWidget::initialize()
{
	setUpdatesEnabled(false);
	clear();
	RobloxQuickAccessConfig::QuickAccessActionCollection& quickAccessBarActions = RobloxQuickAccessConfig::singleton().getQuickAccessBarActions();
	for (int ii = 0; ii < quickAccessBarActions.size(); ++ii)
	{	
		if (isValid(quickAccessBarActions.at(ii).action) && quickAccessBarActions.at(ii).isInitialized) 
			addTopLevelItem(new QuickAccessActionItem(quickAccessBarActions.at(ii).action, quickAccessBarActions.at(ii).isVisible));
	}
	setUpdatesEnabled(true);
}

ActionItem* QuickAccessActionsListWidget::createItem(QAction* action)
{
	// get visibility if it's already in quick access bar (for proxy actions)
	if (RobloxQuickAccessConfig::singleton().availableInQuickAccessBar(action))
		return new QuickAccessActionItem(action, RobloxQuickAccessConfig::singleton().visibleInQuickAccessBar(action));

	return new QuickAccessActionItem(action, true); 
}

void QuickAccessActionsListWidget::moveItem(QTreeWidgetItem* itemToAdd, bool moveUp)
{
	int row = indexOfTopLevelItem(itemToAdd);
	if (itemToAdd && ((moveUp && row > 0) || (!moveUp && row < topLevelItemCount() - 1)))
	{
		takeTopLevelItem(row);
		insertTopLevelItem(moveUp ? row - 1 : row + 1, itemToAdd);

		setCurrentItem(itemToAdd);
	}
}

void QuickAccessActionsListWidget::removeItem(QTreeWidgetItem* itemToRemove)
{
	int row = indexOfTopLevelItem(itemToRemove);
	if (row >= 0)
	{
		QuickAccessActionItem* item = static_cast<QuickAccessActionItem*>(takeTopLevelItem(row));
		if (item)
		{
			delete item;

			// select item above the deleted item
			if (row > topLevelItemCount() - 1)
				row = row - 1;
			selectItem(row);
		}
	}
}

void QuickAccessActionsListWidget::accept()
{
	RobloxQuickAccessConfig::QuickAccessActionCollection quickAccessCollection;
	int numChild = 	topLevelItemCount(), currentChild = 0;
	ActionItem* pCurrentItem = NULL;

	// get list of current actions added in quick access list
	while (currentChild < numChild)
	{		
		pCurrentItem = static_cast<ActionItem*>(topLevelItem(currentChild++));
		if (pCurrentItem && !pCurrentItem->isHidden())
		{
			quickAccessCollection.append(RobloxQuickAccessConfig::ActionData(pCurrentItem->getAction(), 
				                                                             pCurrentItem->data(QAC_VISIBILITY, Qt::CheckStateRole).toBool(),
																			 true));
		}
	}

	// check if we've any uninitialized actions, if not then add them
	RobloxQuickAccessConfig::QuickAccessActionCollection& quickAccessBarActions = RobloxQuickAccessConfig::singleton().getQuickAccessBarActions();
	for (int ii = 0; ii < quickAccessBarActions.size(); ++ii)
	{
		if (!quickAccessBarActions.at(ii).isInitialized)
			quickAccessCollection.append(quickAccessBarActions.at(ii));
	}

	// re-create quick access bar with the latest list of actions
	RobloxQuickAccessConfig::singleton().recreateQuickAccessBar(quickAccessCollection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QuickAccessConfigDialog
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QuickAccessConfigDialog::QuickAccessConfigDialog(QWidget* parent)
: RobloxSavingStateDialog<QDialog>(parent, "QuickAccessConfigDialog/Geometry")
, m_dataChanged(false)
{
	setWindowTitle(tr("Customize Quick Access Bar"));

    setAttribute(Qt::WA_DeleteOnClose,false);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	m_pQuickAccessActionsListWidget = new QuickAccessActionsListWidget(this);
	m_pAllActionListWidget          = new AllActionsListWidget(this);

	m_pFilterEdit = new QLineEdit(this);
	m_pFilterEdit->setPlaceholderText("Search Actions");
	m_pFilterEdit->setFocusPolicy(Qt::ClickFocus);
	
	m_pAddButton = new QPushButton(tr("Add >>"), this);
	m_pRemoveButton = new QPushButton(tr("Remove"), this);

	m_pMoveUpButton = new QPushButton(this);
	m_pMoveUpButton->setIcon(QIcon(":/RibbonBar/images/Studio Ribbon Icons/ribbonMinimize.png"));

	m_pMoveDownButton = new QPushButton(this);
	m_pMoveDownButton->setIcon(QIcon(":/RibbonBar/images/Studio Ribbon Icons/ribbonMaximize.png"));

	m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	connect(m_pAddButton,      SIGNAL(clicked()), this, SLOT(onAddButtonClicked()));
	connect(m_pRemoveButton,   SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()));
	connect(m_pMoveUpButton,   SIGNAL(clicked()), this, SLOT(onMoveButtonClicked()));
	connect(m_pMoveDownButton, SIGNAL(clicked()), this, SLOT(onMoveButtonClicked()));

	connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(cancel()));
    connect(m_dialogButtonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(restoreDefaults(QAbstractButton*)));

	connect(m_pFilterEdit, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));

	connect(m_pQuickAccessActionsListWidget, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(onQuickAccessListDataChanged()));
	connect(&RobloxQuickAccessConfig::singleton(), SIGNAL(defaultsRestored()), this, SLOT(onDefaultsRestored()));

	UpdateUIManager::Instance().getMainWindow().installEventFilter(this);

	doLayout();

	setWindowModality(Qt::ApplicationModal);
}

void QuickAccessConfigDialog::doLayout()
{
	QVBoxLayout* allActionsLayout = new QVBoxLayout();
	allActionsLayout->addWidget(new QLabel(tr("All Actions"), this));
	allActionsLayout->addWidget(m_pFilterEdit);
	allActionsLayout->addWidget(m_pAllActionListWidget);

	QVBoxLayout* buttonLayout = new QVBoxLayout();
	buttonLayout->setObjectName(QString::fromUtf8("verticalLayout"));
	buttonLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
	buttonLayout->addWidget(m_pAddButton);
	buttonLayout->addWidget(m_pRemoveButton);
	buttonLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
	
	QVBoxLayout* quickAccessConfigLayout = new QVBoxLayout();
	quickAccessConfigLayout->addWidget(new QLabel(tr("Quick Access Bar Actions"), this));	
	quickAccessConfigLayout->addWidget(m_pQuickAccessActionsListWidget);

	QVBoxLayout* moveUpDownLayout = new QVBoxLayout();
	moveUpDownLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
	moveUpDownLayout->addWidget(m_pMoveUpButton);
	moveUpDownLayout->addWidget(m_pMoveDownButton);
	moveUpDownLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

	QGridLayout* mainLayout = new QGridLayout(this);
	mainLayout->addLayout(allActionsLayout, 0, 0, 1, 1);
	mainLayout->addLayout(buttonLayout, 0, 1, 1, 1);
	mainLayout->addLayout(quickAccessConfigLayout, 0, 2, 1, 1);
	mainLayout->addLayout(moveUpDownLayout, 0, 3, 1, 1);
	mainLayout->addWidget(m_dialogButtonBox, 1, 0, 1, 4);
}

bool QuickAccessConfigDialog::eventFilter(QObject* watched, QEvent* evt)
{
	if (evt->type() == QEvent::ActionAdded)
	{
		QAction* action = static_cast<QActionEvent*>(evt)->action();

		m_pAllActionListWidget->addAction(action);
		m_pAllActionListWidget->setFilter(m_pFilterEdit->text());
		m_pAllActionListWidget->sort();

		if (RobloxQuickAccessConfig::singleton().availableInQuickAccessBar(action))
		{
			QAction* actionToAdd = RobloxQuickAccessConfig::singleton().getProxyAction(action);
			if (!actionToAdd)
				actionToAdd = action;
			m_pQuickAccessActionsListWidget->addAction(actionToAdd);
		}
	}
	else if (evt->type() == QEvent::ActionRemoved)
	{
		QAction* action = static_cast<QActionEvent*>(evt)->action();

		m_pAllActionListWidget->removeAction(action);
		if (RobloxQuickAccessConfig::singleton().availableInQuickAccessBar(action))
		{
			QAction* isProxyAction = RobloxQuickAccessConfig::singleton().getProxyAction(action);
			if (!isProxyAction)
			{
				m_pQuickAccessActionsListWidget->removeAction(action);
			}
			else
			{
				RobloxQuickAccessConfig::singleton().resetProxyAction(action);
			}
		}
	}

	return QDialog::eventFilter(watched, evt);
}

void QuickAccessConfigDialog::showEvent(QShowEvent* evt)
{
	// reinitialize quickaccess list 
	m_pQuickAccessActionsListWidget->initialize();

	// update all actions list
	m_pAllActionListWidget->setFilter(m_pFilterEdit->text());
	m_pAllActionListWidget->sort();

	// set default focus on cancel button
	if (m_dialogButtonBox->button(QDialogButtonBox::Cancel))
		m_dialogButtonBox->button(QDialogButtonBox::Cancel)->setFocus();

	//now call base class function
	QDialog::showEvent(evt);
}

void QuickAccessConfigDialog::onAddButtonClicked()
{
	ActionItem* pActionItem = static_cast<ActionItem*>(m_pAllActionListWidget->currentItem());
	if (!pActionItem)
		return;
	
	// check if selection action is already available in quick access bar
	if (m_pQuickAccessActionsListWidget->isActionAvailable(pActionItem->getAction()))
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setText("The selected action is already available in Quick Access Bar.\nPlease try to add a different action.");
		msgBox.exec();
		return;
	}

	// map action if it's PluginAction
	QAction* actionToAdd = RobloxQuickAccessConfig::singleton().mapProxyAction(pActionItem->getAction(), true);
	if (!actionToAdd)
		actionToAdd = pActionItem->getAction();

	// add updated action in quick access list, also select the next item in all actions list
	m_pQuickAccessActionsListWidget->addAction(actionToAdd);
	m_pQuickAccessActionsListWidget->selectItem(m_pQuickAccessActionsListWidget->topLevelItemCount() - 1);
	m_pAllActionListWidget->selectItem(m_pAllActionListWidget->indexOfTopLevelItem(pActionItem) + 1);
	m_dataChanged = true;
}

void QuickAccessConfigDialog::onRemoveButtonClicked()
{ 
	// remove current selected item
	m_pQuickAccessActionsListWidget->removeItem(m_pQuickAccessActionsListWidget->currentItem());
	m_dataChanged = true;
}

void QuickAccessConfigDialog::onMoveButtonClicked()
{ 
	// move up/down current selected item
	m_pQuickAccessActionsListWidget->moveItem(m_pQuickAccessActionsListWidget->currentItem(), sender() == m_pMoveUpButton);
	m_dataChanged = true;
}

void QuickAccessConfigDialog::restoreDefaults(QAbstractButton* button)
{
	if (m_dialogButtonBox->button(QDialogButtonBox::RestoreDefaults) == button)
		RobloxQuickAccessConfig::singleton().restoreDefaults();
}

void QuickAccessConfigDialog::cancel()
{
	if (m_dataChanged)
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setText("You have made changes to your settings.\nDo you want to keep your changes?");
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Discard);
		msgBox.setDefaultButton(QMessageBox::Ok);
		int ret = msgBox.exec();

		// recreate quick access bar on OK, cancel will be taken care in showEvent
		if (ret == QMessageBox::Ok)
			m_pQuickAccessActionsListWidget->accept();
	}
    
    m_dataChanged = false;
    hide();
}

void QuickAccessConfigDialog::accept()
{
	if (m_dataChanged)
	{
		m_pQuickAccessActionsListWidget->accept();
		m_dataChanged = false;
	}

    hide();
}

void QuickAccessConfigDialog::onQuickAccessListDataChanged()
{    m_dataChanged = true; }

void QuickAccessConfigDialog::onDefaultsRestored()
{
	m_pQuickAccessActionsListWidget->initialize();
	m_dataChanged = false;
}

void QuickAccessConfigDialog::setFilter(QString filter)
{   m_pAllActionListWidget->setFilter(filter); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RobloxQuickAccessConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RobloxQuickAccessConfig& RobloxQuickAccessConfig::singleton()
{
	static RobloxQuickAccessConfig* robloxQuickAccessConfig = new RobloxQuickAccessConfig();
	return *robloxQuickAccessConfig;
}

RobloxQuickAccessConfig::RobloxQuickAccessConfig()
: m_pMainWindow(&UpdateUIManager::Instance().getMainWindow())
, m_pluginHostInitialized(false)
{
	m_pQuickAccessBar = m_pMainWindow->ribbonBar() ? m_pMainWindow->ribbonBar()->getQuickAccessBar() : NULL;
	connect(m_pQuickAccessBar, SIGNAL(showCustomizeMenu(QMenu*)), this, SLOT(updateQuickAccessMenu(QMenu*)));

	QActionGroup* pActionGroup = m_pQuickAccessBar->findChild<QActionGroup*>();
	if (pActionGroup)
		connect(pActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(updateActionVisiblity(QAction*)));

	// HACK to show "Customize..." action in extension menu!
	QList<QToolButton*> toolButtons = m_pQuickAccessBar->findChildren<QToolButton*>();
	for (int jj = 0; jj < toolButtons.size(); ++jj)
	{
		QToolButton* button = toolButtons.at(jj);
		if (button->objectName() == "qt_toolbar_ext_button" && button->menu())
		{
			connect(button->menu(), SIGNAL(aboutToShow()), this, SLOT(onExtensionMenuShow()));
			connect(button->menu(), SIGNAL(aboutToHide()), this, SLOT(onExtensionMenuHide()));
			break;
		}
	}
}

void RobloxQuickAccessConfig::updateQuickAccessMenu(QMenu* pMenu)
{
	pMenu->addSeparator()->setText(tr("Advanced Customization"));
	pMenu->addAction(m_pMainWindow->customizeQuickAccessAction);
}

void RobloxQuickAccessConfig::updateActionVisiblity(QAction* action)
{
	QuickAccessActionCollection::iterator iter = m_currentQuickAccessBarActions.begin();
	while (iter != m_currentQuickAccessBarActions.end())
	{
		iter->isVisible = m_pQuickAccessBar->isActionVisible(iter->action);
		++iter;
	}
}

void RobloxQuickAccessConfig::onExtensionMenuShow()
{
	QMenu* menu = qobject_cast<QMenu*>(sender());
	if (menu)
	{
		m_pSeparationAction = menu->addSeparator();
		menu->addAction(m_pMainWindow->customizeQuickAccessAction);
	}
}

void RobloxQuickAccessConfig::onExtensionMenuHide()
{
	QMenu* menu = qobject_cast<QMenu*>(sender());
	if (menu)
	{
		menu->removeAction(m_pSeparationAction);
		menu->removeAction(m_pMainWindow->customizeQuickAccessAction);
	}
}

void RobloxQuickAccessConfig::storeDefaults(const QuickAccessActionCollection& defaultQuickAccessBarActions) 
{ m_defaultQuickAccessBarActions = defaultQuickAccessBarActions;  }

void RobloxQuickAccessConfig::restoreDefaults()
{ 
	recreateQuickAccessBar(m_defaultQuickAccessBarActions);
	Q_EMIT defaultsRestored();
}

void RobloxQuickAccessConfig::recreateQuickAccessBar(const QuickAccessActionCollection& quickAccessBarActions)
{
	m_pQuickAccessBar->setUpdatesEnabled(false);

	// first remove all existing actions from Quick Access bar
	QList<QAction*> proxyActions;
	QuickAccessActionCollection::iterator quickAccessActionIter = m_currentQuickAccessBarActions.begin();
	while (quickAccessActionIter != m_currentQuickAccessBarActions.end())
	{
		if (qobject_cast<QuickAccessBarProxyAction*>(quickAccessActionIter->action))
			proxyActions.append(quickAccessActionIter->action);
		removeFromQuickAccessBar(quickAccessActionIter->action);
		++quickAccessActionIter;
	}
	m_currentQuickAccessBarActions.clear();

	// now add actions
	QuickAccessActionCollection::const_iterator iter = quickAccessBarActions.constBegin();
	while (iter != quickAccessBarActions.constEnd()) 
	{
		// if the action is initialized then directly add to quickaccess bar
		if (iter->isInitialized)
		{
			addToQuickAccessBar(iter->action, iter->isVisible);
		}
		// or else wait till it get initialized
		else
		{
			m_currentQuickAccessBarActions.append(*iter);
		}
		
		if (qobject_cast<QuickAccessBarProxyAction*>(iter->action))
			proxyActions.removeAll(iter->action);
		++iter;
	}

	// delete unutilized proxy actions
	QList<QAction*>::iterator proxyActionsIter = proxyActions.begin();
	while (proxyActionsIter != proxyActions.end())
	{
		delete *proxyActionsIter;
		++proxyActionsIter;
	}

	m_pQuickAccessBar->setUpdatesEnabled(true);
}

bool RobloxQuickAccessConfig::loadQuickAccessConfig()
{
	QSettings settings;
	int size = settings.beginReadArray(QuickAccessConfigKey);
	if (size < 1)
	{
		settings.endArray();
		return false;
	}

	QString objectName;
	for (int i = 0; i < size; ++i) 
	{
		settings.setArrayIndex(i);
		objectName = settings.value(QuickAccessActionNameKey).toString();
		
		bool visibility = settings.value(QuickAccessActionVisibleKey, true).toBool();

		QAction*            pAction = m_pMainWindow->findChild<QAction*>(objectName);
		PluginAction* pPluginAction = dynamic_cast<PluginAction*>(pAction);
		
		
		
		if (pPluginAction || !pAction)
		{
			QuickAccessBarProxyAction* pProxyAction = new QuickAccessBarProxyAction(m_pMainWindow);
			pProxyAction->setObjectName(objectName);
			pProxyAction->setText(settings.value(QuickAccessActionTextKey).toString());
			pProxyAction->setShortcut(settings.value(QuickAccessActionShortcutKey).toString());
			// do not add to quick access bar instead just have it in local data
			m_currentQuickAccessBarActions.append(ActionData(pProxyAction, visibility));
		}
		else if (FFlag::StudioSeparateActionByActivationMethod)
		{
			QuickAccessBarProxyAction* pProxyAction = new QuickAccessBarProxyAction(m_pMainWindow);
			pProxyAction->setObjectName(objectName);
			pProxyAction->setText(settings.value(QuickAccessActionTextKey).toString());
			pProxyAction->setShortcut(pAction->shortcut().toString());
			pProxyAction->setSourceAction(pAction);
			addToQuickAccessBar(pProxyAction, visibility);
		}
		else
		{
			addToQuickAccessBar(pAction, visibility);	
		}
	}

	settings.endArray();
	return true;
}

void RobloxQuickAccessConfig::saveQuickAccessConfig()
{
	// remove all settings data for quick access
	QSettings settings;
	settings.beginWriteArray(QuickAccessConfigKey);
	settings.remove("");
	settings.endArray();

	// recreate new data
	settings.beginWriteArray(QuickAccessConfigKey);

	int index = 0;
	QAction* action = NULL;
	QuickAccessActionCollection::const_iterator iter = m_currentQuickAccessBarActions.constBegin();
	while (iter != m_currentQuickAccessBarActions.constEnd()) 
	{
		action = iter->action.data();
		if (action && (iter->isInitialized || !m_pluginHostInitialized))
		{
			settings.setArrayIndex(index++);
			settings.setValue(QuickAccessActionNameKey,    action->objectName());
			settings.setValue(QuickAccessActionVisibleKey, (bool)iter->isVisible);

			if (iter->isProxyAction())
			{
				settings.setValue(QuickAccessActionTextKey, action->text());
				settings.setValue(QuickAccessActionShortcutKey, action->shortcut().toString());
			}
		}

		++iter;
	}

	settings.endArray();
}

void RobloxQuickAccessConfig::addToQuickAccessBar(QAction* actionToAdd, bool visibility)
{
	if (!actionToAdd || !m_pQuickAccessBar)
		return;

	QAction* pActionBefore = NULL;
	// check if we can update already existing proxy action or else add data
	if (!updateProxyAction(actionToAdd))
	{
		m_currentQuickAccessBarActions.append(ActionData(actionToAdd, visibility, true));
	}
	else
	{
		bool actionFound = false;
		QuickAccessActionCollection::iterator iter = m_currentQuickAccessBarActions.begin();
		while (iter != m_currentQuickAccessBarActions.end())
		{
			if (actionFound && iter->isInitialized && iter->action)
			{
				pActionBefore = iter->action;
				break;
			}

			if (iter->action == actionToAdd)
				actionFound = true;

			++iter;
		}
	}
	
	pActionBefore ? m_pQuickAccessBar->insertAction(pActionBefore, actionToAdd) : m_pQuickAccessBar->addAction(actionToAdd);
	if (m_pQuickAccessBar->isActionVisible(actionToAdd) != visibility)
		m_pQuickAccessBar->setActionVisible(actionToAdd, visibility);
	Q_EMIT actionAdded(actionToAdd);
}

void RobloxQuickAccessConfig::removeFromQuickAccessBar(QAction* actionToRemove)
{
	if (!actionToRemove || !m_pQuickAccessBar)
		return;

	QuickAccessActionCollection::iterator iter = m_currentQuickAccessBarActions.begin();
	while (iter != m_currentQuickAccessBarActions.end()) 
	{ 
		if (iter->action == actionToRemove)
		{
			m_currentQuickAccessBarActions.erase(iter);
			break;
		}
		++iter;
	}

	// remove all instances of action i.e. remove from both quick access bar and menu
	Q_EMIT actionRemoved(actionToRemove);
	m_pQuickAccessBar->removeAllInstances(actionToRemove);
}

QAction* RobloxQuickAccessConfig::mapProxyAction(QAction* action, bool force)
{
	// proxy actions are used by PluginAction only
	PluginAction* pluginAction = dynamic_cast<PluginAction*>(action);
	if (!pluginAction && (!FFlag::StudioSeparateActionByActivationMethod || !force))
		return NULL;

	QuickAccessBarProxyAction* pProxyAction = dynamic_cast<QuickAccessBarProxyAction*>(getProxyAction(action));
	if (!pProxyAction && !force)
		return NULL;

	if (!pProxyAction)
	{
		// if no proxy action is available then create new
		pProxyAction = new QuickAccessBarProxyAction(m_pMainWindow);
		pProxyAction->setObjectName(action->objectName() + QuickAccessProxyActionExt);
	}

	// update source action
	pProxyAction->setSourceAction(action);
	return pProxyAction;
}

QAction* RobloxQuickAccessConfig::resetProxyAction(QAction* action)
{
	QuickAccessBarProxyAction* pProxyAction = dynamic_cast<QuickAccessBarProxyAction*>(getProxyAction(action));
	if (!pProxyAction)
		return NULL;

	pProxyAction->setSourceAction(NULL);
	return pProxyAction;
}

bool RobloxQuickAccessConfig::updateProxyAction(QAction* action)
{
	QuickAccessActionCollection::iterator iter = m_currentQuickAccessBarActions.begin();
	while (iter != m_currentQuickAccessBarActions.end())
	{
		if (iter->action == action)
		{
			iter->isInitialized = true;
			return true;
		}
		++iter;
	}
	return false;
}

bool RobloxQuickAccessConfig::visibleInQuickAccessBar(QAction* action)
{
	QAction* actionToLookFor = getProxyAction(action);
	if (!actionToLookFor)
		actionToLookFor = action;
	QuickAccessActionCollection::const_iterator iter = m_currentQuickAccessBarActions.constBegin();
	while (iter != m_currentQuickAccessBarActions.constEnd())
	{
		if (iter->action == actionToLookFor)
			return iter->isVisible;
		++iter;
	}
	return false;
}

bool RobloxQuickAccessConfig::availableInQuickAccessBar(QAction* action)
{
	QAction* actionToLookFor = getProxyAction(action);
	if (!actionToLookFor)
		actionToLookFor = action;

	QuickAccessActionCollection::const_iterator iter = m_currentQuickAccessBarActions.constBegin();
	while (iter != m_currentQuickAccessBarActions.constEnd())
	{
		if (iter->action == actionToLookFor)
			return true;
		++iter;
	}
	return false;
}

QAction* RobloxQuickAccessConfig::getProxyAction(QAction* action)
{
	QString proxyActObjName = action->objectName()  + QuickAccessProxyActionExt;
	QString actionText      = action->text();

	QList<QuickAccessBarProxyAction*> proxyActions = m_pMainWindow->findChildren<QuickAccessBarProxyAction*>();
	for (int ii = 0; ii < proxyActions.size(); ++ii)
	{
		if ((proxyActions[ii]->text() == actionText) && (proxyActions[ii]->objectName() == proxyActObjName))
			return proxyActions[ii];
	}

	return NULL;
}

 bool RobloxQuickAccessConfig::ActionData::isProxyAction() const
 { return dynamic_cast<QuickAccessBarProxyAction*>(action.data()); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QuickAccessBarProxyAction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QuickAccessBarProxyAction::QuickAccessBarProxyAction(QObject* parent)
: QAction(parent)
, m_pCurrentSourceAction(NULL)
{
	onChanged();
}

void QuickAccessBarProxyAction::setSourceAction(QAction* pSourceAction)
{
	if ((m_pCurrentSourceAction != pSourceAction) || !pSourceAction)
	{
		if (m_pCurrentSourceAction)
		{
			disconnect(this, SIGNAL(triggered()), m_pCurrentSourceAction, SLOT(trigger()));
			disconnect(m_pCurrentSourceAction, SIGNAL(changed()), this, SLOT(onChanged()));
		}

		m_pCurrentSourceAction = pSourceAction;

		if (pSourceAction)
		{
			m_pSourceActions.push(pSourceAction);
			
			setCheckable(pSourceAction->isCheckable());
			pSourceAction->setShortcut(shortcut());

			setText(pSourceAction->text());
			setIcon(pSourceAction->icon());
			
			if (FFlag::StudioSeparateActionByActivationMethod)
			{
				setShortcut(QKeySequence());
				connect(this, SIGNAL(triggered()), this, SLOT(onActionTriggered()));
			}
			else
			{
				connect(this, SIGNAL(triggered()), pSourceAction, SLOT(trigger()));
			}
			connect(pSourceAction, SIGNAL(changed()), this, SLOT(onChanged()));
			connect(pSourceAction, SIGNAL(destroyed(QObject*)), this, SLOT(onSourceActionDestroyed(QObject*)));
		}
	}

	onChanged();
}

void QuickAccessBarProxyAction::onActionTriggered()
{
	if (!UpdateUIManager::Instance().getMainWindow().commonSlotQuickAccess(m_pCurrentSourceAction))
	{
		UpdateUIManager::Instance().getMainWindow().setActionCounterSendingEnabled(false);
		m_pCurrentSourceAction->trigger();
		UpdateUIManager::Instance().getMainWindow().setActionCounterSendingEnabled(true);
	}
}

bool QuickAccessBarProxyAction::hasAction(QAction* action)
{
	for (QStack<QAction*>::iterator i = m_pSourceActions.begin(); i != m_pSourceActions.end(); ++i)
	{
		if (*i == action)
			return true;
	}

	return false;
}

void QuickAccessBarProxyAction::onChanged()
{
	setEnabled(m_pCurrentSourceAction ? m_pCurrentSourceAction->isEnabled() : false);
	setChecked(m_pCurrentSourceAction ? m_pCurrentSourceAction->isChecked() : false);
	setVisible(m_pCurrentSourceAction ? m_pCurrentSourceAction->isVisible() : true);
	if (m_pCurrentSourceAction)
		setIcon(m_pCurrentSourceAction->icon());
}

void QuickAccessBarProxyAction::onSourceActionDestroyed(QObject* object)
{
	bool currentDestroyed = (object == m_pSourceActions.top());
	for (QStack<QAction*>::iterator i = m_pSourceActions.begin(); i != m_pSourceActions.end(); ++i)
	{
		if (*i == object)
		{
			m_pSourceActions.erase(i);
			break;
		}
	}

	if (currentDestroyed)
	{
		m_pCurrentSourceAction = NULL;

		QAction* sourceAction = NULL;
		if (!m_pSourceActions.isEmpty())
		{
			sourceAction = m_pSourceActions.pop();
			disconnect(sourceAction, SIGNAL(destroyed(QObject*)), this, SLOT(onSourceActionDestroyed(QObject*)));
		}

		setSourceAction(sourceAction);
	}
}
