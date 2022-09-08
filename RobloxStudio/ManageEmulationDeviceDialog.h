/**
 * ManageEmulationDeviceDialog.h
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QDialog>
#include "ui_ManageEmulationDevDialog.h"

/**
 * Dialog for asking the user what to do with auto-save recovery files
 *  that were not cleaned up properly.
 */
class ManageEmulationDeviceDialog : public QDialog
{
    Q_OBJECT

    public:

        ManageEmulationDeviceDialog(QWidget* Parent);
    
    public Q_SLOTS:
        void onCancel();
		void onPlusButtonPressed();
		void onMinusButtonPressed();

    protected:

        void closeEvent(QCloseEvent* event);

    private:
		
		void rePopulateListWidget();

        static QStringList getFileList();

        Ui::ManageEmulationDevDialog    m_UI;
};
