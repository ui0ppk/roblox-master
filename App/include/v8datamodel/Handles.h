/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/HandlesBase.h"
#include "V8DataModel/EventReplicator.h"
#include "GfxBase/IAdornable.h"
#include "Util/Faces.h"

#include "AppDraw/HandleType.h"

namespace RBX
{
	extern const char* const sHandles;

	class Handles
		: public DescribedCreatable<Handles, HandlesBase, sHandles>
	{
	private:
		typedef DescribedCreatable<Handles, HandlesBase, sHandles> Super;
	public:
		Handles();

		rbx::remote_signal<void(NormalId)>			mouseEnterSignal;
		rbx::remote_signal<void(NormalId)>			mouseLeaveSignal;	
		rbx::remote_signal<void(NormalId,float)>	mouseDragSignal;
		rbx::remote_signal<void(NormalId)>			mouseButton1DownSignal;
		rbx::remote_signal<void(NormalId)>			mouseButton1UpSignal;	

		DECLARE_EVENT_REPLICATOR_SIG(Handles,MouseEnter,		void(NormalId));
		DECLARE_EVENT_REPLICATOR_SIG(Handles,MouseLeave,		void(NormalId));
		DECLARE_EVENT_REPLICATOR_SIG(Handles,MouseDrag,			void(NormalId,float));
		DECLARE_EVENT_REPLICATOR_SIG(Handles,MouseButton1Down,	void(NormalId));
		DECLARE_EVENT_REPLICATOR_SIG(Handles,MouseButton1Up,	void(NormalId));
		

		enum VisualStyle
		{
			RESIZE_HANDLES   = 0, 
			MOVEMENT_HANDLES = 1,
			ARC_HANDLES  = 2,
			VELOCITY_HANDLES = 3,
		};

		void		setVisualStyle(VisualStyle value);
		VisualStyle getVisualStyle() const { return visualStyle; }

		void	setFaces(Faces value);
		Faces	getFaces() const { return faces; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// HandlesBase
		/*override*/ RBX::HandleType getHandleType() const;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// HandlesBase
		/*override*/ int getHandlesNormalIdMask() const { return faces.normalIdMask; }
		/*override*/ void setServerGuiObject();

	private:
		VisualStyle visualStyle;
		Faces faces;
	};

}


