#include "stdafx.h"

#include "V8DataModel/SurfaceGui.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "v8datamodel/TextLabel.h"

#include "GfxBase/AdornSurface.h"

#include "v8datamodel/PlayerGui.h"

#include "Util/RobloxGoogleAnalytics.h"

FASTFLAGVARIABLE(GUIZFighterGPU, true)

static void sendSurfaceGuiStats()
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "SurfaceGui");
}

static void sendSurfaceGUINonTextUsage()
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME,"SurfaceGuiNonText");
}

namespace RBX {
	int SurfaceGui::s_numInstances = 0;
	const char* const sAdornmentSurfaceGui = "SurfaceGui";

    REFLECTION_BEGIN();
	static const Reflection::RefPropDescriptor< SurfaceGui, Instance>	prop_adornee("Adornee", category_Data, &SurfaceGui::getAdornee, &SurfaceGui::setAdornee);
	static const Reflection::PropDescriptor<SurfaceGui, bool>	  prop_Enabled("Enabled", category_Data, &SurfaceGui::getEnabled, &SurfaceGui::setEnabled);
	static const Reflection::PropDescriptor<SurfaceGui, bool>	  prop_Active("Active", category_Data, &SurfaceGui::getActive, &SurfaceGui::setActive);
	static const Reflection::PropDescriptor<SurfaceGui, Vector2>   prop_CanvasSize( "CanvasSize", category_Data, &SurfaceGui::getCanvasSize, &SurfaceGui::setCanvasSize);
	static const Reflection::EnumPropDescriptor<SurfaceGui, NormalId > prop_Surface("Face", category_Data, &SurfaceGui::getFace, &SurfaceGui::setFace);
   	static const Reflection::PropDescriptor<SurfaceGui, float>    prop_maxDistance("ToolPunchThroughDistance", category_Data, &SurfaceGui::getToolPunchThroughDistance, &SurfaceGui::setToolPunchThroughDistance);
    REFLECTION_END();

	SurfaceGui::SurfaceGui()
		:DescribedCreatable<SurfaceGui, GuiLayerCollector, sAdornmentSurfaceGui>("SurfaceGui")
		,enabled(true)
		,active(true)
		,canvasSize(Vector2(800,600))
		,faceID(NORM_Z_NEG)
        ,toolPunchThroughDistance(0)
	{
		++s_numInstances;
		setAbsoluteSize(canvasSize);
	}

	SurfaceGui::~SurfaceGui()
	{
		--s_numInstances;
	}

	bool SurfaceGui::askSetParent(const Instance* instance) const
	{
		return (Instance::fastDynamicCast<GuiBase2d>(instance) == NULL);
	}

	bool SurfaceGui::shouldRender3dSortedAdorn() const
	{
		const shared_ptr<Instance> part(getPart());
		if (enabled && part)
			// only render if part is still in DataModel
			return DataModel::get(part.get()) ? true : false;
		else
			return false;
	}

	boost::shared_ptr<Instance> SurfaceGui::getPart() const
	{
		boost::shared_ptr<Instance> part = adornee.lock();
		if(!part)
		{
			// if no adornee specified, use parent if it is a partInstance.
			// allows for easy insertion of billboard guis in the world that replicate and show for everyone.
			part = shared_from(const_cast<Instance*>(getParent()));
		}

		return part;
	}

	Vector3 SurfaceGui::render3dSortedPosition() const
	{
		if(boost::shared_ptr<Instance> partInstance = getPart())
		{
			if(PartInstance* thePart = fastDynamicCast<PartInstance>(partInstance.get()))
				return thePart->calcRenderingCoordinateFrame().translation;
		}


		//Shouldn't be reached, but just in case.
		return Vector3(0,0,0);
	}

