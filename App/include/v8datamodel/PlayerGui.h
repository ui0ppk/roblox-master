/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved  */

#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"

#include "V8DataModel/InputObject.h"
#include "V8DataModel/UserInputService.h"
#include "v8datamodel/GuiService.h"
#include "Script/IScriptFilter.h"
#include "Gui/GuiEvent.h"

namespace RBX {
    
    class GuiBase2d;
    class GuiObject;
    class ScreenGui;
	class ImageLabel;

	extern const char *const sBasePlayerGui;
	
	//A container used to hold data that replicates only between the server and the specific player
	class BasePlayerGui	: public DescribedNonCreatable<BasePlayerGui, Instance, sBasePlayerGui>
						, public IScriptFilter
	{
	private:
        GuiResponse processChildren(boost::function<GuiResponse(GuiBase2d*)> func);
        GuiResponse processInputOnChild(GuiBase2d* guiBase, const shared_ptr<InputObject>& event);
        
		typedef DescribedNonCreatable<BasePlayerGui, Instance, sBasePlayerGui> Super;

		bool mouseWasOverGui; // disregards active state

		// used by gamepad for ui selection
		weak_ptr<GuiObject> selectedGuiObject;

		// used for visual adornment of gamepad selected gui object
		shared_ptr<ImageLabel> defaultSelectionImage;

		GuiObject* checkForDefaultGuiSelection(const Vector2& direction, const Rect2D& currentRect, const shared_ptr<Instance> selectionAncestor);
		GuiObject* checkForTupleSelection(const Vector2& direction, const Rect2D& currentRect, const shared_ptr<const Reflection::Tuple>& selectionTuple);

		bool isCloserGuiObject(const Rect2D& currentRect, const Vector2& inputDirection, const GuiObject* guiObjectToTest, float& minIntersectDist, float& minProjection, const GuiObject* nextSelectionGuiObject);
	public:

		BasePlayerGui();
		~BasePlayerGui();
		virtual void append3dSortedAdorn(std::vector<AdornableDepth>& sortedAdorn, const Camera* camera) const;

		bool findModalGuiObject();

		bool getMouseWasOverGui() const { return mouseWasOverGui; }

		/*override*/ bool askAddChild(const Instance* instance) const;

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender2d() const {return true;}
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ bool shouldRender3d() const {return true;}
		/*override*/ bool shouldRender3dSortedAdorn() const {return true;}
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render3dAdorn(Adorn* adorn);

		void tryRenderSelectionFrame(Adorn* adorn);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiTarget
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
        
        GuiResponse processGesture(const UserInputService::Gesture gesture, shared_ptr<const RBX::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IScriptFilter
		/*override*/ bool scriptShouldRun(BaseScript* script);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);


		// used for gamepad selection
		GuiObject* selectNewGuiObject(const Vector2& direction);

		GuiObject* getSelectionImageObject() const { return selectionImageObject.get(); }
		virtual void setSelectionImageObject(GuiObject* value) = 0;

		shared_ptr<GuiObject> getSelectedObject() const { return selectedGuiObject.lock(); }
		void setSelectedObject(GuiObject* value);

	protected:
		boost::shared_ptr<IAdornableCollector> adornableCollector;		
		shared_ptr<GuiObject> selectionImageObject;
        
        GuiResponse processGestureOnChild(GuiBase2d* guiBase, const UserInputService::Gesture gesture, shared_ptr<const RBX::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args);
	};

