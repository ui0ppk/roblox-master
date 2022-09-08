/**
 * AddEmulationDeviceDialog.cpp
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "AddEmulationDeviceDialog.h"

// Qt Headers
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>

// Roblox Studio Headers
#include "AuthoringSettings.h"
#include "RobloxApplicationManager.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxMainWindow.h"
#include "StudioDeviceEmulator.h"
#include "UpdateUIManager.h"

AddEmulationDeviceDialog::AddEmulationDeviceDialog(QWidget* Parent)
    : QDialog(Parent,Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
    setAttribute(Qt::WA_DeleteOnClose,false);

    m_UI.setupUi(this);

	setFixedSize(size());
	setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);

	m_UI.dpiValue->setText(QString("%1 DPI").arg(QApplication::desktop()->physicalDpiX()));
    
    connect(m_UI.cancelButton, SIGNAL(clicked()), this, SLOT(onCancel()));
    connect(m_UI.okButton, SIGNAL(clicked()), this, SLOT(submitDevice()));
}

/**
 * Callback for closing the dialog.
 * 
 */
void AddEmulationDeviceDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void AddEmulationDeviceDialog::onCancel()
{
	accept();
}

void AddEmulationDeviceDialog::submitDevice()
{
	StudioDeviceEmulator::EmulationDevice device(m_UI.lineEditName->text(), QApplication::desktop()->physicalDpiX(), m_UI.spinBoxWidth->value(), m_UI.spinBoxHeight->value(), m_UI.checkBoxMobile->checkState());

    StudioDeviceEmulator::Instance().addDevice(device);

	accept();
}
