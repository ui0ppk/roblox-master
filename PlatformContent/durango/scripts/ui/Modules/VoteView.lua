--[[
			// VoteView.lua

			// Manages the vote view for the game details page
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local VoteFrame = require(Modules:FindFirstChild('VoteFrame'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local ErrorOverlayModule = require(Modules:FindFirstChild('ErrorOverlay'))
local Errors = require(Modules:FindFirstChild('Errors'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local UserData = require(Modules:FindFirstChild('UserData'))

local function createVoteView()
	local this = {}

	local canVote = false
	local myVote = nil
	local upVotes = 0
	local downVotes = 0
	local thisGameData = nil
	local defaultSelection = nil

	this.Container = Utility.Create'Frame'
	{
		Name = "VoteContainer";
		Size = UDim2.new(1, 0, 0, 114);
		BackgroundTransparency = 1;
		Visible = false;
	}

	-- Can't Vote Objects
	-- override selection image
	local SelectionImage = Utility.Create'ImageLabel'
	{
		Name = "SelectionImage";
		Size = UDim2.new(1, 32, 1, 32);
		Position = UDim2.new(0, -16, 0, -16);
		Image = 'rbxasset://textures/ui/SelectionBox.png';
		ScaleType = Enum.ScaleType.Slice;
		SliceCenter = Rect.new(21,21,41,41);
		BackgroundTransparency = 1;
	}
	local CannotVoteSelection = Utility.Create'Frame'
	{
		Name = "CannotVoteSelection";
		Size = UDim2.new(1, 0, 0, 94);
		Position = UDim2.new(0, 0, 0, 5);
		BackgroundTransparency = 1;
		Selectable = true;
		SelectionImageObject = SelectionImage;
		SoundManager:CreateSound('MoveSelection');
	}
	local CannotVoteText = Utility.Create'TextLabel'
	{
		Name = "CannotVoteText";
		Size = UDim2.new(0.8, 0, 1, 0);
		Position = UDim2.new(0.1, 0, 0, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.BoldFont;
		FontSize = GlobalSettings.SubHeaderSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Visible = false;
		TextWrapped = true;
		Text = Strings:LocalizedString("CannotVoteWord");
		Parent = CannotVoteSelection;
	}

	-- Vote Objects
	local VoteWidget = VoteFrame(this.Container, UDim2.new(0, 88, 0, 34))
	local VoteContainer = VoteWidget:GetContainer()
	local ThumbsUpImage = Utility.Create'ImageLabel'
	{
		Name = "ThumbsUpImage";
		Size = UDim2.new(0, 48, 0, 48);
		BackgroundTransparency = 1;
	}
	local ThumbsDownImage = ThumbsUpImage:Clone()
	ThumbsDownImage.Name = "ThumbsDownImage"
	-- Buttons act as buffers for selection gui
	local ThumbsUpButton = Utility.Create'ImageButton'
	{
		Name = "ThumbsUpButton";
		Size = UDim2.new(0, ThumbsUpImage.Size.X.Offset + 18, 0, ThumbsUpImage.Size.Y.Offset + 18);
		BackgroundTransparency = 1;
		Image = "";
		Parent = VoteContainer;
		SoundManager:CreateSound('MoveSelection');
	}
	ThumbsUpButton.Position = UDim2.new(0, -ThumbsUpButton.Size.X.Offset - 12, 0,
			-ThumbsUpButton.Size.Y.Offset / 2 + VoteContainer.Size.Y.Offset / 2)
	local ThumbsDownButton = ThumbsUpButton:Clone()
	ThumbsDownButton.Name = "ThumbsDownButton"
	ThumbsDownButton.Position = UDim2.new(1, 12, 0, ThumbsDownButton.Position.Y.Offset)
	ThumbsDownButton.Parent = VoteContainer

	ThumbsUpImage.Parent = ThumbsUpButton
	ThumbsDownImage.Parent = ThumbsDownButton
	ThumbsUpImage.Position = UDim2.new(0.5, -ThumbsUpImage.Size.X.Offset / 2, 0.5, -ThumbsUpImage.Size.Y.Offset / 2)
	ThumbsDownImage.Position = UDim2.new(0.5, -ThumbsDownImage.Size.X.Offset / 2, 0.5, -ThumbsDownImage.Size.Y.Offset / 2)

	local RatingText = Utility.Create'TextLabel'
	{
		Name = "RatingText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, VoteContainer.Position.X.Offset + VoteContainer.Size.X.Offset / 2, 1, -20);
		BackgroundTransparency = 1;
		Font = GlobalSettings.BoldFont;
		FontSize = GlobalSettings.SubHeaderSize;
		Text = "";
		Parent = this.Container;
	}
	local UpCountText = Utility.Create'TextLabel'
	{
		Name = "UpCountText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, 43, 1, RatingText.Position.Y.Offset);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.SubHeaderSize;
		TextColor3 = GlobalSettings.GreenTextColor;
		Text = "";
		Parent = this.Container;
	}
	local DownCountText = UpCountText:Clone()
	DownCountText.Name = "DownCountText"
	DownCountText.Position = UDim2.new(0, 336, 1, RatingText.Position.Y.Offset)
	DownCountText.TextColor3 = GlobalSettings.RedTextColor;
	DownCountText.Parent = this.Container;

	local function setImagesAndText(vote, noVote)
		AssetManager.LocalImage(ThumbsUpImage,
			(vote == true and 'rbxasset://textures/ui/Shell/Icons/ThumbsUpFilled') or
			'rbxasset://textures/ui/Shell/Icons/ThumbsUp', {['720'] = UDim2.new(0,32,0,32); ['1080'] = UDim2.new(0,48,0,48);})
		AssetManager.LocalImage(ThumbsDownImage,
			(vote == false and 'rbxasset://textures/ui/Shell/Icons/ThumbsDownFilled') or
			'rbxasset://textures/ui/Shell/Icons/ThumbsDown', {['720'] = UDim2.new(0,32,0,32); ['1080'] = UDim2.new(0,48,0,48);})
		RatingText.TextColor3 =
			(vote == true and GlobalSettings.GreenTextColor) or
			(vote == false and GlobalSettings.RedTextColor) or GlobalSettings.GreyTextColor
		local textWord = (noVote and "FirstToRateWord") or
			(vote == true and "LikedWord") or (vote == false and "DislikedWord") or nil
		RatingText.Text = textWord and Strings:LocalizedString(textWord) or ""
	end

	local function updateView(newVote)
		local noVote = upVotes == 0 and downVotes == 0

		VoteWidget:SetPercentFilled(not noVote and (upVotes / (upVotes + downVotes)) or nil)
		setImagesAndText(newVote, noVote)
		UpCountText.Text = Utility.FormatNumberString(noVote and "" or upVotes)
		DownCountText.Text = Utility.FormatNumberString(noVote and "" or downVotes)

		ThumbsUpButton.Selectable = canVote
		ThumbsDownButton.Selectable = canVote

		CannotVoteSelection.Parent = not canVote and this.Container.Parent or nil
		defaultSelection = not canVote and CannotVoteSelection or ThumbsUpButton
	end

	local function updateVoteCount(prevVote, newVote)
		-- we must check for a unique vote before sending off the achivement event
		-- unique votes are when prevVote is null.
		if prevVote == "null" then
			UserData:IncrementVote()
		elseif newVote == "null" then
			UserData:DecrementVote()
		end
		EventHub:dispatchEvent(EventHub.Notifications["VotedOnPlace"])
	end

	local isVotingDebouce = false
	local function postVoteAsync(newVote)
		if isVotingDebouce then return end
		isVotingDebouce = true
		local success, reason = nil, nil
		if thisGameData then
			success, reason = thisGameData:PostVoteAsync(newVote)
		else
			success = false
		end
		if success then
			local prevVote = myVote
			if newVote == true then
				upVotes = upVotes + 1
				if prevVote == false then
					downVotes = downVotes - 1
				end
			elseif newVote == false then
				downVotes = downVotes + 1
				if prevVote == true then
					upVotes = upVotes - 1
				end
			elseif newVote == "null" then
				if prevVote == true then
					upVotes = upVotes - 1
				elseif prevVote == false then
					downVotes = downVotes - 1
				end
			end
			myVote = newVote
			updateView(newVote)
			updateVoteCount(prevVote, newVote)
		else
			if reason and Errors.Vote[reason] then
				ScreenManager:OpenScreen(ErrorOverlayModule(Errors.Vote[reason]), false)
			else
				ScreenManager:OpenScreen(ErrorOverlayModule(Errors.Default), false)
			end
		end
		isVotingDebouce = false
	end

	local function toggleCannotVoteDisplay()
		CannotVoteText.Visible = not CannotVoteText.Visible
		this.Container.Visible = not this.Container.Visible
	end
	CannotVoteSelection.SelectionGained:connect(toggleCannotVoteDisplay)
	CannotVoteSelection.SelectionLost:connect(toggleCannotVoteDisplay)

	ThumbsUpButton.MouseButton1Click:connect(function()
		postVoteAsync(myVote == true and "null" or true)
	end)
	ThumbsDownButton.MouseButton1Click:connect(function()
		postVoteAsync(myVote == false and "null" or false)
	end)

	--[[ Public API ]]--
	function this:SetParent(newParent)
		self.Container.Parent = newParent
	end

	function this:SetPosition(newPosition)
		self.Container.Position = newPosition
		CannotVoteSelection.Position = UDim2.new(0, 0, 0, self.Container.Position.Y.Offset + 5)
	end

	function this:SetVisible(value)
		self.Container.Visible = value
	end

	function this:GetDefaultSelection()
		return defaultSelection
	end

	function this:SetCanVote(value)
		canVote = value
		updateView(myVote)
	end

	function this:InitializeAsync(gameData)
		thisGameData = gameData
		local voteData = gameData:GetVoteDataAsync()
		if voteData then
			upVotes = voteData.UpVotes
			downVotes = voteData.DownVotes
			myVote = voteData.UserVote or "null"
			canVote = voteData.CantVoteReason ~= "PlayGame"
		else
			myVote = "null"
		end

		updateView(myVote)
		VoteWidget:SetVisible(true)
	end

	return this
end

return createVoteView
