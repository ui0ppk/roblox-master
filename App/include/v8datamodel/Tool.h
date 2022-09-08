/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/IEquipable.h"
#include "V8DataModel/Hopper.h"
#include "GfxBase/IAdornable.h"
#include "Util/IHasLocation.h"
#include "Util/Selectable.h"
#include "Util/ContentFilter.h"
#include "Gui/ProfanityFilter.h"

namespace RBX {

	class PartInstance;
	class ModelInstance;
	class ToolMouseCommand;
	class Mouse;
	class ServiceProvider;
	class Workspace;
	class Attachment;

	namespace Network {
		class Player;
	}

	extern const char *const sTool;

	class Tool 
		: public DescribedCreatable<Tool, BackpackItem, sTool>
		, public IEquipable					// TODO - move stuff here that's common with Accoutrement
		, public IAdornable
		, public IHasLocation
	{
	public:

		// subclass signal to implement special logic for equipped:
		// * track weather the tool is currently equipped
		// * when a new connection is made to this signal, if the tool is
		//   equipped then fire the signal to the new listener
		class special_equipped_signal : public rbx::signals::signal_with_args<1, void(shared_ptr<Instance>)> //rbx::signal<void(shared_ptr<Instance>)>
		{
		private:
			typedef rbx::signals::signal_with_args<1, void(shared_ptr<Instance>)> Super;
			bool currentlyEquipped;
			weak_ptr<Instance> lastArg;

		public:
			special_equipped_signal();

			// Note: this operator throws. See Implementation.
			void operator()(shared_ptr<Instance> instance);

			void equipped(shared_ptr<Instance> instance);
			
			void unequipped();

			template<class Delegate>
			rbx::signals::connection connect(Delegate& function) {
				rbx::signals::connection result = Super::connect(function);
				if (currentlyEquipped) {
					function(lastArg.lock());
				}
				return result;
			}
		};

	private:

		typedef DescribedCreatable<Tool, BackpackItem, sTool> Super;
		int getNumToolsInCharacter();			// debugging

		typedef enum {	NOTHING,
						HAS_HANDLE,
						IN_WORKSPACE,
						IN_CHARACTER,
						HAS_TORSO,
						EQUIPPED	}	ToolState;

		// Replicated Properties
		ToolState backendToolState;					// backend writes, frontend reads
		int frontendActivationState;			// frontend writes, backend reads
		CoordinateFrame grip;					// replicates, stores - the grip point on the tool
		bool enabled;							// Tool is ready to go (cooldown property)
		bool droppable;
		bool requiresHandle;
		std::string toolTip;
        
        bool manualActivationOnly;
		bool ownWeld;							// owner means it's responsible for creating and deleting the weld

		// "Backend" connections
		rbx::signals::scoped_connection_logged handleTouched;			// watches for handle touched
		rbx::signals::scoped_connection characterChildAdded;		
		rbx::signals::scoped_connection characterChildRemoved;		
		rbx::signals::scoped_connection torsoChildAdded;			
		rbx::signals::scoped_connection torsoChildRemoved;
		rbx::signals::scoped_connection armChildAdded;			


		// Event handlers - hooked to these signals
		void onEvent_AddedBackend(shared_ptr<Instance> child);
		void onEvent_RemovedBackend(shared_ptr<Instance> child);
		void onEvent_AddedToArmBackend(shared_ptr<Instance> child, Instance *handle);
		void onEvent_HandleTouched(shared_ptr<Instance> other);

		static void moveAllToolsToBackpack(Network::Player* player);
		void moveOtherToolsToBackpack(weak_ptr<Network::Player> player_weak_ptr);

		void setTimerCallback(weak_ptr<Network::Player> player);

		static ToolState characterCanPickUpTool(Instance* touchingCharacter);

		Workspace* workspaceForToolMouseCommand;
		shared_ptr<Mouse> createMouse();
        
        shared_ptr<Mouse> currentMouse;
        shared_ptr<ToolMouseCommand> currentToolMouseCommand;
        
        void setMousePositionForInputType();

		///////////////////////////////////////////////////////////////
		//
		// Backend side
		//
		ToolState computeDesiredState();

		ToolState computeDesiredState(Instance* testParent);

		void setDesiredState(ToolState desiredState, const ServiceProvider* serviceProvider);

		void rebuildBackendState();

		void connectTouchEvent();

		// Backend - climbing state
		void upTo_Activated();
		void upTo_Equipped();
		void upTo_HasTorso();
		void upTo_InCharacter();
		void upTo_InWorkspace();
		void upTo_HasHandle();

		// Backend - dropping state
		void downFrom_Activated();
		void downFrom_Equipped(bool connectTouchEvent = true);
		void downFrom_HasTorso();
		void downFrom_InCharacter();
		void downFrom_InWorkspace();
		void downFrom_HasHandle();

		// state short cuts
		void fromNothingToEquipped(bool isBackend);
		void fromEquippedToNothing();

		///////////////////////////////////////////////////////////////////////
		//
		// Frontend side
		shared_ptr<Mouse> onEquipping();
		void onUnequipped();

		// Instance
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoved(Instance* child);
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ bool askSetParent(const Instance* instance) const {return true;}
		/*override*/ bool askAddChild(const Instance* instance) const {return true;}

		// IHasLocation
		/*override*/ const CoordinateFrame getLocation();

		// BackpackItem
		/*override*/ bool drawSelected() const {return (backendToolState >= EQUIPPED);}
		/*override*/ void onLocalClicked();
		/*override*/ void onLocalOtherClicked();

		// IAdornable
		/*override*/ void render3dSelect(Adorn* adorn, SelectState selectState);

		
		
		static bool characterCanUnequipTool(ModelInstance *character);
	public:
		Tool();
		~Tool();

		special_equipped_signal equippedSignal;
		rbx::remote_signal<void()> activatedSignal;
		rbx::signal<void()> unequippedSignal;
		rbx::remote_signal<void()> deactivatedSignal;

		static void dropAll(Network::Player* player);
	
        bool isDroppable() const { return droppable; }
		void setDroppable( bool newDroppable ){ droppable = newDroppable; }
		bool getRequiresHandle() const { return requiresHandle; }
		void setRequiresHandle(bool value);
			
		virtual bool canUnequip() {return true;}
		virtual bool canBePickedUpByPlayer(Network::Player *p) {return true;}
		
        /* override */ bool isSelectable3d();

		PartInstance* getHandle();
		const PartInstance* getHandleConst() const;

		//////////////////////////////////////////////////////////
		//
		// REPLICATION Signals
		void setFrontendActivationState(int value);
		int getFrontendActivationState() const {return frontendActivationState;}

		void setToolTip(std::string value);
		std::string getToolTip() const { return toolTip; }

		void setBackendToolState(int value);
		int getBackendToolState() const {return backendToolState;}

		static Reflection::BoundProp<bool> prop_Enabled;

		const CoordinateFrame& getGrip() const {return grip;}
		void setGrip(const CoordinateFrame& value);

		//////////////////////////////////////////////////////////
		//
		void luaActivate();

		void activate(bool manuallyActivated = false);
		void deactivate();
        
        bool getManualActivationOnly() const { return manualActivationOnly; }
        void setManualActivationOnly(bool value);

		// Auxillary UI props
		const Vector3 getGripPos() const;
		const Vector3 getGripForward() const;
		const Vector3 getGripUp() const;
		const Vector3 getGripRight() const;

		void setGripPos(const Vector3& v);
		void setGripForward(const Vector3& v);
		void setGripUp(const Vector3& v);
		void setGripRight(const Vector3& v);

		static Attachment *findFirstAttachmentByName(const Instance *part, const std::string& findName);
		static Attachment *findFirstAttachmentByNameRecursive(const Instance *part, const std::string& findName);

	};

} // namespace
