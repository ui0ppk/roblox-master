/**
 * Roblox.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QObject>
#include <QMetaType>

// Roblox Headers
#include "v8datamodel/TeleportCallback.h"
#include "v8datamodel/FastLogSettings.h"

namespace RBX
{
	class Game;
	class FunctionMarshaller;
	struct StandardOutMessage;
}

class QTimer;

Q_DECLARE_METATYPE(RBX::MessageType);

class Roblox : public QObject, RBX::TeleportCallback
{
Q_OBJECT
	
Q_SIGNALS:

    /**
     * Emitted when a log entry is made from the engine that should be displayed to the user.
     *  
     * @param   message     note that this is a copy, not a reference to handle multi-threading
     * @param   type        type of log message
     */
    void newOutputMessage(const QString message,RBX::MessageType type);

	void marshallAppEvent(void* event,bool wait);
	
public:
	static Roblox& Instance();
	
	static void globalInit(const QString& urlArg, const QString& ticketArg, RBX::HttpFuture& settingsFuture);				
	static void globalShutdown();
	
	static void sendAppEvent(void *pClosure);
	static void postAppEvent(void *pClosure);
    virtual bool isTeleportEnabled() const;

	void doTeleport(const std::string& url, const std::string& ticket,
		const std::string& script);

	void Teleport(const std::string& url, const std::string& ticket,
		const std::string& script);
	
	void startTimer();
	void stopTimer();

private Q_SLOTS:
	void onTimeOut();
	
private:
	Roblox();                           // Private constructor
	~Roblox();							
	Roblox(const Roblox&);                 // Prevent copy-construction
	Roblox& operator=(const Roblox&);      // Prevent assignment
	
	static void onMessageOut(const RBX::StandardOutMessage& message);

	RBX::FunctionMarshaller*    m_pMarshaller;	
	QTimer*		                m_pTimer;

    static bool                 sInitialized;
};	
