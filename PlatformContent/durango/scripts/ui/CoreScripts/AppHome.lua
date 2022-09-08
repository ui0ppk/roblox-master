-- Written by Kip Turner, Copyright ROBLOX 2015

-- App's Main

local CoreGui = Game:GetService("CoreGui")
local ContentProvider = Game:GetService("ContentProvider")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
-- TODO: Will use for re-auth when finished
local UserInputService = game:GetService('UserInputService')
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local AppHubModule = require(Modules:FindFirstChild('AppHub'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local GameGenreModule = require(Modules:FindFirstChild('GameGenre'))
local GameDetailModule = require(Modules:FindFirstChild('GameDetail'))
local OverscanScreenModule = require(Modules:FindFirstChild('OverscanScreen'))
local EngagementScreenModule = require(Modules:FindFirstChild('EngagementScreen'))
local BadgeScreenModule = require(Modules:FindFirstChild('BadgeScreen'))
local CameraManager = require(Modules:FindFirstChild('CameraManager'))
local FriendsData = require(Modules:FindFirstChild('FriendsData'))
local UserData = require(Modules:FindFirstChild('UserData'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local AchievementManager = require(Modules:FindFirstChild('AchievementManager'))
local HeroStatsManager = require(Modules:FindFirstChild('HeroStatsManager'))
local ControllerStateManager = require(Modules:FindFirstChild('ControllerStateManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))

local TIME_BETWEEN_BACKGROUND_TRANSITIONS = 30
local CROSSFADE_DURATION = 1.5

local BACKGROUND_ASSETS =
{
	'Home_screen_01.png';
	'Home_screen_02.png';
	'Home_screen_03.png';
	'Home_screen_04.png';
}

local AppHomeContainer = Utility.Create'Frame'
{
	Size = UDim2.new(1, 0, 1, 0);
	BackgroundTransparency = 0;
	BorderSizePixel = 0;
	BackgroundColor3 = Color3.new(0,0,0);
	Name = 'AppHomeContainer';
	Parent = GuiRoot;
}


local BackgroundAssetIndex = 1
local Background = Utility.Create'ImageLabel'
{
	Size = UDim2.new(1, 0, 1, 0);
	BackgroundTransparency = 0;
	BorderSizePixel = 0;
	Name = 'Background';
	Image = 'rbxasset://textures/ui/Shell/Background/' .. BACKGROUND_ASSETS[BackgroundAssetIndex];
	Parent = AppHomeContainer;
}
local CrossfadeBackground = Utility.Create'ImageLabel'
{
	Size = UDim2.new(1, 0, 1, 0);
	BackgroundTransparency = 1;
	BorderSizePixel = 0;
	Name = 'CrossfadeBackground';
	Image = '';
	Parent = Background;
}

Background.ImageTransparency = 0

-- get fast flags for 3D backgrounds
local Is3DBackgroundEnabled = Utility.IsFastFlagEnabled("Durango3DBackground")


-- print ("BACKGROUND")
-- print (Background3DSuccess)
-- print (Background3DValue)

local soundHandle = SoundManager:Play('BackgroundLoop', 0.33, true)
if soundHandle then
	local bgmLoopConn = nil
	bgmLoopConn = soundHandle.DidLoop:connect(function(soundId, loopCount)
		-- print("DidLoop BackgroundLoop" , soundId, "times:", loopCount)
		if loopCount >= 1 then
			bgmLoopConn = Utility.DisconnectEvent(bgmLoopConn)
			if soundHandle then
				-- print("Volume fade out")
				SoundManager:TweenSound(soundHandle, 0.1, 3)
				-- Utility.PropertyTweener(soundHandle, 'Volume', soundHandle.Volume, 0.1, 3, Utility.EaseInOutQuad, true)
			end
		end
	end)
end

if Is3DBackgroundEnabled then
	spawn(function()

		while true do
			local queueSize = ContentProvider.RequestQueueSize

			if queueSize == 0 then
				CameraManager:StartTransitionScreenEffect()
				CameraManager:EnableCameraControl()

				spawn(function()
					CameraManager:CameraMoveToAsync()
				end)

				AppHomeContainer.BackgroundTransparency = 1
				Background.BackgroundTransparency = 1;
				Background.ImageTransparency = 1
				CrossfadeBackground.ImageTransparency = 1

				Utility.PropertyTweener(Background, 'ImageTransparency', 0, 1, CROSSFADE_DURATION,  Utility.EaseInOutQuad, true)
				break
			end

			wait(0.01)
		end

	end)
else
	spawn(function()

		CameraManager:StartTransitionScreenEffect()

		while true do
			wait(TIME_BETWEEN_BACKGROUND_TRANSITIONS)

			-- 1-based modulo indexing
			BackgroundAssetIndex = ((BackgroundAssetIndex) % #BACKGROUND_ASSETS) + 1

			CrossfadeBackground.Image = Background.Image
			CrossfadeBackground.ImageTransparency = 0
			Background.Image = 'rbxasset://textures/ui/Shell/Background/' .. BACKGROUND_ASSETS[BackgroundAssetIndex];
			Background.ImageTransparency = 1

			Utility.PropertyTweener(Background, 'ImageTransparency', 1, 0, CROSSFADE_DURATION,  Utility.EaseInOutQuad, true)
			wait(CROSSFADE_DURATION)
			Utility.PropertyTweener(CrossfadeBackground, 'ImageTransparency', 0, 1, CROSSFADE_DURATION,  Utility.EaseInOutQuad, true)
		end
	end)
end

local AspectRatioProtector = Utility.Create'Frame'
{
	Size = UDim2.new(1, 0, 1, 0);
	Position = UDim2.new(0,0,0,0);
	BackgroundTransparency = 1;
	Name = 'AspectRatioProtector';
	Parent = CrossfadeBackground;
}

local function OnAbsoluteSizeChanged()
	local newSize = Utility.CalculateFit(GuiRoot, Vector2.new(16,9))
	if newSize ~= AspectRatioProtector.Size then
		AspectRatioProtector.Size = newSize
		Utility.CalculateAnchor(AspectRatioProtector, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)
	end
end

GuiRoot.Changed:connect(function(prop)
	if prop == 'AbsoluteSize' then
		OnAbsoluteSizeChanged()
	end
end)
OnAbsoluteSizeChanged()

local ActionSafeContainer = Utility.Create'Frame'
{
	Size = UDim2.new(1, 0, 1, 0) - (GlobalSettings.ActionSafeInset + GlobalSettings.ActionSafeInset);
	Position = UDim2.new(0,0,0,0) + GlobalSettings.ActionSafeInset;
	BackgroundTransparency = 1;
	Name = 'ActionSafeContainer';
	Parent = AspectRatioProtector;
}

local titleActionSafeDelta = GlobalSettings.TitleSafeInset -- - GlobalSettings.ActionSafeInset

local TitleSafeContainer = Utility.Create'Frame'
{
	Size = UDim2.new(1, 0, 1, 0) - (titleActionSafeDelta + titleActionSafeDelta);
	Position = UDim2.new(0,0,0,0) + titleActionSafeDelta;
	BackgroundTransparency = 1;
	Name = 'TitleSafeContainer';
	Parent = ActionSafeContainer;
}

local EngagementScreen = EngagementScreenModule()
EngagementScreen:SetParent(TitleSafeContainer)

local userInputConn = nil

local function returnToEngagementScreen()
	if ScreenManager:ContainsScreen(EngagementScreen) then
		while ScreenManager:GetTopScreen() ~= EngagementScreen do
			ScreenManager:CloseCurrent()
		end
	else
		while ScreenManager:GetTopScreen() do
			ScreenManager:CloseCurrent()
		end
		ScreenManager:OpenScreen(EngagementScreen)
	end
end

local AppHub = nil
local OverscanScreen = nil
local function onAuthenticationSuccess(isNewLinkedAccount)
	-- Set UserData
	UserData:Initialize()

	-- Unwind Screens if needed - this will be needed once we put in account linking
	returnToEngagementScreen()

	AppHub = AppHubModule()
	AppHub:SetParent(TitleSafeContainer)

	EventHub:addEventListener(EventHub.Notifications["OpenGameDetail"], "gameDetail",
		function(placeId, placeName, iconId, gameData)
			local gameDetail = GameDetailModule(placeId, placeName, iconId, gameData)
			gameDetail:SetParent(TitleSafeContainer);
			ScreenManager:OpenScreen(gameDetail);
		end);
	EventHub:addEventListener(EventHub.Notifications["OpenGameGenre"], "gameGenre",
		function(sortName, gameCollection)
			local gameGenre = GameGenreModule(sortName, gameCollection)
			gameGenre:SetParent(TitleSafeContainer);
			ScreenManager:OpenScreen(gameGenre);
		end);
	EventHub:addEventListener(EventHub.Notifications["OpenBadgeScreen"], "gameBadges",
		function(badgeData, previousScreenName)
			local badgeScreen = BadgeScreenModule(badgeData, previousScreenName)
			badgeScreen:SetParent(TitleSafeContainer);
			ScreenManager:OpenScreen(badgeScreen);
		end)
	EventHub:addEventListener(EventHub.Notifications["OpenSocialScreen"], "socialScreen",
		function(socialScreen)
			socialScreen:SetParent(TitleSafeContainer);
			ScreenManager:OpenScreen(socialScreen);
		end)
	EventHub:addEventListener(EventHub.Notifications["OpenSettingsScreen"], "settingsScreen",
		function(settingsScreen)
			settingsScreen:SetParent(TitleSafeContainer);
			ScreenManager:OpenScreen(settingsScreen);
		end)
	OverscanScreen = OverscanScreenModule(AppHomeContainer)
	EventHub:addEventListener(EventHub.Notifications["OpenOverscanScreen"], "openOverscan", function(data) ScreenManager:OpenScreen(OverscanScreen) end)

	Utility.DisconnectEvent(userInputConn)
	userInputConn = UserInputService.InputBegan:connect(function(input, processed)
		if input.KeyCode == Enum.KeyCode.ButtonX then
			-- EventHub:dispatchEvent(EventHub.Notifications["TestXButtonPressed"])
		end
	end)

	-- disable control control
	CameraManager:DisableCameraControl()

	print("User and Event initialization finished. Opening AppHub")
	ScreenManager:OpenScreen(AppHub);

	ControllerStateManager:Initialize()
	-- retro check
	ControllerStateManager:CheckUserConnected()

	-- show info popup to users on newly linked accounts
	if isNewLinkedAccount == true then
		local titleAndMsg = {
			Title = Strings:LocalizedString('PlatformLinkInfoTitle');
			Msg = Strings:LocalizedString('PlatformLinkInfoMessage');
		}
		ScreenManager:OpenScreen(ErrorOverlay(titleAndMsg, true), false)
	end
end

local function onReAuthentication(reauthenticationReason)
	print("Beging Reauth, cleaning things up")
	-- unwind ScreenManager
	returnToEngagementScreen()

	UserData:Reset()
	FriendsData.Reset()
	AppHub = nil
	OverscanScreen = nil
	EventHub:removeEventListener(EventHub.Notifications["OpenGameDetail"], "gameDetail")
	EventHub:removeEventListener(EventHub.Notifications["OpenGameGenre"], "gameGenre")
	EventHub:removeEventListener(EventHub.Notifications["OpenBadgeScreen"], "gameBadges")
	EventHub:removeEventListener(EventHub.Notifications["OpenSocialScreen"], "socialScreen")
	EventHub:removeEventListener(EventHub.Notifications["OpenOverscanScreen"], "openOverscan")
	EventHub:removeEventListener(EventHub.Notifications["OpenSocialScreen"], "socialScreen")
	EventHub:removeEventListener(EventHub.Notifications["OpenSettingsScreen"], "settingsScreen")
	userInputConn = Utility.DisconnectEvent(userInputConn)
	print("Reauth complete. Return to engagement screen.")

	-- show reason overlay
	local alert = Errors.Reauthentication[reauthenticationReason] or Errors.Default
	ScreenManager:OpenScreen(ErrorOverlay(alert, true), false)

	-- re-enable camera control
	CameraManager:EnableCameraControl()
end

local function onGameJoin(joinResult)
	-- 0 is success, anything else is an error
	local joinSuccess = joinResult == 0
	if not joinSuccess then
		local err = Errors.GameJoin[joinResult] or Errors.Default
		ScreenManager:OpenScreen(ErrorOverlay(err), false)
	end
	EventHub:dispatchEvent(EventHub.Notifications["GameJoin"], joinSuccess)
end

if PlatformService then
	PlatformService.UserAccountChanged:connect(onReAuthentication)
	PlatformService.GameJoined:connect(onGameJoin)
end

EventHub:addEventListener(EventHub.Notifications["AuthenticationSuccess"], "authUserSuccess", function(isNewLinkedAccount)
		print("User authenticated, initializing app shell")
		onAuthenticationSuccess(isNewLinkedAccount)
	end);
-- EventHub:addEventListener(EventHub.Notifications["OpenEngagementScreen"], "",
-- 	function()
-- 		ScreenManager:OpenScreen(EngagementScreen)
-- 	end);
ScreenManager:OpenScreen(EngagementScreen)

UserInputService.MouseIconEnabled = false

