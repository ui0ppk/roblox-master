/**
 * ShortcutHelpDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QDialog>

// Roblox Headers
#include "rbx/BaldPtr.h"

class QTreeWidget;
class RobloxMainWindow;

/**
 * Dialog that displays all the shortcut key sequences for actions in the main window.
 *  Also displays the actions' icon and statustip text.
 *  The dialog does not delete on close.
 */
class ShortcutHelpDialog : public QDialog
{
    public:

        ShortcutHelpDialog(RobloxMainWindow& MainWindow);
        virtual ~ShortcutHelpDialog();

    private:

        void initialize();
        void populate();

        RBX::BaldPtr<QTreeWidget>   mTree;
        RobloxMainWindow&           mMainWindow;
};