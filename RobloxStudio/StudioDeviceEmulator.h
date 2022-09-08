/**
 * StudioDeviceEmulator.h
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#pragma once
//#include "RobloxRibbonMainWindow.h"
#include <QObject>
#include <QMenu>
#include <QWidgetAction>
#include <QToolButton>

class QDomElement;

class RobloxRibbonMainWindow;

class StudioDeviceEmulator : public QObject
{
	Q_OBJECT
public:
	StudioDeviceEmulator() 
		: m_isEmulatingTouch(false)
		, m_currentWidth(0)
		, m_currentHeight(0)
	{}

	struct EmulationDevice
	{
		QString name;
		unsigned int dpi, width, height;
		bool mobile;

		EmulationDevice(QString name, int dpi, int width, int height, bool mobile)
			: name(name)
			, dpi(dpi)
			, width(width)
			, height(height)
			, mobile(mobile)
		{}

		EmulationDevice()
		{}

		bool operator ==(const EmulationDevice &a) const
		{
			return a.name == name && a.dpi == dpi && a.width == width && a.height == height && a.mobile == mobile;
		}
	};
    
    static StudioDeviceEmulator& Instance();

	int getCurrentDeviceWidth() { return m_currentWidth; }
	int getCurrentDeviceHeight() { return m_currentHeight; }
	bool isEmulatingTouch() { return m_isEmulatingTouch; }

	void setCurrentDeviceWidth(int value) { m_currentWidth = value; }
	void setCurrentDeviceHeight(int value) { m_currentHeight = value; }
	void setIsEmulatingTouch(bool value) { m_isEmulatingTouch = value; }

	void activateCurrentDevice();
	
	void addEmulationChildren(RobloxRibbonMainWindow* mainWindow, QObject* pParent);

	QHash<QAction*, StudioDeviceEmulator::EmulationDevice> getDeviceMap();
	QList<StudioDeviceEmulator::EmulationDevice> getDeviceList();

	void addDevice(EmulationDevice device, bool isInitPopulation = false);
	void removeDevice(EmulationDevice device);
	void removeDevice(QAction* action);

	QAction* getActionFromDevice(const EmulationDevice& device);

private Q_SLOTS:
	void deviceActivated();

private:
	void storeEmulationList();

	QList<StudioDeviceEmulator::EmulationDevice> retrieveEmulationList();

	void setMenuIcon(const EmulationDevice& device);

	QAction* createAction(EmulationDevice device);

	RobloxRibbonMainWindow* mainWindow;
	QMenu*					   m_deviceMenu;

	bool m_isEmulatingTouch;
	int m_currentWidth;
	int m_currentHeight;

	QHash<QAction*, EmulationDevice> m_deviceMap;
	QList<EmulationDevice> m_deviceList;
};
