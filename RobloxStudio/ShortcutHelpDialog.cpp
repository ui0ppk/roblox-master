/**
 * ShortcutHelpDialog.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ShortcutHelpDialog.h"

// Qt Headers
#include <QTreeWidget>
#include <QSettings>

// Roblox Studio Headers
#include "RobloxMainWindow.h"

FASTFLAG(StudioShowTutorialsByDefault)

// shortcuts that should not be visible to the average user
static const QString ShortcutExceptions = 
    "toggleBuildModeAction filePublishedProjectsAction toggleGridAction toggleVideoRecordAction ";

enum eShortcutTableColumns
{
    STC_NAME,
    STC_SHORTCUT,
    STC_DESCRIPTION,

    STC_MAX,
};

ShortcutHelpDialog::ShortcutHelpDialog(RobloxMainWindow& MainWindow)
    : QDialog(&MainWindow,Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      mMainWindow(MainWindow)
{
    setAttribute(Qt::WA_DeleteOnClose,false);
    setWindowTitle("Customize Shorcuts");

    initialize();
    populate();

    // restore window state
    QSettings settings;
    restoreGeometry(settings.value(windowTitle() + "/Geometry").toByteArray());
}

ShortcutHelpDialog::~ShortcutHelpDialog()
{
    // save window state
    QSettings settings;
    settings.setValue(windowTitle() + "/Geometry",saveGeometry());
}

/**
 * Set up controls.
 */
void ShortcutHelpDialog::initialize()
{
    RBXASSERT(!layout());

    RBX::BaldPtr<QLayout> layout = new QVBoxLayout;

    mTree = new QTreeWidget(this);
    layout->addWidget(mTree);
    setLayout(layout);

    mTree->setSelectionMode(QAbstractItemView::NoSelection);
    mTree->setHeaderHidden(false);
    mTree->setContextMenuPolicy(Qt::NoContextMenu);
    mTree->setDragEnabled(false);
    mTree->setColumnCount(STC_MAX);
    mTree->setSortingEnabled(true);

    RBX::BaldPtr<QTreeWidgetItem> header_item = mTree->headerItem();
    header_item->setText(STC_NAME,"Name");
    header_item->setText(STC_SHORTCUT,"Shortcut");
    header_item->setText(STC_DESCRIPTION,"Description");

    RBX::BaldPtr<QHeaderView> header_view = mTree->header();
    header_view->setStretchLastSection(true);
    for ( int i = 0 ; i < STC_MAX - 1 ; ++i )        
        header_view->setResizeMode(i,QHeaderView::ResizeToContents);
}

/**
 * Populates the controls with data.
 */
void ShortcutHelpDialog::populate()
{
    mTree->setUpdatesEnabled(false);
    mTree->clear();

    // hide the toolbars
    QObjectList objects = mMainWindow.children();
    QObjectList::iterator iter = objects.begin();
    for ( ; iter != objects.end() ; ++iter )
    {
        RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
        if ( action && 
             !action->text().isEmpty() &&
             !action->text().startsWith("&") &&
             !action->shortcuts().empty() &&
             !ShortcutExceptions.contains(action->objectName()) &&
			 !(!FFlag::StudioShowTutorialsByDefault && action->objectName() == "viewTutorialsAction"))
        {
            RBX::BaldPtr<QTreeWidgetItem> item = new QTreeWidgetItem(mTree);

            item->setText(STC_NAME,action->text());

            // set icon
            QIcon icon = action->icon();
            if ( icon.isNull() )
            {
                // make a valid icon
                QPixmap pix(16,16);
                pix.fill(QColor(0,0,0,0));  // transparent
                icon = QIcon(pix);   
            }
            item->setIcon(STC_NAME,icon);
            
            // set shortcut key sequence text
            QList<QKeySequence> shortcuts = action->shortcuts();
            QString shortcutText;
            for ( int i = 0 ; i < shortcuts.size() ; ++i )
            {
                if ( i > 0 )
                    shortcutText.append(", ");
                shortcutText += shortcuts[i].toString(QKeySequence::NativeText);
            }
            item->setText(STC_SHORTCUT,shortcutText);

            // set status tip
            item->setText(STC_DESCRIPTION,action->statusTip());
        }
    }

    mTree->sortItems(STC_NAME,Qt::AscendingOrder);
    mTree->setUpdatesEnabled(true);
}

