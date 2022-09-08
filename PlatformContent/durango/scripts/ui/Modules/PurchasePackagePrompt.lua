--[[
			// PurchasePackagePrompt.lua
			// Kip Turner
			// Copyright Roblox 2015
]]

local CoreGui = game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local ContextActionService = game:GetService("ContextActionService")
local GuiService = game:GetService('GuiService')
local MarketplaceService = game:GetService('MarketplaceService')
local PlatformService;
pcall(function() PlatformService = game:GetService('PlatformService') end)

local UserDataModule = require(Modules:FindFirstChild('UserData'))
local Http = require(Modules:FindFirstChild('Http'))
local ScrollingTextBox = require(Modules:FindFirstChild('ScrollingTextBox'))
local CreateConfirmPrompt = require(Modules:FindFirstChild('ConfirmPrompt'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))

local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local Errors = require(Modules:FindFirstChild('Errors'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local CurrencyWidgetModule = require(Modules:FindFirstChild('CurrencyWidget'))

local MOCKUP_WIDTH = 1920
local MOCKUP_HEIGHT = 1080
local CONTENT_WIDTH = 1920
local CONTENT_HEIGHT = 690
local PACKAGE_CONTAINER_WIDTH = 780
local PACKAGE_CONTAINER_HEIGHT = 690
local PACKAGE_BACKGROUND_WIDTH = 580
local PACKAGE_BACKGROUND_HEIGHT = 640

local CONTENT_POSITION = Vector2.new(0, 225)

local DETAILS_CONTAINER_WIDTH = CONTENT_WIDTH - PACKAGE_CONTAINER_WIDTH
local DETAILS_CONTAINER_HEIGHT = 690

local DESCRIPTION_WIDTH = 800
local DESCRIPTION_HEIGHT = 320

local BUY_BUTTON_WIDTH = 320
local BUY_BUTTON_HEIGHT = 64

local TEXT_START_OFFSET = Vector2.new(0, 70)
local TEXT_SPACING = Vector2.new(0, 20)
local ROBUX_TEXT_OFFSET = Vector2.new(0, 25)
local DETAIL_TEXT_OFFSET = Vector2.new(0, 30)

local BUY_BUTTON_OFFSET = Vector2.new(0, -50)

local ROBUX_BALANCE_OFFSET = Vector2.new(100, -130)

local function CreatePurchasePackagePrompt(packageInfo)
	local this = {}

	local MyParent = nil
	local Result = nil
	local purchasing = false
	local finishedLoading = false
	local balance = nil
	local inFocus = false
	local ResultEvent = Utility.Signal()

	local packageName = packageInfo:GetFullName()
	local robuxPrice = packageInfo:GetRobuxPrice()
	local creatorId = packageInfo:GetCreatorId()


	local ModalBackground = Utility.Create'Frame'
	{
		Name = "PurchasePackagePrompt";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		BackgroundColor3 = GlobalSettings.ModalBackgroundColor;
		BorderSizePixel = 0;
		ZIndex = 4;
	}

		local ContentContainer = Utility.Create'Frame'
		{
			Name = "ContentContainer";
			Size = UDim2.new(CONTENT_WIDTH/MOCKUP_WIDTH, 0, CONTENT_HEIGHT/MOCKUP_HEIGHT, 0);
			Position = UDim2.new(CONTENT_POSITION.x/MOCKUP_WIDTH, 0, CONTENT_POSITION.y/MOCKUP_HEIGHT, 0);
			BackgroundTransparency = 0;
			BackgroundColor3 = GlobalSettings.OverlayColor;
			BorderSizePixel = 0;
			ZIndex = 4;
			Parent = ModalBackground;
		}
		-- Utility.CalculateAnchor(ContentContainer, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)

			local PackageContainer = Utility.Create'Frame'
			{
				Name = "PackageContainer";
				Size = UDim2.new(PACKAGE_CONTAINER_WIDTH/CONTENT_WIDTH, 0, PACKAGE_CONTAINER_HEIGHT/CONTENT_HEIGHT, 0);
				Position = UDim2.new(0,0,0,0);
				BackgroundTransparency = 1;
				BorderSizePixel = 0;
				ZIndex = 4;
				Parent = ContentContainer;
			}
				local PackageBackground = Utility.Create'Frame'
				{
					Name = "PackageBackground";
					Size = UDim2.new(PACKAGE_BACKGROUND_WIDTH/PACKAGE_CONTAINER_WIDTH, 0, PACKAGE_BACKGROUND_HEIGHT/CONTENT_HEIGHT, 0);
					BackgroundTransparency = 0;
					BackgroundColor3 = GlobalSettings.ForegroundGreyColor;
					BorderSizePixel = 0;
					ClipsDescendants = true;
					ZIndex = 4;
					Parent = PackageContainer;
				}
				Utility.CalculateAnchor(PackageBackground, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)
					local PackageImage = Utility.Create'ImageLabel'
					{
						Name = 'PackageImage';
						Size = UDim2.new(1,0,1,0);
						Image = Http.GetThumbnailUrlForAsset(packageInfo:GetAssetId());
						BackgroundTransparency = 1;
						ZIndex = 4;
						Parent = PackageBackground;
					};
			local DetailsContainer = Utility.Create'Frame'
			{
				Name = "DetailsContainer";
				Size = UDim2.new(DETAILS_CONTAINER_WIDTH/CONTENT_WIDTH, 0, DETAILS_CONTAINER_HEIGHT/CONTENT_HEIGHT, 0);
				Position = UDim2.new(PACKAGE_CONTAINER_WIDTH/CONTENT_WIDTH,0,0,0);
				BackgroundTransparency = 1;
				BorderSizePixel = 0;
				ZIndex = 4;
				Parent = ContentContainer;
			}
				local PurchasingTitle = Utility.Create'TextLabel'
				{
					Name = 'PurchasingTitle';
					Text = Strings:LocalizedString('PurchasingTitle');
					Position = UDim2.new(0, 0, 0, 66);
					Size = UDim2.new(1,0,0,25);
					TextXAlignment = 'Left';
					TextColor3 = GlobalSettings.WhiteTextColor;
					Font = GlobalSettings.HeadingFont;
					FontSize = GlobalSettings.HeaderSize;
					BackgroundTransparency = 1;
					ZIndex = 4;
					Visible = false;
					Parent = DetailsContainer;
				};
			local DetailsContent = Utility.Create'Frame'
			{
				Name = "DetailsContent";
				Size = UDim2.new(1, 0, 1, 0);
				Position = UDim2.new(0,0,0,0);
				BackgroundTransparency = 1;
				BorderSizePixel = 0;
				ZIndex = 4;
				Parent = DetailsContainer;
			}

				local PackageName = Utility.Create'TextLabel'
				{
					Name = 'PackageName';
					Text = packageName or "Unknown Package";
					-- Position = UDim2.new(TEXT_START_OFFSET.X/DETAILS_CONTAINER_WIDTH, 0, TEXT_START_OFFSET.Y/DETAILS_CONTAINER_HEIGHT, 0);
					Position = UDim2.new(0, 0, 0, 66);
					Size = UDim2.new(1,0,0,25);
					TextXAlignment = 'Left';
					TextColor3 = GlobalSettings.WhiteTextColor;
					Font = GlobalSettings.HeadingFont;
					FontSize = GlobalSettings.HeaderSize;
					BackgroundTransparency = 1;
					ZIndex = 4;
					Parent = DetailsContent;
				};
				local RobuxIcon = Utility.Create'ImageLabel'
				{
					Name = 'RobuxIcon';
					Position = UDim2.new(0,0,0,125);
					-- Position = PackageName.Position + UDim2.new(ROBUX_TEXT_OFFSET.X/DETAILS_CONTAINER_WIDTH,0,ROBUX_TEXT_OFFSET.Y/DETAILS_CONTAINER_HEIGHT + PackageName.Size.Y.Scale, PackageName.Size.Y.Offset);
					Size = UDim2.new(0,50,0,50);
					BackgroundTransparency = 1;
					ZIndex = 4;
					Parent = DetailsContent;
				};
				AssetManager.LocalImage(RobuxIcon, 'rbxasset://textures/ui/Shell/Icons/ROBUXIcon', {['720'] = UDim2.new(0,28,0,28); ['1080'] = UDim2.new(0,42,0,42);})
					local PackageCost = Utility.Create'TextLabel'
					{
						Name = 'PackageCost';
						Text = '';
						Size = UDim2.new(0,0,1,0);
						Position = UDim2.new(1.3,0,0,0);
						TextXAlignment = 'Left';
						TextColor3 = GlobalSettings.GreenTextColor;
						Font = GlobalSettings.RegularFont;
						FontSize = GlobalSettings.HeaderSize;
						BackgroundTransparency = 1;
						ZIndex = 4;
						Parent = RobuxIcon;
					};

				local AlreadyOwnTextLabel = Utility.Create'TextLabel'
				{
					Name = 'AlreadyOwnTextLabel';
					Text = '';
					Size = UDim2.new(0,0,0,50);
					Position = UDim2.new(0,0,0,125);
					-- Position = PackageName.Position + UDim2.new(ROBUX_TEXT_OFFSET.X/DETAILS_CONTAINER_WIDTH,0,ROBUX_TEXT_OFFSET.Y/DETAILS_CONTAINER_HEIGHT + PackageName.Size.Y.Scale, PackageName.Size.Y.Offset);
					TextXAlignment = 'Left';
					TextColor3 = GlobalSettings.GreenTextColor;
					Font = GlobalSettings.ItalicFont;
					FontSize = GlobalSettings.DescriptionSize;
					BackgroundTransparency = 1;
					ZIndex = 4;
					Visible = false;
					Parent = DetailsContent;
				};

				local descriptionScrollingTextBox = ScrollingTextBox(UDim2.new(DESCRIPTION_WIDTH/DETAILS_CONTAINER_WIDTH, 0, DESCRIPTION_HEIGHT/DETAILS_CONTAINER_HEIGHT, 0),
					-- RobuxIcon.Position + UDim2.new(DETAIL_TEXT_OFFSET.X/DETAILS_CONTAINER_WIDTH,0,DETAIL_TEXT_OFFSET.Y/DETAILS_CONTAINER_HEIGHT + RobuxIcon.Size.Y.Scale, RobuxIcon.Size.Y.Offset),
					UDim2.new(0, 0, 0, 200),
					DetailsContent)
				descriptionScrollingTextBox:SetZIndex(5)
				descriptionScrollingTextBox:SetFontSize(GlobalSettings.TitleSize)
				descriptionScrollingTextBox:SetFont(GlobalSettings.LightFont)

				local BuyButton = Utility.Create'ImageButton'
				{
					Name = "BuyButton";
					Size = UDim2.new(BUY_BUTTON_WIDTH/DETAILS_CONTAINER_WIDTH, 0, BUY_BUTTON_HEIGHT/DETAILS_CONTAINER_HEIGHT, 0);
					BorderSizePixel = 0;
					BackgroundColor3 = GlobalSettings.BlueButtonColor;
					BackgroundTransparency = 0;
					ZIndex = 4;
					Parent = DetailsContent;
				}
				Utility.CalculateAnchor(BuyButton, UDim2.new(0, 0, 1 + BUY_BUTTON_OFFSET.Y/DETAILS_CONTAINER_HEIGHT, 0), Utility.Enum.Anchor.BottomLeft)
					local BuyText = Utility.Create'TextLabel'
					{
						Name = 'BuyText';
						Text = '';
						Size = UDim2.new(1,0,1,0);
						TextColor3 = GlobalSettings.TextSelectedColor;
						Font = GlobalSettings.HeadingFont;
						FontSize = GlobalSettings.MediumLargeHeadingSize;
						BackgroundTransparency = 1;
						ZIndex = 4;
						Parent = BuyButton;
					};






	local function OnOwnedUpdate()
		if packageInfo:IsOwned() or robuxPrice == nil then
			if packageInfo:IsOwned() then
				if robuxPrice and robuxPrice > 0 then
					AlreadyOwnTextLabel.Text = string.format(Strings:LocalizedString('PurchasedThisPhrase'), Utility.FormatNumberString(tostring(robuxPrice)))
				else
					AlreadyOwnTextLabel.Text = Strings:LocalizedString('AlreadyOwnedPhrase')
				end
			end
			AlreadyOwnTextLabel.Visible = (packageInfo:IsOwned() == true)
			RobuxIcon.Visible = false
			BuyText.Text = Strings:LocalizedString("OkWord")
		else
			if robuxPrice and robuxPrice == 0 then
				PackageCost.Text = Strings:LocalizedString('FreeWord'):upper();
			else
				PackageCost.Text = robuxPrice and Utility.FormatNumberString(tostring(robuxPrice)) or '-';
			end
			RobuxIcon.Visible = true
			AlreadyOwnTextLabel.Visible = false
			if robuxPrice == 0 then
				BuyText.Text = Strings:LocalizedString('TakeWord'):upper();
			elseif balance and robuxPrice > balance then
				BuyText.Text = Strings:LocalizedString('GetRobuxPhrase'):upper();
			else
				BuyText.Text = Strings:LocalizedString('BuyWord'):upper();
			end
		end
	end

	local function SetBalance(newBalance)
		balance = newBalance
		OnOwnedUpdate()
	end

	function this:UpdateOwned()
		OnOwnedUpdate()
	end

	function this:ResultAsync()
		if Result then
			return Result
		end
		ResultEvent:wait()
		return Result
	end


	do
		local function loadBalanceAsync()
			local balance = UserDataModule.GetPlatformUserBalanceAsync()
			SetBalance(balance)
		end
		local function loadDescription()
			local descriptionText = packageInfo:GetDescriptionAsync()
			descriptionScrollingTextBox:SetText(descriptionText or "")
		end

		DetailsContent.Visible = false

		local loader = LoadingWidget({Parent = DetailsContainer}, {loadBalanceAsync, loadDescription})
		spawn(function()
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil
			ScreenManager:DefaultFadeIn(DetailsContent)
			DetailsContent.Visible = true
			if inFocus then
				GuiService.SelectedCoreObject = this:GetDefaultSelectableObject()
			end
			finishedLoading = true
		end)
	end

	local function DoPurchase()
		purchasing = true
		local wasOwned = packageInfo:IsOwned()
		local purchaseResult = packageInfo:BuyAsync()
		local newBalance = purchaseResult and purchaseResult['balanceAfterSale'] or UserDataModule.GetPlatformUserBalanceAsync()
		-- print('purchaseResult')
		-- print(Utility.PrettyPrint(purchaseResult))
		SetBalance(newBalance)
		local nowOwns = packageInfo:IsOwned() and not wasOwned
		Result = nowOwns
		if nowOwns then
			this:UpdateOwned()
		end
		purchasing = false
		if not wasOwned and not nowOwns then
			if ScreenManager:GetTopScreen() == this then
				ScreenManager:CloseCurrent()
			end
			ScreenManager:OpenScreen(ErrorOverlayModule(Errors.PackagePurchase[1]), false)
		end
		print("Done with purchase")
	end


	OnOwnedUpdate()

	function this:GetDefaultSelectableObject()
		return BuyButton
	end

	function this:FadeInBackground()
		Utility.PropertyTweener(ModalBackground, "BackgroundTransparency", 1, GlobalSettings.ModalBackgroundTransparency, 0.25, Utility.EaseInOutQuad, true)
	end

	local currencyWidget = nil
	local RobuxChangedConn = nil
	function this:Show()
		ModalBackground.Visible = true
		ModalBackground.Parent = MyParent

		local function onPackageBackgroundResize()
			PackageImage.Size = Utility.CalculateFill(PackageBackground, Vector2.new(420, 420))
			Utility.CalculateAnchor(PackageImage, UDim2.new(0.5,0,0.5,0), Utility.Enum.Anchor.Center)
		end

		self.PackageBackgroundChangedConn = Utility.DisconnectEvent(self.PackageBackgroundChangedConn)
		self.PackageBackgroundChangedConn = PackageBackground.Changed:connect(function(prop)
			if prop == 'AbsoluteSize' then
				onPackageBackgroundResize()
			end
		end)
		onPackageBackgroundResize()

		if not currencyWidget then
			currencyWidget = CurrencyWidgetModule({Parent = ModalBackground; Position = UDim2.new(0.052, 0, 0.88, 0); ZIndex = 4;})
		end
		Utility.DisconnectEvent(RobuxChangedConn)
		RobuxChangedConn = currencyWidget.RobuxChanged:connect(SetBalance)

		SoundManager:Play('OverlayOpen')
	end

	function this:Hide()
		ModalBackground.Visible = false
		ModalBackground.Parent = nil

		self.PackageBackgroundChangedConn = Utility.DisconnectEvent(self.PackageBackgroundChangedConn)
		RobuxChangedConn = Utility.DisconnectEvent(RobuxChangedConn)
	end

	function this:ScreenRemoved()
		if currencyWidget then
			currencyWidget:Destroy()
			currencyWidget = nil
		end
		ResultEvent:fire()
	end

	function this:Focus()
		inFocus = true
		ContextActionService:BindCoreAction("ReturnFromPurchasePackageScreen",
			function(actionName, inputState, inputObject)
				if not purchasing then
					if inputState == Enum.UserInputState.End then
						ScreenManager:CloseCurrent()
					end
				end
			end,
			false,
			Enum.KeyCode.ButtonB)

		local buyButtonDebounce = false
		self.BuyButtonConn = Utility.DisconnectEvent(self.BuyButtonConn)
		self.BuyButtonConn = BuyButton.MouseButton1Click:connect(function()
			if buyButtonDebounce or purchasing or not finishedLoading then
				return
			end
			buyButtonDebounce = true

			if packageInfo:IsOwned() or robuxPrice == nil then
				SoundManager:Play('ButtonPress')
				ScreenManager:CloseCurrent()
			else
				if balance then
					if robuxPrice > 0 and balance < robuxPrice then
						print("Goto robux screen")
						EventHub:dispatchEvent(EventHub.Notifications["NavigateToRobuxScreen"])
					else
						local confirmPrompt = CreateConfirmPrompt({ProductName = packageName; Cost = robuxPrice; Balance = balance; ProductImage = Http.GetThumbnailUrlForAsset(packageInfo:GetAssetId()); Currency = "ROBUX"; CurrencySymbol = ""},
																  {ShowRemainingBalance = true; ShowRobuxIcon = true;})
						confirmPrompt:SetParent(MyParent)
						ScreenManager:OpenScreen(confirmPrompt)
						local result = confirmPrompt:ResultAsync()
						if result == true then
							local loader = LoadingWidget({Parent = DetailsContainer}, {DoPurchase})
							DetailsContent.Visible = false
							PurchasingTitle.Visible = true
							spawn(function()
								loader:AwaitFinished()
								loader:Cleanup()
								loader = nil
								ScreenManager:DefaultFadeIn(DetailsContent)
								DetailsContent.Visible = true
								PurchasingTitle.Visible = false

								if ScreenManager:GetTopScreen() == this then
									ScreenManager:CloseCurrent()
								end
							end)

						else
							print("Declined to buy")
						end
					end
				end
			end

			buyButtonDebounce = false
		end)

		GuiService:AddSelectionParent("PurchasePackagePromptSelectionGroup", ContentContainer)
		GuiService.SelectedCoreObject = self:GetDefaultSelectableObject()
	end

	function this:RemoveFocus()
		inFocus = false
		ContextActionService:UnbindCoreAction("ReturnFromPurchasePackageScreen")
		GuiService:RemoveSelectionGroup("PurchasePackagePromptSelectionGroup")
		self.BuyButtonConn = Utility.DisconnectEvent(self.BuyButtonConn)
		GuiService.SelectedCoreObject = nil
	end


	function this:SetParent(parent)
		MyParent = parent
		ModalBackground.Parent = MyParent
	end


	return this
end

return CreatePurchasePackagePrompt
