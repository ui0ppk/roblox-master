/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"


#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Attachment.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "Tool/DragUtilities.h"
#include "Script/Script.h"

DYNAMIC_FASTFLAGVARIABLE(AccessoriesAndAttachments, false)

namespace RBX {

using namespace RBX::Network;

const char* const sAccoutrement = "Accoutrement";
const char* const  sHat = "Hat";
const char* const  sAccessory = "Accessory";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<Accoutrement, CoordinateFrame> prop_AttachmentPoint("AttachmentPoint", category_Appearance, &Accoutrement::getAttachmentPoint, &Accoutrement::setAttachmentPoint, Reflection::PropertyDescriptor::STANDARD);
static const Reflection::PropDescriptor<Accoutrement, Vector3> prop_AttachmentPos("AttachmentPos", category_Appearance, &Accoutrement::getAttachmentPos, &Accoutrement::setAttachmentPos, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Accoutrement, Vector3> prop_AttachmentForward("AttachmentForward", category_Appearance, &Accoutrement::getAttachmentForward, &Accoutrement::setAttachmentForward, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Accoutrement, Vector3> prop_AttachmentUp("AttachmentUp", category_Appearance, &Accoutrement::getAttachmentUp, &Accoutrement::setAttachmentUp, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Accoutrement, Vector3> prop_AttachmentRight("AttachmentRight", category_Appearance, &Accoutrement::getAttachmentRight, &Accoutrement::setAttachmentRight, Reflection::PropertyDescriptor::UI);

static const Reflection::PropDescriptor<Accoutrement, int> prop_BackendAccoutrementState("BackendAccoutrementState", category_Appearance, &Accoutrement::getBackendAccoutrementState, &Accoutrement::setBackendAccoutrementState, Reflection::PropertyDescriptor::REPLICATE_ONLY);

REFLECTION_END();

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

Accoutrement::Accoutrement() 
: DescribedCreatable<Accoutrement, Instance, sAccoutrement>()
, backendAccoutrementState(NOTHING)
, handleTouched(FLog::TouchedSignal)
{
	setName(sAccoutrement);
}


Accoutrement::~Accoutrement()
{
	RBXASSERT(!weld);
	RBXASSERT(!handleTouched.connected());
	RBXASSERT(!characterChildAdded.connected());
	RBXASSERT(!characterChildRemoved.connected());
}

void Accoutrement::onCameraNear(float distance) 
{
	for (size_t i = 0; i < numChildren(); i++) {
		if (CameraSubject* cameraSubject = queryTypedChild<CameraSubject>(i)) {
			cameraSubject->onCameraNear(distance);
		}
	}
}

void Accoutrement::render3dSelect(Adorn* adorn, SelectState selectState)
{
	for (size_t i = 0; i < this->numChildren(); ++i)
	{
		if (IAdornable* iRenderable = dynamic_cast<IAdornable*>(this->getChild(i))) 
		{
			iRenderable->render3dSelect(adorn, selectState);
		}
	}
}

void Accoutrement::dropAll(ModelInstance* character)
{
	Accoutrement::dropAllOthers(character, NULL);
}

void Accoutrement::dropAllOthers(ModelInstance *character, Accoutrement *exception)
{
	if (Workspace* workspace = Workspace::findWorkspace(character))
	{
		while (Accoutrement* acc = character->findFirstChildOfType<Accoutrement>())
		{
			if (acc != exception) acc->setParent(workspace);			// ok - should drop!  check it out
		}
	}
}

PartInstance* Accoutrement::getHandle()
{
	return const_cast<PartInstance*>(getHandleConst());
}

const PartInstance* Accoutrement::getHandleConst() const
{
	return Instance::fastDynamicCast<PartInstance>(findConstFirstChildByName("Handle"));
}



Attachment* Accoutrement::findFirstMatchingAttachment(Instance* model, const std::string& originalAttachmentName)
{
	if (model != NULL)
	{
		if (shared_ptr<const Instances> children = model->getChildren2())
		{
			for (Instances::const_iterator childIterator = children->begin(); childIterator != children->end(); ++childIterator)
			{
				Instance* child = (*childIterator).get();
				Attachment* attachment = Instance::fastDynamicCast<Attachment>(child);
				//check to see if child is an attachment and if the attachment fits with our original attachment
				if ((attachment != NULL) && (attachment->getName() == originalAttachmentName))
				{
					return attachment;
				}

				//Continue recursive search but ignore accourtements/accessories
				if (Instance::fastDynamicCast<Accoutrement>(child) == NULL)
				{
					Attachment* foundAttachment = Accoutrement::findFirstMatchingAttachment(child, originalAttachmentName);
					//If the deeper search found one then keep handign it up, otherwise continue searching horizontally
					if (foundAttachment != NULL)
					{
						return foundAttachment;
					}
				}
			}
		}
	}
	return NULL;
}


const CoordinateFrame Accoutrement::getLocation() 
{
	const PartInstance* handle = getHandleConst();

	return handle ? handle->getCoordinateFrame() : CoordinateFrame();
}


//////////////////////////////////////////////////////////////////////////////
//
// REPLICATION - Front and Back end



/*
						watch_touch	

	IN_CHARACTER			x
	IN_WORKSPACE			x	-> needs touch or parent already set to proceed
	HAS_HANDLE				x	
	NOTHING
*/

void Accoutrement::setBackendAccoutrementStateNoReplicate(int value)
{
	backendAccoutrementState = static_cast<AccoutrementState>(value);
}


void Accoutrement::setBackendAccoutrementState(int value)
{
	if (value != backendAccoutrementState)
	{
		setBackendAccoutrementStateNoReplicate(value);

		raisePropertyChanged(prop_BackendAccoutrementState);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// BACKEND

void Accoutrement::connectTouchEvent()
{
	PartInstance* handle = getHandle();

	if (handle)
	{
		handleTouched = handle->onDemandWrite()-> touchedSignal.connect(boost::bind(&Accoutrement::onEvent_HandleTouched, this, _1));
		FASTLOG3(FLog::TouchedSignal, "Connecting Accoutrement to touched signal, instance: %p, part: %p, part signal: %p", 
			this, handle, &(handle->onDemandRead()->touchedSignal));
	}
	else
	{
	//	RBXASSERT(0);					// only call if >= the HAS_HANDLE state
		handleTouched.disconnect();		// just to be safe
	}
}

void Accoutrement::connectAttachmentAdjustedEvent()
{
	attachmentAdjusted.disconnect();
	if (PartInstance* handle = getHandle())
	{
		if (Attachment* attachment = handle->findFirstChildOfType<Attachment>())
		{
			attachmentAdjusted = attachment->propertyChangedSignal.connect(boost::bind(&Accoutrement::onEvent_AttachmentAdjusted, this, _1));
		}
	}
}

void Accoutrement::rebuildBackendState()
{
	AccoutrementState desiredState = computeDesiredState();

	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(this);
	RBXASSERT(serviceProvider);
	
	setDesiredState(desiredState, serviceProvider);
}

/*
	Backend:
						watch_touch	
	IN_CHARACTER			x
	IN_WORKSPACE			x	-> needs touch or parent already set to proceed
	HAS_HANDLE				x	
	NOTHING
*/

Accoutrement::AccoutrementState Accoutrement::computeDesiredState()
{
	RBXASSERT(Network::Players::backendProcessing(this));

	PartInstance* handle = getHandle();
	if (!handle) {
		return NOTHING;
	}

	if (!Workspace::contextInWorkspace(this)) {
		return HAS_HANDLE;
	}

	return computeDesiredState(getParent());
}

Accoutrement::AccoutrementState Accoutrement::computeDesiredState(Instance *testParent)
{

	Humanoid* humanoid = Humanoid::modelIsCharacter(testParent);

	// Parent is not a character model, we are in the workspace
	if (!humanoid) {
		return IN_WORKSPACE;
	}
	// make sure we are not already wearing a hat
	/*if (testParent->findFirstChildOfType<Accoutrement>() != NULL)
	{
		return IN_WORKSPACE;
	}*/
	
	if (humanoid->getTorsoSlow() ) {
		return EQUIPPED;
	}

	return NOTHING;
}


void Accoutrement::setDesiredState(AccoutrementState desiredState, const ServiceProvider* serviceProvider)
{
	RBXASSERT(Network::Players::backendProcessing(serviceProvider));

	if (desiredState > backendAccoutrementState)
	{
		switch (backendAccoutrementState)
		{
		case IN_CHARACTER:			upTo_Equipped();	break;
		case IN_WORKSPACE:			upTo_InCharacter(); break;
		case HAS_HANDLE:			upTo_InWorkspace();	break;
		case NOTHING:				upTo_HasHandle();	break;
		default:				RBXASSERT(0); 
		}
		setBackendAccoutrementState(static_cast<AccoutrementState>(backendAccoutrementState + 1));
	}
	else if (desiredState < backendAccoutrementState)
	{
		switch (backendAccoutrementState)
		{
		case EQUIPPED:				downFrom_Equipped();	break;
		case IN_CHARACTER:			downFrom_InCharacter(); break;
		case IN_WORKSPACE:			downFrom_InWorkspace();	break;
		case HAS_HANDLE:			downFrom_HasHandle();	break;
		default:				RBXASSERT(0);
		}
		setBackendAccoutrementState(static_cast<AccoutrementState>(backendAccoutrementState - 1));
	}
	// do again if still not there - recursive
	if (desiredState != backendAccoutrementState)
	{
		setDesiredState(desiredState, serviceProvider);
	}
}

void Accoutrement::upTo_HasHandle()
{
	connectTouchEvent();
}

void Accoutrement::downFrom_HasHandle()
{
	handleTouched.disconnect();
	if (DFFlag::AccessoriesAndAttachments)
	{
		attachmentAdjusted.disconnect();
	}
}

void Accoutrement::upTo_InWorkspace()
{
	RBXASSERT(ServiceProvider::findServiceProvider(this));
	RBXASSERT(Network::Players::backendProcessing(this));
}

void Accoutrement::downFrom_InWorkspace()
{
}

void Accoutrement::upTo_InCharacter()
{
	RBXASSERT(Humanoid::modelIsCharacter(getParent()));

	characterChildAdded = getParent()->onDemandWrite()->childAddedSignal.connect(boost::bind(&Accoutrement::onEvent_AddedBackend, this, _1));
	characterChildRemoved = getParent()->onDemandWrite()->childRemovedSignal.connect(boost::bind(&Accoutrement::onEvent_RemovedBackend, this, _1));
}

void Accoutrement::downFrom_InCharacter()
{
	characterChildAdded.disconnect();
	characterChildRemoved.disconnect();
}

void Accoutrement::UnequipThis(shared_ptr<Instance> instance)
{
	// if this is an accoutrement that is equipped, unequip
	Accoutrement *a = Instance::fastDynamicCast<Accoutrement>(instance.get());
	if (!a) return;
	if (a->backendAccoutrementState == EQUIPPED)
	{
		a->setDesiredState(IN_WORKSPACE, ServiceProvider::findServiceProvider(a));
	}
}

// TODO: This code is the nearly the same as Tool::upTo_Equipped.  That's evil!
void Accoutrement::upTo_Equipped()
{
	// Unequip other hats
	/*
	Instance *i = dynamic_cast<Instance *>(getParent());
	RBXASSERT(i);
	if (i->getChildren())
		std::for_each(
			i->getChildren()->begin(),
			i->getChildren()->end(),
			boost::bind(&Accoutrement::UnequipThis, _1)
			);
	*/

	// Disconnect listening for touches
	handleTouched.disconnect();

	// Unjoin tool from other parts if any remaining joints
	PartArray parts;
	PartInstance::findParts(this, parts);
	DragUtilities::unJoinFromOutsiders(parts);

	Humanoid* humanoid = Humanoid::modelIsCharacter(getParent());
	if (!humanoid)
		return;

	if (DFFlag::AccessoriesAndAttachments)
	{
		PartInstance* handle = getHandle();
		if (!handle)
			return;

		//Code for new attachment based system
		Attachment* attachment = handle->findFirstChildOfType<Attachment>();
		Attachment* matchingAttachment =  NULL;
		PartInstance* matchingAttachmentPart = NULL;
		if (attachment)
		{
			if ((matchingAttachment = Accoutrement::findFirstMatchingAttachment(getParent(), attachment->getName())))
			{
				matchingAttachmentPart = Instance::fastDynamicCast<PartInstance>(matchingAttachment->getParent());
			}
		}

		//This infers that both attachments were found
		if (matchingAttachmentPart)
		{
			handle->setCanCollide(false);
			buildWeld(handle, matchingAttachmentPart, attachment->getFrameInPart(), matchingAttachment->getFrameInPart(), "AccessoryWeld");
		}
		else
		{
			//This is legacy functionality
			if (PartInstance* head = humanoid->getHeadSlow())
			{
				handle->setCanCollide(false);
				buildWeld(head, handle, humanoid->getTopOfHead(), attachmentPoint, "HeadWeld");
			}
		}

		// Listen to attachment changed to update weld
		connectAttachmentAdjustedEvent();
	}
	else
	{
		PartInstance* head = humanoid->getHeadSlow();
		if (!head)
			return;

		PartInstance* handle = getHandle();
		if (handle) 
		{
			handle->setCanCollide(false);
			buildWeld(head, handle, humanoid->getTopOfHead(), attachmentPoint, "HeadWeld");
		}
	}
}


void Accoutrement::downFrom_Equipped()
{
	ModelInstance* oldCharacter = NULL;

	if (weld)
	{
		if (DFFlag::AccessoriesAndAttachments)
		{
			PartInstance* handle = getHandle();
			PartInstance* oldAttachmentPart = NULL;
			if (handle && (weld->getParent() == handle))
			{
				//If the weld is under the handle, then it is using the new attachment system
				oldAttachmentPart = weld->getPart1();
			}
			else
			{
				//If the weld is not undle the handle then it is using the legacy system
				oldAttachmentPart = weld->getPart0();
			}

			if (oldAttachmentPart)
			{
				oldCharacter = Instance::fastDynamicCast<ModelInstance>(oldAttachmentPart->getParent());
			}
		}
		else
		{
			if (PartInstance* oldArm = weld->getPart0())
			{
				oldCharacter = Instance::fastDynamicCast<ModelInstance>(oldArm->getParent());
			}
		}
		weld->setParent(NULL);
		weld.reset();
	}

	if (oldCharacter && Humanoid::modelIsCharacter(oldCharacter))
	{
		CoordinateFrame cf = oldCharacter->getLocation();
		Vector3 pos = cf.translation;
		pos.y += 4;
		pos += cf.lookVector() * 8; 
			
		if (PartInstance* handle = getHandle())
		{
			handle->setCanCollide(true);
			handle->moveToPointNoJoin(pos);
			handle->setRotationalVelocity(Vector3(1.0f, 1.0f, 1.0f));
		}
	}

	// UGLY hack for when the camera is zooming in on the character, and he drops his hat, so the hat is not transparent.
	this->onCameraNear(999.0f);

	// Dropping - must start listening again
	connectTouchEvent();

	if (DFFlag::AccessoriesAndAttachments)
	{
		// Dropping - Doesn't need to be dynamically adjust weld
		attachmentAdjusted.disconnect();
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Backend stuff - events


// Used for both the tool and the torso
//
// Character: RightArm, Tool
// Torso: Shoulder?
//
void Accoutrement::onEvent_AddedBackend(shared_ptr<Instance> child)
{
	if (child.get() != this)	// tool parent changed is handled in ancestor stuff, because this might not be hooked up
	{
		RBXASSERT(ServiceProvider::findServiceProvider(this));
		RBXASSERT(Network::Players::backendProcessing(this));
		rebuildBackendState();
	}
}

void Accoutrement::onEvent_RemovedBackend(shared_ptr<Instance> child)
{
	if (child.get() != this)	// tool parent changed is handled in ancestor stuff, because this might not be hooked up
	{
		RBXASSERT(ServiceProvider::findServiceProvider(this));
		RBXASSERT(Network::Players::backendProcessing(this));

		if (child.get() == weld.get())
		{
			RBXASSERT(backendAccoutrementState >= EQUIPPED);
			setDesiredState(NOTHING, ServiceProvider::findServiceProvider(this));	// force down, nuke the weld
		}
		rebuildBackendState();
	}
}



void Accoutrement::onChildAdded(Instance* child)
{
	if (ServiceProvider::findServiceProvider(this) && Network::Players::backendProcessing(this))
	{
		rebuildBackendState();
	}
}

void Accoutrement::onChildRemoved(Instance* child)
{
	if (ServiceProvider::findServiceProvider(this) && Network::Players::backendProcessing(this))
	{
		rebuildBackendState();
	}
}


// This is a backend event
void Accoutrement::onEvent_HandleTouched(shared_ptr<Instance> other)
{
	RBXASSERT(Network::Players::backendProcessing(this));

	if ((backendAccoutrementState == IN_WORKSPACE) && other)
	{
		Instance* touchingCharacter = other->getParent();
		if (computeDesiredState(touchingCharacter) == EQUIPPED)
		{
			if (touchingCharacter->findFirstChildOfType<Accoutrement>() == NULL)
			{
				setParent(touchingCharacter);
			}
			
			//RBXASSERT(backendAccoutrementState == EQUIPPED);
		}
	}
}


void Accoutrement::onEvent_AttachmentAdjusted(const RBX::Reflection::PropertyDescriptor* propertyDescriptor)
{
	RBXASSERT(Network::Players::backendProcessing(this));

	if (propertyDescriptor == &RBX::Attachment::prop_Frame)
	{
		updateWeld();
	}
}


void Accoutrement::onAncestorChanged(const AncestorChanged& event)
{
	const ServiceProvider* oldProvider = ServiceProvider::findServiceProvider(event.oldParent);
	const ServiceProvider* newProvider = ServiceProvider::findServiceProvider(event.newParent);

	if (oldProvider)
	{
		if (Network::Players::backendProcessing(oldProvider))
		{
			setDesiredState(NOTHING, oldProvider);
		}
		else
		{
			setBackendAccoutrementStateNoReplicate(NOTHING);
		}
	}

	Super::onAncestorChanged(event);

	if (newProvider)
	{
		if (Network::Players::backendProcessing(newProvider))
		{
			rebuildBackendState();
		}
	}
}

void Accoutrement::updateWeld()
{
	if (weld != NULL)
	{
		// all this does not occur client side
		RBXASSERT(Network::Players::backendProcessing(this));

		Attachment* attachment = NULL;

		PartInstance* handle = getHandle();
		if (handle && (weld->getParent() == handle))
		{
			//If the weld is under the handle, then it is using the new attachment system
			attachment = handle->findFirstChildOfType<Attachment>();
		}

		if (attachment)
		{
			//new attachment system
			weld->setC0(attachment->getFrameInPart());
		}
		else
		{
			//legacy hat
			weld->setC1(attachmentPoint);
		}
	}
}

void Accoutrement::setAttachmentPoint(const CoordinateFrame& value)
{
	if (value != attachmentPoint)
	{
		attachmentPoint = value;

		raisePropertyChanged(prop_AttachmentPoint);

		if (DFFlag::AccessoriesAndAttachments)
		{
			updateWeld();
		}
		else
		{
			if (weld != NULL)
			{
				// all this does not occur client side
				RBXASSERT(Network::Players::backendProcessing(this));
				weld->setC1(attachmentPoint);
			}
		}
	}
}

// Auxillary UI props
const Vector3 Accoutrement::getAttachmentPos() const
{
	return attachmentPoint.translation;
}

const Vector3 Accoutrement::getAttachmentForward() const
{
	return attachmentPoint.lookVector();
}

const Vector3 Accoutrement::getAttachmentUp() const
{
	return attachmentPoint.upVector();
}

const Vector3 Accoutrement::getAttachmentRight() const
{
	return attachmentPoint.rightVector();
}

void Accoutrement::setAttachmentPos(const Vector3& v)
{
	setAttachmentPoint(CoordinateFrame(attachmentPoint.rotation, v));
}

void Accoutrement::setAttachmentForward(const Vector3& v)
{
	CoordinateFrame c(attachmentPoint);

	//Vector3 oldX = c.rightVector();
	Vector3 oldY = c.upVector();

	Vector3 z = -Math::safeDirection(v);

	Vector3 x = oldY.cross(z);
	x.unitize();

	Vector3 y = z.cross(x);
	y.unitize();

	c.rotation.setColumn(0, x);
	c.rotation.setColumn(1, y);
    c.rotation.setColumn(2, z);

	setAttachmentPoint(c);
}

void Accoutrement::setAttachmentUp(const Vector3& v)
{
	CoordinateFrame c(attachmentPoint);

	Vector3 oldX = c.rightVector();
	//Vector3 oldZ = -c.lookVector();

	Vector3 y = Math::safeDirection(v);

	Vector3 z = oldX.cross(y);
	z.unitize();

	Vector3 x = y.cross(z);
	x.unitize();

	c.rotation.setColumn(0, x);
	c.rotation.setColumn(1, y);
    c.rotation.setColumn(2, z);

	setAttachmentPoint(c);
}

void Accoutrement::setAttachmentRight(const Vector3& v)
{
	CoordinateFrame c(attachmentPoint);

	Vector3 oldY = c.upVector();
	//Vector3 oldZ = -c.lookVector();

	Vector3 x = Math::safeDirection(v);

	Vector3 z = x.cross(oldY);
	z.unitize();

	Vector3 y = z.cross(x);
	y.unitize();

	c.rotation.setColumn(0, x);
	c.rotation.setColumn(1, y);
    c.rotation.setColumn(2, z);

	setAttachmentPoint(c);
}

Hat::Hat() 
: DescribedCreatable<Hat, Accoutrement, sHat>()
{
	setName(sHat);
}

Accessory::Accessory() 
: DescribedCreatable<Accessory, Accoutrement, sAccessory>()
{
	setName(sAccessory);
}

} // namespace
