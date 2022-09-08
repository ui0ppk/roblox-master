#include "stdafx.h"

#include "VMProtect/VMProtectSDK.h"

#include "V8DataModel/GuiObject.h"
#include "V8DataModel/GuiService.h"
#include "V8DataModel/PlayerGui.h"
#include "v8datamodel/ScreenGui.h"
#include "V8DataModel/JointsService.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/ContextActionService.h"
#include "v8datamodel/GamepadService.h"
#include "Script/ScriptContext.h"
#include "Network/Players.h"
#include "V8DataModel/CollectionService.h"
#include "V8DataModel/DialogRoot.h" 
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Bindable.h"

#include <boost/algorithm/string.hpp>

FASTFLAGVARIABLE(NewInGameDevConsole, false)
FASTFLAGVARIABLE(UseNewSubdomainsInCoreScripts, false)
DYNAMIC_FASTFLAG(EnableShowStatsLua)
FASTFLAGVARIABLE(UseGameLoadedInLoadingScript, true)
FASTFLAGVARIABLE(UseUserListMenu, false)
FASTFLAGVARIABLE(EnableSetCoreTopbarEnabled, false)
FASTFLAGVARIABLE(Durango3DBackground, true)     // this is Xbox flag. Defined here so it is accessible in studio and xbox client

namespace RBX
{
	const char* const sGuiService = "GuiService";
    REFLECTION_BEGIN();
	static const Reflection::PropDescriptor<GuiService, bool> prop_IsModalDialog("IsModalDialog", category_Data, &GuiService::getModalDialogStatus, NULL, Reflection::PropertyDescriptor::Attributes::deprecated());
	static const Reflection::PropDescriptor<GuiService, bool> prop_IsWindows("IsWindows", category_Data, &GuiService::getIsWindows, NULL, Reflection::PropertyDescriptor::Attributes::deprecated());

	static Reflection::EventDesc<GuiService, void(std::string, std::string)> event_keypressed(&GuiService::keyPressed, "KeyPressed", "key", "modifiers", Security::RobloxScript);
	static Reflection::EventDesc<GuiService, void(GuiService::SpecialKey, std::string)> event_specialKeypressed(&GuiService::specialKeyPressed, "SpecialKeyPressed", "key", "modifiers", Security::RobloxScript);
	static Reflection::BoundFuncDesc<GuiService, void(std::string)> func_addKey(&GuiService::addKey, "AddKey", "key", Security::RobloxScript);
	static Reflection::BoundFuncDesc<GuiService, void(std::string)> func_removeKey(&GuiService::removeKey, "RemoveKey", "key", Security::RobloxScript);
	static Reflection::BoundFuncDesc<GuiService, void(GuiService::SpecialKey)> func_addSpecialKey(&GuiService::addSpecialKey, "AddSpecialKey", "key", Security::RobloxScript);
	static Reflection::BoundFuncDesc<GuiService, void(GuiService::SpecialKey)> func_removeSpecialKey(&GuiService::removeSpecialKey, "RemoveSpecialKey", "key", Security::RobloxScript);

