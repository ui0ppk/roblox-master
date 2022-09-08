#pragma once

#include "V8DataModel/GuiObject.h"
#include "Util/HeartbeatInstance.h"
#include "Util/Rect.h"
#include "V8DataModel/GuiLayerCollector.h"

namespace RBX {

	extern const char* const sScreenGui;
	
    //todo: remove HeartbeatInstance when FFlag::ResizeGuiOnStep is true
	//A core 2d window on which other windows are created
	class ScreenGui	: public DescribedCreatable<ScreenGui, GuiLayerCollector, sScreenGui>
	{
	private:
		typedef DescribedCreatable<ScreenGui, GuiLayerCollector, sScreenGui> Super;
	public:
		ScreenGui();
        ~ScreenGui();

		void setReplicatingAbsoluteSize(Vector2int16 value);
		void setReplicatingAbsolutePosition(Vector2int16 value);
		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);
		/*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender2d() const { return renderable; }
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render2dContext(Adorn* adorn, const Instance* context);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiTarget
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
        /*override*/ GuiResponse processGesture(const UserInputService::Gesture gesture, shared_ptr<const RBX::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args);
        
        ////////////////////////////////////////////////////////////////////////////////////
        //
        // GuiBase2D
        /*override*/ Vector2 getAbsolutePosition() const;

		const Rect2D& getViewport() const { return bufferedViewport; }

		bool hasModalDialog();
		void onModalButtonChanged(const RBX::Reflection::PropertyDescriptor* desc, RBX::GuiButton* guiButton);

		/*override*/ bool canProcessMeAndDescendants() const;
        
        void setBufferedViewport(Rect2D newViewport);

	protected:
		ScreenGui(const char* name);

		/////////////////////////////////////////////////////////////
		// Instance
		//
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
	private:
		bool isAncestorRenderableGui() const;

		bool removeModalButton(RBX::GuiButton* guiButton);
		bool insertModalButton(RBX::GuiButton* guiButton);

		Rect2D bufferedViewport;		// grab on render (const - don't take action), resize on process;
		bool renderable;

		std::vector<GuiButton*> modalGuiObjects;
		boost::unordered_map<Instance*, rbx::signals::scoped_connection> connections;

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};




	//Legacy GuiMain object, deprecated
	extern const char* const sGuiMain;

	//A core window on which other windows are created
	class GuiMain	: public DescribedCreatable<GuiMain, ScreenGui, sGuiMain>
	{
	public:
		GuiMain();
	};

}
