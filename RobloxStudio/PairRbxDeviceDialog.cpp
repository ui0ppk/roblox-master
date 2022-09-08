/**
 * PairRbxDeviceDialog.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "PairRbxDeviceDialog.h"

// Qt Headers
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

// Roblox Studio Headers
#include "AuthoringSettings.h"
#include "RobloxDocManager.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "RobloxIDEDoc.h"
#include "RobloxApplicationManager.h"
#include "MobileDevelopmentDeployer.h"

PairRbxDeviceDialog::PairRbxDeviceDialog(QWidget* Parent)
    : QDialog(Parent,Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
    setAttribute(Qt::WA_DeleteOnClose,false);

    m_UI.setupUi(this);
    
    updatePairCode();

    connect(m_UI.cancelButton,SIGNAL(clicked()),this,SLOT(onCancel()));
    
	MobileDevelopmentDeployer::singleton().pairWithRbxDevDevices();
    
    connect(&MobileDevelopmentDeployer::singleton(), SIGNAL(connectedToClient(const QString&)), this, SLOT(clientPaired(const QString&)));
}

PairRbxDeviceDialog::~PairRbxDeviceDialog()
{
    MobileDevelopmentDeployer::singleton().disconnectClientsAndShutDown();
}

void PairRbxDeviceDialog::updatePairCode()
{
    m_UI.codeLabel->setText(MobileDevelopmentDeployer::singleton().getPairCodeString());
}

/**
 * Callback for closing the dialog.
 * 
 */
void PairRbxDeviceDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void PairRbxDeviceDialog::onCancel()
{
    MobileDevelopmentDeployer::singleton().disconnectClientsAndShutDown();
	accept();
}

void PairRbxDeviceDialog::clientPaired(const QString& clientName)
{
    QString statusString("Successfully paired with ");
    statusString.append(clientName).append("!");
    
    m_UI.statusLabel->setText("You can now connect to servers started here from your mobile device.");
    m_UI.codeLabel->setText("");
    m_UI.label->setText(statusString);
    m_UI.cancelButton->setText("Done");
    
    MobileDevelopmentDeployer::singleton().disconnectClientsAndShutDown();
}