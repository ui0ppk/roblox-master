--[[
			// GameData.lua

			// Fetches data for a game to be used to fill out
			// the details of that game
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Http = require(Modules:FindFirstChild('Http'))
local Utility = require(Modules:FindFirstChild('Utility'))

local UserData = require(Modules:FindFirstChild('UserData'))
local ConvertMyPlaceNameInXboxAppFlag = Utility.IsFastFlagEnabled("ConvertMyPlaceNameInXboxApp")

local GameData = {}

local gameCreatorCache = {}
function GameData:GetGameCreatorAsync(placeId)
	if placeId then
		if not gameCreatorCache[placeId] then
			local gameDataByPlaceId = self:GetGameDataAsync(placeId)
			gameCreatorCache[placeId] = gameDataByPlaceId:GetCreatorName()
		end
		return gameCreatorCache[placeId]
	end
end

function GameData:ExtractGeneratedUsername(gameName)
	local tempUsername = string.match(gameName, "^([0-9a-fA-F]+)'s Place$")
	if tempUsername and #tempUsername == 32 then
		return tempUsername
	end
end

-- Fix places that have been made with incorrect temporary usernames
-- creatorName is optional and must be used when querying a game that is
-- not the current user's creation
function GameData:GetFilteredGameName(gameName, creatorName)
	if ConvertMyPlaceNameInXboxAppFlag and gameName and type(gameName) == 'string' then
		local tempUsername = self:ExtractGeneratedUsername(gameName)
		if tempUsername then
			local realUsername = creatorName or UserData:GetRobloxName()
			if realUsername then
				local newGameName = string.gsub(gameName, tempUsername, realUsername, 1)
				if newGameName then
					return newGameName
				end
			end
		end
	end
	return gameName
end

function GameData:GetGameDataAsync(placeId)
	local this = {}

	local result = Http.GetGameDetailsAsync(placeId)
	if not result then
		print("GameData:GetGameDataAsync() failed to get web response for placeId "..tostring(placeId))
		result = {}
	end

	this.Data = result

	--[[ Public API ]]--
	function this:GetCreatorName()
		return self.Data["Builder"] or ""
	end
	function this:GetDescription()
		return self.Data["Description"] or ""
	end
	function this:GetIsFavoritedByUser()
		return self.Data["IsFavoritedByUser"] or false
	end
	function this:GetLastUpdated()
		return self.Data["Updated"] or ""
	end
	function this:GetCreationDate()
		return self.Data["Created"] or ""
	end
	function this:GetMaxPlayers()
		return self.Data["MaxPlayers"] or 0
	end
	function this:GetOverridesDefaultAvatar()
		return self.Data["OverridesDefaultAvatar"] or false
	end
	function this:GetCreatorUserId()
		return self.Data["BuilderId"]
	end

	--[[ Async Public API ]]--
	function this:GetVoteDataAsync()
		local result = Http.GetGameVotesAsync(placeId)
		if not result then
			print("GameData:GetVoteDataAsync() failed to get web response for placeId "..tostring(placeId))
		end

		local voteData = {}
		local voteTable = result and result["VotingModel"] or nil

		if voteTable then
			voteData.UpVotes = voteTable["UpVotes"] or 0
			voteData.DownVotes = voteTable["DownVotes"] or 0
			voteData.UserVote = voteTable["UserVote"] or nil
			voteData.CanVote = voteTable["CanVote"] or false
			voteData.CantVoteReason = voteTable["ReasonForNotVoteable"] or "PlayGame"
		end

		return voteData
	end

	function this:GetGameIconIdAsync()
		local iconId = nil
		local result = Http.GetGameIconIdAsync(placeId)
		if result then
			iconId = result["ImageId"]
			-- use placeId as backup
			if not iconId then
				iconId = placeId
			end
		end

		return iconId
	end

	function this:GetRecommendedGamesAsync()
		local result = Http.GetRecommendedGamesAsync(placeId)
		if not result then
			print("GameData:GetRecommendedGamesAsync() failed to get web response for placeId "..tostring(placeId))
			return {}
		end

		local recommendedGames = {}
		for i = 1, #result do
			local data = result[i]
			if data then
				local game = {}
				-- Temp fix for fixing game names
				game.Name = GameData:GetFilteredGameName(data["GameName"], data["Creator"] and data["Creator"]["CreatorName"])
				game.PlaceId = data["PlaceId"]
				game.IconId = data["ImageId"]
				table.insert(recommendedGames, game)
			end
		end

		return recommendedGames
	end

	function this:GetThumbnailIdsAsync()
		local result = Http.GetGameThumbnailsAsync(placeId)
		if not result then
			print("GameData:GetThumbnailIdsAsync() failed to get web response for placeId "..tostring(placeId))
			return {}
		end

		local thumbIds = {}
		local thumbIdTable = result["thumbnails"]
		if thumbIdTable then
			for i = 1, #thumbIdTable do
				local data = thumbIdTable[i]
				-- AssetTypeId of 1 is a Image (33 is a video if can ever play videos)
				if data and data["AssetTypeId"] == 1 then
					local assetId = data["AssetId"]
					if assetId then
						table.insert(thumbIds, assetId)
					end
				end
			end
		end

		return thumbIds
	end

	function this:GetBadgeDataAsync()
		local result = Http.GetGameBadgeDataAsync(placeId)
		if not result then
			print("GameData:GetBadgeDataAsync() failed to get web response for placeId "..tostring(placeId))
			return {}
		end

		local badgeData = {}
		local badgeTable = result["GameBadges"]
		if badgeTable then
			for i = 1, #badgeTable do
				local data = badgeTable[i]
				if data then
					local badge = {}
					badge.Name = data["Name"]
					badge.Description = data["Description"]
					badge.AssetId = data["BadgeAssetId"]
					badge.IsOwned = data["IsOwned"]
					badge.Order = i
					table.insert(badgeData, badge)
				end
			end
		end

		table.sort(badgeData, function(a, b)
			if a["IsOwned"] == true and b["IsOwned"] == true then
				return a.Order < b.Order
			elseif a["IsOwned"] then
				return true
			elseif b["IsOwned"] then
				return false
			end
			return a.Order < b.Order
		end)

		return badgeData
	end

	--[[ Post Public API ]]--
	function this:PostFavoriteAsync()
		local result = Http.PostFavoriteToggleAsync(placeId)
		local success = result and result["success"] == true
		if not success then
			local reason = "Failed"
			-- the floodcheck message is "Whoa. Slow Down.". So if there is a message,
			-- let's just say flood check?
			if result and result["message"] then
				reason = "FloodCheck"
			end
			return success, reason
		else
			self.Data["IsFavoritedByUser"] = not self:GetIsFavoritedByUser()
		end

		return success
	end

	function this:PostVoteAsync(status)
		local result = Http.PostGameVoteAsync(placeId, status)
		if not result then
			return nil
		end

		local success = result["Success"] == true
		if not success then
			return success, result["ModalType"]
		end

		return success
	end

	-- Temp fix for fixing game names
	if this.Data then
		this.Data.Name = GameData:GetFilteredGameName(this.Data.Name, this:GetCreatorName())
	end

	return this
end

return GameData
