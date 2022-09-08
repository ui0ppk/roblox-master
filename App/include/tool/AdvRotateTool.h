/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/AdvMoveTool.h"

namespace RBX {

	extern const char* const sAdvRotateTool;
	class AdvRotateTool : public Named<AdvMoveToolBase, sAdvRotateTool>
	{
	private:
		typedef Named<AdvMoveToolBase, sAdvRotateTool> Super;
		
        int getNormalMask() const;

        mutable NormalId    mOverHandleNormalId;

    protected:

        virtual Color3 getHandleColor() const {return Color3::green();}
		virtual HandleType getDragType() const {return HANDLE_ROTATE;}		
        virtual bool getLocalSpaceMode() const;
		virtual bool getOverHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& normalId) const;
        
	public:
		AdvRotateTool(Workspace* workspace) : 
            Named<AdvMoveToolBase, sAdvRotateTool>(workspace),
            mOverHandleNormalId(NORM_UNDEFINED)
		{}

		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<AdvRotateTool>(workspace);}
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render3dAdorn(Adorn* adorn);

	};



} // namespace RBX
