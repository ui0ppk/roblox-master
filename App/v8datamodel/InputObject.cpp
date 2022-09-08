//
//  InputObject.cpp
//  App
//
//  Created by Ben Tkacheff on 3/6/13.
//
//
#include "stdafx.h"

#include "boost/lexical_cast.hpp"

#include "V8DataModel/InputObject.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/GamepadService.h"

#include "Reflection/Type.h"
#include "Reflection/EnumConverter.h"

namespace RBX {    

	const char* const sInputObject = "InputObject";
    
	template<>
	bool StringConverter<InputObject>::convertToValue(const std::string& text, InputObject& value)
	{
		return false;
	}
    
    namespace Reflection {
        
        template<>
        EnumDesc<InputObject::UserInputType>::EnumDesc()
        :EnumDescriptor("UserInputType")
        {
            addPair(InputObject::TYPE_MOUSEBUTTON1, "MouseButton1");
            addPair(InputObject::TYPE_MOUSEBUTTON2, "MouseButton2");
            addPair(InputObject::TYPE_MOUSEBUTTON3, "MouseButton3");
			addPair(InputObject::TYPE_MOUSEWHEEL, "MouseWheel");
            addPair(InputObject::TYPE_MOUSEMOVEMENT, "MouseMovement");
            addPair(InputObject::TYPE_TOUCH, "Touch");
            addPair(InputObject::TYPE_KEYBOARD, "Keyboard");
			addPair(InputObject::TYPE_FOCUS, "Focus");
            addPair(InputObject::TYPE_ACCELEROMETER, "Accelerometer");
            addPair(InputObject::TYPE_GYRO, "Gyro");
			addPair(InputObject::TYPE_GAMEPAD1, "Gamepad1");
			addPair(InputObject::TYPE_GAMEPAD2, "Gamepad2");
			addPair(InputObject::TYPE_GAMEPAD3, "Gamepad3");
			addPair(InputObject::TYPE_GAMEPAD4, "Gamepad4");
			addPair(InputObject::TYPE_GAMEPAD5, "Gamepad5");
			addPair(InputObject::TYPE_GAMEPAD6, "Gamepad6");
			addPair(InputObject::TYPE_GAMEPAD7, "Gamepad7");
			addPair(InputObject::TYPE_GAMEPAD8, "Gamepad8");
			addPair(InputObject::TYPE_TEXTINPUT, "TextInput");
            addPair(InputObject::TYPE_NONE, "None");
        }
        
        template<>
        EnumDesc<InputObject::UserInputState>::EnumDesc()
        :EnumDescriptor("UserInputState")
        {
            addPair(InputObject::INPUT_STATE_BEGIN, "Begin");
            addPair(InputObject::INPUT_STATE_CHANGE, "Change");
            addPair(InputObject::INPUT_STATE_END, "End");
			addPair(InputObject::INPUT_STATE_CANCEL, "Cancel");
            addPair(InputObject::INPUT_STATE_NONE, "None");
        }
        
        template<>
        InputObject::UserInputType& Variant::convert<InputObject::UserInputType>(void)
        {
            return genericConvert<InputObject::UserInputType>();
        }
        
        template<>
        InputObject::UserInputState& Variant::convert<InputObject::UserInputState>(void)
        {
            return genericConvert<InputObject::UserInputState>();
        }

		template<>
		const Type& Type::getSingleton<shared_ptr<InputObject> >()
		{
			static TType<shared_ptr<InputObject> > type("InputObject");
			return type;
		}
    } // namespace Reflection
    
    template<>
    bool StringConverter<InputObject::UserInputType>::convertToValue(const std::string& text, InputObject::UserInputType& value)
    {
        return Reflection::EnumDesc<InputObject::UserInputType>::singleton().convertToValue(text.c_str(),value);
    }
    
    template<>
    bool StringConverter<InputObject::UserInputState>::convertToValue(const std::string& text, InputObject::UserInputState& value)
    {
        return Reflection::EnumDesc<InputObject::UserInputState>::singleton().convertToValue(text.c_str(),value);
    }

    REFLECTION_BEGIN();
	static const Reflection::EnumPropDescriptor<InputObject, InputObject::UserInputType> prop_userInputType("UserInputType", category_Data, &InputObject::getUserInputType, &InputObject::setInputType, Reflection::PropertyDescriptor::SCRIPTING);
	static const Reflection::EnumPropDescriptor<InputObject, InputObject::UserInputState> prop_userInputState("UserInputState", category_State, &InputObject::getUserInputState, &InputObject::setInputState, Reflection::PropertyDescriptor::SCRIPTING);
	static const Reflection::PropDescriptor<InputObject, Vector3> prop_Position("Position", category_Data, &InputObject::getPosition, &InputObject::setPosition, Reflection::PropertyDescriptor::SCRIPTING);
	static const Reflection::PropDescriptor<InputObject, Vector3> prop_inputDelta("Delta", category_Data, &InputObject::getDelta, &InputObject::setDelta, Reflection::PropertyDescriptor::SCRIPTING);
	static const Reflection::EnumPropDescriptor<InputObject, KeyCode> prop_KeyCode("KeyCode", category_Data, &InputObject::getKeyCode, &InputObject::setKeyCode, Reflection::PropertyDescriptor::SCRIPTING);
    // todo: add scanCode, typedKey, and a way to query mod codes
	REFLECTION_END();

