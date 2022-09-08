/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */
//
//  ContextActionService.cpp
//  App
//
//  Created by Ben Tkacheff on 1/17/13.
//
//
#include "stdafx.h"

#include "V8Datamodel/ContextActionService.h"

#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/GuiService.h"
#include "V8Datamodel/Tool.h"
#include "V8Datamodel/UserInputService.h"
#include "v8datamodel/GuiObject.h"
#include "Script/ScriptContext.h"

#include "Network/Players.h"

DYNAMIC_FASTFLAGVARIABLE(TurnOffFakeEventsForCAS, false)

namespace RBX {

const char* const sContextActionService = "ContextActionService";

REFLECTION_BEGIN();
// tool convenience functions (todo: remove these)
static Reflection::BoundFuncDesc<ContextActionService, std::string()> func_getCurrentLocalToolIcon(&ContextActionService::getCurrentLocalToolIcon, "GetCurrentLocalToolIcon", Security::None);
    
static Reflection::EventDesc<ContextActionService, void(shared_ptr<Instance>)> event_LocalToolEquipped(&ContextActionService::equippedToolSignal, "LocalToolEquipped", "toolEquipped");
static Reflection::EventDesc<ContextActionService, void(shared_ptr<Instance>)> event_LocalToolUnequipped(&ContextActionService::unequippedToolSignal, "LocalToolUnequipped", "toolUnequipped");



// lua function to action binding (user facing functions)
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, Lua::WeakFunctionRef, bool, shared_ptr<const Reflection::Tuple>)> func_bindCore(&ContextActionService::bindCoreActionForInputTypes, "BindCoreAction","actionName","functionToBind","createTouchButton","inputTypes", Security::RobloxScript);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, Lua::WeakFunctionRef, bool, shared_ptr<const Reflection::Tuple>)> func_bind(&ContextActionService::bindActionForInputTypes, "BindAction","actionName","functionToBind","createTouchButton","inputTypes", Security::None);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, Lua::WeakFunctionRef, bool, shared_ptr<const Reflection::Tuple>)> func_bind_old(&ContextActionService::bindActionForInputTypes, "BindActionToInputTypes","actionName","functionToBind","createTouchButton","inputTypes", Security::None, Reflection::Descriptor::Attributes::deprecated(func_bind));

static Reflection::BoundFuncDesc<ContextActionService, void(InputObject::UserInputType, KeyCode)> func_bindActivate(&ContextActionService::bindActivate, "BindActivate", "userInputTypeForActivation", "keyCodeForActivation", SDLK_UNKNOWN, Security::None);
static Reflection::BoundFuncDesc<ContextActionService, void(InputObject::UserInputType, KeyCode)> func_unbindActivate(&ContextActionService::unbindActivate, "UnbindActivate", "userInputTypeForActivation", "keyCodeForActivation", SDLK_UNKNOWN, Security::None);

// touch button interface
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, std::string)> func_AddTitle(&ContextActionService::setTitleForAction, "SetTitle","actionName","title", Security::None);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, std::string)> func_AddDesc(&ContextActionService::setDescForAction, "SetDescription","actionName","description", Security::None);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, std::string)> func_AddImage(&ContextActionService::setImageForAction, "SetImage","actionName","image", Security::None);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, UDim2)> func_SetPosition(&ContextActionService::setPositionForAction, "SetPosition","actionName","position", Security::None);
static Reflection::BoundYieldFuncDesc<ContextActionService, shared_ptr<Instance>(std::string)> func_GetButton(&ContextActionService::getButton, "GetButton","actionName", Security::None);
    
// unbinding
static Reflection::BoundFuncDesc<ContextActionService, void(std::string)> func_unbindCoreAction(&ContextActionService::unbindCoreAction, "UnbindCoreAction", "actionName", Security::RobloxScript);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string)> func_unbindAction(&ContextActionService::unbindAction, "UnbindAction", "actionName", Security::None);
static Reflection::BoundFuncDesc<ContextActionService, void()> func_unbindAll(&ContextActionService::unbindAll, "UnbindAllActions", Security::None);
    
// bound info get
static Reflection::BoundFuncDesc<ContextActionService, shared_ptr<const Reflection::ValueTable>(std::string)> func_getBoundFunctionData(&ContextActionService::getBoundActionData, "GetBoundActionInfo", "actionName", Security::None);
static Reflection::BoundFuncDesc<ContextActionService, shared_ptr<const Reflection::ValueTable>()> func_getAllBoundFunctionData(&ContextActionService::getAllBoundActionData, "GetAllBoundActionInfo", Security::None);

