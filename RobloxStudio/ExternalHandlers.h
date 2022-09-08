/**
 * ExternalHandlers.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QString>

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

// Roblox Studio Headers
#include "IExternalHandler.h"

namespace RBX {
	class DataModel;
}

class TeleportHandler : public IExternalHandler
{
public:
	TeleportHandler(const QString &handlerId, const QString &url, const QString& ticket, const QString &teleportScript);

	QString className() { return "TeleportHandler"; }
	QString handlerId() { return m_handlerId; }
	void setDataModel(boost::shared_ptr<RBX::DataModel> pDataModel) { m_pDataModel = pDataModel; }
	bool handle();

private:
	boost::shared_ptr<RBX::DataModel>		m_pDataModel;
	QString									m_handlerId;
	QString									m_url;
	QString									m_ticket;
	QString									m_teleportScript;
};
