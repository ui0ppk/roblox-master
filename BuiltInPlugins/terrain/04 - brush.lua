while game == nil do
	wait(1/30)
end

---------------
--PLUGIN SETUP-
---------------
local loaded = false
local on = false

self = PluginManager():CreatePlugin()

toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("Brush", "Brush", "brush.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	elseif loaded then
		On()
	end
end)

game:WaitForChild("Workspace")
game.Workspace:WaitForChild("Terrain")

-- Local function definitions
local c = game.Workspace.Terrain
local GetCell = c.GetCell
local SetCell = c.SetCell
local SetCells = c.SetCells
local AutowedgeCells = c.AutowedgeCells
local AutowedgeCell = c.AutowedgeCell
local WorldToCellPreferSolid = c.WorldToCellPreferSolid
local CellCenterToWorld = c.CellCenterToWorld
local AutoWedge = c.AutowedgeCell

local buildTerrainMode = 'Add'
local removeTerrainMode = 'Remove'

-----------------
--DEFAULT VALUES-
-----------------
local radius = 5
local depth = 0
local mousedown = false
local mousemoving = false
local brushheight = nil
local material = 1
local lastMousePos = Vector2.new(-1,-1)
local lastCellFillTime = 0
local maxYExtents = math.min(c.MaxExtents.Max.Y, 512)

-- Which mode (build/remove) it is.
local mode = buildTerrainMode

-- Height and depth to use for the different modes.
local buildTerrainHeight = 5
local removeTerrainDepth = -5

local mouse = self:GetMouse()
mouse.Button1Down:connect(function() buttonDown() end)
mouse.Button1Up:connect(function() 
	mousedown = false
	brushheight = nil
	enablePreview()
	updatePreviewSelection(mouse.Hit)
	game:GetService("ChangeHistoryService"):SetWaypoint("Brush")
end)
mouse.Move:connect(function() mouseMoved() end)
local selectionPart = Instance.new("Part")
selectionPart.Name = "SelectionPart"
selectionPart.Archivable = false
selectionPart.Transparency = 1
selectionPart.Anchored = true
selectionPart.Locked = true
selectionPart.CanCollide = false
selectionPart.FormFactor = Enum.FormFactor.Custom

local selectionBox = Instance.new("SelectionBox")
selectionBox.Archivable = false
selectionBox.Color = BrickColor.new("Lime green")
selectionBox.Adornee = selectionPart
mouse.TargetFilter = selectionPart

-----------------------
--FUNCTION DEFINITIONS-
-----------------------

-- searches the y depth of a particular cell position to find the lowest y that is empty
function findLowestEmptyCell(x,y,z)
	local cellMat = GetCell(c, x,y,z)
	local lowestY = y

	while cellMat == Enum.CellMaterial.Empty do
		lowestY = y
		if y > 0 then
			y = y - 1
			cellMat = GetCell(c, x,y,z)
		else
			lowestY = 0
			cellMat = nil
		end
	end
	return lowestY
end

-- finds the lowest cell that is not filled in the radius that is currently specified
function findLowPoint(x,y,z)
	local lowestPoint = maxYExtents + 1
	for i = -radius, radius do
		local zPos = z + i
		for j = -radius, radius do
			local xPos = x + i
			local cellMat = GetCell(c, xPos, y, zPos)
			if cellMat == Enum.CellMaterial.Empty then
				local emptyDepth = findLowestEmptyCell(xPos, y, zPos)
				if emptyDepth < lowestPoint then
					lowestPoint = emptyDepth
				end
			end
		end
	end
	return lowestPoint
end

