--[[
			// GameGenre.lua
			// Displays a game genre page for a certain sort

			TODO:
				Clean up code
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local ContextActionService = game:GetService("ContextActionService")
local GuiService = game:GetService('GuiService')
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local GameDataModule = require(Modules:FindFirstChild('GameData'))
local SideBarModule = require(Modules:FindFirstChild('SideBar'))
local VoteFrameModule = require(Modules:FindFirstChild('VoteFrame'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local SortCarousel = require(Modules:FindFirstChild('SortCarousel'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GameJoinModule = require(Modules:FindFirstChild('GameJoin'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local GameCollection = require(Modules:FindFirstChild('GameCollection'))

local isUseNewCarouselInXboxAppEnabled = Utility.IsFastFlagEnabled("UseNewCarouselInXboxApp")
local CarouselView = require(Modules:FindFirstChild('CarouselView'))
local CarouselController = require(Modules:FindFirstChild('CarouselController'))

local function CreateGameGenre(sortName, gameCollection)
	local this = {}

	local inFocus = false

	local gameLoadCount = 20
	local sideBarSorts = {} -- array or sort names and ids for the side bar
	local baseButtonTextColor = GlobalSettings.WhiteTextColor
	local selectedButtonTextColor = GlobalSettings.TextSelectedColor

	local newGameSelectedCn = nil
	local sideBarSelectedCn = nil
	local dataModelViewChangedCn = nil
	local canJoinGame = true
	local returnedFromGame = true

	local currentShownCollection = gameCollection
	--[[ Top Level Elements ]]--
	local GameGenreContainer = Utility.Create'Frame'
	{
		Name = "GameGenreContainer";
		Size = UDim2.new(1, 0, 1, 0);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		Visible = false;
		Parent = parent;
	}
	local BackLabel = Utility.Create'ImageLabel'
	{
		Name = "BackLabel";
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		ZIndex = 2;
		Parent = GameGenreContainer;
	}
	AssetManager.LocalImage(BackLabel,
		'rbxasset://textures/ui/Shell/Icons/BackIcon', {['720'] = UDim2.new(0,32,0,32); ['1080'] = UDim2.new(0,48,0,48);})

	local BackText = Utility.Create'TextLabel'
	{
		Name = "BackText";
		Size = UDim2.new(0, 0, 0, BackLabel.Size.Y.Offset);
		Position = UDim2.new(0, BackLabel.Size.X.Offset + 8, 0, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = '';
		Parent = GameGenreContainer;
	}
	local SideBarButton = Utility.Create'ImageButton'
	{
		Name = "SideBarButton";
		Size = UDim2.new(0, 450, 0, 75);
		Position = UDim2.new(0, 0, 0, BackLabel.Size.Y.Offset + 60);
		BackgroundTransparency = 1;
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.GreySelectionColor;
		Parent = GameGenreContainer;

		SoundManager:CreateSound('MoveSelection');
	}
	SideBarButton.NextSelectionRight = SideBarButton
	SideBarButton.NextSelectionLeft = SideBarButton
	SideBarButton.SelectionGained:connect(function()
		Utility.PropertyTweener(SideBarButton, 'BackgroundTransparency', 0, 0, 0, nil, true)
	end)
	SideBarButton.SelectionLost:connect(function()
		Utility.PropertyTweener(SideBarButton, 'BackgroundTransparency', 1, 1, 0, nil, true)
	end)

	local DropDownImage = Utility.Create'ImageLabel'
	{
		Name = "DropDownImage";
		BackgroundTransparency = 1;
		Parent = SideBarButton;
	}
	AssetManager.LocalImage(DropDownImage,
		'rbxasset://textures/ui/Shell/Icons/Dropdown02', {['720'] = UDim2.new(0,29,0,29); ['1080'] = UDim2.new(0,44,0,44);})
	DropDownImage.Position = UDim2.new(1, -DropDownImage.Size.X.Offset - 12, 0.5, -DropDownImage.Size.Y.Offset / 2 + 4)

	local TitleLabel = Utility.Create'TextLabel'
	{
		Name = "TitleLabel";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, 18, 0.5, 0);
		BackgroundTransparency = 1;
		Text = string.upper(sortName);
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.HeaderSize;
		ZIndex = 2;
		Parent = SideBarButton;
	}
	local GameTitleLabel = Utility.Create'TextLabel'
	{
		Name = "GameTitleLabel";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, 18, 0, SideBarButton.Position.Y.Offset + SideBarButton.Size.Y.Offset + 550);
		BackgroundTransparency = 1;
		Text = "";
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.HeaderSize;
		Parent = GameGenreContainer;
	}
	local ThumbsUpImage = Utility.Create'ImageLabel'
	{
		Name = "ThumbsUpImage";
		BackgroundTransparency = 1;
		ZIndex = 2;
		Visible = false;
		Parent = GameGenreContainer;
	}
	local ThumbsDownImage = Utility.Create'ImageLabel'
	{
		Name = "ThumbsDownImage";
		BackgroundTransparency = 1;
		ZIndex = 2;
		Visible = false;
		Parent = GameGenreContainer;
	}
	AssetManager.LocalImage(ThumbsUpImage,
		'rbxasset://textures/ui/Shell/Icons/ThumbsUpIcon', {['720'] = UDim2.new(0,19,0,19); ['1080'] = UDim2.new(0,28,0,28);})
	AssetManager.LocalImage(ThumbsDownImage,
		'rbxasset://textures/ui/Shell/Icons/ThumbsDownIcon', {['720'] = UDim2.new(0,19,0,19); ['1080'] = UDim2.new(0,28,0,28);})

	local VoteWidget = VoteFrameModule(GameGenreContainer,
		UDim2.new(0, 60, 0, GameTitleLabel.Position.Y.Offset + 46))
	local VoteContainer = VoteWidget:GetContainer()
	ThumbsUpImage.Position = UDim2.new(0, VoteContainer.Position.X.Offset - ThumbsUpImage.Size.X.Offset - 10, 0,
		VoteContainer.Position.Y.Offset + VoteContainer.Size.Y.Offset - ThumbsUpImage.Size.Y.Offset)
	ThumbsDownImage.Position = UDim2.new(0, VoteContainer.Position.X.Offset + VoteContainer.Size.X.Offset + 10, 0,
		VoteContainer.Position.Y.Offset)

	local SeparatorDot = Utility.Create'ImageLabel'
	{
		Name = "SeparatorDot";
		BackgroundTransparency = 1;
		Visible = false;
		Parent = GameGenreContainer;
	}
	AssetManager.LocalImage(SeparatorDot,
		'rbxasset://textures/ui/Shell/Icons/SeparatorDot', {['720'] = UDim2.new(0,7,0,7); ['1080'] = UDim2.new(0,10,0,10);})
	SeparatorDot.Position = UDim2.new(0, ThumbsDownImage.Position.X.Offset + ThumbsDownImage.Size.X.Offset + 32, 0,
		VoteContainer.Position.Y.Offset + (VoteContainer.Size.Y.Offset/2) - (SeparatorDot.Size.Y.Offset/2))

	local CreatorIcon = Utility.Create'ImageLabel'
	{
		Name = "CreatorIcon";
		Size = UDim2.new(0, 24, 0, 24);
		Position = UDim2.new(0, SeparatorDot.Position.X.Offset + SeparatorDot.Size.X.Offset + 32, 0,
			SeparatorDot.Position.Y.Offset + SeparatorDot.Size.Y.Offset/2 - 2 - 10);
		BackgroundTransparency = 1;
		Image = 'rbxasset://textures/ui/Shell/Icons/RobloxIcon24.png';
		Visible = false;
		Parent = GameGenreContainer;
	}

	local CreatorNameLabel = Utility. Create'TextLabel'
	{
		Name = "CreatorNameLabel";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, CreatorIcon.Position.X.Offset + CreatorIcon.Size.X.Offset + 8, 0,
			SeparatorDot.Position.Y.Offset + SeparatorDot.Size.Y.Offset/2 - 2);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.DescriptionSize;
		TextColor3 = GlobalSettings.LightGreyTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		Text = "";
		Parent = GameGenreContainer;
	}

	local DescriptionTextLabel = Utility.Create'TextLabel'
	{
		Name = "DescriptionTextLabel";
		Size = UDim2.new(0, 850, 0, 56);
		Position = UDim2.new(0, GameTitleLabel.Position.X.Offset, 0, VoteContainer.Position.Y.Offset + VoteContainer.Size.Y.Offset + 20);
		BackgroundTransparency = 1;
		Text = "";
		TextColor3 = GlobalSettings.LightGreyTextColor;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		Font = GlobalSettings.LightFont;
		TextWrapped = true;
		FontSize = GlobalSettings.DescriptionSize;
		Parent = GameGenreContainer;
	}
	local PlayButton = Utility.Create'ImageButton'
	{
		Name = "PlayButton";
		Position = UDim2.new(0, 0, 1, -77);
		Size = UDim2.new(0, 228, 0, 72);
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.GreenButtonColor;
		Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
		Parent = GameGenreContainer;

		SoundManager:CreateSound('MoveSelection');
		ZIndex = 2;
		AssetManager.CreateShadow(1);
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
		ZIndex = 2;
		Parent = PlayButton;
	}
	local FavoriteButton = Utility.Create'ImageButton'
	{
		Name = "FavoriteButton";
		Position = UDim2.new(0, PlayButton.Size.X.Offset + 10, 1, -77);
		Size = UDim2.new(0, 228, 0, 72);
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.GreyButtonColor;
		Image = 'rbxasset://textures/ui/Shell/Buttons/Generic9ScaleButton@720.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(Vector2.new(4, 4), Vector2.new(28, 28));
		ZIndex = 2;
		Parent = GameGenreContainer;

		SoundManager:CreateSound('MoveSelection');
		AssetManager.CreateShadow(1);
	}

	FavoriteButton.NextSelectionRight = FavoriteButton
	local FavoriteText = Utility.Create'TextLabel'
	{
		Name = "FavoriteText";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Text = string.upper(Strings:LocalizedString("FavoriteWord"));
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = baseButtonTextColor;
		ZIndex = 2;
		Parent = FavoriteButton;
	}
	local FavoriteStarImage = Utility.Create'ImageLabel'
	{
		Name = "FavoriteStarImage";
		BackgroundTransparency = 1;
		Visible = false;
		ZIndex = 2;
		Parent = FavoriteButton;
	}
	AssetManager.LocalImage(FavoriteStarImage,
		'rbxasset://textures/ui/Shell/Icons/FavoriteStar', {['720'] = UDim2.new(0,21,0,21); ['1080'] = UDim2.new(0,32,0,31);})
	FavoriteStarImage.Position = UDim2.new(0, 16, 0.5, -FavoriteStarImage.Size.Y.Offset / 2)

	-- Selection Overrides
	-- NOTE: This is a fix to prevent unintended selection in the carousel do to the nature of
	-- how selection works for a ScrollingFrame.
	PlayButton.NextSelectionLeft = PlayButton
	FavoriteButton.NextSelectionRight = FavoriteButton
	SideBarButton.NextSelectionLeft = SideBarButton
	SideBarButton.NextSelectionRight = SideBarButton

	--[[ Page Events ]]--
	local function toggleFavoriteButton(isFavorited)
		if isFavorited == true then
			FavoriteStarImage.Visible = true
			FavoriteText.Position = UDim2.new(0, FavoriteStarImage.Position.X.Offset + FavoriteStarImage.Size.X.Offset + 12, 0, 0)
			FavoriteText.Text = string.upper(Strings:LocalizedString("FavoritedWord"))
			FavoriteText.TextXAlignment = Enum.TextXAlignment.Left
		elseif isFavorited == false then
			FavoriteStarImage.Visible = false
			FavoriteText.Position = UDim2.new(0, 0, 0, 0)
			FavoriteText.Text = string.upper(Strings:LocalizedString("FavoriteWord"))
			FavoriteText.TextXAlignment = Enum.TextXAlignment.Center
		end
	end
	FavoriteButton.SelectionGained:connect(function()
		FavoriteButton.ImageColor3 = GlobalSettings.GreySelectedButtonColor
		FavoriteText.TextColor3 = selectedButtonTextColor
	end)
	FavoriteButton.SelectionLost:connect(function()
		FavoriteButton.ImageColor3 = GlobalSettings.GreyButtonColor
		FavoriteText.TextColor3 = baseButtonTextColor
	end)
	PlayButton.SelectionGained:connect(function()
		PlayButton.ImageColor3 = GlobalSettings.GreenSelectedButtonColor
		PlayText.TextColor3 = selectedButtonTextColor
	end)
	PlayButton.SelectionLost:connect(function()
		PlayButton.ImageColor3 = GlobalSettings.GreenButtonColor
		PlayText.TextColor3 = baseButtonTextColor
	end)

	--[[ Content Initialization ]]--
	local function setItemsVisible(value)
		ThumbsUpImage.Visible = value
		ThumbsDownImage.Visible = value
		VoteWidget:SetVisible(value)
		GameTitleLabel.Visible = value
		DescriptionTextLabel.Visible = value
		PlayButton.Visible = value
		FavoriteButton.Visible = value
		SideBarButton.Visible = value
		--
		SeparatorDot.Visible = value
		CreatorIcon.Visible = value
		CreatorNameLabel.Visible = value
	end

	local currentSortCarousel = nil
	local currentItemData = nil
	local onNewGameSelectedCn = nil
	local onNewGameSelectedLateCn = nil
	local function setCurrentSortCarousel()
		setItemsVisible(false)
		toggleFavoriteButton(false)
		currentSortCarousel = SortCarousel(UDim2.new(1, 0, 0, 450),
			UDim2.new(0, 0, 0, SideBarButton.Position.Y.Offset + SideBarButton.Size.Y.Offset + 56), currentShownCollection, GameGenreContainer)
		onNewGameSelectedCn = currentSortCarousel.OnNewGameSelected:connect(function(itemData)
			if itemData then
				currentItemData = itemData
				GameTitleLabel.Text = itemData.Name
				DescriptionTextLabel.Text = itemData.Description or ""
				CreatorNameLabel.Text = itemData.CreatorName
				toggleFavoriteButton(itemData.IsFavorited)
				local voteData = itemData.VoteData
				if voteData then
					local upvotes = voteData.UpVotes
					local downvotes = voteData.DownVotes
					if upvotes == 0 and downvotes == 0 then
						VoteWidget:SetPercentFilled(nil)
					else
						VoteWidget:SetPercentFilled(upvotes / (upvotes + downvotes))
					end
				end
				setItemsVisible(true)
			end
		end)
		onNewGameSelectedLateCn = currentSortCarousel.OnNewGameSelectedLate:connect(function(description, isFavorited)
			toggleFavoriteButton(isFavorited)
			DescriptionTextLabel.Text = description or ""
		end)

		spawn(function()
			currentSortCarousel:LoadSortAsync()
			if inFocus then
				local focusItem = currentSortCarousel:GetItemAt(1)
				if focusItem and focusItem:IsDescendantOf(GameGenreContainer) then
					GuiService.SelectedCoreObject = focusItem
				end
			end
		end)
	end

	local function onNewGameSelected(data)
		if not data then return end
		-- TODO: Update this function when caching is finished
		GameTitleLabel.Text = data.Title
		CreatorNameLabel.Text = data.CreatorName

		local voteData = data.VoteData
		if voteData then
			local upVotes = voteData.UpVotes
			local downVotes = voteData.DownVotes
			if upVotes == 0 and downVotes == 0 then
				VoteWidget:SetPercentFilled(nil)
			else
				VoteWidget:SetPercentFilled(upVotes / (upVotes + downVotes))
			end
		end

		DescriptionTextLabel.Text = data.Description or ""
		toggleFavoriteButton(data.IsFavorited)

		if not data.Description or data.IsFavorited == nil then
			spawn(function()
				local gameData = GameDataModule:GetGameDataAsync(data.PlaceId)
				if gameData then
					data.GameData = gameData
					data.Description = gameData:GetDescription()
					data.IsFavorited = gameData:GetIsFavoritedByUser()

					DescriptionTextLabel.Text = data.Description
					toggleFavoriteButton(data.IsFavorited)
				end
			end)
		end

		setItemsVisible(true)
	end

	local myCarouselView = nil
	local myCarouselController = nil
	if isUseNewCarouselInXboxAppEnabled then
		myCarouselView = CarouselView()
		myCarouselView:SetSize(UDim2.new(0, 1724, 0, 450))
		myCarouselView:SetPosition(UDim2.new(0, 0, 0, SideBarButton.Position.Y.Offset + SideBarButton.Size.Y.Offset + 56))
		myCarouselView:SetPadding(18)
		myCarouselView:SetItemSizePercentOfContainer(2/3)
		myCarouselView:SetParent(GameGenreContainer)

		myCarouselController = CarouselController(myCarouselView)
	end

	local function initializeCarouselView()
		setItemsVisible(false)
		toggleFavoriteButton(false)
		myCarouselView:SetParent(nil)

		spawn(function()
			local loader = LoadingWidget(
				{ Parent = GameGenreContainer }, {
				function()
					if myCarouselController then
						myCarouselController:InitializeAsync(currentShownCollection)
					end
				end
			})
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil

			myCarouselView:SetParent(GameGenreContainer)
			if this:IsFocused() and myCarouselView then
				GuiService.SelectedCoreObject = myCarouselView:GetFocusItem()
			end
		end)
	end

	local function canShowFavoritesAsync(collection)
		local favoritesPage = collection:GetSortAsync(0, 1)
		return favoritesPage and favoritesPage.Count > 0
	end
	local function canShowRecentAsync(collection)
		local recentPage = collection:GetSortAsync(0, 1)
		return recentPage and recentPage.Count > 0
	end
	local function canShowMyPlacesAsync(collection)
		local userPlacesPage = collection:GetSortAsync(0, 1)
		return userPlacesPage and userPlacesPage.Count > 0
	end
	local function getSideBarList()
		local sideBarList = {}
		local favoriteCollection = GameCollection:GetUserFavorites()
		if canShowFavoritesAsync(favoriteCollection) then
			table.insert(sideBarList,
				{ Name = string.upper(Strings:LocalizedString("FavoritesSortTitle")), Collection = favoriteCollection })
		end
		local recentCollection = GameCollection:GetUserRecent()
		if canShowRecentAsync(recentCollection) then
			table.insert(sideBarList,
				{ Name = string.upper(Strings:LocalizedString("RecentlyPlayedSortTitle")), Collection = recentCollection })
		end
		table.insert(sideBarList, { Name = string.upper(Strings:LocalizedString("FeaturedTitle")), Collection = GameCollection:GetSort(GameCollection.DefaultSortId.Featured) })
		table.insert(sideBarList, { Name = string.upper(Strings:LocalizedString("PopularTitle")), Collection = GameCollection:GetSort(GameCollection.DefaultSortId.Popular) })
		table.insert(sideBarList, { Name = string.upper(Strings:LocalizedString("TopRatedTitle")), Collection = GameCollection:GetSort(GameCollection.DefaultSortId.TopRated) })
		table.insert(sideBarList, { Name = string.upper(Strings:LocalizedString("TopEarningTitle")), Collection = GameCollection:GetSort(GameCollection.DefaultSortId.TopEarning) })

		local userPlacesCollection = GameCollection:GetUserPlaces()
		if canShowMyPlacesAsync(userPlacesCollection) then
			table.insert(sideBarList,
				{ Name = string.upper(Strings:LocalizedString("PlayMyPlaceMoreGamesTitle")), Collection = userPlacesCollection })
		end

		return sideBarList
	end

	local function createSideBarAsync()
		local sideBar = SideBarModule()
		local sideBarList = getSideBarList()
		local collectionToIndex = {}

		for i = 1, #sideBarList do
			local sort = sideBarList[i]
			collectionToIndex[sort.Collection] = i
			sideBar:AddItem(sort.Name, function()
				if sort.Collection ~= currentShownCollection then
					currentShownCollection = sort.Collection
					TitleLabel.Text = sort.Name

					if isUseNewCarouselInXboxAppEnabled then
						initializeCarouselView()
					else
						currentSortCarousel:Destroy()
						setCurrentSortCarousel()
					end
					if this.TransitionTweens then
						ScreenManager:DefaultCancelFade(this.TransitionTweens)
						this.TransitionTweens = ScreenManager:DefaultFadeIn(GameGenreContainer)
						ScreenManager:PlayDefaultOpenSound()
					end
				end
			end)
		end

		sideBarSelectedCn = Utility.DisconnectEvent(sideBarSelectedCn)
		sideBarSelectedCn = SideBarButton.MouseButton1Click:connect(function()
			sideBar:SetSelectedObject(collectionToIndex[currentShownCollection])
			ScreenManager:OpenScreen(sideBar, false)
		end)
	end

	--[[ Input Events ]]--
	PlayButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		local gameData = nil
		if isUseNewCarouselInXboxAppEnabled then
			if myCarouselController then
				gameData = myCarouselController:GetCurrentFocusGameData()
			end
		else
			gameData = currentSortCarousel:GetCurrentSelectedGameData()
		end

		if gameData then
			local placeId = gameData.PlaceId
			local creatorUserId = gameData.CreatorUserId
			if canJoinGame and returnedFromGame then
				canJoinGame = false
				GameJoinModule:StartGame(GameJoinModule.JoinType.Normal, placeId, creatorUserId)
				canJoinGame = true
			end
		end
	end)

	FavoriteButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		local gameData = nil
		if isUseNewCarouselInXboxAppEnabled then
			if myCarouselController then
				gameData = myCarouselController:GetCurrentFocusGameData()
			end
		else
			gameData = currentItemData
		end

		if gameData and gameData.GameData then
			local success, reason = gameData.GameData:PostFavoriteAsync()
			if success then
				gameData.IsFavorited = gameData.GameData:GetIsFavoritedByUser()
				toggleFavoriteButton(gameData.IsFavorited)
			elseif reason then
				local err = Errors.Favorite[reason]
				ScreenManager:OpenScreen(ErrorOverlayModule(err), false)
			end
		end
	end)

	--[[ Public API ]]--
	function this:SetPosition(newPosition)
		GameGenreContainer.Position = newPosition
	end

	function this:SetParent(newParent)
		GameGenreContainer.Parent = newParent
	end

	function this:GetName()
		return TitleLabel.Text
	end

	function this:Show()
		GameGenreContainer.Visible = true

		local prevScreen = ScreenManager:GetScreenBelow(self)
		if prevScreen and prevScreen.GetName then
			BackText.Text = prevScreen:GetName()
		else
			BackText.Text = ''
		end

		ScreenManager:DefaultCancelFade(this.TransitionTweens)
		self.TransitionTweens = ScreenManager:DefaultFadeIn(GameGenreContainer)
		ScreenManager:PlayDefaultOpenSound()
	end

	function this:Hide()
		GameGenreContainer.Visible = false
		ScreenManager:DefaultCancelFade(self.TransitionTweens)
		self.TransitionTweens = nil
	end

	function this:Destroy()
		if onNewGameSelectedCn then
			onNewGameSelectedCn:disconnect()
			onNewGameSelectedCn = nil
		end
		if not isUseNewCarouselInXboxAppEnabled then
			currentSortCarousel:Destroy()
		end
		VoteWidget:Destroy()
		GameGenreContainer:Destroy()
	end

	function this:IsFocused()
		return inFocus
	end

	function this:Focus()
		inFocus = true
		if isUseNewCarouselInXboxAppEnabled then
			local selection = myCarouselView:GetFocusItem() or SideBarButton
			GuiService.SelectedCoreObject = selection
		else
			if self.SavedSelectedObject and self.SavedSelectedObject:IsDescendantOf(GameGenreContainer) then
				GuiService.SelectedCoreObject = self.SavedSelectedObject
			end
		end

		ContextActionService:BindCoreAction("ReturnFromGameGenreScreen",
			function(actionName, inputState, inputObject)
				if inputState == Enum.UserInputState.End then
					ScreenManager:CloseCurrent()
					self:Destroy()
				end
			end,
			false,
			Enum.KeyCode.ButtonB)

		if PlatformService then
			dataModelViewChangedCn = PlatformService.ViewChanged:connect(function(viewType)
				if viewType == 0 then
					returnedFromGame = false
					wait(1)
					returnedFromGame = true
				end
			end)
		end
		if isUseNewCarouselInXboxAppEnabled then
			if myCarouselController then
				newGameSelectedCn = myCarouselController.NewItemSelected:connect(onNewGameSelected)
				onNewGameSelected(myCarouselController:GetCurrentFocusGameData())
				myCarouselController:Connect()
			end
		end
		spawn(function()
			createSideBarAsync()
		end)
	end

	function this:RemoveFocus()
		inFocus = false
		local selectedObject = GuiService.SelectedCoreObject
		if selectedObject and selectedObject:IsDescendantOf(GameGenreContainer) then
			self.SavedSelectedObject = selectedObject
			GuiService.SelectedCoreObject = nil
		end
		ContextActionService:UnbindCoreAction("ReturnFromGameGenreScreen")
		dataModelViewChangedCn = Utility.DisconnectEvent(dataModelViewChangedCn)
		newGameSelectedCn = Utility.DisconnectEvent(newGameSelectedCn)
		sideBarSelectedCn = Utility.DisconnectEvent(sideBarSelectedCn)
		if isUseNewCarouselInXboxAppEnabled then
			if myCarouselController then
				myCarouselController:Disconnect()
			end
		end
	end

	if isUseNewCarouselInXboxAppEnabled then
		initializeCarouselView()
	else
		setCurrentSortCarousel()
	end

	return this
end

return CreateGameGenre
