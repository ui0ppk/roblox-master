--[[
			// UnlinkAccountScreen.lua
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local ContextActionService = game:GetService('ContextActionService')
local GuiService = game:GetService('GuiService')

local AccountManager = require(Modules:FindFirstChild('AccountManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BaseScreen = require(Modules:FindFirstChild('BaseScreen'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local TextBox = require(Modules:FindFirstChild('TextBox'))
local UnlinkAccountOverlay = require(Modules:FindFirstChild('UnlinkAccountOverlay'))
local Utility = require(Modules:FindFirstChild('Utility'))
local UserData = require(Modules:FindFirstChild('UserData'))

local function createUnlinkAccountScreen()
	local this = BaseScreen()

	this:SetTitle(string.upper(Strings:LocalizedString("AccountSettingsTitle")))
	local gamerTag = UserData:GetDisplayName() or ""
	local robloxName = UserData:GetRobloxName() or ""
	local linkedAsPhrase = string.format(Strings:LocalizedString('LinkedAsPhrase'), gamerTag, robloxName)
	local unlinkButtonText = string.format(Strings:LocalizedString("UnlinkGamerTagPhrase"), gamerTag)

	local dummySelection = Utility.Create'Frame'
	{
		BackgroundTransparency = 1;
	}

	local ModalOverlay = Utility.Create'Frame'
	{
		Name = "ModalOverlay";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = GlobalSettings.ModalBackgroundTransparency;
		BackgroundColor3 = GlobalSettings.ModalBackgroundColor;
		BorderSizePixel = 0;
		ZIndex = 4;
	}

	local LinkedAsText = Utility.Create'TextLabel'
	{
		Name = "LinkedAsText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0.5, 0, 0, 264);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = linkedAsPhrase;
		Parent = this.Container;
	}
	local GamerPic = Utility.Create'ImageLabel'
	{
		Name = "GamerPic";
		Size = UDim2.new(0, 300, 0, 300);
		BackgroundTransparency = 0;
		BorderSizePixel = 0;
		Image = 'rbxapp://xbox/localgamerpic';
		Parent = this.Container;
	}
	Utility.CalculateAnchor(GamerPic, UDim2.new(0.5, 0, 0, LinkedAsText.Position.Y.Offset + 52), Utility.Enum.Anchor.TopMiddle)

	local UnlinkButton = Utility.Create'ImageButton'
	{
		Name = "UnlinkButton";
		Size = UDim2.new(0, 360, 0, 64);
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.GreySelectedButtonColor;
		Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
		ZIndex = 2;
		Parent = this.Container;

		SoundManager:CreateSound('MoveSelection');
		AssetManager.CreateShadow(1)
	}
	Utility.CalculateAnchor(UnlinkButton, UDim2.new(0.5, 0, 0, GamerPic.Position.Y.Offset + GamerPic.Size.Y.Offset + 35), Utility.Enum.Anchor.TopMiddle)

	local UnlinkText = Utility.Create'TextLabel'
	{
		Name = "UnlinkText";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = GlobalSettings.TextSelectedColor;
		Text = string.upper(unlinkButtonText);
		ZIndex = 2;
		Parent = UnlinkButton;
	}

	local isUnlinking = false
	local function unlinkAccountAsync()
		if isUnlinking then return end
		isUnlinking = true
		local unlinkResult = nil
		local loader = LoadingWidget(
			{ Parent = this.Container }, {
			function()
				unlinkResult = AccountManager:UnlinkAccountAsync()
			end
		})

		-- set up full screen loader
		ModalOverlay.Parent = GuiRoot
		ContextActionService:BindCoreAction("BlockB", function() end, false, Enum.KeyCode.ButtonB)
		UnlinkButton.SelectionImageObject = dummySelection
		UnlinkButton.ImageColor3 = GlobalSettings.GreyButtonColor
		UnlinkText.TextColor3 = GlobalSettings.WhiteTextColor

		-- call loader
		loader:AwaitFinished()

		-- clean up
		-- NOTE: Unlink success will fire the UserAccountChanged event. This event will fire and listeners will
		-- run before the loader is finished. The below code needs to run in case of errors, but on success
		-- will not interfere with the reauth logic in AppHome.lua
		loader:Cleanup()
		loader = nil
		UnlinkButton.SelectionImageObject = nil
		UnlinkButton.ImageColor3 = GlobalSettings.GreySelectedButtonColor
		UnlinkText.TextColor3 = GlobalSettings.TextSelectedColor
		ContextActionService:UnbindCoreAction("BlockB")
		ModalOverlay.Parent = nil

		if unlinkResult ~= AccountManager.AuthResults.Success then
			local err = unlinkResult and Errors.Authentication[unlinkResult] or Errors.Default
			ScreenManager:OpenScreen(ErrorOverlay(err), false)
		end
		isUnlinking = false
	end

	UnlinkButton.MouseButton1Click:connect(function()
		if isUnlinking then return end
		SoundManager:Play('ButtonPress')
		local confirmTitleAndMsg = { Title = Strings:LocalizedString("UnlinkTitle"), Msg = Strings:LocalizedString("UnlinkPhrase") }
		ScreenManager:OpenScreen(UnlinkAccountOverlay(confirmTitleAndMsg), false)
	end)

	--[[ Public API ]]--
	-- Override
	function this:GetDefaultSelectionObject()
		return UnlinkButton
	end

	-- Override
	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(self)
		EventHub:addEventListener(EventHub.Notifications["UnlinkAccountConfirmation"], "unlinkAccount",
			function()
				unlinkAccountAsync()
			end)
	end

	-- Override
	local baseRemoveFocus = this.RemoveFocus
	function this:RemoveFocus()
		baseRemoveFocus(self)
		EventHub:removeEventListener(EventHub.Notifications["UnlinkAccountConfirmation"], "unlinkAccount")
	end

	return this
end

return createUnlinkAccountScreen
