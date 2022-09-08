/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "v8datamodel/InputObject.h"
#include "Util/TextureId.h"
#include "rbx/signal.h"

namespace RBX {

class MouseCommand;
class PartInstance;
class PVInstance;
class Workspace;

extern const char *const sMouse;
class Mouse : public DescribedNonCreatable<Mouse, Instance, sMouse>
{
private:

public:
	Mouse();
	~Mouse();

	rbx::signal<void()> moveSignal;
	rbx::signal<void()> idleSignal;
	rbx::signal<void()> button1DownSignal;
	rbx::signal<void()> button2DownSignal;
	rbx::signal<void()> button1UpSignal;
	rbx::signal<void()> button2UpSignal;
	rbx::signal<void()> wheelForwardSignal;
	rbx::signal<void()> wheelBackwardSignal;
	rbx::signal<void(std::string)> keyDownSignal;
	rbx::signal<void(std::string)> keyUpSignal;

	virtual void update(const shared_ptr<InputObject>& inputObject);

	// Returns the hit location, with the frame rotated so that the mouse shoots down -Z
	virtual CoordinateFrame getHit() const;

	// Returns the mouse origin, with the frame rotated so that the mouse shoots down -Z
	virtual CoordinateFrame getOrigin() const;

	virtual PartInstance* getTarget() const;	

	shared_ptr<Instance> getTargetFilter() const;	
	// TODO: Gotta rewrite RefPropDescriptor to take a shared_ptr
	Instance* getTargetFilterUnsafe() const { return targetFilter.lock().get(); }
	virtual void setTargetFilter(Instance* value);

	virtual NormalId getTargetSurface() const;
	
	virtual RBX::RbxRay getUnitRay() const;

	void setCommand(MouseCommand* value);
	void setWorkspace(Workspace* workspace);

	virtual TextureId getIcon() const;
	virtual void setIcon(const TextureId& value);

	virtual int getX() const;
	virtual int getY() const;
	virtual int getViewSizeX() const;
	virtual int getViewSizeY() const;
    
    
	void cacheInputObject(const shared_ptr<InputObject>& inputObject);

protected:
	Workspace* workspace;
	MouseCommand* command;
	TextureId icon;
	shared_ptr<InputObject> lastEvent;
	weak_ptr<Instance> targetFilter;
	void setTargetFilterUnsafe(Instance* value); // solely called by child classes to fire changed event
	virtual void checkActive() const;
	Workspace* getWorkspace() const;

};


} // namespace