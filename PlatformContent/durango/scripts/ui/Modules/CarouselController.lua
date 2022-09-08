--[[
				// CarouselController.lua

				// Controls how the data is updated for a carousel view
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local ContextActionService = game:GetService('ContextActionService')
local GuiService = game:GetService('GuiService')

local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local Utility = require(Modules:FindFirstChild('Utility'))

local function createCarouselController(view)
	local this = {}

	local PAGE_SIZE = 25
	local MAX_PAGES = 2
	local LOAD_BUFFER = 10
	local LOAD_AMOUNT = 100

	local sortCollection = nil
	local sortDataObject = nil 		-- TODO: Remove when caching is finished
	local sortData = nil
	local currentFocusData = nil
	local sortDataIndex = 1
	local guiServiceChangedCn = nil

	local pages = {}
	local firstIndex = 0
	local lastIndex = 0
	local isLoading = false

	-- Events
	this.NewItemSelected = Utility.Signal()

	local function getNewItem(data)
		local item = Utility.Create'ImageButton'
		{
			Name = "CarouselViewImage";
			BackgroundColor3 = GlobalSettings.ModalBackgroundColor;
			SoundManager:CreateSound('MoveSelection');
		}
		spawn(function()
			local loader = ThumbnailLoader:Create(item, data.IconId, ThumbnailLoader.Sizes.Medium, ThumbnailLoader.AssetType.Icon)
			loader:LoadAsync(true, false, nil)
		end)

		return item
	end

	-- TODO: Remove this when caching is finished. Doing it this way is going to leave the old
	-- data issues in place with the carousel
	local function setInternalData(page)
		if not page then
			return {}
		end

		local newData = {}

		local placeIds = page:GetPagePlaceIds()
		local names = page:GetPagePlaceNames()
		local voteData = page:GetPageVoteData()
		local iconIds = page:GetPageIconIds()
		local creatorNames = page:GetCreatorNames()
		local creatorUserIds = page:GetPageCreatorUserIds()

		for i = 1, #page.Data do
			local gameEntry = {
				Title = names[i];
				PlaceId = placeIds[i];
				IconId = iconIds[i];
				VoteData = voteData[i];
				CreatorName = creatorNames[i];
				CreatorUserId = creatorUserIds[i];
				-- Description and IsFavorites needs to be queried for each game when needed
				Description = nil;
				IsFavorited = nil;
				GameData = nil;
			}
			table.insert(newData, gameEntry)
		end

		return newData
	end

	local function createPage(startIndex)
		local page = {}
		local items = {}

		for i = 1, PAGE_SIZE do
			local dataIndex = startIndex + (i - 1)
			if sortData[dataIndex] then
				local item = getNewItem(sortData[dataIndex])
				item.MouseButton1Click:connect(function()
					local currentData = sortData[dataIndex]
					if currentData then
						EventHub:dispatchEvent(EventHub.Notifications["OpenGameDetail"], currentData.PlaceId, currentData.Title, currentData.IconId, currentData.GameData)
					end
				end)
				table.insert(items, item)
			end
		end

		function page:GetCount()
			return #items
		end

		function page:GetItems()
			return items
		end

		function page:Destroy()
			for i,item in pairs(items) do
				items[i] = nil
				item:Destroy()
			end
		end

		return page
	end

	local previousFocusItem = nil
	local function onViewFocusChanged(newFocusItem)
		local offset = 0
		if previousFocusItem then
			offset = view:GetItemIndex(newFocusItem) - view:GetItemIndex(previousFocusItem)
		end

		local visibleItemCount = view:GetVisibleCount()
		local itemCount = view:GetCount()

		if offset > 0 then
			-- scrolled right
			local firstVisibleItemIndex = view:GetFirstVisibleItemIndex()
			if not isLoading and firstVisibleItemIndex + visibleItemCount + LOAD_BUFFER >= itemCount then
				isLoading = true
				local page = createPage(lastIndex + 1)
				if page:GetCount() > 0 then
					local items = page:GetItems()
					view:InsertCollectionBack(items)
					table.insert(pages, page)
					lastIndex = lastIndex + page:GetCount()
					-- remove front page
					if view:GetCount() > PAGE_SIZE * MAX_PAGES then
						local firstPage = table.remove(pages, 1)
						firstIndex = firstIndex + firstPage:GetCount()
						view:RemoveAmountFromFront(firstPage:GetCount())
						firstPage:Destroy()
					end
				end
				isLoading = false
			end
		elseif offset < 0 then
			-- scrolled left
			local lastVisibleItemIndex = view:GetLastVisibleItemIndex()
			if not isLoading and lastVisibleItemIndex - visibleItemCount - LOAD_BUFFER < 0 then
				isLoading = true
				local page = createPage(firstIndex - PAGE_SIZE)
				if page:GetCount() > 0 then
					local items = page:GetItems()
					view:InsertCollectionFront(items)
					table.insert(pages, 1, page)
					firstIndex = firstIndex - page:GetCount()
					if view:GetCount() > PAGE_SIZE * MAX_PAGES then
						local lastPage = table.remove(pages)
						lastIndex = lastIndex - lastPage:GetCount()
						view:RemoveAmountFromBack(lastPage:GetCount())
						lastPage:Destroy()
					end
				end
				isLoading = false
			end
		end

		sortDataIndex = sortDataIndex + offset
		previousFocusItem = newFocusItem
		view:ChangeFocus(newFocusItem)
		currentFocusData = sortData[sortDataIndex]
		this.NewItemSelected:fire(currentFocusData)
	end

	--[[ Public API ]]--
	function this:GetCurrentFocusGameData()
		return currentFocusData
	end

	function this:InitializeAsync(gameCollection)
		view:RemoveAllItems()
		firstIndex = 1
		lastIndex = 1
		currentFocusData = nil
		pages = {}
		sortData = {}

		sortCollection = gameCollection

		local page = gameCollection:GetSortAsync(0, LOAD_AMOUNT)
		sortData = setInternalData(page)

		local firstPage = createPage(firstIndex)
		if firstPage:GetCount() > 0 then
			local items = firstPage:GetItems()
			view:InsertCollectionBack(items)
			table.insert(pages, firstPage)
			lastIndex = firstPage:GetCount()

			local frontViewItem = view:GetFront()
			if frontViewItem then
				previousFocusItem = frontViewItem
				sortDataIndex = 1
				currentFocusData = sortData[sortDataIndex]
				view:SetFocus(frontViewItem)
			end
		end
	end

	function this:Connect()
		guiServiceChangedCn = Utility.DisconnectEvent(guiServiceChangedCn)
		guiServiceChangedCn = GuiService.Changed:connect(function(property)
			if property ~= 'SelectedCoreObject' then
				return
			end
			local newSelection = GuiService.SelectedCoreObject
			if newSelection and view:ContainsItem(newSelection) then
				onViewFocusChanged(newSelection)
			end
		end)

		local seenRightBumper = false
		local function onBumperRight(actionName, inputState, inputObject)
			if inputState == Enum.UserInputState.Begin then
				seenRightBumper = true
			elseif seenRightBumper and inputState == Enum.UserInputState.End then
				local currentSelection = GuiService.SelectedCoreObject
				if currentSelection and view:ContainsItem(currentSelection) then
					local currentFocusIndex = view:GetItemIndex(previousFocusItem)
					local shiftAmount = view:GetFullVisibleItemCount()
					local nextItem = view:GetItemAt(currentFocusIndex + shiftAmount) or view:GetBack()
					if nextItem then
						GuiService.SelectedCoreObject = nextItem
					end
				end
				seenRightBumper = false
			end
		end

		local seenLeftBumper = false
		local function onBumperLeft(actionName, inputState, inputObject)
			if inputState == Enum.UserInputState.Begin then
				seenLeftBumper = true
			elseif seenLeftBumper and inputState == Enum.UserInputState.End then
				local currentSelection = GuiService.SelectedCoreObject
				if currentSelection and view:ContainsItem(currentSelection) then
					local currentFocusIndex = view:GetItemIndex(previousFocusItem)
					local shiftAmount = view:GetFullVisibleItemCount()
					local nextItem = view:GetItemAt(currentFocusIndex - shiftAmount) or view:GetFront()
					if nextItem then
						GuiService.SelectedCoreObject = nextItem
					end
				end
				seenRightBumper = false
			end
		end

		-- Bumper Binds
		ContextActionService:BindCoreAction("BumperRight", onBumperRight, false, Enum.KeyCode.ButtonR1)
		ContextActionService:BindCoreAction("BumperLeft", onBumperLeft, false, Enum.KeyCode.ButtonL1)
	end

	function this:Disconnect()
		guiServiceChangedCn = Utility.DisconnectEvent(guiServiceChangedCn)
		ContextActionService:UnbindCoreAction("BumperRight")
		ContextActionService:UnbindCoreAction("BumperLeft")
	end

	return this
end

return createCarouselController
