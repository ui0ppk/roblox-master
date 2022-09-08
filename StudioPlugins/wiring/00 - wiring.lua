local on = false
local showingWires = false
local allWires = {}

---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
self.Deactivation:connect(function () Off() end)
toolbar = self:CreateToolbar("Wiring")
toolbarbutton = toolbar:CreateButton("", "Hook up CustomEvent and CustomEventReceivers", "wiring.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	else
		On()
	end
end)
allWiresButton = toolbar:CreateButton("", "Show current connections", "showWires.png")
allWiresButton.Click:connect(function()
	if showingWires then
		hideAllWires()
	else
		showAllWires()
	end
end)

local g = nil
local indicatorArrow = nil
local indicatorBox = nil
local storedTerminal = nil
local wires = {}
local WHITE_COLOR = Color3.new(1, 1, 1)
local BASE_WIRE_COLOR = BrickColor.new("Really black")
local BASE_WIRE_RADIUS = 0.0625
local HOVER_WIRE_COLOR = BrickColor.new("Really red")
local HOVER_WIRE_RADIUS = 0.125
local SOURCE_BUTTON_TEXT_COLOR = Color3.new(1, .5, 0)
local SOURCE_BUTTON_ICON_TEXTURE = "http://www.roblox.com/asset/?id=55130296"
local SOURCE_BUTTON_ICON_HOVER_TEXTURE = "http://www.roblox.com/asset/?id=56953981"
local SINK_BUTTON_TEXT_COLOR = Color3.new(0, 1, 0)
local SINK_BUTTON_ICON_TEXTURE = "http://www.roblox.com/asset/?id=55130274"
local SINK_BUTTON_ICON_HOVER_TEXTURE = "http://www.roblox.com/asset/?id=56953950"
local SELECT_CONNECTION = nil
local DOWN_ARROW_TEXTURE = "http://www.roblox.com/asset/?id=48972653"
local SOURCE_BADGE_TEXTURE = "http://www.roblox.com/asset/?id=60730993"
local SINK_BADGE_TEXTURE = "http://www.roblox.com/asset/?id=61334830"
local PENDING_LEAVE_CALLBACK = nil
local receiverTable = {}
local eventTable = {}
local adornmentTable = {}
local receiverBadgeCount = {}
local eventBadgeCount = {}

self:GetMouse().Button1Down:connect(function ()
	local target = self:GetMouse().Target
	if target ~= nil then
		while target.Parent ~= game.Workspace do
			target = target.Parent
		end
	end
	game.Selection:Set({target})
end)
self:GetMouse().KeyDown:connect(function (key)
	if string.byte(key) == 27 then
		game.Selection:Set({})
	end
end)

function getIndicator()
	if indicatorArrow == nil then
		indicatorArrow = Instance.new("BillboardGui", game:GetService("CoreGui"))
		indicatorArrow.Active = true
		indicatorArrow.AlwaysOnTop = true
		indicatorArrow.Size = UDim2.new(4, 0, 4, 0)
		indicatorArrow.StudsOffset = Vector3.new(0, 2, 0)

		local frame = Instance.new("Frame", indicatorArrow)
		frame.Active = true
		frame.BackgroundTransparency = 1
		frame.Size = UDim2.new(1, 0, 1, 0)

		local arrow = Instance.new("ImageLabel", frame)
		arrow.BackgroundTransparency = 1
		arrow.Image = DOWN_ARROW_TEXTURE
		arrow.Size = UDim2.new(1, 0, 1, 0)
	end
	return indicatorArrow
end

function getIndicatorBox()
	if indicatorBox == nil then
		indicatorBox = Instance.new("SelectionBox", game:GetService("CoreGui"))
		indicatorBox.Color = BrickColor.new("New Yeller")
		indicatorBox.Adornee = nil
	end
	return indicatorBox
end

function refreshScreenGui()
	if g ~= nil then
		g:Remove()
	end
	for idx, wire in ipairs(wires) do
		wire:Remove()
	end
	g = Instance.new("ScreenGui", game:GetService("CoreGui"))

	getIndicator().Adornee = nil
	getIndicatorBox().Adornee = nil

	if storedTerminal ~= nil then
		handleStoredTerminal(g)
	end

	selected = game.Selection:Get()
	if #selected == 0 then
		return
	elseif #selected > 1 then
		handleContainer(g, selected)
	elseif selected[1]:IsA("CustomEvent") then
		handleTerminal(g, selected[1], function(customEvent) return customEvent:GetAttachedReceivers() end)
		getIndicatorBox().Adornee = selected.Parent
	elseif selected[1]:IsA("CustomEventReceiver") then
		handleTerminal(g, selected[1], function(customEventReceiver) return { customEventReceiver.Source } end)
		getIndicatorBox().Adornee = selected.Parent
	elseif selected[1] ~= game.Workspace then
		handleContainer(g, selected)
	end
