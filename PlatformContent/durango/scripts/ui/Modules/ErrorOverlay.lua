--[[
			// ErrorOverlay.lau

			// Creates and error overlay

			// NOTE: Right now error and alerts look the same, so we're
			// using the same module to make both. If in the future this
			// changes, we'll need to move alert to it's own module.
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local BaseOverlay = require(Modules:FindFirstChild('BaseOverlay'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local createErrorOverlay = function(errorType, isAlert)
	if not errorType then
		return
	end

	local this = BaseOverlay()

	local title = errorType.Title
	local message = errorType.Msg
	local errorCode = errorType.Code

	local iconImage = Utility.Create'ImageLabel'
	{
		Name = "IconImage";
		BackgroundTransparency = 1;
		ZIndex = this.BaseZIndex;
	}
	iconImage.Image = isAlert and 'rbxasset://textures/ui/Shell/Icons/AlertIcon.png' or
		'rbxasset://textures/ui/Shell/Icons/ErrorIconLargeCopy@1080.png'
	iconImage.Size = isAlert and UDim2.new(0, 416, 0, 416) or UDim2.new(0, 321, 0, 264)
	Utility.CalculateAnchor(iconImage, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
	this:SetImage(iconImage)

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
	if errorCode and not isAlert then
		descriptionText.Text = string.format(Strings:LocalizedString('ErrorMessageAndCodePrase'), message, errorCode)
	end

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
		Text = string.upper(Strings:LocalizedString("OkWord"));
		ZIndex = this.BaseZIndex;
		Parent = this.Container;

		SoundManager:CreateSound('MoveSelection');
	}

	function this:GetPriority()
		return GlobalSettings.ElevatedPriority
	end

	--[[ Input Events ]]--
	okButton.MouseButton1Click:connect(function()
		this:Close()
	end)
	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(this)
		GuiService.SelectedCoreObject = okButton
	end

	function this:GetOverlaySound()
		return 'Error'
	end

	return this
end

return createErrorOverlay
