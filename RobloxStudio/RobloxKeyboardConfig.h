/**
 * RobloxKeyboardConfig.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QAbstractTableModel>
#include <QKeySequence>
#include <QMap>
#include <QString>
#include <QStyledItemDelegate>
#include <QWidget>

#include "rbx/BaldPtr.h"

class RobloxMainWindow;
class QAbstractButton;
class QAction;
class QDialogButtonBox;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QLineEdit;
class QPushButton;
class QTreeView;


//
// ShorcutData is used by the model to track actions and their shortcut
// overrides set by the editor.
//
class ShortcutData
{
public:
    ShortcutData()
        : m_action(0)
        , m_hasOverride(false)
    {}
    
    ShortcutData(QAction* actionIn)
        : m_action(actionIn)
        , m_hasOverride(false)
    {}
    
    bool hasOverride() const { return m_hasOverride; }
    const QKeySequence& shortcutOverride() const { return m_shortcutOverride; }
    
    void setShortcutOverride(const QKeySequence& keySequence)
    {
        m_shortcutOverride = keySequence;
        m_hasOverride = true;
    }
    
    void removeShortcutOverride()
    {
        m_shortcutOverride = QKeySequence();
        m_hasOverride = false;
    }

    QAction* action() { return m_action; }
    const QAction* action() const { return m_action; }

    QString toString(int column) const;

private:
    QAction*          m_action;
    bool              m_hasOverride;
    QKeySequence      m_shortcutOverride;
};

Q_DECLARE_METATYPE(ShortcutData)


//
// KeySequenceEdit is a line edit for capturing key presses and converting
// them into QKeySequences.
//
class KeySequenceEdit : public QWidget
{
    Q_OBJECT
public:
    KeySequenceEdit(QWidget* parent = 0);

    QKeySequence keySequence() const;
    bool eventFilter(QObject* o, QEvent* e);

public Q_SLOTS:
    void setKeySequence(const QKeySequence& sequence);
    
Q_SIGNALS:
    void keySequenceChanged(const QKeySequence& sequence);
    void editingFinished();

protected:
    void focusInEvent(QFocusEvent* e);
    void focusOutEvent(QFocusEvent* e);
    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* e);
    bool event(QEvent *e);

private Q_SLOTS:
    void slotClearShortcut();

private:
    void handleKeyEvent(QKeyEvent *e);

    int             m_num;
    QKeySequence    m_keySequence;
    QLineEdit*      m_lineEdit;
    QPushButton*    m_resetButton;
};


//
// A delegate for QAbstractItemViews.  ShortcutDelegate creates a
// KeySequenceEdit and passes any new shortcut overrides to the ShortcutsModel.
//
class ShortcutDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ShortcutDelegate(QObject* parent=0): QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const;

    void setEditorData(QWidget* editor,
                       const QModelIndex& index) const;

    void setModelData(QWidget *editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const;
    
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
    
};


//
// A table model for storing/querying/modyfing action to shortcut override
// mappings on a given view.
//
class ShortcutsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ShortcutsModel(RobloxMainWindow& mainWindow, QObject *parent);

    void resetData();
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    
    bool removeShortcut(const QKeySequence& keySequence);
    
    void commitOverrides();
    void resetOverrides();  

protected:
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    
private:
    friend class ShortcutComparator;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    
    RobloxMainWindow&   m_mainWindow;
    QList<int>          m_sortOrder;
    QList<ShortcutData> m_shortcutData;
};

//
// A singleton object for storing and handling keyboard configuration changes.
// QActions store the actual configuration state. This class could be
// extended to do a query for a specific action to key mapping.
//
class RobloxKeyboardConfig
{
public:
    RobloxKeyboardConfig();
    
    static RobloxKeyboardConfig& singleton();

    void storeDefaults(RobloxMainWindow& mainWindow);
    void loadKeyboardConfig(RobloxMainWindow& mainWindow);
    void saveKeyboardConfig(RobloxMainWindow& mainWindow);
    
    const QMap<QString, QKeySequence>& defaultShortcuts() const
        { return m_defaultShortcuts; }
    
private:
    QMap<QString, QKeySequence>     m_defaultShortcuts;
    QMap<QString, QKeySequence>     m_reservedClientKeys;
};


//
// A widget for changing and configuring the keyboard bindings for all actions
// under the main window.
//
class RobloxKeyboardConfigWidget : public QWidget
{
    Q_OBJECT
public:
    RobloxKeyboardConfigWidget(RobloxMainWindow& mainWidnow, QWidget* parent);
    virtual ~RobloxKeyboardConfigWidget() {}

Q_SIGNALS:
    void dataChanged();

public Q_SLOTS:
    void accept();
    void cancel();
    void restoreAllDefaults();

    void dataChanged(const QModelIndex& topLeft,
                     const QModelIndex& bottomRight);
	void setFilter(QString filter);

private:
    void initialize();

    RobloxMainWindow&          m_mainWindow;
    ShortcutsModel*            m_model;
    RBX::BaldPtr<QTreeView>    m_treeView;
	QLineEdit*                 m_filterEdit;
};
