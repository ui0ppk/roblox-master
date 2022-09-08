--[[
			// BadgeScreen.lua

			// Displays a 2xN grid of badges for a game
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local ContextActionService = game:GetService("ContextActionService")
local GuiService = game:GetService('GuiService')

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BadgeOverlayModule = require(Modules:FindFirstChild('BadgeOverlay'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local ScrollingGrid = require(Modules:FindFirstChild('ScrollingGrid'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local PopupText = require(Modules:FindFirstChild('PopupText'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local BaseScreen = require(Modules:FindFirstChild('BaseScreen'))

local createBadgeScreen = function(badgeData)
	local this = BaseScreen()

	local ROWS = 2
	-- columns is dynamic

	local BadgeContainer = this.Container
	this:SetTitle(string.upper(Strings:LocalizedString("GameBadgesTitle")))

	local defaultSelection = nil

	-- create grid
	local BadgeScrollGrid = ScrollingGrid()
	BadgeScrollGrid:SetPosition(UDim2.new(0, 0, 0.5 - (0.57 / 2), 0))
	BadgeScrollGrid:SetSize(UDim2.new(1, 0, 0, 570))
	BadgeScrollGrid:SetScrollDirection(BadgeScrollGrid.Enum.ScrollDirection.Horizontal)
	BadgeScrollGrid:SetParent(BadgeContainer)
	BadgeScrollGrid:SetClipping(false)
	BadgeScrollGrid:SetCellSize(Vector2.new(276, 276))
	BadgeScrollGrid:SetSpacing(Vector2.new(18, 18))

	local checkmarkImage = Utility.Create'ImageLabel'
	{
		Name = "CheckMarkImage";
		BackgroundTransparency = 1;
		ZIndex = 2;
	}
	AssetManager.LocalImage(checkmarkImage, 'rbxasset://textures/ui/Shell/Icons/Checkmark',
		{['720'] = UDim2.new(0,23,0,23); ['1080'] = UDim2.new(0,35,0,35);})

	local function connectImageInput(image, data)
		image.MouseButton1Click:connect(function()
			-- Do not play sound because we are opening a screen here
			ScreenManager:OpenScreen(BadgeOverlayModule(data), false)
		end)
	end

	local baseItem = Utility.Create'TextButton'
	{
		Name = "BadgeImage";
		BorderSizePixel = 0;
		BackgroundTransparency = 0.2;
		BackgroundColor3 = Color3.new(64/255, 81/255, 93/255);
		Text = "";

		SoundManager:CreateSound('MoveSelection');
		ZIndex = 2;
		AssetManager.CreateShadow(1);
	}
	local badgeIcon = Utility.Create'ImageLabel'
	{
		Name = "Thumb";
		Size = UDim2.new(0, 228, 0, 228);
		Position = UDim2.new(0.5, -228/2, 0.5, -228/2);
		BackgroundTransparency = 1;
		Image = "";
		ZIndex = 2;
		Parent = baseItem;
	}

	for i = 1, #badgeData do
		local data = badgeData[i]
		local item = baseItem:Clone()
		local thumb = item:FindFirstChild("Thumb")
		if thumb then
			local thumbLoader = ThumbnailLoader:Create(thumb, data.AssetId,
				ThumbnailLoader.Sizes.Medium, ThumbnailLoader.AssetType.Icon)
			spawn(function()
				thumbLoader:LoadAsync()
			end)
		end
		local hasBadge = data["IsOwned"]
		if hasBadge then
			item.BackgroundColor3 = GlobalSettings.BadgeOwnedColor;
			item.BackgroundTransparency = 0;
			--
			local check = checkmarkImage:Clone()
			check.Position = UDim2.new(1, -check.Size.X.Offset - 8, 0, 8)
			check.Parent = item
		end
		item.Name = tostring(i)
		BadgeScrollGrid:AddItem(item)
		connectImageInput(item, data)
		PopupText(item, data["Name"])
		if not defaultSelection then
			defaultSelection = item
		end
	end

	--[[ Public API ]]--
	--Override
	function this:GetDefaultSelectionObject()
		return defaultSelection
	end

	return this
end

return createBadgeScreen
