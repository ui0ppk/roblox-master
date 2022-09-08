--[[
			// GameSort.lua
			// Creates a grid layout for a game sort
]]

local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local PopupText = require(Modules:FindFirstChild('PopupText'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local GameCollection = require(Modules:FindFirstChild('GameCollection'))

local GameSort = {}

local function createSortContainer(size)
	local sortContainer = Utility.Create'Frame'
	{
		Name = "SortContainer";
		Size = size;
		BackgroundTransparency = 1;
	}

	return sortContainer
end

local function createSortTitle(name)
	local sortTitleLabel = Utility.Create'TextLabel'
	{
		Name = "SortTitle";
		Size = UDim2.new(1, 0, 0, 36);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.SubHeaderSize;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = string.upper(name);
		Parent = sortContainer;
	}

	return sortTitleLabel
end

local function createMoreButton(margin, parent)
	-- we override the selection on moreButton to fit around the moreImage
	local overrideSelection = Utility.Create'ImageLabel'
	{
		Name = "OverrideSelection";
		Image = 'rbxasset://textures/ui/SelectionBox.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(19,19,43,43);
		BackgroundTransparency = 1;
	}

	local moreButton = Utility.Create'ImageButton'
	{
		Name = "MoreButton";
		Size = UDim2.new(1, 0, 0, 50);
		Position = UDim2.new(0, 0, 1, margin);
		BackgroundTransparency = 1;
		ZIndex = 2;
		Visible = false;
		SelectionImageObject = overrideSelection;
		Parent = parent;
		SoundManager:CreateSound('MoveSelection');
	}
	local moreImage = Utility.Create'ImageLabel'
	{
		Name = "MoreImage";
		BackgroundTransparency = 1;
		BorderSizePixel = 0;
		ZIndex = 2;
		Parent = moreButton;
	}

	local function updateMoreImage(isSelected)
		local uri = isSelected and 'rbxasset://textures/ui/Shell/Buttons/MoreButtonSelected' or 'rbxasset://textures/ui/Shell/Buttons/MoreButton'
		AssetManager.LocalImage(moreImage, uri, {['720'] = UDim2.new(0,72,0,33); ['1080'] = UDim2.new(0,108,0,50);})
		moreImage.Position = UDim2.new(1, -moreImage.Size.X.Offset, 0, 0)

		overrideSelection.Size = UDim2.new(0, moreImage.Size.X.Offset + 14, 0, moreImage.Size.Y.Offset + 14)
		overrideSelection.Position = UDim2.new(1, -overrideSelection.Size.X.Offset + 7, 0, -7)
	end

	moreButton.SelectionGained:connect(function()
		updateMoreImage(true)
	end)
	moreButton.SelectionLost:connect(function()
		updateMoreImage(false)
	end)

	updateMoreImage(GuiService.SelectedCoreObject == moreButton)

	return moreButton
end

local function createImageButton(size, position)
	return Utility.Create'ImageButton'
		{
			Name = "GameThumbButton";
			Size = size;
			Position = position;
			BackgroundTransparency = 0;
			BorderSizePixel = 0;
			BackgroundColor3 = GlobalSettings.ModalBackgroundColor;
			ZIndex = 2;
			AssetManager.CreateShadow(1);
			SoundManager:CreateSound('MoveSelection');
		}
end

local function createImageGrid(rows, columns, size, spacing, offset, images)
	for i = 1, rows do
		for j = 1, columns do
			local image = createImageButton(size,
				UDim2.new(0, (j - 1) * size.X.Offset + (j - 1) * spacing.x, 0, offset + (i - 1) * size.Y.Offset + (i - 1) * spacing.y))
			table.insert(images, image)
			image.Name = tostring(#images)
		end
	end
end

local function createPopupText(images)
	local popupText = {}
	for i = 1, #images do
		local popup = PopupText(images[i], "")
		table.insert(popupText, popup)
	end

	return popupText
end

local function setImages(imageIds, images)
	local size = ThumbnailLoader.Sizes.Medium
	local assetType = ThumbnailLoader.AssetType.Icon
	for i = 1, #images do
		if imageIds[i] then
			local thumbLoader = ThumbnailLoader:Create(images[i], imageIds[i], size, assetType)
			spawn(function()
				if not thumbLoader:LoadAsync(true, false) then
					-- TODO
				end
			end)
		else
			images[i].BackgroundColor3 = GlobalSettings.GreyButtonColor
			images[i].Image = ""
			local image = Utility.Create'ImageLabel'
			{
				Name = "NoGameImage";
				Size = UDim2.new(0, 102, 0, 102);
				BackgroundTransparency = 1;
				Image = 'rbxasset://textures/ui/Shell/Icons/GamePlusIcon.png';
				ZIndex = images[i].ZIndex;
				Parent = images[i];
			}
			Utility.CalculateAnchor(image, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
		end
	end
end

local function createBaseGrid(size, spacing, images)
	local this = {}

	this.Container = createSortContainer(size)
	this.Title = createSortTitle("")
	this.Title.Parent = this.Container
	this.MoreButton = createMoreButton(spacing, this.Container)

	for i = 1, #images do
		images[i].Parent = this.Container
	end
	local popupText = createPopupText(images)

	function this:SetParent(newParent)
		self.Container.Parent = newParent
	end
	function this:GetContainer()
		return self.Container
	end
	function this:SetPosition(newPosition)
		self.Container.Position = newPosition
	end
	function this:SetTitle(newTitle)
		self.Title.Text = string.upper(newTitle)
	end
	function this:SetVisible(value)
		self.Container.Visible = value
	end
	function this:SetImages(imageIds)
		setImages(imageIds, images)
	end
	function this:ConnectInput(ids, names, iconIds, sortName, gameCollection)
		for i = 1, #images do
			if ids[i] then
				images[i].MouseButton1Click:connect(function()
					EventHub:dispatchEvent(EventHub.Notifications["OpenGameDetail"], ids[i], names[i], iconIds[i])
				end)
			else
				images[i].MouseButton1Click:connect(function()
					-- If there are not enough games in my games section then let them know how to make more!
					if gameCollection == GameCollection:GetUserPlaces() then
						local titleAndMsg = {
							Title = Strings:LocalizedString('PlayMyPlaceMoreGamesTitle');
							Msg = Strings:LocalizedString('PlayMyPlaceMoreGamesPhrase');
						}
						ScreenManager:OpenScreen(ErrorOverlay(titleAndMsg, true), false)
					else
						-- if there is no game to load in this slot, we're going to redirect to the featured sort if the user presses this button
						EventHub:dispatchEvent(EventHub.Notifications["OpenGameGenre"], Strings:LocalizedString('FeaturedTitle'), GameCollection:GetSort(3))
					end
				end)
			end
			if popupText[i] then
				popupText[i]:SetText(names[i] or Strings:LocalizedString("MoreGamesPhrase"))
			end
		end
		self.MoreButton.Visible = #ids > #images
		self.MoreButton.MouseButton1Click:connect(function()
			EventHub:dispatchEvent(EventHub.Notifications["OpenGameGenre"], sortName, gameCollection)
		end)
	end
	function this:GetDefaultSelection()
		local default = nil
		if #images > 0 then
			default = images[1]
		end
		return default
	end
	function this:Contains(guiObject)
		for i = 1, #images do
			if images[i] == guiObject then
				return true
			end
		end
		return false
	end
	function this:Destroy()
		self.Container:Destroy()
	end

	return this
end

function GameSort:CreateMainGridView(size, spacing, lgImageSize, smImageSize)
	local images = {}
	local mainImage = createImageButton(lgImageSize, UDim2.new(0, 0, 0, 36))
	table.insert(images, mainImage)
	mainImage.Name = tostring(#images)

	for i = 1, 2 do
		local smImage = createImageButton(smImageSize,
			UDim2.new(0, (i - 1) * smImageSize.X.Offset + (i - 1) * spacing.x, 1, -smImageSize.Y.Offset))
		table.insert(images, smImage)
		smImage.Name = tostring(#images)
	end

	local this = createBaseGrid(size, 12, images)

	return this
end

-- 2x2 grid for the games page
function GameSort:CreateGridView(size, imageSize, spacing, rows, columns)
	local images = {}
	createImageGrid(rows, columns, imageSize, spacing, 36, images)

	local this = createBaseGrid(size, 12, images)

	return this
end

return GameSort
