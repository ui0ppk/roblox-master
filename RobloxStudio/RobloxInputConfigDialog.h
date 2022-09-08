/**
 * RobloxInputConfigDialog.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QAction>
#include <QDialog>
#include <QString>
#include <QWidget>

#include "rbx/BaldPtr.h"

class RobloxMainWindow;
class QEvent;
class QTreeView;
class QDialogButtonBox;
class QAbstractButton;
class RobloxKeyboardConfigWidget;
class RobloxMouseConfigWidget;

//
// A dialog for configuring mouse and keyboard shortcuts for all actions
// under the RobloxMainWindow.
//
class RobloxInputConfigDialog : public QDialog
{
    Q_OBJECT
    
public:

    RobloxInputConfigDialog(RobloxMainWindow& MainWindow);
    virtual ~RobloxInputConfigDialog();
    
private Q_SLOTS:
    void accept();
    void cancel();
    void clicked(QAbstractButton* button);
    
    void dataChanged();
    
private:
    virtual void closeEvent(QCloseEvent* event);
    void restoreAllDefaults();
    void initialize();

    QDialogButtonBox*           m_dialogButtonBox;
    RobloxKeyboardConfigWidget* m_keyboardConfigWidget;
    RobloxMouseConfigWidget*    m_mouseConfigWidget;
    RobloxMainWindow&           m_mainWindow;
    bool                        m_dataChanged;
};