end

function On()
	RbxGui = LoadLibrary("RbxGui")
	
	setUpConfigurationService()

	self:Activate(true)
	SELECT_CONNECTION = game.Selection.SelectionChanged:connect(refreshScreenGui)
	refreshScreenGui()

	toolbarbutton:SetActive(true)
	on = true
end

function Off()
	destroyConfigurationService()

	storedTerminal = nil
	if g ~= nil then
		g:Remove()
		g = nil
	end
	for idx, wire in ipairs(wires) do
		wire:Remove()
	end
	wires = {}
	SELECT_CONNECTION:disconnect()
	getIndicator().Adornee = nil

	toolbarbutton:SetActive(false)
	on = false
end

function IsChildOfWorkspace(instance)
	return instance ~= nil and (instance == game.Workspace or IsChildOfWorkspace(instance.Parent))
end

function findModel(part)
	local origPart = part
	while part ~= nil do
		if part.className == "Model" then
			return part
		elseif part.Name == "Workspace" or part.Name == "game" then
			return origPart
		end
		part = part.Parent
	end

	return nil
end

----------------------------------------------------------
-- Left Side Popup when CustomEvent/Receiver is selected

function applyTextSize(label, text)
	label.Size = UDim2.new(0, label.TextBounds.x, 0, label.TextBounds.y)
end

function handleConnect(terminal)
	if storedTerminal == nil then
		storedTerminal = terminal
		return
	else
		local storedIsSource = storedTerminal:IsA("CustomEvent")
		local newIsSource = terminal:IsA("CustomEvent")
		if storedIsSource and not newIsSource then
			terminal.Source = storedTerminal
			storedTerminal = nil
			game:GetService("ChangeHistoryService"):SetWaypoint("Wiring")
		elseif not storedIsSource and newIsSource then
			storedTerminal.Source = terminal
			storedTerminal = nil
			game:GetService("ChangeHistoryService"):SetWaypoint("Wiring")
		else
			storedTerminal = terminal
		end
	end
end

function disconnectHelper(terminal1, terminal2)
	if terminal1:IsA("CustomEvent") then
		terminal2.Source = nil
	else
		terminal1.Source = nil
	end
	game:GetService("ChangeHistoryService"):SetWaypoint("Disconnect wiring")
end

function createModelLine(frame, yOffset, terminal)
	local model = findParentUnderWorkspace(terminal)
	
	local button = Instance.new("TextButton", frame)
	button.Text = "Top parent: " .. model.Name
	button.Font = Enum.Font.Arial
	button.FontSize = Enum.FontSize.Size14
	button.BackgroundTransparency = .5
	button.TextColor3 = WHITE_COLOR
	button.BorderSizePixel = 0
	applyTextSize(button)
	button.Position = UDim2.new(0, 0, 0, yOffset)
	button.MouseButton1Click:connect(function () game.Selection:Set({model}) end)

	return button
end

function createParentLine(frame, yOffset, terminal)
	local parent = terminal.Parent

	local button = Instance.new("TextButton", frame)
	button.Text = "Parent: " .. parent.Name
	button.Font = Enum.Font.Arial
	button.FontSize = Enum.FontSize.Size14
	button.BackgroundTransparency = .5
	button.TextColor3 = WHITE_COLOR
	button.BorderSizePixel = 0
	applyTextSize(button)
	button.Position = UDim2.new(0, 0, 0, yOffset)
	button.MouseButton1Click:connect(function () game.Selection:Set({parent}) end)

	return button
end

