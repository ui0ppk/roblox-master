#pragma once
#include <string>
#include "Util/BrickColor.h"
#include "Util/TextureId.h"
#include "Gui/ProfanityFilter.h"
#include "Gui/GuiDraw.h"
#include "Util/ContentFilter.h"
#include "V8DataModel/GuiObject.h"

namespace RBX
{
	class GuiImageMixin
	{
	public:
		GuiImageMixin()
			: imageTransparency(0)
			, imageColor(Color3::white())
			, imageScale(GuiObject::SCALE_STRETCH)
			, sliceCenter(Rect2D())
		{
		}

		TextureId getImage() const { return image;} 
		
		Vector2 getImageRectOffset() const { return imageRectOffset; }
		
		Vector2 getImageRectSize() const { return imageRectSize; }

		float getImageTransparency() const { return imageTransparency; }

		Color3 getImageColor3() const { return imageColor; }

		Rect2D getSliceCenter() const { return sliceCenter; }

		GuiObject::ImageScale getImageScale() const { return imageScale; }

	protected:
		TextureId image;
        float imageTransparency;
		Color3 imageColor;
		Vector2 imageRectOffset;
		Vector2 imageRectSize;
		GuiDrawImage guiImageDraw;
		Rect2D sliceCenter;
		GuiObject::ImageScale imageScale;
	};