// backend signals (for core scripts, not user exposed)
static Reflection::EventDesc<ContextActionService, void(std::string, std::string, shared_ptr<const Reflection::ValueTable>)> event_boundFunctionUpdated(&ContextActionService::boundActionChangedSignal, "BoundActionChanged", "actionChanged", "changeName" ,"changeTable", Security::RobloxScript);
static Reflection::EventDesc<ContextActionService, void(std::string, bool, shared_ptr<const Reflection::ValueTable>)> event_addedBoundFunction(&ContextActionService::boundActionAddedSignal, "BoundActionAdded", "actionAdded","createTouchButton","functionInfoTable", Security::RobloxScript);
static Reflection::EventDesc<ContextActionService, void(std::string, shared_ptr<const Reflection::ValueTable>)> event_removedBoundFunction(&ContextActionService::boundActionRemovedSignal, "BoundActionRemoved", "actionRemoved","functionInfoTable", Security::RobloxScript);
    
static Reflection::EventDesc<ContextActionService, void(std::string)> event_getActionButton(&ContextActionService::getActionButtonSignal, "GetActionButtonEvent", "actionName", Security::RobloxScript);
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, shared_ptr<Instance>)> func_actionButtonFound(&ContextActionService::fireActionButtonFoundSignal, "FireActionButtonFoundSignal", "actionName","actionButton", Security::RobloxScript);
    
static Reflection::BoundFuncDesc<ContextActionService, void(std::string, InputObject::UserInputState, shared_ptr<Instance>)> func_callFunction(&ContextActionService::callFunction, "CallFunction", "actionName", "state", "inputObject", Security::RobloxScript);
REFLECTION_END();

ContextActionService::ContextActionService() : Super()
{
    setName(sContextActionService);

	RBX::Guid::generateRBXGUID(activateGuid);
}
    
Tool* ContextActionService::getCurrentLocalTool()
{
    RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this);
    if (RBX::Network::Player* player = players->getLocalPlayer())
        if(ModelInstance* character = player->getCharacter())
            if( RBX::Tool* activeTool = character->findFirstChildOfType<RBX::Tool>() )
                return activeTool;
    
    return NULL;
}
    
std::string ContextActionService::getCurrentLocalToolIcon()
{
   if(Tool* activeTool = getCurrentLocalTool())
       return activeTool->getTextureId().c_str();
            
    return "";
}
    
void ContextActionService::disconnectAllCharacterConnections()
{
    characterChildAddConnection.disconnect();
    characterChildRemoveConnection.disconnect();
}
    
Tool* ContextActionService::isTool(shared_ptr<Instance> instance)
{
    if (Instance* baldInstance = instance.get())
        if (Tool* tool = Instance::fastDynamicCast<Tool>(baldInstance))
            return tool;
    
    return NULL;

}
    
void ContextActionService::checkForNewTool(shared_ptr<Instance> newChildOfCharacter)
{
    if (Tool* newTool = isTool(newChildOfCharacter))
        equippedToolSignal( shared_from(newTool) );
}
    
void ContextActionService::checkForToolRemoval(shared_ptr<Instance> removedChildOfCharacter)
{
    if (Tool* removedTool = isTool(removedChildOfCharacter))
        unequippedToolSignal( shared_from(removedTool) );
}
    
void ContextActionService::setupLocalCharacterConnections(ModelInstance* character)
{
    characterChildAddConnection = character->onDemandWrite()->childAddedSignal.connect( boost::bind(&ContextActionService::checkForNewTool, this, _1) );
    characterChildRemoveConnection = character->onDemandWrite()->childRemovedSignal.connect( boost::bind(&ContextActionService::checkForToolRemoval, this, _1) );
}
    
void ContextActionService::localCharacterAdded(shared_ptr<Instance> character)
{
    disconnectAllCharacterConnections();
    if (ModelInstance* characterModel = Instance::fastDynamicCast<ModelInstance>(character.get()))
        setupLocalCharacterConnections(characterModel);
}
    
