/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/HandlesBase.h"
#include "V8DataModel/EventReplicator.h"
#include "GfxBase/IAdornable.h"
#include "Util/Axes.h"

#include "AppDraw/HandleType.h"

namespace RBX
{
	extern const char* const sArcHandles;

	class ArcHandles
		: public DescribedCreatable<ArcHandles, HandlesBase, sArcHandles>
	{
	private:
		typedef DescribedCreatable<ArcHandles, HandlesBase, sArcHandles> Super;
	public:
		ArcHandles();

		rbx::remote_signal<void(RBX::Vector3::Axis)>				mouseEnterSignal;
		rbx::remote_signal<void(RBX::Vector3::Axis)>				mouseLeaveSignal;	
		rbx::remote_signal<void(RBX::Vector3::Axis,float,float)>	mouseDragSignal;
		rbx::remote_signal<void(RBX::Vector3::Axis)>				mouseButton1DownSignal;
		rbx::remote_signal<void(RBX::Vector3::Axis)>				mouseButton1UpSignal;	

		DECLARE_EVENT_REPLICATOR_SIG(ArcHandles,MouseEnter,		void(RBX::Vector3::Axis));
		DECLARE_EVENT_REPLICATOR_SIG(ArcHandles,MouseLeave,		void(RBX::Vector3::Axis));
		DECLARE_EVENT_REPLICATOR_SIG(ArcHandles,MouseDrag,		void(RBX::Vector3::Axis,float,float));
		DECLARE_EVENT_REPLICATOR_SIG(ArcHandles,MouseButton1Down,	void(RBX::Vector3::Axis));
		DECLARE_EVENT_REPLICATOR_SIG(ArcHandles,MouseButton1Up,	void(RBX::Vector3::Axis));
		
		void	setAxes(Axes value);
		Axes	getAxes() const { return axes; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);
		

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// HandlesBase
		/*override*/ RBX::HandleType getHandleType() const;

	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// HandlesBase
		/*override*/ int getHandlesNormalIdMask() const;
		/*override*/ void setServerGuiObject();

	private:
		Axes axes;

	};

}


