#include "stdafx.h"

#include "V8DataModel/BillboardGui.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Filters.h"
#include "v8world/ContactManager.h"
#include "v8world/World.h"
#include "GfxBase/ViewportBillboarder.h"
#include "GfxBase/AdornBillboarder.h"
#include "GfxBase/AdornBillboarder2D.h"
#include "GfxBase/AdornSurface.h"
#include "Network/Player.h"
#include "Network/Players.h"

FASTFLAGVARIABLE(BillboardGuiVR, false)

namespace RBX {
	const char* const sAdornmentGui = "BillboardGui";
	const Reflection::RefPropDescriptor<BillboardGui, Instance>	prop_adornee("Adornee", category_Data, &BillboardGui::getAdorneeDangerous, &BillboardGui::setAdornee);
	Reflection::PropDescriptor<BillboardGui, Vector3> prop_studsOffset("StudsOffset", category_Data, &BillboardGui::getStudsOffset, &BillboardGui::setStudsOffset);
	Reflection::PropDescriptor<BillboardGui, Vector3> prop_extentsOffset("ExtentsOffset", category_Data, &BillboardGui::getExtentsOffset, &BillboardGui::setExtentsOffset);
	Reflection::PropDescriptor<BillboardGui, Vector2> prop_sizeOffset("SizeOffset", category_Data, &BillboardGui::getSizeOffset, &BillboardGui::setSizeOffset);
	Reflection::PropDescriptor<BillboardGui, UDim2>	  prop_Size("Size", category_Data, &BillboardGui::getSize, &BillboardGui::setSize);
	Reflection::PropDescriptor<BillboardGui, bool>	  prop_Enabled("Enabled", category_Data, &BillboardGui::getEnabled, &BillboardGui::setEnabled);
	Reflection::PropDescriptor<BillboardGui, bool>	  prop_Active("Active", category_Data, &BillboardGui::getActive, &BillboardGui::setActive);
	Reflection::PropDescriptor<BillboardGui, bool>	  prop_AlwaysOnTop("AlwaysOnTop", category_Data, &BillboardGui::getAlwaysOnTop, &BillboardGui::setAlwaysOnTop);
	const Reflection::RefPropDescriptor<BillboardGui, Instance>	  prop_LocalPlayerVisible("PlayerToHideFrom", category_Data, &BillboardGui::getPlayerToHideFrom, &BillboardGui::setPlayerToHideFrom);

	const float kBillboardGuiPixelsPerStudInVR = 20;

    static bool getBillboardFrame(const Camera* camera, const CoordinateFrame& cameraCFrame, const Vector3& partSize, const CoordinateFrame& partCFrame, const Vector3& partExtentRelativeOffset, const Vector3& partStudsOffset, const Vector2& billboardSizeRelativeOffset, const UDim2& billboardSize, bool vr, CoordinateFrame& result, Rect2D& viewport)
    {
		Extents localExtents = Extents::fromCenterCorner(Vector3(), partSize / 2);
		Extents cameraSpaceExtents = localExtents.toWorldSpace(cameraCFrame.inverse() * partCFrame);

        Vector3 relativeOffsetInCameraSpace = (partExtentRelativeOffset + Vector3::one()) * 0.5f * cameraSpaceExtents.size() + cameraSpaceExtents.min();
        Vector3 billboardCenterInCameraSpace = relativeOffsetInCameraSpace + partStudsOffset;

        Vector3 billboardCenterInWorldSpace = cameraCFrame.pointToWorldSpace(billboardCenterInCameraSpace);
    
        float pixelsPerStud = 1;
        
        if (vr)
        {
			pixelsPerStud = kBillboardGuiPixelsPerStudInVR;
		}
		else
		{
            Vector3 billboardCenterInCameraSpace = camera->project(billboardCenterInWorldSpace);

			if (billboardCenterInCameraSpace.z <= 0 || billboardCenterInCameraSpace.z > 1000)
                return false;

			pixelsPerStud = billboardCenterInCameraSpace.z;
        }

        float studsPerPixel = 1.f / pixelsPerStud;   // undo the perspective effect of finding extents in screenspace.

        //  -1 : to UI coordinates (0,0 upper left, )
        Vector2 billboardSizeInStuds = (billboardSize * Vector2(pixelsPerStud, pixelsPerStud)) * studsPerPixel;
        viewport = Rect2D(billboardSizeInStuds * pixelsPerStud);

        Vector2 UIToCameraSpaceScaler(billboardSizeInStuds / viewport.wh());

        billboardCenterInCameraSpace += Vector3(billboardSizeRelativeOffset * billboardSizeInStuds, 0);

        CoordinateFrame desiredModelView;
        desiredModelView.translation = billboardCenterInCameraSpace - Vector3(billboardSizeInStuds.x * 0.5f, billboardSizeInStuds.y * -0.5f, 0); 
        desiredModelView.rotation.set(UIToCameraSpaceScaler.x, 0,0,0,UIToCameraSpaceScaler.y, 0,0,0, 1);

    	result = cameraCFrame * desiredModelView;
        return true;
    }

