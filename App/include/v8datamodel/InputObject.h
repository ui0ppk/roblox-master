//
//  InputObject.h
//  App
//
//  Created by Ben Tkacheff on 3/6/13.
//
//  Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved
//
// Summary:
// This is for input objects that we use in events exposed to lua
// This class is used in events found in UserInputService

#pragma once

#include "SDL_scancode.h"

#include "Util/KeyCode.h"
#include "G3D/Vector3.h"
#include "G3D/Vector2int16.h"
#include "v8tree/Instance.h"


namespace RBX {

	class DataModel;
	class Workspace;
    
	extern const char* const sInputObject;

	class InputObject : public DescribedCreatable<InputObject, Instance, sInputObject, Reflection::ClassDescriptor::INTERNAL>
	{
	public:
        enum UserInputType
        {
            TYPE_MOUSEBUTTON1 = 0,
            TYPE_MOUSEBUTTON2 = 1,
            TYPE_MOUSEBUTTON3 = 2,
            TYPE_MOUSEWHEEL = 3,
            TYPE_MOUSEMOVEMENT = 4,
			TYPE_MOUSEIDLE = 5, // hold over from uievent, used internally (c++) only, todo: see if we can replace this event with mousemovement
			TYPE_MOUSEDELTA = 6, // hold over from uievent, used internally (c++) only todo: see if we can replace this event with mousemovement
            TYPE_TOUCH = 7,
            TYPE_KEYBOARD = 8,
			TYPE_FOCUS = 9,
            TYPE_ACCELEROMETER = 10,
            TYPE_GYRO = 11,
			TYPE_GAMEPAD1 = 12,
			TYPE_GAMEPAD2 = 13,
			TYPE_GAMEPAD3 = 14,
			TYPE_GAMEPAD4 = 15,
			TYPE_GAMEPAD5 = 16,
			TYPE_GAMEPAD6 = 17,
			TYPE_GAMEPAD7 = 18,
			TYPE_GAMEPAD8 = 19,
			TYPE_TEXTINPUT = 20,
            TYPE_NONE = 21 // this should always be last, largest number
        };
        
        enum UserInputState
        {
            INPUT_STATE_BEGIN = 0,
            INPUT_STATE_CHANGE = 1,
            INPUT_STATE_END = 2,
			INPUT_STATE_CANCEL = 3,
            INPUT_STATE_NONE = 4 // this should always be last, largest number
        };
        
    private:
		typedef DescribedNonCreatable<InputObject, Instance, sInputObject> Super;

        UserInputType inputType;
		UserInputType sourceInputType;
        UserInputState inputState;
        G3D::Vector3 position;
		G3D::Vector3 delta;

		weak_ptr<Workspace> weakWorkspace;

		// todo: switch over key structure to below
		unsigned int modCodes;
		std::string inputText;
		KeyCode keyCode;
		SDL_Scancode scanCode;

		Vector4 getGuiInset() const;

	protected:
		//////////////////////////////////////////////////////////////////////////////////////
		// Instance
		// 
		/*override*/ bool askSetParent(const Instance* instance) const { return false; }
		/*override*/ bool askForbidParent(const Instance* instance) const { return true; }
		/*override*/ bool askAddChild(const Instance* instance) const { return false; }
		/*override*/ bool askForbidChild(const Instance* instance) const { return true; }
        
    public:
		// todo: remove these when we switch to other key struct
		ModCode mod;
		char modifiedKey;

		// constructors for days
		InputObject();
		// Copy constructor
		InputObject(const InputObject &source);

		InputObject(UserInputType inputType, UserInputState inputState, G3D::Vector3 newPosition, KeyCode newKeyCode, DataModel* dm);
		InputObject(UserInputType inputType, UserInputState inputState, G3D::Vector3 newPosition, G3D::Vector3 newDelta, DataModel* dm);
		InputObject(UserInputType inputType, UserInputState inputState, DataModel* dm);
		InputObject(UserInputType inputType, UserInputState inputState, KeyCode k, ModCode m, char key, DataModel* dm);
		// key transitory constructor
		InputObject(UserInputType inputType, UserInputState inputState, KeyCode k, unsigned int newModCodes, SDL_Scancode newScancode, std::string newInputText, DataModel* dm);

		G3D::Vector2int16 getWindowSize() const;

        void setInputType(UserInputType newInputType);
        UserInputType getUserInputType() const { return inputType; }

		UserInputType getSourceUserInputType() const { return sourceInputType;}
		void setSourceUserInputType(UserInputType newInputType) { sourceInputType = newInputType; }
        
        void setInputState(UserInputState newInputState);
        UserInputState getUserInputState() const { return inputState; }
        
        void setPosition(G3D::Vector3 newPosition);
        G3D::Vector3 getPosition() const;
		G3D::Vector3 getRawPosition() const;
		G3D::Vector2 get2DPosition() const;

		void setDelta(G3D::Vector3 newDelta);
		G3D::Vector3 getDelta() const { return delta; }
        
        void setKeyCode(KeyCode newKeyCode);
        KeyCode getKeyCode() const { return keyCode; }

		void setScanCode(SDL_Scancode newScanCode);
		SDL_Scancode getScanCode() { return scanCode; }

		void setInputText(std::string newInputText);
		std::string getInputText() { return inputText; }

		void setModCodes(unsigned int newModCodes);
		unsigned int getModCodes() const { return modCodes; }

		bool isPublicEvent();

