/**
 * PairRbxDevDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QDialog>
#include "ui_PairRbxDevDialog.h"

/**
 * Dialog for asking the user what to do with auto-save recovery files
 *  that were not cleaned up properly.
 */
class PairRbxDeviceDialog : public QDialog
{
    Q_OBJECT

    public:

        PairRbxDeviceDialog(QWidget* Parent);
        virtual ~PairRbxDeviceDialog();
    
        void updatePairCode();

    public Q_SLOTS:

        void onCancel();
        void clientPaired(const QString& clientName);
    protected:

        void closeEvent(QCloseEvent* event);

    private:

        static QStringList getFileList();

        Ui::PairRbxDevDialog    m_UI;
};
