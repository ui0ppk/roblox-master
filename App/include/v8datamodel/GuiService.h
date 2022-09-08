#pragma once 

#include "V8DataModel/InputObject.h"
#include "v8datamodel/GuiObject.h"
#include "V8Tree/Service.h"

namespace RBX {

	namespace Lua
	{
		class WeakFunctionRef;
	}

	extern const char* const sGuiService;
	class GuiService
		: public DescribedNonCreatable<GuiService, Instance, sGuiService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	public:
		typedef std::pair<weak_ptr<GuiObject>, shared_ptr<const Reflection::Tuple> > SelectionGroupPair;
		typedef boost::unordered_map<std::string, SelectionGroupPair> SelectionMap;

		static Reflection::RefPropDescriptor<GuiService, GuiObject> prop_selectedGuiObject;
		static Reflection::RefPropDescriptor<GuiService, GuiObject> prop_selectedCoreGuiObject;

	private:
		typedef DescribedNonCreatable<GuiService, Instance, sGuiService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;	

		Vector4 guiInset; // user's GUIs will be rendered inset by x,y in the top-left corner and by z,w in the bottom-right
        
        rbx::signals::scoped_connection screenGuiConnection;

		std::string errorMessage;
		std::string uiMessage;

		SelectionMap selectionMap;
        
        bool gamepadNavEnabled;
		bool coreGamepadNavEnabled;
		bool menuOpen;
        
        void getScreenResolutionNoRetry(boost::function<void(Vector2)> resumeFunction, boost::function<void(std::string)> errorFunction);
        
	public:

		int getBrickCount();
		shared_ptr<Instance> getClosestDialogToPosition(Vector3 position);

		// Show Stats based on Input
		bool showStatsBasedOnInputString(std::string inputText);

		bool getModalDialogStatus() const;
		bool getIsWindows() const {
#ifdef _WIN32
			return true;
#else
			return false; 
#endif
		};

		enum SpecialKey
		{
			KEY_INSERT   = 0,
			KEY_HOME     = 1,
			KEY_END      = 2, 
			KEY_PAGEUP   = 3,
			KEY_PAGEDOWN = 4,
			KEY_CHATHOTKEY = 5,
		};

		enum CenterDialogType
		{
			CENTER_DIALOG_UNSOLICITED_DIALOG				= 1,
			CENTER_DIALOG_PLAYER_INITIATED_DIALOG			= 2,
			CENTER_DIALOG_MODAL_DIALOG						= 3,
			CENTER_DIALOG_QUIT_DIALOG						= 4,
		};

		enum UiMessageType
		{
			UIMESSAGE_ERROR,
			UIMESSAGE_INFO
		};
	
		void addCenterDialog(shared_ptr<Instance> dialog, GuiService::CenterDialogType type, Lua::WeakFunctionRef show, Lua::WeakFunctionRef hide);
		void removeCenterDialog(shared_ptr<Instance> dialog);

		void openBrowserWindow(std::string url);

		GuiService();
		rbx::signal<void(std::string)> openUrlWindow;
		rbx::signal<void()> urlWindowClosed;

		rbx::signal<void(std::string, std::string)> keyPressed;
		rbx::signal<void()> escapeKeyPressed;
		rbx::signal<void(GuiService::SpecialKey, std::string)> specialKeyPressed;

		rbx::signal<void(std::string)> newErrorSignal;
		rbx::signal<void(UiMessageType, std::string)> newUiMessageSignal;

		rbx::signal<void()> showLeaveConfirmationSignal;

		rbx::signal<void()> menuOpenedSignal;
		rbx::signal<void()> menuClosedSignal;

		void fireMenuOpenedSignal() { menuOpenedSignal(); }
		void fireMenuClosedSignal() { menuClosedSignal(); }

		void setMenuOpen(bool value);
		bool getMenuOpen() const { return menuOpen; }

		void addKey(std::string);
		void removeKey(std::string);

		void addSpecialKey(GuiService::SpecialKey);
		void removeSpecialKey(SpecialKey key);
		bool dispatchKey(GuiService::SpecialKey);
		
		bool processKeyDown(const shared_ptr<RBX::InputObject>& event);

		
		const Vector4& getGlobalGuiInset() const { return guiInset; }
		void setGlobalGuiInset(int x1, int y1, int x2, int y2); // in pixels 
        
        Vector2 getScreenResolution();
        void getScreenResolutionLua(boost::function<void(Vector2)> resumeFunction, boost::function<void(std::string)> errorFunction);

		void setErrorMessage(std::string newErrorMessage);
		std::string getErrorMessage();
		void setUiMessage(UiMessageType msgType, std::string newErrorMessage);
		std::string getUiMessage();

		void toggleFullscreen();

		bool isTenFootInterface();

		// gamepad ui navigation features
		GuiObject* getSelectedGuiObjectLua() const;
		void setSelectedGuiObjectLua(GuiObject* value);

		GuiObject* getSelectedCoreGuiObjectLua() const;
		void setSelectedCoreGuiObjectLua(GuiObject* value);

		GuiObject* getSelectedGuiObject();

		void addSelectionGroup(std::string selectionGroupName, shared_ptr<Instance> selectionParent);
		void addSelectionGroup(std::string selectionGroupName, shared_ptr<const Reflection::Tuple> selectionTuple);

		void removeSelectionGroup(std::string selectionGroupName);

		SelectionGroupPair getSelectedObjectGroup(shared_ptr<GuiObject> object);
        
        bool getGamepadNavEnabled() const { return gamepadNavEnabled; }
        void setGamepadNavEnabled(bool value);

		bool getCoreGamepadNavEnabled() const { return coreGamepadNavEnabled; }
		void setCoreGamepadNavEnabled(bool value);

		bool getAutoGuiSelectionAllowed() const;
		void setAutoGuiSelectionAllowed(bool value);

		typedef boost::function<void(std::string, std::string, Reflection::AsyncCallbackDescriptor::ResumeFunction, Reflection::AsyncCallbackDescriptor::ErrorFunction)> NotificationCallback;
		NotificationCallback notificationCallback;

	protected:
		bool hasSpecial(SpecialKey key);

		struct DialogWrapper
		{
			weak_ptr<GuiObject> dialog;
			CenterDialogType dialogType;
			boost::function<void()> showFunction;
			boost::function<void()> hideFunction;
		};
		DialogWrapper* currentDialog;

		bool shouldPreemptCurrentDialog(DialogWrapper* newDialog) const;
		void queueDialogWrapper(DialogWrapper* newDialog, bool preempted);
		bool showWaitingDialog(CenterDialogType type);

		std::map<CenterDialogType, std::list<DialogWrapper*> > dialogQueue;
		std::map<boost::weak_ptr<GuiObject>, DialogWrapper*> dialogWrapperMap;

		// TODO: Perf: Make this a boost::array<char, 256>
		std::set<char> subscribedChars;
		std::set<SpecialKey> subscribedSpecials;
	};
}