void ContextActionService::checkForLocalPlayer(shared_ptr<Instance> newPlayer)
{
    if ( RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this) )
    {
        if ( RBX::Network::Player* player = Instance::fastDynamicCast<RBX::Network::Player>(newPlayer.get()) )
        {
            if ( player == players->getLocalPlayer() )
            {
                localPlayerAddConnection.disconnect();
                setupLocalPlayerConnections(player);
            }
        }
    }
}

void ContextActionService::setupLocalPlayerConnections(RBX::Network::Player* localPlayer)
{
    if(ModelInstance* character = localPlayer->getCharacter())
        localCharacterAdded(shared_from(character));
    else
        localPlayer->characterAddedSignal.connect( boost::bind(&ContextActionService::localCharacterAdded, this, _1) );
}
   
void ContextActionService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if(oldProvider)
    {
       // currently nothing needed to be done here (we disconnect on new adds)
    }
    
    Super::onServiceProvider(oldProvider, newProvider);

    if(newProvider)
    {
        RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(newProvider);
        if (RBX::Network::Player* player = players->getLocalPlayer())
            setupLocalPlayerConnections(player);
        else
            localPlayerAddConnection = players->onDemandWrite()->childAddedSignal.connect( boost::bind(&ContextActionService::checkForLocalPlayer,this,_1) );
    }
}

bool processInputForPlayerMovement(const PlayerActionType& playerMovement, const shared_ptr<InputObject>& inputObject)
{
	// todo: remove hard coded keys, allow users to set this
	if (inputObject->isKeyEvent())
	{
		switch (inputObject->getKeyCode())
		{
		case SDLK_w:
			{
				if (playerMovement == CHARACTER_FORWARD)
				{
					return true;
				}
				break;
			}
		case SDLK_UP:
			{
				if (playerMovement == CHARACTER_FORWARD)
				{
					return true;
				}
				break;
			}
		case SDLK_s:
			{
				if (playerMovement == CHARACTER_BACKWARD)
				{
					return true;
				}
				break;
			}
		case SDLK_DOWN:
			{
				if (playerMovement == CHARACTER_BACKWARD)
				{
					return true;
				}
				break;
			}
		case SDLK_a:
			{
				if (playerMovement == CHARACTER_LEFT)
				{
					return true;
				}
				break;
			}
		case SDLK_d:
			{
				if (playerMovement == CHARACTER_RIGHT)
				{
					return true;
				}
				break;
			}
		case SDLK_SPACE:
			{
				if (playerMovement == CHARACTER_JUMP)
				{
					return true;
				}
				break;
			}
		default:
			break;
		}
	}

	return false;
}
    
bool tupleContainsInputObject(shared_ptr<const Reflection::Tuple> inputBindings, const shared_ptr<InputObject>& inputObject, bool &sinkInput)
{
	sinkInput = true;
    if (inputBindings && inputBindings->values.size() > 0 )
    {
        bool inputObjectIsKeyEvent = inputObject->isKeyEvent() || inputObject->isGamepadEvent();
        Reflection::ValueArray inputBindingsValues = inputBindings->values;
        
        // loop thru all the input bindings and compare them with the input object
        for (Reflection::ValueArray::iterator tupleIter = inputBindingsValues.begin(); tupleIter != inputBindingsValues.end(); ++tupleIter)
        {
            // we have a key event, check and see if user passed in key code, or string that is key code
            if (inputObjectIsKeyEvent)
            {
                KeyCode keyCode = SDLK_UNKNOWN;
                
                if (tupleIter->isType<KeyCode>())
                {
                    keyCode = tupleIter->cast<KeyCode>();
                }
                else if (tupleIter->isString())
                {
                    std::string keyString = tupleIter->get<std::string>();
                    if (keyString.length() == 1)
                    {
                        keyCode = (KeyCode)keyString[0];
                    }
                }
                
                if (keyCode != SDLK_UNKNOWN && inputObject->getKeyCode() == keyCode)
                {
                    return true;
                }
                
                if(tupleIter->isType<InputObject::UserInputType>())
                {
                    InputObject::UserInputType inputType = tupleIter->cast<InputObject::UserInputType>();
                    if (inputType == inputObject->getUserInputType())
                    {
                        return true;
                    }
                }
				else if (tupleIter->isType<PlayerActionType>())
				{
					 PlayerActionType playerMovement = tupleIter->cast<PlayerActionType>();
					 if (processInputForPlayerMovement(playerMovement, inputObject))
					 {
						 sinkInput = false;
						 return true;
					 }
				}
            }
            // otherwise we might have a userinputtype, check against these
            else if(tupleIter->isType<InputObject::UserInputType>())
            {
                InputObject::UserInputType inputType = tupleIter->cast<InputObject::UserInputType>();
                if (inputType == inputObject->getUserInputType())
                {
                    return true;
                }
            }
        }
    }
    
	sinkInput = false;
    return false;
}

