/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Gui/GuiDraw.h"
#include "V8Kernel/Constants.h"
#include "V8DataModel/PVInstance.h"
#include "AppDraw/Draw.h"
#include "Util/Utilities.h"
#include "Util/Math.h"
#include "AppDraw/DrawPrimitives.h"

#include "GfxBase/Adorn.h"
#include "v8datamodel/contentprovider.h"

namespace RBX {

/////////////////////////////////////////////////////////////
//
// GuiDrawImage
//

void GuiDrawImage::tryCreateTextureProxy(Adorn *adorn, const std::string& contentString, const std::string& context, RBX::TextureProxyBaseRef& textureRef, bool& isWaiting)
{
	ContentId content(contentString);

	if ( (content.isAsset() && !RBX::ContentProvider::findAsset(content).empty()) || !content.isAsset())
	{
		textureRef = adorn->createTextureProxy(content, isWaiting, false, context);
	}
}

bool GuiDrawImage::setImage(Adorn *adorn, const TextureId& textureId, unsigned imageState, Vector2* outSize, Instance* contextInstance, const char* context)
{
	if (textureId != currentTexture) {
		std::string contextString = "";
		if (contextInstance != NULL)
		{
			contextString = contextInstance->getFullName() + context;
		}
        currentTexture = TextureId::nullTexture();

		// lazy connect to signal
		if(!unbindResourceSignalHint.connected())
		{
			unbindResourceSignalHint = adorn->getUnbindResourcesSignal().connect(boost::bind(&GuiDrawImage::OnUnbindResourceSignalHint, this));
		}

		if(textureId != loadingTexture){
			normal.reset();
			hover.reset();
			down.reset();
			disable.reset();
			selected.reset();
			selectedHover.reset();
			selectedDown.reset();

			size = Vector2(0, 0);
			loadingTexture = textureId;
		}

		if (textureId.isNull()){
			currentTexture = textureId;
		}
		else{
			bool waitingNormal = false;
			if(!normal) 
			{
				normal = adorn->createTextureProxy(textureId, waitingNormal, false, contextString);
			}
			if(textureId.isHttp() || imageState == NORMAL || textureId.isAssetId() || textureId.isNamedAsset()) {
				if (normal) {
					currentTexture = textureId;
				}
			}
			else {
				std::string id = textureId.toString();				// hack - thrash createTextureProxy for old gui stuff if not all four are present
				std::string base = id.substr(0, id.size() - 4);
				bool waitingHover = false;
				bool waitingDown = false;
				bool waitingDisable = false;
				bool waitingSelected = false;
				bool waitingSelectedHover = false;
				bool waitingSelectedDown = false;

				//TODO: eliminate this polling
				if(!hover && (imageState & HOVER)) 
				{
					tryCreateTextureProxy(adorn, base + "_ovr.png", contextString, hover, waitingHover);
				}
				if(!down && (imageState & DOWN))
				{
					tryCreateTextureProxy(adorn, base + "_dn.png", contextString, down, waitingDown);
				}
				if(!disable && (imageState & DISABLE)) 
				{
					tryCreateTextureProxy(adorn, base + "_ds.png", contextString, disable, waitingDisable);
				}
				if(!selected && (imageState & SELECTED)) 
				{
					tryCreateTextureProxy(adorn, base + "_sel.png", contextString, selected, waitingSelected);
				}
				if(!selectedHover && (imageState & SELECTED_HOVER)) 
				{
					tryCreateTextureProxy(adorn, base + "_sovr.png", contextString, selectedHover, waitingSelectedHover);
				}
				if(!selectedDown && (imageState & SELECTED_DOWN)) 
				{
					tryCreateTextureProxy(adorn, base + "_sdn.png", contextString, selectedDown, waitingSelectedDown);
				}

				if (normal && 
					(hover || !waitingHover) && 
					(down || !waitingDown) && 
					(disable || !waitingDisable) && 
					(selected || !waitingSelected) &&
					(selectedHover || !waitingSelectedHover) &&
					(selectedDown || !waitingSelectedDown)) {
					currentTexture = textureId;
				}
			}
		}
	}
	
	if (outSize && normal)
	{
		*outSize = normal->getOriginalSize();
	}
	
	return !!normal;
}

bool GuiDrawImage::setImageFromName(Adorn *adorn, const std::string& textureName, unsigned imageState, Instance* contextInstance, const char* context)
{
	std::string asset = "Textures/" + textureName + ".png";
	ContentId contentId = ContentId::fromAssets(asset.c_str());
	return setImage(adorn, contentId, imageState, NULL, contextInstance, context);
}


void GuiDrawImage::setImageSize(const Vector2& _size)
{
	size = _size;
}

Vector2 GuiDrawImage::getImageSize() const
{
	if (size.isZero() && normal)
	{
		RBX::Vector2 sz = normal->getOriginalSize();
		size.x = sz.x;
		size.y = sz.y;
	}
	return size;
}

void GuiDrawImage::draw(Adorn* adorn, const RBX::TextureProxyBaseRef& texture, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
    const Rect& clipRect, const Color4& behind, const Color4& inFront)
{
	Rect2D rect2D = rect.toRect2D();
	Rect2D clipRect2D = clipRect.toRect2D();

	if( clipRect2D.intersects(rect2D) )
	{
		if (texture) {
			adorn->setTexture(0, texture);
			adorn->rect2d(rect2D, texul, texbr, color, clipRect2D);
			adorn->setTexture(0, TextureProxyBaseRef());
		}
		else if (normal) {		// backup - use the normal texture
			RBX::Rect2D intersectRect = clipRect2D.intersect(rect2D);
			adorn->rect2d(intersectRect, texul, texbr, behind);
			draw(adorn, normal, rect, texul, texbr, color, clipRect, Color4::clear(), Color4::clear());
			adorn->rect2d(intersectRect, texul, texbr, inFront);
		}
	}
}

void GuiDrawImage::draw(Adorn* adorn, const RBX::TextureProxyBaseRef& texture, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
    const Rotation2D& rotation, const Color4& behind, const Color4& inFront)
{
	Rect2D rect2D = rect.toRect2D();

    if (texture) {
        adorn->setTexture(0, texture);
        adorn->rect2d(rect2D, texul, texbr, color, rotation);
        adorn->setTexture(0, TextureProxyBaseRef());
    }
    else if (normal) {		// backup - use the normal texture
        adorn->rect2d(rect2D, texul, texbr, behind);
        draw(adorn, normal, rect, texul, texbr, color, rotation, Color4::clear(), Color4::clear());
        adorn->rect2d(rect2D, texul, texbr, inFront);
	}
}

template <typename Modifier>
void GuiDrawImage::render2dImpl(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Modifier& modifier,
    Gui::WidgetState state, bool isSelected)
{
	if (!enabled) {
		draw(adorn, disable, rect, texul, texbr, color, modifier, Color4::clear(), Color4(1,1,1,0.5));	// disabled: draw gray over it
	}
	else if (!isSelected) {
		if (state == Gui::NOTHING) {
			draw(adorn, normal, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());	// no special state, no special outline
		}
		else if (state == Gui::HOVER || state == Gui::DOWN_AWAY) {
			draw(adorn, hover, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());
		}
		else if (state == Gui::DOWN_OVER) {
			draw(adorn, down, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());		// Depressed: draw blue
		}
	} else {
		if (state == Gui::NOTHING) {
			draw(adorn, selected, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());	// no special state, no special outline
		}
		else if (state == Gui::HOVER) {
			draw(adorn, hover, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());
		}
		else if (state == Gui::DOWN_AWAY) {			// hover/depressed+away: draw yellow
			draw(adorn, selectedHover, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());
		}
		else if (state == Gui::DOWN_OVER) {
			draw(adorn, selectedDown, rect, texul, texbr, color, modifier, Color4::clear(), Color4::clear());		// Depressed: draw blue
		}
	}
}

void GuiDrawImage::render2d(Adorn* adorn, bool enabled, const Rect& rect,
    Gui::WidgetState state, bool isSelected)
{
	render2dImpl(adorn, enabled, rect, Vector2(0, 0), Vector2(1, 1), Color3::white(), Rotation2D(), state, isSelected);
}

void GuiDrawImage::render2d(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rotation2D& rotation,
    Gui::WidgetState state, bool isSelected)
{
	render2dImpl(adorn, enabled, rect, texul, texbr, color, rotation, state, isSelected);
}

void GuiDrawImage::render2d(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rect& clipRect,
    Gui::WidgetState state, bool isSelected)
{
	render2dImpl(adorn, enabled, rect, texul, texbr, color, clipRect, state, isSelected);
}

void GuiDrawImage::OnUnbindResourceSignalHint()
{
	currentTexture = TextureId::nullTexture();
	normal.reset();
	hover.reset();
	down.reset();
	disable.reset();
	selected.reset();
	selectedHover.reset();
	selectedDown.reset();

	unbindResourceSignalHint.disconnect();
}

void GuiDrawImage::computeUV(Vector2& uvtl, Vector2& uvbr, const Vector2& imageRectOffset, const Vector2& imageRectSize, const Vector2& imageSize)
{
    if ((imageRectOffset.isZero() && imageRectSize.isZero()) || imageSize.isZero())
	{
		uvtl = Vector2(0, 0);
		uvbr = Vector2(1, 1);
	}
	else
	{
		uvtl = G3D::clamp(imageRectOffset / imageSize, Vector2(0, 0), Vector2(1, 1));
		uvbr = G3D::clamp((imageRectOffset + imageRectSize) / imageSize, Vector2(0, 0), Vector2(1, 1));
	}
}

} // namespace