	#define DECLARE_GUI_IMAGE_MIXIN(Class)		    \
		void setImage(TextureId value);		        \
		void setImageRectOffset(Vector2 value);		\
		void setImageRectSize(Vector2 value);       \
		void setImageTransparency(float value);		\
		void setImageColor3(Color3 value);			\
		void setSliceCenter(Rect2D value);			\
		void setImageScale(ImageScale value);		\
		void renderStretched(Adorn* adorn);			\
		void renderSliced(Adorn* adorn);			\
		void renderImage(Adorn* adorn);

	
	#define IMPLEMENT_GUI_IMAGE_MIXIN(Class) \
		static const Reflection::PropDescriptor<Class, TextureId> prop_Image("Image", category_Image, &Class::getImage, &Class::setImage); \
		static const Reflection::PropDescriptor<Class, Vector2> prop_ImageRectOffset("ImageRectOffset", category_Image, &Class::getImageRectOffset, &Class::setImageRectOffset); \
		static const Reflection::PropDescriptor<Class, Vector2> prop_ImageRectSize("ImageRectSize", category_Image, &Class::getImageRectSize, &Class::setImageRectSize); \
		static const Reflection::PropDescriptor<Class, float> prop_ImageTransparency("ImageTransparency", category_Image, &Class::getImageTransparency, &Class::setImageTransparency); \
		static const Reflection::PropDescriptor<Class, Color3> prop_ImageColor3("ImageColor3", category_Image, &Class::getImageColor3, &Class::setImageColor3);		\
		static const Reflection::PropDescriptor<Class, Rect2D> prop_SliceCenter("SliceCenter", category_Image, &Class::getSliceCenter, &Class::setSliceCenter); \
		static const Reflection::EnumPropDescriptor<Class, GuiObject::ImageScale> prop_ImageScale("ScaleType", category_Image, &Class::getImageScale, &Class::setImageScale); \
		void Class::setImage(TextureId value)				\
		{													\
			if(image != value){								\
				image = value;								\
				raisePropertyChanged(prop_Image);			\
			}												\
		}													\
		void Class::setImageRectOffset(Vector2 value)		\
		{													\
			if(imageRectOffset != value){					\
				Rect2D offsetSliceCenter = Rect2D::xyxy(sliceCenter.x0y0() + value, sliceCenter.x1y1() + value);									\
				Rect2D imageRect = Rect2D::xywh(value, imageRectSize);																				\
				if (sliceCenter != Rect2D::xywh(0,0,0,0) && !imageRect.contains(offsetSliceCenter))													\
				{																																	\
					RBX::StandardOut::singleton()->printf(MESSAGE_WARNING,"SliceCenter ((%f,%f), (%f,%f)) is outside the bounds of imageOffset ((%f,%f), (%f,%f)).", offsetSliceCenter.x0(),offsetSliceCenter.y0(),offsetSliceCenter.x1(), offsetSliceCenter.y1(), imageRect.x0(),imageRect.y0(),imageRect.x1(), imageRect.y1()); \
				}																																	\
				imageRectOffset = value;					\
				raisePropertyChanged(prop_ImageRectOffset);	\
			}												\
		}													\
		void Class::setImageRectSize(Vector2 value)			\
		{													\
			if(imageRectSize != value){						\
				Rect2D offsetSliceCenter = Rect2D::xyxy(sliceCenter.x0y0() + imageRectOffset, sliceCenter.x1y1() + imageRectOffset);				\
				Rect2D imageRect = Rect2D::xywh(imageRectOffset, value);																				\
				if (sliceCenter != Rect2D::xywh(0,0,0,0) && !imageRect.contains(offsetSliceCenter))													\
				{																																	\
					RBX::StandardOut::singleton()->printf(MESSAGE_WARNING,"SliceCenter ((%f,%f), (%f,%f)) is outside the bounds of imageOffset ((%f,%f), (%f,%f))", offsetSliceCenter.x0(),offsetSliceCenter.y0(),offsetSliceCenter.x1(), offsetSliceCenter.y1(), imageRect.x0(),imageRect.y0(),imageRect.x1(), imageRect.y1()); \
					return;																															\
				}																																	\
				imageRectSize = value;						\
				raisePropertyChanged(prop_ImageRectSize);	\
			}												\
		}                                                   \
		void Class::setImageTransparency(float value)		\
		{													\
		    value = G3D::clamp(value, 0, 1);                \
		                                                    \
			if(imageTransparency != value){					\
				imageTransparency = value;					\
				raisePropertyChanged(prop_ImageTransparency); 	\
			}												\
		}													\
		void Class::setImageColor3(Color3 value)			\
		{													\
			if(imageColor != value){						\
				imageColor = value;							\
				raisePropertyChanged(prop_ImageColor3);		\
			}												\
		}													\
		void Class::setSliceCenter(Rect2D value)			\
		{													\
			if(sliceCenter != value)						\
			{												\
				Rect2D offsetSliceCenter = Rect2D::xyxy(value.x0y0() + imageRectOffset, value.x1y1() + imageRectOffset);							\
				Rect2D imageRect = Rect2D::xywh(imageRectOffset, imageRectSize);																	\
				if (imageRect != Rect2D::xywh(0,0,0,0) && !imageRect.contains(offsetSliceCenter))													\
				{																																	\
					RBX::StandardOut::singleton()->printf(MESSAGE_WARNING,"SliceCenter ((%f,%f), (%f,%f)) is outside the bounds of imageOffset ((%f,%f), (%f,%f))", offsetSliceCenter.x0(),offsetSliceCenter.y0(),offsetSliceCenter.x1(), offsetSliceCenter.y1(), imageRect.x0(),imageRect.y0(),imageRect.x1(), imageRect.y1()); \
				}																																	\
				sliceCenter = value;						\
				raisePropertyChanged(prop_SliceCenter);		\
			}												\
		}													\
		void Class::setImageScale(ImageScale value)			\
		{													\
			if(imageScale != value){						\
				imageScale = value;							\
				raisePropertyChanged(prop_ImageScale);		\
			}												\
		}													\
															\
		void Class::renderStretched(Adorn* adorn)			\
		{													\
			Vector2 imageSize;								\
			if (guiImageDraw.setImage(adorn, image, GuiDrawImage::NORMAL, &imageSize, this, ".Image"))	\
			{																			\
				Vector2 texul, texbr;													\
				guiImageDraw.computeUV(texul, texbr, imageRectOffset, imageRectSize, imageSize);	\
																									\
				Color4 color = Color4(getImageColor3(), 1 - imageTransparency);						\
																									\
				GuiObject* clippingObject = firstAncestorClipping();								\
				if( clippingObject == NULL || !absoluteRotation.empty())							\
					guiImageDraw.render2d(adorn, true, getRect2D(), texul, texbr, color, absoluteRotation, Gui::NOTHING, false);	\
				else																												\
					guiImageDraw.render2d(adorn, true, getRect2D(), texul, texbr, color, clippingObject->getClippedRect(), Gui::NOTHING, false);	\
			}																																		\
		}																																			\
																																					\
		void Class::renderSliced(Adorn* adorn)																										\
		{																																			\
			TextureId selectImage = getImage();																										\
																																					\
			if(guiImageDraw.setImage(adorn, selectImage, GuiDrawImage::NORMAL, NULL, this, ".Image"))																			\
			{																																		\
                Color4 rectColor = Color4(getImageColor3(), 1.0 - getImageTransparency());                                                          \
				Rect2D imageRectTextureOffset = Rect2D::xywh(imageRectOffset, imageRectSize);														\
				if (imageRectTextureOffset.width() > 0.0f && imageRectTextureOffset.height() > 0.0f)												\
				{																																											\
					render2dScale9Impl2(adorn, selectImage, guiImageDraw, sliceCenter, firstAncestorClipping(), rectColor, NULL, &imageRectTextureOffset);                                         \
				}																																											\
				else																																										\
				{																																											\
					render2dScale9Impl2(adorn, selectImage, guiImageDraw, sliceCenter, firstAncestorClipping(), rectColor);                                                                        \
				}																																											\
			}																																					\
		}																																						\
																																								\
		void Class::renderImage(Adorn* adorn)																													\
		{																																						\
			switch (imageScale)																																	\
			{																																					\
				case GuiObject::SCALE_STRETCH:																													\
					{																																			\
						renderStretched(adorn);																													\
						break;																																	\
					}																																			\
				case GuiObject::SCALE_SLICED:																													\
					{																																			\
						renderSliced(adorn);																													\
						break;																																	\
					}																																			\
				default:																																		\
					break;																																		\
			}																																					\
																																								\
			renderStudioSelectionBox(adorn);																													\
		}


}	// namespace RBX												
	