	bool SurfaceGui::buildGuiMatrix(Adorn* adorn, CoordinateFrame* partFrame, CoordinateFrame* projectionFrame)
	{
		boost::shared_ptr<Instance> partInstance = getPart();
        PartInstance* part = fastDynamicCast<PartInstance>(partInstance.get());
		Workspace* workspace = ServiceProvider::find<Workspace>(part);
        if (!part || !workspace)
            return false;

		CoordinateFrame cframe = part->calcRenderingCoordinateFrame();
        Vector3 size = part->getPartSizeXml();
        
        if (adorn && !adorn->isVisible(Extents::fromCenterCorner(Vector3(), size * 0.5f), cframe))
            return false;

		static const float yaw[6] =   {   90,  90,   0,   -90,    90,  180 };
		static const float pitch[6] = {   0,   90,   0,   0,     -90,    0 };

		Matrix4 rot = Matrix4::rollDegrees( pitch[faceID] ) * Matrix4::yawDegrees( yaw[faceID] );
		Matrix4 vp  = Matrix4::scale( 1/canvasSize.x, 1/canvasSize.y, 1 );
		Matrix4 tr = Matrix4::translation( -0.5f, 0.5f, 0.5f );
		Matrix4 sc = Matrix4::scale( size );
		Matrix4 proj = sc * rot * tr * vp;
		
		projectionFrame->rotation = proj.upper3x3();
		projectionFrame->translation = proj.column(3).xyz();

		*partFrame = cframe;
		return true;
	}

	void SurfaceGui::render3dSortedAdorn(Adorn* adorn)
	{
		if(!enabled || isYetAnotherSpecialCase() )
			return;

		CoordinateFrame part, proj;
		if (buildGuiMatrix(adorn, &part, &proj))
		{
			AdornSurface adornView(adorn, Rect2D(canvasSize), part * proj);
			Super::render2d(&adornView);
		}
	}

	// see class def for explanation
	bool SurfaceGui::isYetAnotherSpecialCase()
	{ 
		return !!Network::Players::findLocalPlayer(this) && this->isDescendantOf( RBX::ServiceProvider::find<RBX::StarterGuiService>(this) ); 
	}

	void SurfaceGui::unProcess()
	{
		shared_ptr<InputObject> mouseMoveEvent = Creatable<Instance>::create<InputObject>(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CANCEL,RBX::DataModel::get(this));
		mouseMoveEvent->setPosition(Vector3(-1e4,-1e4,-1e4));
		Super::process(mouseMoveEvent);

		shared_ptr<InputObject> focusEndEvent = Creatable<Instance>::create<InputObject>(InputObject::TYPE_FOCUS, InputObject::INPUT_STATE_CANCEL,RBX::DataModel::get(this));
		focusEndEvent->setPosition(Vector3(-1e4,-1e4,-1e4));;
		Super::process(focusEndEvent);
	}

	static Vector3 unproject( const CoordinateFrame& part, const CoordinateFrame& proj, Vector3 v )
	{
		v = part.pointToObjectSpace( v );
		v -= proj.translation;
		v = proj.rotation.inverse() * v;
		v.y = -v.y;
		
		return v;
	}

	GuiResponse SurfaceGui::process(const shared_ptr<InputObject>& event)
	{
		return GuiResponse::notSunk();
	}

	GuiResponse SurfaceGui::process3d(const shared_ptr<InputObject>& event, Vector3 point3d, bool ignoreMaxDistance)
	{
		CoordinateFrame part, proj;
		if (!buildGuiMatrix(NULL, &part, &proj))
			return GuiResponse::notSunk();

		Vector3 newpoint = unproject( part, proj, point3d );

		if(!enabled || !active)
			return GuiResponse::notSunk();

        if( RBX::ModelInstance* character = Network::Players::findLocalCharacter(this) )
        {
            Vector3 loc = character->getLocation().translation;
            if( !ignoreMaxDistance && (loc - point3d).length() >= toolPunchThroughDistance )
                return GuiResponse::notSunk();
        }

		if(event->isMouseEvent())
		{
			Vector3 oldpt  = event->getRawPosition();
			newpoint.z     = oldpt.z;
			event->setPosition(newpoint);
			GuiResponse response = Super::process(event, false);
			event->setPosition(oldpt);
			return response;
		}
		
		return GuiResponse::notSunk();
	}

	bool SurfaceGui::canProcessMeAndDescendants() const
	{
		return false;
	}

	void SurfaceGui::setEnabled(bool value)
	{
		if(enabled != value)
		{
			enabled = value;
			raisePropertyChanged(prop_Enabled);
			shouldRenderSetDirty();
		}
	}

