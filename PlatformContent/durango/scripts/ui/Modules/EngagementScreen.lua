-- Written by Kip Turner, Copyright ROBLOX 2015

local CoreGui = Game:GetService("CoreGui")
local GuiService = game:GetService('GuiService')
local PlayersService = game:GetService("Players")
local ContextActionService = game:GetService("ContextActionService")
local UserInputService = game:GetService("UserInputService")

local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local AccountManager = require(Modules:FindFirstChild('AccountManager'))
local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SetAccountCredentialsScreen = require(Modules:FindFirstChild('SetAccountCredentialsScreen'))
local SignInScreen = require(Modules:FindFirstChild('SignInScreen'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))

local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local EventHub = require(Modules:FindFirstChild('EventHub'))

local ANY_KEY_CODES =
{
	[Enum.KeyCode.ButtonA] = true;
	-- [Enum.KeyCode.ButtonB] = true;
	[Enum.KeyCode.ButtonX] = true;
	[Enum.KeyCode.ButtonY] = true;
	[Enum.KeyCode.ButtonStart] = true;
	[Enum.KeyCode.ButtonSelect] = true;
	[Enum.KeyCode.ButtonL1] = true;
	[Enum.KeyCode.ButtonR1] = true;
}

local GAMEPAD_INPUT_TYPES =
{
	[Enum.UserInputType.Gamepad1] = true;
	[Enum.UserInputType.Gamepad2] = true;
	[Enum.UserInputType.Gamepad3] = true;
	[Enum.UserInputType.Gamepad4] = true;
}

local function CreateHomePane(parent)
	local this = {}

	local AnyButtonBeganConnection, AnyButtonEndedConnection = nil

	local EngagementScreenContainer = Utility.Create'Frame'
	{
		Name = 'EngagementScreen';
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Parent = parent;
	}

		local RobloxLogo = Utility.Create'ImageLabel'
		{
			Name = 'RobloxLogo';
			BackgroundTransparency = 1;
			Size = UDim2.new(0, 594, 0, 199);
			Image = 'rbxasset://textures/ui/Shell/Icons/ROBLOXSplashLogo.png';
			Parent = EngagementScreenContainer;
		}
		Utility.CalculateAnchor(RobloxLogo, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)

		local AnyButtonHint = Utility.Create'TextLabel'
		{
			Name = 'AnyButtonHint';
			Text = Strings:LocalizedString('EngagementScreenHint');
			TextColor3 = GlobalSettings.WhiteTextColor;
			Font = GlobalSettings.RegularFont;
			FontSize = GlobalSettings.ButtonSize;
			Size = UDim2.new(0,0,0,0);
			BackgroundTransparency = 1;
			Parent = RobloxLogo;
		};
		Utility.CalculateAnchor(AnyButtonHint, UDim2.new(0.5,0,0,415), Utility.Enum.Anchor.Center)

	local function beginAuthenticationAsync(gamePad)
		local authResult = nil
		local hasRobloxCredentialsResult = nil
		local function auth()
			authResult = AccountManager:BeginAuthenticationAsync(gamePad)
			if not authResult then
				print("beginAuthenticationAsync() failed because", result)
				authResult = AccountManager.AuthResults.Error
			end

			if authResult == AccountManager.AuthResults.Success then
				hasRobloxCredentialsResult = AccountManager:HasRobloxCredentialsAsync()
			end
		end

		local loader = LoadingWidget(
			{ Parent = RobloxLogo, Position = UDim2.new(0.5, 0, 0, 415) }, { auth })
		loader:AwaitFinished()
		loader:Cleanup()
		loader = nil

		print("beginAuthenticationAsync has completed with code:", authResult)
		if authResult == AccountManager.AuthResults.Success then
			if hasRobloxCredentialsResult == AccountManager.AuthResults.Success then
				EventHub:dispatchEvent(EventHub.Notifications["AuthenticationSuccess"])
			elseif hasRobloxCredentialsResult == AccountManager.AuthResults.UsernamePasswordNotSet then
				local setAccountCredentialsScreen = SetAccountCredentialsScreen(Strings:LocalizedString("SetCredentialsTitle"),
					Strings:LocalizedString("SetCredentialsPhrase"), Strings:LocalizedString("SetCredentialsWord"))
				setAccountCredentialsScreen:SetParent(EngagementScreenContainer.Parent)
				ScreenManager:OpenScreen(setAccountCredentialsScreen, true)
			else
				local err = Errors.Authentication[hasRobloxCredentialsResult] or Errors.Default
				ScreenManager:OpenScreen(ErrorOverlay(err), false)
			end
		elseif authResult == AccountManager.AuthResults.AccountUnlinked then
			local signInScreen = SignInScreen()
			signInScreen:SetParent(EngagementScreenContainer.Parent)
			ScreenManager:OpenScreen(signInScreen, true)
		else
			local err = Errors.Authentication[authResult] or Errors.Default
			ScreenManager:OpenScreen(ErrorOverlay(err), false)
		end
	end

	local function onAnyButtonPressed(gamePad)
		AnyButtonBeganConnection = Utility.DisconnectEvent(AnyButtonBeganConnection)
		AnyButtonEndedConnection = Utility.DisconnectEvent(AnyButtonEndedConnection)
		AnyButtonHint.TextColor3 = GlobalSettings.WhiteTextColor
		Utility.PropertyTweener(AnyButtonHint, 'TextTransparency', 0, 1, 0.25, Utility.EaseOutQuad, true,
			function()
				beginAuthenticationAsync(gamePad)
			end)
	end

	function this:Show()
		EngagementScreenContainer.Visible = true
	end

	function this:Hide()
		EngagementScreenContainer.Visible = false
	end

	function this:Focus()
		AnyButtonHint.TextColor3 = GlobalSettings.WhiteTextColor
		AnyButtonHint.TextTransparency = 0

		Utility.DisconnectEvent(AnyButtonBeganConnection)
		local anyButtonDown = {}
		AnyButtonBeganConnection = UserInputService.InputBegan:connect(function(inputObject)
			if GAMEPAD_INPUT_TYPES[inputObject.UserInputType] then
				if ANY_KEY_CODES[inputObject.KeyCode] then
					AnyButtonHint.TextColor3 = GlobalSettings.GreyTextColor
					anyButtonDown[inputObject.KeyCode] = true
				end
			end
		end)
		Utility.DisconnectEvent(AnyButtonEndedConnection)
		local isAuthenticating = false
		AnyButtonEndedConnection = UserInputService.InputEnded:connect(function(inputObject)
			if isAuthenticating then return end
			isAuthenticating = true
			if GAMEPAD_INPUT_TYPES[inputObject.UserInputType] then
				if ANY_KEY_CODES[inputObject.KeyCode] and anyButtonDown[inputObject.KeyCode] == true then
					SoundManager:Play('ButtonPress')
					onAnyButtonPressed(inputObject.UserInputType)
				end
			end
			isAuthenticating = false
			anyButtonDown[inputObject.KeyCode] = false
		end)
	end

	function this:RemoveFocus()
		AnyButtonBeganConnection = Utility.DisconnectEvent(AnyButtonBeganConnection)
		AnyButtonEndedConnection = Utility.DisconnectEvent(AnyButtonEndedConnection)
	end

	function this:SetParent(newParent)
		EngagementScreenContainer.Parent = newParent
	end

	return this
end

return CreateHomePane
