--[[
			// SortCarousel.lua
			// Creates a sort carousel with data for the sort
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local EventHub = require(Modules:FindFirstChild('EventHub'))
local GameDataModule = require(Modules:FindFirstChild('GameData'))
local ImageSlider = require(Modules:FindFirstChild('ImageSlider'))
local Utility = require(Modules:FindFirstChild('Utility'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))

local createSortCarousel = function(size, position, gameCollection, parent)
	local this = {}

	--[[ Constants ]]--
	local LOAD_COUNT = 20
	local IMAGE_COUNT = 9

	local carousel = ImageSlider(size, position)
	carousel:SetPadding(18)
	carousel:SetParent(parent)

	--[[ Variables ]]--
	local images = {}
	local gameData = {}
	local lastSelectedIndex = nil
	local lastLoadedCount = 0

	--[[ Event ]]--
	this.OnNewGameSelected = Utility.Signal()
	-- we need to fetch for description/isFavorited, so we send a late signal to update those
	this.OnNewGameSelectedLate = Utility.Signal()

	--[[ Private Functions ]]--
	local function insertSortData(page)
		if not page then
			print("Error: SortCarousel failed to get a valid page for insertSortData.")
			return
		end
		-- TODO: Get new square icons
		local ids = page:GetPagePlaceIds()
		local names = page:GetPagePlaceNames()
		local voteData = page:GetPageVoteData()
		local iconIds = page:GetPageIconIds()
		local creatorNames = page:GetCreatorNames()
		--
		for i = 1, #page.Data do
			local entry = {
				Name = names[i];
				PlaceId = ids[i];
				IconId = iconIds[i];
				VoteData = voteData[i];
				CreatorName = creatorNames[i];
				-- Description/IsFavorited needs to be queried for each game, do it only when needed
				-- we'll also cache gameData
				Description = nil;
				IsFavorited = nil;
				GameData = nil;
			}
			gameData[#gameData + 1] = entry
		end
		lastLoadedCount = #gameData
	end

	local function createCarouselImages()
		local thumbSize = ThumbnailLoader.Sizes.Medium
		local assetType = ThumbnailLoader.AssetType.Icon
		for i = 1, IMAGE_COUNT do
			local data = gameData[i]
			if data then
				local image = Utility.Create'ImageButton'
				{
					Name = "GameImage";
					BorderSizePixel = 0;
					BackgroundColor3 = GlobalSettings.GreyButtonColor;
					ZIndex = 2;

					SoundManager:CreateSound('MoveSelection');
					AssetManager.CreateShadow(1);
				}
				local thumbLoader = ThumbnailLoader:Create(image, data.IconId, thumbSize, assetType)
				spawn(function()
					if not thumbLoader:LoadAsync() then
						-- TODO
					end
				end)
				table.insert(images, image)
				carousel:AddItem(image)

				-- connect button press based on current selected index mapped to gameData
				image.MouseButton1Click:connect(function()
					if lastSelectedIndex and gameData[lastSelectedIndex] then
						local currentData = gameData[lastSelectedIndex]
						EventHub:dispatchEvent(EventHub.Notifications["OpenGameDetail"], currentData.PlaceId,
							currentData.Name, currentData.IconId, currentData.GameData)
					end
				end)
			end
		end
	end

	--[[ Event Connections ]]--
	local nextLoadIndex = 0
	local lastSelectedObject = nil
	local onNewFocusCn = nil

	local function onNewFocusItem(newIndex, isRight, selectedObject)
		lastSelectedObject = selectedObject
		lastSelectedIndex = newIndex
		--
		if gameData[newIndex] then
			local currentData = gameData[newIndex]
			if this then
				this.OnNewGameSelected:fire(currentData)
			end

			-- description/isFavorited needs to fetch, so send a late update
			spawn(function()
				if not currentData.GameData then
					local gameData = GameDataModule:GetGameDataAsync(currentData.PlaceId)
					if gameData then
						currentData.GameData = gameData
						currentData.Description = gameData:GetDescription()
						currentData.IsFavorited = gameData:GetIsFavoritedByUser()
					end
				end
				if this and selectedObject == lastSelectedObject then
					this.OnNewGameSelectedLate:fire(currentData.Description, currentData.IsFavorited)
				end
			end)
		end

		-- load more data
		if isRight and #gameData > IMAGE_COUNT and newIndex == nextLoadIndex then
			nextLoadIndex = nextLoadIndex + LOAD_COUNT
			local page = gameCollection:GetSortAsync(lastLoadedCount, LOAD_COUNT)
			insertSortData(page)
			carousel:SetMaxItems(#gameData)
		end
	end

	onNewFocusCn = carousel.OnNewFocusItem:connect(onNewFocusItem)
	--[[ Public API ]]--
	function this:SetParent(newParent)
		carousel:SetParent(newParent)
	end

	function this:GetCurrentSelectedGameData()
		if gameData[lastSelectedIndex] then
			return gameData[lastSelectedIndex]
		end
	end

	function this:GetItemAt(index)
		return images[index]
	end

	function this:Destroy()
		carousel:Destroy()
		if onNewFocusCn then
			onNewFocusCn:disconnect()
			onNewFocusCn = nil
		end
		spawn(function()
			gameData = nil
			this = nil
		end)
	end

	function this:LoadSortAsync()
		local loader = LoadingWidget(
			{ Parent = parent },{
			function()
				local page = gameCollection:GetSortAsync(0, LOAD_COUNT)
				insertSortData(page)
				createCarouselImages()
				if carousel then
					carousel:SetDataTable(gameData)
					carousel:SetMaxItems(#gameData)
					carousel:SetFocusPosition(1)
					onNewFocusItem(1, nil, images[1])
					nextLoadIndex = LOAD_COUNT / 2
				end
			end
			})

		loader:AwaitFinished()
		loader:Cleanup()
		loader = nil
	end

	return this
end

return createSortCarousel
