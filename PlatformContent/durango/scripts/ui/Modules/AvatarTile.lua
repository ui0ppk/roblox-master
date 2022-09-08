--[[
			// AvatarTile.lua

			// Created by Kip Turner
			// Copyright Roblox 2015
]]



local TextService = game:GetService('TextService')

local ContextActionService = game:GetService("ContextActionService")
local CoreGui = game:GetService("CoreGui")
local ContentProvider = game:GetService("ContentProvider")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")


local Utility = require(Modules:FindFirstChild('Utility'))
local Http = require(Modules:FindFirstChild('Http'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))

local Errors = require(Modules:FindFirstChild('Errors'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local PopupText = require(Modules:FindFirstChild('PopupText'))
local PurchasePackagePrompt = require(Modules:FindFirstChild('PurchasePackagePrompt'))

local BaseTile = require(Modules:FindFirstChild('BaseTile'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))

local ACTIVE_AVATAR_BACKGROUND_COLOR = Color3.new(45/255, 96/255, 128/255)
local INACTIVE_AVATAR_BACKGROUND_COLOR = Color3.new(106/255, 120/255, 129/255)

local function createAvatarInfoContainer(packageInfo)
	local this = BaseTile()
	local focused = false

	local packageName = packageInfo:GetFullName()

	local function wearPackageAsync()
		if packageInfo:IsOwned() and not packageInfo:IsWearing() then

			local result = packageInfo:WearAsync()

			if result and result['success'] == true then
				this:UpdateEquipButton()
			else
				local err = Errors.PackageEquip['Default']
				ScreenManager:OpenScreen(ErrorOverlayModule(err), false)
			end

		end
	end

	local function buyPackageAsync()
		local newPurchasePrompt = PurchasePackagePrompt(packageInfo)
		newPurchasePrompt:SetParent(GuiRoot)
		newPurchasePrompt:FadeInBackground()
		ScreenManager:OpenScreen(newPurchasePrompt, false)
		spawn(function()
			local didPurchase = newPurchasePrompt:ResultAsync()
			-- print("Buy Package Result:" , didPurchase , "ownsAsset:" , packageInfo:IsOwned())
			if didPurchase then
				SoundManager:Play('PurchaseSuccess')
				wearPackageAsync()
			end
		end)
	end

	local PriceText = Utility.Create'TextLabel'
	{
		Name = 'PriceText';
		Text = '';
		Size = UDim2.new(1,0,0,36);
		Position = UDim2.new(0,0,0,0);
		TextColor3 = GlobalSettings.BlackTextColor;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.DescriptionSize;
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.PriceLabelColor;
		ZIndex = 2;
		Visible = false;
		Parent = this.AvatarItemContainer;
	};

	function this:UpdatePriceText()
		local newText = ""
		local price = packageInfo:GetRobuxPrice()
		if price == 0 then
			newText = Strings:LocalizedString('FreeWord'):upper()
		elseif price then
			newText = "R$ " .. Utility.FormatNumberString(price)
		end

		PriceText.Text = newText
		local priceTextSize = TextService:GetTextSize(PriceText.Text, Utility.ConvertFontSizeEnumToInt(PriceText.FontSize), PriceText.Font, Vector2.new())
		PriceText.Size = UDim2.new(0,priceTextSize.X + 28,0,36)
		Utility.CalculateAnchor(PriceText, UDim2.new(1,-6, 0, 6), Utility.Enum.Anchor.TopRight)
		PriceText.Visible = price ~= nil and not packageInfo:IsOwned()
	end

	if packageInfo:GetAssetId() then
		this:SetImage(Http.GetThumbnailUrlForAsset(packageInfo:GetAssetId()))
	else
		--TODO: show a no package image?
	end


	this:ColorizeImage(packageInfo:IsOwned() and 1 or 0, 0)
	this:SetPopupText(packageInfo:GetName())

	function this:GetAssetId()
		return packageInfo:GetAssetId()
	end

	function this:GetPackageInfo()
		return packageInfo
	end

	function this:UpdateOwnership()
		local ownsAsset = packageInfo:IsOwned()
		self:SetActive(ownsAsset)
		self:ColorizeImage(ownsAsset and 1 or 0, 0)
		self:UpdateEquipButton()
		self:UpdatePriceText()
	end

	function this:UpdateEquipButton()
		self.EquippedCheckmark.Visible = packageInfo:IsWearing()
	end

	local selectDebounce = false
	function this:Select()
		if selectDebounce then return false end
		local result = false
		if packageInfo:IsOwned() and not packageInfo:IsWearing() then
			selectDebounce = true
			spawn(function()
				wearPackageAsync()
				selectDebounce = false
			end)
			result = true
		end
		return result
	end

	function this:OnClick()
		buyPackageAsync()
	end

	local isWearingConn = nil
	local ownershipChangedCn = nil
	local baseShow = this.Show
	function this:Show()
		baseShow(self)
		Utility.DisconnectEvent(isWearingConn)
		packageInfo.IsWearingChanged:connect(function() self:UpdateEquipButton() end)
		Utility.DisconnectEvent(ownershipChangedCn)
		ownershipChangedCn = packageInfo.OwnershipChanged:connect(function()
			self:UpdateOwnership()
		end)
		self:UpdateEquipButton()

		self:UpdateOwnership()
	end

	local baseHide = this.Hide
	function this:Hide()
		baseHide(self)
		isWearingConn = Utility.DisconnectEvent(isWearingConn)
		ownershipChangedCn = Utility.DisconnectEvent(ownershipChangedCn)
	end

	local baseFocus = this.Focus
	local avatarItemClickConn = nil
	function this:Focus()
		baseFocus(self)
		focused = true

		Utility.DisconnectEvent(avatarItemClickConn)
		avatarItemClickConn = self.AvatarItemContainer.MouseButton1Click:connect(function()
			self:OnClick()
		end)

		self:UpdateEquipButton()

		spawn(function()
			wait(0.17)
			if focused then
				if not packageInfo:IsOwned() then
					self:ColorizeImage(1)
				end
			end
		end)
	end

	local baseRemoveFocus = this.RemoveFocus
	function this:RemoveFocus()
		baseRemoveFocus(self)
		focused = false
		avatarItemClickConn = Utility.DisconnectEvent(avatarItemClickConn)

		-- Decolorize unowned packages
		if not packageInfo:IsOwned() then
			self:ColorizeImage(0)
		end
	end

	return this
end

return createAvatarInfoContainer