--brushes terrain at point (x, y, z) in cluster c
function brush(x, y, z)
	if depth == 0 then return end

	if depth > 0 then
		local findY = findLowPoint(x,y + depth,z)
		local yWithDepth = y + depth

		local lowY = nil
		if findY < yWithDepth then
			lowY = findY
		else
			lowY = yWithDepth
		end

		local lowVec = Vector3int16.new(x - radius, lowY, z - radius)
		local highVec = Vector3int16.new(x + radius, yWithDepth, z + radius)
		local regionToFill = Region3int16.new(lowVec,highVec)

		SetCells(c, regionToFill, material, 0, 0)
		AutowedgeCells(c, regionToFill)
	else
		local lowVec = Vector3int16.new(x - radius, y + depth + 1, z - radius)
		local highVec = Vector3int16.new(x + radius, maxYExtents, z + radius)
		local regionToEmpty = Region3int16.new(lowVec,highVec)
		SetCells(c,regionToEmpty,Enum.CellMaterial.Empty,0,0)
	end
end

function disablePreview()
	selectionBox.Parent = nil
end

function enablePreview()
	selectionBox.Parent = game.Workspace
end

function updatePreviewSelection(position)
	if not position then return end
	--if not mouse.Target then disablePreview() return end
	if depth == 0 then disablePreview() return end

	local vectorPos = Vector3.new(position.x,position.y,position.z)
	local cellPos = WorldToCellPreferSolid(c, vectorPos)
	local solidCell = WorldToCellPreferSolid(c, vectorPos)
	
	-- If nothing was hit, do the plane intersection.
	if 0 == GetCell(c, solidCell.X, solidCell.Y, solidCell.Z).Value then
		local success = false
		success, cellPos = PlaneIntersection(vectorPos)
		if not success then 
			if mouse.Target then
				cellPos = solidCell
			else
				return
			end
		end
	end

	local regionToSelect = nil
	if depth > 0 then
		local yWithDepth = nil
		if brushheight then
			yWithDepth = brushheight + depth
		else
			yWithDepth = cellPos.y + depth
		end

		local lowY = nil
		if brushheight then
			lowY = brushheight + 1
		else
			local findY = findLowPoint(cellPos.x,yWithDepth,cellPos.z)
			if findY < yWithDepth then
				lowY = findY
			else
				lowY = yWithDepth
			end
		end

		local lowVec = CellCenterToWorld(c, cellPos.x - radius, lowY - 1, cellPos.z - radius)
		local highVec = CellCenterToWorld(c, cellPos.x + radius, yWithDepth + 1, cellPos.z + radius)
		selectionBox.Color = BrickColor.new("Lime green")
		regionToSelect = Region3.new(lowVec,highVec)
	else
		local yPos = cellPos.y + depth
		if brushheight then
			yPos = brushheight + depth
		end

		local lowVec = CellCenterToWorld(c, cellPos.x - radius, yPos, cellPos.z - radius)
		local highVec = CellCenterToWorld(c, cellPos.x + radius, maxYExtents, cellPos.z + radius)
		selectionBox.Color = BrickColor.new("Really red")
		regionToSelect = Region3.new(lowVec,highVec)
	end

	selectionPart.Size = regionToSelect.Size - Vector3.new(-4,4,-4)
	selectionPart.CFrame = regionToSelect.CFrame

	enablePreview()
end

function doFillCells(position, mouseDrag, needsCellPos)
	if mouseDrag then
		local timeBetweenFills = tick() - lastCellFillTime
		local totalDragPixels = math.abs(mouseDrag.x) + math.abs(mouseDrag.y)
		local editDistance = (game.Workspace.CurrentCamera.CoordinateFrame.p - 
			Vector3.new(position.x,position.y,position.z)).magnitude

		if  (timeBetweenFills <= 0.05) then
			if editDistance * totalDragPixels < 450 then
				lastCellFillTime = tick()
				return
			end
		end
	end

	local x = position.x
	local y = position.y
	local z = position.z

	if needsCellPos then
		local cellPos = WorldToCellPreferSolid(c, Vector3.new(x,y,z))
		
		local solidCell = WorldToCellPreferSolid(c, Vector3.new(x,y,z))
		
		-- If nothing was hit, do the plane intersection.
		if 0 == GetCell(c, solidCell.X, solidCell.Y, solidCell.Z).Value then
			local success = false
			success, cellPos = PlaneIntersection(Vector3.new(x,y,z))
			if not success then 
				if mouse.Target then
					cellPos = solidCell
				else
					return
				end
			end	
		end

		
		x = cellPos.x
		y = cellPos.y
		z = cellPos.z
	end

	if brushheight == nil then
		brushheight = y
	end
	
	brush(x, brushheight, z)
	lastCellFillTime = tick()