void ContextActionService::callFunction(boost::function<void(shared_ptr<Reflection::Tuple>)> luaFunction, const std::string actionName, const InputObject::UserInputState state, const shared_ptr<Instance> inputObject)
{
	if (!luaFunction.empty())
	{
		shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();
		args->values.push_back(actionName);
		args->values.push_back(state);
		args->values.push_back(inputObject);

		luaFunction(args);
	}
}

void ContextActionService::callFunction(const std::string actionName, const InputObject::UserInputState state, const shared_ptr<Instance> inputObject)
{
	FunctionMap::iterator iter = findAction(actionName);
	if(iter == functionMap.end())
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "ContextActionService::CallFunction does have a function for %s", actionName.c_str());
		return;
	}

	callFunction(iter->second.luaFunction, actionName, state, inputObject);
}

GuiResponse ContextActionService::tryProcess(shared_ptr<InputObject> inputObject, FunctionVector& funcVector, bool menuIsOpen)
{
	if (DFFlag::TurnOffFakeEventsForCAS && !inputObject->isPublicEvent())
		return GuiResponse::notSunk();
	if(!inputObject)
		return GuiResponse::notSunk();
	if(!inputObject.get())
		return GuiResponse::notSunk();

	for (FunctionVector::reverse_iterator iter = funcVector.rbegin(); iter != funcVector.rend(); ++iter)
	{
		if (iter->second.inputTypes && iter->second.inputTypes->values.size() > 0)
		{
			bool sink = false;
			if ( tupleContainsInputObject(iter->second.inputTypes, inputObject, sink) )
			{
				// first check to see if BindAction should call an action
				if (!iter->second.luaFunction.empty())
				{
					if ( shared_ptr<Instance> instance = shared_from(Instance::fastDynamicCast<Instance>(inputObject.get())) )
					{
						iter->second.lastInput =  weak_from(inputObject.get());
						callFunction(iter->second.luaFunction, iter->first, menuIsOpen ? InputObject::INPUT_STATE_CANCEL : inputObject->getUserInputState(), instance);
					}

					if (sink)
						return GuiResponse::sunk();

					return GuiResponse::notSunk();
				}
				// see if we should fire Activate from BindActivate
				else if (iter->first.compare(activateGuid) == 0)
				{
					bool canActivate = false;
					Reflection::ValueArray activateTuple = iter->second.inputTypes->values;
					if (!activateTuple.empty() && activateTuple.begin()->isType<InputObject::UserInputType>() &&
						activateTuple.begin()->get<InputObject::UserInputType>() == inputObject->getUserInputType())
					{
						if (activateTuple.size() == 1)
						{
							canActivate = true;
						}
						else if( activateTuple.back().isType<KeyCode>() && activateTuple.back().get<KeyCode>() == inputObject->getKeyCode())
						{
							canActivate = true;
						}
					}

					if (canActivate && !menuIsOpen)
					{
						if (UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(this))
						{
							Vector2 currentPos = inputService->getCurrentMousePosition()->get2DPosition();
							bool isDown = inputObject->getUserInputState() == InputObject::INPUT_STATE_BEGIN;
							shared_ptr<InputObject> simulateInputObject = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(InputObject::TYPE_MOUSEBUTTON1, isDown ? InputObject::INPUT_STATE_BEGIN : InputObject::INPUT_STATE_END,
								Vector3(currentPos.x,currentPos.y,0), Vector3::zero(), RBX::DataModel::get(this));

							// r2 and l2 are special kinds of input, they don't have simple up and down states
							if (inputObject->getKeyCode() == SDLK_GAMEPAD_BUTTONR2 || inputObject->getKeyCode() == SDLK_GAMEPAD_BUTTONL2)
							{
								float zPos = inputObject->getPosition().z;
								float lastZPos = 0.0f;
								InputObject* input = inputObject.get();

								if (lastZPositionsForActivate.find(input) != lastZPositionsForActivate.end())
								{
									lastZPos = lastZPositionsForActivate[input];
								}

								if (zPos >= 0.5 && lastZPos < 0.5)
								{
									simulateInputObject->setInputState(InputObject::INPUT_STATE_BEGIN);
									RBX::DataModel::get(this)->processInputObject(simulateInputObject);
								}
								else if(zPos <= 0.2 && lastZPos > 0.2)
								{
									simulateInputObject->setInputState(InputObject::INPUT_STATE_END);
									RBX::DataModel::get(this)->processInputObject(simulateInputObject);
								}

								lastZPositionsForActivate[input] = zPos;
							}
							else
							{
								RBX::DataModel::get(this)->processInputObject(simulateInputObject);
							}

						}
					}
				}
				// guess we have nothing, error out
				else
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "ContextActionService::CallFunction does have a function for %s", iter->first.c_str());
					return GuiResponse::notSunk();
				}
			}
		}
	}

	return GuiResponse::notSunk();
}

