/**
 * ExternalHandlers.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ExternalHandlers.h"

// Qt Headers
#include <QUrl>

// Roblox Studio Headers
#include "FastLog.h"
#include "RobloxSettings.h"
#include "StudioUtilities.h"

TeleportHandler::TeleportHandler(const QString &handlerId, const QString &url, const QString& ticket, const QString &teleportScript)
: m_handlerId(handlerId)
, m_url(url)
, m_ticket(ticket)
, m_teleportScript(teleportScript)
{
}

bool TeleportHandler::handle()
{
	return true;
}

