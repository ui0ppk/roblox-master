--[[
			// GameJoin.lua

			// Handles game join logic
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local UserData = require(Modules:FindFirstChild('UserData'))

local GameJoin = {}

GameJoin.JoinType = {
	Normal = 0;			-- use placeId
	GameInstance = 1;	-- use game instance id
	Follow = 2;			-- use userId or user you are following
	PMPCreator = 3;		-- use placeId, used when a player joins their own place
}

-- joinType - GameJoin.JoinType
-- joinId - can be a userId or placeId, see JoinType for which one to use
function GameJoin:StartGame(joinType, joinId, creatorUserId)
	if UserSettings().GameSettings:InStudioMode() then
		ScreenManager:OpenScreen(ErrorOverlayModule(Errors.Test.CannotJoinGame), false)
	else
		local success, result = pcall(function()
			-- check if we are the creator for normal joins
			if joinType == self.JoinType.Normal and creatorUserId == UserData:GetRbxUserId() then
				joinType = self.JoinType.PMPCreator
			end

			return PlatformService:BeginStartGame3(joinType, joinId)
		end)
		-- catch pcall error, something went wrong with call into API
		-- all other game join errors are caught in AppHome.lua
		if not success then
			ScreenManager:OpenScreen(ErrorOverlayModule(Errors.GameJoin[#Errors.GameJoin]), false)
		end
	end
end

return GameJoin