GuiResponse ContextActionService::processCoreBindings(const shared_ptr<InputObject>& inputObject)
{
	return tryProcess(inputObject, coreFunctionVector, false);
}

GuiResponse ContextActionService::processDevBindings(const shared_ptr<InputObject>& inputObject, bool menuIsOpen)
{
	return tryProcess(inputObject, functionVector, menuIsOpen);
}

static shared_ptr<Reflection::ValueTable> constructTableFromBoundFunctionData(const BoundFunctionData& functionData)
{
	shared_ptr<Reflection::ValueTable> functionTable = rbx::make_shared<Reflection::ValueTable>();
	if (!functionData.title.empty())
	{
		(*functionTable)["title"] = functionData.title;
	}
	if (!functionData.image.empty())
	{
		(*functionTable)["image"] = functionData.image;
	}
	if (!functionData.description.empty())
	{
		(*functionTable)["description"] = functionData.description;
	}
	if (functionData.hasTouchButton)
	{
		(*functionTable)["createTouchButton"] = functionData.hasTouchButton;
	}
	if (functionData.inputTypes)
	{
		shared_ptr<Reflection::ValueArray> inputArray = rbx::make_shared<Reflection::ValueArray>(functionData.inputTypes->values);
		(*functionTable)["inputTypes"] = Reflection::Variant(shared_ptr<const Reflection::ValueArray>(inputArray));
	}

	return functionTable;
}

void unbindActionInternal(const std::string actionName, FunctionMap& funcMap, FunctionVector& funcVector, rbx::signal<void(std::string, shared_ptr<const Reflection::ValueTable>)>& removedSignal)
{
	if (funcMap.find(actionName) != funcMap.end())
	{
		BoundFunctionData functionData = funcMap[actionName];

		shared_ptr<const Reflection::ValueTable> functionTable = constructTableFromBoundFunctionData(functionData);

		funcMap.erase(actionName);

		for (FunctionVector::iterator iter = funcVector.begin(); iter != funcVector.end(); ++iter)
		{
			if ( (*iter).first == actionName )
			{
				funcVector.erase(iter);
				break;
			}
		}

		// now fire a signal so we know a new function was removed (used at least by core scripts)
		removedSignal(actionName, functionTable);
	}
}

void ContextActionService::unbindCoreAction(const std::string actionName)
{
	unbindActionInternal(actionName, coreFunctionMap, coreFunctionVector, boundActionRemovedSignal);
}
    
void ContextActionService::unbindAction(const std::string actionName)
{
	unbindActionInternal(actionName, functionMap, functionVector, boundActionRemovedSignal);
}
    
void ContextActionService::unbindAll()
{
    for ( FunctionMap::iterator iter = functionMap.begin(); iter != functionMap.end(); ++iter )
    {
		shared_ptr<const Reflection::ValueTable> functionTable = constructTableFromBoundFunctionData(iter->second);

        boundActionRemovedSignal((iter->first),functionTable);
    }

    functionMap.clear();
	functionVector.clear();
}
static void InvokeCallback(weak_ptr<ContextActionService> weakActionService, Lua::WeakFunctionRef callback, shared_ptr<Reflection::Tuple> args)
{
    if(shared_ptr<ContextActionService> strongThis = weakActionService.lock())
    {
		if (Lua::ThreadRef threadRef = callback.lock())
		{
			if (ScriptContext* sc = ServiceProvider::create<ScriptContext>(strongThis.get()))
			{
				try
				{
					sc->callInNewThread(callback, *(args.get()));
				}
				catch (RBX::base_exception& e)
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "ContextActionService: Unexpected error while invoking callback: %s", e.what());
				}
			}
		}
    }
}

