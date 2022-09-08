/* Copyright 2003-2013 ROBLOX Corporation, All Rights Reserved */
//
//  ContextActionService.h
//  App
//
//  Created by Ben Tkacheff on 1/17/13.
//
// This service is used to figure out what actions a local player can do (right now this is just for tools, but could expand to several functions)

#pragma once

#include "Util/TextureId.h"
#include "Util/UDim.h"

#include "gui/GuiEvent.h"

#include "V8Tree/Service.h"

#include "script/ThreadRef.h"

namespace RBX {
    
    namespace Network
    {
        class Player;
    }
    
    class ModelInstance;
    class Tool;
    class GuiButton;
    
    struct BoundFunctionData
    {
        std::string title;
        std::string description;
        std::string image;
        UDim2 position;
        bool hasTouchButton;

        shared_ptr<const Reflection::Tuple> inputTypes;
        boost::function<void(shared_ptr<Reflection::Tuple>)> luaFunction;

		weak_ptr<InputObject> lastInput;
        
        BoundFunctionData()
        : image("")
        , inputTypes()
        , title("")
        , description("")
        , position()
        , hasTouchButton(false)
        { }

		BoundFunctionData(shared_ptr<const Reflection::Tuple> newInputTypes)
			: inputTypes(newInputTypes)
			, image("")
			, title("")
			, description("")
			, position()
			, hasTouchButton(false)
		{ }
        
        BoundFunctionData(shared_ptr<const Reflection::Tuple> newInputTypes, boost::function<void(shared_ptr<Reflection::Tuple>)> newFunction, bool touchButton)
        : inputTypes(newInputTypes)
        , image("")
        , title("")
        , description("")
        , position()
        , luaFunction(newFunction)
        , hasTouchButton(touchButton)
        { }
        
        BoundFunctionData(boost::function<void(shared_ptr<Reflection::Tuple>)> newFunction, bool touchButton)
        : inputTypes()
        , image("")
        , title("")
        , description("")
        , position()
        , luaFunction(newFunction)
        , hasTouchButton(touchButton)
        { }

		friend bool operator==(const BoundFunctionData& lhs, const BoundFunctionData& rhs)
		{ 
			if (lhs.inputTypes && rhs.inputTypes)
			{
				return (lhs.inputTypes.get() == rhs.inputTypes.get()) && (lhs.image == rhs.image) && (lhs.title == rhs.title) 
					&& (lhs.description == rhs.description) && (lhs.position == rhs.position) && (lhs.hasTouchButton == rhs.hasTouchButton);
			}

			return (lhs.image == rhs.image) && (lhs.title == rhs.title) && (lhs.description == rhs.description)
				&& (lhs.position == rhs.position) && (lhs.hasTouchButton == rhs.hasTouchButton);
		}
    };
    
    typedef boost::unordered_map<std::string, BoundFunctionData> FunctionMap;
    typedef std::vector<std::pair<std::string,BoundFunctionData> > FunctionVector;

    typedef std::pair<boost::function<void(shared_ptr<Instance>)>, boost::function<void(std::string)> > FunctionPair;
    typedef boost::unordered_map<std::string, FunctionPair> FunctionPairMap;

	typedef enum
	{
		CHARACTER_FORWARD = 0,
		CHARACTER_BACKWARD = 1,
		CHARACTER_LEFT = 2,
		CHARACTER_RIGHT = 3,
		CHARACTER_JUMP = 4
	} PlayerActionType;
    
