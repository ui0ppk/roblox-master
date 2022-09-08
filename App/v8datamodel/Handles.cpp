/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Handles.h"
#include "V8World/Primitive.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/MouseCommand.h"
#include "Network/Players.h"
#include "AppDraw/DrawAdorn.h"
#include "Util/HitTest.h"

namespace RBX {

const char* const sHandles = "Handles";

REFLECTION_BEGIN();
static const Reflection::EnumPropDescriptor<Handles, Handles::VisualStyle> prop_Style("Style", category_Appearance, &Handles::getVisualStyle, &Handles::setVisualStyle);
static const Reflection::PropDescriptor<Handles, Faces>	prop_Faces("Faces", category_Data, &Handles::getFaces, &Handles::setFaces);

static Reflection::RemoteEventDesc<Handles, void(NormalId)>	   event_MouseEnter(&Handles::mouseEnterSignal,		  "MouseEnter",       "face", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Handles, void(NormalId)>	   event_MouseLeave(&Handles::mouseLeaveSignal,		  "MouseLeave",		  "face", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Handles, void(NormalId, float)> event_MouseDrag(&Handles::mouseDragSignal,		  "MouseDrag",	      "face", "distance", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Handles, void(NormalId)> event_MouseButton1Down(&Handles::mouseButton1DownSignal, "MouseButton1Down", "face", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Handles, void(NormalId)> event_MouseButton1Up  (&Handles::mouseButton1UpSignal,   "MouseButton1Up",	  "face", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

IMPLEMENT_EVENT_REPLICATOR(Handles,event_MouseEnter,		"MouseEnter",		MouseEnter);
IMPLEMENT_EVENT_REPLICATOR(Handles,event_MouseLeave,		"MouseLeave",		MouseLeave);
IMPLEMENT_EVENT_REPLICATOR(Handles,event_MouseDrag,			"MouseDrag",		MouseDrag);
IMPLEMENT_EVENT_REPLICATOR(Handles,event_MouseButton1Down,	"MouseButton1Down",	MouseButton1Down);
IMPLEMENT_EVENT_REPLICATOR(Handles,event_MouseButton1Up,	"MouseButton1Up",	MouseButton1Up);
REFLECTION_END();


Handles::Handles()
	: DescribedCreatable<Handles, HandlesBase, sHandles>("Handles")
	, faces(NORM_ALL_MASK)
	, visualStyle(RESIZE_HANDLES)
	, CONSTRUCT_EVENT_REPLICATOR(Handles,Handles::mouseEnterSignal,		  event_MouseEnter,			MouseEnter)
	, CONSTRUCT_EVENT_REPLICATOR(Handles,Handles::mouseLeaveSignal,		  event_MouseLeave,			MouseLeave)
	, CONSTRUCT_EVENT_REPLICATOR(Handles,Handles::mouseDragSignal,		  event_MouseDrag,			MouseDrag)
	, CONSTRUCT_EVENT_REPLICATOR(Handles,Handles::mouseButton1DownSignal, event_MouseButton1Down,	MouseButton1Down)
	, CONSTRUCT_EVENT_REPLICATOR(Handles,Handles::mouseButton1UpSignal,   event_MouseButton1Up,		MouseButton1Up)
{
	CONNECT_EVENT_REPLICATOR(MouseEnter);
	CONNECT_EVENT_REPLICATOR(MouseLeave);
	CONNECT_EVENT_REPLICATOR(MouseDrag);
	CONNECT_EVENT_REPLICATOR(MouseButton1Down);
	CONNECT_EVENT_REPLICATOR(MouseButton1Up);
}


void Handles::setFaces(Faces value)
{
	if(faces != value){
		faces = value;
		raisePropertyChanged(prop_Faces);
	}
}
void Handles::setServerGuiObject()
{
	Super::setServerGuiObject();

	//Since we are running the server, we need to be watching for local listeners
	eventReplicatorMouseEnter.setListenerMode(!mouseEnterSignal.empty());
	eventReplicatorMouseLeave.setListenerMode(!mouseLeaveSignal.empty());
	eventReplicatorMouseDrag.setListenerMode(!mouseDragSignal.empty());
	eventReplicatorMouseButton1Down	.setListenerMode(!mouseButton1DownSignal.empty());
	eventReplicatorMouseButton1Up.setListenerMode(!mouseButton1UpSignal.empty());
}


void Handles::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
{
	Super::onPropertyChanged(descriptor);

	eventReplicatorMouseEnter.onPropertyChanged(descriptor);
	eventReplicatorMouseLeave.onPropertyChanged(descriptor);
	eventReplicatorMouseDrag.onPropertyChanged(descriptor);
	eventReplicatorMouseButton1Down.onPropertyChanged(descriptor);
	eventReplicatorMouseButton1Up.onPropertyChanged(descriptor);
}

GuiResponse Handles::process(const shared_ptr<InputObject>& event)
{
	if(event->isMouseEvent())
    {
        // ignore mouse if not visible
        if ( !getVisible() )
            return GuiResponse::notSunk();

		switch(event->getUserInputType())
		{
			case InputObject::TYPE_MOUSEMOVEMENT:
			{
				if(mouseDownCaptureInfo){
					float distance;
					if(getDistanceFromHandle(event,mouseDownCaptureInfo->hitNormalId, mouseDownCaptureInfo->hitPointWorld, distance)){
						mouseDragSignal(mouseDownCaptureInfo->hitNormalId, distance);
					}
				}

				Vector3 hitPointWorld;
				NormalId hitNormalId;
				if(findTargetHandle(event, hitPointWorld, hitNormalId)){
					if(mouseOver != NORM_UNDEFINED && mouseOver != hitNormalId){
						mouseLeaveSignal(mouseOver);
					}

					mouseOver = hitNormalId;
					mouseEnterSignal(hitNormalId);
				}
				else{
					if(mouseOver != NORM_UNDEFINED){
						mouseLeaveSignal(hitNormalId);
						mouseOver = NORM_UNDEFINED;
					}
				}
				return GuiResponse::notSunk();
			}
			case InputObject::TYPE_MOUSEBUTTON1:
			{
				RBXASSERT(event->isLeftMouseDownEvent() || event->isLeftMouseUpEvent());
				if (event->isLeftMouseDownEvent())
				{
					Vector3 hitPointWorld;
					NormalId hitNormalId;
					if(findTargetHandle(event, hitPointWorld, hitNormalId)){
						//Capture the MouseDown information for later use
						mouseDownCaptureInfo.reset(new MouseDownCaptureInfo(adornee.lock()->getLocation(),hitPointWorld, hitNormalId));

						mouseButton1DownSignal(hitNormalId);

						return GuiResponse::sunk();
					}
				}
				else if(event->isLeftMouseUpEvent())
				{
					//Clear our mouse down capture info
					mouseDownCaptureInfo.reset();

					Vector3 hitPointWorld;
					NormalId hitNormalId;
					if(findTargetHandle(event, hitPointWorld, hitNormalId))
						mouseButton1UpSignal(hitNormalId);
				}
				return GuiResponse::notSunk();
			}
            default:
                break;
		}
	}
	return GuiResponse::notSunk();
}

RBX::HandleType Handles::getHandleType() const 
{
	switch(visualStyle){
		case RESIZE_HANDLES:	return HANDLE_RESIZE;
		case MOVEMENT_HANDLES:	return HANDLE_MOVE;
		case ARC_HANDLES:		return HANDLE_ROTATE;
		case VELOCITY_HANDLES:	return HANDLE_VELOCITY;
		default:
			return HANDLE_RESIZE;
	}
}

void Handles::setVisualStyle(VisualStyle value)
{
	if(visualStyle != value){
		visualStyle = value;
		raisePropertyChanged(prop_Style);
	}
}

}
