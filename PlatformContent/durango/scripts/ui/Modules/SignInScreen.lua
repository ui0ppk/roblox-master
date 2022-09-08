--[[
			// SignInScreen.lua
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local GuiService = game:GetService('GuiService')
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)
local TextService = game:GetService('TextService')
local UserInputService = game:GetService('UserInputService')

local AccountManager = require(Modules:FindFirstChild('AccountManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local LinkAccountScreen = require(Modules:FindFirstChild('LinkAccountScreen'))
local SetAccountCredentialsScreen = require(Modules:FindFirstChild('SetAccountCredentialsScreen'))

local function createSignInScreen()
	local this = {}

	local isFocused = false

	local DefaultButtonColor = GlobalSettings.GreyButtonColor
	local SelectedButtonColor = GlobalSettings.GreySelectedButtonColor
	local DefaultButtonTextColor = GlobalSettings.WhiteTextColor
	local SelectedButtonTextColor = GlobalSettings.TextSelectedColor

	-- get gamertag and set display text
	local createAccountText = Strings:LocalizedString("PlayAsPhrase")
	if PlatformService then
		local userInfo = PlatformService:GetPlatformUserInfo()
		local gamertag = userInfo["Gamertag"]
		if gamertag then
			createAccountText = string.format(createAccountText, gamertag)
		end
	end

	local ModalOverlay = Utility.Create'Frame'
	{
		Name = "ModalOverlay";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = GlobalSettings.ModalBackgroundTransparency;
		BackgroundColor3 = GlobalSettings.ModalBackgroundColor;
		BorderSizePixel = 0;
		ZIndex = 4;
	}

	local Container = Utility.Create'Frame'
	{
		Name = "SignInScreen";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Visible = false;
	}
	local RobloxLogo = Utility.Create'ImageLabel'
	{
		Name = "RobloxLogo";
		Size = UDim2.new(0, 594, 0, 199);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/ROBLOXSplashLogo.png';
		Parent = Container;
	}
	Utility.CalculateAnchor(RobloxLogo, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)

	local FadeInFrame = Utility.Create'Frame'
	{
		Name = "FadeInFrame";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Visible = false;
		Parent = Container;
	}

	local LinkAccountButton = Utility.Create'ImageButton'
	{
		Name = "LinkAccountButton";
		Size = UDim2.new(0, 200, 0, 64);
		Position = UDim2.new(0.5, -100, 1, -64 - 140);
		BackgroundTransparency = 1;
		ImageColor3 = DefaultButtonColor;
		Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
		ZIndex = 2;
		Parent = FadeInFrame;

		SoundManager:CreateSound('MoveSelection');
		AssetManager.CreateShadow(1)
	}

	local CreateAccountButton = LinkAccountButton:Clone()
	CreateAccountButton.Name = "CreateAccountButton"
	CreateAccountButton.Size = UDim2.new(0, 360, 0, 64);
	CreateAccountButton.Position = UDim2.new(0.5, -180, 1, LinkAccountButton.Position.Y.Offset - 64 - 44)
	CreateAccountButton.Parent = FadeInFrame

	local LinkAccountText = Utility.Create'TextLabel'
	{
		Name = "LinkAccountText";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = DefaultButtonTextColor;
		Text = string.upper(Strings:LocalizedString("SignInPhrase"));
		ZIndex = 2;
		Parent = LinkAccountButton;
	}
	local CreateAccountText = LinkAccountText:Clone()
	CreateAccountText.Text = string.upper(createAccountText)
	CreateAccountText.Parent = CreateAccountButton
	local createAccountTextSize = TextService:GetTextSize(createAccountText, Utility.ConvertFontSizeEnumToInt(CreateAccountText.FontSize),
		CreateAccountText.Font, Vector2.new(0, 0))
	CreateAccountButton.Size = UDim2.new(0, createAccountTextSize.X + 64, 0, CreateAccountButton.Size.Y.Offset)
	CreateAccountButton.Position = UDim2.new(0.5, -CreateAccountButton.Size.X.Offset/2, 1, CreateAccountButton.Position.Y.Offset)

	CreateAccountButton.SelectionGained:connect(function()
		CreateAccountButton.ImageColor3 = SelectedButtonColor
		CreateAccountText.TextColor3 = SelectedButtonTextColor
	end)
	CreateAccountButton.SelectionLost:connect(function()
		CreateAccountButton.ImageColor3 = DefaultButtonColor
		CreateAccountText.TextColor3 = DefaultButtonTextColor
	end)
	LinkAccountButton.SelectionGained:connect(function()
		LinkAccountButton.ImageColor3 = SelectedButtonColor
		LinkAccountText.TextColor3 = SelectedButtonTextColor
	end)
	LinkAccountButton.SelectionLost:connect(function()
		LinkAccountButton.ImageColor3 = DefaultButtonColor
		LinkAccountText.TextColor3 = DefaultButtonTextColor
	end)

	local function animateOnShow()
		ScreenManager:DefaultCancelFade(this.TransitionTweens)
		FadeInFrame.Visible = true
		this.TransitionTweens = ScreenManager:FadeInSitu(FadeInFrame)
		if isFocused then
			GuiService.SelectedCoreObject = CreateAccountButton
		end
	end

	--[[ Input ]]--
	CreateAccountButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		local setAccountCredentialsScreen = SetAccountCredentialsScreen(Strings:LocalizedString("SignUpTitle"),
			Strings:LocalizedString("SignUpPhrase"), Strings:LocalizedString("SignUpWord"))
		setAccountCredentialsScreen:SetParent(Container.Parent)
		ScreenManager:OpenScreen(setAccountCredentialsScreen, true)
	end)

	LinkAccountButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		local linkAccountScreen = LinkAccountScreen()
		linkAccountScreen:SetParent(Container.Parent)
		ScreenManager:OpenScreen(linkAccountScreen, true)
	end)

	--[[ Public API ]]--
	function this:SetParent(newParent)
		Container.Parent = newParent
	end
	function this:Show()
		Utility.CalculateAnchor(RobloxLogo, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)
		Container.Visible = true
		animateOnShow()
	end
	function this:Hide()
		Container.Visible = false
		FadeInFrame.Visible = false
	end
	function this:Focus()
		isFocused = true
		GuiService:AddSelectionParent("SignInScreen", FadeInFrame)
		GuiService.SelectedCoreObject = CreateAccountButton
	end
	function this:RemoveFocus()
		isFocused = false
		GuiService:RemoveSelectionGroup("SignInScreen")
		GuiService.SelectedCoreObject = nil
	end

	return this
end

return createSignInScreen
