while game == nil do
	wait(1/30)
end

-----------------
--DEFAULT VALUES-
-----------------
loaded = false
-- True if the plugin is on, false if not.
on = false

---------------
--PLUGIN SETUP-
---------------
plugin = PluginManager():CreatePlugin()
mouse = plugin:GetMouse()
mouse.Button1Down:connect(function() onClicked(mouse) end)
toolbar = plugin:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("Remover", "Remover", "destroyer.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	elseif loaded then
		On()
	end
end)

game:WaitForChild("Workspace")
game.Workspace:WaitForChild("Terrain")

local c = game.Workspace.Terrain
local SetCell = c.SetCell
local GetCell = c.GetCell
local WorldToCellPreferSolid = c.WorldToCellPreferSolid
local CellCenterToWorld = c.CellCenterToWorld
local AutoWedge = c.AutowedgeCells

-- What color to use for the mouse highlighter.
mouseHighlightColor = BrickColor.new("Really red")

-- Used to create a highlighter that follows the mouse.
-- It is a class mouse highlighter.  To use, call MouseHighlighter.Create(mouse) where mouse is the mouse to track.
MouseHighlighter = {}
MouseHighlighter.__index = MouseHighlighter

-- Create a mouse movement highlighter.
-- plugin - Plugin to get the mouse from.
function MouseHighlighter.Create(mouseUse)
	local highlighter = {}
	
	local mouse = mouseUse
	highlighter.OnClicked = nil
	highlighter.mouseDown = false
	
	-- Store the last point used to draw.
	highlighter.lastUsedPoint = nil
	
	-- Will hold a part the highlighter will be attached to.  This will be moved where the mouse is.
	highlighter.selectionPart = nil

	-- Hook the mouse up to check for movement.
	mouse.Move:connect(function() MouseMoved() end)	
	
	mouse.Button1Down:connect(function() highlighter.mouseDown = true end)
	mouse.Button1Up:connect(function() highlighter.mouseDown = false
                                      end)
	
	
	-- Create the part that the highlighter will be attached to.
	highlighter.selectionPart = Instance.new("Part")
	highlighter.selectionPart.Name = "SelectionPart"
	highlighter.selectionPart.Archivable = false
	highlighter.selectionPart.Transparency = 1
	highlighter.selectionPart.Anchored = true
	highlighter.selectionPart.Locked = true
	highlighter.selectionPart.CanCollide = false
	highlighter.selectionPart.FormFactor = Enum.FormFactor.Custom

	highlighter.selectionBox = Instance.new("SelectionBox")
	highlighter.selectionBox.Archivable = false
	highlighter.selectionBox.Color = mouseHighlightColor
	highlighter.selectionBox.Adornee = highlighter.selectionPart
	mouse.TargetFilter = highlighter.selectionPart	
	setmetatable(highlighter, MouseHighlighter)

	-- Function to call when the mouse has moved.  Updates where to display the highlighter.
	function MouseMoved()
		if on then
			UpdatePosition(mouse.Hit)
		end
	end

	-- Update where the highlighter is displayed.
	-- position - Where to display the highlighter, in world space.
	function UpdatePosition(position)
		if not position then 
			return 
		end
		
		if not mouse.Target then 
			highlighter.selectionBox.Parent = nil
			return 
		end
		
		-- NOTE:
		-- Change this gui to be the one you want to use.
		highlighter.selectionBox.Parent = game:GetService("CoreGui")
		
		local vectorPos = Vector3.new(position.x,position.y,position.z)
		local cellPos = WorldToCellPreferSolid(c, vectorPos)

		local regionToSelect = nil

		local lowVec = CellCenterToWorld(c, cellPos.x , cellPos.y - 1, cellPos.z)
		local highVec = CellCenterToWorld(c, cellPos.x, cellPos.y + 1, cellPos.z)
		regionToSelect = Region3.new(lowVec, highVec)

		highlighter.selectionPart.Size = regionToSelect.Size - Vector3.new(-4, 4, -4)
		highlighter.selectionPart.CFrame = regionToSelect.CFrame
		
		if nil ~= highlighter.OnClicked and highlighter.mouseDown then
			if nil == highlighter.lastUsedPoint then 
				highlighter.lastUsedPoint = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
			else
				cellPos = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
				
				holdChange = cellPos - highlighter.lastUsedPoint
			
				-- Require terrain.
				if 0 == GetCell(c, cellPos.X, cellPos.Y, cellPos.Z).Value then
					return
				else
					highlighter.lastUsedPoint = cellPos
				end

			end
		end		
	end	
	
	return highlighter
