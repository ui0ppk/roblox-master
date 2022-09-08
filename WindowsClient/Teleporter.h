#pragma once

#include "v8datamodel/TeleportCallback.h"

namespace RBX {

class Application;
class FunctionMarshaller;

// The Teleporter is responsible for teleporting the player across places.
class Teleporter : public TeleportCallback
{
	Application* app;
	FunctionMarshaller* marshaller;

public:
	void initialize(Application* app, FunctionMarshaller* marshaller);

	// Submits a callback to the function marshaller to teleport the player.
	virtual void doTeleport(const std::string& url, const std::string& ticket,
		const std::string& script);

    virtual bool isTeleportEnabled() const { return true; }
};

}  // namespace RBX