function createTerminalElement(type, frame, font, yOffset, terminal, dontShowParent)
	local button = Instance.new(type, frame)

	local icon = Instance.new("ImageLabel", button)
	icon.Position = UDim2.new(0, 0, 0, -8)
	icon.Size = UDim2.new(0, 35, 0 , 35)
	icon.BackgroundTransparency = 1
	
	if dontShowParent then
		button.Text = terminal.Name
	else 
		button.Text = terminal.Name .. " (" .. terminal.Parent.Name .. ")"
	end
	button.TextXAlignment = 1 -- right
	button.Font = font
	button.FontSize = Enum.FontSize.Size18
	button.BorderSizePixel = 0
	button.BackgroundTransparency = .5
	applyTextSize(button)
	button.Position = UDim2.new(0, 0, 0, yOffset)

	if terminal:IsA("CustomEvent") then
		button.TextColor3 = SOURCE_BUTTON_TEXT_COLOR
		icon.Image = SOURCE_BUTTON_ICON_TEXTURE
	else
		button.TextColor3 = SINK_BUTTON_TEXT_COLOR
		icon.Image = SINK_BUTTON_ICON_TEXTURE
	end

	button.Size = UDim2.new(0, button.Size.X.Offset + 35 + 10, 0, button.Size.Y.Offset)

	return button
end

function createTextLabelHeader(labelParent, yOffset, text)
	local label = Instance.new("TextLabel", labelParent)
	label.Text = text
	label.Font = Enum.Font.ArialBold
	label.FontSize = Enum.FontSize.Size18
	label.TextColor3 = WHITE_COLOR
	label.BorderSizePixel = 0
	label.BackgroundTransparency = 1
	label.Position = UDim2.new(0, 0, 0, yOffset)
	applyTextSize(label)
	return label
end

function createTextLabel(labelParent, yOffset, childName, parentName)
	local label = Instance.new("TextButton", labelParent)
	label.Text = childName .. " (" .. parentName .. ")"
	label.Font = Enum.Font.Arial
	label.FontSize = Enum.FontSize.Size18
	label.TextColor3 = WHITE_COLOR
	label.BorderSizePixel = 0
	label.BackgroundTransparency = .5
	-- label.Position = UDim2.new(0, 0, 0, yOffset)
	applyTextSize(label)
	return label
end

function createDisconnectButton(frame, xOffset, yOffset, rcb)
	local button = Instance.new("TextButton", frame)
	button.Text = "Disconnect"
	button.Font = Enum.Font.ArialBold
	button.FontSize = Enum.FontSize.Size18
	button.TextColor3 = WHITE_COLOR
	applyTextSize(button)
	button.BorderSizePixel = 0
	button.BackgroundTransparency = 0.5
	button.Position = UDim2.new(1, -button.Size.X.Offset - 2, 0, 0)
	button.MouseButton1Click:connect(rcb)
	return button
end

function createWire(terminal1, terminal2)
	local from = nil
	local to = nil
	if terminal1:IsA("CustomEvent") then
		from = terminal1.Parent
		to = terminal2.Parent
	else
		from = terminal2.Parent
		to = terminal1.Parent
	end

	local wire = Instance.new("FloorWire", game:GetService("CoreGui"))
	wire.From = from
	wire.To = to
	wire.Color = BASE_WIRE_COLOR
	wire.WireRadius = BASE_WIRE_RADIUS
	table.insert(wires, wire)

	if showingWires then
		local wireCopy = Instance.new("FloorWire", game:GetService("CoreGui"))
		wireCopy.From = wire.From
		wireCopy.To = wire.To
		wireCopy.Color = wire.Color
		wireCopy.WireRadius = wire.WireRadius
		table.insert(allWires, wireCopy)
	end
	return wire
end

function createNoneLabel(frame, yOffset)
	local label = Instance.new("TextLabel", frame)
	label.Text = "-- none --"
	label.Font = Enum.Font.Arial
	label.FontSize = Enum.FontSize.Size18
	label.TextColor3 = WHITE_COLOR
	label.BorderSizePixel = 0
	label.BackgroundTransparency = 1
	label.Position = UDim2.new(0, 0, 0, yOffset)
	applyTextSize(label)
	return label
end

function createConnectButton(frame, yOffset, terminal)
	local button = Instance.new("TextButton", frame)

	if storedTerminal == nil then
		button.Text = "Add Connection"
	else
		local storedIsSource = storedTerminal:IsA("CustomEvent")
		local terminalIsSource = terminal:IsA("CustomEvent")
		if storedIsSource ~= terminalIsSource then
			button.Text = "Connect to " .. storedTerminal.Name .. " (" .. storedTerminal.Parent.Name .. ")"
		else
			button.Text = "Add Connection"
		end
	end
	button.Font = Enum.Font.ArialBold
	button.FontSize = Enum.FontSize.Size18
	button.TextColor3 = WHITE_COLOR
	applyTextSize(button)
	button.BorderSizePixel = 0
	button.BackgroundTransparency = 0.5
	button.Position = UDim2.new(0, 0, 0, yOffset)

	return button
