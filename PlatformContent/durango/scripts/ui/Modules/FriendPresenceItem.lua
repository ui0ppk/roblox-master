--[[
			// FriendPresenceItem.lua
			// Creates a friend activity gui item to be used with a ScrollingGrid
			// for friends social status
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local FAIL_IMG = 'rbxasset://textures/ui/Shell/Icons/DefaultProfileIcon.png'

local function FriendPresenceItem(size, idStr)
	local this = {}

	local TEXT_OFFSET = 12

	local container = Utility.Create'ImageButton'
	{
		Name = idStr;
		Size = size;
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundColor3 = GlobalSettings.WhiteTextColor;
		BorderSizePixel = 0;
		BackgroundTransparency = 1;
		AutoButtonColor = false;

		SoundManager:CreateSound('MoveSelection');
	}
	container.SelectionGained:connect(function()
		local startTween = Utility.PropertyTweener(container, "BackgroundTransparency", 1, GlobalSettings.AvatarBoxBackgroundSelectedTransparency, 0,
			Utility.EaseInOutQuad, true, nil)
	end)
	container.SelectionLost:connect(function()
		container.BackgroundTransparency = 1
	end)
		local avatarImage = Utility.Create'ImageLabel'
		{
			Name = "AvatarImage";
			Image = '';
			Size = UDim2.new(0, 104, 0, 104);
			Position = UDim2.new(0, 0, 0, 0);
			BackgroundTransparency = 0;
			BorderSizePixel = 0;
			BackgroundColor3 = GlobalSettings.CharacterBackgroundColor;
			ZIndex = 2;
			Parent = container;
			AssetManager.CreateShadow(1);
		}
		local nameLabel = Utility.Create'TextLabel'
		{
			Name = "NameLabel";
			Text = "";
			Size = UDim2.new(0, 0, 0, 0);
			Position = UDim2.new(0, avatarImage.Size.X.Offset + TEXT_OFFSET, 0, 32);
			TextXAlignment = Enum.TextXAlignment.Left;
			TextColor3 = GlobalSettings.WhiteTextColor;
			Font = GlobalSettings.RegularFont;
			FontSize = GlobalSettings.TitleSize;
			BackgroundTransparency = 1;
			Parent = container;
		}
		local presenceStatusImage = Utility.Create'ImageLabel'
		{
			Name = "PresenceStatusImage";
			BackgroundTransparency = 1;
			Parent = container;
		}
		AssetManager.LocalImage(presenceStatusImage,
			'rbxasset://textures/ui/Shell/Icons/OnlineStatusIcon', {['720'] = UDim2.new(0,13,0,13); ['1080'] = UDim2.new(0,19,0,20);})
		presenceStatusImage.Position = UDim2.new(0, nameLabel.Position.X.Offset, 0,
			nameLabel.Position.Y.Offset + 36 - (presenceStatusImage.Size.Y.Offset/2) + 1)

		local presenceLabel = Utility.Create'TextLabel'
		{
			Name = "PresenceLabel";
			Text = "";
			Size = UDim2.new(0,
				container.Size.X.Offset - presenceStatusImage.Position.X.Offset - presenceStatusImage.Size.X.Offset - 12, 0, 32);
			Position = UDim2.new(0, presenceStatusImage.Position.X.Offset + presenceStatusImage.Size.X.Offset + 12, 0,
				presenceStatusImage.Position.Y.Offset + presenceStatusImage.Size.Y.Offset / 2 - 16);
			TextXAlignment = Enum.TextXAlignment.Left;
			TextColor3 = GlobalSettings.LightGreyTextColor;
			Font = GlobalSettings.RegularFont;
			FontSize = GlobalSettings.DescriptionSize;
			BackgroundTransparency = 1;
			ClipsDescendants = true;
			Parent = container;
		}
		local lineBreak = Utility.Create'Frame'
		{
			Name = "Break";
			Size = UDim2.new(1, -avatarImage.Size.X.Offset - TEXT_OFFSET, 0, 2);
			Position = UDim2.new(0, avatarImage.Size.X.Offset + TEXT_OFFSET, 1, -2);
			BorderSizePixel = 0;
			BackgroundColor3 = GlobalSettings.LineBreakColor;
			Parent = container;
		}

		local failImage = Utility.Create'ImageLabel'
		{
			Name = "FailImage";
			Size = UDim2.new(0.5, 0, 0.5, 0);
			Position = UDim2.new(0.25, 0, 0.25, 0);
			BackgroundTransparency = 1;
			Image = FAIL_IMG;
			ZIndex = 2;
		}

	function this:GetContainer()
		return container
	end

	function this:SetAvatarImage(userId)
		local loader = ThumbnailLoader:Create(avatarImage, userId,
			ThumbnailLoader.Sizes.Small, ThumbnailLoader.AssetType.Avatar)
		spawn(function()
			if not loader:LoadAsync(true, false) then
				failImage.Parent = avatarImage
			else
				failImage.Parent = nil
			end
		end)
	end

	function this:GetNameText()
		return nameLabel.Text
	end

	function this:SetNameText(name)
		nameLabel.Text = name
	end

	function this:SetPresence(str, isInRobloxGame)
		presenceLabel.Text = str
		presenceStatusImage.ImageColor3 = isInRobloxGame and GlobalSettings.GreenTextColor
			or GlobalSettings.GreySelectedButtonColor
	end

	function this:SetLastActivityText(str)
		lastActivityLabel.Text = str
	end

	function this:Destroy()
		container:Destroy()
		this = nil
	end

	return this
end

return FriendPresenceItem
