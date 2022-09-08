/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"
#include "V8DataModel/ManualJointHelper.h"

namespace RBX {

class PartInstance;
class Decal;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//
//		ARROW

#define STUDIO_CAMERA_CONTROL_SHORTCUTS 1

	class ArrowToolBase : public MouseCommand
	{
	private:

		typedef MouseCommand Super;

        bool altKeyDown;

	protected:

		Decal* findDecal(PartInstance* p, const shared_ptr<InputObject>& inputObject);

		virtual void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		virtual void onMouseHover(const shared_ptr<InputObject>& inputObject);
		virtual shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		virtual const std::string getCursorName() const;
		virtual shared_ptr<MouseCommand> onPeekKeyDown(const shared_ptr<InputObject>& inputObject);
        virtual void render3dAdorn(Adorn* adorn);

        void renderHoverOver(Adorn* adorn,bool drillDownOnly = true);

        PartInstance* overInstance;
        
	public:

		ArrowToolBase(Workspace* workspace) 
            : MouseCommand(workspace), 
              overInstance(NULL), 
              altKeyDown(false)
		{
			FASTLOG1(FLog::MouseCommandLifetime, "ArrowTool created: %p", this);
		}

		virtual ~ArrowToolBase() 
        {
			FASTLOG1(FLog::MouseCommandLifetime, "ArrowTool destroyed: %p", this);
		}

		static bool  showDraggerGrid;
	};

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//
//		ADVANCED ARROW (parent class for all advanced mouse manipulation

	class AdvArrowToolBase : public ArrowToolBase
	{
	private:
		ManualJointHelper manualJointHelper;
		typedef ArrowToolBase Super;
	protected:
		typedef std::map<boost::weak_ptr<RBX::PartInstance>, float> PartsTransparencyCollection;
		static PartsTransparencyCollection originalPartsTransparency;
	public:
		AdvArrowToolBase(Workspace* workspace) : ArrowToolBase(workspace), manualJointHelper(workspace)
		{}
		virtual ~AdvArrowToolBase() {}

		enum JointCreationMode
		{
			WELD_ALL = 0,
			SURFACE_JOIN_ONLY = 1,
			NO_JOIN = 2
		};

		virtual shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		virtual void onMouseMove(const shared_ptr<InputObject>& inputObject);
		virtual shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		virtual const std::string getCursorName() const;
		shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		
		void determineManualJointConditions(void);

		static DRAG::DraggerGridMode advGridMode;
		static bool advManualJointMode;
		static DRAG::ManualJointType advManualJointType;
		static bool advManipInProgress;

		static bool advCollisionCheckMode;
        static bool advLocalTranslationMode;
        static bool advLocalRotationMode;
		static bool advCreateJointsMode;

		static void restoreSavedPartsTransparency();
		static void savePartTransparency(shared_ptr<PartInstance> part);

		static AdvArrowToolBase::JointCreationMode getJointCreationMode();

		virtual void getSelectedTargetPrimitives(std::vector<Primitive*>& targetPrims) {}
		virtual void setCursor(std::string cursor) {}
	};
	
	extern const char* const sAdvArrowTool;
	class AdvArrowTool : public Named<AdvArrowToolBase, sAdvArrowTool>
	{
	public:
		AdvArrowTool(Workspace* workspace) : Named<AdvArrowToolBase, sAdvArrowTool>(workspace)
		{
		}
		
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<AdvArrowTool>(workspace);}

		virtual void setCursor(std::string cursor) {}

	};

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//
//		BOX SELECT

	extern const char* const sBoxSelectCommand;
	class BoxSelectCommand : public Named<MouseCommand, sBoxSelectCommand>
	{
	private:
		typedef Named<MouseCommand, sBoxSelectCommand> Super;
		ServiceClient<Selection>	selection;
		bool						reverseSelecting;
		Vector2int16				mouseDownView;
		Vector2int16				mouseCurrentView;
		std::set< shared_ptr<Instance> >			previousItemsInBox;

		void getMouseInstances(std::set< shared_ptr<Instance> >& instances, 
									const shared_ptr<InputObject>& inputObject,
									const Rect2D& selectBox, const Camera* camera,
									Instance* currentInstance);

		void selectAnd(const std::set< shared_ptr<Instance> >& newItemsInBox);

		void selectReverse(const std::set< shared_ptr<Instance> >& newItemsInBox);

	public:
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ void render2d(Adorn* adorn);

		BoxSelectCommand(Workspace* workspace);
		~BoxSelectCommand();
	};

} // namespace RBX