void ContextActionService::checkForInputOverride(const Reflection::Variant& newInputType, const FunctionVector& funcVector)
{
	for (FunctionVector::const_reverse_iterator iter = funcVector.rbegin(); iter != funcVector.rend(); ++iter)
	{
		if (iter->second.inputTypes && iter->second.inputTypes->values.size() > 0)
		{
			Reflection::ValueArray inputBindingsValues = iter->second.inputTypes->values;

			for (Reflection::ValueArray::const_iterator tupleIter = inputBindingsValues.begin(); tupleIter != inputBindingsValues.end(); ++tupleIter)
			{
				bool didOverride = false;

				if (newInputType.isType<KeyCode>() && tupleIter->isType<KeyCode>())
				{
					didOverride = (tupleIter->cast<KeyCode>() == newInputType.cast<KeyCode>());
				}
				else if(newInputType.isString() && tupleIter->isString())
				{
					didOverride = (tupleIter->cast<std::string>() == newInputType.cast<std::string>());
				}
				else if(newInputType.isType<InputObject::UserInputType>() && tupleIter->isType<InputObject::UserInputType>())
				{
					didOverride = (tupleIter->cast<InputObject::UserInputType>() == newInputType.cast<InputObject::UserInputType>());
				}
				else if(newInputType.isType<PlayerActionType>() && tupleIter->isType<PlayerActionType>())
				{
					didOverride = (tupleIter->cast<PlayerActionType>() == newInputType.cast<PlayerActionType>());
				}

				if (didOverride)
				{
					shared_ptr<InputObject> cancelInputObject = iter->second.lastInput.lock();
					InputObject::UserInputState prevInputState = InputObject::INPUT_STATE_NONE;

					if (cancelInputObject)
					{
						prevInputState = cancelInputObject->getUserInputState();
						cancelInputObject->setInputState(InputObject::INPUT_STATE_CANCEL);
					}
					else
					{
						cancelInputObject = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_NONE,
							RBX::InputObject::INPUT_STATE_CANCEL,
							RBX::Vector3::zero(),
							RBX::Vector3::zero(),
							RBX::DataModel::get(this));
					}


					if ( shared_ptr<Instance> instance = shared_from(Instance::fastDynamicCast<Instance>(cancelInputObject.get())) )
					{
						callFunction(iter->second.luaFunction, iter->first, InputObject::INPUT_STATE_CANCEL, instance);
					}

					cancelInputObject->setInputState(prevInputState);
					return;
				}
			}
		}
	}
}

void ContextActionService::bindActionInternal(const std::string actionName, Lua::WeakFunctionRef functionToBind, bool createTouchButton, shared_ptr<const Reflection::Tuple> hotkeys, FunctionMap& funcMap, FunctionVector& funcVector)
{
	if (!Network::Players::frontendProcessing(this))
	{
		throw RBX::runtime_error("ContextActionService:BindAction can only be called from a local script");
		return;
	}

	if (actionName.empty())
	{
		throw RBX::runtime_error("ContextActionService:BindAction called with an empty actionName");
		return;
	}

	if (hotkeys && !hotkeys->values.empty())
	{
		for(Reflection::ValueArray::const_iterator iter = hotkeys->values.begin(); iter != hotkeys->values.end(); ++iter)
		{
			if (!iter->isType<KeyCode>() && !iter->isString() && !iter->isType<InputObject::UserInputType>() && !iter->isType<PlayerActionType>())
			{
				throw RBX::runtime_error("ContextActionService:BindAction called with an invalid hotkey (should be either Enum.KeyCode, Enum.UserInputType or string)");
				return;
			}

			//Now check to see if this bind is overriding another bind action (so we can signal the action it is no longer active)

			// core ROBLOX overrides only happen when its another core bind
			if (funcVector == coreFunctionVector)
			{
				checkForInputOverride(*iter, coreFunctionVector);
			}
			// game developer overrides happen for all new binds
			checkForInputOverride(*iter, functionVector);
		}
	}

	if (funcMap.find(actionName) != funcMap.end())
	{
		if (funcMap == coreFunctionMap)
		{
			unbindCoreAction(actionName);
		}
		else
		{
			unbindAction(actionName);
		}
	}

	boost::function<void(shared_ptr<Reflection::Tuple>)> luaFunction = boost::bind(&InvokeCallback,shared_from(this),functionToBind, _1);

	BoundFunctionData data;
	if (hotkeys)
	{
		data = BoundFunctionData(hotkeys, luaFunction, createTouchButton);
	}
	else
	{
		data = BoundFunctionData(luaFunction, createTouchButton);
	}

	funcMap[actionName] = data;
	funcVector.push_back( std::pair<std::string, BoundFunctionData>(actionName,data) );

	// now fire a signal so we know a new function was bound (used by core scripts)
	shared_ptr<Reflection::ValueTable> functionTable(rbx::make_shared<Reflection::ValueTable>());

	(*functionTable)["title"] = RBX::Reflection::Variant();
	(*functionTable)["image"] = RBX::Reflection::Variant();
	(*functionTable)["description"] = RBX::Reflection::Variant();
	(*functionTable)["createTouchButton"] = RBX::Reflection::Variant(createTouchButton);
	if (hotkeys && !hotkeys->values.empty())
	{
		shared_ptr<Reflection::ValueArray> hotkeyArray = rbx::make_shared<Reflection::ValueArray>(hotkeys->values);
		(*functionTable)["inputTypes"] = shared_ptr<const RBX::Reflection::ValueArray>(hotkeyArray);
	}

	boundActionAddedSignal(actionName,createTouchButton,functionTable);
}

