
--[[
			// PackageData.lua

			// Created by Kip Turner
			// Copyright Roblox 2015
]]

local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")


local Utility = require(Modules:FindFirstChild('Utility'))

local Http = require(Modules:FindFirstChild('Http'))
local UserData = require(Modules:FindFirstChild('UserData'))
local EventHub = require(Modules:FindFirstChild('EventHub'))


local ContentProvider = game:GetService("ContentProvider")
local MarketplaceService = Game:GetService('MarketplaceService')
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)


local CurrentlyWearingAssetId = nil

local RequestingWearAsset = false

local function AwaitWearAssetRequest()
	while RequestingWearAsset do wait(0.1) end
end

-- local function PreloadCharacterAppearanceAsync()
-- 	-- Preload content so that when we join a game our appearance is cached
-- 	local myAsset = Http.GetCharactersAssetsAsync(UserData:GetLocalUserIdAsync())
-- 	print("MyAsset:" , myAsset)
-- 	if myAsset then
-- 		local assetList = Utility.SplitString(myAsset, ";")
-- 		print("Parsed:" , Utility.PrettyPrint(assetList))
-- 		ContentProvider:PreloadAsync(assetList)
-- 	end
-- end

local function PreloadCharacterAppearanceAsync()
	local character = nil
	local success, msg = pcall(function()
		character = game.Players:GetCharacterAppearanceAsync(UserData:GetLocalUserIdAsync())
	end)
	if character then
		local assetUrl = Http.BaseUrl .. 'asset/?id='
		local assetList = Utility.FindAssetsInModel(character, assetUrl)
		-- print("Preloading:" , Utility.PrettyPrint(assetList))
		ContentProvider:PreloadAsync(assetList)
	end


	return success
end

local function SetOwnedInternal(packageData, newValue)
	if packageData.Owned ~= newValue then
		packageData.Owned = newValue
		packageData.OwnershipChanged:fire(packageData.Owned)
	end
end


local function CreatePackageItem(data)
	local this = {}

	this.Owned = false
	this.OwnershipChanged = Utility.Signal()
	this.IsWearingChanged = Utility.Signal()

	local productInfo = nil

	-- print("PackageData:" , Utility.PrettyPrint(data))


	function this:GetAssetId()
		local assetId = data and data['AssetId']
		if not assetId then
			assetId = data and data['Item'] and data['Item']['AssetId']
		end
		return assetId
	end

	function this:GetProductIdAsync()
		while not productInfo do wait() end
		return productInfo and productInfo['ProductId']
	end

	function this:BuyAsync()
		print("Do buy" , 'productId' , self:GetProductIdAsync() , 'robuxPrice' , self:GetRobuxPrice())
		local purchaseResult = Http.PurchaseProductAsync(self:GetProductIdAsync(), self:GetRobuxPrice(), self:GetCreatorId(), 1)
		local nowOwns = purchaseResult and purchaseResult['TransactionVerb'] == 'bought'
		if nowOwns then
			SetOwnedInternal(self, nowOwns)
		end
		return purchaseResult
	end

	function this:IsOwned()
		return self.Owned
	end

	local lastIsWearing = nil
	function this:IsWearing()
		local wasWearing = lastIsWearing
		lastIsWearing = CurrentlyWearingAssetId and CurrentlyWearingAssetId == self:GetAssetId() and self:IsOwned()
		if lastIsWearing ~= wasWearing then
			self.IsWearingChanged:fire(lastIsWearing)
		end
		return lastIsWearing
	end

	function this:GetRobuxPrice()
		local robuxPrice = data and data['PriceInRobux']
		if not robuxPrice then
			robuxPrice = data and data['Product'] and data['Product']['PriceInRobux']
		end
		local isPublicDomain = data and data['IsPublicDomain'] == true
		if isPublicDomain == nil then
			isPublicDomain = data and data['Product'] and data['Product']['IsPublicDomain'] == true
		end
		if not robuxPrice and isPublicDomain then
			robuxPrice = 0
		end

		return robuxPrice
	end

	function this:GetCreatorId()
		return data and data['Creator'] and data['Creator']['Id']
	end

	function this:WearAsync()
		local assetId = self:GetAssetId()
		if assetId then
			AwaitWearAssetRequest()

			EventHub:dispatchEvent(EventHub.Notifications["AvatarEquipBegin"], assetId)

			RequestingWearAsset = true
			local result = Http.PostWearAssetAsync(assetId)
			RequestingWearAsset = false

			if result and result['success'] == true then
				EventHub:dispatchEvent(EventHub.Notifications["DonnedDifferentPackage"], assetId)
			end


			-- print("Http result from post wear asset:")
			-- print(Utility.PrettyPrint(result))
			return result
		end
	end

	function this:GetName()
		local resultPackageName = self:GetFullName()

		if resultPackageName then
			local colonPosition = string.find(resultPackageName, ":")
			if colonPosition then
				resultPackageName = string.sub(resultPackageName, 1, colonPosition - 1)
			end
		else
			resultPackageName = "Unknown"
		end

		return resultPackageName
	end

	function this:GetFullName()
		local name = data and data['Name']
		if not name then
			name = data and data['Item'] and data["Item"]['Name']
		end
		return name or "Unknown"
	end

	function this:GetDescriptionAsync()
		while not productInfo do wait() end
		return productInfo and productInfo['Description']
	end

	spawn(function()
		productInfo = MarketplaceService:GetProductInfo(this:GetAssetId())
		if productInfo == nil then productInfo = {} end
	end)

	return this