	void SurfaceGui::setActive(bool value)
	{
		if(active != value)
		{
			active = value;
			raisePropertyChanged(prop_Active);
		}
	}

	void SurfaceGui::setCanvasSize( Vector2 s )
	{
		if( canvasSize != s )
		{
			canvasSize = s;
			handleResize( Rect2D(canvasSize), false );
			raisePropertyChanged(prop_CanvasSize);
		}
	}

	//////////////////////////////////////////////////////////////////////////
    struct ExpiredOrEqual
    {
        weak_ptr<SurfaceGui> value;

        bool operator()(const weak_ptr<SurfaceGui>& sg) const
        {
            return sg.expired() || weak_equal(sg, value);
        }
    };

	static void removeExpiredOrEqual(std::vector< weak_ptr< SurfaceGui > >& sglist, const weak_ptr<SurfaceGui>& value)
	{
        ExpiredOrEqual pred = {value};

		sglist.erase(std::remove_if(sglist.begin(), sglist.end(), pred), sglist.end());
	}

	static void removeSGFromCookie( PartInstance* pi, SurfaceGui* sg )
	{
		if( !pi )  return;

		if( !pi->getSurfaceGuiCookiesRead() )
			return;

		std::vector< weak_ptr< SurfaceGui > >* sglist = pi->getSurfaceGuiCookiesWrite();

		removeExpiredOrEqual(*sglist, shared_from(sg));
	}

	static void addSGToCookie( PartInstance* pi, SurfaceGui* sg )
	{
		if( !pi ) return;

		std::vector< weak_ptr< SurfaceGui > >* sglist = pi->getSurfaceGuiCookiesWrite();

		removeExpiredOrEqual(*sglist, weak_ptr<SurfaceGui>());

		sglist->push_back( shared_from(sg) );
	}

	void SurfaceGui::setAdornee(Instance* value)
	{
		if(adornee.lock().get() != value)
		{
			PartInstance* oldPart = fastDynamicCast<PartInstance>( adornee.lock().get() );
			PartInstance* newPart = fastDynamicCast<PartInstance>( value );

			removeSGFromCookie(oldPart,this);

			addSGToCookie(newPart,this);

			adornee = shared_from(value);
			raisePropertyChanged(prop_adornee);
			shouldRenderSetDirty();
		}
	}

	void SurfaceGui::onAncestorChanged(const AncestorChanged& event)
	{
		Super::onAncestorChanged(event);
		shouldRenderSetDirty();
		if (Workspace::serverIsPresent(this))
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendSurfaceGuiStats, flag);
		}
	}

	void SurfaceGui::onDescendantAdded(Instance* instance)
	{
		Super::onDescendantAdded(instance);
		if (instance->isA<TextLabel>()) return;
		if (Workspace::serverIsPresent(this))
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendSurfaceGUINonTextUsage, flag);
		}
	}

	void SurfaceGui::setFace( NormalId id )
	{
		if( id != faceID )
		{
			faceID = id;
			raisePropertyChanged(prop_Surface);
		}
	}

    void SurfaceGui::setToolPunchThroughDistance( float v )
    {
        if( v != toolPunchThroughDistance )
        {
            toolPunchThroughDistance = v;
            raisePropertyChanged(prop_maxDistance);
        }
    }

	SurfaceGui* SurfaceGui::findSurfaceGui( PartInstance* pi, NormalId surf )
	{
		if( !pi ) 
		{
			return 0;
		}

		if( const std::vector< weak_ptr< SurfaceGui > >* sglist = pi->getSurfaceGuiCookiesRead() )
		{
			for( size_t j=0, e = sglist->size(); j<e; ++j )
			{
				if( SurfaceGui* sg = (*sglist)[j].lock().get() )
				{
					if( sg->getFace() == surf && !sg->isYetAnotherSpecialCase() )
					{
						return sg;
					}
				}
			}
		}

		for( size_t j=0, e = pi->numChildren(); j<e; ++j )
		{
			if( SurfaceGui* sg = fastDynamicCast<SurfaceGui>( pi->getChild(j) ) )
			if( sg->getFace() == surf )
			{
				return sg;
			}
		}
		
		return 0;
	}

}
