--[[
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BaseScreen = require(Modules:FindFirstChild('BaseScreen'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local AccountScreen = require(Modules:FindFirstChild('AccountScreen'))

local function createSettingsScreen()
	local this = BaseScreen()

	this:SetTitle(string.upper(Strings:LocalizedString("SettingsWord")))


	local VersionBuildIdText = Utility.Create'TextLabel'
	{
		Name = "VersionBuildIdText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Right;
		TextYAlignment = Enum.TextYAlignment.Bottom;
		Text = '';
		Parent = this.Container;
	}
	do
		local versionInfo;
		if UserSettings().GameSettings:InStudioMode() then
			versionInfo = {Major = 1, Minor = 0, Build = 0, Revision = 0}
		elseif PlatformService then
			versionInfo = PlatformService:GetVersionIdInfo();
		else
			versionInfo = {Major = 1, Minor = 1, Build = 1, Revision = 1}
		end

		local versionStr = string.format(Strings:LocalizedString('VersionIdString'), tostring(versionInfo['Major']) , tostring(versionInfo['Minor']), tostring(versionInfo['Build']), tostring(versionInfo['Revision']))
		VersionBuildIdText.Text = versionStr
	end


	local spacing = 40
	local DefaultTransparency = GlobalSettings.TextBoxDefaultTransparency
	local SelectedTransparency = GlobalSettings.TextBoxSelectedTransparency

	local AccountButton = Utility.Create'TextButton'
	{
		Name = "AccountButton";
		Size = UDim2.new(0, 394, 0, 612);
		Position = UDim2.new(0, 0, 0, 238);
		BackgroundTransparency = DefaultTransparency;
		BackgroundColor3 = GlobalSettings.TextBoxColor;
		BorderSizePixel = 0;
		Text = "";
		Parent = this.Container;
		SoundManager:CreateSound('MoveSelection');
	}

	local SwitchProfileButton = AccountButton:Clone()
	SwitchProfileButton.Name = "SwitchProfileButton"
	SwitchProfileButton.Position = UDim2.new(0, AccountButton.Size.X.Offset + spacing, 0, 238)
	SwitchProfileButton.Parent = this.Container

	local OverscanButton = SwitchProfileButton:Clone()
	OverscanButton.Name = "OverscanButton"
	OverscanButton.Position = UDim2.new(0, SwitchProfileButton.Position.X.Offset + SwitchProfileButton.Size.X.Offset + spacing, 0, 238)
	OverscanButton.Parent = this.Container

	local HelpButton = OverscanButton:Clone()
	HelpButton.Name = "HelpButton"
	HelpButton.Position = UDim2.new(0, OverscanButton.Position.X.Offset + OverscanButton.Size.X.Offset + spacing, 0, 238)
	HelpButton.Parent = this.Container

	local AccountIcon = Utility.Create'ImageLabel'
	{
		Name = "AccountIcon";
		Size = UDim2.new(0, 256, 0, 256);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/AccountIcon.png';
		Parent = AccountButton;
	}
	Utility.CalculateAnchor(AccountIcon, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
	local AccountText = Utility.Create'TextLabel'
	{
		Name = "AccountText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0.5, 0, 1, -96);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = Strings:LocalizedString("AccountWord");
		Parent = AccountButton;
	}

	local SwitchProfileIcon = Utility.Create'ImageLabel'
	{
		Name = "SwitchProfileIcon";
		Size = UDim2.new(0, 224, 0, 255);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/ProfileIcon.png';
		Parent = SwitchProfileButton;
	}
	Utility.CalculateAnchor(SwitchProfileIcon, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
	SwitchProfileText = AccountText:Clone()
	SwitchProfileText.Name = "SwitchProfileText"
	SwitchProfileText.Text = Strings:LocalizedString("SwitchProfileWord");
	SwitchProfileText.Parent = SwitchProfileButton

	local OverscanIcon = Utility.Create'ImageLabel'
	{
		Name = "OverscanIcon";
		Size = UDim2.new(0, 256, 0, 181);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/TVIcon.png';
		Parent = OverscanButton;
	}
	Utility.CalculateAnchor(OverscanIcon, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
	OverscanText = AccountText:Clone()
	OverscanText.Name = "OverscanText";
	OverscanText.Text = Strings:LocalizedString("OverscanWord");
	OverscanText.Parent = OverscanButton

	local HelpIcon = Utility.Create'ImageLabel'
	{
		Name = "HelpIcon";
		Size = UDim2.new(0, 256, 0, 256);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/HelpIcon.png';
		Parent = HelpButton;
	}
	Utility.CalculateAnchor(HelpIcon, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
	HelpText = AccountText:Clone()
	HelpText.Name = "HelpText";
	HelpText.Text = Strings:LocalizedString("HelpWord");
	HelpText.Parent = HelpButton

	AccountButton.SelectionGained:connect(function()
		Utility.PropertyTweener(AccountButton, "BackgroundTransparency", SelectedTransparency,
			SelectedTransparency, 0, Utility.EaseInOutQuad, true)
	end)
	AccountButton.SelectionLost:connect(function()
		AccountButton.BackgroundTransparency = DefaultTransparency
	end)
	SwitchProfileButton.SelectionGained:connect(function()
		Utility.PropertyTweener(SwitchProfileButton, "BackgroundTransparency", SelectedTransparency,
			SelectedTransparency, 0, Utility.EaseInOutQuad, true)
	end)
	SwitchProfileButton.SelectionLost:connect(function()
		SwitchProfileButton.BackgroundTransparency = DefaultTransparency
	end)
	OverscanButton.SelectionGained:connect(function()
		Utility.PropertyTweener(OverscanButton, "BackgroundTransparency", SelectedTransparency,
			SelectedTransparency, 0, Utility.EaseInOutQuad, true)
	end)
	OverscanButton.SelectionLost:connect(function()
		OverscanButton.BackgroundTransparency = DefaultTransparency
	end)
	HelpButton.SelectionGained:connect(function()
		Utility.PropertyTweener(HelpButton, "BackgroundTransparency", SelectedTransparency,
			SelectedTransparency, 0, Utility.EaseInOutQuad, true)
	end)
	HelpButton.SelectionLost:connect(function()
		HelpButton.BackgroundTransparency = DefaultTransparency
	end)

	--[[ Input ]]--
	AccountButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		local accountScreen = AccountScreen()
		if accountScreen then
			accountScreen:SetParent(this.Container.Parent)
			ScreenManager:OpenScreen(accountScreen, true)
		else
			ScreenManager:OpenScreen(ErrorOverlay(Errors.Default), false)
		end
	end)

	local switchProfileDebounce = false
	SwitchProfileButton.MouseButton1Click:connect(function()
		if switchProfileDebounce then return end
		switchProfileDebounce = true
		SoundManager:Play('ButtonPress')
		if UserSettings().GameSettings:InStudioMode() then
			ScreenManager:OpenScreen(ErrorOverlay(Errors.Test.FeatureNotAvailableInStudio), false)
		elseif PlatformService then
			PlatformService:PopupAccountPickerUI(Enum.UserInputType.Gamepad1)
		end
		switchProfileDebounce = false
	end)

	local overscanDebounce = false
	OverscanButton.MouseButton1Click:connect(function()
		if overscanDebounce then return end
		overscanDebounce = true
		SoundManager:Play('ButtonPress')
		EventHub:dispatchEvent(EventHub.Notifications["OpenOverscanScreen"], "")
		overscanDebounce = false
	end)

	local helpDebounce = false
	HelpButton.MouseButton1Click:connect(function()
		if helpDebounce then return end
		helpDebounce = true
		SoundManager:Play('ButtonPress')
		if UserSettings().GameSettings:InStudioMode() then
			ScreenManager:OpenScreen(ErrorOverlay(Errors.Test.FeatureNotAvailableInStudio), false)
			helpDebounce = false
		else
			local success, result = pcall(function()
				-- errors will be handled by xbox
				return PlatformService:PopupHelpUI()
			end)
			helpDebounce = false
		end
	end)

	--[[ Public API ]]--
	--Override
	function this:GetDefaultSelectionObject()
		return AccountButton
	end

	return this
end

return createSettingsScreen