end

function mouseMoved()
	if on then
		if mousedown == true then
			if mousemoving then return end

			mousemoving = true
				local currMousePos = Vector2.new(mouse.X,mouse.Y)
				local mouseDrag = currMousePos - lastMousePos
				doFillCells(mouse.Hit, mouseDrag, true)
				lastMousePos = currMousePos
			mousemoving = false
		end
		updatePreviewSelection(mouse.Hit)
	end
end

	-- Do a line/plane intersection.  The line starts at the camera.  The plane is at y == 0, normal(0, 1, 0)
	--
	-- vectorPos - End point of the line.
	-- 
	-- Return:
	-- success      - Value is true if there was a plane intersection, false if not.
	-- intersection - Value is the terrain cell intersection point if there is one, vectorPos if there isn't.
	function PlaneIntersection(vectorPos)
		local currCamera = game.Workspace.CurrentCamera
		local startPos = Vector3.new(currCamera.CoordinateFrame.p.X, currCamera.CoordinateFrame.p.Y, currCamera.CoordinateFrame.p.Z)
		local endPos = Vector3.new(vectorPos.X, vectorPos.Y, vectorPos.Z)
		local normal = Vector3.new(0, 1, 0)
		local p3 = Vector3.new(0, 0, 0)
		local startEndDot = normal:Dot(endPos - startPos)
		local cellPos = vectorPos
		local success = false
		if startEndDot ~= 0  then
			local t = normal:Dot(p3 - startPos) / startEndDot
			if(t >=0 and t <=1) then
				local intersection = ((endPos - startPos) * t) + startPos
				cellPos = c:WorldToCell(intersection)
				success = true
			end
		end

		return success, cellPos
	end