void ContextActionService::bindCoreActionForInputTypes(const std::string actionName, Lua::WeakFunctionRef functionToBind, bool createTouchButton, shared_ptr<const Reflection::Tuple> hotkeys)
{
	bindActionInternal(actionName, functionToBind, createTouchButton, hotkeys, coreFunctionMap, coreFunctionVector);
}


void ContextActionService::bindActionForInputTypes(const std::string actionName, Lua::WeakFunctionRef functionToBind, bool createTouchButton, shared_ptr<const Reflection::Tuple> hotkeys)
{
	bindActionInternal(actionName, functionToBind, createTouchButton, hotkeys, functionMap, functionVector);
}

void ContextActionService::bindActivate(InputObject::UserInputType inputType, KeyCode keyCode)
{
	if (!Network::Players::frontendProcessing(this))
	{
		throw RBX::runtime_error("ContextActionService:BindActivate can only be called from a local script");
	}

	if (inputType == InputObject::TYPE_NONE)
	{
		throw RBX::runtime_error("ContextActionService:BindActivate called with Enum.UserInputType.None, must be some other input type.");
	}

	unbindActivate(inputType, keyCode);

	shared_ptr<Reflection::Tuple> activateTuple = rbx::make_shared<Reflection::Tuple>();
	activateTuple->values.push_back(inputType);
	if (keyCode != SDLK_UNKNOWN)
	{
		activateTuple->values.push_back(keyCode);
	}

	functionVector.push_back( std::pair<std::string, BoundFunctionData>(activateGuid, BoundFunctionData(activateTuple)) );
}

void ContextActionService::unbindActivate(InputObject::UserInputType inputType, KeyCode keyCode)
{
	if (!Network::Players::frontendProcessing(this))
	{
		throw RBX::runtime_error("ContextActionService:UnbindActivate can only be called from a local script");
	}

	if (inputType == InputObject::TYPE_NONE)
	{
		throw RBX::runtime_error("ContextActionService:UnbindActivate called with Enum.UserInputType.None, must be some other input type.");
	}

	for (FunctionVector::reverse_iterator iter = functionVector.rbegin(); iter != functionVector.rend(); ++iter)
	{
		if ( (*iter).first == activateGuid )
		{
			Reflection::ValueArray currentInputTypes = (*iter).second.inputTypes->values;
			if (!currentInputTypes.empty() && currentInputTypes.begin()->isType<InputObject::UserInputType>())
			{
				if ( inputType == currentInputTypes.begin()->get<InputObject::UserInputType>() )
				{
					functionVector.erase(--iter.base());
					break;
				}
			}
		}
	}
}
    
FunctionMap::iterator ContextActionService::findAction(const std::string actionName)
{
    FunctionMap::iterator iter = functionMap.find(actionName);
    if (iter == functionMap.end())
    {
        StandardOut::singleton()->printf(MESSAGE_WARNING, "ContextActionService could not find the function passed in, doing nothing.");
    }
    
    return iter;
}
    
