/* Copyright 2003-2012 ROBLOX Corporation, All Rights Reserved */
#if 1 // disable until we are ready for new joint schema
#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"
#include "Util/NormalId.h"

namespace RBX {

    // This is not a streaming/replication safe option, do not enable!
    // #define RBX_ATTACHMENT_LOCKING

	extern const char *const sAttachment;
	class Attachment
		: public DescribedCreatable<Attachment, Instance, sAttachment>
		, public IAdornable
	{
    public:
        // The adorn looks of the attachment
        static float adornRadius;

        // The attachment adorn looks under the AttachmentTool
        static float toolAdornHandleRadius;
        static float toolAdornMajorAxisSize;
        static float toolAdornMajorAxisRadius;
        static float toolAdornMinorAxisSize;
        static float toolAdornMinorAxisRadius;

	private:
        typedef DescribedCreatable<Attachment, Instance, sAttachment> Super;
        
        // Should we render something in game
		bool visible;

        bool locked;

        Vector3 pivotPositionInPart;
        Vector3 axisDirectionInPart;
        Vector3 secondaryAxisDirectionInPart;

        bool shouldRender3dAdorn() const override {return true;}
        void verifySetParent(const Instance* instance) const override;
        void verifyAddChild(const Instance* newChild) const override;

	public:
		static Reflection::PropDescriptor<Attachment, CoordinateFrame> prop_Frame;

		Attachment();
		~Attachment() {}	

        // For UI, scripting and streaming
        bool getVisible( void ) const { return visible; }
        void setVisible( bool value );

        bool getLocked( void ) const { return locked; }
        void setLocked( bool value );

        // UI and replication
        CoordinateFrame getFrameInPart() const;
        void setFrameInPart( const CoordinateFrame& frame );
        CoordinateFrame getFrameInWorld() const;

        // UI and scripting
        Vector3 getPivotInPart() const;
        void setPivotInPart( const Vector3& pivot );
        Vector3 getPivotInWorld(void) const;

        Vector3 getEulerAnglesInPart() const;
        void setEulerAnglesInPart( const Vector3& v );
        Vector3 getEulerAnglesInWorld() const;

        Vector3 getAxisInPart( void ) const;
        Vector3 getAxisInWorld(void) const;

        Vector3 getSecondaryAxisInPart( void ) const;
        Vector3 getSecondaryAxisInWorld(void) const;

        CoordinateFrame getParentFrame() const;

        // Only for scripting
        void setAxes( Vector3 axis, Vector3 secondaryAxis );

        // Hidden from API as it has side effects
        // Used only by AttachmentTool
        void setAxisInPart( Vector3 axis );

        virtual float intersectAdornWithRay( const RbxRay& r );

#ifdef RBX_ATTACHMENT_LOCKING
        // disabled because not streaming/replication safe
        static const Reflection::PropDescriptor<Attachment,	bool>  prop_Locked;
#endif

        // Rendering
        enum SelectState
        {
            SelectState_None = 0,
            SelectState_Normal = 1,
            SelectState_Hovered = 2,
            SelectState_Paired = 4,
            SelectState_Hidden = 8
        };
        void render3dToolAdorn(Adorn* adorn, Attachment::SelectState selectState);
        void render3dAdorn(Adorn* adorn) override;

    private:
        // Hidden from API for having order dependency issues

        // This might change the secondary axis in order to keep it orthogonal to the axis
        void setAxisInPartInternal( const Vector3& axis );

        // This will not change the axis, but will project the secondary axis onto the orthogonormal circle to the axis
        void setSecondaryAxisInPartInternal( const Vector3& axis );

        void setOrientationInPartInternal( const Matrix3& o );
        Matrix3 getOrientationInPart( ) const;
        Matrix3 getOrientationInWorld( ) const;
	};

} // namespace

#endif