	static Reflection::EventDesc<GuiService, void()> event_escapeKeypressed(&GuiService::escapeKeyPressed, "EscapeKeyPressed", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void(shared_ptr<Instance>, GuiService::CenterDialogType, Lua::WeakFunctionRef, Lua::WeakFunctionRef)> func_showCenterDialog(&GuiService::addCenterDialog, "AddCenterDialog", "dialog", "centerDialogType", "showFunction", "hideFunction", Security::RobloxScript);
	static Reflection::BoundFuncDesc<GuiService, void(shared_ptr<Instance>)> func_hideCenterDialog(&GuiService::removeCenterDialog, "RemoveCenterDialog", "dialog", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void(int, int, int, int)> func_globalGuiInset(&GuiService::setGlobalGuiInset, "SetGlobalGuiInset", "x1", "y1", "x2", "y2", Security::RobloxScript);
    
    static Reflection::BoundYieldFuncDesc<GuiService, Vector2(void)> func_getScreenResolution(&GuiService::getScreenResolutionLua, "GetScreenResolution", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void(std::string)> func_openBrowserWindow(&GuiService::openBrowserWindow, "OpenBrowserWindow", "url", Security::RobloxScript);
	static Reflection::EventDesc<GuiService, void()> event_urlWindowClosed(&GuiService::urlWindowClosed, "BrowserWindowClosed", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, shared_ptr<Instance>(Vector3)> func_GetClosestDialogToPosition(&GuiService::getClosestDialogToPosition, "GetClosestDialogToPosition", "position", Security::RobloxScript);
	
	static Reflection::BoundFuncDesc<GuiService, int()> func_GetBrickCount(&GuiService::getBrickCount, "GetBrickCount", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void(GuiService::UiMessageType, std::string)> func_setUiMessage(&GuiService::setUiMessage, "SetUiMessage", "msgType", "uiMessage", Security::LocalUser);
	static Reflection::BoundFuncDesc<GuiService, std::string()> func_getUiMessage(&GuiService::getUiMessage, "GetUiMessage", Security::RobloxScript);
	static Reflection::EventDesc<GuiService, void(GuiService::UiMessageType, std::string)> event_newUiMessage(&GuiService::newUiMessageSignal, "UiMessageChanged", "msgType", "newUiMessage", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void(std::string)> func_setErrorMessage(&GuiService::setErrorMessage, "SetErrorMessage", "errorMessage", Security::LocalUser, Reflection::Descriptor::Attributes::deprecated(func_setUiMessage));
	static Reflection::BoundFuncDesc<GuiService, std::string()> func_getErrorMessage(&GuiService::getErrorMessage, "GetErrorMessage", Security::RobloxScript, Reflection::Descriptor::Attributes::deprecated(func_getUiMessage));
	static Reflection::EventDesc<GuiService, void(std::string)> event_newError(&GuiService::newErrorSignal, "ErrorMessageChanged", "newErrorMessage", Security::RobloxScript, Reflection::Descriptor::Attributes::deprecated(event_newUiMessage));

	static Reflection::EventDesc<GuiService, void()> event_showLeaveConfirmation(&GuiService::showLeaveConfirmationSignal, "ShowLeaveConfirmation", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void()> func_toggleFullscreen(&GuiService::toggleFullscreen, "ToggleFullscreen", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, bool()> func_isTenFootInterface(&GuiService::isTenFootInterface, "IsTenFootInterface", Security::RobloxScript);

	static Reflection::BoundFuncDesc<GuiService, void(std::string, shared_ptr<Instance>)> func_addSelectionGroup(&GuiService::addSelectionGroup, "AddSelectionParent", "selectionName", "selectionParent", Security::None);
	static Reflection::BoundFuncDesc<GuiService, void(std::string, shared_ptr<const Reflection::Tuple>)> func_addSelectionGroupTuple(&GuiService::addSelectionGroup, "AddSelectionTuple", "selectionName", "selections", Security::None);

	static Reflection::BoundFuncDesc<GuiService, void(std::string)> func_removeSelectionGroup(&GuiService::removeSelectionGroup, "RemoveSelectionGroup", "selectionName", Security::None);

	Reflection::RefPropDescriptor<GuiService, GuiObject> GuiService::prop_selectedGuiObject("SelectedObject", category_Data, &GuiService::getSelectedGuiObjectLua, &GuiService::setSelectedGuiObjectLua);
    Reflection::RefPropDescriptor<GuiService, GuiObject> GuiService::prop_selectedCoreGuiObject("SelectedCoreObject", category_Data, &GuiService::getSelectedCoreGuiObjectLua, &GuiService::setSelectedCoreGuiObjectLua, Reflection::PropertyDescriptor::UI, Security::RobloxScript);
    
	static const Reflection::PropDescriptor<GuiService, bool> prop_autoGuiSelectionAllowed("AutoSelectGuiEnabled", category_Data, &GuiService::getAutoGuiSelectionAllowed, &GuiService::setAutoGuiSelectionAllowed);
    static const Reflection::PropDescriptor<GuiService, bool> prop_gamepadNavigationEnabled("GuiNavigationEnabled", category_Data, &GuiService::getGamepadNavEnabled, &GuiService::setGamepadNavEnabled);
	static const Reflection::PropDescriptor<GuiService, bool> prop_coreGamepadNavigationEnabled("CoreGuiNavigationEnabled", category_Data, &GuiService::getCoreGamepadNavEnabled, &GuiService::setCoreGamepadNavEnabled);

	static Reflection::BoundAsyncCallbackDesc<GuiService, void(std::string, std::string)> callback_notificationCallback("SendCoreUiNotification", &GuiService::notificationCallback, "title", "text", Security::RobloxScript);
	static const Reflection::PropDescriptor<GuiService, bool> prop_menuOpen("MenuIsOpen", category_Data, &GuiService::getMenuOpen, NULL);
	static const Reflection::BoundFuncDesc<GuiService, void(bool)> func_setMenuOpen(&GuiService::setMenuOpen, "SetMenuIsOpen", "open", Security::RobloxScript);
		
	static Reflection::EventDesc<GuiService, void()> event_menuOpened(&GuiService::menuOpenedSignal, "MenuOpened", Security::None);
	static Reflection::EventDesc<GuiService, void()> event_menuClosed(&GuiService::menuClosedSignal, "MenuClosed", Security::None);

	static Reflection::BoundFuncDesc<GuiService, bool(std::string)> func_ShowStatsBasedOnInputString(&GuiService::showStatsBasedOnInputString, "ShowStatsBasedOnInputString","input", Security::RobloxScript);
    REFLECTION_END();

	namespace Reflection {
	template<>
	EnumDesc<GuiService::SpecialKey>::EnumDesc()
		:EnumDescriptor("SpecialKey")
	{
		addPair(GuiService::KEY_INSERT, "Insert");
		addPair(GuiService::KEY_HOME, "Home");
		addPair(GuiService::KEY_END, "End");
		addPair(GuiService::KEY_PAGEUP, "PageUp");
		addPair(GuiService::KEY_PAGEDOWN, "PageDown");
		addPair(GuiService::KEY_CHATHOTKEY, "ChatHotkey");
	}

	template<>
	GuiService::SpecialKey& Variant::convert<GuiService::SpecialKey>(void)
	{
		return genericConvert<GuiService::SpecialKey>();
	}

	template<>
	EnumDesc<GuiService::CenterDialogType>::EnumDesc()
		:EnumDescriptor("CenterDialogType")
	{
		addPair(GuiService::CENTER_DIALOG_UNSOLICITED_DIALOG				, "UnsolicitedDialog");
		addPair(GuiService::CENTER_DIALOG_PLAYER_INITIATED_DIALOG			, "PlayerInitiatedDialog");
		addPair(GuiService::CENTER_DIALOG_MODAL_DIALOG						, "ModalDialog");
		addPair(GuiService::CENTER_DIALOG_QUIT_DIALOG						, "QuitDialog");
	}

	template<>
	GuiService::CenterDialogType& Variant::convert<GuiService::CenterDialogType>(void)
	{
		return genericConvert<GuiService::CenterDialogType>();
	}

	template<>
	EnumDesc<GuiService::UiMessageType>::EnumDesc()
		:EnumDescriptor("UiMessageType")
	{
		addPair(GuiService::UIMESSAGE_ERROR			, "UiMessageError");
		addPair(GuiService::UIMESSAGE_INFO			, "UiMessageInfo");
	}

	template<>
	GuiService::UiMessageType& Variant::convert<GuiService::UiMessageType>(void)
	{
		return genericConvert<GuiService::UiMessageType>();
	}
	}//namespace Reflection

	template<>
	bool StringConverter<GuiService::SpecialKey>::convertToValue(const std::string& text, GuiService::SpecialKey& value)
	{
		return Reflection::EnumDesc<GuiService::SpecialKey>::singleton().convertToValue(text.c_str(),value);
	}
	template<>
	bool StringConverter<GuiService::CenterDialogType>::convertToValue(const std::string& text, GuiService::CenterDialogType& value)
	{
		return Reflection::EnumDesc<GuiService::CenterDialogType>::singleton().convertToValue(text.c_str(),value);
	}
	template<>
	bool StringConverter<GuiService::UiMessageType>::convertToValue(const std::string& text, GuiService::UiMessageType& value)
	{
		return Reflection::EnumDesc<GuiService::UiMessageType>::singleton().convertToValue(text.c_str(),value);
	}


	GuiService::GuiService()
		:Super("GuiService")
		,currentDialog(NULL)
		,subscribedChars()
		,subscribedSpecials()
		,errorMessage("")
        ,gamepadNavEnabled(true)
		,coreGamepadNavEnabled(true)
		,menuOpen(false)
	{
	}

	void GuiService::setMenuOpen(bool value)
	{
		if (value != menuOpen)
		{
			menuOpen = value;
			menuOpen ? fireMenuOpenedSignal() : fireMenuClosedSignal();
			raisePropertyChanged(prop_menuOpen);
		}
	}

	int GuiService::getBrickCount()
	{
		return DataModel::get(this)->getNumPartInstances();
	}

	bool GuiService::shouldPreemptCurrentDialog(DialogWrapper* newDialog) const
	{
		if(currentDialog == NULL)
			return true;
		if(newDialog->dialogType > currentDialog->dialogType)
			return true;
		if(newDialog->dialogType < currentDialog->dialogType)
			return false;
		RBXASSERT(newDialog->dialogType == currentDialog->dialogType);

		switch(newDialog->dialogType){
			case CENTER_DIALOG_UNSOLICITED_DIALOG:
				return false;
			case CENTER_DIALOG_PLAYER_INITIATED_DIALOG:
			case CENTER_DIALOG_MODAL_DIALOG:						
			case CENTER_DIALOG_QUIT_DIALOG:
				return true;
			default:
				RBXASSERT(0);
				return false;
		}
	}
	bool GuiService::getModalDialogStatus() const
	{
		if(currentDialog == NULL)
			return false;
		switch(currentDialog->dialogType)
		{
			case CENTER_DIALOG_UNSOLICITED_DIALOG:
			case CENTER_DIALOG_PLAYER_INITIATED_DIALOG:
				return false;
			case CENTER_DIALOG_MODAL_DIALOG:						
			case CENTER_DIALOG_QUIT_DIALOG:
				return true;
			default:
				RBXASSERT(0);
				return false;
		}
	}
    
    Vector2 GuiService::getScreenResolution()
    {
        if (CoreGuiService* coreGui = ServiceProvider::create<CoreGuiService>(this))
        {
            if (ScreenGui* screenGui = coreGui->findFirstChildOfType<ScreenGui>())
            {
               return screenGui->getAbsoluteSize();
            }
        }
        return Vector2::zero();
    }
    
    void GuiService::getScreenResolutionNoRetry(boost::function<void(Vector2)> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
        screenGuiConnection.disconnect();
        resumeFunction(getScreenResolution());
    }
    
    void GuiService::getScreenResolutionLua(boost::function<void(Vector2)> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
        // if screen res is zero then we haven't done a heartbeat yet, try one more time after heartbeat
        if (getScreenResolution() == Vector2::zero())
        {
            if (RunService* runService = ServiceProvider::create<RunService>(this))
               screenGuiConnection = runService->heartbeatSignal.connect(boost::bind(&GuiService::getScreenResolutionNoRetry,this,resumeFunction,errorFunction));
        }
        else
        {
            getScreenResolutionNoRetry(resumeFunction,errorFunction);
        }
    }

	void GuiService::queueDialogWrapper(DialogWrapper* newDialog, bool preempted)
	{
		switch(newDialog->dialogType){
			case CENTER_DIALOG_UNSOLICITED_DIALOG:
				if(preempted)
					dialogQueue[newDialog->dialogType].push_front(newDialog);
				else
					dialogQueue[newDialog->dialogType].push_back(newDialog);
				break;
			case CENTER_DIALOG_PLAYER_INITIATED_DIALOG:
			case CENTER_DIALOG_MODAL_DIALOG:			
			case CENTER_DIALOG_QUIT_DIALOG:
				dialogQueue[newDialog->dialogType].push_front(newDialog);
				break;
		}
	}

	static void InvokeCallback(boost::weak_ptr<GuiObject> weakThis, Lua::WeakFunctionRef callback, bool visible)
	{
		if (shared_ptr<GuiObject> strongThis = weakThis.lock())
        {
			if (Lua::ThreadRef threadRef = callback.lock())
            {
				if (ScriptContext* sc = ServiceProvider::create<ScriptContext>(strongThis.get()))
                {
					Reflection::Tuple args;
                    
					try
					{
						sc->callInNewThread(callback, args);
						return;
					}
					catch (RBX::base_exception& e)
					{
                        StandardOut::singleton()->printf(MESSAGE_ERROR, "Unexpected error while invoking callback: %s", e.what());
                    }
				}
			}
            
			strongThis->setVisible(visible);
		}
	}

	void GuiService::addCenterDialog(shared_ptr<Instance> dialogInstance, GuiService::CenterDialogType type, Lua::WeakFunctionRef show, Lua::WeakFunctionRef hide)
	{
		shared_ptr<GuiObject> dialog = Instance::fastSharedDynamicCast<GuiObject>(dialogInstance);
		if(!dialog)
			throw RBX::runtime_error("dialog must be a GuiObject instance");

		weak_ptr<GuiObject> weakDialog = weak_from(dialog.get());
		std::map<boost::weak_ptr<GuiObject>, DialogWrapper*>::iterator iter = dialogWrapperMap.find(weakDialog);
		if(iter != dialogWrapperMap.end()){
			if( DialogWrapper* wrapper = iter->second){
				if(wrapper->dialogType != type){
					throw RBX::runtime_error("A dialogInstance should not change CenterDialogTypes");
				}

				if(wrapper == currentDialog){
					//Already on top -- ignore the request
					return;
				}
				else{
					//Not on top -- remove it, so we can readd it
					removeCenterDialog(dialogInstance);
				}
			}
			else{
				dialogWrapperMap.erase(iter);
			}
		}
		RBXASSERT(dialogWrapperMap.find(weakDialog) == dialogWrapperMap.end());
		
		std::list<boost::function<void()> > callbacks;
		
		DialogWrapper* wrapper = new DialogWrapper();
		wrapper->dialog = weakDialog;
		wrapper->dialogType = type;
		wrapper->showFunction = boost::bind(&InvokeCallback, weakDialog, show, true);
		wrapper->hideFunction = boost::bind(&InvokeCallback, weakDialog, hide, false);

		dialogWrapperMap[weakDialog] = wrapper;
		if(shouldPreemptCurrentDialog(wrapper)){
			if(currentDialog){
				//Hide/Cleanup old dialog
				DataModel::get(this)->submitTask(boost::bind(currentDialog->hideFunction), DataModelJob::Write);

				queueDialogWrapper(currentDialog, true);
				currentDialog = NULL;
			}

			currentDialog = wrapper;

			//Show new Dialog
			DataModel::get(this)->submitTask(boost::bind(currentDialog->showFunction), DataModelJob::Write);
		}
		else {
			queueDialogWrapper(wrapper, false);
		}
	}
	bool GuiService::showWaitingDialog(CenterDialogType type)
	{
		RBXASSERT(currentDialog == NULL);

		if(!dialogQueue[type].empty()){
			DialogWrapper* result = dialogQueue[type].front();
			dialogQueue[type].pop_front();

			currentDialog = result;

			return true;
		}
		return false;
	}
	void GuiService::removeCenterDialog(shared_ptr<Instance> dialogInstance)
	{
		shared_ptr<GuiObject> dialog = Instance::fastSharedDynamicCast<GuiObject>(dialogInstance);
		if(!dialog)
			throw RBX::runtime_error("dialog must be a GuiObject instance");
		
		weak_ptr<GuiObject> weakDialog = weak_from(dialog.get());
		if(DialogWrapper* wrapper = dialogWrapperMap[weakDialog]){
			dialogWrapperMap.erase(weakDialog);
			dialogQueue[wrapper->dialogType].remove(wrapper);

			if(wrapper == currentDialog){
				currentDialog = NULL;

				if(showWaitingDialog(CENTER_DIALOG_QUIT_DIALOG) || 
					showWaitingDialog(CENTER_DIALOG_MODAL_DIALOG) || 
					showWaitingDialog(CENTER_DIALOG_PLAYER_INITIATED_DIALOG) || 
					showWaitingDialog(CENTER_DIALOG_UNSOLICITED_DIALOG))
                {
					//Make it show
                    DataModel::get(this)->submitTask(boost::bind(currentDialog->showFunction), DataModelJob::Write);
				}
			}
			delete wrapper;
		}
	}

	bool GuiService::dispatchKey(GuiService::SpecialKey special)
	{
		if(hasSpecial(special)){
			specialKeyPressed(special, ""); 
			return true;
		} 
		return false;
	}
	
	bool GuiService::processKeyDown(const shared_ptr<RBX::InputObject>& event)
	{
		// ensure we are dealing with a key down
		RBXASSERT(event->isKeyDownEvent());

		if(event->isTextCharacterKey() || (UserInputService::IsUsingNewKeyboardEvents() && event->getUserInputType() == RBX::InputObject::TYPE_KEYBOARD))
		{
			if(!event->isAltEvent() && !event->isCtrlEvent())
			{
				if(event->modifiedKey == '/' || (UserInputService::IsUsingNewKeyboardEvents() && event->getKeyCode() == SDLK_SLASH) )
				{
					if(dispatchKey(KEY_CHATHOTKEY)) 
						return true;
				}
				else
				{
					char character = tolower(event->modifiedKey);
					if(subscribedChars.find(character) != subscribedChars.end())
					{
						keyPressed(std::string() + character,"");
						return true;
					}
				}
			}
		}
		else
			switch(event->getKeyCode())
			{
				case SDLK_INSERT:	if(dispatchKey(KEY_INSERT)) return true; break;
				case SDLK_HOME:		if(dispatchKey(KEY_HOME)) return true; break;
				case SDLK_END:		if(dispatchKey(KEY_END)) return true; break;
				case SDLK_PAGEUP:	if(dispatchKey(KEY_PAGEUP)) return true; break;
				case SDLK_PAGEDOWN:	if(dispatchKey(KEY_PAGEDOWN)) return true; break;
                default:
                    break;
			}
		return false;
	}	

	void GuiService::setGlobalGuiInset(int x1, int y1, int x2, int y2)
	{
		guiInset = Vector4(x1, y1, x2, y2);
	}

	void GuiService::addKey(std::string key)
	{
		if(key.size() != 1)
			throw RBX::runtime_error("GuiService:AddKey requires a string with a single character");
		char character = tolower(key[0]);

		if(subscribedChars.find(character) == subscribedChars.end()){
			subscribedChars.insert(character);
		}
	}
	void GuiService::removeKey(std::string key)
	{
		if(key.size() != 1)
			throw RBX::runtime_error("GuiService:RemoveKey requires a string with a single character");
		char character = tolower(key[0]);

		std::set<char>::iterator it = subscribedChars.find(character);
		if (it != subscribedChars.end())
			subscribedChars.erase(it);
	}
	bool GuiService::hasSpecial(SpecialKey key)
	{
		return subscribedSpecials.find(key) != subscribedSpecials.end();
	}
	void GuiService::addSpecialKey(SpecialKey key)
	{
		if(!hasSpecial(key)){
			subscribedSpecials.insert(key);
		}
	}
	void GuiService::removeSpecialKey(SpecialKey key)
	{
		std::set<SpecialKey>::iterator iter = subscribedSpecials.find(key);
		if (iter != subscribedSpecials.end())
		{
			subscribedSpecials.erase(iter);
		}
	}

	void GuiService::openBrowserWindow(std::string url)
	{
		if( !RBX::Http::isRobloxSite(url.c_str()) )
		{
			StandardOut::singleton()->print(MESSAGE_WARNING, "GuiService::OpenBrowserWindow() was called on non-Roblox url.");
			return;
		}
		if(!RBX::Network::Players::frontendProcessing(this))
		{
			StandardOut::singleton()->print(MESSAGE_WARNING, "GuiService::OpenBrowserWindow() was called on not a client (use local scripts on this call).");
			return;
		}

		openUrlWindow(url);
	}

	void GuiService::setErrorMessage(std::string newErrorMessage)
	{
		if (newErrorMessage.compare(errorMessage) != 0)
		{
			errorMessage = newErrorMessage;
			newErrorSignal(errorMessage);
		}
	}

	std::string GuiService::getErrorMessage()
	{
		return errorMessage;
	}

	void GuiService::setUiMessage(GuiService::UiMessageType msgType, std::string newUiMessage)
	{
		if (newUiMessage.compare(uiMessage) != 0)
		{
			uiMessage = newUiMessage;
			newUiMessageSignal(msgType, uiMessage);
		}
	}

	std::string GuiService::getUiMessage()
	{
		return uiMessage;
	}

	void GuiService::toggleFullscreen()
	{
		// People were modifying verbString when it was loaded into a register by using CE.
		VMProtectBeginMutation(NULL);

		std::string verbString = "ToggleFullScreen";
		Verb* verb = NULL;

		if(Workspace* workspace = ServiceProvider::find<Workspace>(this))
		{
#ifdef RBX_STUDIO_BUILD
			verb = workspace->getVerb(verbString);
#else
			verb = workspace->getWhitelistVerb(verbString);
			if (verb && verb->getVerbSecurity())
			{
				RBX::Tokens::simpleToken |= HATE_VERB_SNATCH;
				verb = NULL;
			}
#endif
		}

		if (verb && verb->isEnabled())
		{
			Verb::doItWithChecks(verb, RBX::DataModel::get(this));
		}

		VMProtectEnd();
	}

	bool GuiService::isTenFootInterface()
	{
		return UserInputService::isTenFootInterface();
	}

	void GuiService::setSelectedGuiObjectLua(GuiObject* value)
	{
		if (RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this))
		{
			if (RBX::Network::Player* player = players->getLocalPlayer()) 
			{
				if (PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>())
				{
					if (value != playerGui->getSelectedObject().get())
					{
						playerGui->setSelectedObject(value);
					}
				}
			}
		}
	}

	void GuiService::setSelectedCoreGuiObjectLua(GuiObject* value)
	{
		if (CoreGuiService* coreGuiService = ServiceProvider::find<CoreGuiService>(this))
		{
			if (value != coreGuiService->getSelectedObject().get())
			{
				coreGuiService->setSelectedObject(value);
			}
		}
	}
    
    GuiObject* GuiService::getSelectedGuiObjectLua() const
    {
		if (RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this))
		{
			if (RBX::Network::Player* player = players->getLocalPlayer()) 
			{
				if (PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>())
				{
					if (shared_ptr<GuiObject> selectedObject = playerGui->getSelectedObject())
					{
						return selectedObject.get();
					}
				}
			}
		}

		return NULL;
    }

	GuiObject* GuiService::getSelectedCoreGuiObjectLua() const
	{
		if (CoreGuiService* coreGuiService = ServiceProvider::find<CoreGuiService>(this))
		{
			if (shared_ptr<GuiObject> selectedCoreObject = coreGuiService->getSelectedObject())
			{
				return selectedCoreObject.get();
			}
		}

		return NULL;
	}

	GuiObject* GuiService::getSelectedGuiObject()
	{
		if (GuiObject* selectedCoreObject = getSelectedCoreGuiObjectLua())
		{
			return selectedCoreObject;
		}

		if (GuiObject* selectedObject = getSelectedGuiObjectLua())
		{
			return selectedObject;
		}

		return NULL;
	}

	void GuiService::setCoreGamepadNavEnabled(bool value)
	{
		if (value != coreGamepadNavEnabled)
		{
			coreGamepadNavEnabled = value;
			raisePropertyChanged(prop_coreGamepadNavigationEnabled);
		}
	}

    void GuiService::setGamepadNavEnabled(bool value)
    {
        if (value != gamepadNavEnabled)
        {
            gamepadNavEnabled = value;
            raisePropertyChanged(prop_gamepadNavigationEnabled);
        }
    }

	bool GuiService::getAutoGuiSelectionAllowed() const
	{
		if (GamepadService* gamepadService = RBX::ServiceProvider::create<GamepadService>(this))
		{
			return gamepadService->getAutoGuiSelectionAllowed();
		}

		return false;
	}
	void GuiService::setAutoGuiSelectionAllowed(bool value)
	{
		if (GamepadService* gamepadService = RBX::ServiceProvider::create<GamepadService>(this))
		{
			if (gamepadService->setAutoGuiSelectionAllowed(value))
			{
				raisePropertyChanged(prop_autoGuiSelectionAllowed);
			}
		}
	}

	void GuiService::addSelectionGroup(std::string selectionGroupName, shared_ptr<Instance> selectionParent)
	{
		shared_ptr<GuiObject> guiObjectParent = shared_from( Instance::fastDynamicCast<GuiObject>(selectionParent.get()) );
		if (!guiObjectParent)
		{
			RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "GuiService:AddSelectionParent for group name %s: parent is not a GuiObject.", selectionGroupName.c_str());
			return;
		}

		if (selectionMap.find(selectionGroupName) != selectionMap.end())
		{
			RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "GuiService:AddSelectionParent already has selection group with name %s, overwriting selection group.", selectionGroupName.c_str());
			removeSelectionGroup(selectionGroupName);
		}

		selectionMap[selectionGroupName] = SelectionGroupPair(guiObjectParent, shared_ptr<const Reflection::Tuple>());
	}
	void GuiService::addSelectionGroup(std::string selectionGroupName, shared_ptr<const Reflection::Tuple> selectionTuple)
	{
		if (selectionMap.find(selectionGroupName) != selectionMap.end())
		{
			RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "GuiService:AddSelectionTuple already has selection group with name %s, overwriting selection group.", selectionGroupName.c_str());
			removeSelectionGroup(selectionGroupName);
		}

		selectionMap[selectionGroupName] = SelectionGroupPair(weak_ptr<GuiObject>(), selectionTuple);
	}

	void GuiService::removeSelectionGroup(std::string selectionGroupName)
	{
		SelectionMap::iterator iter = selectionMap.find(selectionGroupName);
		if (iter != selectionMap.end())
		{
			selectionMap.erase(iter);
		}
	}

	GuiService::SelectionGroupPair GuiService::getSelectedObjectGroup(shared_ptr<GuiObject> object)
	{
		for (SelectionMap::iterator iter = selectionMap.begin(); iter != selectionMap.end(); ++iter)
		{
			if (shared_ptr<GuiObject> parentSelection = (*iter).second.first.lock())
			{
				if (object == parentSelection || object->isDescendantOf2(parentSelection))
				{
					return (*iter).second;
				}
			}

			if (shared_ptr<const Reflection::Tuple> guiObjects = (*iter).second.second)
			{
				if (guiObjects->values.size() > 0 )
				{
					Reflection::ValueArray guiObjectValues = guiObjects->values;
					for (Reflection::ValueArray::iterator tupleIter = guiObjectValues.begin(); tupleIter != guiObjectValues.end(); ++tupleIter)
					{
						if (tupleIter->isType<shared_ptr<Instance> >())
						{
							shared_ptr<Instance> selectableObject = tupleIter->cast<shared_ptr<Instance> >();
							if (selectableObject == object)
							{
								return (*iter).second;
							}
						}
					}
				}
			}
		}

		return SelectionGroupPair();
	}

	shared_ptr<Instance> GuiService::getClosestDialogToPosition(Vector3 position)
	{
		float smallestDistance = Math::inf();
		shared_ptr<Instance> currentClosest = shared_ptr<Instance>();
		
		if (CollectionService* collectionService = ServiceProvider::create<CollectionService>(this))
		{
			if (shared_ptr<const Instances> collection = collectionService->getCollection("Dialog"))
			{
				for (Instances::const_iterator iter = collection->begin(); iter != collection->end(); ++iter)
				{
					if (shared_ptr<DialogRoot> dialog = Instance::fastSharedDynamicCast<DialogRoot>(*iter))
					{
						if (shared_ptr<PartInstance> dialogParent = Instance::fastSharedDynamicCast<PartInstance>(shared_from(dialog->getParent())))
						{
							float distanceFromPosition = (dialogParent->getCoordinateFrame().translation - position).magnitude();
							if (distanceFromPosition < smallestDistance && distanceFromPosition < dialog->getConversationDistance())
							{
								smallestDistance = distanceFromPosition;
								currentClosest = dialog;
							}
						}
					}
				}
			}
		}
		return currentClosest;
	}

	static void doNothing(Reflection::Variant var) {}

	bool GuiService::showStatsBasedOnInputString(std::string inputText)
	{
		if (!DFFlag::EnableShowStatsLua)
			return false;
		const char * text = inputText.c_str();
		bool retVal = false;

		shared_ptr<DataModel> dataModel = shared_from(DataModel::get(this));

		if (!dataModel)
			return retVal;
	
		if (strcmp(text, "Genstats") == 0)
		{
			dataModel->getGuiBuilder().toggleGeneralStats();
			retVal = true;
		}
		else if (strcmp(text, "Renstats") == 0)
		{
			dataModel->getGuiBuilder().toggleRenderStats();
			retVal = true;
		}
		else if (strcmp(text, "Netstats") == 0)
		{
			dataModel->getGuiBuilder().toggleNetworkStats();
			retVal = true;
		}
		else if (strcmp(text, "Phystats") == 0)
		{
			dataModel->getGuiBuilder().togglePhysicsStats();
			retVal = true;
		}
		else if (strcmp(text, "Sumstats") == 0)
		{
			dataModel->getGuiBuilder().toggleSummaryStats();
			retVal = true;
		}
		else if (strcmp(text, "Cusstats") == 0)
		{
			dataModel->getGuiBuilder().toggleCustomStats();
			retVal = true;
		}
		else if (boost::iequals(text, "/console"))
		{
			if (CoreGuiService* cgs = dataModel->find<CoreGuiService>())
			{
				if (Instance* parent = cgs->findFirstChildByName("RobloxGui"))
				{
					if (Instance* controlFrame = parent->findFirstChildByName("ControlFrame"))
					{
						parent = controlFrame;
					}
					if (BindableFunction* toggle = Instance::fastDynamicCast<BindableFunction>(parent->findFirstChildByName("ToggleDevConsole")))
					{
						toggle->invoke(rbx::make_shared<Reflection::Tuple>(), 
							boost::bind(&doNothing, _1), boost::bind(&doNothing, _1));
					}
				}
			}
			retVal = true;
		}

		return retVal;
	}

}