	extern const char* const sContextActionService;
	class ContextActionService : public DescribedNonCreatable<ContextActionService, Instance, sContextActionService>
        , public Service
	{
	public:
		ContextActionService();
        
        rbx::signal<void(shared_ptr<Instance>)> equippedToolSignal;
        rbx::signal<void(shared_ptr<Instance>)> unequippedToolSignal;
        
        rbx::signal<void(std::string, std::string, shared_ptr<const Reflection::ValueTable>)>  boundActionChangedSignal;
        rbx::signal<void(std::string, bool, shared_ptr<const Reflection::ValueTable>)> boundActionAddedSignal;
        rbx::signal<void(std::string, shared_ptr<const Reflection::ValueTable>)> boundActionRemovedSignal;
        
        rbx::signal<void(std::string)> getActionButtonSignal;
        rbx::signal<void(std::string, shared_ptr<Instance>)> actionButtonFoundSignal;
        
        std::string getCurrentLocalToolIcon();
        
		void bindCoreActionForInputTypes(const std::string actionName, Lua::WeakFunctionRef functionToBind, bool createTouchButton, shared_ptr<const Reflection::Tuple> hotkeys);
		void unbindCoreAction(const std::string actionName);

        void bindActionForInputTypes(const std::string actionName, Lua::WeakFunctionRef functionToBind, bool createTouchButton, shared_ptr<const Reflection::Tuple> hotkeys);
        void unbindAction(const std::string actionName);
        void unbindAll();

		void bindActivate(InputObject::UserInputType inputType, KeyCode keyCode);
		void unbindActivate(InputObject::UserInputType inputType, KeyCode keyCode);
        
        void setTitleForAction(const std::string actionName, std::string title);
        void setDescForAction(const std::string actionName, std::string description);
        void setImageForAction(const std::string actionName, std::string image);
        void setPositionForAction(const std::string actionName, UDim2 position);
        
        void getButton(const std::string actionName, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction);
        
        shared_ptr<const Reflection::ValueTable> getBoundCoreActionData(const std::string actionName);
        shared_ptr<const Reflection::ValueTable> getBoundActionData(const std::string actionName);
        shared_ptr<const Reflection::ValueTable> getAllBoundActionData();
        
        void fireActionButtonFoundSignal(const std::string actionName, shared_ptr<Instance> actionButton);
        
        //////////////////////////////////////////////////////////////
        //
        // Instance
        /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
        
        //////////////////////////////////////////////////////////////
        //
        // Proccessing of input
        //
		GuiResponse processCoreBindings(const shared_ptr<InputObject>& inputObject);
		GuiResponse processDevBindings(const shared_ptr<InputObject>& inputObject, bool menuIsOpen);
        
		void callFunction(boost::function<void(shared_ptr<Reflection::Tuple>)> luaFunction, const std::string actionName, const InputObject::UserInputState state, const shared_ptr<Instance> inputObject);
        void callFunction(const std::string actionName, const InputObject::UserInputState state, const shared_ptr<Instance> inputObject);
        
    protected:
        rbx::signals::scoped_connection characterChildAddConnection;
        rbx::signals::scoped_connection characterChildRemoveConnection;
        rbx::signals::scoped_connection localPlayerAddConnection;
        
	private:
        typedef DescribedNonCreatable<ContextActionService, Instance, sContextActionService> Super;
        
		// these are used for user context binds
        FunctionMap functionMap;
		FunctionVector functionVector;

		// these are used for ROBLOX context binds (For ROBLOX menu, etc.)
		FunctionMap coreFunctionMap;
		FunctionVector coreFunctionVector;
        
        FunctionPairMap yieldFunctionMap;

		// used for binding activate (used to simulate mouse clicks on different inputs
		std::string activateGuid;
        boost::unordered_map<InputObject*, float> lastZPositionsForActivate;

        Tool* getCurrentLocalTool();
        Tool* isTool(shared_ptr<Instance> instance);
        
        void checkForToolRemoval(shared_ptr<Instance> removedChildOfCharacter);
        void checkForNewTool(shared_ptr<Instance> newChildOfCharacter);

        void disconnectAllCharacterConnections();
        void localCharacterAdded(shared_ptr<Instance> character);
        void setupLocalCharacterConnections(ModelInstance* character);
        
        void checkForLocalPlayer(shared_ptr<Instance> newPlayer);
        void setupLocalPlayerConnections(RBX::Network::Player* localPlayer);

		void bindActionInternal(const std::string actionName, Lua::WeakFunctionRef functionToBind, bool createTouchButton, shared_ptr<const Reflection::Tuple> hotkeys, FunctionMap& funcMap, FunctionVector& funcVector);
        
        FunctionMap::iterator findAction(const std::string actionName);

		GuiResponse tryProcess(shared_ptr<InputObject> inputObject, FunctionVector& funcVector, bool menuIsOpen);

		void processActivateAction(const shared_ptr<InputObject>& inputObject);
        
        void fireBoundActionChangedSignal(FunctionMap::iterator iter, const std::string& changeName);
		void checkForInputOverride(const Reflection::Variant& newInputType, const FunctionVector& funcVector);
	};
    
}