	InputObject::InputObject()
		: inputType(TYPE_NONE)
		, inputState(INPUT_STATE_NONE)
		, position(0,0,0)
		, keyCode(SDLK_UNKNOWN)
		, mod(KMOD_NONE)
		, modifiedKey(SDLK_UNKNOWN)
		, modCodes(0)
		, inputText("")
		, scanCode(SDL_SCANCODE_UNKNOWN)
		, sourceInputType(TYPE_NONE)

	{
		setName(sInputObject);
	}

	InputObject::InputObject(const InputObject &source)
	{
		inputType = source.inputType;
		inputState = source.inputState;
		position = source.position;
		keyCode = source.keyCode;

		mod = source.mod;
		modifiedKey = source.modifiedKey;

		weakWorkspace = source.weakWorkspace;
	}

	InputObject::InputObject(UserInputType inputType, UserInputState inputState, G3D::Vector3 newPosition, G3D::Vector3 newDelta, DataModel* dm)
		: inputType(inputType)
		, inputState(inputState)
		, position(newPosition)
		, keyCode(SDLK_UNKNOWN)
		, mod(KMOD_NONE)
		, modifiedKey(SDLK_UNKNOWN)
		, delta(newDelta)
		, modCodes(0)
		, inputText("")
		, scanCode(SDL_SCANCODE_UNKNOWN)
		, weakWorkspace(weak_from(RBX::ServiceProvider::find<Workspace>(dm)))
		, sourceInputType(inputType)
	{
		setName(sInputObject);
	}

	InputObject::InputObject(UserInputType inputType, UserInputState inputState, G3D::Vector3 newPosition, KeyCode newKeyCode, DataModel* dm)
		: inputType(inputType)
		, inputState(inputState)
		, position(newPosition)
		, keyCode(newKeyCode)
		, mod(KMOD_NONE)
		, modifiedKey(SDLK_UNKNOWN)
		, modCodes(0)
		, inputText("")
		, scanCode(SDL_SCANCODE_UNKNOWN)
		, weakWorkspace(weak_from(RBX::ServiceProvider::find<Workspace>(dm)))
		, sourceInputType(inputType)
	{
		setName(sInputObject);
	}

	InputObject::InputObject(UserInputType inputType, UserInputState inputState, DataModel* dm)
		: inputType(inputType)
		, inputState(inputState)
		, position(0,0,0)
		, keyCode(SDLK_UNKNOWN)
		, mod(KMOD_NONE)
		, modifiedKey(SDLK_UNKNOWN)
		, modCodes(0)
		, inputText("")
		, scanCode(SDL_SCANCODE_UNKNOWN)
		, weakWorkspace(weak_from(RBX::ServiceProvider::find<Workspace>(dm)))
		, sourceInputType(inputType)
	{
		setName(sInputObject);
	}

	InputObject::InputObject(UserInputType inputType, UserInputState inputState, KeyCode k, ModCode m, char key, DataModel* dm)
		: inputType(inputType)
		, inputState(inputState)
		, position(0,0,0)
		, keyCode(k)
		, mod(m)
		, modifiedKey(key)
		, modCodes(0)
		, inputText("")
		, scanCode(SDL_SCANCODE_UNKNOWN)
		, weakWorkspace(weak_from(RBX::ServiceProvider::find<Workspace>(dm)))
		, sourceInputType(inputType)
	{
		setName(sInputObject);
	}

	InputObject::InputObject(UserInputType inputType, UserInputState inputState, KeyCode k, unsigned int newModCodes, SDL_Scancode newScanCode, std::string newInputText, DataModel* dm)
		: inputType(inputType)
		, inputState(inputState)
		, position(0,0,0)
		, keyCode(k)
		, modCodes(newModCodes)
		, scanCode(newScanCode)
		, inputText(newInputText)
		, mod(KMOD_NONE)
		, modifiedKey(newInputText[0])
		, weakWorkspace(weak_from(RBX::ServiceProvider::find<Workspace>(dm)))
		, sourceInputType(inputType)
	{
		setName(sInputObject);
	}

	void InputObject::setInputType(UserInputType newInputType)
	{
		if(inputType != newInputType)
		{
			inputType = newInputType;
			raisePropertyChanged(prop_userInputType);
		}
	}

	void InputObject::setInputState(UserInputState newInputState)
	{
		if(inputState != newInputState)
		{
			inputState = newInputState;
			raisePropertyChanged(prop_userInputState);
		}
	}

	void InputObject::setPosition(G3D::Vector3 newPosition)
	{
		if(position != newPosition)
		{
			position = newPosition;
			raisePropertyChanged(prop_Position);
		}
	}

	void InputObject::setDelta(G3D::Vector3 newDelta)
	{
		if (delta != newDelta)
		{
			delta = newDelta;
			raisePropertyChanged(prop_inputDelta);
		}
	}

