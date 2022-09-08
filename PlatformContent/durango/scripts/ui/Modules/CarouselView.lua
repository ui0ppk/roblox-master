--[[
				// CarouselView.lua

				// View for a carousel. Used for GameGenre screen
				// TODO: Support Vertical?
				//
				// Current this supports a focus that is aligned to the left (0, 0), in the future we
				// could do other alignments if we need them
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Utility = require(Modules:FindFirstChild('Utility'))

local function createCarouselView()
	local this = {}

	local items = {}
	local padding = 0
	local itemSizePercentOfContainer = 1
	local focusItem = nil

	local BASE_TWEEN_TIME = 0.2

	local container = Utility.Create'ScrollingFrame'
	{
		Name = "CarouselContainer";
		BackgroundTransparency = 1;
		ClipsDescendants = false;
		ScrollingEnabled = false;
		Selectable = false;
		ScrollBarThickness = 0;
	}

	local function isVisible(item)
		return item.AbsolutePosition.x + item.AbsoluteSize.x >= 0 and
			item.AbsolutePosition.x < GuiRoot.AbsoluteSize.x
	end
	local function getFocusSize()
		local size = container.Size.Y.Offset
		return UDim2.new(0, size, 0, size)
	end
	local function getNonFocusSize()
		local size = container.Size.Y.Offset * itemSizePercentOfContainer
		return UDim2.new(0, size, 0, size)
	end
	local function getItemSize(item)
		if item == focusItem then
			return getFocusSize()
		else
			return getNonFocusSize()
		end
	end

	local function getItemLayoutPosition(index)
		local focusIndex = this:GetItemIndex(focusItem)
		local offsetFromFocus = index - focusIndex
		local x, y = 0, 0

		if index > focusIndex then
			-- items to the right of focus need additional buffer due to focus size being larger
			x = getFocusSize().X.Offset + offsetFromFocus * padding + (offsetFromFocus - 1) * getNonFocusSize().X.Offset
		else
			x = offsetFromFocus * padding + offsetFromFocus * getNonFocusSize().X.Offset
		end
		y = (index == focusIndex and 0) or (container.Size.Y.Offset - getNonFocusSize().Y.Offset) / 2

		return UDim2.new(0, x, 0, y)
	end

	local function recalcLayout()
		for i = 1, #items do
			local item = items[i]
			local size = getItemSize(item)
			local position = getItemLayoutPosition(i)
			if item:IsDescendantOf(game.Workspace) then
				item:TweenSizeAndPosition(size, position, Enum.EasingDirection.Out, Enum.EasingStyle.Quad, 0, true)
			else
				item.Size = size
				item.Position = position
			end
		end
	end

	--[[ Public API ]]--
	function this:ChangeFocus(newFocus)
		-- We don't use SetFocus() as that function will do a recalc. We want to get the next position
		-- from current position, not recalcd postion for each item
		if self:ContainsItem(newFocus) then
			focusItem = newFocus
			for i = 1, #items do
				local item = items[i]
				item:TweenSizeAndPosition(getItemSize(item), getItemLayoutPosition(i), Enum.EasingDirection.Out, Enum.EasingStyle.Quad, BASE_TWEEN_TIME, true)
			end
		end
	end

	function this:SetSize(newSize)
		if newSize ~= container.Size then
			container.Size = newSize
			container.CanvasSize = UDim2.new(0, container.Size.X.Offset * 2, 1, 0)
			recalcLayout()
		end
	end

	function this:SetPosition(newPosition)
		container.Position = newPosition
	end

	function this:SetPadding(newPadding)
		if newPadding ~= padding then
			padding = newPadding
			recalcLayout()
		end
	end

	function this:SetItemSizePercentOfContainer(value)
		if value ~= itemSizePercentOfContainer then
			itemSizePercentOfContainer = value
			recalcLayout()
		end
	end

	function this:SetParent(newParent)
		container.Parent = newParent
	end

	function this:SetFocus(newFocusItem)
		if self:ContainsItem(newFocusItem) and newFocusItem ~= focusItem then
			focusItem = newFocusItem
			recalcLayout()
		end
	end

	function this:GetFocusItem()
		return focusItem
	end

	function this:GetItemAt(index)
		return items[index]
	end

	function this:GetFront()
		return items[1]
	end

	function this:GetBack()
		return items[#items]
	end

	function this:GetItemIndex(item)
		for i = 1, #items do
			if items[i] == item then
				return i
			end
		end

		return 0
	end

	function this:GetCount()
		return #items
	end

	function this:GetVisibleCount()
		local visibleItemCount = 0
		for i = 1, #items do
			if isVisible(items[i]) then
				visibleItemCount = visibleItemCount + 1
			end
		end

		return visibleItemCount
	end

	function this:GetFirstVisibleItemIndex()
		for i = 1, #items do
			if isVisible(items[i]) then
				return i
			end
		end
	end

	function this:GetLastVisibleItemIndex()
		for i = #items, 1, -1 do
			if isVisible(items[i]) then
				return i
			end
		end
	end

	function this:GetFullVisibleItemCount()
		local containerSizeX = container.AbsoluteSize.x
		-- remove focus from the size, and figure out how many other items can fit
		local fittingSize = containerSizeX - getFocusSize().X.Offset
		if fittingSize <= 0 then
			return 0
		end

		local itemSize = getNonFocusSize().X.Offset + padding
		local count = math.floor(fittingSize/itemSize) + 1
		return count
	end

	function this:InsertCollectionFront(collection)
		for i = #collection, 1, -1 do
			local item = collection[i]
			table.insert(items, 1, item)
			item.Parent = container
		end
		recalcLayout()
	end

	function this:InsertCollectionBack(collection)
		for i = 1, #collection do
			local item = collection[i]
			table.insert(items, item)
			item.Parent = container
		end
		recalcLayout()
	end

	function this:RemoveAmountFromFront(amount)
		for i = 1, amount do
			local item = table.remove(items, 1)
			item.Parent = nil
		end
		recalcLayout()
	end

	function this:RemoveAmountFromBack(amount)
		for i = 1, amount do
			local item = table.remove(items)
			item.Parent = nil
		end
		recalcLayout()
	end

	function this:InsertFront(newItem)
		table.insert(items, 1, newItem)
		newItem.Parent = container
		recalcLayout()
	end

	function this:InsertBack(newItem)
		table.insert(items, newItem)
		newItem.Parent = container
		recalcLayout()
	end

	function this:RemoveFront()
		local item = table.remove(items, 1)
		item.Parent = nil
		recalcLayout()
	end

	function this:RemoveBack()
		local item = table.remove(items, #items)
		item.Parent = nil
		recalcLayout()
	end

	function this:RemoveItem(item)
		for i = 1, #items do
			if items[i] == item then
				local removedItem = table.remove(items, i)
				removedItem.Parent = nil
				break
			end
		end
		recalcLayout()
	end

	function this:RemoveAllItems()
		for i = #items, 1, -1 do
			local item = table.remove(items, #items)
			item.Parent = nil
		end
	end

	function this:ContainsItem(item)
		if not item then
			return false
		end
		return item.Parent == container
	end

	return this
end

return createCarouselView
