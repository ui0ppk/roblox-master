#pragma once

#include "Util/KeyCode.h"
#include "Util/G3DCore.h"
#include "GfxBase/TextureProxyBase.h"
#include "Util/ContentId.h"
#include "Util/TextureId.h"
#include "Util/Object.h"
#include "GfxBase/Adorn.h"

LOGGROUP(UserInputProfile)

namespace RBX {


	class Adorn;
	class NavKeys;

	// TODO: Rename HardwareDevice or something
	class RBXBaseClass UserInputBase
	{
	private:
		ContentId currentCursorId;
		TextureProxyBaseRef currentCursor;
		TextureProxyBaseRef fallbackCursor;
		rbx::signals::scoped_connection unbindResourceSignal;

        void onUnbindResourceSignal();

	protected:
		virtual Vector2 getCursorPosition() = 0;
		virtual TextureProxyBaseRef getGameCursor(Adorn* adorn);
		TextureProxyBaseRef getCurrentCursor() { return currentCursor; }
		TextureProxyBaseRef getFallbackCursor() { return fallbackCursor; }

	public:
		UserInputBase();
		~UserInputBase() {}

		rbx::signal<void()> cursorIdChangedSignal;

		// This function is purely intended for debugging and diagnostics
		Vector2 getCursorPositionForDebugging()
		{
			return getCursorPosition();
		}

		virtual void removeJobs() {}

		/////////////////////////////////////////////////////////////////////
		// Mouse Wrapping
		//
		virtual void centerCursor() = 0;

		/////////////////////////////////////////////////////////////////////
		// Real-time Key Handling
		//
		virtual bool keyDown(KeyCode code) const = 0;

		void getNavKeys(NavKeys& navKeys,const bool shouldSuppressNavKeys) const;
	
		bool altKeyDown() const		{return keyDown(SDLK_RALT) || keyDown(SDLK_LALT);}
		bool shiftKeyDown() const	{return keyDown(SDLK_RSHIFT) || keyDown(SDLK_LSHIFT);}
		bool ctrlKeyDown() const	{return keyDown(SDLK_RCTRL) || keyDown(SDLK_LCTRL);}
		
		// allows Gui Key buttons to "press" keys
        virtual void setKeyState(RBX::KeyCode code, RBX::ModCode modCode, char modifiedKey, bool isDown) = 0;

		/////////////////////////////////////////////////////////////////////
		// Cursor Handling
		//
		ContentId getCurrentCursorId() { return currentCursorId; }
		virtual bool setCursorId(RBX::Adorn *adorn, const RBX::TextureId& id);
		virtual void renderGameCursor(Adorn* adorn);
	};
} // namespace
