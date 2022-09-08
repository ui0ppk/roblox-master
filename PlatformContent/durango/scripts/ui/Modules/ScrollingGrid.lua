-- Written by Kip Turner, Copyright ROBLOX 2015

local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))

local GuiService = game:GetService('GuiService')

local DEFAULT_WINDOW_SIZE = UDim2.new(1,0,1,0)


local function ScrollingGrid()

	local this = {}

	this.Enum =
	{
		ScrollDirection = {["Vertical"] = 1; ["Horizontal"] = 2;};
		StartCorner = {["UpperLeft"] = 1; ["UpperRight"] = 2; ["BottomLeft"] = 3; ["BottomRight"] = 4;};
		--ChildAlignment = {["UpperLeft"] = 1; ["UpperRight"] = 2; ["BottomLeft"] = 3; ["BottomRight"] = 4;};
	}


	this.GridItems = {}
	this.ItemSet = {}

	this.ScrollDirection = this.Enum.ScrollDirection.Vertical

	this.StartCorner = this.Enum.StartCorner.UpperLeft

	this.FixedRowColumnCount = nil

	--this.ChildAlignment = nil

	this.CellSize = Vector2.new(100,100)
	this.Padding = Vector2.new(0,0)
	this.Spacing = Vector2.new(0,0)




	function this:GetPadding()
		return self.Padding
	end

	function this:SetPadding(newPadding)
		if newPadding ~= self.Padding then
			self.Padding = newPadding
			self:RecalcLayout()
		end
	end

	function this:GetSpacing()
		return self.Spacing
	end

	function this:SetSpacing(newSpacing)
		if newSpacing ~= self.Spacing then
			self.Spacing = newSpacing
			self:RecalcLayout()
		end
	end

	function this:GetCellSize()
		return self.CellSize
	end

	function this:SetCellSize(cellSize)
		if cellSize ~= self.CellSize then
			self.CellSize = cellSize
			self:RecalcLayout()
		end
	end

	function this:GetScrollDirection()
		return self.ScrollDirection
	end

	function this:SetScrollDirection(newDirection)
		if newDirection ~= self.ScrollDirection then
			self.ScrollDirection = newDirection
			self:RecalcLayout()
		end
	end

	function this:GetStartCorner()
		return self.StartCorner
	end

	function this:SetStartCorner(newStartCorner)
		if newStartCorner ~= self.StartCorner then
			self.StartCorner = newStartCorner
			self:RecalcLayout()
		end
	end

	function this:GetRowColumnConstraint()
		return self.FixedRowColumnCount
	end

	function this:SetRowColumnConstraint(fixedRowColumnCount)
		if fixedRowColumnCount < 1 then
			fixedRowColumnCount = nil
		end
		if fixedRowColumnCount ~= self.FixedRowColumnCount then
			self.FixedRowColumnCount = fixedRowColumnCount
			self:RecalcLayout()
		end
	end


	function this:GetClipping()
		return self.Container.ClipsDescendants
	end

	function this:SetClipping(clippingEnabled)
		self.Container.ClipsDescendants = clippingEnabled;
	end

	function this:GetVisible()
		return self.Container.Visible
	end

	function this:SetVisible(isVisible)
		self.Container.Visible = isVisible;
	end

	function this:GetSize()
		return self.Container.Size
	end

	function this:SetSize(size)
		self.Container.Size = size
	end

	function this:GetPosition()
		return self.Container.Position
	end

	function this:SetPosition(position)
		self.Container.Position = position
	end

	function this:GetParent()
		return self.Container.Parent
	end

	function this:SetParent(parent)
		self.Container.Parent = parent
	end

	function this:GetGuiObject()
		return self.Container
	end

	-- Default selection handles the case of removing the last item in the grid while it is selected
	-- Set to nil if do not want a default selection
	function this:SetDefaultSelection(selectionObject)
		self.DefaultSelection = selectionObject
	end

	function this:ResetDefaultSelection()
		self.DefaultSelection = self.Container
	end

	----


	function this:ContainsItem(gridItem)
		return self.ItemSet[gridItem] ~= nil
	end

	function this:SortItems(sortFunc)
		table.sort(self.GridItems, sortFunc)
		self:RecalcLayout()

		local selectedObject = self:FindAncestorGridItem(GuiService.SelectedCoreObject)
		if selectedObject and self:ContainsItem(selectedObject) then
			local thisPos = self:GetCanvasPositionForOffscreenItem(selectedObject)
			if thisPos then
				Utility.PropertyTweener(self.Container, 'CanvasPosition', thisPos, thisPos, 0, Utility.EaseOutQuad, true)
			end
		end
	end

	function this:AddItem(gridItem)
		if not self:ContainsItem(gridItem) then
			table.insert(self.GridItems, gridItem)
			self.ItemSet[gridItem] = true
			gridItem.Parent = self.Container
			if GuiService.SelectedCoreObject == self.DefaultSelection then
				GuiService.SelectedCoreObject = gridItem
			end
			self:RecalcLayout()
		end
	end

	function this:RemoveItem(gridItem)
		if self:ContainsItem(gridItem) then
			for i, otherItem in pairs(self.GridItems) do
				if otherItem == gridItem then
					table.remove(self.GridItems, i)
					-- Assign a new selection
					if GuiService.SelectedCoreObject == gridItem then
						GuiService.SelectedCoreObject = self.GridItems[i] or self.GridItems[i-1] or self.GridItems[1] or self.DefaultSelection
					end
					-- Clean-up
					self.ItemSet[gridItem] = nil
					gridItem.Parent = nil
					self:RecalcLayout()
					return
				end
			end
		end
	end

	function this:RemoveAllItems()
		local wasSelected = false
		do
			local currentSelection = GuiService.SelectedCoreObject
			while currentSelection ~= nil and wasSelected == false do
	  			wasSelected = wasSelected or self:ContainsItem(currentSelection)
	  			currentSelection = currentSelection.Parent
	  		end
		end
		for i = #self.GridItems, 1, -1 do
			local removed = table.remove(self.GridItems, i)
			self.ItemSet[removed] = nil
			removed.Parent = nil
		end

		if wasSelected then
			GuiService.SelectedCoreObject = self.Container
		end

		self:RecalcLayout()
		self.Container.CanvasPosition = Vector2.new(0, 0)
	end

	function this:Get2DGridIndex(index)
		-- 0 base index
		local zerobasedIndex = index - 1
		local rows, columns = self:GetNumRowsColumns()
		local row, column;

		-- TODO: implement StartCorner here
		if self.ScrollDirection == self.Enum.ScrollDirection.Vertical then
			row = math.floor(zerobasedIndex / columns)
			column = zerobasedIndex % columns
		else
			column = math.floor(zerobasedIndex / rows)
			row = zerobasedIndex % rows
		end

		return row, column
	end

	function this:GetNumRowsColumns()
		local rows, columns = 0, 0

		local windowSize = self.Container.AbsoluteWindowSize
		local padding = self:GetPadding()
		local cellSize = self:GetCellSize()
		local cellSpacing = self:GetSpacing()
		local adjustedWindowSize = Utility.ClampVector2(Vector2.new(0, 0), windowSize - padding, windowSize - padding)
		local absoluteCellSize = Utility.ClampVector2(Vector2.new(1,1), cellSize + cellSpacing, cellSize + cellSpacing)
		local windowSizeCalc = (adjustedWindowSize + cellSpacing) / absoluteCellSize

		if self.ScrollDirection == self.Enum.ScrollDirection.Vertical then
			columns = math.max(1, self:GetRowColumnConstraint() or math.floor(windowSizeCalc.x))
			rows = math.ceil(math.max(1, #self.GridItems) / columns)
		else
			rows = math.max(1, self:GetRowColumnConstraint() or math.floor(windowSizeCalc.y))
			columns = math.ceil(math.max(1, #self.GridItems) / rows)
		end

		return rows, columns
	end

	function this:GetGridPosition(row, column, gridItemSize)
		local cellSize = self:GetCellSize()
		local spacing = self:GetSpacing()
		local padding = self:GetPadding()
		return UDim2.new(0, padding.X + column * cellSize.X + column * spacing.X,
						 0, padding.Y + row * cellSize.Y + row * spacing.Y)
	end

	function this:GetGridItemSize()
		return self.CellSize
		--[[
		if self.CellSize then
			return self.CellSize
		end
		return UDim2.new(0, (self.Container.AbsoluteSize.X - ((self.Columns + 1) * self.CellPadding.X)) / self.Columns,
			             0, (self.Container.AbsoluteSize.Y - ((self.Rows + 1) * self.CellPadding.Y)) / self.Rows)
		--]]
		-- if self.ScrollDirection == EnumScrollDirection.Vertical then
		-- 	return UDim2.new(0,(self.Container.AbsoluteSize.X - ((self.Columns + 1) * self.CellPadding.X)) / self.Columns, 0, self.Container.AbsoluteSize.Y / self.Rows)
		-- else
		-- 	return UDim2.new(0,self.Container.AbsoluteSize.Y / self.Rows, 0, (self.Container.AbsoluteSize.Y - ((self.Rows + 1) * self.CellPadding.Y)) / self.Rows)
		-- end
	end

	function this:GetCanvasPositionForOffscreenItem(selectedObject)
		-- NOTE: using <= and >= instead of < and > because scrollingframe
		-- code may automatically bump it while we are observing the change
		if selectedObject and self.Container and self:ContainsItem(selectedObject) then
			if self.ScrollDirection == self.Enum.ScrollDirection.Vertical then
				if selectedObject.AbsolutePosition.Y <= self.Container.AbsolutePosition.Y then
					return Utility.ClampCanvasPosition(self.Container, Vector2.new(0, selectedObject.Position.Y.Offset)) -- - selectedObject.AbsoluteSize.Y/2))
				elseif selectedObject.AbsolutePosition.Y + selectedObject.AbsoluteSize.Y >= self.Container.AbsolutePosition.Y + self.Container.AbsoluteWindowSize.Y then
					return Utility.ClampCanvasPosition(self.Container, Vector2.new(0, -(self.Container.AbsoluteWindowSize.Y - selectedObject.Position.Y.Offset - selectedObject.AbsoluteSize.Y)  )) --+ selectedObject.AbsoluteSize.Y/2))
				end
			else -- Horizontal
				if selectedObject.AbsolutePosition.X <= self.Container.AbsolutePosition.X then
					return Utility.ClampCanvasPosition(self.Container, Vector2.new(selectedObject.Position.X.Offset, 0))
				elseif selectedObject.AbsolutePosition.X + selectedObject.AbsoluteSize.X >= self.Container.AbsolutePosition.X + self.Container.AbsoluteWindowSize.X then
					return Utility.ClampCanvasPosition(self.Container, Vector2.new(-(self.Container.AbsoluteWindowSize.X - selectedObject.Position.X.Offset - selectedObject.AbsoluteSize.X), 0))
				end
			end
		end
	end

	function this:RecalcLayout()
		local padding = self:GetPadding()
		local cellSpacing = self:GetSpacing()
		local gridItemSize = self:GetGridItemSize()
		local rows, columns = self:GetNumRowsColumns()

		if self.ScrollDirection == self.Enum.ScrollDirection.Vertical then
			self.Container.CanvasSize = UDim2.new(self.Container.Size.X.Scale, self.Container.Size.X.Offset, 0, padding.Y * 2 + rows * gridItemSize.Y + (math.max(0, rows - 1)) * cellSpacing.Y)
		else
			self.Container.CanvasSize = UDim2.new(0, padding.X * 2 + columns * gridItemSize.X + (math.max(0, columns - 1)) * cellSpacing.X, self.Container.Size.Y.Scale, self.Container.Size.Y.Offset)
		end

		local grid2DtoIndex = {}
		for i = 1, #self.GridItems do
			local row, column = self:Get2DGridIndex(i)
			local gridItem = self.GridItems[i]

			gridItem.Size = UDim2.new(0, gridItemSize.X, 0, gridItemSize.Y)
			gridItem.Position = self:GetGridPosition(row, column, gridItemSize)

			grid2DtoIndex[row] = grid2DtoIndex[row] or {}
			grid2DtoIndex[row][column] = gridItem
		end

		for rowNum, row in pairs(grid2DtoIndex) do
			for columnNum, column in pairs(row) do
				local gridItem = grid2DtoIndex[rowNum][columnNum]
				if gridItem then
					if self.ScrollDirection == self.Enum.ScrollDirection.Vertical then
						gridItem.NextSelectionUp = grid2DtoIndex[rowNum - 1] and grid2DtoIndex[rowNum - 1][columnNum] or nil
						gridItem.NextSelectionDown = grid2DtoIndex[rowNum + 1] and grid2DtoIndex[rowNum + 1][columnNum] or nil
						if gridItem.NextSelectionDown == nil and grid2DtoIndex[rowNum + 1] ~= nil then
							gridItem.NextSelectionDown = self.GridItems[#self.GridItems]
						end
						gridItem.NextSelectionLeft = nil
						gridItem.NextSelectionRight = nil
					else
						gridItem.NextSelectionLeft = grid2DtoIndex[rowNum] and grid2DtoIndex[rowNum][columnNum - 1] or nil
						gridItem.NextSelectionRight = grid2DtoIndex[rowNum] and grid2DtoIndex[rowNum][columnNum + 1] or nil
						if gridItem.NextSelectionRight == nil and grid2DtoIndex[0] and grid2DtoIndex[0][columnNum + 1] then
								gridItem.NextSelectionRight = self.GridItems[#self.GridItems]
						end
						gridItem.NextSelectionUp = nil
						gridItem.NextSelectionDown = nil
					end
				end
			end
		end
	end


	function this:Destroy()
		if self.Container then
			self.Container:Destroy()
		end
	end

	do
		local container = Utility.Create'ScrollingFrame'
		{
			Size = DEFAULT_WINDOW_SIZE;
			Name = "Container";
			BackgroundTransparency = 1;
			ClipsDescendants = true;
			ScrollingEnabled = false;
			ScrollBarThickness = 0;
			Selectable = false;
		}
		this.Container = container
		this.DefaultSelection = this.Container

		this.Container.Changed:connect(function(prop)
			if prop == 'AbsoluteSize' then
				this:RecalcLayout()
			end
		end)

		this:RecalcLayout()


		function this:FindAncestorGridItem(object)
			if object ~= nil then
				if self:ContainsItem(object) then
					return object
				end
				return self:FindAncestorGridItem(object.Parent)
			end
		end

		local lastSelectedObject = nil
		GuiService.Changed:connect(function(prop)
			if prop == 'SelectedCoreObject' then
				local selectedObject = this:FindAncestorGridItem(GuiService.SelectedCoreObject)
				if selectedObject and this:ContainsItem(selectedObject) then
					-- print(selectedObject.NextSelectionUp, selectedObject.NextSelectionDown, selectedObject.NextSelectionLeft, selectedObject.NextSelectionRight)

					local upDirection = (this.ScrollDirection == this.Enum.ScrollDirection.Vertical) and 'NextSelectionUp' or 'NextSelectionLeft'
					local downDirection = (this.ScrollDirection == this.Enum.ScrollDirection.Vertical) and 'NextSelectionDown' or 'NextSelectionRight'
					local upObject = selectedObject[upDirection]
					local downObject = selectedObject[downDirection]

					local nextPos, upPos, downPos;


					local gridItemSize = this:GetGridItemSize()
					local thisPos = this:GetCanvasPositionForOffscreenItem(selectedObject)

					if lastSelectedObject then
						local lastUpObject = lastSelectedObject[upDirection]
						local lastDownObject = lastSelectedObject[downDirection]

						if upObject and lastUpObject == selectedObject then
							upPos = this:GetCanvasPositionForOffscreenItem(upObject)
							upPos = upPos and upPos + gridItemSize / 2
						elseif downObject and lastDownObject == selectedObject then
							downPos = this:GetCanvasPositionForOffscreenItem(downObject)
							downPos = downPos and downPos - gridItemSize / 2
						end
					end

					if upPos and (upPos.Y < this.Container.CanvasPosition.Y or upPos.X < this.Container.CanvasPosition.X) then
						nextPos = upPos
						-- print('up' , nextPos , selectedObject.Position, lastSelectedObject and lastSelectedObject.Position)
					elseif downPos and (downPos.Y > this.Container.CanvasPosition.Y or downPos.X > this.Container.CanvasPosition.X) then
						nextPos = downPos
						-- print('down' , nextPos , selectedObject.Position, lastSelectedObject and lastSelectedObject.Position)
					else
						nextPos = thisPos
						-- print('this' , selectedObject.Name , nextPos , selectedObject.Position, lastSelectedObject and lastSelectedObject.Position, selectedObject.AbsolutePosition, this.Container.AbsolutePosition)
					end

					if nextPos then
						-- print("nextPos" , selectedObject.Name , nextPos)
						nextPos = Utility.ClampCanvasPosition(this.Container, nextPos)
						if thisPos then --and thisPos ~= nextPos then
							-- Sort of a hack to not snap on the last one
							if (upObject and downObject) then
								Utility.PropertyTweener(this.Container, 'CanvasPosition', thisPos, thisPos, 0, Utility.EaseOutQuad, true)
							end
						end
						Utility.PropertyTweener(this.Container, 'CanvasPosition', this.Container.CanvasPosition, nextPos, 0.2, Utility.EaseOutQuad, true)
					end
					lastSelectedObject = selectedObject
				else
					lastSelectedObject = nil
				end
			end
		end)
	end

	return this
end

return ScrollingGrid