function buttonDown()
	if on then
		local firstCellPos = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		local solidCell = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		
		-- If nothing was hit, do the plane intersection.
		if 0 == GetCell(c, solidCell.X, solidCell.Y, solidCell.Z).Value then
			local success = false
			success, firstCellPos = PlaneIntersection(Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
			if not success then 
				if mouse.Target then
					firstCellPos = solidCell
				else
					return
				end
			end	
		end
		
		local celMat = GetCell(c, firstCellPos.x, firstCellPos.y, firstCellPos.z)
		if celMat.Value > 0 then 
			material = celMat.Value 
		else
			if 0 == material then
				-- It was nothing, give it a default type and the plane intersection. 
				material = 1
			end		
		end

		brushheight = firstCellPos.y
		lastMousePos = Vector2.new(mouse.X,mouse.Y)
		doFillCells(firstCellPos)

		mousedown = true
	end
end

function On()
	if not c then
		return
	end
	if self then
		self:Activate(true)
	end
	if toolbarbutton then
		toolbarbutton:SetActive(true)
	end
	if enablePreview then
		enablePreview()
	end
	if brushDragBar then
		brushDragBar.Visible = true
	end
	on = true
end

function Off()
	if toolbarbutton then
		toolbarbutton:SetActive(false)
	end
	if disablePreview then
		disablePreview()
	end
	if brushDragBar then
		brushDragBar.Visible = false
	end
	on = false
end




------
--GUI-
------

--load library for with sliders
local RbxGui = LoadLibrary("RbxGui")

-- Create a standard dropdown.  Use this for all dropdowns in the popup so it is easy to standardize.
-- name - What to set the text label name as.
-- pos    	 	 - Where to position the label.  Should be of type UDim2.
-- values    	 - A table of the values that will be in the dropbox, in the order they are to be shown.
-- initValue	 - Initial value the dropdown should be set to.
-- funcOnChange  - Function to run when a dropdown selection is made.
-- parent 	 	 - What to set the parent as.
-- Return:
-- dropdown 	   - The dropdown gui.
-- updateSelection - Object to use to change the current dropdown.																																		
function CreateStandardDropdown(name,
						        pos,
								values,
								initValue,
								funcOnChange,
								parent)					
	-- Create a dropdown selection for the modes to fill in a river
	local dropdown, updateSelection=RbxGui.CreateDropDownMenu(values, funcOnChange);
	dropdown.Name = name
	dropdown.Position = pos
	dropdown.Active = true
	dropdown.Size = UDim2.new(0,150,0,32)
	dropdown.Parent = parent

	updateSelection(initValue)		

	return dropdown, updateSelection
end	

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

-- Create a standardized slider.
-- name  - Name to use for the slider.
-- pos   - Position to display the slider at.
-- steps - How many steps there are in the slider.
-- funcOnChange - Function to call when the slider changes.
-- initValue    - Initial value to set the slider to.  If nil the slider stays at the default. 
-- parent       - What to set as the parent.
-- Return:
-- sliderGui      - Slider gui object.
-- sliderPosition - Object that can set the slider value.
function CreateStandardSlider(name,
                              pos,
							  lengthBarPos,
							  steps,
							  funcOnChange,
							  initValue,
							  parent)
	local sliderGui, sliderPosition = RbxGui.CreateSlider(steps, 0, UDim2.new(0,0,0,0))
	
	sliderGui.Name = name
	sliderGui.Parent = parent
	sliderGui.Position = pos
	sliderGui.Size = UDim2.new(0,160,0,20)
	local lengthBar = sliderGui:FindFirstChild("Bar")
	lengthBar.Size = UDim2.new(1, -20, 0, 5)
	lengthBar.Position = lengthBarPos
	
	if nil ~= funcOnChange then
		sliderPosition.Changed:connect(function() funcOnChange(sliderPosition) end)
	end
	
	if nil ~= initValue then 
		sliderPosition.Value = initValue
	end
	
	return sliderGui, sliderPosition 
end



--screengui
local g = Instance.new("ScreenGui", game:GetService("CoreGui"))
g.Name = "TerrainBrushGui"

brushDragBar, elevationFrame, elevationHelpFrame, elevationCloseEvent = RbxGui.CreatePluginFrame("Terrain Brush",UDim2.new(0,151,0,160),UDim2.new(0,0,0,0),false,g)
brushDragBar.Visible = false
elevationCloseEvent.Event:connect(function (  )
	Off()
end)

elevationHelpFrame.Size = UDim2.new(0,200,0,210)
local helpText = Instance.new("TextLabel",elevationHelpFrame)
helpText.Font = Enum.Font.ArialBold
helpText.FontSize = Enum.FontSize.Size12
helpText.TextColor3 = Color3.new(1,1,1)
helpText.BackgroundTransparency = 1
helpText.TextWrap = true
helpText.Size = UDim2.new(1,-10,1,-10)
helpText.Position = UDim2.new(0,5,0,5)
helpText.TextXAlignment = Enum.TextXAlignment.Left
helpText.Text = 
[[Drag the mouse by holding the left mouse button to either create or destroy terrain defined by the selection box.

Radius:
Half of the width of the selection box to be used.

Height:
How tall to make terrain from the mouse location.  If this value is negative, the brush will remove terrain instead of creating terrain (indicated by the red selection box).
]]

--current radius display label
radl = Instance.new("TextLabel", elevationFrame)
radl.Position = UDim2.new(0,0,0,70)
radl.Size = UDim2.new(1, 0, 0, 14)
radl.Text = ""
radl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
radl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
radl.Font = Enum.Font.ArialBold
radl.FontSize = Enum.FontSize.Size14
radl.TextXAlignment = Enum.TextXAlignment.Left
radl.BorderColor3 = Color3.new(0, 0, 0)
radl.BackgroundTransparency = 1

--radius slider
radSliderGui, radSliderPosition = RbxGui.CreateSlider(5,  0, UDim2.new(0, 10, 0, 92))
radSliderGui.Parent = elevationFrame
radBar = radSliderGui:FindFirstChild("Bar")
radBar.Size = UDim2.new(1,-20,0,5)
radSliderPosition.Changed:connect(function()
	radius = radSliderPosition.Value + 1
	radl.Text = " Radius: ".. radius
end)
radSliderPosition.Value = radius - 1

--current depth factor display label
dfl = Instance.new("TextLabel", elevationFrame)
dfl.Position = UDim2.new(0, 0, 0, 110)
dfl.Size = UDim2.new(1, 0, 0, 14)
dfl.Text = ""
dfl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
dfl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
dfl.Font = Enum.Font.ArialBold
dfl.FontSize = Enum.FontSize.Size14
dfl.BorderColor3 = Color3.new(0, 0, 0)
dfl.TextXAlignment = Enum.TextXAlignment.Left
dfl.BackgroundTransparency = 1

--depth factor slider
addSliderGui, addSliderPosition = RbxGui.CreateSlider(31, 0, UDim2.new(0, 10, 0,132))
addSliderGui.Parent = elevationFrame
dfBar = addSliderGui:FindFirstChild("Bar")
dfBar.Size = UDim2.new(1,-20,0,5)
addSliderPosition.Changed:connect(function()
	depth = addSliderPosition.Value
	dfl.Text = " Height: ".. depth
end)
addSliderPosition.Value = buildTerrainHeight


--depth factor slider
removeSliderGui, removeSliderPosition = RbxGui.CreateSlider(31, 0, UDim2.new(0, 10, 0,132))
removeSliderGui.Parent = elevationFrame
dfBar = removeSliderGui:FindFirstChild("Bar")
dfBar.Size = UDim2.new(1,-20,0,5)
removeSliderPosition.Changed:connect(function()
	depth = -(removeSliderPosition.Value)
	dfl.Text = " Height: ".. depth
end)
removeSliderPosition.Value = removeTerrainDepth


-- Set which mode is to be used.  Show/hide as needed.
--
-- mode - Which build mode to run.
function SetMode(mode)
	if mode == buildTerrainMode then
		addSliderGui.Visible = true
		local hold = addSliderPosition.Value
		addSliderPosition.Value = 0
		addSliderPosition.Value = hold
		removeSliderGui.Visible = false
	elseif mode == removeTerrainMode then
		addSliderGui.Visible = false
		removeSliderGui.Visible = true
		local hold = removeSliderPosition.Value
		removeSliderPosition.Value = 0
		removeSliderPosition.Value = hold
	end
end


-- Create/Remove mode
-- Create the build mode gui.
buildModeLabel = CreateStandardLabel("BuildModeLabel", UDim2.new(0, 8, 0, 10), UDim2.new(0, 67, 0, 14), "Build Mode:", elevationFrame)
buildModeDropdown, buildModeSet = CreateStandardDropdown("BuildModeDropdown",
						        UDim2.new(0,1,0,26),
								{buildTerrainMode, removeTerrainMode},
								buildMode,
								function(value) 
									if 'Add' == value then
										SetMode(buildTerrainMode)
									elseif 'Remove' == value then
										SetMode(removeTerrainMode)
									end
								end, 
								elevationFrame)
	--[[							
	buildModeSet(buildTerrainMode)
	buildModeSet(removeTerrainMode)
	--]]
	buildModeSet(mode)
	SetMode(mode)

self.Deactivation:connect(function()
	Off()
end)								

--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------

loaded = true