end

function handleTerminal(screenGui, terminal, getAttached)
	local frame = Instance.new("Frame", screenGui)
	frame.Style = Enum.FrameStyle.RobloxRound
	frame.Visible = true
	local yOffset = 0
	local maxX = 0

	local title = createTerminalElement("TextLabel", frame, Enum.Font.ArialBold, yOffset, terminal, true)
	title.BackgroundTransparency = 1
	yOffset = yOffset + title.Size.Y.Offset + 2
	maxX = math.max(maxX, title.Size.X.Offset)

	local modelLine = createModelLine(frame, yOffset, terminal)
	yOffset = yOffset + modelLine.Size.Y.Offset + 2
	maxX = math.max(maxX, modelLine.Size.X.Offset)

	local parentLine = createParentLine(frame, yOffset, terminal)
	yOffset = yOffset + parentLine.Size.Y.Offset + 12.5
	maxX = math.max(maxX, parentLine.Size.X.Offset)

	local header = createTextLabelHeader(frame, yOffset, "Connections")
	yOffset = yOffset + header.Size.Y.Offset + 3
	maxX = math.max(maxX, header.Size.X.Offset)

	local attached = getAttached(terminal)

	local scrollHolder = Instance.new("Frame", frame)
	local scrollYOffset = 0
	local scrollFrame, scrollUp, scrollDown, recalculateScroll = RbxGui.CreateScrollingFrame()
	scrollFrame.Parent = scrollHolder
	scrollUp.Parent = scrollHolder
	scrollDown.Parent = scrollHolder
	for idx, other in ipairs(attached) do
		if IsChildOfWorkspace(other) then
			local rowFrame = Instance.new("Frame", scrollFrame)
			rowFrame.BackgroundTransparency = 1
			rowFrame.BorderSizePixel = 0

			local label = createTextLabel(rowFrame, scrollYOffset, other.Name, other.Parent.Name)
			label.MouseButton1Click:connect(function () game.Selection:Set({other}) end)
			label.MouseEnter:connect(function() getIndicator().Adornee = other.Parent end)
			label.MouseLeave:connect(function() getIndicator().Adornee = nil end)

			local button = createDisconnectButton(rowFrame, label.Size.X.Offset, scrollYOffset,
				function()
					disconnectHelper(terminal, other)
					refreshScreenGui()
				end)
			local wire = createWire(terminal, other)
			label.MouseEnter:connect(function() wire.WireRadius = HOVER_WIRE_RADIUS end)
			label.MouseLeave:connect(function() wire.WireRadius = BASE_WIRE_RADIUS end)
			button.MouseEnter:connect(function()
				wire.Color = HOVER_WIRE_COLOR
				wire.WireRadius = HOVER_WIRE_RADIUS
			end)
			button.MouseLeave:connect(function()
				wire.Color = BASE_WIRE_COLOR
				wire.WireRadius = BASE_WIRE_RADIUS
			end)

			rowFrame.Size = UDim2.new(1, 0, 0, label.Size.Y.Offset + 5)
			scrollYOffset = scrollYOffset + rowFrame.Size.Y.Offset
			maxX = math.max(maxX, label.Size.X.Offset + 3 + button.Size.X.Offset)
		end
	end

	if #attached == 0 then
		local label = createNoneLabel(frame, yOffset)
		scrollYOffset = scrollYOffset + label.Size.Y.Offset + 3
		maxX = math.max(maxX, label.Size.X.Offset)
	end

	if #attached < 4 then
		scrollUp.Visible = false
		scrollDown.Visible = false
	end

	scrollFrame.Size = UDim2.new(1, -scrollUp.Size.X.Offset, 1, 0)
	scrollUp.Position = UDim2.new(1, -scrollUp.Size.X.Offset, 0, 0)
	scrollDown.Position = UDim2.new(1, -scrollDown.Size.X.Offset, 1, -scrollDown.Size.Y.Offset)
	scrollHolder.Position = UDim2.new(0, 0, 0, yOffset)
	scrollHolder.Size = UDim2.new(1, 0, 0, math.min(70, scrollYOffset))

	scrollHolder.BackgroundTransparency = 1
	scrollHolder.BorderSizePixel = 0
	scrollFrame.BackgroundTransparency = 1
	scrollFrame.BorderSizePixel = 0

	-- give room for scroll bars
	maxX = maxX + scrollUp.Size.X.Offset + 3
	yOffset = yOffset + scrollHolder.Size.Y.Offset

	-- add another gap before the connect button
	yOffset = yOffset + 12.5
	local connect = createConnectButton(frame, yOffset, terminal)
	connect.MouseButton1Click:connect(function ()
		handleConnect(terminal)
		refreshScreenGui()
	end)
	maxX = math.max(maxX, connect.Size.X.Offset)


	frame.Size = UDim2.new(0, maxX + 17.5, 0, yOffset + connect.Size.Y.Offset + 17.5)
	recalculateScroll()
	frame.BackgroundTransparency = 0.5
	delay(0, function()
		local etime = time() + 10
		while time() < etime do
			recalculateScroll()
			wait(.5)
		end
	end)

