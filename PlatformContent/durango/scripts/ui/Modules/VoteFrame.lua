--[[
			// VoteFrame.lua
			// Creates a vote frame for a game
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local CreateVoteFrame = function(parent, position)
	local this = {}

	-- Assume 1080p
	local MAX_SIZE = 203

	local voteContainer = Utility.Create'Frame'
	{
		Name = "VoteContainer";
		Size = UDim2.new(0, MAX_SIZE, 0, 16);
		Position = position;
		BackgroundTransparency = 1;
		Visible = false;
		Parent = parent;
	}
	local batteryImageRed = Utility.Create'ImageLabel'
	{
		Name = "BatteryImageRed";
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.RedTextColor;
		Parent = voteContainer;
	}

	AssetManager.LocalImage(batteryImageRed,
		'rbxasset://textures/ui/Shell/Icons/RatingBar', {['720'] = UDim2.new(0,134,0,11); ['1080'] = UDim2.new(0,203,0,16);})
	local batteryImageGreen = batteryImageRed:Clone()
	batteryImageGreen.ImageColor3 = GlobalSettings.GreenTextColor
	batteryImageGreen.ZIndex = 2
	batteryImageGreen.Parent = batteryImageRed

	--[[ Public API ]]--
	function this:SetPercentFilled(percent)
		if not percent then
			batteryImageGreen.Size = batteryImageRed.Size
			batteryImageGreen.ImageRectSize = Vector2.new(0, 0)
			batteryImageGreen.ImageColor3 = GlobalSettings.GreyTextColor
		else
			percent = Utility.Round(percent, 0.1)
			local drawSize = math.floor(percent * MAX_SIZE)
			batteryImageGreen.ImageColor3 = GlobalSettings.GreenTextColor
			batteryImageGreen.Size = UDim2.new(0, drawSize, 0, batteryImageGreen.Size.Y.Offset)
			batteryImageGreen.ImageRectSize = Vector2.new(drawSize, 0)
		end
	end

	function this:SetVisible(value)
		voteContainer.Visible = value
	end

	function this:GetContainer()
		return voteContainer
	end

	function this:Destroy()
		voteContainer:Destroy()
		this = nil
	end

	return this
end

return CreateVoteFrame
