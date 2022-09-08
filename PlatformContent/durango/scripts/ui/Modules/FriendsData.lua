--[[
			// FriendsData.lua

			// Caches the current friends pagination to used by anyone in the app
			// polls every POLL_DELAY and gets the latest pagination

			// TODO:
				Need polling to update friends. How are we going to handle all the cases
					like the person you're selecting going offline, etc..
]]
local CoreGui = game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local Players = game:GetService('Players')
local HttpService = game:GetService('HttpService')
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local Http = require(Modules:FindFirstChild('Http'))
local Utility = require(Modules:FindFirstChild('Utility'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local UserData = require(Modules:FindFirstChild('UserData'))
local SortData = require(Modules:FindFirstChild('SortData'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))

-- NOTE: This is just required for fixing Usernames in auto-generatd games
local GameData = require(Modules:FindFirstChild('GameData'))
local ConvertMyPlaceNameInXboxAppFlag = Utility.IsFastFlagEnabled("ConvertMyPlaceNameInXboxApp")

local FriendsData = {}


local pollDelay = Utility.GetFastVariable("XboxFriendsPolling")
if pollDelay then
	pollDelay = tonumber(pollDelay)
end

local POLL_DELAY = pollDelay or 30
local STATUS = {
	UNKNOWN = "Unknown";
	ONLINE = "Online";
	OFFLINE = "Offline";
	AWAY = "Away";
}

local isOnlineFriendsPolling = false
local myCurrentFriendsData = nil
local updateEventCns = {}

local function filterXboxFriends(friendsData)
	local titleId = PlatformService and tostring(PlatformService:GetTitleId()) or ''
	local onlineRobloxFriendsData = {}
	local onlineRobloxFriendsUserIds = {}
	local onlineFriendsData = {}
	-- filter into two list, those online in roblox and those online
	-- for those online, also get their roblox userId
	for i = 1, #friendsData do
		local data = friendsData[i]
		if data["status"] and data["status"] == STATUS.ONLINE then
			if data["rich"] then
				-- get rich presence table, last entry is most recent activity from user
				local richTbl = data["rich"]
				richTbl = richTbl[#richTbl]
				if richTbl["titleId"] == titleId then
					table.insert(onlineRobloxFriendsData, data)
					table.insert(onlineRobloxFriendsUserIds, data["robloxuid"])
				else
					table.insert(onlineFriendsData, data)
				end
			end
		end
	end

	-- now get roblox friends presence in roblox
	local robloxPresence = {}
	if #onlineRobloxFriendsUserIds > 0 then
		local jsonTable = {}
		jsonTable["userIds"] = onlineRobloxFriendsUserIds
		local jsonPostBody = HttpService:JSONEncode(jsonTable)
		robloxPresence = Http.GetUsersOnlinePresenceAsync(jsonPostBody)
		if robloxPresence and robloxPresence["UserPresences"] then
			robloxPresence = robloxPresence["UserPresences"]
		end
	end

	-- now append roblox presence data to each users data
	for i = 1, #onlineRobloxFriendsData do
		if robloxPresence[i] then
			local data = onlineRobloxFriendsData[i]
			-- make sure we have the right person
			for j = 1, #robloxPresence do
				local rbxPresenceData = robloxPresence[j]
				local rbxUserId = rbxPresenceData["VisitorId"]
				if rbxUserId == data["robloxuid"] then
					if rbxPresenceData["IsOnline"] == true then
						local placeId = rbxPresenceData["PlaceId"]
						local lastLocation = rbxPresenceData["LastLocation"]

						-- If the lastLocation for a user is some user place with a GeneratedUsername in it
						-- then replace it with the actual creator name!
						if ConvertMyPlaceNameInXboxAppFlag and placeId and lastLocation and GameData:ExtractGeneratedUsername(lastLocation) then
							local gameCreator = GameData:GetGameCreatorAsync(placeId)
							if gameCreator then
								lastLocation = GameData:GetFilteredGameName(lastLocation, gameCreator)
							end
						end

						-- If the user is not in a featured game, then hide their presence
						if placeId and not SortData:IsFeaturedGameAsync(placeId) then
							data["IsPrivateSession"] = true
							lastLocation = Strings:LocalizedString("PrivateSessionPhrase")
						end

						data["PlaceId"] = placeId
						data["LastLocation"] = lastLocation
					end
					-- remove from list and gtfo
					table.remove(robloxPresence, j)
					break
				end
			end
		end
	end

	-- now sort those in roblox
	table.sort(onlineRobloxFriendsData, function(a, b)
		if a["PlaceId"] and b["PlaceId"] then
			return a["display"] < b["display"]
		end
		if a["PlaceId"] then
			return true
		end
		if b["PlaceId"] then
			return false
		end

		return a["display"] < b["display"]
	end)

	-- now sort all other friends
	table.sort(onlineFriendsData, function(a, b)
		return a["display"] < b["display"]
	end)

	-- now concat tables
	for i = 1, #onlineFriendsData do
		onlineRobloxFriendsData[#onlineRobloxFriendsData + 1] = onlineFriendsData[i]
	end

	return onlineRobloxFriendsData
end

--[[
		// Returns table with an array with the key friends
		// Keys
			// xuid - number, xbox user id
			// gamertage - string, users gamertag
			// display - string, users display name (depends on user settings, could be same as gamertag)
			// robloxuid - number, users roblox userId
			// status - string (Online, Away, Offline, Unknown)
			// rich - array of rich presence, might currently be empty?
				// timestamp - number, UTC timestamp of record
				// device - string, XboxOne, Windows8, etc.
				// title - string, of game
				// playing - boolean, are they playing the game
				// presence - string, rich presence string from that title
]]
local function fetchXboxFriendsAsync()
	local success, result = pcall(function()
		if PlatformService then
			return PlatformService:BeginFetchFriends(Enum.UserInputType.Gamepad1)
		end
	end)
	if success then
		return HttpService:JSONDecode(result)
	else
		print("fetchXboxFriends failed because", result)
	end
end

local function getOnlineFriends()
	if UserSettings().GameSettings:InStudioMode() then
-- Roblox Friends - leaving this in for testing purposes in studio
		local result = Http.GetOnlineFriendsAsync()
		if not result then
			-- TODO: Error code
			return nil
		end
		--
		local myOnlineFriends = {}

		for i = 1, #result do
			local data = result[i]
			local friend = {
				Name = Players:GetNameFromUserIdAsync(data["VisitorId"]);
				UserId = data["VisitorId"];
				LastLocation = data["LastLocation"];
				PlaceId = data["PlaceId"];
				LocationType = data["LocationType"];
				GameId = data["GameId"];
			}
			table.insert(myOnlineFriends, friend)
		end

		local function sortFunc(a, b)
			if a.LocationType == b.LocationType then
				return a.Name:lower() < b.Name:lower()
			end
			return a.LocationType > b.LocationType
		end

		table.sort(myOnlineFriends, sortFunc)

		return myOnlineFriends
	elseif game:GetService('UserInputService'):GetPlatform() == Enum.Platform.XBoxOne then
-- Xbox Friends
		local myXboxFriends = fetchXboxFriendsAsync()
		local myOnlineFriends = {}
		if myXboxFriends then
			myXboxFriends = myXboxFriends["friends"]
			myOnlineFriends = filterXboxFriends(myXboxFriends)
		end

		return myOnlineFriends
	end
end

--[[ Public API ]]--
FriendsData.OnFriendsDataUpdated = Utility.Signal()

local isFetchingFriends = false
function FriendsData.GetOnlineFriendsAsync()
	-- can only make one call into PlatformService:BeginFetchFriends() at a time
	while isFetchingFriends do
		wait()
	end
	-- we have current data, this will be updated when polling
	if myCurrentFriendsData then
		return myCurrentFriendsData
	end
	isFetchingFriends = true

	myCurrentFriendsData = getOnlineFriends()
	-- spawn polling on first request
	if not isOnlineFriendsPolling then
		FriendsData.BeginPolling()
	end

	isFetchingFriends = false

	return myCurrentFriendsData
end

function FriendsData.BeginPolling()
	if not isOnlineFriendsPolling then
		isOnlineFriendsPolling = true
		local requesterId = UserData:GetRbxUserId()
		spawn(function()
			wait(POLL_DELAY)
			while requesterId == UserData:GetRbxUserId() do
				myCurrentFriendsData = getOnlineFriends()
				FriendsData.OnFriendsDataUpdated:fire(myCurrentFriendsData)
				wait(POLL_DELAY)
			end
		end)
	end
end

-- we make connections through this function so we can clean them all up upon
-- clearing the friends data
function FriendsData.ConnectUpdateEvent(cbFunc)
	local cn = FriendsData.OnFriendsDataUpdated:connect(cbFunc)
	table.insert(updateEventCns, cn)
end

function FriendsData.Reset()
	isOnlineFriendsPolling = false
	myCurrentFriendsData = nil
	for index,cn in pairs(updateEventCns) do
		cn = Utility.DisconnectEvent(cn)
		updateEventCns[index] = nil
	end
	print('FriendsData: Cleared last users FriendsData')
end

return FriendsData
