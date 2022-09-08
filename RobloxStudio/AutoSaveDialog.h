/**
 * AutoSaveDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QDialog>
#include "ui_AutoSave.h"

/**
 * Dialog for asking the user what to do with auto-save recovery files
 *  that were not cleaned up properly.
 */
class AutoSaveDialog : public QDialog
{
    Q_OBJECT

    public:

        AutoSaveDialog(QWidget* Parent);
        virtual ~AutoSaveDialog();

        static bool checkForAutoSaveFiles();

    public Q_SLOTS:

        void onOpen();

        void onIgnore();

        void onDelete();

    protected:

        void closeEvent(QCloseEvent* event);

    private:

        static QStringList getFileList();

        Ui::AutoSave    m_UI;
};
