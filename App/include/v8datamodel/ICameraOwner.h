 /* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX {

// assumed to be descended from Instance when used...
// RootInstance is descended from this
class Camera;
class ModelInstance;

class RBXBaseClass ICameraOwner
{
public:
	ICameraOwner() {}
	virtual ~ICameraOwner() {}

	virtual Camera* getCamera() = 0;	
	virtual const Camera* getConstCamera() const = 0;	

	virtual const ModelInstance* getCameraOwnerModel() const = 0;
};

}  // namespace RBX
