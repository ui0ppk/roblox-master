--[[
			// FriendsView.lua

			// Creates a view for the users friends.
			// Handles user input, updating view

			TODO:
				Connect selected/deselected to change color
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)
local UserInputService = game:GetService('UserInputService')
local GuiService = game:GetService('GuiService')

local FriendsData = require(Modules:FindFirstChild('FriendsData'))
local FriendPresenceItem = require(Modules:FindFirstChild('FriendPresenceItem'))
local SideBarModule = require(Modules:FindFirstChild('SideBar'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local GameJoinModule = require(Modules:FindFirstChild('GameJoin'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))

-- Array of tables
	-- see FriendsData - fetchXboxFriends() for full documentation
local myFriendsData = nil

local FOLLOW_MODE = 2

local SIDE_BAR_ITEMS = {
	JoinGame =  string.upper(Strings:LocalizedString("JoinGameWord"));
	ViewDetails = string.upper(Strings:LocalizedString("ViewGameDetailsWord"));
	ViewProfile = string.upper(Strings:LocalizedString("ViewGamerCardWord"));
}

-- side bar is shared between all views
local SideBar = SideBarModule()

local function setPresenceData(item, data)
	if UserSettings().GameSettings:InStudioMode() then
		item:SetAvatarImage(data.UserId)
		item:SetNameText(data.Name)
		item:SetPresence(data.LastLocation, data.PlaceId ~= nil)
	elseif UserInputService:GetPlatform() == Enum.Platform.XBoxOne then
		local rbxuid = data["robloxuid"]
		item:SetAvatarImage(rbxuid)
		item:SetNameText(data["display"])
		if data["PlaceId"] and data["LastLocation"] then
			item:SetPresence(data["LastLocation"], true)
		elseif data["rich"] then
			local richTbl = data["rich"]
			local presence = richTbl[#richTbl]
			item:SetPresence(presence["title"], false)
		end
	end
end

-- viewGridContainer - ScrollingGrid
-- friendData - FriendsData
-- sizeConstraint - limit view to this many items
-- updateFunc - function that will be called when FriendData update
local createFriendsView = function(viewGridContainer, friendsData, sizeConstraint, updateFunc)
	local this = {}

	-- map of userId to presenceItem
	local presenceItems = {}
	local selectedItemOnFocus = nil
	local guiObjectToSortIndex = {}
	local presenceItemToData = {}

	local count = #friendsData
	if sizeConstraint then
		count = math.min(count, sizeConstraint)
	end

	local function connectSideBar(item)
		local container = item:GetContainer()
		container.MouseButton1Click:connect(function()
			local data = presenceItemToData[item]
			if data then
				-- rebuild side bar based on current data
				SideBar:RemoveAllItems()
				local inGame = data["PlaceId"] ~= nil
				if inGame and not data["IsPrivateSession"] then
					SideBar:AddItem(SIDE_BAR_ITEMS.JoinGame, function()
						GameJoinModule:StartGame(GameJoinModule.JoinType.Follow, data["robloxuid"])
					end)
					SideBar:AddItem(SIDE_BAR_ITEMS.ViewDetails, function()
						-- pass nil for iconId, gameDetail will fetch
						EventHub:dispatchEvent(EventHub.Notifications["OpenGameDetail"], data["PlaceId"], data["LastLocation"], nil)
					end)
				end
				SideBar:AddItem(SIDE_BAR_ITEMS.ViewProfile, function()
					if PlatformService and data["xuid"] then
						local success, result = pcall(function()
							PlatformService:PopupProfileUI(Enum.UserInputType.Gamepad1, data["xuid"])
						end)
						-- NOTE: This will try to pop up the xbox system gamer card, failure will be handled
						-- by the xbox.
						if not success then
							print("PlatformService:PopupProfileUI failed because,", result)
						end
					end
				end)
				ScreenManager:OpenScreen(SideBar, false)
			else
				ScreenManager:OpenScreen(ErrorOverlayModule(Errors.Default), false)
			end
		end)
	end

	-- initialize view
	for i = 1, count do
		local data = friendsData[i]
		if data then
			local idStr = tostring(data.UserId or data["robloxuid"])
			local presenceItem = FriendPresenceItem(UDim2.new(0, 446, 0, 114), idStr)
			presenceItems[idStr] = presenceItem
			presenceItemToData[presenceItem] = data
			viewGridContainer:AddItem(presenceItem:GetContainer())
			setPresenceData(presenceItem, data)
			connectSideBar(presenceItem)
			if not selectedItemOnFocus then
				selectedItemOnFocus = presenceItem:GetContainer()
			end
			guiObjectToSortIndex[presenceItem:GetContainer()] = i
		end
	end

	local function onFriendsUpdated(newFriendsData)
		local size = #newFriendsData
		if sizeConstraint then
			size = math.min(size, sizeConstraint)
		end

		-- map of valid userIds to bool
		local validEntries = {}

		-- a,b are guiObjects
		local function sortGridItems(a, b)
			if guiObjectToSortIndex[a] and guiObjectToSortIndex[b] then
				return guiObjectToSortIndex[a] < guiObjectToSortIndex[b]
			end
			if guiObjectToSortIndex[a] then return true end
			if guiObjectToSortIndex[b] then return false end
			return a.Name < b.Name
		end

		-- refresh view
		for i = 1, size do
			local data = newFriendsData[i]
			if data then
				local idStr = tostring(data.UserId or data["robloxuid"])
				local presenceItem = presenceItems[idStr]
				if not presenceItem then
					presenceItem = FriendPresenceItem(UDim2.new(0, 446, 0, 114), idStr)
					presenceItems[idStr] = presenceItem
					viewGridContainer:AddItem(presenceItem:GetContainer())
					connectSideBar(presenceItem)
				end
				presenceItemToData[presenceItem] = data
				setPresenceData(presenceItem, data)
				validEntries[idStr] = true
				guiObjectToSortIndex[presenceItem:GetContainer()] = i
				if i == 1 then
					selectedItemOnFocus = presenceItem:GetContainer()
				end
			end
		end

		-- remove items if needed
		for userId,presenceItem in pairs(presenceItems) do
			if not validEntries[userId] then
				local container = presenceItem:GetContainer()
				guiObjectToSortIndex[container] = nil
				viewGridContainer:RemoveItem(container)
				presenceItems[userId] = nil
				presenceItem:Destroy()
				presenceItem = nil
			end
		end

		viewGridContainer:SortItems(sortGridItems)

		if updateFunc then
			updateFunc(#newFriendsData)
		end
	end

	FriendsData.ConnectUpdateEvent(onFriendsUpdated)

	function this:GetDefaultFocusItem()
		return selectedItemOnFocus
	end

	return this
end

return createFriendsView