end

-- Hide the highlighter.
function MouseHighlighter:DisablePreview()
	self.selectionBox.Parent = nil
end

-- Show the highlighter.
function MouseHighlighter:EnablePreview()
	self.selectionBox.Parent = game:GetService("CoreGui") -- This will make it not show up in workspace.
end	

-- Create the mouse movement highlighter.
mouseHighlighter = MouseHighlighter.Create(mouse)

-- Create a standard text label.  Use this for all lables in the popup so it is easy to standardize.
-- labelName - What to set the text label name as.
-- pos    	 - Where to position the label.  Should be of type UDim2.
-- size   	 - How large to make the label.	 Should be of type UDim2.
-- text   	 - Text to display.
-- parent 	 - What to set the text parent as.
-- Return:
-- Value is the created label.
function CreateStandardLabel(labelName,
                             pos,
							 size,
							 text,
							 parent)
	local label = Instance.new("TextLabel", parent)
	label.Name = labelName
	label.Position = pos
	label.Size = size
	label.Text = text
	label.TextColor3 = Color3.new(0.95, 0.95, 0.95)
	label.Font = Enum.Font.ArialBold
	label.FontSize = Enum.FontSize.Size14
	label.TextXAlignment = Enum.TextXAlignment.Left
	label.BackgroundTransparency = 1	
	label.Parent = parent	
	
	return label
end

------
--GUI-
------
--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))
g.Name = 'RemoverGui'

-- UI gui load.  Required for sliders.
local RbxGui = LoadLibrary("RbxGui")

-- Store properties here.
local properties = {autoWedgeEnabled = false}

-- Gui frame for the plugin.
removerPropertiesDragBar, removerFrame, removerHelpFrame, removerCloseEvent = RbxGui.CreatePluginFrame("Remover",UDim2.new(0,143,0,40),UDim2.new(0,0,0,0),false,g)
removerPropertiesDragBar.Visible = false
removerCloseEvent.Event:connect(function (  )
	Off()
end)


removerHelpFrame.Size = UDim2.new(0,160,0,60)

local removerHelpText = Instance.new("TextLabel",removerHelpFrame)
removerHelpText.Name = "HelpText"
removerHelpText.Font = Enum.Font.ArialBold
removerHelpText.FontSize = Enum.FontSize.Size12
removerHelpText.TextColor3 = Color3.new(227/255,227/255,227/255)
removerHelpText.TextXAlignment = Enum.TextXAlignment.Left
removerHelpText.TextYAlignment = Enum.TextYAlignment.Top
removerHelpText.Position = UDim2.new(0,4,0,4)
removerHelpText.Size = UDim2.new(1,-8,0,177)
removerHelpText.BackgroundTransparency = 1
removerHelpText.TextWrap = true
removerHelpText.Text = [[
Clicking terrain removes a single block at the location clicked (shown with red highlight).]]

removeText = CreateStandardLabel("removeText", UDim2.new(0, 8, 0, 10), UDim2.new(0, 67, 0, 14), "Click to remove terrain", removerFrame)

-- Function to connect to the mouse button 1 down event.  This is what will run when the user clicks.
-- mouse 	- Mouse data.
function onClicked(mouse)
	if on then		
		local cellPos = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		local x = cellPos.x
		local y = cellPos.y
		local z = cellPos.z		

		SetCell(c, x, y, z, 0, 0, 0)
		
		-- Mark undo point.
		game:GetService("ChangeHistoryService"):SetWaypoint("Remover")		
		
		UpdatePosition(mouse.Hit)
		
		if properties.autoWedgeEnabled then
			AutoWedge(c, Region3int16.new(Vector3int16.new(x - 1, y - 1, z - 1), Vector3int16.new(x + 1, y + 1, z + 1)))
		end
	end
end

mouseHighlighter.OnClicked = onClicked

-- Run when the popup is activated.
function On()
	if not c then
		return
	end
	if plugin then
		plugin:Activate(true)
	end
	if toolbarbutton then
		toolbarbutton:SetActive(true)
	end
	if removerPropertiesDragBar then
		removerPropertiesDragBar.Visible = true
	end
	on = true
end

-- Run when the popup is deactivated.
function Off()
	on = false

	if toolbarbutton then
		toolbarbutton:SetActive(false)
	end
	
	-- Hide the popup gui.
	if removerPropertiesDragBar then
		removerPropertiesDragBar.Visible = false
	end
	if mouseHighlighter then
		mouseHighlighter:DisablePreview()
	end
end

plugin.Deactivation:connect(function()
	Off()
end)

loaded = true