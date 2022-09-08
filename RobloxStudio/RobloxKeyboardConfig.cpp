/**
 * RobloxKeyboardConfig.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxKeyboardConfig.h"

#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeView>

#include "RobloxMainWindow.h"
#include "StudioUtilities.h"

FASTFLAG(StudioShowTutorialsByDefault)

// shortcuts that should not be visible to the average user
static const QString ShortcutExceptions = 
    "toggleBuildModeAction filePublishedProjectsAction screenShotAction";

enum eShortcutTableColumns
{
    STC_NAME = 0,
    STC_SHORTCUT,
    STC_DESCRIPTION,
    STC_MAX,
};

// A comparator for sorting shortcuts and their overrides.
class ShortcutComparator
{
public:
    ShortcutComparator(int columnArg, Qt::SortOrder orderArg, ShortcutsModel& modelArg)
        : column(columnArg)
        , order(orderArg)
        , model(modelArg)
        {}
    
    bool operator()(const int& a, const int& b) const
    {
        QString aData = model.m_shortcutData[a].toString(column);
        QString bData = model.m_shortcutData[b].toString(column);
        
        if (order == Qt::AscendingOrder)
            return aData.toLower() > bData.toLower();
        else
            return aData.toLower() < bData.toLower();
    }
    
    int column;
    Qt::SortOrder order;
    ShortcutsModel& model;
};


KeySequenceEdit::KeySequenceEdit(QWidget* parent)
    : QWidget(parent)
    , m_num(0)
{
    setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *layout = new QHBoxLayout(this);
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setFixedHeight(20);
    
    layout->addWidget(m_lineEdit);
    layout->setMargin(0);
    layout->setContentsMargins(0, 0, 0, 0);
    m_lineEdit->installEventFilter(this);
    m_lineEdit->setReadOnly(true);
    m_lineEdit->setFocusProxy(this);
    setFocusPolicy(m_lineEdit->focusPolicy());
    setAttribute(Qt::WA_InputMethodEnabled);
}

bool KeySequenceEdit::eventFilter(QObject* o, QEvent* e)
{
    // Create a right click context menu for clearing the QKeySequence set.
    if (o == m_lineEdit && e->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent *c = static_cast<QContextMenuEvent *>(e);
        QMenu *menu = m_lineEdit->createStandardContextMenu();
        menu->clear();

        QAction *clearAction = new QAction(tr("Clear Shortcut"), menu);
        menu->insertAction(NULL, clearAction);
        clearAction->setEnabled(!m_keySequence.isEmpty());
        connect(clearAction, SIGNAL(triggered()), this, SLOT(slotClearShortcut()));

        menu->exec(c->globalPos());
        delete menu;
        e->accept();

        return true;
    }

    return QWidget::eventFilter(o, e);
}

void KeySequenceEdit::slotClearShortcut()
{
    if (m_keySequence.isEmpty())
        return;
    
    setKeySequence(QKeySequence());
    Q_EMIT keySequenceChanged(m_keySequence);
}

void KeySequenceEdit::handleKeyEvent(QKeyEvent* e)
{
    // Main key processing function.  All keys pressed will pass through
    // here and then be converted to QKeySequences.
    
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
    {
        Q_EMIT(editingFinished());
        return;
    }
    
    int key = e->key();
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Meta    || key == Qt::Key_Alt   ||
        key == Qt::Key_Super_L || key == Qt::Key_AltGr)
        return;

    key |= StudioUtilities::translateKeyModifiers(e->modifiers(), e->text());
    m_keySequence = QKeySequence(key);
    m_lineEdit->setText(m_keySequence.toString(QKeySequence::NativeText));
    e->accept();
    Q_EMIT keySequenceChanged(m_keySequence);
}

void KeySequenceEdit::setKeySequence(const QKeySequence &sequence)
{
    if (sequence == m_keySequence)
        return;
    m_num = 0;
    m_keySequence = sequence;
    m_lineEdit->setText(m_keySequence.toString(QKeySequence::NativeText));
}

QKeySequence KeySequenceEdit::keySequence() const
{
    return m_keySequence;
}

void KeySequenceEdit::focusInEvent(QFocusEvent *e)
{
    m_lineEdit->event(e);
    m_lineEdit->selectAll();
    QWidget::focusInEvent(e);
}

void KeySequenceEdit::focusOutEvent(QFocusEvent *e)
{
    m_num = 0;
    m_lineEdit->event(e);
    QWidget::focusOutEvent(e);
}

void KeySequenceEdit::keyPressEvent(QKeyEvent *e)
{
    handleKeyEvent(e);
    e->accept();
}

void KeySequenceEdit::keyReleaseEvent(QKeyEvent *e)
{
    m_lineEdit->event(e);
}

bool KeySequenceEdit::event(QEvent *e)
{
    if (e->type() == QEvent::Shortcut ||
        e->type() == QEvent::ShortcutOverride  ||
        e->type() == QEvent::KeyRelease)
    {
        e->accept();
        return true;
    }
    
    return QWidget::event(e);
}


////////////////////////////////////////////////////////////////////////////////

QString ShortcutData::toString(int column) const
{
    if (column == STC_NAME)
    {
        return m_action->text();
    }
    else if (column == STC_SHORTCUT)
    {
        if (hasOverride())
        {
            const QKeySequence& override = m_shortcutOverride;
            return override.toString(QKeySequence::NativeText);
        }
        
        return m_action->shortcut().toString(QKeySequence::NativeText);
    }
    else if (column == STC_DESCRIPTION)
    {
        return m_action->statusTip();
    }
    
    return QString();
}


////////////////////////////////////////////////////////////////////////////////

ShortcutsModel::ShortcutsModel(RobloxMainWindow& mainWindow, QObject *parent)
    : QAbstractTableModel(parent)
    , m_mainWindow(mainWindow)
{
    resetData();
}

void ShortcutsModel::resetData()
{
    // Performs a complete flush of the model and repopulates it based
    // off of the mainWindow's action loadout.
    
    m_shortcutData.clear();
    m_sortOrder.clear();

    int i = 0;
    QObjectList objects = m_mainWindow.children();
    QObjectList::iterator iter = objects.begin();
    for ( ; iter != objects.end() ; ++iter )
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if ( action && 
             !action->text().isEmpty() &&
             !action->text().startsWith("&") &&
             !ShortcutExceptions.contains(action->objectName())  &&
			 !(!FFlag::StudioShowTutorialsByDefault && action->objectName() == "viewTutorialsAction"))
        {
            m_shortcutData.push_back(ShortcutData(action));
            m_sortOrder.push_back(i++);
        }
    }

    Q_EMIT(dataChanged(index(0,0), index(rowCount(), columnCount())));
}

int ShortcutsModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_shortcutData.size();
}

int ShortcutsModel::columnCount(const QModelIndex& /*parent*/) const
{
    return STC_MAX;
}

