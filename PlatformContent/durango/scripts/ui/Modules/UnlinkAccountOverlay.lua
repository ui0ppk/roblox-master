--[[
				// UnlinkAccountOverlay.lua

				// Confirmation overlay for when you unlink your account
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BaseOverlay = require(Modules:FindFirstChild('BaseOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local function createUnlinkAccountOverlay(titleAndMsg)
	local this = BaseOverlay()

	local title = titleAndMsg.Title
	local message = titleAndMsg.Msg

	local errorIcon = Utility.Create'ImageLabel'
	{
		Name = "ReportIcon";
		Position = UDim2.new(0, 226, 0, 204);
		BackgroundTransparency = 1;
		ZIndex = this.BaseZIndex;
	}
	AssetManager.LocalImage(errorIcon, 'rbxasset://textures/ui/Shell/Icons/ErrorIconLargeCopy',
		{['720'] = UDim2.new(0,214,0,176); ['1080'] = UDim2.new(0,321,0,264);})
	this:SetImage(errorIcon)

	local titleText = Utility.Create'TextLabel'
	{
		Name = "TitleText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, this.RightAlign, 0, 136);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.HeaderSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = title;
		TextXAlignment = Enum.TextXAlignment.Left;
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}

	local descriptionText = Utility.Create'TextLabel'
	{
		Name = "DescriptionText";
		Size = UDim2.new(0, 762, 0, 304);
		Position = UDim2.new(0, this.RightAlign, 0, titleText.Position.Y.Offset + 62);
		BackgroundTransparency = 1;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextWrapped = true;
		Text = message;
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}

	local okButton = Utility.Create'TextButton'
	{
		Name = "OkButton";
		Size = UDim2.new(0, 320, 0, 66);
		Position = UDim2.new(0, this.RightAlign, 1, -100 - 66);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.BlueButtonColor;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = GlobalSettings.TextSelectedColor;
		Text = string.upper(Strings:LocalizedString("ConfirmWord"));
		ZIndex = this.BaseZIndex;
		Parent = this.Container;

		SoundManager:CreateSound('MoveSelection');
	}

	--[[ Input Events ]]--
	local okButtonDebounce
	okButton.MouseButton1Click:connect(function()
		if this:Close() then
			EventHub:dispatchEvent(EventHub.Notifications["UnlinkAccountConfirmation"])
		end
	end)

	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(self)
		GuiService.SelectedCoreObject = okButton
	end

	function this:GetOverlaySound()
		return 'Error'
	end

	return this
end

return createUnlinkAccountOverlay
