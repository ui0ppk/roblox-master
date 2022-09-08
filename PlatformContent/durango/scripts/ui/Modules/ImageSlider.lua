--[[
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService("GuiService")
local UserInputService = game:GetService("UserInputService")

local ThumbnailLoader = require(Modules:FindFirstChild('ThumbnailLoader'))
local Utility = require(Modules:FindFirstChild('Utility'))

local function CreateImageSlider(size, position)
	local this = {}

	local items = {}
	local maxItems = 0
	local currentItemIndex = 1
	local padding = 0
	local focusSize = 450
	local itemSize = 300
	local dataTable = nil

	local lastSelectedObject = nil
	local newSelectedObjectCn = nil
	this.OnNewFocusItem = Utility.Signal()

	local imageObjectToLoaderMap = {}

	this.Container = Utility.Create'ScrollingFrame'
	{
		Name = "ImageSliderContainer";
		Size = size;
		Position = position;
		BackgroundTransparency = 1;
		ClipsDescendants = false;
		ScrollingEnabled = false;
		Selectable = false;
		ScrollBarThickness = 0;
	}

	local function loadNewImage(item, dataIndex)
		if imageObjectToLoaderMap[item] then
			imageObjectToLoaderMap[item]:Cancel()
			imageObjectToLoaderMap[item] = nil
		end
		local iconId = dataTable[dataIndex].IconId
		local thumbLoader = ThumbnailLoader:Create(item, iconId,
			ThumbnailLoader.Sizes.Medium, ThumbnailLoader.AssetType.Icon)
		imageObjectToLoaderMap[item] = thumbLoader
		spawn(function()
			thumbLoader:LoadAsync()
			imageObjectToLoaderMap[item] = nil
		end)
	end

	local MAX_LEFT = 2
	local MAX_RIGHT = 6
	-- TODO: Figure out left/right better
	local function recalcPositionAndSize(tweenTime, isRight, newSelectedObject)
		local smIconYPos = (focusSize - itemSize) / 2
		local startPosition = 0
		if currentItemIndex == 1 then
			startPosition = 0
		elseif currentItemIndex == 2 then
			startPosition = -1
		elseif maxItems == #items then
			startPosition = 1 - currentItemIndex
		elseif maxItems - currentItemIndex  < MAX_RIGHT then
			startPosition = maxItems - currentItemIndex - MAX_RIGHT - MAX_LEFT
		else
			startPosition = -MAX_LEFT
		end

		-- put front at back
		if maxItems > #items then
			if isRight == true and startPosition == -MAX_LEFT and currentItemIndex > 3 then
				local front = items[1]
				local back = items[#items]
				front.Position = UDim2.new(0, back.Position.X.Offset + back.Size.X.Offset + padding, 0, smIconYPos)
				this:RemoveFromFront()
				this:AddItemToBack(front)
				if dataTable and dataTable[currentItemIndex + MAX_RIGHT] then
					loadNewImage(front, currentItemIndex + MAX_RIGHT)
				end
			-- put back in front
			elseif isRight == false and currentItemIndex < maxItems - MAX_RIGHT and startPosition == -MAX_LEFT then
				local back = items[#items]
				local front = items[1]
				back.Position = UDim2.new(0, front.Position.X.Offset - padding - itemSize, 0, smIconYPos)
				this:RemoveFromBack()
				this:AddToFront(back)
				if dataTable and dataTable[currentItemIndex - MAX_LEFT] then
					loadNewImage(back, currentItemIndex - MAX_LEFT)
				end
			end
		end

		local currentXPosition = itemSize * startPosition + padding * startPosition
		for i = 1, #items do
			if items[i] == newSelectedObject then
				if tweenTime == 0 then
					items[i].Size = UDim2.new(0, focusSize, 0, focusSize)
					items[i].Position = UDim2.new(0, currentXPosition, 0, 0)
				else
					items[i]:TweenSizeAndPosition(UDim2.new(0, focusSize, 0, focusSize), UDim2.new(0, currentXPosition, 0, 0),
						Enum.EasingDirection.InOut, Enum.EasingStyle.Sine, tweenTime, true)
				end
				currentXPosition = currentXPosition + focusSize + padding
			else
				if tweenTime == 0 then
					items[i].Size = UDim2.new(0, itemSize, 0, itemSize)
					items[i].Position = UDim2.new(0, currentXPosition, 0, smIconYPos)
				else
					items[i]:TweenSizeAndPosition(UDim2.new(0, itemSize, 0, itemSize), UDim2.new(0, currentXPosition, 0, smIconYPos),
						Enum.EasingDirection.InOut, Enum.EasingStyle.Sine, tweenTime, true)
				end
				currentXPosition = currentXPosition + itemSize + padding
			end
		end
	end

	--[[ Events ]]--
	newSelectedObjectCn = GuiService.Changed:connect(function(property)
		if property == 'SelectedCoreObject' then
			local selectedObject = GuiService.SelectedCoreObject
			if selectedObject and selectedObject ~= lastSelectedObject and selectedObject:IsDescendantOf(this.Container) then
				local isRight = nil
				if lastSelectedObject then
					-- slide left
					if lastSelectedObject.AbsolutePosition.x < selectedObject.AbsolutePosition.x then
						currentItemIndex = currentItemIndex + 1
						isRight = true
					-- slide right
					else
						currentItemIndex = currentItemIndex - 1
						isRight = false
					end
				end
				recalcPositionAndSize(lastSelectedObject and 0.25 or 0, isRight, selectedObject)
				lastSelectedObject = selectedObject
				this.OnNewFocusItem:fire(currentItemIndex, isRight, selectedObject)
			end
		end
	end)

	--[[ Public API ]]--
	function this:SetPosition(position)
		self.Container.Position = position
	end

	function this:SetSize(size)
		self.Container.Size = size
	end

	function this:SetParent(parent)
		self.Container.Parent = parent
	end

	function this:SetFocusPosition(index)
		if index > 0 and index < #items + 1 then
			currentItemIndex = index
			recalcPositionAndSize(0, nil, items[currentItemIndex])
		end
	end

	function this:SetDataTable(tbl)
		dataTable = tbl
	end

	function this:SetMaxItems(value)
		local index = nil
		-- we're near the end, so we need to update image content
		if currentItemIndex > maxItems - MAX_RIGHT then
			-- find the current selected index in relation to image pool
			for i = 1, #items do
				if lastSelectedObject and items[i] == GuiService.SelectedCoreObject then
					index = i
					break
				end
			end
			-- TODO: Still might be a visual bug in some cases
			if index then
				-- replace images from front of pool
				for i = 1, index - 3 do
					if dataTable[currentItemIndex + i] then
						local item = items[1]
						loadNewImage(item, currentItemIndex + (maxItems - currentItemIndex) + i)
						print("NEW IMAGE:", currentItemIndex, ":", maxItems, ":", i, ":", currentItemIndex + (maxItems - currentItemIndex) + i)
						self:RemoveFromFront()
						self:AddItemToBack(item)
					end
				end
			end
		end
		maxItems = value
		recalcPositionAndSize(0)
	end

	function this:SetPadding(newPadding)
		padding = newPadding
	end

	function this:AddToFront(newItem)
		table.insert(items, 1, newItem)
		newItem.Parent = this.Container
	end

	function this:AddItemToBack(newItem)
		if this and this.Container then
			items[#items + 1] = newItem
			newItem.Parent = this.Container
		end
	end

	function this:AddItem(newItem)
		self:AddItemToBack(newItem)
	end

	function this:RemoveFromFront()
		table.remove(items, 1)
	end

	function this:RemoveFromBack()
		table.remove(items)
	end

	function this:Destroy()
		if newSelectedObjectCn then
			newSelectedObjectCn:disconnect()
			newSelectedObjectCn = nil
		end
		this.Container:Destroy()
		this.OnNewFocus = nil
		this = nil
	end

	return this
end

return CreateImageSlider
