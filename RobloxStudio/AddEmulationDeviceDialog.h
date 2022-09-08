/**
 * AddEmulationDeviceDialog.h
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QDialog>
#include "ui_AddEmulationDevDialog.h"

/**
 * Dialog for asking the user what to do with auto-save recovery files
 *  that were not cleaned up properly.
 */
class AddEmulationDeviceDialog : public QDialog
{
    Q_OBJECT

    public:

        AddEmulationDeviceDialog(QWidget* Parent);
    
    public Q_SLOTS:
		void submitDevice();
        void onCancel();
    protected:

        void closeEvent(QCloseEvent* event);

    private:

        static QStringList getFileList();

        Ui::AddEmulationDevDialog    m_UI;
};
