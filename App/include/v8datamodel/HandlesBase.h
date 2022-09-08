/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Adornment.h"
#include "V8DataModel/EventReplicator.h"
#include "GfxBase/IAdornable.h"
#include "AppDraw/HandleType.h"

namespace RBX
{
	extern const char* const sHandlesBase;

	class HandlesBase
		: public DescribedNonCreatable<HandlesBase, PartAdornment, sHandlesBase>
	{
	private:
		typedef DescribedNonCreatable<HandlesBase, PartAdornment, sHandlesBase> Super;
	public:
		HandlesBase(const char* name);

		virtual RBX::HandleType getHandleType() const { return RBX::HANDLE_RESIZE; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		///*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);  // must implement in dervied
		/*override*/ void onAncestorChanged(const AncestorChanged& event);


		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*override*/ bool canProcessMeAndDescendants() const;
		

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render3dAdorn(Adorn* adorn);

	protected:
		virtual void setServerGuiObject();
		virtual int getHandlesNormalIdMask() const { return 0; }

		bool findTargetHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& hitNormalId);
		bool getDistanceFromHandle(const shared_ptr<InputObject>& inputObject, NormalId localNormalId, const Vector3& hitPointWorld, float& distance);
		bool getFacePosFromHandle(const shared_ptr<InputObject>& inputObject, NormalId faceId, const Vector3& hitPointWorld, Vector2& relativePos, Vector2& absolutePos);
		bool getAngleRadiusFromHandle(const shared_ptr<InputObject>& inputObject, NormalId faceId, const Vector3& hitPointWorld, float& angle, float& radius, float& absangle, float& absradius);
		
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender2d() const {return shouldRender3dAdorn();}

		struct MouseDownCaptureInfo
		{
			CoordinateFrame partLocation;
			Vector3 		hitPointWorld;
			NormalId		hitNormalId;
			MouseDownCaptureInfo(const CoordinateFrame& partLocation, const Vector3& hitPointWorld, NormalId hitNormalId)
				: partLocation(partLocation)
				, hitPointWorld(hitPointWorld)
				, hitNormalId(hitNormalId)
			{}
		};

		NormalId mouseOver;
		shared_ptr<MouseDownCaptureInfo> mouseDownCaptureInfo;
	private:
		bool serverGuiObject;
	};

}


