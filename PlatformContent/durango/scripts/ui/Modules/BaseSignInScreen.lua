--[[
				// BaseSignInScreen.lua

				// Creates a base screen to be used for account linking and sign in
]]
local CoreGui = game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BaseScreen = require(Modules:FindFirstChild('BaseScreen'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local TextBox = require(Modules:FindFirstChild('TextBox'))
local Utility = require(Modules:FindFirstChild('Utility'))


local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local TERMS_OF_SERVICE_URI = "https://en.help.roblox.com/hc/en-us/articles/205358110"

local function createBaseAccountScreen()
	local this = BaseScreen()

	local DefaultButtonColor = GlobalSettings.GreyButtonColor
	local SelectedButtonColor = GlobalSettings.GreySelectedButtonColor
	local DefaultButtonTextColor = GlobalSettings.WhiteTextColor
	local SelectedButtonTextColor = GlobalSettings.TextSelectedColor

	local ScreenDivide = Utility.Create'Frame'
	{
		Name = "ScreenDivide";
		Size = UDim2.new(0, 2, 0, 610);
		Position = UDim2.new(0, 822, 0.5, -305);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.PageDivideColor;
		Parent = this.Container;
	}
	local DescriptionText = Utility.Create'TextLabel'
	{
		Name = "DescriptionText";
		Size = UDim2.new(0, 740, 0, 500);
		Position = UDim2.new(0, 16, 0, 334);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		TextWrapped = true;
		Text = "";
		Parent = this.Container;
	}

	local TermsOfServiceText = Utility.Create'TextLabel'
	{
		Name = "TermsOfServiceText";
		Size = UDim2.new(0, 740, 0, 128);
		Position = UDim2.new(0, 16, 0, 835);
		BackgroundTransparency = 1;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		TextWrapped = true;
		Text = Strings:LocalizedString("ToSInfoLinkPhrase");
		Parent = this.Container;
	}

	local UsernameObject = TextBox(UDim2.new(0, 600, 0, 84))
	UsernameObject:SetPosition(UDim2.new(0, ScreenDivide.Position.X.Offset + ScreenDivide.Size.X.Offset + 86, 0, 334))
	UsernameObject:SetSpacing(Vector2.new(20, 0))
	UsernameObject:SetParent(this.Container)
	local UsernameTextBox = UsernameObject:GetTextBox()
	local UsernameSelection = UsernameObject:GetContainer()
	this.UsernameObject = UsernameObject
	this.UsernameTextBox = UsernameTextBox
	this.UsernameSelection = UsernameSelection

	local PasswordObject = TextBox(UDim2.new(0, 600, 0, 84))
	PasswordObject:SetPosition(UDim2.new(0, ScreenDivide.Position.X.Offset + ScreenDivide.Size.X.Offset + 86, 0, 484))
	PasswordObject:SetSpacing(Vector2.new(20, 0))
	PasswordObject:SetParent(this.Container)
	local PasswordTextBox = PasswordObject:GetTextBox()
	local PasswordSelection = PasswordObject:GetContainer()
	this.PasswordObject = PasswordObject
	this.PasswordTextBox = PasswordTextBox
	this.PasswordSelection = PasswordSelection

	local function CreateSlicedTextButton(name, text, position)
		name = name or ""

		local newButton = Utility.Create'ImageButton'
		{
			Name = name .. "Button";
			Size = UDim2.new(0, 320, 0, 64);
			Position = position or UDim2.new();
			BackgroundTransparency = 1;
			ImageColor3 = DefaultButtonColor;
			Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
			ScaleType = Enum.ScaleType.Slice;
			SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
			ZIndex = 2;
			Parent = this.Container;

			SoundManager:CreateSound('MoveSelection');
			AssetManager.CreateShadow(1)
		}
		local newText = Utility.Create'TextLabel'
		{
			Name = name .. "Text";
			Size = UDim2.new(1, 0, 1, 0);
			BackgroundTransparency = 1;
			Font = GlobalSettings.RegularFont;
			FontSize = GlobalSettings.ButtonSize;
			TextColor3 = DefaultButtonTextColor;
			Text = text;
			ZIndex = 2;
			Parent = newButton;
		}

		newButton.SelectionGained:connect(function()
			newButton.ImageColor3 = SelectedButtonColor
			newText.TextColor3 = SelectedButtonTextColor
		end)
		newButton.SelectionLost:connect(function()
			newButton.ImageColor3 = DefaultButtonColor
			newText.TextColor3 = DefaultButtonTextColor
		end)

		return newButton, newText
	end

	local function TryLaunchUri(uri)
		local success, msg = pcall(function()
			assert(not UserSettings().GameSettings:InStudioMode(), "Can't use in studio")
			PlatformService:LaunchPlatformUri(uri)
		end)
		if not success then
			print(string.format("PlatformService:LaunchPlatformUri failed to launch uri: %s, for reason: %s", uri, msg))
		end
	end

	local SignInButton, SignInText = CreateSlicedTextButton("SignIn",
		string.upper(Strings:LocalizedString("SignInPhrase")),
		UDim2.new(0, PasswordSelection.Position.X.Offset, 0,
		          PasswordSelection.Position.Y.Offset + PasswordSelection.Size.Y.Offset + 66))
	this.SignInButton = SignInButton

	local tosButtonY = SignInButton.Position.Y.Offset -- + SignInButton.Size.Y.Offset + 30
	local ToSButton, ToSText = CreateSlicedTextButton("ToS",
		string.upper(Strings:LocalizedString("ToSPhrase")),
		UDim2.new(0, SignInButton.Position.X.Offset + SignInButton.Size.X.Offset + 10,
			      0, tosButtonY))
	ToSButton.Size = UDim2.new(0, 270, 0, ToSButton.Size.Y.Offset)

	local tosButtonLastPress = tick() - 1
	ToSButton.MouseButton1Click:connect(function()
		if tick() - tosButtonLastPress < 1 then return end
		tosButtonLastPress = tick()
		TryLaunchUri(TERMS_OF_SERVICE_URI)
	end)
--[[
	local PrivacyButton, PrivacyText = CreateSlicedTextButton("Privacy",
		string.upper(Strings:LocalizedString("PrivacyPhrase")),
		UDim2.new(0, ToSButton.Position.X.Offset + ToSButton.Size.X.Offset + 5,
			      0, tosButtonY))
	PrivacyButton.MouseButton1Click:connect(function()
		TryLaunchUri("http://www.roblox.com/info/Privacy.aspx")
	end)
--]]


	-- Override selection - issue with selections remembering their last selection, so in some cases
	-- the password selection become unselectable. I've talk to  Ben about this and we're going to fix it
	-- TODO: Remove this when selection memory is fixed
	UsernameSelection.NextSelectionDown = PasswordSelection
	SignInButton.NextSelectionUp = PasswordSelection

	--[[ Public API ]]--
	--Override
	function this:GetDefaultSelectionObject()
		return UsernameSelection
	end

	function this:SetDescriptionText(newText)
		DescriptionText.Text = newText
	end

	function this:SetButtonText(newText)
		SignInText.Text = string.upper(newText)
	end

	return this
end

return createBaseAccountScreen
