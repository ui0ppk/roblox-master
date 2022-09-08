/**
 * StudioDeviceEmulator.cpp
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "StudioDeviceEmulator.h"

//Qt Headers
#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QMouseEvent>
#include <QObject>
#include <QToolButton>

//Client Headers
#include "V8DataModel/UserInputService.h"

//Studio Headers
#include "IRobloxDoc.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxRibbonMainWindow.h"
#include "RobloxSettings.h"

const static QString deviceActionName = "EmulationDevice";

StudioDeviceEmulator& StudioDeviceEmulator::Instance()
{
	static StudioDeviceEmulator instance;
	return instance;
}

void StudioDeviceEmulator::storeEmulationList()
{
	if (m_deviceList.isEmpty())
		return;

	RobloxSettings settings;
	settings.beginGroup("emulationDevices");

	settings.remove("");

	settings.setValue("rbxEmulationDevices", m_deviceList.size());
	
	int i = 0;
	for (QList<StudioDeviceEmulator::EmulationDevice>::const_iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter, ++i)
	{
		EmulationDevice eDevice = *iter;

		settings.setValue(QString("d%1name").arg(i), eDevice.name);
		settings.setValue(QString("d%1dpi").arg(i), eDevice.dpi);
		settings.setValue(QString("d%1width").arg(i), eDevice.width);
		settings.setValue(QString("d%1height").arg(i), eDevice.height);
		settings.setValue(QString("d%1mobile").arg(i), eDevice.mobile);
	}

	settings.endGroup();
}

QList<StudioDeviceEmulator::EmulationDevice> StudioDeviceEmulator::retrieveEmulationList()
{
	RobloxSettings settings;

	settings.beginGroup("emulationDevices");

	int numOfDevices = settings.value("rbxEmulationDevices", 0).toInt();

	QList<EmulationDevice> devices;

	for (int i = 0; i < numOfDevices; ++i)
	{
		QString name = settings.value(QString("d%1name").arg(i)).toString();
		int dpi = settings.value(QString("d%1dpi").arg(i)).toInt();
		int width = settings.value(QString("d%1width").arg(i)).toInt();
		int height = settings.value(QString("d%1height").arg(i)).toInt();
		bool mobile = settings.value(QString("d%1mobile").arg(i)).toBool();

		devices.push_back(EmulationDevice(name, dpi, width, height, mobile));
	}

	if (!devices.size())
	{
		devices.push_back(EmulationDevice("IPad Mini", 330, 960, 640, true));
		devices.push_back(EmulationDevice("IPad 2", 330, 960, 640, true));
		devices.push_back(EmulationDevice("IPhone 4S", 330, 960, 640, true));
		devices.push_back(EmulationDevice("Nexus 7", 323, 1280, 800, true));
		devices.push_back(EmulationDevice("Average Laptop", 96, 1366, 768, false));
		devices.push_back(EmulationDevice("HD 720", 96, 1280, 720, false));
		devices.push_back(EmulationDevice("HD 1080", 96, 1920, 1080, false));
		devices.push_back(EmulationDevice("VGA", 96, 640, 480, false));
		devices.push_back(EmulationDevice("Default", 96, 0, 0, false));
	}

	settings.endGroup();
	return devices;
}

QHash<QAction*, StudioDeviceEmulator::EmulationDevice> StudioDeviceEmulator::getDeviceMap()
{
	return m_deviceMap;
}

QList<StudioDeviceEmulator::EmulationDevice> StudioDeviceEmulator::getDeviceList()
{
	return m_deviceList;
}

void StudioDeviceEmulator::setMenuIcon(const EmulationDevice& device)
{
	if (!m_deviceMenu)
		return;

	QString iconLocation = device.mobile ? "EmulateMobile.png" : "EmulateComputer.png";
	m_deviceMenu->setIcon(QIcon(QString(":/RibbonBar/images/RibbonIcons/Test/%1").arg(iconLocation)));
}

void StudioDeviceEmulator::activateCurrentDevice()
{
	IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
	if (!playDoc)
		return;

	RobloxIDEDoc* ide = dynamic_cast<RobloxIDEDoc*>(playDoc);
	if (!ide)
		return;
	
	EmulationDevice device("", 96, m_currentWidth, m_currentHeight, m_isEmulatingTouch);

	setMenuIcon(device);
	ide->forceViewSize(QSize(device.width, device.height));

	if( RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(ide->getDataModel()) )
		userInputService->setStudioEmulatingMobileEnabled(device.mobile);
}

void StudioDeviceEmulator::deviceActivated()
{
	QAction* action = dynamic_cast<QAction*>(QObject::sender());
	if (!action)
		return;

	IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
	if (!playDoc)
		return;

	RobloxIDEDoc* ide = dynamic_cast<RobloxIDEDoc*>(playDoc);
	if (!ide)
		return;

	QHash<QAction*, EmulationDevice>::const_iterator deviceIter = m_deviceMap.find(action);

	if (deviceIter != m_deviceMap.end())
	{
		EmulationDevice device = *deviceIter;

		setMenuIcon(device);
		ide->forceViewSize(QSize(device.width, device.height));

		setIsEmulatingTouch(device.mobile);
		setCurrentDeviceWidth(device.width);
		setCurrentDeviceHeight(device.height);

		if( RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(ide->getDataModel()) )
			userInputService->setStudioEmulatingMobileEnabled(device.mobile);
	}
}

void StudioDeviceEmulator::addEmulationChildren(RobloxRibbonMainWindow* mainWindow, QObject* pParent)
{
	m_deviceMenu = qobject_cast<QMenu*>(pParent);
	this->mainWindow = mainWindow;

	if (!m_deviceMenu || !mainWindow)
		return;

	RobloxSettings settings;

	QList<EmulationDevice> devices = retrieveEmulationList();

	for (QList<EmulationDevice>::const_iterator iter = devices.begin(); iter != devices.end(); ++iter)
		addDevice(*iter, true);
}

QAction* StudioDeviceEmulator::createAction(EmulationDevice device)
{
	QAction* pAction = new QAction(this);

	pAction->setEnabled(true);
	pAction->setVisible(true);

	QString iconLocation = device.mobile ? "mobile_16.png" : "computer_16.png";
	pAction->setIcon(QIcon(QString(":/16x16/images/Studio 2.0 icons/16x16/%1").arg(iconLocation)));		

	pAction->setText(device.name);

	pAction->setCheckable(true);

	return pAction;
}

void StudioDeviceEmulator::addDevice(EmulationDevice device, bool isInitPopulation)
{
	if (!m_deviceMenu || !mainWindow)
		return;

	QAction* addEmulationAction = NULL;

	if (!isInitPopulation)
	{
		addEmulationAction = m_deviceMenu->actions().last();
		m_deviceMenu->removeAction(addEmulationAction);
	}

	QAction* pAction = createAction(device);
	
	mainWindow->addAction(pAction);
	m_deviceMenu->addAction(pAction);
	pAction->setParent(m_deviceMenu);

	connect(pAction, SIGNAL(triggered()), this, SLOT(deviceActivated()));

	m_deviceMap.insert(pAction, device);
	m_deviceList.append(device);

	if (addEmulationAction)
		m_deviceMenu->addAction(addEmulationAction);

	storeEmulationList();
}

void StudioDeviceEmulator::removeDevice(EmulationDevice device)
{
	for (QHash<QAction*, EmulationDevice>::iterator iter = m_deviceMap.begin(); iter != m_deviceMap.end(); ++iter)
	{
		if (*iter == device)
		{
			m_deviceMenu->removeAction(iter.key());
			m_deviceMap.erase(iter);
		}
	}

	int deviceIndex = m_deviceList.indexOf(device);
	if (deviceIndex != -1)
		m_deviceList.takeAt(deviceIndex);
}

void StudioDeviceEmulator::removeDevice(QAction* action)
{
	QHash<QAction*, EmulationDevice>::iterator iter = m_deviceMap.find(action);
	if (iter != m_deviceMap.end())
	{
		int deviceIndex = m_deviceList.indexOf(iter.value());
		if (deviceIndex != -1)
			m_deviceList.takeAt(deviceIndex);
	}
	m_deviceMenu->removeAction(action);
	m_deviceMap.remove(action);
	storeEmulationList();
}

QAction* StudioDeviceEmulator::getActionFromDevice(const EmulationDevice& device)
{
	for (QHash<QAction*, EmulationDevice>::iterator iter = m_deviceMap.begin(); iter != m_deviceMap.end(); ++iter)
		if (*iter == device)
			return iter.key();

	return NULL;
}