end

--------------------------------------------------
-- Left Side Popup when non-terminal is selected

function collectTerminalsHelper(root, terminals)
	if root:IsA("CustomEvent") or root:IsA("CustomEventReceiver") then
		table.insert(terminals, root)
	end
	for idx, child in ipairs(root:GetChildren()) do
		collectTerminals(child, terminals)
	end
end

function collectTerminals(root, terminals)
	collectTerminalsHelper(root, terminals)
	table.sort(terminals, function (item1, item2)
		local item1IsSource = item1:IsA("CustomEvent")
		local item2IsSource = item2:IsA("CustomEvent")
		if item1IsSource and not item2IsSource then
			return true
		elseif item2IsSource and not item1IsSource then
			return false
		else
			return item1.Name < item2.Name
		end
	end)
end

function handleContainer(screenGui, root)
	local frame = Instance.new("Frame", screenGui)
	frame.Style = Enum.FrameStyle.RobloxRound
	frame.Visible = true
	local yOffset = 0
	local maxX = 0

	local header = createTextLabelHeader(frame, yOffset, "Custom Events and Receivers")
	yOffset = yOffset + header.Size.Y.Offset + 3
	maxX = header.Size.X.Offset

	local terminals = {}
	for idx, sel in ipairs(root) do
		if sel ~= game.Workspace then
			collectTerminals(sel, terminals)
		end
	end

	local scrollHolder = Instance.new("Frame", frame)
	scrollHolder.BackgroundTransparency = 1
	scrollHolder.BorderSizePixel = 0
	local scrollFrame, scrollUp, scrollDown, recalculateScroll = RbxGui.CreateScrollingFrame()
	scrollFrame.BackgroundTransparency = 1
	scrollFrame.BorderSizePixel = 0
	scrollFrame.Parent = scrollHolder
	scrollUp.Parent = scrollHolder
	scrollDown.Parent = scrollHolder
	scrollHolder.Position = UDim2.new(0, 0, 0, yOffset)

	for idx, terminal in ipairs(terminals) do
		local button = createTerminalElement("TextButton", frame, Enum.Font.Arial, yOffset, terminal)
		button.MouseButton1Click:connect(function ()
			game.Selection:Set({terminal})
		end)
		button.MouseEnter:connect(function () getIndicator().Adornee = terminal.Parent end)
		button.MouseLeave:connect(function () getIndicator().Adornee = nil end)
		yOffset = yOffset + button.Size.Y.Offset + 3
		maxX = math.max(maxX, button.Size.X.Offset)
	end

	if #terminals == 0 then
		local label = createNoneLabel(frame, yOffset)
		yOffset = yOffset + label.Size.Y.Offset + 3
		maxX = math.max(maxX, label.Size.X.Offset)
	end

	if #terminals < 2 then
		scrollUp.Visible = false
		scrollDown.Visible = false
	end

	scrollFrame.Size = UDim2.new(1, -scrollUp.Size.X.Offset, 1, 0)
	scrollUp.Position = UDim2.new(1, -scrollUp.Size.X.Offset, 0, 0)
	scrollDown.Position = UDim2.new(1, -scrollDown.Size.X.Offset, 1, -scrollDown.Size.Y.Offset)
	scrollHolder.Size = UDim2.new(1, 0, 0, yOffset - (header.Size.Y.Offset + 3))
	recalculateScroll()
	maxX = maxX + scrollUp.Size.X.Offset

	frame.Size = UDim2.new(0, maxX + 17.5, 0, yOffset + 17.5)
	frame.BackgroundTransparency = 0.5
