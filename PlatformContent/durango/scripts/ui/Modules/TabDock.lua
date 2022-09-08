-- Written by Kip Turner, Copyright ROBLOX 2015

local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")
local UserInputService = game:GetService("UserInputService")

local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))

local function CreateTabDock()
	local this = {}

	local Tabs = {}
	local SelectedTab = nil
	local SizeChangedConns = {}
	this.SelectedTabChanged = Utility.Signal()
	this.SelectedTabClicked = Utility.Signal()
	local guiServiceChangedCn = nil

	local TabContainer = Utility.Create'ImageButton'
	{
		Size = UDim2.new(1, 0, 0, 36);
		BackgroundTransparency = 1;
		Name = 'TabContainer';
	}

	local SelectionBorderObject = Utility.Create'ImageLabel'
	{
		Name = 'SelectionBorderObject';
		Size = UDim2.new(1,0,0,4);
		Position = UDim2.new(0,0,1,5);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.TabUnderlineColor;
		-- Image = 'rbxasset://textures/ui/SelectionBox.png';
		-- ScaleType = Enum.ScaleType.Slice;
		-- SliceCenter = Rect.new(19,19,43,43);
		BackgroundTransparency = 0;
	};

	local DownSelector = Utility.Create'ImageButton'
	{
		Size = UDim2.new(1, 0, 0, 36);
		BackgroundTransparency = 1;
		Name = 'DownSelector';
		Selectable = false;
		Parent = TabContainer;
	}

	DownSelector.SelectionGained:connect(function()
		if SelectedTab then
			GuiService.SelectedCoreObject = SelectedTab:GetGuiObject()
			this.SelectedTabClicked:fire(SelectedTab)
		end
	end)

	local function onGuiServiceChanged(prop)
		if prop == 'SelectedCoreObject' then
			if GuiService.SelectedCoreObject == TabContainer then
				local currentTab = this:GetSelectedTab()
				local currentTabItem = currentTab and currentTab:GetGuiObject()
				if currentTabItem then
					GuiService.SelectedCoreObject = currentTabItem
				end
			end

			local selectedObject = GuiService.SelectedCoreObject
			local focusedTab = this:FindFocusedTabByGuiObject(selectedObject)

			if SelectedTab and selectedObject ~= SelectedTab:GetGuiObject() then
				SelectedTab:OnClickRelease()
			end

			if focusedTab then
				this:SetSelectedTab(focusedTab)

				for _, inputObject in pairs(UserInputService:GetGamepadState(Enum.UserInputType.Gamepad1)) do
					if inputObject.KeyCode == Enum.KeyCode.ButtonA and inputObject.UserInputState == Enum.UserInputState.Begin then
						if SelectedTab then
							SelectedTab:OnClick()
						end
					end
				end

				ContextActionService:UnbindCoreAction("OnClickSelectedTab")
				ContextActionService:BindCoreAction("OnClickSelectedTab",
					function(actionName, inputState, inputObject)
						if inputState == Enum.UserInputState.Begin then
							if SelectedTab then
								SelectedTab:OnClick()
							end
						elseif inputState == Enum.UserInputState.End then
							if SelectedTab then
								SelectedTab:OnClickRelease()
							end
							this.SelectedTabClicked:fire(SelectedTab)
						end
					end,
					false,
					Enum.KeyCode.ButtonA)
			else
				ContextActionService:UnbindCoreAction("OnClickSelectedTab")
			end
		end
	end

	function this:FindFocusedTabByGuiObject(selectedObject)
		-- NOTE: This is a sort of cheater way of culling look-up checks
		if selectedObject and selectedObject:IsDescendantOf(TabContainer) then
			for _, currTab in pairs(Tabs) do
				local guiObject = currTab and currTab:GetGuiObject()
				if guiObject and guiObject == selectedObject then
					return currTab
				end
			end
		end
	end

	function this:IsFocused()
		local selectedObject = GuiService.SelectedCoreObject
		return self:FindFocusedTabByGuiObject(selectedObject) ~= nil
	end

	function this:SetSelectedTab(newSelectedTab)
		if newSelectedTab ~= SelectedTab then
			if SelectedTab then
				SelectedTab:SetSelected(false)
				SelectedTab:OnClickRelease()
			end

			SelectedTab = newSelectedTab

			if SelectedTab then
				SelectedTab:SetSelected(true)
				if self:IsFocused() then
					local currentTabItem = SelectedTab and SelectedTab:GetGuiObject()
					if currentTabItem then
						GuiService.SelectedCoreObject = currentTabItem
					end
				end
			end
			-- Fire tab selection event with the name of the new tab selection
			this.SelectedTabChanged:fire(SelectedTab)
		end
	end

	function this:Focus()
		if SelectedTab then
			SelectedTab:SetSelected(true)
			local currentTabItem = SelectedTab and SelectedTab:GetGuiObject()
			if currentTabItem then
				GuiService.SelectedCoreObject = currentTabItem
			end
		end
	end

	function this:GetSelectedTab()
		return SelectedTab
	end

	local arrangeCount = 0
	function this:ArrangeTabs()
		arrangeCount = arrangeCount + 1
		local currentCount = arrangeCount

		local x = 0
		for i, tabItem in pairs(Tabs) do
			local tabItemSize = tabItem:GetSize()
			local xSize = tabItemSize.X.Offset

			local spacing = GlobalSettings.TabItemSpacing
			if i == 1 then
				spacing = 0
			end
			-- Stop recursion in its tracks
			if currentCount == arrangeCount then
				tabItem:SetPosition(UDim2.new(0, x + spacing, 0, 0))

				local tabItemGuiObject = tabItem:GetGuiObject()
				if tabItemGuiObject then
					local prevItemGuiObject = Tabs[i-1] and Tabs[i-1]:GetGuiObject()
					local nextItemGuiObject = Tabs[i+1] and Tabs[i+1]:GetGuiObject()
					tabItemGuiObject.NextSelectionLeft = prevItemGuiObject
					tabItemGuiObject.NextSelectionRight = nextItemGuiObject
				end

			else
				return
			end
			x = x + spacing + xSize
		end
	end

	function this:FindTabIndex(tab)
		for i, currTab in pairs(Tabs) do
			if tab == currTab then
				return i
			end
		end
	end

	function this:GetNextTab()
		if SelectedTab then
			local index = this:FindTabIndex(SelectedTab)
			return index and Tabs[index + 1]
		end
	end

	function this:GetPreviousTab()
		if SelectedTab then
			local index = this:FindTabIndex(SelectedTab)
			return index and Tabs[index - 1]
		end
	end

	function this:AddTab(newTab)
		local existingIndex = self:FindTabIndex(newTab)
		if existingIndex then
			print("Not adding tab:" , newTab:GetName() , "because that tab already exists.")
			return
		end

		local guiObject = newTab and newTab:GetGuiObject()
		if guiObject then
			guiObject.SelectionImageObject = SelectionBorderObject
			guiObject.NextSelectionDown = DownSelector
		end

		table.insert(Tabs, newTab)
		newTab:SetParent(TabContainer)

		Utility.DisconnectEvent(SizeChangedConns[newTab])
		SizeChangedConns[newTab] = newTab.SizeChanged:connect(function()
			self:ArrangeTabs()
		end)

		this:ArrangeTabs()

		return newTab
	end

	function this:RemoveTab(tab)
		local removeIndex = self:FindTabIndex(tab)

		if removeIndex then
			table.remove(Tabs, removeIndex)
			if tab == SelectedTab then
				this:SetSelectedTab(nil)
			end
			Utility.DisconnectEvent(SizeChangedConns[tab])
			return true
		end
		return false
	end

	function this:SetPosition(newPosition)
		TabContainer.Position = newPosition
	end

	function this:SetParent(newParent)
		TabContainer.Parent = newParent
	end

	function this:ConnectEvents()
		onGuiServiceChanged('SelectedCoreObject')
		guiServiceChangedCn = GuiService.Changed:connect(onGuiServiceChanged)
	end

	function this:DisconnectEvents()
		if guiServiceChangedCn then
			guiServiceChangedCn:disconnect()
			guiServiceChangedCn = nil
		end
	end

	return this
end

return CreateTabDock
