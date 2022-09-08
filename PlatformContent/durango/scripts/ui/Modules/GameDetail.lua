--[[
			// GameDetail.lua
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")
local PlatformService = nil
pcall(function() PlatformService = Game:GetService('PlatformService') end)
local GameOptionsSettings = settings():FindFirstChild("Game Options")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GameData = require(Modules:FindFirstChild('GameData'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Utility = require(Modules:FindFirstChild('Utility'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local ImageOverlayModule = require(Modules:FindFirstChild('ImageOverlay'))
local ReportOverlayModule = require(Modules:FindFirstChild('ReportOverlay'))
local BadgeSortModule = require(Modules:FindFirstChild('BadgeSort'))
local ScrollingTextBox = require(Modules:FindFirstChild('ScrollingTextBox'))
local PopupText = require(Modules:FindFirstChild('PopupText'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local GameJoinModule = require(Modules:FindFirstChild('GameJoin'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local VoteViewModule = require(Modules:FindFirstChild('VoteView'))
local BaseScreen = require(Modules:FindFirstChild('BaseScreen'))

local function CreateGameDetail(placeId, placeName, iconId, gameData)
	local this = BaseScreen()

	local LEFT_MARGIN = 8
	local TWEEN_TIME = 0.5
	local BASE_ZINDEX = 2
	local joinOnShow = false
	local isFavorited = false

	local baseButtonTextColor = GlobalSettings.WhiteTextColor
	local selectedButtonTextColor = GlobalSettings.TextSelectedColor

	local selectionChangedCn = nil
	local dataModelViewChangedCn = nil
	local playButtonDebounce = false
	local canJoinGame = true
	local returnedFromGame = true

	--[[ Cache Game Data ]]--
	local getGameDataDebounce = false
	local function getDataAsync()
		while getGameDataDebounce do
			wait()
		end
		getGameDataDebounce = true

		if not gameData then
			gameData = GameData:GetGameDataAsync(placeId)
		end
		getGameDataDebounce = false
		return gameData
	end

	-- override selection image
	local edgeSelectionImage = Utility.Create'ImageLabel'
	{
		Name = "EdgeSelectionImage";
		Size = UDim2.new(1, 32, 1, 32);
		Position = UDim2.new(0, -16, 0, -16);
		Image = 'rbxasset://textures/ui/SelectionBox.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(21,21,41,41);
		BackgroundTransparency = 1;
	}

--[[ Top Level Elements ]]--
	--this:SetParent(parent)
	local GameDetailContainer = this.Container
	this:SetTitle(string.upper(placeName or ''))

	--[[ Inner Class - ContentManager - Handles Positioning/Visiblity of content on this screen ]]--
	local function createContentManager(size, position, parent)
		local contentManager = {}

		-- items are an array from left to right
		local contentItems = {}
		local isTweeningContentLeft = false
		local isTweeningContentRight = false

		local contentContainer = Utility.Create'Frame'
		{
			Name = "ContentContainer";
			Size = size;
			Position = position;
			BackgroundTransparency = 1;
			Parent = parent;
		}

		local function recalcLayout()
			local offset = 0
			for i = 1, #contentItems do
				local currentItem = contentItems[i]
				currentItem.Item.Position = UDim2.new(0, offset + currentItem.Padding.x, 0, currentItem.Padding.y)
				offset = offset + currentItem.Item.Size.X.Offset + currentItem.Padding.x
			end
		end

		function contentManager:AddItem(newItem, padding)
			local contentItem = {}
			contentItem.Item = newItem
			contentItem.Padding = padding or Vector2.new(0, 0)
			newItem.Parent = contentContainer
			table.insert(contentItems, contentItem)
			recalcLayout()
		end

		function contentManager:RemoveItem(itemToRemove)
			for i = 1, #contentItems do
				local contentItem = contentItems[i]
				if contentItem.Item == itemToRemove then
					contentItem.Item.Parent = nil
					table.remove(contentItems, i)
					recalcLayout()
					return
				end
			end
		end

		function contentManager:TweenContent(selectedObject)
			if not selectedObject:IsDescendantOf(contentContainer) then
				return
			end

			-- find parent container of selection
			local parentContainer = selectedObject
			while parentContainer.Parent ~= contentContainer do
				parentContainer = parentContainer.Parent
			end

			if parentContainer.Position.X.Offset + parentContainer.Size.X.Offset > contentContainer.AbsoluteSize.x then
				if not isTweeningContentRight then
					isTweeningContentRight = true
					contentContainer:TweenPosition(UDim2.new(-1, LEFT_MARGIN, 0, contentContainer.Position.Y.Offset), Enum.EasingDirection.InOut,
						Enum.EasingStyle.Quad, TWEEN_TIME, true, function(tweenStatus)
							isTweeningContentRight = false
						end)
				end
			elseif parentContainer.Position.X.Offset < contentContainer.AbsoluteSize.x then
				if not isTweeningContentLeft then
					isTweeningContentLeft = true
					contentContainer:TweenPosition(UDim2.new(0, LEFT_MARGIN, 0, contentContainer.Position.Y.Offset), Enum.EasingDirection.InOut,
						Enum.EasingStyle.Quad, TWEEN_TIME, true, function(tweenStatus)
							isTweeningContentLeft = false
						end)
				end
			end
		end

		return contentManager
	end

	local MyContentManager = createContentManager(UDim2.new(1, 0, 0, 622), UDim2.new(0, LEFT_MARGIN, 0, 210), GameDetailContainer)

	local PlayButton = Utility.Create'ImageButton'
	{
		Name = "PlayButton";
		Size = UDim2.new(0, 228, 0, 72);
		Position = UDim2.new(0, 0, 1, -77);
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.GreenButtonColor;
		Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
		Parent = GameDetailContainer;

		SoundManager:CreateSound('MoveSelection');
		ZIndex = BASE_ZINDEX;
		AssetManager.CreateShadow(1)
	}

	local PlayText = Utility.Create'TextLabel'
	{
		Name = "PlayText";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Text = string.upper(Strings:LocalizedString("PlayWord"));
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = baseButtonTextColor;
		ZIndex = BASE_ZINDEX;
		Parent = PlayButton;
	}
	local FavoriteButton = Utility.Create'ImageButton'
	{
		Name = "FavoriteButton";
		Size = UDim2.new(0, 228, 0, 72);
		Position = UDim2.new(0, PlayButton.Size.X.Offset + 10, 1, -77);
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.GreyButtonColor;
		Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
		Parent = GameDetailContainer;

		SoundManager:CreateSound('MoveSelection');
		ZIndex = BASE_ZINDEX;
		AssetManager.CreateShadow(1)
	}

	local FavoriteText = Utility.Create'TextLabel'
	{
		Name = "FavoriteText";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Text = string.upper(Strings:LocalizedString("FavoriteWord"));
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = baseButtonTextColor;
		ZIndex = BASE_ZINDEX;
		Parent = FavoriteButton;
	}
	local FavoriteStarImage = Utility.Create'ImageLabel'
	{
		Name = "FavoriteStarImage";
		BackgroundTransparency = 1;
		Visible = false;
		ZIndex = BASE_ZINDEX;
		Parent = FavoriteButton;
	}
	AssetManager.LocalImage(FavoriteStarImage,
		'rbxasset://textures/ui/Shell/Icons/FavoriteStar', {['720'] = UDim2.new(0,21,0,21); ['1080'] = UDim2.new(0,32,0,31);})
	FavoriteStarImage.Position = UDim2.new(0, 16, 0.5, -FavoriteStarImage.Size.Y.Offset / 2)

	local FavoriteRedirectFrame = Utility.Create'Frame'
	{
		Name = "FavoriteRedirectFrame";
		Size = UDim2.new(1, 0, 0, FavoriteButton.Size.Y.Offset);
		Position = UDim2.new(0, FavoriteButton.Position.X.Offset + FavoriteButton.Size.X.Offset, 1, FavoriteButton.Position.Y.Offset);
		BackgroundTransparency = 1;
		Selectable = true;
		Parent = GameDetailContainer;
	}

--[[ Scrolling Content Items ]]--

	--[[ Icon Image ]]--
	local GameIconContainer = Utility.Create'Frame'
	{
		Name = "GameIconContainer";
		Size = UDim2.new(0, 566, 1, 0);
		BackgroundTransparency = 1;
	}
	MyContentManager:AddItem(GameIconContainer, Vector2.new(0, 0))
		local GameIconImage = Utility.Create'ImageLabel'
		{
			Name = "GameIconImage";
			Size = UDim2.new(1, 0, 0, 566);
			Position = UDim2.new(0, 0, 0.5, -566/2);
			BackgroundTransparency = 0;
			BorderSizePixel = 0;
			BackgroundColor3 = Color3.new();
			ZIndex = BASE_ZINDEX;
			Parent = GameIconContainer;
			AssetManager.CreateShadow(1);
		}
		local function loadGameIcon(assetId)
			local gameIconLoader = ThumbnailLoader:Create(GameIconImage, assetId,
				ThumbnailLoader.Sizes.Medium, ThumbnailLoader.AssetType.Icon)
			spawn(function()
				gameIconLoader:LoadAsync(true, true, { ZIndex = GameIconImage.ZIndex } )
			end)
		end
		if iconId and type(iconId) == "number" then
			loadGameIcon(iconId)
		else
			spawn(function()
				local data = getDataAsync()
				local id = data:GetGameIconIdAsync()
				loadGameIcon(id)
			end)
		end

	--[[ Rating and Description ]]--
	local RatingDescriptionContainer = Utility.Create'Frame'
	{
		Name = "RatingDescriptionContainer";
		Size = UDim2.new(0, 394, 1, 0);
		BackgroundTransparency = 1;
	}
	MyContentManager:AddItem(RatingDescriptionContainer, Vector2.new(38, 0))
	local RatingDescriptionTitle = Utility.Create'TextLabel'
	{
		Name = "RatingDescriptionTitle";
		Size = UDim2.new(0, 0, 0, 33);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		Text = string.upper(Strings:LocalizedString("RatingDescriptionTitle"));
		TextXAlignment = Enum.TextXAlignment.Left;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.SubHeaderSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Parent = RatingDescriptionContainer;
	}
	local VoteView = VoteViewModule()
	VoteView:SetPosition(UDim2.new(0, 0, 0, RatingDescriptionTitle.Size.Y.Offset))
	local VoteViewContainer = VoteView.Container

	local RatingDescriptionLineBreak = Utility.Create'Frame'
	{
		Name = "RatingDescriptionLineBreak";
		Size = UDim2.new(0, 362, 0, 2);
		Position = UDim2.new(0, 0, 0, VoteViewContainer.Position.Y.Offset + VoteViewContainer.Size.Y.Offset);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.LineBreakColor;
		Parent = RatingDescriptionContainer;
	}

	--[[ Description Container ]]--
	local DescriptionContainer = Utility.Create'Frame'
	{
		Name = "DescriptionContainer";
		Size = UDim2.new(1, 0, 0, 430);
		Position = UDim2.new(0, 0, 0, RatingDescriptionLineBreak.Position.Y.Offset + RatingDescriptionLineBreak.Size.Y.Offset + 20);
		BackgroundTransparency = 1;
		Parent = RatingDescriptionContainer;
	}
	local DescriptionScrollingTextBox = ScrollingTextBox(UDim2.new(1, 0, 1, 0), UDim2.new(0, 0, 0), DescriptionContainer)

--[[ Line Break ]]--
	local RatingThumbsLineBreak = Utility.Create'Frame'
	{
		Name = "LineBreak";
		Size = UDim2.new(0, 2, 0, 566);
		BackgroundTransparency = 0;
		BorderSizePixel = 0;
		BackgroundColor3 = Color3.new(78/255, 78/255, 78/255);
		BorderSizePixel = 0;
	}
	MyContentManager:AddItem(RatingThumbsLineBreak, Vector2.new(32, 33))

--[[ Additional Thumbs ]]--
	local ThumbsContainer = Utility.Create'Frame'
	{
		Name = "ThumbsContainer";
		Size = UDim2.new(0, 630, 1, 0);
		BackgroundTransparency = 1;
	}
	MyContentManager:AddItem(ThumbsContainer, Vector2.new(52, 0))

	local ThumbsTitle = RatingDescriptionTitle:Clone()
	ThumbsTitle.Text = string.upper(Strings:LocalizedString("GameImagesTitle"))
	ThumbsTitle.Parent = ThumbsContainer

	local ThumbsContent = Utility.Create'Frame'
	{
		Name = "ThumbsContent";
		Size = UDim2.new(1, 0, 1, 0);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		Parent = ThumbsContainer;
	}

	local MainThumbImage = Utility.Create'ImageButton'
	{
		Name = "MainThumbImage";
		Size = UDim2.new(1, 0, 0, 374);
		Position = UDim2.new(0, 0, 0, ThumbsTitle.Size.Y.Offset);
		BackgroundTransparency = 0;
		BorderSizePixel = 0;
		BackgroundColor3 = Color3.new();
		ZIndex = BASE_ZINDEX;
		Parent = ThumbsContent;

		SoundManager:CreateSound('MoveSelection');
		AssetManager.CreateShadow(1);
	}
	local SmThumbLeft = Utility.Create'ImageButton'
	{
		Name = "SmThumbLeft";
		Size = UDim2.new(0, 310, 0, 184);
		Position = UDim2.new(0, 0, 0, MainThumbImage.Position.Y.Offset + MainThumbImage.Size.Y.Offset + 8);
		BackgroundTransparency = 0;
		BackgroundColor3 = Color3.new();
		BorderSizePixel = 0;
		ZIndex = BASE_ZINDEX;
		Parent = ThumbsContent;

		SoundManager:CreateSound('MoveSelection');
		AssetManager.CreateShadow(1);
	}
	SmThumbRight = SmThumbLeft:Clone()
	SmThumbRight.Position = UDim2.new(0, SmThumbLeft.Size.X.Offset + 10, 0, SmThumbRight.Position.Y.Offset)
	SmThumbRight.Parent = ThumbsContent

	local MoreThumbsButton = Utility.Create'ImageButton'
	{
		Name = "MoreThumbsButton";
		BackgroundTransparency = 1;
		Visible = false;
		Parent = ThumbsContent;

		SoundManager:CreateSound('MoveSelection');
		ZIndex = BASE_ZINDEX;
	}
	AssetManager.LocalImage(MoreThumbsButton,
		'rbxasset://textures/ui/Shell/Buttons/MoreButton', {['720'] = UDim2.new(0,72,0,33); ['1080'] = UDim2.new(0,108,0,50);})
	MoreThumbsButton.Position = UDim2.new(1, -MoreThumbsButton.Size.X.Offset + 3, 0,
		SmThumbRight.Position.Y.Offset + SmThumbRight.Size.Y.Offset + 12)

--[[ Badges ]]--
	local BadgeContainer = Utility.Create'Frame'
	{
		Name = "BadgeContainer";
		Size = UDim2.new(0, 568, 1, 0);
		BackgroundTransparency = 1;
	}
	MyContentManager:AddItem(BadgeContainer, Vector2.new(52, 0))
	local BadgeTitle = RatingDescriptionTitle:Clone()
	BadgeTitle.Text = string.upper(Strings:LocalizedString("GameBadgesTitle"))
	BadgeTitle.Parent = BadgeContainer

	local BadgeSort = BadgeSortModule(placeName, UDim2.new(1, 0, 0, 566), UDim2.new(0, 0, 0, BadgeTitle.Size.Y.Offset), BadgeContainer)

	--[[ Related Games ]]--
	local RelatedGamesContainer = Utility.Create'Frame'
	{
		Name = "RelatedGamesContainer";
		Size = BadgeContainer.Size;
		BackgroundTransparency = 1;
	}
	MyContentManager:AddItem(RelatedGamesContainer, Vector2.new(52, 0))
	local RelatedGamesTitle = BadgeTitle:Clone()
	RelatedGamesTitle.Text = string.upper(Strings:LocalizedString("RelatedGamesTitle"))
	RelatedGamesTitle.Parent = RelatedGamesContainer

	local RelatedGamesImageFrame = Utility.Create'Frame'
	{
		Name = "RelatedGamesImageFrame";
		Size = UDim2.new(1, 0, 566, 0);
		Position = UDim2.new(0, 0, 0, RelatedGamesTitle.Size.Y.Offset);
		BackgroundTransparency = 1;
		Parent = RelatedGamesContainer;
	}
	-- 2x2 grid of related games
	local RelatedGameImages = {}
	local relatedIndex = 1
	local relatedMargin = 14
	local relatedSize = (566 - relatedMargin) / 2
	for i = 1, 2 do
		for j = 1, 2 do
			local image = Utility.Create'ImageButton'
			{
				Name = tostring(relatedIndex);
				Size = UDim2.new(0, relatedSize, 0, relatedSize);
				Position = UDim2.new(0, (i - 1) * relatedSize + (i - 1) * relatedMargin, 0, (j - 1) * relatedSize + (j - 1) * relatedMargin);
				BackgroundTransparency = 0;
				BackgroundColor3 = GlobalSettings.GreyButtonColor;
				BorderSizePixel = 0;
				ZIndex = BASE_ZINDEX;
				Parent = RelatedGamesImageFrame;

				SoundManager:CreateSound('MoveSelection');
				AssetManager.CreateShadow(1);
			}
			RelatedGameImages[relatedIndex] = image
			relatedIndex = relatedIndex + 1
		end
	end

	local RelatedGamesMoreDetailsLineBreak = RatingThumbsLineBreak:Clone()
	MyContentManager:AddItem(RelatedGamesMoreDetailsLineBreak, Vector2.new(52, RelatedGamesMoreDetailsLineBreak.Position.Y.Offset))

	--[[ More Details ]]--
	local MoreDetailsContainer = Utility.Create'Frame'
	{
		Name = "MoreDetailsContainer";
		Size = UDim2.new(0, 386, 1, 0);
		BackgroundTransparency = 1;
		Selectable = true;
	}
	MyContentManager:AddItem(MoreDetailsContainer, Vector2.new(52, 0))

	local MoreDetailsTitle = RelatedGamesTitle:Clone()
	MoreDetailsTitle.Text = string.upper(Strings:LocalizedString("MoreDetailsTitle"))
	MoreDetailsTitle.Parent = MoreDetailsContainer

	local MoreDetailsContent = Utility.Create'Frame'
	{
		Name = "MoreDetailsContent";
		Size = UDim2.new(1, 0, 1, 0);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		Parent = MoreDetailsContainer;
	}
	local UpdatedText = Utility.Create'TextLabel'
	{
		Name = "UpdatedText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, 0, 0, MoreDetailsTitle.Size.Y.Offset + 48);
		BackgroundTransparency = 1;
		Text = Strings:LocalizedString("LastUpdatedWord");
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.SmallTitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		Parent = MoreDetailsContent;
	}
	local LastUpdatedText = Utility.Create'TextLabel'
	{
		Name = "LastUpdatedText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, 0, 0, UpdatedText.Position.Y.Offset + 34);
		BackgroundTransparency = 1;
		Text = "";
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		Parent = MoreDetailsContent;
	}
	local CreationText = UpdatedText:Clone()
	CreationText.Name = "CreationText"
	CreationText.Position = UDim2.new(0, 0, 0, LastUpdatedText.Position.Y.Offset + 70)
	CreationText.Text = Strings:LocalizedString("CreationDateWord")
	CreationText.Parent = MoreDetailsContent

	local CreationDateText = LastUpdatedText:Clone()
	CreationDateText.Name = "CreationDateText"
	CreationDateText.Position = UDim2.new(0, 0, 0, CreationText.Position.Y.Offset + 34)
	CreationDateText.Parent = MoreDetailsContent

	local MaxPlayersText = UpdatedText:Clone()
	MaxPlayersText.Name = "MaxPlayersText"
	MaxPlayersText.Position = UDim2.new(0, 0, 0, CreationDateText.Position.Y.Offset + 70)
	MaxPlayersText.Text = Strings:LocalizedString("MaxPlayersWord")
	MaxPlayersText.Parent = MoreDetailsContent

	local MaxPlayersCountText = LastUpdatedText:Clone()
	MaxPlayersCountText.Name = "MaxPlayersCountText"
	MaxPlayersCountText.Position = UDim2.new(0, 0, 0, MaxPlayersText.Position.Y.Offset + 34)
	MaxPlayersCountText.Parent = MoreDetailsContent

	local CreatorText = UpdatedText:Clone()
	CreatorText.Name = "CreatorText"
	CreatorText.Position = UDim2.new(0, 0, 0, MaxPlayersCountText.Position.Y.Offset + 70)
	CreatorText.Text = Strings:LocalizedString("CreatedByWord")
	CreatorText.Parent = MoreDetailsContent

	local CreatorIcon = Utility.Create'ImageLabel'
	{
		Name = "CreatorIcon";
		Size = UDim2.new(0, 32, 0, 32);
		Position = UDim2.new(0, 0, 0, CreatorText.Position.Y.Offset + 34 - 14);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/RobloxIcon32.png';
		Parent = MoreDetailsContent;
	}
	local CreatorNameText = LastUpdatedText:Clone()
	CreatorNameText.Name = "CreatorNameText"
	CreatorNameText.Position = UDim2.new(0, CreatorIcon.Size.X.Offset + 8, 0, CreatorText.Position.Y.Offset + 34)
	CreatorNameText.Parent = MoreDetailsContent

	local DetailsBottomLineBreak = Utility.Create'Frame'
	{
		Name = "DetailsBottomLineBreak";
		Size = UDim2.new(0, 308, 0, 2);
		Position = UDim2.new(0, 107, 0, MoreDetailsTitle.Size.Y.Offset + 566 - 2);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.LineBreakColor;
		Parent = MoreDetailsContainer;
	}
	local ReportFrame = Utility.Create'TextButton'
	{
		Name = "ReportFrame";
		Size = UDim2.new(0, 91, 0, 91);
		Position = UDim2.new(0, 0, 0, DetailsBottomLineBreak.Position.Y.Offset - 95);
		BackgroundColor3 = GlobalSettings.GreyButtonColor;
		BackgroundTransparency = 0;
		BorderSizePixel = 0;
		Text = "";
		Parent = MoreDetailsContainer;

		SoundManager:CreateSound('MoveSelection');
		ZIndex = BASE_ZINDEX;
		AssetManager.CreateShadow(1);
	}
	local ReportIcon = Utility.Create'ImageLabel'
	{
		Name = "ReportIcon";
		BackgroundTransparency = 1;
		ZIndex = BASE_ZINDEX;
		Parent = ReportFrame;
	}
	AssetManager.LocalImage(ReportIcon, 'rbxasset://textures/ui/Shell/Icons/ReportIcon',
		{['720'] = UDim2.new(0,35,0,29); ['1080'] = UDim2.new(0,52,0,43);})
	ReportIcon.Position = UDim2.new(0.5, -ReportIcon.Size.X.Offset / 2, 0.5, -ReportIcon.Size.Y.Offset / 2)
	local ReportText = Utility.Create'TextLabel'
	{
		Name = "ReportText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(1, 16, 0.5, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		Text = Strings:LocalizedString("ReportGameWord");
		Parent = ReportFrame;
	}
	ReportFrame.MouseButton1Click:connect(function()
		ScreenManager:OpenScreen(ReportOverlayModule:CreateReportOverlay(ReportOverlayModule.ReportType.REPORT_GAME, placeId), false)
	end)

	--[[ Custom Selection Logic ]]--
	PlayButton.NextSelectionRight = FavoriteButton
	SmThumbLeft.NextSelectionDown = MoreThumbsButton
	SmThumbLeft.NextSelectionRight = SmThumbRight

	local function JoinGame()
		if canJoinGame and returnedFromGame then
			canJoinGame = false
			local gameData = getDataAsync()
			local creatorUserId = nil
			if gameData then
				creatorUserId = gameData:GetCreatorUserId()
			end
			GameJoinModule:StartGame(GameJoinModule.JoinType.Normal, placeId, creatorUserId)
			canJoinGame = true
		end
	end

	local function toggleFavoriteButton(value)
		if value == true then
			FavoriteStarImage.Visible = true
			FavoriteText.Position = UDim2.new(0, FavoriteStarImage.Position.X.Offset + FavoriteStarImage.Size.X.Offset + 12, 0, 0)
			FavoriteText.Text = string.upper(Strings:LocalizedString("FavoritedWord"))
			FavoriteText.TextXAlignment = Enum.TextXAlignment.Left
		elseif value == false then
			FavoriteStarImage.Visible = false
			FavoriteText.Position = UDim2.new(0, 0, 0, 0)
			FavoriteText.Text = string.upper(Strings:LocalizedString("FavoriteWord"))
			FavoriteText.TextXAlignment = Enum.TextXAlignment.Center
		end
	end

	--[[ SelectionGained/Lost Connections ]]--
	PlayButton.SelectionGained:connect(function()
		PlayButton.ImageColor3 = GlobalSettings.GreenSelectedButtonColor
		PlayText.TextColor3 = selectedButtonTextColor
	end)
	PlayButton.SelectionLost:connect(function()
		PlayButton.ImageColor3 = GlobalSettings.GreenButtonColor
		PlayText.TextColor3 = baseButtonTextColor
	end)
	FavoriteButton.SelectionGained:connect(function()
		FavoriteButton.ImageColor3 = GlobalSettings.GreySelectedButtonColor
		FavoriteText.TextColor3 = selectedButtonTextColor
	end)
	FavoriteButton.SelectionLost:connect(function()
		FavoriteButton.ImageColor3 = GlobalSettings.GreyButtonColor
		FavoriteText.TextColor3 = baseButtonTextColor
	end)
	MoreDetailsContainer.SelectionGained:connect(function()
		-- redirect
		GuiService.SelectedCoreObject = ReportFrame
	end)
	FavoriteRedirectFrame.SelectionGained:connect(function()
		GuiService.SelectedCoreObject = FavoriteButton
	end)
	local function updateMoreThumbsButton(isSelected)
		local uri = isSelected and 'rbxasset://textures/ui/Shell/Buttons/MoreButtonSelected' or 'rbxasset://textures/ui/Shell/Buttons/MoreButton'
		AssetManager.LocalImage(MoreThumbsButton, uri, {['720'] = UDim2.new(0,72,0,33); ['1080'] = UDim2.new(0,108,0,50);})
	end
	MoreThumbsButton.SelectionGained:connect(function()
		updateMoreThumbsButton(true)
	end)
	MoreThumbsButton.SelectionLost:connect(function()
		updateMoreThumbsButton(false)
	end)

	--[[ Input Events ]]--
	PlayButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		if playButtonDebounce then return end
		playButtonDebounce = true
		local needToOverscan = false
		local success, errormsg = pcall(function()
			if GameOptionsSettings.OverscanPX < 0 or GameOptionsSettings.OverscanPY < 0 then
				needToOverscan = true
			end
		end)
		if not success then
			print("Error opening the Overscan Screen:" , errormsg)
		end

		if needToOverscan or UserSettings().GameSettings:InStudioMode() then
			EventHub:dispatchEvent(EventHub.Notifications["OpenOverscanScreen"], "")
			joinOnShow = true
		else
			JoinGame()
		end
		playButtonDebounce = false
	end)

	local favoriteDebounce = false
	FavoriteButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		if favoriteDebounce then return end
		favoriteDebounce = true
		local data = getDataAsync()
		if data then
			local result, reason = data:PostFavoriteAsync()
			if result == true then
				isFavorited = not isFavorited
				toggleFavoriteButton(isFavorited)
			elseif reason then
				local err = Errors.Favorite[reason]
				ScreenManager:OpenScreen(ErrorOverlayModule(err), false)
			end
		end
		favoriteDebounce = false
	end)

	--[[ Content Set Functions ]]--
	local function setBadgeContainer(hasBadges, badgeData)
		if hasBadges then
			BadgeSort:Initialize(badgeData)
		else
			MyContentManager:RemoveItem(BadgeContainer)
			BadgeSort:Destroy()
		end
	end

	local function connectThumbImage(image, index, thumbIds)
		local loader = ThumbnailLoader:Create(image, thumbIds[index], ThumbnailLoader.Sizes.Large,
			ThumbnailLoader.AssetType.Icon, false)
		image.MouseButton1Click:connect(function()
			SoundManager:Play('ScreenChange')
			ScreenManager:OpenScreen(ImageOverlayModule(thumbIds, index), false)
		end)
		spawn(function()
			loader:LoadAsync(true, true, { ZIndex = image.ZIndex } )
		end)
	end
	local function setAdditionalThumbs(thumbIds)
		if #thumbIds >= 3 then
			connectThumbImage(MainThumbImage, 1, thumbIds)
			connectThumbImage(SmThumbLeft, 2, thumbIds)
			connectThumbImage(SmThumbRight, 3, thumbIds)

			if #thumbIds > 3 then
				MoreThumbsButton.Visible = true
				MoreThumbsButton.MouseButton1Click:connect(function()
					SoundManager:Play('ScreenChange')
					ScreenManager:OpenScreen(ImageOverlayModule(thumbIds, 1), false)
				end)
			end
		else
			MyContentManager:RemoveItem(ThumbsContainer)
		end
	end
	local function setRelatedGames(data)
		if #data > 0 then
			for i = 1, #RelatedGameImages do
				local image = RelatedGameImages[i]
				if data[i] then
					local thisData = data[i]
					local thumbLoader = ThumbnailLoader:Create(image, thisData.IconId,
						ThumbnailLoader.Sizes.Medium, ThumbnailLoader.AssetType.Icon)
					spawn(function()
						thumbLoader:LoadAsync(true, true, { ZIndex = image.ZIndex } )
					end)
					-- connect open detail event
					image.MouseButton1Click:connect(function()
						EventHub:dispatchEvent(EventHub.Notifications["OpenGameDetail"], thisData.PlaceId, thisData.Name, thisData.IconId)
					end)
					PopupText(image, thisData["Name"])
				end
			end
		else
			MyContentManager:RemoveItem(RelatedGamesContainer)
		end
	end
	local function setScrollingTextBoxSelections(isSelectable)
		if isSelectable then
			local descriptionSelectionObject = DescriptionScrollingTextBox:GetSelectableObject()
			FavoriteButton.NextSelectionRight = descriptionSelectionObject
			descriptionSelectionObject.NextSelectionRight = MainThumbImage
			descriptionSelectionObject.NextSelectionLeft = FavoriteButton
		end
	end
	local function setVotingSelection()
		local defaultSelection = VoteView:GetDefaultSelection()
		if defaultSelection then
			defaultSelection.NextSelectionLeft = FavoriteButton
			FavoriteButton.NextSelectionRight = defaultSelection
			FavoriteButton.NextSelectionUp = defaultSelection
			DescriptionScrollingTextBox:GetSelectableObject().NextSelectionUp = defaultSelection
		end
	end
	local function setOverrideAvatarDisplay()
		local overrideFrame = Utility.Create'Frame'
		{
			Name = "OverrideFrame";
			Size = UDim2.new(1, 0, 0.1, 0);
			BackgroundTransparency = 1;
			Parent = DescriptionContainer;
		}
		local overrideText = Utility.Create'TextLabel'
		{
			Name = "OverrideText";
			Size = UDim2.new(0, 0, 0, 0);
			Position = UDim2.new(0, 190, 0.25, 0);
			BackgroundTransparency = 1;
			Font = GlobalSettings.BoldFont;
			FontSize = GlobalSettings.SubHeaderSize;
			TextColor3 = GlobalSettings.GreyTextColor;
			Text = Strings:LocalizedString("CustomAvatarPhrase");
			Parent = overrideFrame;
		}
		DescriptionScrollingTextBox:SetPosition(UDim2.new(0, 0, 0.1, 0))
		DescriptionScrollingTextBox:SetSize(UDim2.new(1, 0, 0.9, 0))
	end

	--[[ Initialize Content - Don't Block ]]--
	local scrollingTextBoxSelectableChangedCn = nil

	local function waitForTweensToFinish()
		while this and this.TransitionTweens and this.TransitionTweens[1] and not this.TransitionTweens[1]:IsFinished() do
			wait()
		end
	end
	local function concatTables(t1, t2)
		for _, value in pairs(t2) do
			table.insert(t1, value)
		end
	end
	do
		RatingDescriptionLineBreak.Visible = false
		DescriptionContainer.Visible = false

		local function loadFavoritedDataAsync()
			local data = getDataAsync()
			if not data then
				print("GameDetail:loadFavoritedDataAsync() could not fetch data.")
				return
			end

			-- set favorite
			isFavorited = data:GetIsFavoritedByUser()
			toggleFavoriteButton(isFavorited)
		end

		local function loadDescriptionDataAsync()
			local data = getDataAsync()
			if not data then
				print("GameDetail:loadDescriptionDataAsync() could not fetch data.")
				return
			end

			local descriptionData = data:GetDescription()
			local overridesDefaultAvatar = data:GetOverridesDefaultAvatar()
			if overridesDefaultAvatar then
				setOverrideAvatarDisplay()
			end

			DescriptionScrollingTextBox:SetText(descriptionData)
			scrollingTextBoxSelectableChangedCn = DescriptionScrollingTextBox.OnSelectableChaged:connect(function(value)
				setScrollingTextBoxSelections(value)
			end)
		end

		local function loadVoteDataAsync()
			local data = getDataAsync()
			if not data then
				print("GameDetail:loadVoteDataAsync() could not fetch data.")
				return
			end

			VoteView:SetParent(RatingDescriptionContainer)
			VoteView:InitializeAsync(data)
			local defaultSelection = VoteView:GetDefaultSelection()
			if defaultSelection then
				defaultSelection.NextSelectionLeft = FavoriteButton
			end
			FavoriteButton.NextSelectionRight = FavoriteButton.NextSelectionRight or defaultSelection
			FavoriteButton.NextSelectionUp = defaultSelection
			DescriptionScrollingTextBox:GetSelectableObject().NextSelectionUp = defaultSelection
		end

		local loader = LoadingWidget({Parent = RatingDescriptionContainer}, {loadFavoritedDataAsync, loadDescriptionDataAsync, loadVoteDataAsync, waitForTweensToFinish})
		spawn(function()
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil
			RatingDescriptionLineBreak.Visible = true
			DescriptionContainer.Visible = true
			VoteView:SetVisible(true)

			if this then
				this.TransitionTweens = this.TransitionTweens or {}
				concatTables(this.TransitionTweens, ScreenManager:FadeInSitu(RatingDescriptionLineBreak))
				concatTables(this.TransitionTweens, ScreenManager:FadeInSitu(DescriptionContainer))
			end
		end)
	end

	do
		local function loadMoreGameDetailsAsync()
			local data = getDataAsync()
			if not data then
				print("GameDetail:loadMoreGameDetailsAsync() could not fetch data.")
				return
			end

			-- set related games sort data
			local recommendedGames = data:GetRecommendedGamesAsync()
			setRelatedGames(recommendedGames)

			-- set more details
			LastUpdatedText.Text = data:GetLastUpdated()
			CreationDateText.Text = data:GetCreationDate()
			CreatorNameText.Text = data:GetCreatorName()
			MaxPlayersCountText.Text = data:GetMaxPlayers()
		end

		MoreDetailsContent.Visible = false
		local loader = LoadingWidget({Parent = MoreDetailsContainer}, {loadMoreGameDetailsAsync, waitForTweensToFinish})
		spawn(function()
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil
			MoreDetailsContent.Visible = true

			if this then
				this.TransitionTweens = this.TransitionTweens or {}
				concatTables(this.TransitionTweens, ScreenManager:FadeInSitu(MoreDetailsContent))
			end
		end)
	end

	do
		local gameThumbIds = nil
		local badgeData = nil
		local function loadThumbsAsync()
			local data = getDataAsync()
			if not data then
				print("GameDetail:loadThumbsAsync() could not fetch data.")
				return
			end

			gameThumbIds = data:GetThumbnailIdsAsync()
		end
		local function loadBadgeDataAsync()
			local data = getDataAsync()
			if not data then
				print("GameDetail:loadBadgeDataAsync() could not fetch data.")
				return
			end
			-- set badge data
			badgeData = data:GetBadgeDataAsync()
		end

		ThumbsContent.Visible = false
		local loader = LoadingWidget({Parent = ThumbsContainer}, {loadThumbsAsync, loadBadgeDataAsync, waitForTweensToFinish})
		spawn(function()
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil
			ThumbsContent.Visible = true

			-- set additional thumbnails
			setAdditionalThumbs(gameThumbIds)

			-- set badges
			local hasBadges = #badgeData >= 4
			setBadgeContainer(hasBadges, badgeData)

			if this then
				this.TransitionTweens = this.TransitionTweens or {}
				concatTables(this.TransitionTweens, ScreenManager:FadeInSitu(ThumbsContent))
				if BadgeContainer and BadgeContainer.Parent then
					concatTables(this.TransitionTweens, ScreenManager:FadeInSitu(BadgeContainer))
				end
			end
		end)
	end

	--[[ Public API ]]--

	--Override
	function this:GetDefaultSelectionObject()
		return PlayButton
	end

	-- Override
	local baseShow = this.Show
	function this:Show()
		baseShow(self)
		if joinOnShow then
			joinOnShow = false
			JoinGame()
		end
	end

	-- Override
	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(self)
		selectionChangedCn = GuiService.Changed:connect(function(property)
			if property == 'SelectedCoreObject' then
				local newSelectedObject = GuiService.SelectedCoreObject
				if newSelectedObject then
					MyContentManager:TweenContent(newSelectedObject)
				end
			end
		end)
		if PlatformService then
			dataModelViewChangedCn = PlatformService.ViewChanged:connect(function(viewType)
				if viewType == 0 then
					returnedFromGame = false
					wait(1)
					returnedFromGame = true
				end
			end)
		end
		EventHub:addEventListener(EventHub.Notifications["GameJoin"], "gameJoin",
		function(success)
			if success then
				VoteView:SetCanVote(true)
				setVotingSelection()
			end
		end)
	end

	-- Override
	local baseRemoveFocus = this.RemoveFocus
	function this:RemoveFocus()
		baseRemoveFocus(self)
		selectionChangedCn = Utility.DisconnectEvent(selectionChangedCn)
		dataModelViewChangedCn = Utility.DisconnectEvent(dataModelViewChangedCn)
		scrollingTextBoxSelectableChangedCn = Utility.DisconnectEvent(scrollingTextBoxSelectableChangedCn)
		EventHub:removeEventListener(EventHub.Notifications["GameJoin"], "gameJoin")
	end

	return this
end

return CreateGameDetail
