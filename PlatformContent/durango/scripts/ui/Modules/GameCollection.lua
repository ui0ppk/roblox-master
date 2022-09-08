--[[
				// GameCollection.lua

				// Used to get a collection of games
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local SortData = require(Modules:FindFirstChild('SortData'))
local UserData = require(Modules:FindFirstChild('UserData'))
local Utility = require(Modules:FindFirstChild('Utility'))

local GameCollection = {}

GameCollection.DefaultSortId = {
	Popular = 1;
	Featured = 3;
	TopEarning = 8;
	TopRated = 11;
}

local function createBaseCollection()
	local this = {}

	function this:GetSortAsync(startIndex, pageSize)
		print("GameCollection GetSortAsync() must be implemented by sub class")
	end

	return this
end

local SortCollections = {}
function GameCollection:GetSort(sortId)
	if SortCollections[sortId] then
		return SortCollections[sortId]
	end

	local collection = createBaseCollection()

	-- Override
	function collection:GetSortAsync(startIndex, pageSize)
		local sort = SortData.GetSort(sortId)
		local timeFilter = nil
		-- top rated is time filtered to show most recent top rated
		if sortId == GameCollection.DefaultSortId.TopRated then
			timeFilter = 2
		end
		return sort:GetPageAsync(startIndex, pageSize, timeFilter)
	end

	SortCollections[sortId] = collection

	return collection
end

local UserFavoriteCollection = nil
function GameCollection:GetUserFavorites()
	if UserFavoriteCollection then
		return UserFavoriteCollection
	end

	UserFavoriteCollection = createBaseCollection()

	-- Override
	function UserFavoriteCollection:GetSortAsync(startIndex, pageSize)
		local sort = SortData.GetUserFavorites()
		return sort:GetPageAsync(startIndex, pageSize)
	end

	return UserFavoriteCollection
end

local UserRecentCollection = nil
function GameCollection:GetUserRecent()
	if UserRecentCollection then
		return UserRecentCollection
	end

	UserRecentCollection = createBaseCollection()

	-- Override
	function UserRecentCollection:GetSortAsync(startIndex, pageSize)
		local sort = SortData.GetUserRecent()
		return sort:GetPageAsync(startIndex, pageSize)
	end

	return UserRecentCollection
end

local UserPlacesCollection = nil
function GameCollection:GetUserPlaces()
	if UserPlacesCollection then
		return UserPlacesCollection
	end

	UserPlacesCollection = createBaseCollection()
	
	-- Override
	function UserPlacesCollection:GetSortAsync(startIndex, pageSize)
		if Utility.IsFastFlagEnabled("XboxPlayMyPlace") then
			local userId = UserData:GetRbxUserId()
			if userId then
				local sort = SortData.GetUserPlaces(userId)
				return sort:GetPageAsync(startIndex, pageSize)
			end
		end
	end

	return UserPlacesCollection
end

if PlatformService then
	PlatformService.UserAccountChanged:connect(function()
		SortCollections = {}
		UserFavoritCollection = nil
		UserRecentCollection = nil
		UserPlacesCollection = nil
	end)
end

return GameCollection