	// This is Moeller–Trumbore ray-triangle intersection algorithm, adapted for quads
	// The only difference with quads is the barycentric coordinate boundary check.
	static bool rayQuad(const Vector3& raySource, const Vector3& rayDir, const Vector3& v0, const Vector3& v1, const Vector3& v2, float& t, float& u, float& v)
	{
		Vector3 edge1 = v1 - v0;
		Vector3 edge2 = v2 - v0;
		Vector3 P = rayDir.cross(edge2);

		float det = edge1.dot(P);

		if (det <= 0)
			return false;

		Vector3 T = raySource - v0;
		Vector3 Q = T.cross(edge1);

		t = edge2.dot(Q) / det;
		u = T.dot(P) / det;
		v = rayDir.dot(Q) / det;

		return t >= 0 && u >= 0 && u <= 1 && v >= 0 && v <= 1;
	}
    
    static bool getBillboardHit(Workspace* workspace, const Vector2& pos, const RbxRay& ray, const Vector2& viewport, const CoordinateFrame& projectionFrame, bool alwaysOnTop, Vector2& result)
    {
        Vector3 v0 = projectionFrame.pointToWorldSpace(Vector3(0, 0, 0));
        Vector3 v1 = projectionFrame.pointToWorldSpace(Vector3(0, -viewport.y, 0));
        Vector3 v2 = projectionFrame.pointToWorldSpace(Vector3(viewport.x, 0, 0));
        
        float t, u, v;
		if (!rayQuad(ray.origin(), ray.direction(), v0, v1, v2, t, u, v))
			return false;
        
		result.x = v * viewport.x;
		result.y = u * viewport.y;

        if (alwaysOnTop)
            return true;

        RbxRay searchRay = RbxRay::fromOriginAndDirection(ray.origin(), ray.direction() * 2048);

        FilterInvisibleNonColliding filter; // invisible & non-colliding parts don't block mouse-clicks

        Vector3 hitPoint;
        if (workspace->getWorld()->getContactManager()->getHit(searchRay, NULL, &filter, hitPoint) == NULL)
            return true;
        
        float wt = ray.direction().dot(hitPoint - ray.origin());
        
        return (t < wt);
    }

	BillboardGui::BillboardGui()
		:DescribedCreatable<BillboardGui, GuiLayerCollector, sAdornmentGui>("BillboardGui")
        ,IStepped(StepType_Render)
        ,alwaysOnTop(false)
		,enabled(true)
		,active(false)
        ,visibleAndValid(false)
	{
        if (!FFlag::BillboardGuiVR)
    		viewportBillboarder.reset(new ViewportBillboarder());
	}
	
	void BillboardGui::setRenderFunction(boost::function<void(BillboardGui*, Adorn*)> func) 
	{ 
		adornFunc = func; 
	}

	bool BillboardGui::askSetParent(const Instance* instance) const
	{
		return (Instance::fastDynamicCast<GuiBase2d>(instance) == NULL);
	}

    void BillboardGui::onStepped(const Stepped& event)
    {
        if (enabled)
        {
            shared_ptr<Instance> part = getPart();
            Workspace* workspace = ServiceProvider::find<Workspace>(part.get());

            if (part && workspace)
            {
                CoordinateFrame adornCFrame;
                Vector3 adornSize;

                calcAdornPlacement(part.get(), adornCFrame, adornSize);

                if (FFlag::BillboardGuiVR)
                {
                    const Camera* camera = workspace->getConstCamera();
                    CoordinateFrame cameraCFrame = camera->getRenderingCoordinateFrame();
                    
                    UserInputService* uis = ServiceProvider::find<UserInputService>(workspace);
                    
                    if (uis && uis->getVREnabled())
                    {
                        Vector3 aboveHead = adornCFrame.translation;
                        Vector3 look = cameraCFrame.translation - aboveHead;

                        CoordinateFrame cframe(aboveHead);
        
                        Vector3 heading = Vector3(look.x, 0, look.z);
        
                        cframe.lookAt(aboveHead - heading);
 
                        visibleAndValid = getBillboardFrame(camera, cframe, adornSize, adornCFrame, partExtentRelativeOffset, partStudsOffset, billboardSizeRelativeOffset, billboardSize, /* vr= */ true, projectionFrame, viewport);
                    }
                    else
                    {
                        visibleAndValid = getBillboardFrame(camera, cameraCFrame, adornSize, adornCFrame, partExtentRelativeOffset, partStudsOffset, billboardSizeRelativeOffset, billboardSize, /* vr= */ false, projectionFrame, viewport);
                    }

                    if (visibleAndValid)
                        handleResize(viewport, false);
                }
                else
                {
                    viewportBillboarder->partExtentRelativeOffset = partExtentRelativeOffset;
                    viewportBillboarder->partStudsOffset = partStudsOffset;
                    viewportBillboarder->billboardSizeRelativeOffset = billboardSizeRelativeOffset;
                    viewportBillboarder->billboardSize = billboardSize;
            
                    viewportBillboarder->alwaysOnTop = alwaysOnTop;

                    viewportBillboarder->update(workspace->getViewPort(), *workspace->getConstCamera(), adornSize, adornCFrame);

                    if (viewportBillboarder->isVisibleAndValid())
    					handleResize(viewportBillboarder->getViewport(), false);
                }
            }
        }
    }

