--[[
			// AccountPage.lua
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local ContextActionService = game:GetService('ContextActionService')

local GuiService = game:GetService('GuiService')
local AccountManager = require(Modules:FindFirstChild('AccountManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BaseScreen = require(Modules:FindFirstChild('BaseScreen'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Utility = require(Modules:FindFirstChild('Utility'))
local UserData = require(Modules:FindFirstChild('UserData'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))

local SetAccountCredentialsScreen = require(Modules:FindFirstChild('SetAccountCredentialsScreen'))
local UnlinkAccountScreen = require(Modules:FindFirstChild('UnlinkAccountScreen'))
local LinkAccountScreen = require(Modules:FindFirstChild('LinkAccountScreen'))

-- This is an empty page that is a place holder. Account page changes depending on cases
local function createAccountScreen()
	local hasLinkedAccount = UserData:HasLinkedAccount()
	local hasRobloxCredentials = UserData:HasRobloxCredentials()

	-- Cases
	-- 1. Has roblox credentials, which implies they have a linked account
	-- 2. No credentials but a linked account
	-- 3. No Credentials/No Linked account - this should never happen, but cover it
	-- 4. One of these calls has a web error, result will be nil in that case

	local this = nil


	if hasRobloxCredentials ~= nil and hasLinkedAccount ~= nil then
		if hasRobloxCredentials == true then
			this = UnlinkAccountScreen()
		elseif hasLinkedAccount == true and hasRobloxCredentials == false then
			this = SetAccountCredentialsScreen(Strings:LocalizedString("SignUpTitle"),
				Strings:LocalizedString("SignUpPhrase"), Strings:LocalizedString("SignUpWord"))
		end
	end

	return this
end

return createAccountScreen
