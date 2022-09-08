/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"
#include "Util/BrickColor.h"
#include "V8DataModel/PartInstance.h"
//#include "V8Xml/Reference.h"
//#include "V8Tree/Verb.h"
//#include "util/NormalId.h"

namespace RBX {

	class PartTool : public MouseCommand
	{
	private:
		typedef MouseCommand Super;
		shared_ptr<PartInstance> partInstance;

	protected:
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);

	public:
		PartTool(Workspace* workspace);
		~PartTool();
	};


	class FillToolColor 
	{
		BrickColor color;
	public:
		rbx::signal<void(RBX::BrickColor)> brickColorSignal;

		FillToolColor();
		RBX::BrickColor get() const {return color;}
		void set(const RBX::BrickColor& newColor)			{
			if (color!=newColor) {
				color = newColor;
				brickColorSignal(color);
			}
		}
	};

	// Sets the color of a Part
	extern const char* const sFillTool;
	class FillTool : public Named<PartTool, sFillTool>
	{
	public:
		static FillToolColor color;
	protected:
		/*override*/ virtual shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const	{return "FillCursor";}
	public:
		FillTool(Workspace* workspace) : 
		  Named<PartTool, sFillTool>(workspace)
		  {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<FillTool>(workspace);}
	};


	extern const char* const sDropperTool;
	class DropperTool : public Named<PartTool, sDropperTool>
	{
	protected:
		/*override*/ virtual shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const	{return "DropperCursor";}
	public:
		DropperTool(Workspace* workspace) : Named<PartTool, sDropperTool>(workspace) {}
	};


	// Sets the color of a Part
	extern const char* const sMaterialTool;
	class MaterialTool : public Named<PartTool, sMaterialTool>
	{
	public:
		static PartMaterial material;
	protected:
		/*override*/ virtual shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const	{return "MaterialCursor";}
	public:
		MaterialTool(Workspace* workspace) : 
		  Named<PartTool, sMaterialTool>(workspace)
		  {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<MaterialTool>(workspace);}
	};


} // namespace RBX