	bool BillboardGui::shouldRender3dSortedAdorn() const
	{
		const shared_ptr<Instance> part(getPart());
		if (enabled && part)
			// only render if part is still in DataModel
			return DataModel::get(part.get()) ? true : false;
		else
			return false;
	}

	boost::shared_ptr<Instance> BillboardGui::getPart() const
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

	Vector3 BillboardGui::render3dSortedPosition() const
	{
		boost::shared_ptr<Instance> part = getPart();

        CoordinateFrame cframe;
        Vector3 size;
        calcAdornPlacement(part.get(), cframe, size);

        return cframe.translation;
	}

	void BillboardGui::render3dSortedAdorn(Adorn* adorn)
	{
        if (FFlag::BillboardGuiVR)
        {
    		if (!enabled || !visibleAndValid)
    			return;

    		boost::shared_ptr<Instance> part = getPart();
    		Workspace* workspace = ServiceProvider::find<Workspace>(part.get());
            
            if (!part || !workspace)
                return;

    		if (RBX::Network::Player* player = Network::Players::findLocalPlayer(DataModel::get(part.get())))
    			if (shared_ptr<Instance> noPlayerRender = playerToHideFrom.lock())
    				if (player == fastDynamicCast<RBX::Network::Player>(noPlayerRender.get()))
    					return;

            if (alwaysOnTop && !adorn->isVR())
            {
                Vector3 screenOffset = workspace->getConstCamera()->project(projectionFrame.translation);

                AdornBillboarder2D adornView(adorn, viewport, Math::roundVector2(screenOffset.xy()));
                Super::render2d(&adornView);
            }
            else
            {
                AdornBillboarder adornView(adorn, viewport, projectionFrame, alwaysOnTop);
                Super::render2d(&adornView);
            }
        }
        else
        {
    		if (adornFunc)
    		{
    			adornFunc(this, adorn);
    			return;
    		}

    		if (!enabled || !viewportBillboarder->isVisibleAndValid())
    			return;

    		boost::shared_ptr<Instance> part = getPart();
    		Workspace* workspace = ServiceProvider::find<Workspace>(part.get());

    		if (RBX::Network::Player* player = Network::Players::findLocalPlayer(DataModel::get(part.get())))
    			if (Instance* noPlayerRender = playerToHideFrom.lock().get())
    				if (player == fastDynamicCast<RBX::Network::Player>(noPlayerRender))
    					return;

    		if (part && workspace)
    		{
                if (getAlwaysOnTop() && !adorn->isVR())
                {
                    AdornBillboarder2D adornView(adorn, viewportBillboarder->getViewport(), viewportBillboarder->getScreenOffset());
                    Super::render2d(&adornView);
                }
                else
                {
                    AdornBillboarder adornView(adorn, *viewportBillboarder);
                    Super::render2d(&adornView);
                }
    		}
    	}
    }