end

----------------------------------------------------------
-- Right side gui when connecting a CustomEvent/Receiver

function findParentUnderWorkspace(instance)
	while instance.Parent ~= game.Workspace and instance.Parent ~= nil do
		instance = instance.Parent
	end
	return instance
end

function createCancelButton(frame, yOffset)
	local button = Instance.new("TextButton", frame)
	button.Text = "Cancel"
	button.Font = Enum.Font.ArialBold
	button.FontSize = Enum.FontSize.Size18
	button.TextColor3 = WHITE_COLOR
	applyTextSize(button)
	button.BorderSizePixel = 0
	button.BackgroundTransparency = 0.5
	button.Position = UDim2.new(1, -button.Size.X.Offset, 0, yOffset)
	button.MouseButton1Click:connect(function ()
		storedTerminal = nil
		refreshScreenGui()
	end)
	return button
end

function handleStoredTerminal(screenGui)
	if storedTerminal == nil then return end

	local frame = Instance.new("Frame", screenGui)
	frame.Style = Enum.FrameStyle.RobloxRound
	frame.Visible = true
	local yOffset = 0
	local maxX = 0

	local terminalButton = createTerminalElement("TextButton", frame, Enum.Font.Arial, yOffset, storedTerminal)
	terminalButton.MouseButton1Click:connect(function () game.Selection:Set({storedTerminal}) end)
	yOffset = yOffset + terminalButton.Size.Y.Offset + 3
	maxX = math.max(maxX, terminalButton.Size.X.Offset)

	local cancelButton = createCancelButton(frame, yOffset)
	yOffset = yOffset + cancelButton.Size.Y.Offset
	maxX = math.max(maxX, cancelButton.Size.X.Offset)

	frame.Size = UDim2.new(0, maxX + 17.5, 0, yOffset + 17.5)
	frame.Position = UDim2.new(1, -frame.Size.X.Offset, 0, 0)
	frame.BackgroundTransparency = 0.5			
end

------------------------
-- Show/Hide All Wires
function showWiresHelper(terminal, othersFunc, completedTerminals)
	local terminalIsSource = terminal:IsA("CustomEvent")
	for idx, other in ipairs(othersFunc(terminal)) do
		if completedTerminals[other] == nil and IsChildOfWorkspace(other) then
			local wire = Instance.new("FloorWire", game:GetService("CoreGui"))
			if terminalIsSource then
				wire.From = terminal.Parent
				wire.To = other.Parent
			else
				wire.From = other.Parent
				wire.To = terminal.Parent
			end
			wire.Color = BASE_WIRE_COLOR
			wire.WireRadius = BASE_WIRE_RADIUS
			table.insert(allWires, wire)
		end
	end
	completedTerminals[terminal] = true
end


function showAllWires()
	allWiresButton:SetActive(true)
	terminals = {}
	collectTerminals(game.Workspace, terminals)

	local completedTerminals = {}
	for idx, terminal in ipairs(terminals) do
		local othersFunc = nil
		if terminal:IsA("CustomEvent") then
			othersFunc = function (source) return source:GetAttachedReceivers() end
		elseif terminal:IsA("CustomEventReceiver") then
			othersFunc = function (sink) return { sink.Source } end
		end
		if othersFunc ~= nil then
			showWiresHelper(terminal, othersFunc, completedTerminals)
		end
	end
	showingWires = true
end

function hideAllWires()
	allWiresButton:SetActive(false)
	for idx, wire in ipairs(allWires) do
		wire:Remove()
	end
	allWires = {}
	showingWires = false
end

function refreshAllWires()
	hideAllWires()
	showAllWires()
end

game.DescendantAdded:connect(function (newChild)
	if newChild:IsA("CustomEvent") or newChild:IsA("CustomEventReceiver") then
		-- refresh screen gui now, because we may be showing the parent dialog
		if on then refreshScreenGui() end

		-- Attach a listener to the terminal to refresh Guis when properties changed.
		-- This will catch re-wiring because connection changes show up as property
		-- changes as well.
		newChild.Changed:connect(function()
			if on then
				if showingWires then
					hideAllWires()
					showAllWires()
				end
				refreshScreenGui()
			end
		end)

	end
end)

