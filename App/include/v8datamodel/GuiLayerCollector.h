#pragma once

#include "Gui/GuiEvent.h"
#include "V8DataModel/InputObject.h"
#include "GuiBase.h"
#include "V8DataModel/GuiBase2d.h"
#include <boost/unordered_map.hpp>

namespace RBX {

	class Instance;
	class Adorn;
	class GuiObject;

	extern const char* const sLayerCollector;
	
	// Controls the rendering order of GUI elements
	class GuiLayerCollector	: public DescribedNonCreatable<GuiLayerCollector, GuiBase2d, sLayerCollector>
	{
	protected:
		GuiLayerCollector(const char* name);

	public:
		~GuiLayerCollector();

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiTarget
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event, bool sinkIfMouseOver = true);
        /*override*/ GuiResponse processGesture(const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args);

		void render2d(Adorn* adorn);
		void render2dContext(Adorn* adorn, const Instance* context);
        
        /*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);

		void getGuiObjectsForSelection(std::vector<GuiObject*>& guiObjects);

	private:
        typedef DescribedNonCreatable<GuiLayerCollector, GuiBase2d, sLayerCollector> Super;
        
		typedef std::vector<shared_ptr<GuiBase> > GuiVector;
		typedef std::vector<GuiVector> GuiLayers;

		bool rebuildGuiVector;
        
		static void LoadZ(const shared_ptr<RBX::Instance>& instance, GuiLayers guiVectors[]);
		void loadZVectors();

		void tryReleaseLastButtonDown(const shared_ptr<InputObject>& event);
		GuiResponse processDescendants(const shared_ptr<InputObject>& event);
        
        GuiResponse doProcessGesture(const boost::shared_ptr<GuiBase>& guiBase, const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args);

		void render2dStandardGuiElements(Adorn* adorn, const Instance* context, GuiVector& batch, const Rect2D& viewport);
		void render2dTextGuiElements(Adorn* adorn, const Instance* context, GuiVector& batch, const Rect2D& viewport);
        
        void descendantPropertyChanged(const shared_ptr<GuiBase>& gb, const Reflection::PropertyDescriptor* descriptor);
        
		GuiLayers mGuiVectors[RBX::GUIQUEUE_COUNT];		// temp arrays for rendering - never realloc, always fast clear
        
        boost::unordered_map<Instance*,rbx::signals::scoped_connection> propertyConnections;
	};

}
