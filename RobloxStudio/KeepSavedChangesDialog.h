/**
 * KeepSavedChangesDialog.h
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */
#pragma once

#include <QMessageBox>

class QAbstractButton;

class KeepSavedChangesDialog : public QMessageBox
{
    Q_OBJECT

public:
    KeepSavedChangesDialog(QWidget* Parent);
    virtual ~KeepSavedChangesDialog();

    enum DialogResult
    {
        Save,
        Discard,
        Fail,
        Cancel
    };

    enum DialogResult getResult() { return dialogResult; }

public Q_SLOTS:
    void onClicked(QAbstractButton* button);
    void onSave();
    void onDiscard();
    void onCancel();
    
private:
    enum DialogResult dialogResult;
};
