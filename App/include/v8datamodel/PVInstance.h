/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/Extents.h"
#include "Util/IHasLocation.h"

namespace G3D {	
	class RenderDevice;
}

namespace RBX {

class Primitive;
class Adorn;
class PartInstance;

extern const char *const sPVInstance;
class RBXBaseClass PVInstance
	: public Reflection::Described<PVInstance, sPVInstance, Instance >
	, public virtual IHasLocation
{
private:
	typedef Reflection::Described<PVInstance, sPVInstance, Instance > Super;

protected:

	PVInstance(const char* name);

public:
	virtual ~PVInstance();

	/*override*/ int getPersistentDataCost() const 
	{
		return Super::getPersistentDataCost() + 6;
	}
	
protected:
	/////////////////////////////////////////////////////////////
	// INSTANCE
	//
	/*override*/ virtual void readProperty(const XmlElement* propertyElement, IReferenceBinder& binder);

	////////////////////////////////////////////////////////////////////////
	// PVINSTANCE
	//
	void renderCoordinateFrame(Adorn* adorn);
	/*implement*/ virtual bool hitTestImpl(const RBX::RbxRay& worldRay, Vector3& worldHitPoint) = 0;
public:
	/*implement*/ virtual Extents computeExtentsWorld() const = 0;
	/*implement*/ virtual PartInstance* getPrimaryPart() = 0;	

	////////////////////////////////////////////////////////////////////////
	// SCRIPT FUNCTIONS - .cpp file uses workspace.h
	//
	// These are functions used by Lua.  They can be renamed as necessary.  These
	// functions apply to both Parts and Models, which can be handy.
	//
	// moveToPoint - attemps to move the PVInstance to the point given in world coordinates.
	// Parts and Models have a "SnapLocation" - given by the center of a part, or the center
	// of a model's PrimaryPart.

	void moveToPointNoUnjoinNoJoin(Vector3 point);

	void moveToPointAndJoin(Vector3 point);

	void moveToPointNoJoin(Vector3 point);	

	bool hitTest(const RBX::RbxRay& worldRay, Vector3& worldHitPoint) {
		return hitTestImpl(worldRay, worldHitPoint);
	}

	////////////////////////////////////////////////////////////////////////
	// Utilities - mirrors of Instance functions cast to PVInstance
	//

	PVInstance* getTopLevelPVParent();					// is right below the root

	const PVInstance* getTopLevelPVParent() const;		// is right below the root

	bool isTopLevelPVInstance() const	{
		return (	(queryTypedParent<PVInstance>() == NULL)
				||	(getTypedRoot<PVInstance>() == getTypedParent<PVInstance>()));
	}

	////////////////////////////////////////////////////////////////////////
	// LEGACY
	// Legacy CanSelect - changed to Locked 1/1/06
	// void setCanSelectLegacy(bool set) {setLocked(!set);}

	// Legacy Offset - pre 11/20/2005 - now all parts store in global
	void setPVGridOffsetLegacy(const CoordinateFrame& _offset);
};

} // namespace
