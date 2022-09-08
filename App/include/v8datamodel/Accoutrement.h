/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8DataModel/IEquipable.h"
#include "V8DataModel/PartInstance.h"
#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"
#include "Util/CameraSubject.h"
#include "Util/Selectable.h"

namespace RBX {

	class PartInstance;
	class ModelInstance;
	class Weld;
	class ServiceProvider;
	class Workspace;
	class Attachment;

	namespace Network {
		class Player;
	}

	extern const char *const sAccoutrement;
	extern const char *const sHat;
	extern const char *const sAccessory;

	class Accoutrement 
		: public DescribedCreatable<Accoutrement, Instance, sAccoutrement>
		, public IEquipable					// TODO - move stuff here that's common with Tool
		, public IAdornable
		, public CameraSubject
	{		
		typedef DescribedCreatable<Accoutrement, Instance, sAccoutrement> Super;

	protected:		

		typedef enum {	NOTHING,
						HAS_HANDLE,
						IN_WORKSPACE,
						IN_CHARACTER,
						EQUIPPED
					 }	AccoutrementState;

		// Replicated Properties
		AccoutrementState backendAccoutrementState;			// backend writes, frontend reads
		CoordinateFrame attachmentPoint;					// replicates, stores

		// "Backend" connections
		rbx::signals::scoped_connection_logged handleTouched;			// watches for handle touched
		rbx::signals::scoped_connection characterChildAdded;		
		rbx::signals::scoped_connection characterChildRemoved;				
		rbx::signals::scoped_connection attachmentAdjusted;

		// Event handlers - hooked to these signals
		void onEvent_AddedBackend(shared_ptr<Instance> child);
		void onEvent_RemovedBackend(shared_ptr<Instance> child);
		void onEvent_HandleTouched(shared_ptr<Instance> other);
		void onEvent_AttachmentAdjusted(const RBX::Reflection::PropertyDescriptor*);

		static AccoutrementState characterCanPickUpAccoutrement(Instance* touchingCharacter);

		static void UnequipThis(shared_ptr<Instance> instance);

		void updateWeld();

		///////////////////////////////////////////////////////////////
		//
		// Backend side
		//
		AccoutrementState computeDesiredState();
		AccoutrementState computeDesiredState(Instance *testParent);
		

		void setDesiredState(AccoutrementState desiredState, const ServiceProvider* serviceProvider);

		void setBackendAccoutrementStateNoReplicate(int value);

		void rebuildBackendState();

		void connectTouchEvent();

		void connectAttachmentAdjustedEvent();

		// Backend - climbing state
		void upTo_Equipped();
		void upTo_InCharacter();
		void upTo_InWorkspace();
		void upTo_HasHandle();
		

		// Backend - dropping state
		void downFrom_Equipped();
		void downFrom_InCharacter();
		void downFrom_InWorkspace();
		void downFrom_HasHandle();

		///////////////////////////////////////////////////////////////////////
		//
		// Frontend side

		// Instance
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoved(Instance* child);
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ bool askSetParent(const Instance* instance) const {return true;}
		/*override*/ bool askAddChild(const Instance* instance) const {return true;}

		////////////////////////////////////////////////////////////////////////
		// IHasLocation
		/*override*/ const CoordinateFrame getLocation();

		////////////////////////////////////////////////////////////////////////
		// CameraSubject
		//
		/*override*/ const CoordinateFrame getRenderLocation() {return getLocation();}
		/*override*/ const Vector3 getRenderSize()
		{ 
			if(PartInstance* handle = getHandle()) 
				return handle->getPartSizeUi(); 
			return Vector3(0,0,0);
		}
		/*override*/ void onCameraNear(float distance);

		// BackpackItem
		/*override*/ bool drawSelected() const {return (backendAccoutrementState >= EQUIPPED);}

		// IAdornable
		/*override*/ void render3dSelect(Adorn* adorn, SelectState selectState);

	public:

		Accoutrement();
		~Accoutrement();

		static void dropAll(ModelInstance* character);
		static void dropAllOthers(ModelInstance *character, Accoutrement *exception);


		PartInstance* getHandle();
		const PartInstance* getHandleConst() const;

		Attachment* findFirstMatchingAttachment(Instance* model, const std::string& originalAttachment);

		//////////////////////////////////////////////////////////
		//
		// REPLICATION Signals
		void setBackendAccoutrementState(int value);
		int getBackendAccoutrementState() const {return backendAccoutrementState;}

		const CoordinateFrame& getAttachmentPoint() const {return attachmentPoint;}
		void setAttachmentPoint(const CoordinateFrame& value);

		// Auxillary UI props
		const Vector3 getAttachmentPos() const;
		const Vector3 getAttachmentForward() const;
		const Vector3 getAttachmentUp() const;
		const Vector3 getAttachmentRight() const;

		void setAttachmentPos(const Vector3& v);
		void setAttachmentForward(const Vector3& v);
		void setAttachmentUp(const Vector3& v);
		void setAttachmentRight(const Vector3& v);
	};


	class Hat 
		: public DescribedCreatable<Hat, Accoutrement, sHat>
	{
	public:
		Hat();
		
	};

	class Accessory
		: public DescribedCreatable<Accessory, Accoutrement, sAccessory>
	{
	public:
		Accessory();

	};
} // namespace
