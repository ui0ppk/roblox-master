--[[
			// SortData.lua
			// API for Game Sorts
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local GameData = require(Modules:FindFirstChild('GameData'))

local Http = require(Modules:FindFirstChild('Http'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local SortData = {}

-- this is a temp cache for play my place. We need to be able to check if a user is joining
-- a featured game or not in order to set their session privacy.
local featuredSortCache = nil
local FEATURED_SORT_ID = 3
local REFRESH_TIME = 300
local lastTimeUpdated = 0
local isFetching = false
local function getFeaturedCacheAsync()
	while isFetching do
		wait()
	end

	if featuredSortCache and (tick() - lastTimeUpdated < REFRESH_TIME) then
		return featuredSortCache
	end

	isFetching = true
	local newFeaturedSortCache = {}
	local lastIndexLoaded = 0
	local PAGE_SIZE = 100
	local sort = SortData.GetSort(FEATURED_SORT_ID)
	local getFeaturedSortSuccess = true

	local isLastPage = false
	repeat
		local currentPage = nil
		local function tryGetSortAsync()
			currentPage = sort:GetPageAsync(lastIndexLoaded, PAGE_SIZE)
			if currentPage then
				return true
			end
		end
		Utility.ExponentialRepeat(
			function() return currentPage == nil end, tryGetSortAsync, 3)

		if currentPage then
			if currentPage.Count > 0 then
				local placeIds = currentPage:GetPagePlaceIds()
				for i = 1, #placeIds do
					newFeaturedSortCache[placeIds[i]] = true
				end
				lastIndexLoaded = lastIndexLoaded + currentPage.Count
			end
			isLastPage = currentPage.Count < PAGE_SIZE
		else
			getFeaturedSortSuccess = false
		end
	until not getFeaturedSortSuccess == false or isLastPage == true

	if getFeaturedSortSuccess == true then
		featuredSortCache = newFeaturedSortCache
	end

	lastTimeUpdated = tick()
	isFetching = false

	return featuredSortCache
end

function SortData:IsFeaturedGameAsync(placeId)
	local featuredSort = getFeaturedCacheAsync()
	if featuredSort then
		return featuredSort[placeId]
	end

	return false
end

local function initPage(data)
	local page = {}

	page.Data = data
	page.Count = #data

	function page:GetPagePlaceIds()
		local ids = {}

		for i = 1, #data do
			ids[i] = data[i]["PlaceID"]
		end

		return ids
	end

	function page:GetPagePlaceNames()
		local names = {}

		for i = 1, #data do
			names[i] = GameData:GetFilteredGameName(data[i]["Name"], data[i]["CreatorName"])
		end

		return names
	end

	function page:GetCreatorNames()
		local names = {}

		for i = 1, #data do
			names[i] = data[i]["CreatorName"]
		end

		return names
	end

	function page:GetPageIconIds()
		local iconIds = {}

		for i = 1, #data do
			local id = data[i]["ImageId"]
			iconIds[i] = id
		end

		return iconIds
	end

	function page:GetPageVoteData()
		local voteData = {}

		for i = 1, #data do
			local vote = {
				UpVotes = data[i]["TotalUpVotes"];
				DownVotes = data[i]["TotalDownVotes"];
			}
			voteData[i] = vote
		end

		return voteData
	end

	function page:GetPageCreatorUserIds()
		local creatorIds = {}

		for i = 1, #data do
			local id = data[i]["CreatorID"]
			table.insert(creatorIds, id)
		end

		return creatorIds
	end

	return page
end

-- returns class style table for sorts
function SortData.GetSortCategoriesAsync()
	local result = Http.GetGameSortsAsync()
	if not result then
		-- TODO: Error codes
		return
	end
	--
	local sorts = {}

	for i = 1, #result do
		local sort = {
			Id = result[i]["Id"];
			Name = result[i]["Name"];
		}
		table.insert(sorts, sort)
	end

	return sorts
end

local function createSort(sort, sortId, httpFunc)
	function sort:GetPageAsync(startIndex, pageSize, timeFilter)
		local result = httpFunc(startIndex, pageSize, sortId, timeFilter)
		if not result then
			return nil
		end

		return initPage(result)
	end
end

function SortData.GetSort(sortId)
	local sort = {}

	local httpFunc = Http.GetSortAsync
	createSort(sort, sortId, httpFunc)

	return sort
end

function SortData.GetUserFavorites()
	local sort = {}
	local sortId = "MyFavorite"

	local httpFunc = Http.GetUserFavoritesAsync
	createSort(sort, sortId, httpFunc)

	return sort
end

function SortData.GetUserRecent()
	local sort = {}
	local sortId = "MyRecent"

	local httpFunc = Http.GetUserRecentAsync
	createSort(sort, sortId, httpFunc)

	return sort
end

function SortData.GetUserPlaces(userId)
	local sort = {}

	local httpFunc = Http.GetUserPlacesAsync
	createSort(sort, userId, httpFunc)

	return sort
end

return SortData
