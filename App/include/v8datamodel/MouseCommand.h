/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Service.h"
#include "GfxBase/IAdornable.h"
#include "Util/HitTestFilter.h"
#include "Util/Name.h"
#include "Util/TextureId.h"
#include "Util/G3DCore.h"

#include <string>

#define MAX_SEARCH_DEPTH (2048.0f)

LOGGROUP(MouseCommandLifetime)

namespace RBX {
	class PartInstance;
	class PVInstance;
	class Workspace;
	class ICameraOwner;
	class Surface;
	class Selection;
	class ContactManager;
	class InputObject;
	class Adorn;

	class MouseCommand 
		: public INamed
		, public IAdornable
		, public boost::enable_shared_from_this<MouseCommand>
	{
		/*
			MouseCommand has a "captured()" state indicating that it
			wants to handle mouse events exclusively.

			World calls onMouseXXX() to feed events. onMouse() returns
			the MouseCommand that will handle subsequent events.

			onMouseXXX functions can call capture() and releaseCapture()
		*/

	private:
		static float maxSearch() {return MAX_SEARCH_DEPTH;}
		static Vector3 ignoreVector3;
		static bool advArrowToolEnabled;
		bool capturedMouse;

	protected:
		friend class Workspace;
		Workspace* workspace;
		shared_ptr<PartInstance> lastHoverPart;
        
        // TODO:: REMOVE ME -- flag "UsedFixedRightMouseClickBehaviour" in "NullTool.cpp"
        shared_ptr<PartInstance> rightMouseClickPart;
        
	public:
		static RbxRay getUnitMouseRay(const shared_ptr<InputObject>& inputObject, ICameraOwner* workspace);
		static RbxRay getSearchRay(const shared_ptr<InputObject>& inputObject, ICameraOwner* workspace);
		static RbxRay getSearchRay(const RbxRay& unitRay);
		Instance* getTopSelectable3d(PartInstance* part) const;

		RbxRay getUnitMouseRay(const shared_ptr<InputObject>& inputObject) const;
		RbxRay getSearchRay(const shared_ptr<InputObject>& inputObject) const;

		static PartInstance* getPart(Workspace* workspace, const shared_ptr<InputObject>& inputObject, const HitTestFilter* filter = NULL, Vector3& hitWorld = ignoreVector3);
		PartInstance* getUnlockedPart(const shared_ptr<InputObject>& inputObject, Vector3& hitWorld = ignoreVector3);
		static PartInstance* getPartByLocalCharacter(Workspace* workspace, const shared_ptr<InputObject>& inputObject, const HitTestFilter* filter = NULL, Vector3& hitWorld = ignoreVector3);
		PartInstance* getUnlockedPartByLocalCharacter(const shared_ptr<InputObject>& inputObject, Vector3& hitWorld = ignoreVector3);

		static Surface getSurface(Workspace* workspace, const shared_ptr<InputObject>& inputObject, const HitTestFilter* filter = NULL);
		static Surface getSurface(Workspace* workspace, const shared_ptr<InputObject>& inputObject, const HitTestFilter* filter, PartInstance* &part, int& surfaceId);

		// Override these - the implementations
		virtual shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject) {return shared_from(this);}
		virtual shared_ptr<MouseCommand> onRightMouseDown(const shared_ptr<InputObject>& inputObject) { return shared_from(this); }
	protected:
		virtual void onMouseIdle(const shared_ptr<InputObject>& inputObject) {}	// Called on heartbeat regardless of capture status
		virtual void onMouseHover(const shared_ptr<InputObject>& inputObject) {}
		virtual shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject) {return shared_from(this);}
		virtual shared_ptr<MouseCommand> onKeyUp(const shared_ptr<InputObject>& inputObject) {return shared_from(this);}
		virtual shared_ptr<MouseCommand> onPeekKeyDown(const shared_ptr<InputObject>& inputObject) {return shared_ptr<MouseCommand>();}
		virtual shared_ptr<MouseCommand> onPeekKeyUp(const shared_ptr<InputObject>& inputObject) {return shared_ptr<MouseCommand>();}
		virtual void onMouseMove(const shared_ptr<InputObject>& inputObject) {}
		virtual void onMouseDelta(const shared_ptr<InputObject>& inputObject) {}
		virtual shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject) { releaseCapture(); return shared_ptr<MouseCommand>(); }
		virtual shared_ptr<MouseCommand> onRightMouseUp(const shared_ptr<InputObject>& inputObject) { return shared_from(this); }
		virtual shared_ptr<MouseCommand> onMouseWheelForward(const shared_ptr<InputObject>& inputObject) {return shared_ptr<MouseCommand>();}
		virtual shared_ptr<MouseCommand> onMouseWheelBackward(const shared_ptr<InputObject>& inputObject) {return shared_ptr<MouseCommand>();}

		virtual void capture();
		virtual void releaseCapture() {capturedMouse = false;}
		virtual void cancel() {
			if (captured())
				releaseCapture();
		}

		bool characterCanReach(const Vector3& hitPoint) const;
		float distanceToCharacter(const Vector3& hitPoint) const;

		MouseCommand(Workspace* workspace);


	public:
		virtual ~MouseCommand();
		bool captured()	const					{return capturedMouse;}

		virtual shared_ptr<MouseCommand> isSticky() const {return shared_ptr<MouseCommand>();}
		virtual bool drawConnectors() const {return false;}			// default mouse command no draw connectors
		virtual TextureId getCursorId() const;	// called on draw

		static PartInstance* getMousePart(
			const RbxRay& unitRay,
			const ContactManager& contactManager,
			const Primitive* ignore,
			const HitTestFilter* filter,
			Vector3& hitPoint = ignoreVector3,
			float maxSearchGrid = maxSearch());

		static PartInstance* getMousePart(
			const RbxRay& unitRay,
			const ContactManager& contactManager,
			const std::vector<const Primitive*> &ignore,
			const HitTestFilter* filter,
			Vector3& hitPoint = ignoreVector3,
			float maxSearchGrid = maxSearch());
		
		// NOTE - Following two functions are added to prevent the modification of arrow tool behavior in MFC Studio
		//        These functions should be removed once Qt Studio becomes default
		//		  These functions also disable software mouse rendering, and enabled hardware mouse rendering (something only Qt does right now)
		static void enableAdvArrowTool(bool state) { advArrowToolEnabled = state; }
		static bool isAdvArrowToolEnabled() { return advArrowToolEnabled; }

		static void predelete(MouseCommand*){}

		Workspace* getWorkspace() { return workspace; }

	private:
		// TODO: deprecated. Override getCursorId instead
		virtual const std::string getCursorName() const	{return isAdvArrowToolEnabled() ? "advCursor-default" : "ArrowCursor";}
	};

} // namespace RBX