	void InputObject::setKeyCode(KeyCode newKeyCode)
	{
		if(keyCode != newKeyCode)
		{
			keyCode = newKeyCode;
			raisePropertyChanged(prop_KeyCode);
		}
	}

	void InputObject::setScanCode(SDL_Scancode newScanCode)
	{
		if(scanCode != newScanCode)
		{
			scanCode = newScanCode;
			//raisePropertyChanged(prop_KeyCode);
		}
	}

	void InputObject::setInputText(std::string newInputText)
	{
		if(inputText != newInputText)
		{
			inputText = newInputText;
			//raisePropertyChanged(prop_InputText);
		}
	}

	void InputObject::setModCodes(unsigned int newModCodes)
	{
		if(modCodes != newModCodes)
		{
			modCodes = newModCodes;
			//raisePropertyChanged(prop_KeyCode);
		}
	}

	Vector4 InputObject::getGuiInset() const
	{
		Vector4 guiInset = Vector4::zero();
		if (shared_ptr<RBX::Workspace> workspace = weakWorkspace.lock())
		{
			if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(workspace.get()))
			{
				guiInset = guiService->getGlobalGuiInset();
			}
		}

		return guiInset;
	}

	G3D::Vector2int16 InputObject::getWindowSize() const
	{
		if (shared_ptr<RBX::Workspace> workspace = weakWorkspace.lock())
		{
			if (Camera* camera = workspace->getCamera())
			{
				return camera->getViewport();
			}
		}

		return Vector2int16(0,0);
	}

	G3D::Vector3 InputObject::getRawPosition() const
	{
		return position;
	}
	G3D::Vector3 InputObject::getPosition() const
	{
		const Vector4 guiInset = getGuiInset();
        return isScreenPositionEvent() ? Vector3(position.x - guiInset.x, position.y - guiInset.y, position.z) : position;
	}
	G3D::Vector2 InputObject::get2DPosition() const
	{
		return Vector2(position.x, position.y);
	}

	bool InputObject::isPublicEvent()
	{
		// these are two old events we only keep around for workspace tool processing, do not show user
		if (inputType == InputObject::TYPE_MOUSEIDLE ||
			inputType == InputObject::TYPE_MOUSEDELTA ||
			sourceInputType != inputType)
		{
			return false;
		}

		return true;
	}

	bool InputObject::isTextCharacterKey() const 
	{
		return modifiedKey != 0;
	}
	bool InputObject::isAltEvent() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return ((modCodes & KMOD_LALT) != 0) || ((modCodes & KMOD_RALT) != 0);
		}

		return ((mod & KMOD_LALT) != 0) || ((mod & KMOD_RALT) != 0);
	}
	bool InputObject::isCtrlEvent() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return ((modCodes & KMOD_LCTRL) != 0) || ((modCodes & KMOD_RCTRL) != 0);
		}

		return ((mod & KMOD_LCTRL) != 0) || ((mod & KMOD_RCTRL) != 0);
	}
	bool InputObject::isCarriageReturnKey() const
	{
		return ((keyCode == SDLK_RETURN) || (keyCode == SDLK_KP_ENTER));
	}

	bool InputObject::isDeleteKey() const
	{
		return (keyCode == SDLK_DELETE);
	}

	bool InputObject::isBackspaceKey() const
	{
		return (keyCode == SDLK_BACKSPACE);
	}

	bool InputObject::isClearKey() const
	{
		return (keyCode == SDLK_CLEAR);
	}

	bool InputObject::isEscapeKey() const
	{
		return (keyCode == SDLK_ESCAPE);
	}

	bool InputObject::isLeftArrowKey() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return scanCode == SDL_SCANCODE_LEFT;
		}

		return keyCode == SDLK_LEFT;
	}
	bool InputObject::isRightArrowKey() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return scanCode == SDL_SCANCODE_RIGHT;
		}
		return keyCode == SDLK_RIGHT;
	}
	bool InputObject::isUpArrowKey() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return scanCode == SDL_SCANCODE_UP;
		}
		return keyCode == SDLK_UP;
	}
	bool InputObject::isDownArrowKey() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return scanCode == SDL_SCANCODE_DOWN;
		}
		return keyCode == SDLK_DOWN;
	}

	bool InputObject::isNavigationKey() const
	{
		if (UserInputService::IsUsingNewKeyboardEvents())
		{
			return scanCode == SDL_SCANCODE_DOWN || scanCode == SDL_SCANCODE_LEFT || scanCode == SDL_SCANCODE_RIGHT || scanCode == SDL_SCANCODE_UP ||
				scanCode == SDL_SCANCODE_A || scanCode == SDL_SCANCODE_W || scanCode == SDL_SCANCODE_S || scanCode == SDL_SCANCODE_D;
		}

		return keyCode == SDLK_DOWN || keyCode == SDLK_LEFT || keyCode == SDLK_RIGHT || keyCode == SDLK_UP ||
			keyCode == SDLK_a || keyCode == SDLK_w || keyCode == SDLK_s || keyCode == SDLK_d;
	}

	bool InputObject::isGamepadEvent() const
	{
		return (GamepadService::getGamepadIntForEnum(inputType) != -1);
	}
} // namespace RBX