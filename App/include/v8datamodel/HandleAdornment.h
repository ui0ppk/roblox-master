/**
 * HandleAdornment.h
 * Copyright (c) 2015 ROBLOX Corp. All Rights Reserved.
 * Created by Tyler Berg on 3/24/2015
 */

#pragma once

#include "V8DataModel/Adornment.h"
#include "GfxBase/IAdornable.h"
#include "AppDraw/Draw.h"

namespace RBX
{
	extern const char* const sHandleAdornment;

	class HandleAdornment
		: public DescribedNonCreatable<HandleAdornment, PVAdornment, sHandleAdornment>
	{
	public:

		typedef DescribedNonCreatable<HandleAdornment, PVAdornment, sHandleAdornment> Super;

		HandleAdornment(const char* name);

		static Reflection::PropDescriptor<HandleAdornment, Vector3> prop_sizeRelativeOffset;
		static Reflection::PropDescriptor<HandleAdornment, CoordinateFrame> prop_adornCFrame;
		static Reflection::PropDescriptor<HandleAdornment, int> prop_adornZIndex;
		static Reflection::PropDescriptor<HandleAdornment, bool> prop_alwaysOnTop;

		Vector3 getOffset() const { return sizeRelativeOffset; }
		void setOffset(Vector3 value)
		{
			if (sizeRelativeOffset != value)
			{
				sizeRelativeOffset = value;
				raisePropertyChanged(prop_sizeRelativeOffset);
			}
		}

		CoordinateFrame getCFrame() const { return coordinateFrame; }
		void setCFrame(CoordinateFrame value)
		{
			if (coordinateFrame != value)
			{
				coordinateFrame = value;
				raisePropertyChanged(prop_adornCFrame);
			}
		}

		int getZIndex() const { return zIndex; }
		void setZIndex(int value);

		bool getAlwaysOnTop() const { return alwaysOnTop; }
		void setAlwaysOnTop(bool value)
		{
			if (alwaysOnTop != value)
			{
				alwaysOnTop = value;
				raisePropertyChanged(prop_alwaysOnTop);
			}
		}

		virtual void render3dAdorn(Adorn* adorn) = 0;

		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject) = 0;
		virtual GuiResponse process(const shared_ptr<InputObject>& event);

		virtual CoordinateFrame getWorldCoordinateFrame() const;

		rbx::remote_signal<void()>	mouseEnterSignal;
		rbx::remote_signal<void()>	mouseLeaveSignal;
		rbx::remote_signal<void()>	mouseButton1DownSignal;
		rbx::remote_signal<void()>	mouseButton1UpSignal;

	protected:
		Vector3			sizeRelativeOffset;
		CoordinateFrame coordinateFrame;
		int				zIndex;
		bool			alwaysOnTop;

		bool			mouseOver;

	};
	
	extern const char* const sBoxHandleAdornment;

	class BoxHandleAdornment
		: public DescribedCreatable<BoxHandleAdornment, HandleAdornment, sBoxHandleAdornment>
	{
	public:
		BoxHandleAdornment();
				
		Vector3 getSize() const { return boxSize; }
		void setSize(Vector3 value) { boxSize = value; }

		virtual void render3dAdorn(Adorn* adorn);
		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject);
	private:
		Vector3 boxSize;
	};

	extern const char* const sConeHandleAdornment;

	class ConeHandleAdornment
		: public DescribedCreatable<ConeHandleAdornment, HandleAdornment, sConeHandleAdornment>
	{
	public:
		ConeHandleAdornment();
		
		float getRadius() const { return radius; }
		void setRadius(float value) { radius = value; }

		virtual CoordinateFrame getWorldCoordinateFrame();

		float getHeight() const { return height; }
		void setHeight(float value) { height = value; }
		
		virtual void render3dAdorn(Adorn* adorn);
		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject);
	private:
		float radius;
		float height;
	};

	extern const char* const sCylinderHandleAdornment;

	class CylinderHandleAdornment
		: public DescribedCreatable<CylinderHandleAdornment, HandleAdornment, sCylinderHandleAdornment>
	{
	public:
		CylinderHandleAdornment();

		float getRadius() const { return radius; }
		void setRadius(float value) { radius = value; }

		virtual CoordinateFrame getWorldCoordinateFrame();

		float getHeight() const { return height; }
		void setHeight(float value) { height = value; }

		virtual void render3dAdorn(Adorn* adorn);
		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject);
	private:
		float radius;
		float height;
	};

	extern const char* const sSphereHandleAdornment;

	class SphereHandleAdornment
		: public DescribedCreatable<SphereHandleAdornment, HandleAdornment, sSphereHandleAdornment>
	{
	public:
		SphereHandleAdornment();

		float getRadius() const { return radius; }
		void setRadius(float value) { radius = value; }

		virtual void render3dAdorn(Adorn* adorn);
		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject);
	private:
		float radius;
	};

	extern const char* const sLineHandleAdornment;

	class LineHandleAdornment
		: public DescribedCreatable<LineHandleAdornment, HandleAdornment, sLineHandleAdornment>
	{
	public:
		LineHandleAdornment();

		float getLength() const { return length; }
		void setLength(float value) { length = value; }

		float getThickness() const { return thickness; }
		void setThickness(float value) { thickness = value; }

		virtual void render3dAdorn(Adorn* adorn);
		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject);
	private:
		float length;
		float thickness;
	};

	extern const char* const sImageHandleAdornment;

	class ImageHandleAdornment
		: public DescribedCreatable<ImageHandleAdornment, HandleAdornment, sImageHandleAdornment>
	{
	public:
		ImageHandleAdornment();

		Vector2 getSize() const { return size; }
		void setSize(Vector2 value) { size = value; }

		TextureId getImage() const { return image; }
		void setImage(TextureId value) { image = value; }

		virtual void render3dAdorn(Adorn* adorn);
		virtual bool isCollidingWithHandle(const shared_ptr<InputObject>& inputObject);
	private:
		Vector2 size;
		TextureId image;
	};

}