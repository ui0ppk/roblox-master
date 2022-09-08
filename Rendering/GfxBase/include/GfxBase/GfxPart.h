#pragma once

#include "boost/shared_ptr.hpp"
#include "Util/SpatialRegion.h"
#include "V8Tree/Instance.h"
#include "v8world/BasicSpatialHashPrimitive.h"
#include "rbx/signal.h"
#include "reflection/Property.h"

namespace RBX { 
	class PartInstance;
	class AsyncResult;

	class GfxBinding
	{
	protected:
		GfxBinding(const boost::shared_ptr<RBX::PartInstance>& part)
			: partInstance(part)
		{}

		GfxBinding()
		{}

		virtual ~GfxBinding();
	public:

		RBX::PartInstance* getPartInstance() { return partInstance.get(); };
		// unlinks from PartInstance.
		// will cause delete on next updateEntity();
		void zombify();

		bool isBound();

		// helper method. probably should be elsewhere.
		static bool isInWorkspace(RBX::Instance* part);

		virtual void invalidateEntity() {};
		virtual void updateEntity(bool assetsUpdated = false) {};
		virtual void updateChunk(const SpatialRegion::Id& pos, bool isWaterChunk) {};
		virtual void onCoordinateFrameChanged() {};
		virtual void onSizeChanged() { invalidateEntity(); };
		virtual void onTransparencyChanged() { invalidateEntity(); };
		virtual void onSpecialShapeChanged() { invalidateEntity(); }

		// disconnects all event listeners.
		virtual void unbind();
		void cleanupStaleConnections();

		// meant to connect all listeners relevant to this instance.
		// basic implementation doesn't listen to much. overide and bind some more.
//		virtual void bind();

		// helper: connects property change event listeners.
		void bindProperties(const shared_ptr<RBX::PartInstance>& part);

	protected:
		boost::shared_ptr<RBX::PartInstance> partInstance;
		std::vector<rbx::signals::connection> connections;

	private:
		void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor);
		void onAncestorChanged(const shared_ptr<RBX::Instance>& ancestor);
		void onChildAdded(const shared_ptr<RBX::Instance>& child);
		void onChildRemoved(const shared_ptr<RBX::Instance>& child);
		void onSpecialShapeChangedEx();
		
		void onCombinedSignal(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data);
		void onHumanoidChanged();
		void onOutfitChanged();
		void onDecalPropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor);
		void onTexturePropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor);
	};


	// class used as a simple base class for linking PartInstances with graphics objects.
	class GfxPart : public GfxBinding, public RBX::BasicSpatialHashPrimitive
	{
	public:
		GfxPart(const boost::shared_ptr<RBX::PartInstance>& part)
			: GfxBinding(part)
			, lastFrustumVisibleFrameNumber(-1)
		{}

		GfxPart()
			: lastFrustumVisibleFrameNumber(-1)
		{}

		// accessors?
		int lastFrustumVisibleFrameNumber; // most recent frame where this object was within the view frustum

	public:
		virtual void updateCoordinateFrame(bool recalcLocalBounds = false) {};
		virtual unsigned int getPartCount() { return 1; }

		virtual void onSleepingChanged(bool sleeping, PartInstance* part) {};
		virtual void onClumpChanged(PartInstance* part) {};
        
        virtual Vector3 getCenter() const { return Vector3(); }
	};

	// serves to allow the gfx engine to have persistent gfxobject tracking the position of a part.
	class GfxAttachment : public GfxBinding
	{
	public:
		GfxAttachment(const boost::shared_ptr<RBX::PartInstance>& part)
			: GfxBinding(part)
		{}
	protected:
		GfxAttachment()
		{}
	public:

		/*override*/ void unbind();
	protected:
		virtual void onSleepingChanged(bool sleeping) = 0;
	public:
		virtual void updateCoordinateFrame(bool recalcLocalBounds = false) = 0;
	};


}