		bool isMouseEvent() const 
		{
			return ( (inputType >= TYPE_MOUSEBUTTON1) && (inputType <= TYPE_MOUSEMOVEMENT) );
		}
		static bool IsMouseEvent(UserInputType InputType)
		{
			return ((InputType >= TYPE_MOUSEBUTTON1) && (InputType <= TYPE_MOUSEMOVEMENT));
		}
		bool isKeyEvent() const 
		{
			return inputType == TYPE_KEYBOARD;
		}
		bool isTextInputEvent() const
		{
			return inputType == TYPE_TEXTINPUT;
		}

		bool isMouseDownEvent() const
		{
			return ( inputState == INPUT_STATE_BEGIN &&
				(inputType == TYPE_MOUSEBUTTON1 ||	inputType == TYPE_MOUSEBUTTON2 || inputType == TYPE_MOUSEBUTTON3) );
		}

		bool isMouseUpEvent() const
		{
			return ( inputState == INPUT_STATE_END &&
				(inputType == TYPE_MOUSEBUTTON1 ||	inputType == TYPE_MOUSEBUTTON2 || inputType == TYPE_MOUSEBUTTON3) );
		}

		bool isLeftMouseDownEvent() const
		{
			return ( inputState == INPUT_STATE_BEGIN && inputType == TYPE_MOUSEBUTTON1 );
		}

		bool isLeftMouseUpEvent() const
		{
			return ( inputState == INPUT_STATE_END && inputType == TYPE_MOUSEBUTTON1 );
		}

		bool isRightMouseDownEvent() const
		{
			return ( inputState == INPUT_STATE_BEGIN && inputType == TYPE_MOUSEBUTTON2 );
		}

		bool isRightMouseUpEvent() const
		{
			return ( inputState == INPUT_STATE_END && inputType == TYPE_MOUSEBUTTON2 );
		}

		bool isMiddleMouseDownEvent() const
		{
			return ( inputState == INPUT_STATE_BEGIN && inputType == TYPE_MOUSEBUTTON3 );
		}

		bool isMiddleMouseUpEvent() const
		{
			return ( inputState == INPUT_STATE_END && inputType == TYPE_MOUSEBUTTON3 );
		}

		bool isMouseWheelEvent() const
		{
			return ( inputType == TYPE_MOUSEWHEEL );
		}

		bool isMouseWheelForward() const
		{
			return ( inputType == TYPE_MOUSEWHEEL && position.z > 0 );
		}

		bool isMouseWheelBackward() const
		{
			return ( inputType == TYPE_MOUSEWHEEL && position.z < 0 );
		}

		bool isKeyDownEvent() const 
		{
			return (inputType == TYPE_KEYBOARD
				&& (inputState != INPUT_STATE_END && inputState != INPUT_STATE_NONE));
		}
		bool isKeyUpEvent() const
		{
			return (inputType == TYPE_KEYBOARD
				&& (inputState == INPUT_STATE_END));
		}
		bool isKeyDownEvent(KeyCode code) const 
		{
			return (inputType == TYPE_KEYBOARD
				&& (inputState != INPUT_STATE_END && inputState != INPUT_STATE_NONE) 
				&& (keyCode == code));
		}
		bool isKeyUpEvent(KeyCode code) const
		{
			return (inputType == TYPE_KEYBOARD
				&& (inputState == INPUT_STATE_END) 
				&& (keyCode == code));
		}
		bool isKeyPressedWithShiftEvent() const 
		{
			return (inputType == TYPE_KEYBOARD
				&& (inputState != INPUT_STATE_END && inputState != INPUT_STATE_NONE)
				&& ((mod & KMOD_LSHIFT) || (mod & KMOD_RSHIFT)));
		}
		bool isKeyPressedWithShiftEvent(KeyCode code) const
		{
			return (isKeyPressedWithShiftEvent() &&	(keyCode == code));
		}
		bool isKeyPressedWithCtrlEvent() const 
		{
			return (inputType == TYPE_KEYBOARD
				&& (inputState != INPUT_STATE_END && inputState != INPUT_STATE_NONE)
				&& ((mod & KMOD_LCTRL) || (mod & KMOD_RCTRL)));
		}
		bool isKeyPressedWithCtrlEvent(KeyCode code) const
		{
			return (isKeyPressedWithCtrlEvent() &&	(keyCode == code));
		}
        bool isTouchEvent() const
        {
            return (inputType == TYPE_TOUCH);
        }
        bool isGamepadEvent() const;
        bool isScreenPositionEvent() const
        {
			// IDLE and DELTA are not apart of isMouseEvent because they are internal only, but need to account for them
			return isMouseEvent() || isTouchEvent() || inputType == TYPE_MOUSEIDLE || inputType == TYPE_MOUSEDELTA;
        }
		bool isDPadEvent() const
		{
			return isGamepadEvent() && 
				(keyCode == SDLK_GAMEPAD_DPADDOWN || keyCode == SDLK_GAMEPAD_DPADUP || keyCode == SDLK_GAMEPAD_DPADLEFT || keyCode == SDLK_GAMEPAD_DPADRIGHT);
		}

		bool isTextCharacterKey() const;
		bool isCarriageReturnKey() const;
		bool isClearKey() const;
		bool isBackspaceKey() const;
		bool isDeleteKey() const;
		bool isEscapeKey() const;
		bool isUpArrowKey() const;
		bool isDownArrowKey() const;
		bool isLeftArrowKey() const;
		bool isRightArrowKey() const;
		bool isNavigationKey() const;
		bool isCtrlEvent() const;
		bool isAltEvent() const;
	};
}	// namespace RBX
