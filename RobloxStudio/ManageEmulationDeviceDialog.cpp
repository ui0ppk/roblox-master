/**
 * AddEmulationDeviceDialog.cpp
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ManageEmulationDeviceDialog.h"

// Qt Headers
#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

// Roblox Studio Headers
#include "AddEmulationDeviceDialog.h"
#include "AuthoringSettings.h"
#include "RobloxApplicationManager.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxMainWindow.h"
#include "StudioDeviceEmulator.h"
#include "UpdateUIManager.h"

ManageEmulationDeviceDialog::ManageEmulationDeviceDialog(QWidget* Parent)
    : QDialog(Parent,Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
    setAttribute(Qt::WA_DeleteOnClose,false);

    m_UI.setupUi(this);

	setFixedSize(size());
	setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);

	rePopulateListWidget();

	connect(m_UI.plusButton, SIGNAL(clicked()), this, SLOT(onPlusButtonPressed()));
	connect(m_UI.minusButton, SIGNAL(clicked()), this, SLOT(onMinusButtonPressed()));
	connect(m_UI.okButton, SIGNAL(clicked()), this, SLOT(onCancel()));
     
}

void ManageEmulationDeviceDialog::rePopulateListWidget()
{
	m_UI.deviceListWidget->clear();


	QList<StudioDeviceEmulator::EmulationDevice> devices = StudioDeviceEmulator::Instance().getDeviceList();

	for (QList<StudioDeviceEmulator::EmulationDevice>::const_iterator iter = devices.begin(); iter != devices.end(); ++iter)
	{
		if (*iter == StudioDeviceEmulator::EmulationDevice("Default", 96, 0, 0, false))
			continue;

		QListWidgetItem* deviceItem = new QListWidgetItem(m_UI.deviceListWidget);
		deviceItem->setText((*iter).name);

		QString iconLocation = (*iter).mobile ? "EmulateMobile.png" : "EmulateComputer.png";
		deviceItem->setIcon(QIcon(QString(":/RibbonBar/images/RibbonIcons/Test/%1").arg(iconLocation)));

		QAction* action = StudioDeviceEmulator::Instance().getActionFromDevice(*iter);

		deviceItem->setData(Qt::UserRole, (int)action);
		
		m_UI.deviceListWidget->addItem(deviceItem);
	}
}

void ManageEmulationDeviceDialog::onPlusButtonPressed()
{
	AddEmulationDeviceDialog dialog(NULL);
	dialog.exec();
	rePopulateListWidget();
}

void ManageEmulationDeviceDialog::onMinusButtonPressed()
{
	QList<QListWidgetItem*> selectedItems = m_UI.deviceListWidget->selectedItems();

	for (QList<QListWidgetItem*>::const_iterator iter = selectedItems.begin(); iter != selectedItems.end(); ++iter)
	{
		QAction* action = (QAction*)((*iter)->data(Qt::UserRole).toInt());
		StudioDeviceEmulator::Instance().removeDevice(action);
	}
	rePopulateListWidget();
}

void ManageEmulationDeviceDialog::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}

void ManageEmulationDeviceDialog::onCancel()
{
	accept();
}