	extern const char *const sPlayerGui;
	class PlayerGui
		: public DescribedCreatable<PlayerGui, BasePlayerGui, sPlayerGui, Reflection::ClassDescriptor::INTERNAL_PLAYER>
	{
	private:
		typedef DescribedCreatable<PlayerGui, BasePlayerGui, sPlayerGui, Reflection::ClassDescriptor::INTERNAL_PLAYER> Super;
        float topbarTransparency;
	public:
        PlayerGui();
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

        rbx::signal<void(float)> topbarTransparencyChangedSignal;
        
        void setTopbarTransparency(float transparency);
        float getTopbarTransparency(void);
        
		/*override*/ void setSelectionImageObject(GuiObject* value);
	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		///*override*/ void verifySetParent(const Instance* newParent) const;
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const;
	};

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// StarterGuiService
	// - the gui I bring with me in game
	//
	extern const char *const sStarterGuiService;
	class StarterGuiService
		: public DescribedNonCreatable<StarterGuiService, BasePlayerGui, sStarterGuiService>
		, public Service
	{	
	public:
		enum CoreGuiType
		{
			COREGUI_PLAYERLISTGUI	= 0, // this should ALWAYS be at the start! (make sure this is lowest number)
			COREGUI_HEALTHGUI		= 1,
			COREGUI_BACKPACKGUI		= 2,
			COREGUI_CHATGUI			= 3,
			COREGUI_ALL				= 4 // this should ALWAYS be at the end! (make sure this is highest number)
		};

	private:
		typedef DescribedNonCreatable<StarterGuiService, BasePlayerGui, sStarterGuiService> Super;
		boost::unordered_map<StarterGuiService::CoreGuiType,bool> coreGuiEnabledState;

		typedef boost::unordered_map<std::string, Lua::WeakFunctionRef> FunctionMap;
		FunctionMap setCoreFunctions;
		FunctionMap getCoreFunctions;

	public:

		rbx::signal<void(StarterGuiService::CoreGuiType,bool)> coreGuiChangedSignal;
		static const Reflection::PropDescriptor<StarterGuiService, bool> prop_ResetPlayerGui;

		StarterGuiService();
		bool getShowGui() const { return showGui; }
		void setShowGui(bool value);

		bool getResetPlayerGui() const { return resetPlayerGui;}
		void setResetPlayerGui(bool value);

		void setCoreGuiEnabled(CoreGuiType type, bool enabled);
		bool getCoreGuiEnabled(CoreGuiType type);

		void registerSetCore(std::string parameterName, Lua::WeakFunctionRef setFunction);
		void registerGetCore(std::string parameterName, Lua::WeakFunctionRef getFunction);
		void setCore(std::string parameterName, Reflection::Variant value);
		void getCore(std::string parameterName,  boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction);

		/*override*/ void setSelectionImageObject(GuiObject* value) {}

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// PlayerGui
		/*override*/ bool canClientCreate() { return true; }
		/*override*/ bool scriptShouldRun(BaseScript* script) { return false; }
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ void append3dSortedAdorn(std::vector<AdornableDepth>& sortedAdorn, const Camera* camera) const;
		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);
		
	protected:
		bool showGui;
		bool resetPlayerGui; 
	};

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	// CoreGuiService
	// - the gui I bring with me in game
	//
	extern const char *const sCoreGuiService;

	class CoreGuiService
		: public DescribedNonCreatable<CoreGuiService, BasePlayerGui, sCoreGuiService, Reflection::ClassDescriptor::INTERNAL_LOCAL, Security::Plugin>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<CoreGuiService, BasePlayerGui, sCoreGuiService, Reflection::ClassDescriptor::INTERNAL_LOCAL, Security::Plugin> Super;

	public:
		CoreGuiService();

		void createRobloxScreenGui();

		// for showing on screen messages, (duration == 0) for infinite time.
		void displayOnScreenMessage(int slot, const std::string &message, double duration);

		void clearOnScreenMessage(int slot);
		
		void setGuiVisibility(bool visible);

		// used to add/remove standard lua gui elements in (for ROBLOX in-game gui) -bt
		void addChild(Instance* instance);
		void removeChild(Instance* instance);
		void removeChild(const std::string &Name);
		Instance* findGuiChild(const std::string &Name);

		int getGuiVersion() const;
        
        shared_ptr<RBX::ScreenGui> getRobloxScreenGui();

		////////////////////////////////////////////////////////////////////////////////////
		//
		// Instance
		/*override*/ void onDescendantAdded(Instance* instance);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// PlayerGui
		/*override*/ bool canClientCreate() { return false; }
		/*override*/ bool scriptShouldRun(BaseScript* script) { return false; }
		/*override*/ void setSelectionImageObject(GuiObject* value);
	private:
		shared_ptr<RBX::Instance> screenGui;

		RBX::Instances onScreenMessages;
	};
}

