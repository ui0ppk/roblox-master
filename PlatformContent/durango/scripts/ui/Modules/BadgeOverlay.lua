--[[
				// BadgeOverlay.lua

			// Displays information for a single badge
			// Used by GameDetail and BadgeScreen
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local ScrollingTextBox = require(Modules:FindFirstChild('ScrollingTextBox'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local BaseOverlay = require(Modules:FindFirstChild('BaseOverlay'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local createBadgeOverlay = function(badgeData)
	local this = BaseOverlay()

	local hasBadge = badgeData["IsOwned"]
	this:SetImageBackgroundTransparency(0)
	this:SetImageBackgroundColor(hasBadge and GlobalSettings.BadgeOwnedColor or GlobalSettings.BadgeOverlayColor)
	this:SetDropShadow()

	local badgeImage = Utility.Create'ImageLabel'
	{
		Name = "BadgeImage";
		Size = UDim2.new(0, 394, 0, 394);
		BackgroundTransparency = 1;
		Image = 'http://www.roblox.com/Thumbs/Asset.ashx?width='..
				tostring(250)..'&height='..tostring(250)..'&assetId='..tostring(badgeData.AssetId);
		ZIndex = this.BaseZIndex + 1;
		Parent = badgeImageContainer;
	}
	badgeImage.Position = UDim2.new(0.5, -197, 0.5, -197)
	this:SetImage(badgeImage)

	local titleText = Utility.Create'TextLabel'
	{
		Name = "TitleText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, this.RightAlign, 0, 88);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.HeaderSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = badgeData.Name;
		TextXAlignment = Enum.TextXAlignment.Left;
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}

	--[[ Has Badge ]]--
	local hasBadgeContainer = nil
	if hasBadge then
		hasBadgeContainer = Utility.Create'Frame'
		{
			Name = "HasBadgeContainer";
			Position = UDim2.new(0, titleText.Position.X.Offset, 0, titleText.Position.Y.Offset + 34);
			BackgroundTransparency = 1;
			Parent = this.Container;
		}
		local hasBadgeImage = Utility.Create'ImageLabel'
		{
			Name = "HasBadgeImage";
			BackgroundTransparency = 1;
			ZIndex = this.BaseZIndex;
			Parent = hasBadgeContainer;
		}
		AssetManager.LocalImage(hasBadgeImage,
			'rbxasset://textures/ui/Shell/Icons/Checkmark', {['720'] = UDim2.new(0,23,0,23); ['1080'] = UDim2.new(0,35,0,35);})
		hasBadgeContainer.Size = UDim2.new(0, 200, 0, hasBadgeImage.Size.Y.Offset)
		local hasBadgeText = Utility.Create'TextLabel'
		{
			Name = "HasBadgeText";
			Size = UDim2.new(0, 0, 0, 0);
			Position = UDim2.new(0, hasBadgeImage.Size.X.Offset + 12, 0.5, 0);
			BackgroundTransparency = 1;
			Font = GlobalSettings.ItalicFont;
			FontSize = GlobalSettings.DescriptionSize;
			TextColor3 = GlobalSettings.GreenTextColor;
			TextXAlignment = Enum.TextXAlignment.Left;
			Text = Strings:LocalizedString("HaveBadgeWord");
			ZIndex = this.BaseZIndex;
			Parent = hasBadgeContainer;
		}
	end

	--[[ Description ]]--
	local descriptionYOffset = hasBadgeContainer and hasBadgeContainer.Position.Y.Offset + hasBadgeContainer.Size.Y.Offset + 10 or
		titleText.Position.Y.Offset + 40

	local descriptionScrollingTextBox = ScrollingTextBox(UDim2.new(0, 762, 0, 304),
		UDim2.new(0, titleText.Position.X.Offset, 0, descriptionYOffset),
		this.Container)
	descriptionScrollingTextBox:SetText(badgeData.Description)
	descriptionScrollingTextBox:SetFontSize(GlobalSettings.TitleSize)
	descriptionScrollingTextBox:SetZIndex(this.BaseZIndex)
	local descriptionFrame = descriptionScrollingTextBox:GetContainer()

	local okButton = Utility.Create'TextButton'
	{
		Name = "OkButton";
		Size = UDim2.new(0, 320, 0, 66);
		Position = UDim2.new(0, titleText.Position.X.Offset, 1, -66 - 55);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.BlueButtonColor;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = GlobalSettings.TextSelectedColor;
		Text = string.upper(Strings:LocalizedString("OkWord"));
		ZIndex = this.BaseZIndex;
		Parent = this.Container;

		SoundManager:CreateSound('MoveSelection');
	}

	--[[ Input Events ]]--
	okButton.MouseButton1Click:connect(function()
		this:Close()
	end)
	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(this)
		GuiService.SelectedCoreObject = okButton
	end

	return this
end

return createBadgeOverlay
