-- Written by Kip Turner, Copyright ROBLOX 2015

local CoreGui = Game:GetService("CoreGui")
local GuiService = game:GetService('GuiService')
local PlayersService = game:GetService("Players")
local RunService = game:GetService("RunService")
local ContextActionService = game:GetService('ContextActionService')

local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local UserData = require(Modules:FindFirstChild('UserData'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local FriendsData = require(Modules:FindFirstChild('FriendsData'))
local FriendsView = require(Modules:FindFirstChild('FriendsView'))
local ScrollingGridModule = require(Modules:FindFirstChild('ScrollingGrid'))
local GameSort = require(Modules:FindFirstChild('GameSort'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local GameCollection = require(Modules:FindFirstChild("GameCollection"))

local MOCKUP_SIZE = Vector2.new(1920, 1080)
local PROFILE_SIZE = Vector2.new(450, 343)
local PROFILE_NAME_SIZE = Vector2.new(450, 38)
local PROFILE_BUTTON_SIZE = Vector2.new(450, 300)
local PROFILE_IMAGE_SIZE = Vector2.new(450, 300)
local PROFILE_AVATAR_BRUSH_SIZE = Vector2.new(301, 149)
local SORTS_SIZE = Vector2.new(1234, 608)


local function CreateHomePane(parent)
	local this = {}

	local inFocus = false

	local sortsObjects = {}


	local HomePaneContainer = Utility.Create'Frame'
	{
		Name = 'HomePane';
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		Parent = parent;
	}

	local ProfileContainer = Utility.Create'Frame'
	{
		Name = 'ProfileContainer';
		Position = UDim2.new(0,0,0,0);
		BackgroundTransparency = 1;
		Parent = HomePaneContainer;
	}

		local NameLabel = Utility.Create'TextLabel'
		{
			Name = 'NameLabel';
			Text = '';
			TextXAlignment = 'Left';
			TextYAlignment = 'Top';
			-- TextScaled = true;
			TextColor3 = GlobalSettings.WhiteTextColor;
			Font = GlobalSettings.RegularFont;
			FontSize = GlobalSettings.SubHeaderSize;
			BackgroundTransparency = 1;
			Parent = ProfileContainer;
		};
		local ProfileButton = Utility.Create'ImageButton'
		{
			Name = 'ProfileButton';
			AutoButtonColor = false;
			BackgroundTransparency = 1;
			Parent = ProfileContainer;

			SoundManager:CreateSound('MoveSelection');
		}
			local ProfileImage = Utility.Create'ImageLabel'
			{
				Name = 'ProfileImage';
				Position = UDim2.new(0,0,0,0);
				BackgroundTransparency = 1;
				BorderSizePixel = 0;
				ZIndex = 3;
				Parent = ProfileButton;
			};
				local ProfileImageBackground = Utility.Create'Frame'
				{
					Name = 'ProfileImageBackground';
					Size = UDim2.new(1,0,1,0);
					BorderSizePixel = 0;
					BackgroundTransparency = 0;
					BackgroundColor3 = GlobalSettings.CharacterBackgroundColor;
					ZIndex = 2;
					Parent = ProfileImage;
					AssetManager.CreateShadow(1);
				}
			local AvatarBrushBackground = Utility.Create'Frame'
			{
				Name = 'AvatarBrushImage';
				BorderSizePixel = 0;
				BackgroundColor3 = GlobalSettings.AvatarBoxBackgroundColor;
				BackgroundTransparency = GlobalSettings.AvatarBoxBackgroundDeselectedTransparency;
				ZIndex = 2;
				-- Parent = ProfileButton;
			}
				local AvatarBrushImage = Utility.Create'ImageLabel'
				{
					Name = 'AvatarBrushImage';
					BorderSizePixel = 0;
					BackgroundTransparency = 1;
					ZIndex = 2;
					Parent = AvatarBrushBackground;
				};
				AssetManager.LocalImage(AvatarBrushImage,
					'rbxasset://textures/ui/Shell/Icons/CustomizeIcon', {['720'] = UDim2.new(0,31,0,32); ['1080'] = UDim2.new(0,47,0,48);})
				local AvatarTextLabel = Utility.Create'TextLabel'
				{
					Name = 'AvatarTextLabel';
					Text = Strings:LocalizedString('EditAvatarPhrase');
					TextColor3 = GlobalSettings.WhiteTextColor;
					Font = GlobalSettings.RegularFont;
					FontSize = GlobalSettings.ButtonSize;
					BackgroundTransparency = 1;
					ZIndex = 2;
					Parent = AvatarBrushBackground;
				};

	ProfileButton.MouseButton1Click:connect(function()
		SoundManager:Play('ButtonPress')
		EventHub:dispatchEvent(EventHub.Notifications["NavigateToEquippedAvatar"])
		-- EventHub:dispatchEvent(EventHub.Notifications["OpenProfileDetail"], "AppHub");
	end)

	local function OnProfileButtonSelectionChanged()
		local selectedObject = GuiService.SelectedCoreObject
		local newBackgroundTransparency = selectedObject == ProfileButton and
			GlobalSettings.AvatarBoxBackgroundSelectedTransparency or
			GlobalSettings.AvatarBoxBackgroundDeselectedTransparency
		local newTextTransparency = selectedObject == ProfileButton and
			GlobalSettings.AvatarBoxTextSelectedTransparency or
			GlobalSettings.AvatarBoxTextDeselectedTransparency

		Utility.PropertyTweener(AvatarBrushBackground, 'BackgroundTransparency', newBackgroundTransparency, newBackgroundTransparency, 0, nil, true)
		Utility.PropertyTweener(AvatarTextLabel, 'TextTransparency', newTextTransparency, newTextTransparency, 0, nil, true)
		Utility.PropertyTweener(AvatarBrushImage, 'ImageTransparency', newTextTransparency, newTextTransparency, 0, nil, true)
	end

	local existingThumbnailLoader = nil
	local function UpdateProfileInfo()
		local playerName = UserData:GetDisplayName()
		NameLabel.Text = playerName and playerName:upper() or ''
		-- ProfileImage.Image = UserData:GetAvatarUrl(420, 420)..'&cb='..tostring(tick())

		local rbxuid = UserData:GetRbxUserId()

		if rbxuid then
			if existingThumbnailLoader then
				existingThumbnailLoader:Cancel()
			end
			local thumbnailSize = ThumbnailLoader.Sizes.Medium
			local thumbLoader = ThumbnailLoader:Create(ProfileImage, rbxuid,
				thumbnailSize, ThumbnailLoader.AssetType.Avatar, true)
			existingThumbnailLoader = thumbLoader
			spawn(function()
				thumbLoader:LoadAsync()
				-- ProfileImage.ImageRectOffset = Vector2.new(thumbnailSize.X, 0)
				-- ProfileImage.ImageRectSize = Vector2.new(-thumbnailSize.X, (PROFILE_IMAGE_SIZE.Y/PROFILE_IMAGE_SIZE.X) * thumbnailSize.X)
				ProfileImage.ImageRectSize = Vector2.new(thumbnailSize.X, (PROFILE_IMAGE_SIZE.Y/PROFILE_IMAGE_SIZE.X) * thumbnailSize.X)
			end)
		end

		-- local thumbLoader = ThumbnailLoader:Create(ProfileImage, data.AssetId, ThumbnailLoader.Sizes.Medium)
		-- spawn(function()
		-- 	thumbLoader:LoadAsync()
		-- end)
	end
	UpdateProfileInfo()

	local FriendActivityContainer = Utility.Create'Frame'
	{
		Name = 'FriendActivityContainer';
		BackgroundTransparency = 1;
		Parent = HomePaneContainer;
	}
		local FriendActivityTitle = Utility.Create'TextLabel'
		{
			Name = 'FriendActivityTitle';
			Text = Strings:LocalizedString('FriendActivityWord'):upper();
			Size = UDim2.new(1,0,0,50);
			Position = UDim2.new(0,0,0,10);
			TextXAlignment = 'Left';
			TextColor3 = GlobalSettings.WhiteTextColor;
			Font = GlobalSettings.RegularFont;
			FontSize = GlobalSettings.SubHeaderSize;
			BackgroundTransparency = 1;
			Parent = FriendActivityContainer;
		};

		local FriendsStatusMessage = Utility.Create'TextLabel'
		{
			Name = 'FriendsStatusMessage';
			Text = Strings:LocalizedString('NoFriendsOnlinePhrase');
			Size = UDim2.new(0.9,0,1,-125);
			Position = UDim2.new(0.05, 0, 0, 125);
			TextYAlignment = 'Top';
			TextColor3 = GlobalSettings.GreyTextColor;
			TextWrapped = true;
			TextTransparency = GlobalSettings.FriendStatusTextTransparency;
			Font = GlobalSettings.BoldFont;
			FontSize = GlobalSettings.DescriptionSize;
			BackgroundTransparency = 1;
			Visible = false;
			Parent = FriendActivityContainer;
		};

	local friendsScroller = ScrollingGridModule()
	friendsScroller:SetSize(UDim2.new(1,0,1,-60))
	friendsScroller:SetRowColumnConstraint(1)
	friendsScroller:SetScrollDirection(friendsScroller.Enum.ScrollDirection.Vertical)
	friendsScroller:SetCellSize(Vector2.new(446,114))
	friendsScroller:SetSpacing(Vector2.new(0, 26))
	friendsScroller:SetPosition(UDim2.new(0,0,0,FriendActivityTitle.Position.Y.Offset + FriendActivityTitle.Size.Y.Offset))
	local friendScrollerContainer = friendsScroller:GetGuiObject()
	friendScrollerContainer.Visible = false
	friendsScroller:SetParent(FriendActivityContainer)

	local function setFriendItems()
		local function onFriendsUpdated(friendCount)
			FriendsStatusMessage.Visible = friendCount < 1
		end

		local friendsData = FriendsData.GetOnlineFriendsAsync()
		local myFriendsView = FriendsView(friendsScroller, friendsData, nil, onFriendsUpdated)
		onFriendsUpdated(#friendsData)
	end

	local friendsLoader = LoadingWidget(
		{ Parent = FriendActivityContainer }, { setFriendItems } )

	-- Don't Block
	spawn(function()
		friendsLoader:AwaitFinished()
		friendsLoader:Cleanup()
		friendScrollerContainer.Visible = true
	end)

	local SortsContainer = Utility.Create'Frame'
	{
		Name = 'SortsContainer';
		BackgroundTransparency = 1;
		Parent = HomePaneContainer;
	}

	local populateSortsDebounce = false
	local function PopulateSorts()
		if populateSortsDebounce then return end
		populateSortsDebounce = true
		local function loadGameSorts()
			while #sortsObjects > 0 do
				local sortObject = table.remove(sortsObjects)
				if sortObject then
					if this.SavedSelectedObject and this.SavedSelectedObject:IsDescendantOf(sortObject:GetContainer()) then
						this.SavedSelectedObject = nil
					end
					if GuiService.SelectedCoreObject and GuiService.SelectedCoreObject:IsDescendantOf(sortObject:GetContainer()) then
						if inFocus then
							GuiService.SelectedCoreObject = this:GetDefaultSelectionObject()
						end
					end
					sortObject:Destroy()
				end
			end

			local favoriteGamesCollection = GameCollection:GetUserFavorites()
			local favoritesPage1 = favoriteGamesCollection:GetSortAsync(0, 4)

			local recentGamesCollection = GameCollection:GetUserRecent()
			local recentlyPage1 = recentGamesCollection:GetSortAsync(0, 4)

			local showRecent = recentlyPage1 and #recentlyPage1:GetPagePlaceIds() > 2
			local showFavorites = favoritesPage1 and #favoritesPage1:GetPagePlaceIds() > 2

			local function setGridView(view, page, title, collection)
				if page then
					local names = page:GetPagePlaceNames()
					local placeIds = page:GetPagePlaceIds()
					local iconIds = page:GetPageIconIds()
					view:SetImages(iconIds)
					view:ConnectInput(placeIds, names, iconIds, title, collection)
				end
				view:SetTitle(title)
				view:SetVisible(false)
				view:SetParent(SortsContainer)
				table.insert(sortsObjects, view)
			end

			local function createMainSortGrid(title, page, collection)
				local view = GameSort:CreateMainGridView(UDim2.new(0, 378, 1, 0), Vector2.new(10, 10),
					UDim2.new(1, 0, 0, 378), UDim2.new(0, 184, 0, 184))
				setGridView(view, page, title, collection)

				return view
			end
			local function createGridSortView(size, imageSize, spacing, rows, columns, page, title, collection)
				local view = GameSort:CreateGridView(size, imageSize, spacing, rows, columns)
				setGridView(view, page, title, collection)

				return view
			end

			local currentPosition = UDim2.new(0, 0, 0, 0)
			local margin = (showFavorites and showRecent) and 50 or 28

			if showFavorites then
				local view = createMainSortGrid(Strings:LocalizedString('FavoritesSortTitle'), favoritesPage1, favoriteGamesCollection)
				view:SetPosition(currentPosition)
				currentPosition = currentPosition + UDim2.new(0, view:GetContainer().Size.X.Offset + margin, 0, 0)
			end
			if showRecent then
				local view = createMainSortGrid(Strings:LocalizedString('RecentlyPlayedSortTitle'), recentlyPage1, recentGamesCollection)
				view:SetPosition(currentPosition)
				currentPosition = currentPosition + UDim2.new(0, view:GetContainer().Size.X.Offset + margin, 0, 0)
			end

			local featuredCollection = GameCollection:GetSort(GameCollection.DefaultSortId.Featured)
			local featuredTitle = Strings:LocalizedString('FeaturedTitle')
			local recommendedView = nil
			if showRecent and showFavorites then
				local page = featuredCollection:GetSortAsync(0, 4)
				recommendedView = createMainSortGrid(featuredTitle, page, featuredCollection)
			elseif showRecent or showFavorites then
				local page = featuredCollection:GetSortAsync(0, 7)
				recommendedView = createGridSortView(UDim2.new(0, 858, 1, 0), UDim2.new(0, 276, 0, 276), Vector2.new(15, 20),
					2, 3, page, featuredTitle, featuredCollection)
			else
				local page = featuredCollection:GetSortAsync(0, 9)
				recommendedView = createGridSortView(UDim2.new(0, 1236, 0, 648), UDim2.new(0, 300, 0, 300), Vector2.new(12, 12),
					2, 4, page, featuredTitle, featuredCollection)
			end
			recommendedView:SetPosition(currentPosition)
		end
		local loader = LoadingWidget(
			{Parent = SortsContainer}, {loadGameSorts})
		spawn(function()
			loader:AwaitFinished()
			loader:Cleanup()
			loader = nil
			populateSortsDebounce = false
			if this.TransitionTweens == nil or #this.TransitionTweens == 0 then
				this.TransitionTweens = ScreenManager:FadeInSitu(SelectableAvatarsContainer)
			end
			for i = 1, #sortsObjects do
				sortsObjects[i]:SetVisible(true)
			end
		end)
	end

	local function UpdateLayout()
		ProfileContainer.Size = Utility.CalculateRelativeDimensions(ProfileContainer, PROFILE_SIZE, MOCKUP_SIZE)

		NameLabel.Size = Utility.CalculateRelativeDimensions(NameLabel, PROFILE_NAME_SIZE, MOCKUP_SIZE)

		ProfileButton.Size = Utility.CalculateRelativeDimensions(ProfileButton, PROFILE_BUTTON_SIZE, MOCKUP_SIZE)
		Utility.CalculateAnchor(ProfileButton, UDim2.new(0, 0, 1, -6), Utility.Enum.Anchor.BottomLeft)


		ProfileImage.Size = Utility.CalculateRelativeDimensions(ProfileImage, PROFILE_IMAGE_SIZE, MOCKUP_SIZE)
		AvatarBrushBackground.Size = UDim2.new(1 - ProfileImage.Size.X.Scale, -ProfileImage.Size.X.Offset, 1, 0)
		-- AvatarBrushBackground.Size = Utility.CalculateRelativeDimensions(AvatarBrushBackground, PROFILE_AVATAR_BRUSH_SIZE, MOCKUP_SIZE)
		AvatarBrushBackground.Position = UDim2.new(1 - AvatarBrushBackground.Size.X.Scale, AvatarBrushBackground.Size.X.Offset, 0, 0)
		Utility.CalculateAnchor(AvatarBrushImage, UDim2.new(0.5, 0, 0.42, 0), Utility.Enum.Anchor.Center)
		Utility.CalculateAnchor(AvatarTextLabel, UDim2.new(0.5, 0, 0.7, 0), Utility.Enum.Anchor.Center)


		FriendActivityContainer.Size = UDim2.new(ProfileContainer.Size.X.Scale, 0, 0, 300);
		FriendActivityContainer.Position = UDim2.new(0,0,ProfileContainer.Size.Y.Scale,0);

		SortsContainer.Size = Utility.CalculateRelativeDimensions(SortsContainer, SORTS_SIZE, MOCKUP_SIZE)
		SortsContainer.Position = UDim2.new(1 - SortsContainer.Size.X.Scale, 0, 0, 0)
	end

	UpdateLayout()

	local screenResolutionChangedConn = nil
	local profileButtonSelectedConn = nil
	local profileButtonDeselectedConn = nil

	function this:GetName()
		return Strings:LocalizedString('HomeWord')
	end

	function this:IsFocused()
		return inFocus
	end

	function this:GetDefaultSelectionObject()
		return ProfileButton
	end

	function this:SetSelectionObject()
		GuiService.SelectedCoreObject = self:GetDefaultSelectionObject()
		-- TODO: Remember selection while staying in Pane. Pressing A on the Home tab should remember
		-- last selection only while in pane.
	end

	function this:Show()
		HomePaneContainer.Visible = true
		PopulateSorts()
		UpdateLayout()

		Utility.DisconnectEvent(screenResolutionChangedConn)
		screenResolutionChangedConn = GuiRoot.Changed:connect(function(prop)
			if prop == 'AbsoluteSize' then
				RunService.RenderStepped:wait()
				UpdateLayout()
			end
		end)

		Utility.DisconnectEvent(profileButtonSelectedConn)
		Utility.DisconnectEvent(profileButtonDeselectedConn)
		profileButtonSelectedConn = ProfileButton.SelectionGained:connect(OnProfileButtonSelectionChanged)
		profileButtonDeselectedConn = ProfileButton.SelectionLost:connect(OnProfileButtonSelectionChanged)
		OnProfileButtonSelectionChanged()
		UpdateProfileInfo()

		ScreenManager:DefaultCancelFade(self.TransitionTweens)
		self.TransitionTweens = ScreenManager:DefaultFadeIn(HomePaneContainer)
		delay(0.5, function()
			if inFocus and GuiService.SelectedCoreObject == nil then
				self:SetSelectionObject()
			end
		end)
		ScreenManager:PlayDefaultOpenSound()
	end

	function this:Hide()
		HomePaneContainer.Visible = false

		profileButtonSelectedConn = Utility.DisconnectEvent(profileButtonSelectedConn)
		profileButtonDeselectedConn = Utility.DisconnectEvent(profileButtonDeselectedConn)
		screenResolutionChangedConn = Utility.DisconnectEvent(screenResolutionChangedConn)

		ScreenManager:DefaultCancelFade(self.TransitionTweens)
		self.TransitionTweens = nil
	end

	function this:Focus()
		inFocus = true
		UpdateLayout()
		self:SetSelectionObject()
		OnProfileButtonSelectionChanged()
	end

	function this:RemoveFocus()
		inFocus = false
		local selectedObject = GuiService.SelectedCoreObject
		if selectedObject and selectedObject:IsDescendantOf(HomePaneContainer) then
			GuiService.SelectedCoreObject = nil
		end
	end

	function this:SetPosition(newPosition)
		HomePaneContainer.Position = newPosition
	end

	function this:SetParent(newParent)
		HomePaneContainer.Parent = newParent
	end

	function this:IsAncestorOf(object)
		return HomePaneContainer and HomePaneContainer:IsAncestorOf(object)
	end

	return this
end

return CreateHomePane
