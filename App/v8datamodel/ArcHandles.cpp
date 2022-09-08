/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ArcHandles.h"
#include "V8World/Primitive.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/MouseCommand.h"
#include "Network/Players.h"
#include "AppDraw/DrawAdorn.h"
#include "Util/HitTest.h"
#include "Util/Faces.h"

namespace RBX {

const char* const sArcHandles = "ArcHandles";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<ArcHandles, Axes>	prop_Axes("Axes", category_Data, &ArcHandles::getAxes, &ArcHandles::setAxes);

Reflection::RemoteEventDesc<ArcHandles, void(RBX::Vector3::Axis)>	   event_MouseEnter(&ArcHandles::mouseEnterSignal,		  "MouseEnter",       "axis", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
Reflection::RemoteEventDesc<ArcHandles, void(RBX::Vector3::Axis)>	   event_MouseLeave(&ArcHandles::mouseLeaveSignal,		  "MouseLeave",		  "axis", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
Reflection::RemoteEventDesc<ArcHandles, void(RBX::Vector3::Axis, float, float)> event_MouseDrag(&ArcHandles::mouseDragSignal, "MouseDrag",		  "axis", "relativeAngle", "deltaRadius", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
Reflection::RemoteEventDesc<ArcHandles, void(RBX::Vector3::Axis)> event_MouseButton1Down(&ArcHandles::mouseButton1DownSignal, "MouseButton1Down", "axis", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
Reflection::RemoteEventDesc<ArcHandles, void(RBX::Vector3::Axis)> event_MouseButton1Up  (&ArcHandles::mouseButton1UpSignal,   "MouseButton1Up",	  "axis", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

IMPLEMENT_EVENT_REPLICATOR(ArcHandles,event_MouseEnter,		"MouseEnter",		MouseEnter);
IMPLEMENT_EVENT_REPLICATOR(ArcHandles,event_MouseLeave,		"MouseLeave",		MouseLeave);
IMPLEMENT_EVENT_REPLICATOR(ArcHandles,event_MouseDrag,		"MouseDrag",		MouseDrag);
IMPLEMENT_EVENT_REPLICATOR(ArcHandles,event_MouseButton1Down,	"MouseButton1Down",	MouseButton1Down);
IMPLEMENT_EVENT_REPLICATOR(ArcHandles,event_MouseButton1Up,	"MouseButton1Up",	MouseButton1Up);
REFLECTION_END();


ArcHandles::ArcHandles()
	: DescribedCreatable<ArcHandles, HandlesBase, sArcHandles>("ArcHandles")
	, axes(0x7)
	, CONSTRUCT_EVENT_REPLICATOR(ArcHandles,ArcHandles::mouseEnterSignal,		event_MouseEnter,		MouseEnter)
	, CONSTRUCT_EVENT_REPLICATOR(ArcHandles,ArcHandles::mouseLeaveSignal,		event_MouseLeave,		MouseLeave)
	, CONSTRUCT_EVENT_REPLICATOR(ArcHandles,ArcHandles::mouseDragSignal,	    event_MouseDrag,		MouseDrag)
	, CONSTRUCT_EVENT_REPLICATOR(ArcHandles,ArcHandles::mouseButton1DownSignal, event_MouseButton1Down,	MouseButton1Down)
	, CONSTRUCT_EVENT_REPLICATOR(ArcHandles,ArcHandles::mouseButton1UpSignal,   event_MouseButton1Up,	MouseButton1Up)
{
	CONNECT_EVENT_REPLICATOR(MouseEnter);
	CONNECT_EVENT_REPLICATOR(MouseLeave);
	CONNECT_EVENT_REPLICATOR(MouseDrag);
	CONNECT_EVENT_REPLICATOR(MouseButton1Down);
	CONNECT_EVENT_REPLICATOR(MouseButton1Up);
}

void ArcHandles::setAxes(Axes value)
{
	if(axes != value){
		axes = value;
		raisePropertyChanged(prop_Axes);
	}
}
int ArcHandles::getHandlesNormalIdMask() const
{
	Faces faces;
	faces.setNormalId(RBX::NORM_X, axes.getAxisByNormalId(RBX::NORM_X));
	faces.setNormalId(RBX::NORM_Y, axes.getAxisByNormalId(RBX::NORM_Y));
	faces.setNormalId(RBX::NORM_Z, axes.getAxisByNormalId(RBX::NORM_Z));

	faces.setNormalId(RBX::NORM_X_NEG, axes.getAxisByNormalId(RBX::NORM_X));
	faces.setNormalId(RBX::NORM_Y_NEG, axes.getAxisByNormalId(RBX::NORM_Y));
	faces.setNormalId(RBX::NORM_Z_NEG, axes.getAxisByNormalId(RBX::NORM_Z));

	return faces.normalIdMask;
}

void ArcHandles::setServerGuiObject()
{
	Super::setServerGuiObject();

	//Since we are running the server, we need to be watching for local listeners
	eventReplicatorMouseEnter.setListenerMode(!mouseEnterSignal.empty());
	eventReplicatorMouseLeave.setListenerMode(!mouseLeaveSignal.empty());
	eventReplicatorMouseDrag.setListenerMode(!mouseDragSignal.empty());
	eventReplicatorMouseButton1Down	.setListenerMode(!mouseButton1DownSignal.empty());
	eventReplicatorMouseButton1Up.setListenerMode(!mouseButton1UpSignal.empty());
}


void ArcHandles::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
{
	Super::onPropertyChanged(descriptor);

	eventReplicatorMouseEnter.onPropertyChanged(descriptor);
	eventReplicatorMouseLeave.onPropertyChanged(descriptor);
	eventReplicatorMouseDrag.onPropertyChanged(descriptor);
	eventReplicatorMouseButton1Down.onPropertyChanged(descriptor);
	eventReplicatorMouseButton1Up.onPropertyChanged(descriptor);
}

GuiResponse ArcHandles::process(const shared_ptr<InputObject>& event)
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
				if(mouseDownCaptureInfo)
                {
					float relangle;
					float relradius;
					float absangle;
					float absradius;
					if (getAngleRadiusFromHandle(event,mouseDownCaptureInfo->hitNormalId,
						mouseDownCaptureInfo->hitPointWorld, relangle, relradius, absangle, absradius))
					{
						bool reverse = mouseDownCaptureInfo->hitNormalId >= NORM_X_NEG;

						mouseDragSignal(Axes::normalIdToAxis(mouseDownCaptureInfo->hitNormalId), reverse ? -relangle : relangle, relradius);
					}
				}

				Vector3 hitPointWorld;
				NormalId hitNormalId;
				if(findTargetHandle(event, hitPointWorld, hitNormalId)){
					if(mouseOver != NORM_UNDEFINED && mouseOver != hitNormalId){
						mouseLeaveSignal(Axes::normalIdToAxis(mouseOver));
					}

					mouseOver = hitNormalId;
					mouseEnterSignal(Axes::normalIdToAxis(hitNormalId));
				}
				else{
					if(mouseOver != NORM_UNDEFINED){
						mouseLeaveSignal(Axes::normalIdToAxis(hitNormalId));
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

						mouseButton1DownSignal(Axes::normalIdToAxis(hitNormalId));

						return GuiResponse::sunk();
					}
					return GuiResponse::notSunk();
				}
				else if(event->isLeftMouseUpEvent())
				{
					//Clear our mouse down capture info
					mouseDownCaptureInfo.reset();

					Vector3 hitPointWorld;
					NormalId hitNormalId;
					if(findTargetHandle(event, hitPointWorld, hitNormalId)){
						mouseButton1UpSignal(Axes::normalIdToAxis(hitNormalId));
					}
					return GuiResponse::notSunk();
				}
				return GuiResponse::notSunk();
			}
            default:
                break;
		}
	}
	return GuiResponse::notSunk();
}

RBX::HandleType ArcHandles::getHandleType() const 
{
	return HANDLE_ROTATE;
}

}
