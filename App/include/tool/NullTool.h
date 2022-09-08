/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX {

	namespace Network {
		class Player;
	}

	// Super tool - combines click to move, etc.
	extern const char* const sNewNullTool;
	class NewNullTool : public Named<MouseCommand, sNewNullTool>
	{
	private:
		typedef Named<MouseCommand, sNewNullTool> Super;
		std::string cursor;
		bool hasWaypoint;
		Vector3 waypoint;

		bool isInFirstPerson();
		void getIndicatedPart(const shared_ptr<InputObject>& inputObject, const bool& clickEvent,
				PartInstance** instance, bool* clickable, Vector3* waypoint);
		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onRightMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const {return cursor;}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<NewNullTool>(workspace);}
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject) {
			releaseCapture(); 
			return shared_from(this); 
		}
		/*override*/ shared_ptr<MouseCommand> onRightMouseUp(const shared_ptr<InputObject>& inputObject);

		/////////////////////////////////////////////////////////
		// IAdornable
		//
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ void render3dAdorn(Adorn* adorn);

		void updateClickDetectorHover(const shared_ptr<InputObject>& inputObject);

	public:
		NewNullTool(Workspace* workspace);
		~NewNullTool();

	};

} // namespace RBX