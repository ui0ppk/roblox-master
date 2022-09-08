#pragma once

#include "V8DataModel/GuiBase2d.h"
#include "Util/UDim.h"
#include "util/SteppedInstance.h"
#include "V8DataModel/GuiLayerCollector.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {
	class PartInstance;
	class ViewportBillboarder;

	extern const char* const sAdornmentGui;
	
	//A core window on which other windows are created
	//Ugh: this component is also a "PartAdornment", but we can't easily derive from both Adornment and GuiBase2d, resolve later.
	class BillboardGui
        : public DescribedCreatable<BillboardGui, GuiLayerCollector, sAdornmentGui>
        , public IStepped
	{
	private:
		typedef DescribedCreatable<BillboardGui, GuiLayerCollector, sAdornmentGui> Super;
		std::auto_ptr<ViewportBillboarder> viewportBillboarder;
	public:
		BillboardGui();

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ bool askSetParent(const Instance* instance) const;

		/*override*/ bool processMeAndDescendants() const {return false;}

		const Instance*		getAdornee() const				{ return adornee.lock().get(); }
		Instance*			getAdornee()					{ return adornee.lock().get(); }
		void				setAdornee(Instance* value);
		Instance*			getAdorneeDangerous() const		{ return adornee.lock().get(); }

		const Vector3& getStudsOffset() const;
		void setStudsOffset(const Vector3& value);

		const Vector3& getExtentsOffset() const;
		void setExtentsOffset(const Vector3& value);

		const Vector2& getSizeOffset() const;
		void setSizeOffset(const Vector2& value);

		UDim2 getSize() const;
		void setSize(UDim2 value);

		bool getAlwaysOnTop() const;
		void setAlwaysOnTop(bool value);
		
		bool getActive() const { return active; }
		void setActive(bool value);

		bool getEnabled() const { return enabled; }
		void setEnabled(bool value);
		
		void setRenderFunction(boost::function<void(BillboardGui*, Adorn*)> func);

		Instance* getPlayerToHideFrom() const { return playerToHideFrom.lock().get(); }
		void setPlayerToHideFrom(Instance* value);
	private:
		boost::shared_ptr<Instance> getPart() const;

		boost::function<void(BillboardGui*, Adorn*)> adornFunc;

        // reflected state
        Vector3 partExtentRelativeOffset;
        Vector3 partStudsOffset;
        Vector2 billboardSizeRelativeOffset;
        UDim2 billboardSize;
        
        bool alwaysOnTop;
		bool enabled;
		bool active;
        
        // dynamic state
        bool visibleAndValid;
        CoordinateFrame projectionFrame;
        Rect2D viewport;
        
        // reflected state
		boost::weak_ptr<Instance> playerToHideFrom;
		boost::weak_ptr<Instance> adornee;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			Super::onServiceProvider(oldProvider, newProvider);
            onServiceProviderIStepped(oldProvider, newProvider);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender3dSortedAdorn() const;
		/*override*/ void render3dSortedAdorn(Adorn* adorn);
		/*override*/ Vector3 render3dSortedPosition() const;
		/*override*/ bool isVisible(const Rect2D& rect) const { return true; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiTarget
		/*override*/ bool canProcessMeAndDescendants() const;
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
		
        // IStepped
		virtual void onStepped(const Stepped& event);

        void calcAdornPlacement(Instance* part, CoordinateFrame& cframe, Vector3& size) const;
	};
}