void ContextActionService::fireBoundActionChangedSignal(FunctionMap::iterator iter, const std::string& changeName)
{
    if (iter == functionMap.end())
    {
        return;
    }
	
	shared_ptr<Reflection::ValueTable> functionTable = constructTableFromBoundFunctionData(iter->second);
    (*functionTable)["position"] = iter->second.position;
    
    boundActionChangedSignal((iter->first), changeName, functionTable);
}
    
void ContextActionService::setTitleForAction(const std::string actionName, std::string title)
{
    FunctionMap::iterator iter = findAction(actionName);
    if (iter == functionMap.end())
    {
        return;
    }
    
    iter->second.title = title;
    fireBoundActionChangedSignal(iter, "title");
}
void ContextActionService::setDescForAction(const std::string actionName, std::string description)
{
    FunctionMap::iterator iter = findAction(actionName);
    if (iter == functionMap.end())
    {
        return;
    }
    
    iter->second.description = description;
    fireBoundActionChangedSignal(iter, "description");
}
    
void ContextActionService::setImageForAction(const std::string actionName, std::string image)
{
    FunctionMap::iterator iter = findAction(actionName);
    if (iter == functionMap.end())
    {
        return;
    }
    
    iter->second.image = image;
    fireBoundActionChangedSignal(iter, "image");
}
    
void ContextActionService::setPositionForAction(const std::string actionName, UDim2 position)
{
    FunctionMap::iterator iter = findAction(actionName);
    if (iter == functionMap.end())
    {
        return;
    }
    
    iter->second.position = position;
    fireBoundActionChangedSignal(iter, "position");
}
    
void ContextActionService::getButton(const std::string actionName, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    FunctionMap::iterator iter = findAction(actionName);
    if(iter == functionMap.end())
    {
        return;
    }
    
    yieldFunctionMap[actionName] = FunctionPair(resumeFunction,errorFunction);
    getActionButtonSignal(actionName);
}
    
void ContextActionService::fireActionButtonFoundSignal(const std::string actionName, shared_ptr<Instance> actionButton)
{
    try
    {
        yieldFunctionMap.at(actionName);
    }
    catch (std::exception& e)
    {
        StandardOut::singleton()->print(MESSAGE_ERROR, e.what());
        return;
    }
    
    FunctionPair functionPair = yieldFunctionMap[actionName];
    functionPair.first(actionButton);
}


shared_ptr<const Reflection::ValueTable> ContextActionService::getAllBoundActionData()
{
    shared_ptr<Reflection::ValueTable> reflectedFuncMap = rbx::make_shared<Reflection::ValueTable>();
    
    for(FunctionMap::iterator iter = functionMap.begin(); iter != functionMap.end(); ++ iter)
    {
        if (iter != functionMap.end())
        {
            shared_ptr<const Reflection::ValueTable> functionInfo = getBoundActionData(iter->first);

            Reflection::Variant variantValue = shared_ptr<const RBX::Reflection::ValueTable>(functionInfo);
            (*reflectedFuncMap)[iter->first] = variantValue;
        }
    }
    
    return  reflectedFuncMap;
}
    
shared_ptr<const Reflection::ValueTable> ContextActionService::getBoundCoreActionData(const std::string actionName)
{
    FunctionMap::iterator iter = coreFunctionMap.find(actionName);
    if (iter != coreFunctionMap.end())
    {
        return constructTableFromBoundFunctionData(iter->second);
    }
    
    return rbx::make_shared<Reflection::ValueTable>();
}
    
shared_ptr<const Reflection::ValueTable> ContextActionService::getBoundActionData(const std::string actionName)
{
    FunctionMap::iterator iter = functionMap.find(actionName);
    if (iter != functionMap.end())
    {
		return constructTableFromBoundFunctionData(iter->second);
    }
    
    return rbx::make_shared<Reflection::ValueTable>();
}

namespace Reflection {
	template<>
	EnumDesc<PlayerActionType>::EnumDesc()
		:EnumDescriptor("PlayerActions")
	{
		addPair(CHARACTER_FORWARD, "CharacterForward");
		addPair(CHARACTER_BACKWARD, "CharacterBackward");
		addPair(CHARACTER_LEFT, "CharacterLeft");
		addPair(CHARACTER_RIGHT, "CharacterRight");
		addPair(CHARACTER_JUMP, "CharacterJump");
	}
} // Namespace Reflection
    
} // Namespace RBX