	GuiResponse BillboardGui::process(const shared_ptr<InputObject>& event)
	{
        if (FFlag::BillboardGuiVR)
        {
    		if (!enabled || !active || !visibleAndValid)
                return GuiResponse::notSunk();
            
    		if (event->isMouseEvent())
            {
    			if (Workspace* workspace = ServiceProvider::find<Workspace>(this))
    			{
                    Vector2 pos = event->get2DPosition();
                    RbxRay ray = workspace->getCamera()->worldRay(pos.x, pos.y);
                    
                    Vector2 hit;
                    if (getBillboardHit(workspace, pos, ray, viewport.wh(), projectionFrame, alwaysOnTop, hit))
                    {
    					shared_ptr<InputObject> transformedEvent = RBX::Creatable<Instance>::create<InputObject>(*event);
    					transformedEvent->setPosition(Vector3(Math::roundVector2(hit), 0));
    					return Super::process(transformedEvent);
    				}
    			}
    		}

    		return GuiResponse::notSunk();
        }
        else
        {
    		if(!enabled || !active)
    			return GuiResponse::notSunk();

    		if(event->isMouseEvent()){
    			if(Workspace* workspace = ServiceProvider::find<Workspace>(this))
    			{
    				Vector2 billboardPosition;
    				if(viewportBillboarder->hitTest(event->get2DPosition(), event->getWindowSize(), workspace, billboardPosition))
    				{
    					shared_ptr<InputObject> transformedEvent = RBX::Creatable<Instance>::create<InputObject>(*event);
    					transformedEvent->setPosition( Vector3((int)billboardPosition.x, (int)billboardPosition.y, 0) );
    					return Super::process(transformedEvent);
    				}

    			}
    		}
    		return GuiResponse::notSunk();
    	}
    }

    bool BillboardGui::canProcessMeAndDescendants() const
    {
        return false;
    }

	void BillboardGui::setPlayerToHideFrom(Instance* value)
	{
		if (value && !fastDynamicCast<RBX::Network::Player>(value))
			throw std::runtime_error("HideFromPlayer can only be of type Player");

		if (playerToHideFrom.lock().get() != value)
		{
			playerToHideFrom = shared_from(value);
			raisePropertyChanged(prop_LocalPlayerVisible);
			shouldRenderSetDirty();
		}
	}

	void BillboardGui::setEnabled(bool value)
	{
		if (enabled != value)
		{
			enabled = value;
			raisePropertyChanged(prop_Enabled);
			shouldRenderSetDirty();
		}
	}

	void BillboardGui::setActive(bool value)
	{
		if (active != value)
		{
			active = value;
			raisePropertyChanged(prop_Active);
		}
	}

	void BillboardGui::setAdornee(Instance* value)
	{
		if (adornee.lock().get() != value)
        {
			adornee = shared_from(value);
			raisePropertyChanged(prop_adornee);
			shouldRenderSetDirty();
		}
	}

	void BillboardGui::onAncestorChanged(const AncestorChanged& event)
	{
		Super::onAncestorChanged(event);
		shouldRenderSetDirty();
	}

	const Vector3& BillboardGui::getStudsOffset() const 
	{ 
		return partStudsOffset;
	}

	void BillboardGui::setStudsOffset(const Vector3& value)
	{
		if (partStudsOffset != value)
        {
			partStudsOffset = value;
			raisePropertyChanged(prop_studsOffset);
		}
	}

	const Vector3& BillboardGui::getExtentsOffset() const 
	{ 
		return partExtentRelativeOffset;
	}

	void BillboardGui::setExtentsOffset(const Vector3& value)
	{
		if (partExtentRelativeOffset != value)
        {
			partExtentRelativeOffset = value;
			raisePropertyChanged(prop_extentsOffset);
		}
	}

	const Vector2& BillboardGui::getSizeOffset() const 
	{ 
		return billboardSizeRelativeOffset;
	}

	void BillboardGui::setSizeOffset(const Vector2& value)
	{
		if (billboardSizeRelativeOffset != value)
        {
			billboardSizeRelativeOffset = value;
			raisePropertyChanged(prop_sizeOffset);
		}
	}

	UDim2 BillboardGui::getSize() const 
	{ 
		return billboardSize;
	}

	void BillboardGui::setSize(UDim2 value)
	{
		if (billboardSize != value)
        {
			billboardSize = value;
			raisePropertyChanged(prop_Size);
		}
	}

	bool BillboardGui::getAlwaysOnTop() const 
	{ 
		return alwaysOnTop;
	}

	void BillboardGui::setAlwaysOnTop(bool value)
	{
		if (alwaysOnTop != value)
        {
			alwaysOnTop = value;
			raisePropertyChanged(prop_AlwaysOnTop);
		}
	}

    void BillboardGui::calcAdornPlacement(Instance* part, CoordinateFrame& cframe, Vector3& size) const
    {
        if (PartInstance* thePart = fastDynamicCast<PartInstance>(part))
        {
            cframe = thePart->calcRenderingCoordinateFrame();
            size = thePart->getPartSizeXml();
        }
        else if (ModelInstance* theModel = fastDynamicCast<ModelInstance>(part) )
        {
            cframe = theModel->calculateModelCFrame();
            size = theModel->calculateModelSize();
        }
        else
        {
            cframe = CoordinateFrame();
		    size = Vector3(0, 0, 0);
        }
    }
}
