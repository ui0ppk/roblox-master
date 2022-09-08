/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/ToolsArrow.h"
#include "AppDraw/HandleType.h"
#include "Util/NormalId.h"

namespace RBX {

	extern const char* const sResizeTool;

	class ResizeTool : public Named<ArrowToolBase, sResizeTool>
	{

	private:
		typedef Named<ArrowToolBase, sResizeTool> Super;
		void findTargetPV(const shared_ptr<InputObject>& inputObject);
		void capturedDrag(int axisDelta);

		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors

	protected:
		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ void render2d(Adorn* adorn);

		// Tool
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const;

		weak_ptr<PVInstance> targetPV;	

		bool				overHandle;
		NormalId			localNormalId;
		Vector3				hitPointGrid;
		Vector2int16		down;
		int					moveAxis;
		int					movePerp;
		int  				moveIncrement;

	public:
		ResizeTool(Workspace* workspace) :
		  Named<ArrowToolBase, sResizeTool>(workspace),
		  overHandle(false),
		  moveAxis(0),
		  movePerp(0)
		  {}
		  /*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<ResizeTool>(workspace);}
	};

} // namespace RBX