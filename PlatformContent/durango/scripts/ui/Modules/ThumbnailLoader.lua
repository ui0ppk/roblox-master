--[[
			// ThumbnailLoader.lua

			// Creates a thumbnail loader object that handles the loading
			// of thumb nails.

			// Thumbnails may not yet be generated, so this will retry generation and
			// assign the final thumbnail
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local ContentProvider = game:GetService('ContentProvider')

local Http = require(Modules:FindFirstChild('Http'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local Utility = require(Modules:FindFirstChild('Utility'))

local ThumbnailLoader = {}

local BaseUrl = Http.BaseUrl
local AssetGameBaseUrl = Http.AssetGameBaseUrl

local RETRIES = 6
local FORMAT = "png"
local FADE_IN_TIME = 0.25

--[[ Sizes ]]--
ThumbnailLoader.Sizes = {
	Small = Vector2.new(100, 100);
	Medium = Vector2.new(250, 250);
	Large  = Vector2.new(576, 324);
}

ThumbnailLoader.SubstitutionTypeResult = {
	None = 0;
	Unapproved = 1;
	PendingReview = 2;
	Broken = 3;
	Unavailable = 4;
	Unknown = 5;
}

ThumbnailLoader.AssetType = {
	Icon = { IsFinal = Http.GetAssetThumbnailFinalAsync;
			 SetImageUrl = 'Thumbs/Asset.ashx?width=%d&height=%d&assetId=%d&ignorePlaceMediaItems=true'; };
	Avatar = { IsFinal = Http.GetAssetAvatarFinalAsync;
			 SetImageUrl = 'Thumbs/Avatar.ashx?width=%d&height=%d&userId=%d&ignorePlaceMediaItems=true'; };
	Outfit = { IsFinal = Http.GetOutfitThumbnailFinalAsync;
			 SetImageUrl = 'Thumbs/Avatar.ashx?width=%d&height=%d&userId=%d&ignorePlaceMediaItems=true'; };
}

--[[
	imageObject - a roblox gui image object (ImageLabel, ImageButton)
	assetId 	- the id of the asset you want an image for
	size 		- a ThumbnailLoader.Sizes
	assetType 	- a ThumbnailLoader.AssetType
]]
function ThumbnailLoader:Create(imageObject, assetId, size, assetType, cachebust)
	local this = {}

	local isLoading = false
	local cancelled = false
	local isFinalSuccess = false
	local uri = AssetGameBaseUrl..string.format(assetType.SetImageUrl, size.x, size.y, assetId or -1)
	if cachebust then
		uri = uri .. '&cb=' .. tostring(tick())
	end
	local getIsFinalFunc = assetType.IsFinal

	local function preloadThumbnailAsync()
		local preloadTable = { uri }
		ContentProvider:PreloadAsync(preloadTable)
	end

	local function tryGetFinalAsync()
		local result = getIsFinalFunc(assetId, size.x, size.y, FORMAT)
		if result then
			local isFinal = result["Final"] or result["thumbnailFinal"]
			local substitutionType = result["substitutionType"] or result["SubstitutionType"]
			if isFinal == true and
					(substitutionType == nil or substitutionType == ThumbnailLoader.SubstitutionTypeResult.None) then
				isFinalSuccess = true
				preloadThumbnailAsync()
				return true
			end
		end
		return false
	end

	local function loadThumbInternalAsync()
		local tryCount = 0
		isLoading = true
		isFinalSuccess = false
		while tryCount <= RETRIES and isLoading and not cancelled do
			if tryGetFinalAsync() then
				break
			end
			tryCount = tryCount + 1
			wait(tryCount ^ 2)
		end
		isLoading = false
	end

	function this:LoadAsync(showSpinner, fadeImage, spinnerProperties)
		spinnerProperties = spinnerProperties or {}

		if not assetId then return end
		if showSpinner == nil then
			showSpinner = true
		end
		if fadeImage == nil then
			fadeImage = true
		end
		-- reset image
		imageObject.Image = ""
		if fadeImage then
			local tween = Utility.PropertyTweener(imageObject, "ImageTransparency", 1, 1, 0,
					Utility.EaseInOutQuad, true, nil)
		end

		-- try first time before starting loading widget
		if not tryGetFinalAsync() then
			if showSpinner then
				spinnerProperties['Parent'] = spinnerProperties['Parent'] or imageObject
				local loader = LoadingWidget(
					spinnerProperties,
					{ loadThumbInternalAsync } )
				loader:AwaitFinished()
				loader:Cleanup()
			else
				loadThumbInternalAsync()
			end
		end

		if not cancelled then
			imageObject.Image = isFinalSuccess and uri or ""
			if fadeImage then
				local tween = Utility.PropertyTweener(imageObject, "ImageTransparency", 1, 0, FADE_IN_TIME,
						Utility.EaseInOutQuad, true, nil)
			end
		end

		return isFinalSuccess
	end

	function this:Cancel()
		isLoading = false
		cancelled = true
	end

	return this
end

return ThumbnailLoader
