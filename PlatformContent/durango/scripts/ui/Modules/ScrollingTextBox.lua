--[[
			// ScrollingTextBox.lua

			// Creates a scrolling text box to be used with controlers and selectable
			// guis

			// NOTE: Add any api needed to further expand this module
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local createScrollingTextBox = function(size, position, parent)
	local this = {}

	local SCROLL_BUFFER = 2

	this.OnSelectableChaged = Utility.Signal()

	-- adjust selection image
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

	local container = Utility.Create'Frame'
	{
		Name = "ScrollingTextBox";
		Size = size or UDim2.new();
		Position = position or UDim2.new();
		BackgroundTransparency = 1;
		Parent = parent;
	}
	local scrollingFrame = Utility.Create'ScrollingFrame'
	{
		Name = "ScrollingBox";
		Size = UDim2.new(1, 0, 1, 0);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		ScrollBarThickness = 0;
		SelectionImageObject = edgeSelectionImage;
		Parent = container;

		SoundManager:CreateSound('MoveSelection');
	}
	local textLabel = Utility.Create'TextLabel'
	{
		Name = "TextLabel";
		Size = UDim2.new(1, 0, 4, 0);
		Position = UDim2.new(0, 0, 0, 0);
		BackgroundTransparency = 1;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.DescriptionSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextWrapped = true;
		Text = "";
		Parent = scrollingFrame;
	}
	local upArrow = Utility.Create'ImageLabel'
	{
		Name = "UpArrow";
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.WhiteTextColor;
		Visible = false;
		Parent = container;
	}
	local downArrow = Utility.Create'ImageLabel'
	{
		Name = "DownArrow";
		BackgroundTransparency = 1;
		ImageColor3 = GlobalSettings.WhiteTextColor;
		Visible = false;
		Parent = container;
	}
	AssetManager.LocalImage(upArrow,
		'rbxasset://textures/ui/Shell/Icons/UpIndicatorIcon', {['720'] = UDim2.new(0,14,0,13); ['1080'] = UDim2.new(0,21,0,19);})
	upArrow.Position = UDim2.new(0, 0, 1, 16)
	AssetManager.LocalImage(downArrow,
		'rbxasset://textures/ui/Shell/Icons/DownIndicatorIcon', {['720'] = UDim2.new(0,14,0,13); ['1080'] = UDim2.new(0,21,0,19);})
	downArrow.Position = UDim2.new(0, upArrow.Size.X.Offset, 1, 16)

	--[[ Private Functions ]]--
	local function setArrowState()
		local canvasPosition = scrollingFrame.CanvasPosition
		local maxSizeY = textLabel.AbsoluteSize.y - scrollingFrame.AbsoluteWindowSize.y
		if canvasPosition.y >= maxSizeY - SCROLL_BUFFER then
			downArrow.ImageColor3 = GlobalSettings.GreyTextColor
		else
			downArrow.ImageColor3 = GlobalSettings.WhiteTextColor
		end
		if canvasPosition.y <= SCROLL_BUFFER then
			upArrow.ImageColor3 = GlobalSettings.GreyTextColor
		else
			upArrow.ImageColor3 = GlobalSettings.WhiteTextColor
		end
	end

	local function setScrollSize()
		-- NOTE: this is a hack to get the actual height on textbounds
		textLabel.Size = UDim2.new(1, 0, 0, 100000)

		local ySize = textLabel.TextBounds.y
		textLabel.Size = UDim2.new(1, 0, 0, ySize)
		scrollingFrame.CanvasSize = UDim2.new(0, 0, 0, ySize)
		local areArrowsVisible = ySize > scrollingFrame.AbsoluteSize.y
		this:SetArrowsVisible(areArrowsVisible)
		this:SetSelectable(areArrowsVisible)
		setArrowState();
	end

	--[[ Events ]]--
	scrollingFrame.Changed:connect(function(property)
		if property == 'CanvasPosition' or property == 'AbsoluteWindowSize' then
			setArrowState()
		end
	end)

	--[[ Public API ]]--
	function this:SetParent(newParent)
		scrollingFrame.Parent = newParent
		spawn(function()
			setScrollSize()
		end)
	end

	function this:SetPosition(newPosition)
		scrollingFrame.Position = newPosition
	end

	function this:SetSize(newSize)
		scrollingFrame.Size = newSize
		spawn(function()
			setScrollSize()
		end)
	end

	function this:SetFontSize(newFontSize)
		textLabel.FontSize = newFontSize
		spawn(function()
			setScrollSize()
		end)
	end

	function this:SetFont(newFont)
		textLabel.Font = newFont
		spawn(function()
			setScrollSize()
		end)
	end

	function this:SetText(text)
		textLabel.Text = tostring(text)
		spawn(function()
			setScrollSize()
		end)
	end

	function this:SetSelectable(value)
		scrollingFrame.Selectable = value
		this.OnSelectableChaged:fire(value)
	end

	function this:SetZIndex(value)
		container.ZIndex = value
		textLabel.ZIndex = value
		upArrow.ZIndex = value
		downArrow.ZIndex = value
	end

	function this:SetArrowsVisible(value)
		upArrow.Visible = value
		downArrow.Visible = value
	end

	function this:GetContainer()
		return container
	end

	function this:GetSelectableObject()
		return scrollingFrame
	end

	function this:GetArrowsVisible()
		return upArrow.Visible
	end

	function this:IsSelectable()
		return scrollingFrame.Selectable
	end

	return this
end

return createScrollingTextBox
