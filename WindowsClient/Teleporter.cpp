#include "stdafx.h"
#include "Teleporter.h"

#include "Application.h"
#include "FunctionMarshaller.h"
#include "util/Statistics.h"
#include "v8datamodel/TeleportService.h"

namespace RBX {

static boost::thread releaseGameThread;

void Teleporter::initialize(Application* app,
							FunctionMarshaller* marshaller)
{
	this->app = app;
	this->marshaller = marshaller;

	TeleportService::SetCallback(this);
	TeleportService::SetBaseUrl(GetBaseURL().c_str());
}

void Teleporter::doTeleport(const std::string& url, const std::string& ticket,
							const std::string& script)
{
	marshaller->Submit(boost::bind(&Application::Teleport, app, url, ticket, script));
}

}  // namespace RBX
