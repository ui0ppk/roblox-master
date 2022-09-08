/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"
#include "V8Xml/Reference.h"
#include "V8Tree/Verb.h"
#include "Tool/ICancelableTool.h"
#include "util/NormalId.h"
#include "V8DataModel/Decal.h"
#include "v8datamodel/Surface.h"


namespace RBX {


	class SurfaceTool : public MouseCommand
	{
	private:
		typedef MouseCommand Super;
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void render3dAdorn(Adorn* adorn);

		int surfaceId;
		std::string name;

	protected:
		virtual void doAction(Surface* surface) = 0;
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);

		shared_ptr<PartInstance> partInstance;
		Surface surface;

	public:
		SurfaceTool(Workspace* workspace);
		~SurfaceTool();
	};

	
	extern const char* const sDecalTool;
	class DecalTool 
		: public Named<SurfaceTool, sDecalTool>
		, public ICancelableTool
	{
	private:
		shared_ptr<Decal> decal;
		RBX::InsertMode insertMode;
		bool parentIsPart;
		/*override*/ const std::string getCursorName() const	{return "ArrowCursorDecalDrag";}
		/*override*/ void doAction(Surface* surface) {}
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
	public:
		DecalTool(Workspace* workspace, Decal *decal, RBX::InsertMode insertMode);
		/*override*/ //MouseCommand* isSticky() const {return new DecalTool(workspace);}
		
		/*override*/ virtual shared_ptr<MouseCommand> onCancelOperation();
	};

	extern const char* const sFlatTool;
	class FlatTool : public Named<SurfaceTool, sFlatTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "FlatCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		FlatTool(Workspace* workspace) : Named<SurfaceTool, sFlatTool>(workspace)	{}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<FlatTool>(workspace);}
	};

	extern const char* const sGlueTool;
	class GlueTool : public Named<SurfaceTool, sGlueTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "GlueCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		GlueTool(Workspace* workspace) : Named<SurfaceTool, sGlueTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<GlueTool>(workspace);}
	};


	extern const char* const sWeldTool;
	class WeldTool : public Named<SurfaceTool, sWeldTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "WeldCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		WeldTool(Workspace* workspace) : Named<SurfaceTool, sWeldTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<WeldTool>(workspace);}
	};


	extern const char* const sStudsTool;
	class StudsTool : public Named<SurfaceTool, sStudsTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "StudsCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		StudsTool(Workspace* workspace) : Named<SurfaceTool, sStudsTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<StudsTool>(workspace);}
	};

	extern const char* const sInletTool;
	class InletTool : public Named<SurfaceTool, sInletTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "InletCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		InletTool(Workspace* workspace) : Named<SurfaceTool, sInletTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<InletTool>(workspace);}
	};

	extern const char* const sUniversalTool;
	class UniversalTool : public Named<SurfaceTool, sUniversalTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "UniversalCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		UniversalTool(Workspace* workspace) : Named<SurfaceTool, sUniversalTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<UniversalTool>(workspace);}
	};

	extern const char* const sHingeTool;
	class HingeTool : public Named<SurfaceTool, sHingeTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "HingeCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		static bool isStickyVerb;
		HingeTool(Workspace* workspace) : Named<SurfaceTool, sHingeTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const;
	};

	extern const char* const sRightMotorTool;
	class RightMotorTool : public Named<SurfaceTool, sRightMotorTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "MotorCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		static bool isStickyVerb;
		RightMotorTool(Workspace* workspace) : Named<SurfaceTool, sRightMotorTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const;
	};

	extern const char* const sLeftMotorTool;
	class LeftMotorTool : public Named<SurfaceTool, sLeftMotorTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "MotorCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		LeftMotorTool(Workspace* workspace) : Named<SurfaceTool, sLeftMotorTool>(workspace) {}
	};


	extern const char* const sOscillateMotorTool;
	class OscillateMotorTool : public Named<SurfaceTool, sOscillateMotorTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "MotorCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		OscillateMotorTool(Workspace* workspace) : Named<SurfaceTool, sOscillateMotorTool>(workspace) {}
	};

	extern const char* const sSmoothNoOutlinesTool;
	class SmoothNoOutlinesTool : public Named<SurfaceTool, sSmoothNoOutlinesTool>
	{
	private:
		/*override*/ const std::string getCursorName() const	{return "FlatCursor";}
		/*override*/ void doAction(Surface* surface);
	public:
		SmoothNoOutlinesTool(Workspace* workspace) : Named<SurfaceTool, sSmoothNoOutlinesTool>(workspace) {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<SmoothNoOutlinesTool>(workspace);}
	};

} // namespace RBX