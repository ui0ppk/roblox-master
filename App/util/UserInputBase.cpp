#include "stdafx.h"

#include "Util/UserInputBase.h"
#include "rbx/Debug.h"
#include "Util/NavKeys.h"
#include "Util/Rect.h"

namespace RBX {


UserInputBase::UserInputBase() 
{
}

void UserInputBase::getNavKeys(NavKeys& navKeys, const bool shouldSuppressNavKeys) const 
{
	if (!shouldSuppressNavKeys)
	{
		navKeys.forward_arrow	= keyDown(SDLK_UP); 
		navKeys.backward_arrow	= keyDown(SDLK_DOWN); 
		navKeys.left_arrow		= keyDown(SDLK_LEFT); 
		navKeys.right_arrow		= keyDown(SDLK_RIGHT);

		navKeys.strafe_right_e	= keyDown(SDLK_e);
		navKeys.strafe_left_q	= keyDown(SDLK_q);
		navKeys.strafe_right_e	= keyDown(SDLK_e);
		navKeys.strafe_left_q	= keyDown(SDLK_q);
		navKeys.forward_asdw	= keyDown(SDLK_w); 
		navKeys.backward_asdw	= keyDown(SDLK_s); 
		navKeys.left_asdw		= keyDown(SDLK_a); 
		navKeys.right_asdw		= keyDown(SDLK_d); 
		navKeys.space			= keyDown(SDLK_SPACE); 
		navKeys.backspace		= keyDown(SDLK_BACKSPACE); 
		navKeys.shift			= keyDown(SDLK_LSHIFT) || keyDown(SDLK_RSHIFT);
	}
}

void UserInputBase::onUnbindResourceSignal()
{
    currentCursor.reset();
    fallbackCursor.reset();

	unbindResourceSignal.disconnect();
}

RBX::TextureProxyBaseRef UserInputBase::getGameCursor(Adorn* adorn)
{
	if (!currentCursor || currentCursor == fallbackCursor)
	{
		// lazy connect to signal
		if (!unbindResourceSignal.connected())
		{
			unbindResourceSignal = adorn->getUnbindResourcesSignal().connect(boost::bind(&UserInputBase::onUnbindResourceSignal, this));
		}

		bool waitingCurrentCursor;
		currentCursor = adorn->createTextureProxy(currentCursorId, waitingCurrentCursor, false, "Mouse Cursor");

		// always show a cursor!
		// blocking call to fallback cursor.
		if (!currentCursor)
		{
			if (!fallbackCursor)
			{
				bool waitingFallbackCursor;
				fallbackCursor = adorn->createTextureProxy(ContentId::fromAssets("Textures/ArrowCursor.png"), waitingFallbackCursor, true /*blocking*/);
			}
			currentCursor = fallbackCursor;
		}
	}

	return currentCursor;
}

bool UserInputBase::setCursorId(Adorn *adorn, const TextureId& id)
{
	if (currentCursorId != id)
	{
		currentCursorId = id;
		currentCursor.reset();
		cursorIdChangedSignal();
		return true;
	}

	return false;
}

void UserInputBase::renderGameCursor(Adorn* adorn)
{	
	RBX::TextureProxyBaseRef cursor = getGameCursor(adorn);
		
	if (cursor)
	{
		Rect2D rect = adorn->getTextureSize(cursor);

		// Center the texture on the cursor position
//		G3D::Rect2D rect = cursor->rect2DBounds();
		rect = rect - rect.center() + getCursorPosition();

		adorn->setTexture(0, cursor);
		adorn->rect2d(rect, G3D::Color4(1,1,1,1));
		adorn->setTexture(0, TextureProxyBaseRef());
	}
}

} // namespace
