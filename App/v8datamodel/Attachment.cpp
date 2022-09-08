/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#if 1 // disable until we are ready for new joint schema

#include "V8DataModel/Attachment.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Surface.h"
#include "V8DataModel/Workspace.h"
#include "AppDraw/Draw.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "V8World/Primitive.h"
#include "Util/Color.h"

// #define ENABLE_AXES_API

namespace RBX {

using namespace Reflection;

const char *const sAttachment = "Attachment";

REFLECTION_BEGIN()
// Streaming
Reflection::PropDescriptor<Attachment, CoordinateFrame> Attachment::prop_Frame("CFrame",category_Data, &Attachment::getFrameInPart, &Attachment::setFrameInPart, PropertyDescriptor::STANDARD);

// UI
static const PropDescriptor<Attachment, Vector3> prop_Position("Position", category_Data, &Attachment::getPivotInPart, &Attachment::setPivotInPart, PropertyDescriptor::UI);
static const PropDescriptor<Attachment, Vector3> prop_Rotation("Rotation", category_Data, &Attachment::getEulerAnglesInPart, &Attachment::setEulerAnglesInPart, PropertyDescriptor::UI);
static const PropDescriptor<Attachment, Vector3> prop_WorldPosition("WorldPosition", "Derived Data", &Attachment::getPivotInWorld, NULL, PropertyDescriptor::UI);
static const PropDescriptor<Attachment, Vector3> prop_WorldRotation("WorldRotation", "Derived Data", &Attachment::getEulerAnglesInWorld, NULL, PropertyDescriptor::UI);

#ifdef ENABLE_AXES_API
static const PropDescriptor<Attachment, Vector3> prop_Axis("Axis", "Derived Data", &Attachment::getAxisInPart, NULL, PropertyDescriptor::UI);
static const PropDescriptor<Attachment, Vector3> prop_SecondaryAxis("SecondaryAxis", "Derived Data", &Attachment::getSecondaryAxisInPart, NULL, PropertyDescriptor::UI);
static const PropDescriptor<Attachment, Vector3> prop_WorldAxis("WorldAxis", "Derived Data", &Attachment::getAxisInWorld, NULL, PropertyDescriptor::UI);
static const PropDescriptor<Attachment, Vector3> prop_WorldSecondaryAxis("WorldSecondaryAxis", "Derived Data", &Attachment::getSecondaryAxisInWorld, NULL, PropertyDescriptor::UI);

static BoundFuncDesc<Attachment, void(Vector3,Vector3)> func_setAxes(&Attachment::setAxes, "SetAxes", "axis0", "axis1", Security::None);
static BoundFuncDesc<Attachment, void(Vector3)> func_setAxis(&Attachment::setAxisInPart, "SetAxis", "axis", Security::None);
#endif

#ifdef RBX_ATTACHMENT_LOCKING
const PropDescriptor<Attachment, bool>  Attachment::prop_Locked("Locked", category_Behavior, &Attachment::getLocked, &Attachment::setLocked, PropertyDescriptor::UI);
#endif

REFLECTION_END()

float Attachment::toolAdornHandleRadius = 0.2f;
float Attachment::toolAdornMajorAxisSize = 0.4f;
float Attachment::toolAdornMajorAxisRadius = 0.07f;
float Attachment::toolAdornMinorAxisSize = 0.3f;
float Attachment::toolAdornMinorAxisRadius = 0.04f;
float Attachment::adornRadius = 0.2f;

Attachment::Attachment()
	: visible(false)
    , locked(false)
    , axisDirectionInPart(1, 0, 0)
    , secondaryAxisDirectionInPart(0, 1, 0)
    , pivotPositionInPart(1, 0, 0)
{
	setName("Attachment");
}

void Attachment::setVisible(bool value)
{
    if (value != visible)
    {
        visible = value;
    }
}

Vector3 Attachment::getEulerAnglesInPart() const
{
    Matrix3 o = getOrientationInPart();
    Vector3 r;
    o.toEulerAnglesXYZ(r.x,r.y,r.z);
    return Vector3(
        Math::radiansToDegrees(r.x),
        Math::radiansToDegrees(r.y),
        Math::radiansToDegrees(r.z) );
}

Vector3 Attachment::getEulerAnglesInWorld() const
{
    Matrix3 o = getOrientationInWorld();
    Vector3 r;
    o.toEulerAnglesXYZ(r.x,r.y,r.z);
    return Vector3(
        Math::radiansToDegrees(r.x),
        Math::radiansToDegrees(r.y),
        Math::radiansToDegrees(r.z) );
}

void Attachment::setEulerAnglesInPart( const Vector3& v )
{
    Matrix3 o = Matrix3::fromEulerAnglesXYZ(Math::degreesToRadians(v.x),Math::degreesToRadians(v.y),Math::degreesToRadians(v.z));
    setOrientationInPartInternal( o );
    raiseChanged(prop_Frame);
}

void Attachment::setOrientationInPartInternal( const Matrix3& o )
{
    setAxisInPartInternal( o.column(0) );
    setSecondaryAxisInPartInternal( o.column(1) );

    raiseChanged(prop_Rotation);
    raiseChanged(prop_WorldRotation);
}

Matrix3 Attachment::getOrientationInPart( ) const
{
    Matrix3 o;
    o.setColumn( 0, axisDirectionInPart );
    o.setColumn( 1, secondaryAxisDirectionInPart );
    o.setColumn( 2, o.column(0).cross( o.column(1) ) );
    return o;
}

Matrix3 Attachment::getOrientationInWorld( ) const
{
    Matrix3 o;
    CoordinateFrame partCF = getParentFrame();
    o.setColumn( 0, partCF.vectorToWorldSpace( axisDirectionInPart ) );
    o.setColumn( 1, partCF.vectorToWorldSpace( secondaryAxisDirectionInPart ) );
    o.setColumn( 2, o.column(0).cross( o.column(1) ) );
    return o;
}

void Attachment::setPivotInPart( const Vector3& pivot ) 
{
    pivotPositionInPart = pivot; 
    raiseChanged(prop_Position);
    raiseChanged(prop_WorldPosition);
    raiseChanged(prop_Frame);
}

Vector3 Attachment::getPivotInPart() const 
{
    return pivotPositionInPart;
}

Vector3 Attachment::getPivotInWorld(void) const
{
    return getParentFrame().pointToWorldSpace( pivotPositionInPart );
}

void Attachment::setAxisInPartInternal( const Vector3& _axis )
{
    if( _axis.magnitude() < 0.0001f )
    {
        return;
    }
    axisDirectionInPart = _axis;
    axisDirectionInPart.unitize();
#ifdef ENABLE_AXES_API
    raiseChanged( prop_Axis );
    raiseChanged( prop_WorldAxis );
#endif

	// This will orthonormalize the secondary axis to the axis
    setSecondaryAxisInPartInternal(secondaryAxisDirectionInPart);
}

void Attachment::setAxisInPart( Vector3 _axis )
{
    setAxisInPartInternal( _axis );
    raiseChanged( prop_Rotation );
    raiseChanged( prop_WorldRotation );
    raiseChanged( prop_Frame );
}

Vector3 Attachment::getAxisInPart( void ) const 
{
    return axisDirectionInPart;
}

Vector3 Attachment::getAxisInWorld(void) const
{
    return getParentFrame().vectorToWorldSpace( axisDirectionInPart );
}

Vector3 Attachment::getSecondaryAxisInWorld(void) const
{
    return getParentFrame().vectorToWorldSpace( secondaryAxisDirectionInPart );
}

Vector3 Attachment::getSecondaryAxisInPart( void ) const 
{
    return secondaryAxisDirectionInPart;
}

void Attachment::setSecondaryAxisInPartInternal( const Vector3& _axis2 )
{
    if( _axis2.magnitude() < 0.00001f )
    {
        return;
    }
    Vector3 axis2 = _axis2;

    Vector3 axis;
    axis = axisDirectionInPart;

    axis2.unitize();
    if( fabsf( axis2.dot( axis ) ) < 0.999f )
    {
        // The input secondary axis is not completely parallel to the axis, we can project
        axis2 = axis2 - axis2.dot( axis ) * axis;
    }
    else
    {
        // The secondary axis is parallel with the axis, we need to make a choice
        if( fabsf( axis.dot( Vector3::unitY() ) ) < sqrtf(2.0f)*0.5f )
        {
            // If the axis makes more than 45deg angle with Y axis, project the Y axis on the orthogonal plane of the axis
            Vector3 t = Vector3::unitY().cross( axis );
            axis2 = axis.cross( t );
        }
        else
        {
            if( fabsf( axis.x ) + 0.1 > fabsf( axis.z ) )
            {
                Vector3 t = Vector3::unitZ().cross( axis );
                axis2 = axis.cross( t );
            }
            else
            {
                Vector3 t = Vector3::unitX().cross( axis );
                axis2 = axis.cross( t );
            }
        }
    }
    axis2.unitize();
    RBXASSERT( fabsf( axis2.dot(axis) ) < 0.00001f );

    secondaryAxisDirectionInPart = axis2;
#ifdef ENABLE_AXES_API
    raiseChanged( prop_SecondaryAxis );
    raiseChanged( prop_WorldSecondaryAxis );
#endif
}

void Attachment::setAxes( Vector3 axis, Vector3 axis2 )
{
    setAxisInPartInternal( axis );
    setSecondaryAxisInPartInternal( axis2 );
    raiseChanged( prop_Rotation );
    raiseChanged( prop_WorldRotation );
    raiseChanged( prop_Frame );
}

CoordinateFrame Attachment::getFrameInPart() const
{
    CoordinateFrame cf;
    cf.translation = getPivotInPart();
    cf.rotation = getOrientationInPart();
    return cf;
}

void Attachment::setFrameInPart( const CoordinateFrame& frame )
{
    setOrientationInPartInternal( frame.rotation );
    setPivotInPart( frame.translation );
    raiseChanged( prop_Frame );
}

CoordinateFrame Attachment::getFrameInWorld() const
{
    CoordinateFrame cf;
    cf.translation = getPivotInWorld();
    cf.rotation = getOrientationInWorld();
    return cf;
}

CoordinateFrame Attachment::getParentFrame() const
{
    if( !locked )
    {
        const PartInstance* part = Instance::fastDynamicCast<const PartInstance>(getParent());
        if( part )
        {
            return part->getCoordinateFrame();
        }
    }
    return CoordinateFrame();
}

void Attachment::setLocked( bool value )
{
    if( !locked && value )
    {
        Vector3 pivotInWorld = getPivotInWorld();
        Vector3 axisInWorld = getAxisInWorld();
        Vector3 secondaryAxisInWorld = getSecondaryAxisInWorld();

        locked = true;
        pivotPositionInPart = pivotInWorld;
        axisDirectionInPart = axisInWorld;
        secondaryAxisDirectionInPart = secondaryAxisInWorld;
#ifdef RBX_ATTACHMENT_LOCKING
        raiseChanged( prop_Locked );
#endif
        raiseChanged(prop_Position);
        raiseChanged(prop_WorldPosition);
        raiseChanged(prop_Rotation);
        raiseChanged(prop_WorldRotation);
        raiseChanged(prop_Frame);
    }

    if( locked && !value )
    {
        Vector3 pivotInPart = getPivotInPart();
        Vector3 axisInPart = getAxisInPart();
        Vector3 secondaryAxisInPart = getSecondaryAxisInPart();

        locked = false;
        CoordinateFrame partFrame = getParentFrame();

        pivotPositionInPart = partFrame.pointToObjectSpace( pivotInPart );
        axisDirectionInPart = partFrame.vectorToObjectSpace( axisInPart );
        secondaryAxisDirectionInPart = partFrame.vectorToObjectSpace( secondaryAxisInPart );
#ifdef RBX_ATTACHMENT_LOCKING
        raiseChanged( prop_Locked );
#endif
        raiseChanged(prop_Position);
        raiseChanged(prop_WorldPosition);
        raiseChanged(prop_Rotation);
        raiseChanged(prop_WorldRotation);
        raiseChanged( prop_Frame );
    }
}

void Attachment::render3dAdorn(Adorn* adorn)
{
    if( !visible )
        return;

    adorn->setMaterial( Adorn::Material_Default );
    adorn->setObjectToWorldMatrix( getFrameInWorld() );
    Sphere sphere;
    sphere.radius = adornRadius;
    adorn->sphere(sphere, Color3::green());
}

static void renderToolAdornAxes( Adorn* adorn, const CoordinateFrame& cf )
{
    {
        CoordinateFrame cf1 = cf;
        cf1.translation += 0.5f * Attachment::toolAdornMajorAxisSize * cf.rotation.column(0);
        DrawAdorn::cylinder( adorn, cf1, 0, Attachment::toolAdornMajorAxisSize, Attachment::toolAdornMajorAxisRadius, Color3::yellow() );
    }

    {
        CoordinateFrame cf1 = cf;
        cf1.translation += 0.5f * Attachment::toolAdornMinorAxisSize * cf.rotation.column(1);
        DrawAdorn::cylinder( adorn, cf1, 1, Attachment::toolAdornMinorAxisSize, Attachment::toolAdornMinorAxisRadius, Color3::red() );
    }
}

void Attachment::render3dToolAdorn(Adorn* adorn, Attachment::SelectState selectState)
{
    float alpha = 1.0f;
    static float dimming = 0.7f;
    float intensity = dimming;
    Color3 color = ( selectState & SelectState_Paired ) ? Color3::green() : Color3::orange();
    adorn->setMaterial( Adorn::Material_SelfLit );

    if( selectState & SelectState_Normal )
    {
        intensity = 1.0f;
    }

    if( selectState & SelectState_Hovered )
    {
        intensity = 1.0f;
        adorn->setMaterial( Adorn::Material_SelfLitHighlight );
    }

    if( selectState & SelectState_Hidden )
    {
        alpha = 0.1f;
    }
    
    if( ( selectState & SelectState_Normal ) || ( selectState & SelectState_Hovered ) )
    {
        renderToolAdornAxes( adorn, getFrameInWorld() );
    }

    adorn->setObjectToWorldMatrix( getFrameInWorld() );
    Sphere sphere;
    sphere.radius = toolAdornHandleRadius;
    adorn->sphere( sphere, Color4( intensity * color, alpha ) );
}

float Attachment::intersectAdornWithRay( const RbxRay& r )
{
    Sphere s;
    s.center = getPivotInWorld();
    s.radius = toolAdornHandleRadius;
    return r.intersectionTime( s );
}

void Attachment::verifySetParent(const Instance* instance) const
{
    Super::verifySetParent(instance);

    if( instance != NULL && instance->fastDynamicCast< PartInstance >() == NULL && instance->fastDynamicCast< Workspace >() == NULL )
    {
        throw RBX::runtime_error("Attachments can only be parented to parts or the workspace");
    }
}

void Attachment::verifyAddChild(const Instance* newChild) const
{
    Super::verifyAddChild( newChild );

    if( newChild != NULL /*&& newChild->fastDynamicCast< Constraints::Constraint >() == NULL*/ )
    {
        throw RBX::runtime_error("Attachments can't have children.");
    }
}

} // namespace

#endif
