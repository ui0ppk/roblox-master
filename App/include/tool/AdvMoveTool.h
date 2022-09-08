/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/ToolsArrow.h"
#include "Tool/MegaDragger.h"
#include "AppDraw/HandleType.h"
#include "Util/NormalId.h"

namespace RBX {

	class MegaDragger;
	class Extents;

	class AdvMoveToolBase	: public AdvArrowToolBase
	{	
	protected:
		bool			dragging;
		Ray				dragRay;
		int				dragAxis;
		int             dragAxisDirection;
        NormalId        dragNormalId;
		std::string		cursor;

		//TODO: Have a single variable in Move and Rotate tool
		mutable NormalId overHandleNormalId;

	private:
		typedef AdvArrowToolBase Super;
		std::auto_ptr<MegaDragger> megaDragger;
		Vector2int16	downPoint2d;
	
		// dynamic - last point on the Ray we dragged to
        // only works with one-stud grid...
		Vector3			lastPoint3d;	

		typedef std::map<boost::weak_ptr<RBX::PartInstance>, float> PartsTransparencyCollection;
		PartsTransparencyCollection origPartsTransparency;

        mutable Matrix3 mMultiRotation;
        mutable Extents mExtents;
        mutable bool    mInitializedExtents;

        float snapRotationAngle(float Angle) const; 
		void saveAndModifyPartsTransparency();
		void restoreSavedPartsTransparency();

		rbx::signals::scoped_connection selectionChangedConnection;
		void setToSelection();

    protected:

        virtual bool getLocalSpaceMode() const;
		virtual bool getOverHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& normalId) const;

		bool getExtents(Extents& extents) const;	
        bool getExtentsAndLocation(
            Extents&            extents,
            CoordinateFrame&    location,
            bool&               isLocal ) const;
        bool getOverHandle(const shared_ptr<InputObject>& inputObject) const;
        
		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);

		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ const std::string getCursorName() const {return cursor;}
		/*override*/ void setCursor(std::string newCursor) {cursor = newCursor;}


		/*implement*/ virtual Color3 getHandleColor() const = 0;
		/*implement*/ virtual HandleType getDragType() const = 0;

	public:
		AdvMoveToolBase(Workspace* workspace);
		virtual ~AdvMoveToolBase() 
		{}
	};


	extern const char* const sAdvMoveTool;
	class AdvMoveTool	: public Named<AdvMoveToolBase, sAdvMoveTool>
	{
	private:
		/*override*/ Color3 getHandleColor() const {return Color3::orange();}
		/*override*/ HandleType getDragType() const {return HANDLE_MOVE;}
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);

		void getGridXYUsingCamera(RBX::PartInstance *part, G3D::Vector3 &gridXDir, G3D::Vector3 &gridYDir);

		G3D::Vector3 originalLocation;

	public:
		AdvMoveTool(Workspace* workspace) : Named<AdvMoveToolBase, sAdvMoveTool>(workspace)
		{}
		~AdvMoveTool() {}

		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<AdvMoveTool>(workspace);}		
	};


} // namespace RBX
