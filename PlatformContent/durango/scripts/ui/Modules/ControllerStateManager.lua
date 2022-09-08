--[[
				// ControllerStateManager.lua

				// Handles controller state changes
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)
local UserInputService = game:GetService('UserInputService')

local EventHub = require(Modules:FindFirstChild('EventHub'))
local NoActionOverlay = require(Modules:FindFirstChild('NoActionOverlay'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local UserData = require(Modules:FindFirstChild('UserData'))
local Utility = require(Modules:FindFirstChild('Utility'))

local ControllerStateManager = {}

local LostUserGamepadCn = nil
local LostActiveUserCn = nil
local GainedUserGamepadCn = nil
local GainedActiveUserCn = nil
local DisconnectCn = nil

local currentOverlay = nil

local DATAMODEL_TYPE = {
	APP_SHELL = 0;
	GAME = 1;
}

local function closeOverlay(dataModelType)
	if dataModelType == DATAMODEL_TYPE.GAME then
		UserInputService.OverrideMouseIconBehavior = Enum.OverrideMouseIconBehavior.None

		currentOverlay:Hide()
		currentOverlay = nil
	else
		ScreenManager:CloseCurrent()
	end
end

local function showErrorOverlay(titleName, bodyName, userDisplayName, dataModelType)
	-- create error
	local err = { Title = Strings:LocalizedString(titleName),
		Msg = string.format(Strings:LocalizedString(bodyName), userDisplayName) }
	local noActionOverlay = NoActionOverlay(err)
	if dataModelType == DATAMODEL_TYPE.GAME then
		UserInputService.OverrideMouseIconBehavior = Enum.OverrideMouseIconBehavior.ForceHide

		currentOverlay = noActionOverlay
		noActionOverlay:Show()
	else
		ScreenManager:OpenScreen(noActionOverlay, false)
	end
end

local function onLostActiveUser(userDisplayName, dataModelType)
	showErrorOverlay("ActiveUserLostConnectionTitle", "ActiveUserLostConnectionPhrase", userDisplayName, dataModelType)
end

local function onGainedActiveUser(dataModelType)
	closeOverlay(dataModelType)
end

local function onLostUserGamepad(userDisplayName, dataModelType)
	showErrorOverlay("ControllerLostConnectionTitle", "ControllerLostConnectionPhrase", userDisplayName, dataModelType)
end

local function onGainedUserGamepad(dataModelType)
	closeOverlay(dataModelType)
end

local function disconnectEvents()
	LostActiveUserCn = Utility.DisconnectEvent(LostActiveUserCn)
	GainedActiveUserCn = Utility.DisconnectEvent(GainedActiveUserCn)

	LostUserGamepadCn = Utility.DisconnectEvent(LostUserGamepadCn)
	GainedUserGamepadCn = Utility.DisconnectEvent(GainedUserGamepadCn)
end

function ControllerStateManager:Initialize()
	if not PlatformService then return end

	local dataModelType = PlatformService.DatamodelType

	disconnectEvents()
	LostUserGamepadCn = PlatformService.LostUserGamepad:connect(function(userDisplayName)
		onLostUserGamepad(userDisplayName, dataModelType)
	end)
	GainedUserGamepadCn = PlatformService.GainedUserGamepad:connect(function(userDisplayName)
		onGainedUserGamepad(dataModelType)
	end)

	LostActiveUserCn = PlatformService.LostActiveUser:connect(function(userDisplayName)
		onLostActiveUser(userDisplayName, dataModelType)
	end)
	GainedActiveUserCn = PlatformService.GainedActiveUser:connect(function(userDisplayName)
		onGainedActiveUser(dataModelType)
	end)

	-- disconnect based on DataModel type
	if dataModelType == DATAMODEL_TYPE.GAME then
		DisconnectCn = PlatformService.ViewChanged:connect(function(viewType)
			if viewType == 0 then
				disconnectEvents()
			end
		end)
	else
		DisconnectCn = PlatformService.UserAccountChanged:connect(function(reauthenticationReason)
			disconnectEvents()
		end)
	end
end

function ControllerStateManager:CheckUserConnected()
	if not PlatformService then return end

	local isGamepadConnected = UserInputService:GetGamepadConnected(Enum.UserInputType.Gamepad1)
	local dataModelType = PlatformService.DatamodelType
	if not isGamepadConnected then
		local userInfo = PlatformService:GetPlatformUserInfo()
		local userDisplayName = userInfo["Gamertag"] or ""
		onLostUserGamepad(userDisplayName, dataModelType)
	end
end

return ControllerStateManager