game.DescendantRemoving:connect(function (removed)
	-- when a descendant is removed, refresh the screen gui
	if on and (removed:IsA("CustomEvent") or removed:IsA("CustomEventReceiver")) then
		refreshScreenGui()
	end
end)
	
	
	function findBillboard(guiTable)
	if not guiTable then return end

	for i = 1, #guiTable do
		if guiTable[i] and guiTable[i]:IsA("BillboardGui") then
			return guiTable[i]
		end
	end
end

function getBillboard(adornee)
	local guiKey = adornee

	
	local billboard = findBillboard(adornmentTable[guiKey])
	if not billboard then
		local screen = Instance.new("BillboardGui")
		screen.Name = adornee.Name .. "BadgeGUI"
		screen.Size = UDim2.new(1.5,0,1.5,0)
		screen.Enabled = true
		screen.Active = true
		screen.AlwaysOnTop = true
		screen.ExtentsOffset = Vector3.new(0,0,0)
		screen.Adornee = adornee
		screen.Parent = game:GetService("CoreGui")

		if not adornmentTable[guiKey] then return end
		table.insert(adornmentTable[guiKey],screen)

		local badgeFrame = Instance.new("Frame")
		badgeFrame.Name = "BadgeFrame"
		badgeFrame.Size = UDim2.new(2,0,1,0)
		badgeFrame.Position = UDim2.new(-0.5,0,0,0)
		badgeFrame.BackgroundTransparency = 1
		badgeFrame.Parent = screen

		return screen
	end

	return billboard
end

function repositionBadges(badgeFrame)
	local badges = badgeFrame:GetChildren()
	if #badges == 1 then
		badges[1].Position = UDim2.new(0.25,0,0,0)
	elseif #badges == 2 then
		badges[1].Position = UDim2.new(0,0,0,0)
		badges[2].Position = UDim2.new(0.5,0,0)
	end
end

function hasBadge(adornee, type)
	local screen = getBillboard(adornee)
	return screen:FindFirstChild(type .. "Badge",true)
end

function removeBadge(adornee, type)
	local screen = getBillboard(adornee)
	local badge = screen:FindFirstChild(type .. "Badge",true)
	if badge then badge:remove() end
end

function createBadge(adornee,type)
	local screen = getBillboard(adornee)

	local wiringBadge = Instance.new("ImageLabel")
	wiringBadge.Name = type .. "Badge"
	wiringBadge.BackgroundTransparency = 1
	if type == "Receiver" then
		wiringBadge.Image = SOURCE_BADGE_TEXTURE
	else
		wiringBadge.Image = SINK_BADGE_TEXTURE
	end

	wiringBadge.Position = UDim2.new(0.25,0,0,0)
	wiringBadge.Size = UDim2.new(0.5,0,1,0)
	wiringBadge.Parent = screen.BadgeFrame
	wiringBadge.Changed:connect(function(prop)
		if prop == "AbsoluteSize" then
			if wiringBadge.AbsoluteSize.X < 10 then
				wiringBadge.Visible = false
			else
				wiringBadge.Visible = true
			end
		end
	end)

	repositionBadges(screen.BadgeFrame)
end

function upAdorneeCount(adornee,type)
	local typeLower = string.lower(type)
	if typeLower == "receiver" then
		if not receiverBadgeCount[adornee] then
			receiverBadgeCount[adornee] = 1
		else
			receiverBadgeCount[adornee] = receiverBadgeCount[adornee] + 1
		end
	elseif typeLower == "event" then
		if not eventBadgeCount[adornee] then
			eventBadgeCount[adornee] = 1
		else
			eventBadgeCount[adornee] = eventBadgeCount[adornee] + 1
		end
	end
end

function downAdorneeCount(adornee,type)
	local typeLower = string.lower(type)
	if typeLower == "receiver" then
		if receiverBadgeCount[adornee] then
			receiverBadgeCount[adornee] = receiverBadgeCount[adornee] - 1
			if receiverBadgeCount[adornee] < 1 then
				receiverBadgeCount[adornee] = nil
			end
		end
	elseif typeLower == "event" then
		if eventBadgeCount[adornee] then
			eventBadgeCount[adornee] = eventBadgeCount[adornee] - 1
			if eventBadgeCount[adornee] < 1 then
				eventBadgeCount[adornee] = nil
			end
		end
	end