// awagnerTODO: add a search bar and use a QSortFilterProxyModel
void ShortcutsModel::sort(int column, Qt::SortOrder order)
{
    ShortcutComparator comparator(column, order, *this);
    qSort(m_sortOrder.begin(), m_sortOrder.end(), comparator);
    Q_EMIT(layoutChanged());
}

QVariant ShortcutsModel::data(const QModelIndex &index, int role) const
{
    const ShortcutData& shortcutData = m_shortcutData[m_sortOrder[index.row()]];
    const QAction* action = shortcutData.action();

    if (role == Qt::DecorationRole)
    {
        if (index.column() == STC_NAME)
        {
            QIcon icon = action->icon();
            if ( icon.isNull() )
            {
                QPixmap pix(16,16);
                pix.fill(QColor(0,0,0,0));
                icon = QIcon(pix);   
            }
            
            return icon;
        }
    }
    if (role == Qt::EditRole)
    {
        return QVariant::fromValue(shortcutData);
    }
    if (role == Qt::DisplayRole)
    {
        return shortcutData.toString(index.column());
    }
    return QVariant();
}

bool ShortcutsModel::setData(const QModelIndex & index, const QVariant& value, int role)
{
    if (role == Qt::EditRole && index.column() == STC_SHORTCUT)
    {
        ShortcutData shortcut = qvariant_cast<ShortcutData>(value);
        ShortcutData& shortcutData = m_shortcutData[m_sortOrder[index.row()]];
        shortcutData = shortcut;
        Q_EMIT(dataChanged(index, index));
        return true;
    }
    
    return false;
}

