//
//  KeyCode.cpp
//  App
//
//  Created by Ben Tkacheff on 11/19/13.
//
//
#include "stdafx.h"

#include "Util/KeyCode.h"
#include "Reflection/EnumConverter.h"

namespace RBX {

namespace Reflection {
    
    template<>
    EnumDesc<KeyCode>::EnumDesc() : EnumDescriptor("KeyCode")
    {
        addPair(SDLK_UNKNOWN, "Unknown");
        addPair(SDLK_BACKSPACE, "Backspace");
        addPair(SDLK_TAB, "Tab");
        addPair(SDLK_CLEAR, "Clear");
        addPair(SDLK_RETURN, "Return");
        addPair(SDLK_PAUSE, "Pause");
        addPair(SDLK_ESCAPE, "Escape");
        addPair(SDLK_SPACE, "Space");
        addPair(SDLK_QUOTEDBL, "QuotedDouble");
        addPair(SDLK_HASH, "Hash");
        addPair(SDLK_DOLLAR, "Dollar");
        addPair(SDLK_PERCENT, "Percent");
        addPair(SDLK_AMPERSAND, "Ampersand");
        addPair(SDLK_QUOTE, "Quote");
        addPair(SDLK_LEFTPAREN, "LeftParenthesis");
        addPair(SDLK_RIGHTPAREN, "RightParenthesis");
        addPair(SDLK_ASTERISK, "Asterisk");
        addPair(SDLK_PLUS, "Plus");
        addPair(SDLK_COMMA, "Comma");
        addPair(SDLK_MINUS, "Minus");
        addPair(SDLK_PERIOD, "Period");
        addPair(SDLK_SLASH, "Slash");
        
        addPair(SDLK_0, "Zero");
        addPair(SDLK_1, "One");
        addPair(SDLK_2, "Two");
        addPair(SDLK_3, "Three");
        addPair(SDLK_4, "Four");
        addPair(SDLK_5, "Five");
        addPair(SDLK_6, "Six");
        addPair(SDLK_7, "Seven");
        addPair(SDLK_8, "Eight");
        addPair(SDLK_9, "Nine");
        
        addPair(SDLK_COLON, "Colon");
        addPair(SDLK_SEMICOLON, "Semicolon");
        addPair(SDLK_LESS, "LessThan");
        addPair(SDLK_EQUALS, "Equals");
        addPair(SDLK_GREATER, "GreaterThan");
        addPair(SDLK_QUESTION, "Question");
        addPair(SDLK_AT, "At");
        addPair(SDLK_LEFTBRACKET, "LeftBracket");
        addPair(SDLK_BACKSLASH, "BackSlash");
        addPair(SDLK_RIGHTBRACKET, "RightBracket");
        addPair(SDLK_CARET, "Caret");
        addPair(SDLK_UNDERSCORE, "Underscore");
        addPair(SDLK_BACKQUOTE, "Backquote");
        
        addPair(SDLK_a, "A");
        addPair(SDLK_b, "B");
        addPair(SDLK_c, "C");
        addPair(SDLK_d, "D");
        addPair(SDLK_e, "E");
        addPair(SDLK_f, "F");
        addPair(SDLK_g, "G");
        addPair(SDLK_h, "H");
        addPair(SDLK_i, "I");
        addPair(SDLK_j, "J");
        addPair(SDLK_k, "K");
        addPair(SDLK_l, "L");
        addPair(SDLK_m, "M");
        addPair(SDLK_n, "N");
        addPair(SDLK_o, "O");
        addPair(SDLK_p, "P");
        addPair(SDLK_q, "Q");
        addPair(SDLK_r, "R");
        addPair(SDLK_s, "S");
        addPair(SDLK_t, "T");
        addPair(SDLK_u, "U");
        addPair(SDLK_v, "V");
        addPair(SDLK_w, "W");
        addPair(SDLK_x, "X");
        addPair(SDLK_y, "Y");
        addPair(SDLK_z, "Z");
        
        addPair(SDLK_LEFTCURLY, "LeftCurly");
        addPair(SDLK_PIPE, "Pipe");
        addPair(SDLK_RIGHTCURLY, "RightCurly");
        addPair(SDLK_TILDE, "Tilde");
        addPair(SDLK_DELETE, "Delete");
        
        addPair(SDLK_KP0, "KeypadZero");
        addPair(SDLK_KP1, "KeypadOne");
        addPair(SDLK_KP2, "KeypadTwo");
        addPair(SDLK_KP3, "KeypadThree");
        addPair(SDLK_KP4, "KeypadFour");
        addPair(SDLK_KP5, "KeypadFive");
        addPair(SDLK_KP6, "KeypadSix");
        addPair(SDLK_KP7, "KeypadSeven");
        addPair(SDLK_KP8, "KeypadEight");
        addPair(SDLK_KP9, "KeypadNine");
        addPair(SDLK_KP_PERIOD, "KeypadPeriod");
        addPair(SDLK_KP_DIVIDE, "KeypadDivide");
        addPair(SDLK_KP_MULTIPLY, "KeypadMultiply");
        addPair(SDLK_KP_MINUS, "KeypadMinus");
        addPair(SDLK_KP_PLUS, "KeypadPlus");
        addPair(SDLK_KP_ENTER, "KeypadEnter");
        addPair(SDLK_KP_EQUALS, "KeypadEquals");
        
        addPair(SDLK_UP, "Up");
        addPair(SDLK_DOWN, "Down");
        addPair(SDLK_RIGHT, "Right");
        addPair(SDLK_LEFT, "Left");
        addPair(SDLK_INSERT, "Insert");
        addPair(SDLK_HOME, "Home");
        addPair(SDLK_END, "End");
        addPair(SDLK_PAGEUP, "PageUp");
        addPair(SDLK_PAGEDOWN, "PageDown");
        
        addPair(SDLK_LSHIFT, "LeftShift");
        addPair(SDLK_RSHIFT, "RightShift");
        addPair(SDLK_LMETA, "LeftMeta");
        addPair(SDLK_RMETA, "RightMeta");
        addPair(SDLK_LALT, "LeftAlt");
        addPair(SDLK_RALT, "RightAlt");
        addPair(SDLK_LCTRL, "LeftControl");
        addPair(SDLK_RCTRL, "RightControl");
        addPair(SDLK_CAPSLOCK, "CapsLock");
        addPair(SDLK_NUMLOCK, "NumLock");
        addPair(SDLK_SCROLLOCK, "ScrollLock");

		addPair(SDLK_LSUPER, "LeftSuper");
		addPair(SDLK_RSUPER, "RightSuper");
		addPair(SDLK_MODE, "Mode");
		addPair(SDLK_COMPOSE, "Compose");

		addPair(SDLK_HELP, "Help");
		addPair(SDLK_PRINT, "Print");
		addPair(SDLK_SYSREQ, "SysReq");
		addPair(SDLK_BREAK, "Break");
		addPair(SDLK_MENU, "Menu");
		addPair(SDLK_POWER, "Power");
		addPair(SDLK_EURO, "Euro");
		addPair(SDLK_UNDO, "Undo");

		addPair(SDLK_F1, "F1");
		addPair(SDLK_F2, "F2");
		addPair(SDLK_F3, "F3");
		addPair(SDLK_F4, "F4");
		addPair(SDLK_F5, "F5");
		addPair(SDLK_F6, "F6");
		addPair(SDLK_F7, "F7");
		addPair(SDLK_F8, "F8");
		addPair(SDLK_F9, "F9");
		addPair(SDLK_F10, "F10");
		addPair(SDLK_F11, "F11");
		addPair(SDLK_F12, "F12");
		addPair(SDLK_F13, "F13");
		addPair(SDLK_F14, "F14");
		addPair(SDLK_F15, "F15");

		const std::string worldString("World");
		for (int i = SDLK_WORLD_0; i <= SDLK_WORLD_95; ++i)
		{
			int num = i - SDLK_WORLD_0;

			std::stringstream ss;
			ss << worldString;
			ss << num;

			addPair((RBX::KeyCode)i, ss.str().c_str());
		}

		addPair(SDLK_GAMEPAD_BUTTONX, "ButtonX");
		addPair(SDLK_GAMEPAD_BUTTONY, "ButtonY");
		addPair(SDLK_GAMEPAD_BUTTONA, "ButtonA");
		addPair(SDLK_GAMEPAD_BUTTONB, "ButtonB");
		addPair(SDLK_GAMEPAD_BUTTONR1, "ButtonR1");
		addPair(SDLK_GAMEPAD_BUTTONL1, "ButtonL1");
		addPair(SDLK_GAMEPAD_BUTTONR2, "ButtonR2");
		addPair(SDLK_GAMEPAD_BUTTONL2, "ButtonL2");
		addPair(SDLK_GAMEPAD_BUTTONR3, "ButtonR3");
        addPair(SDLK_GAMEPAD_BUTTONL3, "ButtonL3");
		addPair(SDLK_GAMEPAD_BUTTONSTART, "ButtonStart");
		addPair(SDLK_GAMEPAD_BUTTONSELECT, "ButtonSelect");
		addPair(SDLK_GAMEPAD_DPADLEFT, "DPadLeft");
		addPair(SDLK_GAMEPAD_DPADRIGHT, "DPadRight");
		addPair(SDLK_GAMEPAD_DPADUP, "DPadUp");
		addPair(SDLK_GAMEPAD_DPADDOWN, "DPadDown");
		addPair(SDLK_GAMEPAD_THUMBSTICK1, "Thumbstick1");
		addPair(SDLK_GAMEPAD_THUMBSTICK2, "Thumbstick2");
    }
    
    template<>
    KeyCode& Variant::convert<RBX::KeyCode>(void)
    {
        return genericConvert<RBX::KeyCode>();
    }
    
} // namespace Reflection

template<>
bool StringConverter<RBX::KeyCode>::convertToValue(const std::string& text, RBX::KeyCode& value)
{
    return Reflection::EnumDesc<RBX::KeyCode>::singleton().convertToValue(text.c_str(),value);
}

} // namespace RBX