end



local PackageData = {}
local PackageCache = nil


local function GetAvailableXboxCatalogPackagesAsync()
	local isFinalPage = false

	local packages = {}
	local index = 0
	local count = 20

	repeat
		local result = nil

		Utility.ExponentialRepeat(
			function() return result == nil end,
			function() result = Http.GetXboxProductsAsync(index, count) end,
			2)

		if result then
			local items = result['Products']
			if items then
				if #items < count then
					isFinalPage = true
				end
				for _, itemInfo in pairs(items) do
					table.insert(packages, CreatePackageItem(itemInfo))
				end
			end
		end

		index = index + count
	until result == nil or isFinalPage
	-- print("Xbox products:" , packages , Utility.PrettyPrint(packages))

	if isFinalPage then
		return packages
	end
end

local function GetOwnedCatalogPackageIdsByUserAsync(userId)
	local packages = Http.GetUserOwnedPackagesAsync(userId)
	if packages then
		local data = packages['IsValid'] and packages['Data']
		local items = data and data['Items']
		local result = {}
		-- print("Items"  , Utility.PrettyPrint(items))
		if items then
			for _, itemInfo in pairs(items) do
				local assetId = itemInfo and itemInfo['Item'] and itemInfo['Item']['AssetId']
				result[assetId] = itemInfo
			end
		end
		return result
	end
end


local function getCatalogPackagesAsync()
	local xboxCatalogPackages = GetAvailableXboxCatalogPackagesAsync()
	local myPackages = GetOwnedCatalogPackageIdsByUserAsync(UserData:GetRbxUserId())

	if xboxCatalogPackages and myPackages then
		local result = {}
		for _, xboxPackage in pairs(xboxCatalogPackages) do
			local owned = (myPackages[xboxPackage:GetAssetId()] ~= nil)
			SetOwnedInternal(xboxPackage, owned)
			table.insert(result, xboxPackage)
		end


		-- NOTE: Temporary changed to get items that you bought outside xbox
		-- Create a map of what we already have
		local haveAssets = {}
		for _, package in pairs(result) do
			haveAssets[package:GetAssetId()] = true
		end

		for assetId, packageInfo in pairs(myPackages) do
			if not haveAssets[assetId] then
				local myPackage = CreatePackageItem(packageInfo)
				SetOwnedInternal(myPackage, true)
				table.insert(result, myPackage)
			end
		end
		return result
	end
end

local function SetCurrentlyWearingAssetId(newValue)
	if CurrentlyWearingAssetId ~= newValue then
		CurrentlyWearingAssetId = newValue
		if PackageCache then
			for _, package in pairs(PackageCache) do
				package:IsWearing()
			end
		end
	end
end


local UserChangedCount = 0
local function OnUserAccountChanged()
	UserChangedCount = UserChangedCount + 1

	PackageCache = nil
	SetCurrentlyWearingAssetId(nil)

	local wearingAsset = PackageData:GetCurrentlyWearingPackageAssetIdAsync()
	if not CurrentlyWearingAssetId then
		SetCurrentlyWearingAssetId(wearingAsset)
	end
end

spawn(function()
	local function queryWearingAsset()
		local wearingAsset = PackageData:GetCurrentlyWearingPackageAssetIdAsync()
		SetCurrentlyWearingAssetId(wearingAsset)
	end

	EventHub:addEventListener(EventHub.Notifications["DonnedDifferentPackage"], "PackageData",
	function(assetId)
		SetCurrentlyWearingAssetId(assetId)
		spawn(PreloadCharacterAppearanceAsync)
	end)
	EventHub:addEventListener(EventHub.Notifications["DonnedDifferentOutfit"], "PackageData",
	function(outfitId)
		queryWearingAsset()
		spawn(PreloadCharacterAppearanceAsync)
	end)
	EventHub:addEventListener(EventHub.Notifications["AuthenticationSuccess"], "PackageData", OnUserAccountChanged)
	-- Wait until we are ready to make the call be being signed in
	if UserData:GetRbxUserId() then
		queryWearingAsset()
		spawn(PreloadCharacterAppearanceAsync)
	end
end)

local debounceGetXboxCatalogPackages = false
function PackageData:GetXboxCatalogPackagesAsync()
	if debounceGetXboxCatalogPackages then
		while debounceGetXboxCatalogPackages do wait() end
	end
	debounceGetXboxCatalogPackages = true

	-- Ensure that the catalog data we are getting is applicable to the
	-- currently logged in user
	-- while not PackageCache do
		local startCount = UserChangedCount
		local packageData = nil

		Utility.ExponentialRepeat(
			function() return packageData == nil and startCount == UserChangedCount end,
			function() packageData = getCatalogPackagesAsync() end,
			3)

		if startCount == UserChangedCount then
			PackageCache = packageData
		end
	-- end

	debounceGetXboxCatalogPackages = false

	return PackageCache
end


function PackageData:GetCurrentlyWearingPackageAssetIdAsync()
	local currentWearingData = Http.GetXboxCurrentlyWearingPackageAsync()
	return currentWearingData and currentWearingData['AssetId']
end

function PackageData:GetCachedWearingPackage()
	return CurrentlyWearingAssetId
end

function PackageData:AwaitWearAssetRequest()
	AwaitWearAssetRequest()
end


return PackageData