QVariant ShortcutsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case STC_NAME:
                return QString("Name");
            case STC_SHORTCUT:
                return QString("Shortcut");
            case STC_DESCRIPTION:
                return QString("Description");
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags ShortcutsModel::flags(const QModelIndex& index) const
{
    if (index.column() == STC_SHORTCUT)
        return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void ShortcutsModel::commitOverrides()
{
    for (QList<ShortcutData>::iterator iter = m_shortcutData.begin();
         iter != m_shortcutData.end();
         iter++)
    {
        if (iter->hasOverride())
        {
            iter->action()->setShortcut(iter->shortcutOverride());
        }
    }
}

void ShortcutsModel::resetOverrides()
{
    for (QList<ShortcutData>::iterator iter = m_shortcutData.begin();
         iter != m_shortcutData.end();
         iter++)
    {
        if (iter->hasOverride())
        {
            iter->removeShortcutOverride();
        }
    }
}

bool ShortcutsModel::removeShortcut(const QKeySequence& keySequence)
{
    for (QList<ShortcutData>::iterator iter = m_shortcutData.begin();
         iter != m_shortcutData.end();
         iter++)
    {
        // If an action already has the shortcut, then add a override.  Or if
        // the shortcut is overriden and it matches the keysequence, then set
        // the override to empty.

        if (iter->hasOverride())
        {
            if (iter->shortcutOverride().toString() == keySequence.toString())
            {
                iter->setShortcutOverride(QKeySequence());
            }
        }
        else if (iter->action()->shortcut().toString() == keySequence.toString())
        {
            iter->setShortcutOverride(QKeySequence());
        }
    }

	return false;
}


////////////////////////////////////////////////////////////////////////////////

QWidget* ShortcutDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const

{
    if (index.column() == STC_SHORTCUT)
    {
        KeySequenceEdit *editor = new KeySequenceEdit(parent);
        return editor;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void ShortcutDelegate::setEditorData(QWidget *editor,
                   const QModelIndex &index) const
{
    if (index.column() == STC_SHORTCUT)
    {
        const ShortcutsModel* model = qobject_cast<const ShortcutsModel*>(index.model());

        if (!model)
            return;
    
        ShortcutData shortcutData = qvariant_cast<ShortcutData>(model->data(index, Qt::EditRole));
       
        KeySequenceEdit *keySequenceEdit = qobject_cast<KeySequenceEdit*>(editor);
        
        if (shortcutData.hasOverride())
        {
            const QKeySequence& override = shortcutData.shortcutOverride();
            keySequenceEdit->setKeySequence(override);
            return;
        }
        
        keySequenceEdit->setKeySequence(shortcutData.action()->shortcut());
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

QSize ShortcutDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.column() == STC_SHORTCUT)
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(28);
        return size;
    }
    else
    {
        return QStyledItemDelegate::sizeHint(option, index);
    }
}

void ShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    // Called when the edit has finished.  Saves the QKeySequence generated
    // to a shortcut override on the model, first removing it from any action
    // that may already have it.
    
    if (index.column() == STC_SHORTCUT)
    {
        KeySequenceEdit *keySequenceEdit = qobject_cast<KeySequenceEdit*>(editor);

        if (!keySequenceEdit)
            return;

        ShortcutsModel* shortcutsModel = qobject_cast<ShortcutsModel*>(model);
    
        if (!shortcutsModel)
            return;
    
        ShortcutData shortcut = qvariant_cast<ShortcutData>(shortcutsModel->data(index, Qt::EditRole));

        const QKeySequence& keySequence = keySequenceEdit->keySequence();
        
        // Remove the shortcut from all actions if it's already assigned.
        shortcutsModel->removeShortcut(keySequence);
        
        shortcut.setShortcutOverride(keySequence);
        shortcutsModel->setData(index, QVariant::fromValue(shortcut));
    }
    else
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}


////////////////////////////////////////////////////////////////////////////////

RobloxKeyboardConfig::RobloxKeyboardConfig()
{
}

RobloxKeyboardConfig& RobloxKeyboardConfig::singleton()
{
    static RobloxKeyboardConfig* robloxKeyboardConfig = new RobloxKeyboardConfig();
    return *robloxKeyboardConfig;
}

void RobloxKeyboardConfig::storeDefaults(RobloxMainWindow& mainWindow)
{
    // Store off all the action shortcuts in their default state.

    QObjectList objects = mainWindow.children();
    QObjectList::iterator iter = objects.begin();
    for (; iter != objects.end(); ++iter)
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if (action &&
            !action->text().isEmpty() &&
            !action->text().startsWith("&") &&
            !ShortcutExceptions.contains(action->objectName()))
        {
            m_defaultShortcuts[action->objectName()] = action->shortcut();
        }
    }
}

