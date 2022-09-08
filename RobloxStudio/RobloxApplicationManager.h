/**
 * RobloxApplicationManager.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QSharedMemory>
#include <QProcess>
#include <QObject>
#include <QPointer>

class QString;
class QStringList;
class ChildProcessHandler;
class QLocalServer;

enum NewInstanceMode
{
	NewInstanceMode_Detached = 0,
	NewInstanceMode_Server,
	NewInstanceMode_Player
};

class RobloxApplicationManager
{
public:

	static RobloxApplicationManager& instance();
	
	// launches a new process
	bool createNewStudioInstance(
        const QString&  script = QString::null, 
        const QString&  fileLocation = QString::null,
        bool            isTestMode = false,
        bool            isAvatarMode = false,
		NewInstanceMode mode = NewInstanceMode_Detached);
	
	// Increment/decrement counters for application types
	void registerApplication();
	void unregisterApplication();

	int getApplicationCount();

	// manages child process
	bool hasChildProcesses();
	void cleanupChildProcesses(int timeout = 3000);

	void startLocalServer();
	bool hasLocalServer();
	
private:
	struct SharedData
	{
		int numStudioApplications;
		int numPlayerApplications;
	};
	RobloxApplicationManager();

	void composeArgList(
        QStringList&    argsContainer, 
        const QString&  script, 
        const QString&  fileLocation,
        bool            isTestMode,
        bool            isAvatarMode );

	// Updates shared memory
	void modifyApplicationCount(int delta);
	
	QSharedMemory sharedMemory;
	ChildProcessHandler* childProcessHandler;
};

class ChildProcessHandler : public QObject
{
	Q_OBJECT
public:
	ChildProcessHandler(QObject* parent);

	bool startChildProcess(const QString& robloxApp, const QStringList &args, NewInstanceMode mode);
	void cleanupChildProcesses(int timeout = 3000);
	bool hasChildProcesses();

	void startLocalServer();
	bool hasLocalServer();

private Q_SLOTS:
	void onProcessFinished();
	void onNewConnection();

private:
	QLocalServer*       m_pLocalServer;

	typedef QList<QPointer<QProcess> > ProcessCollection;
	ProcessCollection   serverProcesses;
	ProcessCollection   playerProcesses;
};