end

function createAdornment(adornee,adornColor,type)
	upAdorneeCount(adornee,type)

	if receiverBadgeCount[adornee] == 1 or eventBadgeCount[adornee] == 1 then
		local box = Instance.new("SelectionBox")
		box.Color = adornColor
		box.Name = adornee.Name .. "Selection" .. tostring(type)
		box.Adornee = adornee
		box.Transparency = 0.5
		box.Parent = game:GetService("CoreGui")
		if not adornmentTable[adornee] then
			adornmentTable[adornee] = {}
		end

		table.insert(adornmentTable[adornee],box)

		if not hasBadge(adornee,type) then
			createBadge(adornee,type)
		end
	end
end

function doRemoveAdornment(adornee, type)
	local key = adornee
	if not adornmentTable[key] then return end
	for i = 1, #adornmentTable[key] do
		if adornmentTable[key] and adornmentTable[key][i] then
			if string.find(adornmentTable[key][i].Name,type) then
				adornmentTable[key][i]:remove()
				adornmentTable[key][i] = nil
			end
		end
	end
end

function removeAdornment(adornee, type)
	downAdorneeCount(adornee,type)

	if type == "Receiver" then
		if not receiverBadgeCount[adornee]then
			removeBadge(adornee, type)
			doRemoveAdornment(adornee, type)
		end
	elseif type == "Event" then
		if not eventBadgeCount[adornee] then
			removeBadge(adornee, type)
			doRemoveAdornment(adornee, type)
		end
	end
end

function eventReceiverAdded(receiver,wirePartCount)
	if isRestricted then
		if not inBaseplate(receiver) then return wirePartCount end
	end
	receiverTable[receiver] = findModel(receiver.Parent)
	createAdornment(receiverTable[receiver], BrickColor.new("Lime green"), "Receiver")

	if wirePartCount then
		return wirePartCount + 1
	else
		return 0
	end

end

function eventAdded(event,wirePartCount)
	if isRestricted then
		if not inBaseplate(event) then return wirePartCount end
	end
	eventTable[event] = findModel(event.Parent)
	createAdornment(eventTable[event], BrickColor.new("Bright orange"), "Event")

	if wirePartCount then
		return wirePartCount + 1
	else
		return 0
	end
end

function eventReceiverRemoved(receiver)
	if not receiverTable[receiver] then return end
	
	removeAdornment(receiverTable[receiver],"Receiver")
	receiverTable[receiver] = nil
end

function eventRemoved(event)
	if not eventTable[event] then return end

	removeAdornment(eventTable[event], "Event")
	eventTable[event] = nil
end

function setUpConfigurationService()
	local wirePartCount = 0
	ServiceConnections = {}
	local collectionService = game:GetService("CollectionService")

	-- first lets check if anything already exists
	local receivers = collectionService:GetCollection("CustomEventReceiver")
	if receivers then
		for pos, receiver in pairs(receivers) do
			wirePartCount = eventReceiverAdded(receiver, wirePartCount)
		end
	end

	local events = collectionService:GetCollection("CustomEvent")
	if events then
		for pos, event in pairs(events) do
			wirePartCount = eventAdded(event, wirePartCount)
		end
	end

	-- Now lets listen for any future additions/removals
	ServiceConnections[#ServiceConnections+1] = collectionService.ItemAdded:connect(function(instance)
		if instance:IsA("CustomEventReceiver") then
			eventReceiverAdded(instance)
		elseif instance:IsA("CustomEvent") then
			eventAdded(instance)
		end 
	end)
	ServiceConnections[#ServiceConnections+1] = collectionService.ItemRemoved:connect(function(instance)
		if instance:IsA("CustomEventReceiver") then
			eventReceiverRemoved(instance)
		elseif instance:IsA("CustomEvent") then
			eventRemoved(instance)
		end
	end)

	return wirePartCount
end

function destroyConfigurationService()
	-- first lets destroy the collection service
	for index, connection in pairs(ServiceConnections) do
		connection:disconnect()
	end
	ServiceConnections = {}

	-- now lets remove all of our collection service objects that were generated
	for event, object in pairs(eventTable) do
		eventRemoved(event)
	end
	for eventReceiver, object in pairs(receiverTable) do
		eventReceiverRemoved(eventReceiver)
	end
end