void RobloxKeyboardConfig::saveKeyboardConfig(RobloxMainWindow& mainWindow)
{
    // Save any action shortcut that deviates from its default in a QSetting.

    QMap<QString, QKeySequence> keymap;

    QObjectList objects = mainWindow.children();
    QObjectList::iterator iter = objects.begin();
    for ( ; iter != objects.end() ; ++iter )
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if (action &&
            !action->text().isEmpty() &&
            !action->text().startsWith("&") &&
            !ShortcutExceptions.contains(action->objectName()) && 
			!(!FFlag::StudioShowTutorialsByDefault && action->objectName() == "viewTutorialsAction"))
        {
            QMap<QString, QKeySequence>::iterator iter = m_defaultShortcuts.find(action->objectName());
            if (iter != m_defaultShortcuts.end())
            {
                if(iter.value().toString(QKeySequence::NativeText) != action->shortcut().toString(QKeySequence::NativeText))
                {
                    keymap[action->objectName()] = action->shortcut();
                }
            }
        }
    }
    
    QSettings settings;
    
    // The only way to clear a group is by calling remove on an empty string.
    // http://qt-project.org/doc/qt-4.8/qsettings.html#remove
    //
    settings.beginGroup("Roblox Studio Key Mapping");
    settings.remove("");
    settings.endGroup();
    
    settings.beginGroup("Roblox Studio Key Mapping");
    QMap<QString, QKeySequence>::const_iterator i = keymap.constBegin();
    while (i != keymap.constEnd())
    {
         settings.setValue(i.key(), i.value().toString());
         ++i;
    }
    settings.endGroup();
}

