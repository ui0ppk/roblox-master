--[[
			// UserData.lua
			// API for all user related data

			// TODO:
				Update all local create calls to use GetLocalUserData() and update
					.Create()
				Remove all friends stuff and move to new module
]]
local CoreGui = game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local Players = game:GetService('Players')
local HttpService = game:GetService('HttpService')
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local AccountManager = require(Modules:FindFirstChild('AccountManager'))
local Http = require(Modules:FindFirstChild('Http'))

local BaseUrl = Http.BaseUrl

local UserData = {}

local currentUserData = nil

local CONSTANT_RETRY_TIME = 30

local function getLocalPlayer()
	local plr = Players.LocalPlayer
	if not plr then
		while not plr do
			wait()
			plr = Players.LocalPlayer
		end
	end
	return plr
end

local function setVoteCountAsync()
	local voteResult = Http.GetVoteCountAsync()
	currentUserData["VoteCount"] = voteResult and voteResult['VoteCount'] or 0
end

local function verifyHasLinkedAccountAsync()
	local result = AccountManager:HasLinkedAccountAsync()

	while result ~= AccountManager.AuthResults.Success and result ~= AccountManager.AuthResults.AccountUnlinked do
		result = AccountManager:HasLinkedAccountAsync()
		wait(CONSTANT_RETRY_TIME)
	end

	currentUserData["LinkedAccountResult"] = result
end

local function verifyHasRobloxCredentialsAsync()
	local result = AccountManager:HasRobloxCredentialsAsync()

	while result ~= AccountManager.AuthResults.Success and result ~= AccountManager.AuthResults.UsernamePasswordNotSet do
		result = AccountManager:HasRobloxCredentialsAsync()
		wait(CONSTANT_RETRY_TIME)
	end

	currentUserData["RobloxCredentialsResult"] = result
end

function UserData:Initialize()
	if currentUserData then
		print("Trying to initialize UserData when we already have valid data.")
	end

	currentUserData = {}

	if UserSettings().GameSettings:InStudioMode() then
		local localPlayer = getLocalPlayer()
		currentUserData["Gamertag"] = "InStudioNoGamertag"
		currentUserData["RbxUid"] = localPlayer.userId
		currentUserData["RobloxName"] = localPlayer.Name
		spawn(function()
			setVoteCountAsync()
		end)
	elseif PlatformService then
		local userInfo = PlatformService:GetPlatformUserInfo()
		if userInfo then
			currentUserData["Gamertag"] = userInfo["Gamertag"]
			currentUserData["RbxUid"] = userInfo["RobloxUserId"]
		else
			currentUserData["Gamertag"] = Players.LocalPlayer.Name
		end

		spawn(setVoteCountAsync)
		spawn(verifyHasLinkedAccountAsync)
		spawn(verifyHasRobloxCredentialsAsync)
		spawn(function()
			currentUserData["RobloxName"] = Players:GetNameFromUserIdAsync(currentUserData["RbxUid"])
		end)
	end
end

function UserData:GetRbxUserId()
	if not currentUserData then
		print("Error: UserData:GetRbxUserId() - UserData has not been initialized. Don't do that!")
		return nil
	end
	return currentUserData["RbxUid"]
end

function UserData:GetDisplayName()
	if not currentUserData then
		print("Error: UserData:GetDisplayName() - UserData has not been initialized. Don't do that!")
		return nil
	end
	return currentUserData["Gamertag"]
end

function UserData:GetRobloxName()
	if not currentUserData then
		print("Error: UserData:GetRobloxName() - UserData has not been initialized. Don't do that!")
		return nil
	end
	return currentUserData["RobloxName"]
end

function UserData:SetRobloxName(name)
	if currentUserData then
		currentUserData["RobloxName"] = name
		spawn(function()
			currentUserData["RobloxName"] = Players:GetNameFromUserIdAsync(currentUserData["RbxUid"])
		end)
	end
end

function UserData:GetAvatarUrl(width, height)
	-- TODO: Update when Thumbnail manager gets fixed
	if not currentUserData then
		print("Error: UserData:GetAvatarUrl() - UserData has not been initialized. Don't do that!")
		return nil
	end
	return Http.AssetGameBaseUrl..'Thumbs/Avatar.ashx?userid='..tostring(currentUserData.RbxUid)..
			'&width='..tostring(width)..'&height='..tostring(height)
end

function UserData:GetVoteCount()
	if not currentUserData then
		print("Error: UserData:GetVoteCount() - UserData has not been initialized. Don't do that!")
		return nil
	end
	return currentUserData["VoteCount"]
end

function UserData:IncrementVote()
	currentUserData["VoteCount"] = (currentUserData["VoteCount"] or 0) + 1
end

function UserData:DecrementVote()
	currentUserData["VoteCount"] = math.max((currentUserData["VoteCount"] or 0) - 1, 0)
end

-- returns true, false or nil in the case of error
function UserData:HasLinkedAccount()
	local result = currentUserData["LinkedAccountResult"]
	if result == AccountManager.AuthResults.Success then
		return true
	elseif result == AccountManager.AuthResults.AccountUnlinked then
		return false
	else
		return nil
	end
end

-- returns true, false or nil in the case of error
function UserData:HasRobloxCredentials()
	local result = currentUserData["RobloxCredentialsResult"]
	if result == AccountManager.AuthResults.Success then
		return true
	elseif result == AccountManager.AuthResults.UsernamePasswordNotSet then
		return false
	else
		return nil
	end
end

function UserData:SetHasRobloxCredentials(value)
	currentUserData["RobloxCredentialsResult"] = value
end

function UserData:Reset()
	currentUserData = nil
end

--[[ This should no longer be used ]]--
function UserData.GetLocalUserIdAsync()
	return UserData.GetLocalPlayerAsync().userId
end

function UserData.GetLocalPlayerAsync()
	local localPlayer = Players.LocalPlayer
	while not localPlayer do
		wait()
		localPlayer = Players.LocalPlayer
	end
	return localPlayer
end

function UserData.GetPlatformUserBalanceAsync()
	local result = Http.GetPlatformUserBalanceAsync()
	if not result then
		-- TODO: Error Code
		return nil
	end
	--

	return result["Robux"]
end

function UserData.GetTotalUserBalanceAsync()
	local result = Http.GetTotalUserBalanceAsync()
	if not result then
		return nil
	end

	return result["robux"]
end

return UserData
