-- Copyright ROBLOX 2015
--[[
		Filename: GamePane.lua
		Written By: Kyler Mulherin, Jason Roth

		TODO:
			Make responsive
]]

local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local GuiService = game:GetService('GuiService')

local GameSort = require(Modules:FindFirstChild('GameSort'))
local SortData = require(Modules:FindFirstChild('SortData'))
local ScrollingGrid = require(Modules:FindFirstChild('ScrollingGrid'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local GameCollection = require(Modules:FindFirstChild('GameCollection'))

--CONSTANTS
local SPACING = 33

local function CreateGamePane(parent)
	local this = {}

	local sortCatagories = nil
	local imageSize = UDim2.new(0, 298, 0, 298)
	local spacing = Vector2.new(14, 14)
	local rows, columns = 2, 2
	local views = {}


	local inFocus = false

	local noSelectionObject = Utility.Create'ImageLabel'
	{
		BackgroundTransparency = 1;
	}

	-- UI Elements
	local GamePaneContainer = Utility.Create'Frame'
	{
		Name = 'GamePane';
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Parent = parent;
		SelectionImageObject = noSelectionObject;
		Visible = false;
	}

	local gameSortsPane  = ScrollingGrid()
	gameSortsPane:SetPosition(UDim2.new(0, 0, 0, 0))
	gameSortsPane:SetCellSize(Vector2.new(610, 646))
	gameSortsPane:SetSize(UDim2.new(1, GlobalSettings.TitleSafeInset.X.Offset, 1, 0))
	gameSortsPane:SetScrollDirection(gameSortsPane.Enum.ScrollDirection.Horizontal)
	gameSortsPane:SetParent(GamePaneContainer)
	gameSortsPane:SetClipping(false)
	gameSortsPane:SetSpacing(Vector2.new(SPACING, SPACING))
	gameSortsPane.Container.Visible = false

	local function setSelectionOnLoad()
		if this:IsFocused() then
			GuiService.SelectedCoreObject = this:GetDefaultSelection()
		end
	end

	local setSortsDebounce = false
	local function setSortView()
		if setSortsDebounce then return end
		setSortsDebounce = true
		if not sortCatagories then
			spawn(function()
				sortCatagories = SortData.GetSortCategoriesAsync()
			end)
		end

		local function createView(gameCollection, name)
			local sortPage = gameCollection:GetSortAsync(0, 5)

			if sortPage and sortPage.Count > 0 then
				local view = GameSort:CreateGridView(UDim2.new(), imageSize, spacing, rows, columns)
				local iconIds = sortPage:GetPageIconIds()
				local names = sortPage:GetPagePlaceNames()
				local placeIds = sortPage:GetPagePlaceIds()
				view:SetImages(iconIds)
				view:ConnectInput(placeIds, names, iconIds, name, gameCollection)
				view:SetTitle(name)
				return view
			end
			return nil
		end

		local newViews = {}
		local function loadSortsAsync()
			while not sortCatagories do
				wait()
			end

			-- create all the views
			for i = 1, #sortCatagories do
				local sort = sortCatagories[i]
				local sortCollection = GameCollection:GetSort(sort["Id"])
				table.insert(newViews, createView(sortCollection, sort["Name"]))
			end

			local favoriteGamesCollection = GameCollection:GetUserFavorites()
			table.insert(newViews, createView(favoriteGamesCollection, Strings:LocalizedString("FavoritesSortTitle")))

			local recentGamesCollection = GameCollection:GetUserRecent()
			table.insert(newViews, createView(recentGamesCollection, Strings:LocalizedString("RecentlyPlayedSortTitle")))

			local userPlacesCollection = GameCollection:GetUserPlaces()
			table.insert(newViews, createView(userPlacesCollection, Strings:LocalizedString("PlayMyPlaceMoreGamesTitle")))
		end
		local loader = LoadingWidget(
			{ Parent = GamePaneContainer }, { loadSortsAsync })
		spawn(function()
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil

			-- Remove old items
			gameSortsPane:RemoveAllItems()
			views = newViews
			for i = 1, #views do
				gameSortsPane:AddItem(views[i]:GetContainer())
			end
			gameSortsPane.Container.Visible = true
			setSelectionOnLoad()
			if GamePaneContainer.Visible then
				this.TransitionTweens = ScreenManager:DefaultFadeIn(gameSortsPane.Container)
				ScreenManager:PlayDefaultOpenSound()
			end

			setSortsDebounce = false
		end)
	end

	-- SCREEN FUNCTIONS
	function this:SetPosition(newPosition)
		GamePaneContainer.Position = newPosition
	end
	function this:SetParent(newParent)
		GamePaneContainer.Parent = newParent
	end

	function this:IsAncestorOf(object)
		return GamePaneContainer and GamePaneContainer:IsAncestorOf(object)
	end

	function this:GetName()
		return Strings:LocalizedString('GameWord')
	end

	function this:GetDefaultSelection()
		for i = 1, #views do
			if views[i]:GetDefaultSelection() then
				return views[i]:GetDefaultSelection()
			end
		end
		return GamePaneContainer
	end

	function this:ViewsContainObject(guiObject)
		if guiObject then
			for i = 1, #views do
				if views[i]:Contains(guiObject) then
					return true
				end
			end
		end
		return false
	end

	function this:IsFocused()
		return inFocus
	end

	function this:Show()
		ScreenManager:DefaultCancelFade(self.TransitionTweens)
		setSortView()
		GamePaneContainer.Visible = true
	end
	function this:Hide()
		GamePaneContainer.Visible = false
		gameSortsPane.Container.Visible = false

		ScreenManager:DefaultCancelFade(self.TransitionTweens)
		self.TransitionTweens = nil
	end
	function this:Focus()
		inFocus = true

		if self.SavedSelectObject and self:ViewsContainObject(self.SavedSelectObject) then
			GuiService.SelectedCoreObject = self.SavedSelectObject
		else
			GuiService.SelectedCoreObject = self:GetDefaultSelection()
		end

		-- Clear out the saved selected object, it has done its work
		self.SavedSelectObject = nil
	end
	function this:RemoveFocus()
		inFocus = false

		local selectedObject = GuiService.SelectedCoreObject
		if selectedObject and self:ViewsContainObject(selectedObject) then
			self.SavedSelectObject = selectedObject
		else
			self.SavedSelectObject = nil
		end

		if selectedObject and (selectedObject == GamePaneContainer or self:IsAncestorOf(selectedObject)) then
			GuiService.SelectedCoreObject = nil
		end
	end

	--[[ Initialize - Don't Block ]]--
	spawn(function()
		sortCatagories = SortData.GetSortCategoriesAsync()
	end)

	return this
end

return CreateGamePane