void RobloxKeyboardConfig::loadKeyboardConfig(RobloxMainWindow& mainWindow)
{
    // Load the shortcut QSettings group into a map and map it back to each
    // action by objectName().
	
    QMap<QString, QKeySequence> keymap;

    QSettings settings;
    settings.beginGroup("Roblox Studio Key Mapping");
    QStringList keys = settings.childKeys();
    Q_FOREACH (QString key, keys) {
         keymap[key] = QKeySequence(settings.value(key).toString());
    }
    settings.endGroup();
    
    QObjectList objects = mainWindow.children();
    QObjectList::iterator iter = objects.begin();
    for ( ; iter != objects.end() ; ++iter )
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if (action &&
            !action->text().isEmpty() &&
            !action->text().startsWith("&") &&
            !ShortcutExceptions.contains(action->objectName()) &&
			!(!FFlag::StudioShowTutorialsByDefault && action->objectName() == "viewTutorialsAction"))
        {
            QMap<QString, QKeySequence>::iterator iter = keymap.find(action->objectName());
            if (iter != keymap.end())
            {
                action->setShortcut(iter.value());
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////

RobloxKeyboardConfigWidget::RobloxKeyboardConfigWidget(RobloxMainWindow& MainWindow,
                                                       QWidget* parent)
    : QWidget(parent)
    , m_mainWindow(MainWindow)
	, m_filterEdit(NULL)
{
    initialize();
}

void RobloxKeyboardConfigWidget::initialize()
{
    RBXASSERT(!layout());

    RBX::BaldPtr<QLayout> layout = new QVBoxLayout;

	m_filterEdit = new QLineEdit(this);
	m_filterEdit->setPlaceholderText("Search Actions");
	m_filterEdit->setFocusPolicy(Qt::ClickFocus);
	layout->addWidget(m_filterEdit);
	connect(m_filterEdit, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));
    
    m_treeView = new QTreeView(this);
    layout->addWidget(m_treeView);
    setLayout(layout);

    m_model = new ShortcutsModel(m_mainWindow, this);
    m_treeView->setModel(m_model);
    m_treeView->setItemsExpandable(false);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSortingEnabled(true);
    m_treeView->header()->setDefaultSectionSize(200);
    connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            this,    SLOT(dataChanged(const QModelIndex&, const QModelIndex&)));
    
    m_treeView->setItemDelegateForColumn(STC_SHORTCUT, new ShortcutDelegate(this));
}

void RobloxKeyboardConfigWidget::cancel()
{
    m_model->resetOverrides();
}

void RobloxKeyboardConfigWidget::accept()
{
    m_model->commitOverrides();
	m_mainWindow.updateShortcutSet();
    RobloxKeyboardConfig::singleton().saveKeyboardConfig(m_mainWindow);
}

void RobloxKeyboardConfigWidget::restoreAllDefaults()
{
    // Iterate through all the actions on mainWindow and set them back to the
    // defaultShortcuts we stored off after application load.

    m_treeView->closePersistentEditor(m_treeView->currentIndex());

    QObjectList objects = m_mainWindow.children();
    QObjectList::iterator iter = objects.begin();
    for (; iter != objects.end(); ++iter)
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if (action &&
            !action->text().isEmpty() &&
            !action->text().startsWith("&") &&
            !ShortcutExceptions.contains(action->objectName()))
        {
            QMap<QString, QKeySequence>::const_iterator iter =
                RobloxKeyboardConfig::singleton().defaultShortcuts().find(action->objectName());
            if (iter != RobloxKeyboardConfig::singleton().defaultShortcuts().end())
            {
                action->setShortcut(iter.value());
            }
        }
    }
    
    m_model->resetData();
    m_treeView->sortByColumn(0, Qt::DescendingOrder);
    Q_EMIT(dataChanged());
}

void RobloxKeyboardConfigWidget::dataChanged(const QModelIndex&,
                                             const QModelIndex&)
{
    Q_EMIT(dataChanged());
}

void RobloxKeyboardConfigWidget::setFilter(QString filter)
{
	setUpdatesEnabled(false);
	QAbstractItemModel* treeViewModel = m_treeView->model();

	int numRows = treeViewModel->rowCount();
	QString text;
	
	for (int ii = 0; ii < numRows; ++ii)
	{
		text = treeViewModel->data(treeViewModel->index(ii, 0)).toString();
		if (filter.isEmpty() || text.contains(filter, Qt::CaseInsensitive))
			m_treeView->setRowHidden(ii, QModelIndex(), false);
		else
			m_treeView->setRowHidden(ii, QModelIndex(), true);
	}
	setUpdatesEnabled(true);
}