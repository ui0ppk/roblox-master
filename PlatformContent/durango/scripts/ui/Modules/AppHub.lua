-- Written by Kip Turner, Copyright ROBLOX 2015

local CoreGui = game:GetService("CoreGui")
local ContextActionService = game:GetService("ContextActionService")
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)
local UserInputService = game:GetService("UserInputService")
local GuiService = game:GetService('GuiService')
local TextService = game:GetService('TextService')

local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local AppTabDockModule = require(Modules:FindFirstChild('TabDock'))
local AppTabDockItemModule = require(Modules:FindFirstChild('TabDockItem'))
local HomePaneModule = require(Modules:FindFirstChild('HomePane'))
local GamePaneModule = require(Modules:FindFirstChild('GamePane'))
local AvatarPaneModule = require(Modules:FindFirstChild('AvatarPane'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SocialPaneModule = require(Modules:FindFirstChild('SocialPane'))
local StorePaneModule = require(Modules:FindFirstChild('StorePane'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local SettingsScreen = require(Modules:FindFirstChild('SettingsScreen'))

local function CreateAppHub()
	local this = {}
	-- Game.CoreGui.SelectionImageObject = Instance.new('ImageLabel')

	local AppTabDock = AppTabDockModule()
	local appHubCns = {}
	local selectionChangedConn = nil

	local lastSelectedContentPane = nil
	local lastParent = nil

	local HubContainer = Utility.Create'Frame'
	{
		Name = 'HubContainer';
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Visible = false;
	}

	local PaneContainer = Utility.Create'Frame'
	{
		Name = 'PaneContainer';
		Size = UDim2.new(1, 0, 0.786, 0);
		Position = UDim2.new(0,0,0.214,0);
		BackgroundTransparency = 1;
		Parent = HubContainer;
	}

	AppTabDock:SetParent(HubContainer)
	AppTabDock:SetPosition(UDim2.new(0,0,0.132,0))
	local HomeTab = AppTabDock:AddTab(AppTabDockItemModule(Strings:LocalizedString('HomeWord'):upper(), HomePaneModule(PaneContainer)))
	local AvatarTab = AppTabDock:AddTab(AppTabDockItemModule(Strings:LocalizedString('AvatarWord'):upper(), AvatarPaneModule(PaneContainer)))
	local GameTab = AppTabDock:AddTab(AppTabDockItemModule(Strings:LocalizedString('GameWord'):upper(), GamePaneModule(PaneContainer)))
	local SocialTab = AppTabDock:AddTab(AppTabDockItemModule(Strings:LocalizedString('FriendsWord'):upper(), SocialPaneModule(PaneContainer)))
	local StoreTab = AppTabDock:AddTab(AppTabDockItemModule(Strings:LocalizedString('CatalogWord'):upper(), StorePaneModule(PaneContainer)))


	local RobloxLogo = Utility.Create'ImageLabel'
	{
		Name = 'RobloxLogo';
		Size = UDim2.new(0, 232, 0, 56);
		Position = UDim2.new(0,0,0,0);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/ROBLOXLogoSmall@1080.png';
		Parent = HubContainer;
	}

	-- Positin/Size set after text bounds set
	local BoundHintContainer = Utility.Create'Frame'
	{
		Name = "BoundHintContainer";
		BackgroundTransparency = 1;
		Visible = false;
		Parent = HubContainer;
	}
	local BoundHintText = Utility.Create'TextLabel'
	{
		Name = "BoundHintText";
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = GlobalSettings.Right;
		Text = "";
		Parent = BoundHintContainer;
	}
	local BoundHintImage = Utility.Create'ImageLabel'
	{
		Name = "BoundHintImage";
		Size = UDim2.new(0, 83, 0, 83);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/ButtonIcons/XButton.png';
		Parent = BoundHintContainer;
	}

	local function setHintText(newText)
		local textSize = TextService:GetTextSize(newText, 42, BoundHintText.Font, Vector2.new(0, 0))
		BoundHintText.Size = UDim2.new(0, textSize.x, 0, 83)
		BoundHintText.Position = UDim2.new(1, -textSize.x, 0, 0)
		BoundHintText.Text = newText
		BoundHintContainer.Size = UDim2.new(0, textSize.x + BoundHintImage.Size.X.Offset, 0, 83)
		BoundHintContainer.Position = UDim2.new(1, -BoundHintContainer.Size.X.Offset, 1, -BoundHintContainer.Size.Y.Offset)
		BoundHintContainer.Visible = true
	end

	local seenXButtonPressed = false
	local function onOpenSettings(actionName, inputState, inputObject)
		if inputState == Enum.UserInputState.Begin then
			seenXButtonPressed = true
		elseif inputState == Enum.UserInputState.End and seenXButtonPressed then
			local settingsScreen = SettingsScreen()
			EventHub:dispatchEvent(EventHub.Notifications["OpenSettingsScreen"], settingsScreen);
		end
	end

	local function onOpenPartyUI(actionName, inputState, inputObject)
		if inputState == Enum.UserInputState.Begin then
			seenXButtonPressed = true
		elseif inputState == Enum.UserInputState.End and seenXButtonPressed then
			if UserSettings().GameSettings:InStudioMode() then
				ScreenManager:OpenScreen(ErrorOverlay(Errors.Test.FeatureNotAvailableInStudio), false)
			else
				local success, result = pcall(function()
					-- PlatformService may not exist in studio
					return PlatformService:PopupPartyUI(inputObject.UserInputType)
				end)
				if not success then
					ScreenManager:OpenScreen(ErrorOverlay(Errors.PlatformError.PopupPartyUI), false)
				end
			end
		end
	end

	local function setHintAction(selectedTab)
		ContextActionService:UnbindCoreAction("OpenHintAction")
		BoundHintContainer.Visible = false
		if selectedTab == HomeTab then
			setHintText(Strings:LocalizedString("SettingsWord"))
			ContextActionService:BindCoreAction("OpenHintAction", onOpenSettings, false, Enum.KeyCode.ButtonX)
		elseif selectedTab == SocialTab then
			setHintText(Strings:LocalizedString("StartPartyPhrase"))
			ContextActionService:BindCoreAction("OpenHintAction", onOpenPartyUI, false, Enum.KeyCode.ButtonX)
		end
	end

	function this:GetName()
		-- print("Apphub get name called - lastSelectedContentPane:" , lastSelectedContentPane, "name:" , lastSelectedContentPane and lastSelectedContentPane:GetName())
		return lastSelectedContentPane and lastSelectedContentPane:GetName() or Strings:LocalizedString('HomeWord')
	end


	function this:Show()
		HubContainer.Visible = true
		HubContainer.Parent = lastParent

		EventHub:removeEventListener(EventHub.Notifications["NavigateToRobuxScreen"], 'AppHubListenToRobuxScreenSwitch')
		EventHub:addEventListener(EventHub.Notifications["NavigateToRobuxScreen"], 'AppHubListenToRobuxScreenSwitch', function()
			if ScreenManager:ContainsScreen(this) then
				while ScreenManager:GetTopScreen() ~= this and ScreenManager:ContainsScreen(this) do
					ScreenManager:CloseCurrent()
				end
				if ScreenManager:GetTopScreen() == this then
					if AppTabDock:GetSelectedTab() ~= StoreTab then
						AppTabDock:SetSelectedTab(StoreTab)
					end
				end
			end
		end)

		local openEquippedDebounce = false
		EventHub:removeEventListener(EventHub.Notifications["NavigateToEquippedAvatar"], 'AppHubListenToAvatarScreenSwitch')
		EventHub:addEventListener(EventHub.Notifications["NavigateToEquippedAvatar"], 'AppHubListenToAvatarScreenSwitch', function()
			if openEquippedDebounce then return end
			openEquippedDebounce = true
			if ScreenManager:ContainsScreen(this) then
				while ScreenManager:GetTopScreen() ~= this and ScreenManager:ContainsScreen(this) do
					ScreenManager:CloseCurrent()
				end
				if ScreenManager:GetTopScreen() == this then
					if AppTabDock:GetSelectedTab() ~= AvatarTab then
						AppTabDock:SetSelectedTab(AvatarTab)
						-- local avatarPane = AvatarTab:GetContentItem()
						-- if avatarPane then
						-- 	avatarPane:OpenEquippedPackage()
						-- end
					end
				end
			end
			openEquippedDebounce = false
		end)

		local currentlySelectedTab = AppTabDock:GetSelectedTab()
		AppTabDock:SetSelectedTab(currentlySelectedTab)
		if lastSelectedContentPane then
			lastSelectedContentPane:Show()
		end
	end

	function this:Hide()
		if not ScreenManager:ContainsScreen(self) then
			EventHub:removeEventListener(EventHub.Notifications["NavigateToRobuxScreen"], 'AppHubListenToRobuxScreenSwitch')
			EventHub:removeEventListener(EventHub.Notifications["NavigateToEquippedAvatar"], 'AppHubListenToAvatarScreenSwitch')
		end
		HubContainer.Visible = false
		HubContainer.Parent = nil
		if lastSelectedContentPane then
			lastSelectedContentPane:Hide()
		end
	end

	function this:Focus()
		AppTabDock:ConnectEvents()
		ContextActionService:BindCoreAction("CycleTabDock",
			function(actionName, inputState, inputObject)
				if inputState == Enum.UserInputState.End then
					if inputObject.KeyCode == Enum.KeyCode.ButtonL1 then
						local prevTab = AppTabDock:GetPreviousTab()
						if prevTab then
							AppTabDock:SetSelectedTab(prevTab)
						end
					elseif inputObject.KeyCode == Enum.KeyCode.ButtonR1 then
						local nextTab = AppTabDock:GetNextTab()
						if nextTab then
							AppTabDock:SetSelectedTab(nextTab)
						end
					end
				end
			end,
			false,
			Enum.KeyCode.ButtonL1, Enum.KeyCode.ButtonR1)

		local seenBButtonBegin = false
		ContextActionService:BindCoreAction("CloseAppHub",
			function(actionName, inputState, inputObject)
				if inputState == Enum.UserInputState.Begin then
					seenBButtonBegin = true
				elseif inputState == Enum.UserInputState.End then
					if seenBButtonBegin then
						local currentlySelectedTab = AppTabDock:GetSelectedTab()
						if currentlySelectedTab ~= HomeTab then
							AppTabDock:SetSelectedTab(HomeTab)
						end
					end
				end
			end,
			false,
			Enum.KeyCode.ButtonB)

		local function onSelectedTabChanged(selectedTab)
			if selectedTab then
				if lastSelectedContentPane then
					lastSelectedContentPane:Hide()
					lastSelectedContentPane:RemoveFocus()
				end
				local selectedContentPane = selectedTab:GetContentItem()
				if selectedContentPane then
					selectedContentPane:Show()
					if not AppTabDock:IsFocused() then
						AppTabDock:Focus()
						selectedContentPane:Focus()
					end
				end
				lastSelectedContentPane = selectedContentPane

				-- set X action
				setHintAction(selectedTab)
			end
		end
		table.insert(appHubCns, AppTabDock.SelectedTabChanged:connect(onSelectedTabChanged))

		local function onSelectedTabClicked(selectedTab)
			local selectedContentPane = selectedTab and selectedTab:GetContentItem()
			if selectedContentPane and selectedContentPane == lastSelectedContentPane then
				selectedContentPane:Focus()
			end
		end
		table.insert(appHubCns, AppTabDock.SelectedTabClicked:connect(onSelectedTabClicked))

		selectionChangedConn = Utility.DisconnectEvent(selectionChangedConn)
		selectionChangedConn = GuiService.Changed:connect(function(prop)
			if prop == "SelectedCoreObject" then
				local currentSelection = GuiService.SelectedCoreObject
				if currentSelection and lastSelectedContentPane then
					-- first condition checks if function exist
					if lastSelectedContentPane.IsFocused and not lastSelectedContentPane:IsFocused() and lastSelectedContentPane.IsAncestorOf then
						if lastSelectedContentPane:IsAncestorOf(currentSelection) then
							-- print("Doing our focus")
							lastSelectedContentPane:Focus()
						else
							-- lastSelectedContentPane:RemoveFocus()
						end
					end
				end
			end
		end)

		--set the Home tab to be the starting tab
		if AppTabDock:GetSelectedTab() == nil then
			AppTabDock:SetSelectedTab(HomeTab)
		end

		if lastSelectedContentPane then
			lastSelectedContentPane:Focus()
		end

		setHintAction(AppTabDock:GetSelectedTab())
	end

	function this:RemoveFocus()
		AppTabDock:DisconnectEvents()
		ContextActionService:UnbindCoreAction("CycleTabDock")
		ContextActionService:UnbindCoreAction("CloseAppHub")

		if lastSelectedContentPane then
			lastSelectedContentPane:RemoveFocus()
		end
		for k,v in pairs(appHubCns) do
			v:disconnect()
			v = nil
			appHubCns[k] = nil
		end

		selectionChangedConn = Utility.DisconnectEvent(selectionChangedConn)

		ContextActionService:UnbindCoreAction("OpenHintAction")
	end

	function this:SetParent(newParent)
		HubContainer.Parent = newParent
		lastParent = newParent
	end

	local hubID = "AppHub"
	--EventHub:addEventListener(EventHub.Notifications["OpenGameDetail"], hubID, function(data) AppTabDock:SetSelectedTab(GameTab); end);
	--EventHub:addEventListener(EventHub.Notifications["OpenGameGenre"],  hubID, function(data) AppTabDock:SetSelectedTab(GameTab); end);


	return this
end

return CreateAppHub
