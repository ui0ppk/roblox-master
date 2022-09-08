#pragma once

#include "V8DataModel/GuiBase2d.h"
#include "Util/HeartbeatInstance.h"
#include "Util/UDim.h"
#include "V8DataModel/GuiLayerCollector.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/PartInstance.h"



namespace RBX {
	class PartInstance;

	extern const char* const sAdornmentSurfaceGui;

	//A core window on which other windows are created
	//Ugh: this component is also a "PartAdornment", but we can't easily derive from both Adornment and GuiBase2d, resolve later.
	class SurfaceGui	: public DescribedCreatable<SurfaceGui, GuiLayerCollector, sAdornmentSurfaceGui>
	{
	private:
		typedef DescribedCreatable<SurfaceGui, GuiLayerCollector, sAdornmentSurfaceGui> Super;
	public:
		SurfaceGui();
		virtual ~SurfaceGui();

		// given a part and part side, finds a surface gui object attached to it
		static SurfaceGui* findSurfaceGui( PartInstance* pi, NormalId surf );

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ bool askSetParent(const Instance* instance) const;

		/*override*/ bool processMeAndDescendants() const {return false;}

		Instance*		getAdornee() const				{ return adornee.lock().get(); }
		void				setAdornee(Instance* value);

		bool getActive() const { return active; }
		void setActive(bool value);

		bool getEnabled() const { return enabled; }
		void setEnabled(bool value);

		void setCanvasSize( Vector2 s );
		Vector2 getCanvasSize() const { return canvasSize; }

		// point3d contains world space position of the ray hitting SG's parent
		GuiResponse process3d(const shared_ptr<InputObject>& event, Vector3 point3d, bool ignoreMaxDistance);

		void unProcess(); // notification when mouse leaves this particular GUI
		static int numInstances() { return s_numInstances; }

		void setFace( NormalId id );
		NormalId getFace() const { return faceID; }

        float getToolPunchThroughDistance() const { return toolPunchThroughDistance; }
        void setToolPunchThroughDistance(float val);
        

	private:
		boost::shared_ptr<Instance> getPart() const;

		bool enabled;
		bool active;
		Vector2 canvasSize;
		static int s_numInstances;
		NormalId faceID;
		boost::weak_ptr<Instance> adornee;
        float toolPunchThroughDistance; // <= 0 means unlimited
        
		bool buildGuiMatrix(Adorn* adorn, CoordinateFrame* partFrame, CoordinateFrame* projectionFrame);
		

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ virtual void onDescendantAdded(Instance* instance);

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
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
		/*override*/ bool canProcessMeAndDescendants() const;

		// This function always returns true, UNLESS the SG instance is under StarterGui AND the we're in game.
		// When the game starts, the all objects under StarterGUI are *copied* to player's section of the data model. 
		// That means, we'll have *two* SG instances attached to the same part, but only one of them will be active to user input, and
		// it is not clear which one. 
		// So this function is used to effectively disable the original SG instance and let the player copy do the job.
		bool isYetAnotherSpecialCase();
	};
}
