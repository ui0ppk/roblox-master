while game == nil do
	wait(1/30)
end

---------------
--PLUGIN SETUP-
---------------
loaded = false
on = false

self = PluginManager():CreatePlugin()
mouse = self:GetMouse()
mouse.Button1Down:connect(function() mouseDown(mouse) end)
mouse.Button1Up:connect(function() mouseUp(mouse) end)
mouse.Move:connect(function() mouseMove(mouse) end)

toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("Material Brush", "Material Brush", "materialBrush.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	elseif loaded then
		On()
	end
end)

game:WaitForChild("Workspace")
game.Workspace:WaitForChild("Terrain")

-----------------------------
--LOCAL FUNCTION DEFINITIONS-
-----------------------------
local c = game.Workspace.Terrain
local SetCell = c.SetCell
local SetWaterCell = c.SetWaterCell
local GetWaterCell = c.GetWaterCell
local GetCell = c.GetCell
local WorldToCellPreferSolid = c.WorldToCellPreferSolid
local CellCenterToWorld = c.CellCenterToWorld
local waterMaterial = 17

local brushTypes = {"Circular", "Square"}
local waterForceDirections = {"NegX","X","NegY","Y","NegZ","Z"}
local waterForces = {"None", "Small", "Medium", "Strong", "Max"}

local mediumWaterForce = Enum.WaterForce.Medium

-----------------
--DEFAULT VALUES-
-----------------
local terrainSelectorGui, terrainSelected, radiusLabel, dragBar, closeEvent, helpFrame, currSelectionUpdate, currSelectionDestroy, lastCell, lastLastCell, waterPanel = nil
local dragging = false
local painting = false

--- exposed values to user via gui
local currentMaterial = Enum.CellMaterial.Grass
local radius = 3
local brushType = "Square"
local currWaterForceDirection = "NegX"
local currWaterForce = "None"

-- lua library load
local RbxGui = LoadLibrary("RbxGui")
local RbxUtil = LoadLibrary("RbxUtility")

-----------------------
--FUNCTION DEFINITIONS-
-----------------------
function paintWaterfall(setCells)
	if setCells then
		for i = 1, #setCells do
			SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce,  Enum.WaterDirection.NegY)
		end
	end
end

function setWaterDirection(mouseCellPos, setCells)
	if not setCells then return end
	if #setCells <= 0 then return end

	if directionIsDown(lastCell, mouseCellPos) then
		paintWaterfall(setCells)
		return
	end


	local initX = setCells[1].xPos
	local initZ = setCells[1].zPos

	local endX = setCells[#setCells].xPos
	local endZ = setCells[#setCells].zPos

	local zWidth = math.abs(endZ - initZ)
	local zMiddle = math.ceil(zWidth/2 + initZ)
	local xMiddle = math.ceil(zWidth/2 + initX)

	local down = endX - initX
	local up,left,right = nil
	if down < 0 then
		down = Enum.WaterDirection.NegX
		up = Enum.WaterDirection.X
		left = Enum.WaterDirection.Z
		right = Enum.WaterDirection.NegZ
	else
		down = Enum.WaterDirection.X
		up = Enum.WaterDirection.NegX
		left = Enum.WaterDirection.NegZ
		right = Enum.WaterDirection.Z
	end

	if #setCells == 1 then

		if not mouseCellPos or not lastCell then return end

		local overallDirection = (mouseCellPos - lastCell).unit
		if math.abs(overallDirection.x) > math.abs(overallDirection.y) and math.abs(overallDirection.x) > math.abs(overallDirection.z) then
			if overallDirection.x > 0 then
				SetWaterCell(c, setCells[1].xPos, setCells[1].yPos, setCells[1].zPos, mediumWaterForce, Enum.WaterDirection.X)
			else
				SetWaterCell(c, setCells[1].xPos, setCells[1].yPos, setCells[1].zPos, mediumWaterForce, Enum.WaterDirection.NegX)
			end
		elseif math.abs(overallDirection.z) > math.abs(overallDirection.y) and math.abs(overallDirection.z) > math.abs(overallDirection.x) then
			if overallDirection.z > 0 then
				SetWaterCell(c, setCells[1].xPos, setCells[1].yPos, setCells[1].zPos, mediumWaterForce, Enum.WaterDirection.Z)
			else
				SetWaterCell(c, setCells[1].xPos, setCells[1].yPos, setCells[1].zPos, mediumWaterForce, Enum.WaterDirection.NegZ)
			end
		elseif math.abs(overallDirection.y) > math.abs(overallDirection.z) and math.abs(overallDirection.y) > math.abs(overallDirection.x) then
			if overallDirection.y > 0 then
				SetWaterCell(c, setCells[1].xPos, setCells[1].yPos, setCells[1].zPos, mediumWaterForce, Enum.WaterDirection.Y)
			else
				SetWaterCell(c, setCells[1].xPos, setCells[1].yPos, setCells[1].zPos, mediumWaterForce, Enum.WaterDirection.NegY)
			end
		end
		
	else

		for i = 1, #setCells do
			if(setCells[i].xPos == initX) then
				SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce, down)
			elseif(setCells[i].xPos == endX) then
				SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce, up)
			else
				if setCells[i].zPos < zMiddle then
					SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce, right)
				elseif setCells[i].zPos > zMiddle then
					SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce, left)
				else
					if setCells[i].xPos < xMiddle then
						SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce, down)
					elseif  setCells[i].xPos > xMiddle then
						SetWaterCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, mediumWaterForce, up)
					end
				end
			end
		end

	end
end

function getSquare( cellPos, setCells)
	local finalX = cellPos.x + radius - 1 
	local finalZ = cellPos.z + radius - 1
	local finalY = cellPos.y + radius - 1
	
	for x = cellPos.x - radius + 1, finalX do
		for z = cellPos.z - radius + 1, finalZ do
			for y = cellPos.y - radius + 1, finalY do
				local tempCellPos = Vector3.new(x, y, z)
				local oldMaterial, oldType, oldOrientation = GetCell(c, x, y, z)
				if oldMaterial.Value > 0 then 
					table.insert(setCells,{xPos = x, yPos = y, zPos = z, theType = oldType, orientation = oldOrientation})
				end
			end
		end
	end
end

function  getCircular( cellPos, setCells)
	local radiusSquared = radius * radius
	
	local finalX = cellPos.x + radius
	local finalZ = cellPos.z + radius
	local finalY = cellPos.y + radius

	for x = cellPos.x - radius, finalX do
		for z = cellPos.z - radius, finalZ do
		for y = cellPos.y - radius, finalY do
				local tempCellPos = Vector3.new(x, y, z)
				local holdDist = tempCellPos - cellPos
				local distSq = (holdDist):Dot(holdDist)
				if (distSq < radiusSquared) then
					local oldMaterial, oldType, oldOrientation = GetCell(c, x, y, z)
					if oldMaterial.Value > 0 then 
						table.insert(setCells,{xPos = x, yPos = y, zPos = z, theType = oldType, orientation = oldOrientation})
					end
						end
					end
				end
			end
end

function paintCircular(cellPos, setCells)
	getCircular(cellPos, setCells)

	if currentMaterial ~= waterMaterial then
		for i = 1, #setCells do
			SetCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, currentMaterial, setCells[i].theType, setCells[i].orientation)
		end
	end
end

function paintSquare(cellPos, setCells)
	getSquare(cellPos, setCells)
	
	if currentMaterial ~= waterMaterial then
		for i = 1, #setCells do
			SetCell(c, setCells[i].xPos, setCells[i].yPos, setCells[i].zPos, currentMaterial,  setCells[i].theType, setCells[i].orientation)
		end	
	end
end

function paint(startPos)
	if startPos and c then
		local cellPos = startPos
		local setCells = {}

		if brushType == "Circular" then
			paintCircular(cellPos, setCells)
		elseif brushType == "Square" then
			paintSquare(cellPos, setCells)
			end

		if (currentMaterial == waterMaterial) then
			setWaterDirection(cellPos, setCells)
		end

		return setCells
	end
end

function getAffectedCells(startPos)
	local setCells = {}

	if startPos and c then
		if brushType == "Circular" then
			getCircular(startPos, setCells)
		elseif brushType == "Square" then
			getSquare(startPos, setCells)
		end
	end

	return setCells
end

function calculateRegion( mouse )
	local cellPos = WorldToCellPreferSolid(c, mouse.Hit.p)

	local lowVec = Vector3.new(cellPos.x - radius,cellPos.y - radius,cellPos.z - radius)
	local highVec = Vector3.new(cellPos.x + radius,cellPos.y + radius,cellPos.z + radius)
	lowVec = CellCenterToWorld(c,lowVec.x,lowVec.y,lowVec.z)
	highVec = CellCenterToWorld(c,highVec.x,highVec.y,highVec.z)

	return Region3.new(lowVec + Vector3.new(2,2,2),highVec - Vector3.new(2,2,2))
end

function createSelection(mouse, massSelection)
	currSelectionUpdate, currSelectionDestroy = RbxUtil.SelectTerrainRegion(calculateRegion(mouse),BrickColor.new("Lime green"),massSelection,game.CoreGui)
end

function updateSelection(mouse)
	if not currSelectionUpdate then
		createSelection(mouse, radius > 4)
	else
		currSelectionUpdate(calculateRegion(mouse),BrickColor.new("Lime green"))
	end
end

function setPositionDirectionality()
	if nil == lastCell then
		return
	end
	
	-- no dragging occured, lets set our water to be stagnant or be a waterfall
	if lastCell and not lastLastCell then
		local cellsToSet = paint(lastCell)
		if directionIsDown(nil, lastCell) then
			paintWaterfall(cellsToSet)
		else
			for i = 1, #cellsToSet do
				SetWaterCell(c, cellsToSet[i].xPos, cellsToSet[i].yPos, cellsToSet[i].zPos, Enum.WaterForce.None, Enum.WaterDirection.NegX)
			end
		end
		return
	end

	if directionIsDown(lastLastCell, lastCell) then
		local cellsToSet = paint(lastCell)
		paintWaterfall(cellsToSet)
		return
	end

	local overallDirection = (lastCell - lastLastCell).unit
	local cellsToSet = paint(lastCell)

	local absX, absY, absZ = math.abs(overallDirection.X),math.abs(overallDirection.Y),math.abs(overallDirection.Z)
	local direction = nil

	if absX > absY and absX > absZ then
		if overallDirection.X > 0 then
			direction = Enum.WaterDirection.X
		else
			direction = Enum.WaterDirection.NegX
		end
	elseif absY > absX and absY > absZ then
		if overallDirection.Y > 0 then
			direction = Enum.WaterDirection.Y
		else
			direction = Enum.WaterDirection.NegY
		end
	elseif absZ > absX and absZ > absY then
		if overallDirection.Z > 0 then
			direction = Enum.WaterDirection.Z
		else
			direction = Enum.WaterDirection.NegZ
		end
	end

	if not direction then -- this should never be hit, but just in case
		direction = Enum.WaterDirection.NegX
	end

	for i = 1, #cellsToSet do
		SetWaterCell(c, cellsToSet[i].xPos, cellsToSet[i].yPos, cellsToSet[i].zPos, mediumWaterForce, direction)
	end
end

function mouseDown(mouse)
	if on and mouse.Target == game.Workspace.Terrain then
		dragging = true
		if mouse and mouse.Hit then
			if mouse.Target == game.Workspace.Terrain then
				local newCell = WorldToCellPreferSolid(c, mouse.Hit.p)
				if newCell then
					local setCells = paint(newCell)
					if currentMaterial == waterMaterial and directionIsDown(lastCell, newCell) then
						paintWaterfall(setCells)
					end
					lastCell = newCell
				end
			end
		end
	end
end

function mouseUp(mouse)
	dragging = false		

	-- we need to fix directionality on last cell set (if water)
	if currentMaterial == waterMaterial then
		setPositionDirectionality()
	end

	game:GetService("ChangeHistoryService"): SetWaypoint("MaterialPaint")		

	lastLastCell = nil
	lastCell = nil
end

function mouseMove(mouse)
	if on then
		if mouse.Target == game.Workspace.Terrain then
			if lastCell ~= WorldToCellPreferSolid(c, mouse.Hit.p) then
				updateSelection(mouse)
				local newCell = WorldToCellPreferSolid(c, mouse.Hit.p)

				if dragging then
					painting = true
						paint(newCell)
						if lastCell and newCell then
							if (lastCell - newCell).magnitude > 1 then
								paintBetweenPoints(lastCell,newCell)
							end
						end
						lastLastCell = lastCell
						lastCell = newCell
					painting = false
				end

			end
		else
			destroySelection()
		end
	end
end

function directionIsDown(fromCell, toCell)
	if not toCell then
		return false
	end

	if toCell and fromCell then
		local direction = (toCell - fromCell).unit
		local absX,absY,absZ = math.abs(direction.X),math.abs(direction.Y),math.abs(direction.Z)
		if absY > absX and absY > absZ then
			return true
		end
	end

	local viableCells = getAffectedCells(toCell)
	if not viableCells then return false end
	if #viableCells < 2 then return false end

	local lowX, lowY,lowZ = viableCells[1].xPos,viableCells[1].yPos,viableCells[1].zPos
	local highX, highY, highZ = lowX, lowY,lowZ

	for i = 2, #viableCells do
		if viableCells[i].xPos < lowX then lowX = viableCells[i].xPos end
		if viableCells[i].xPos > highX then highX = viableCells[i].xPos end

		if viableCells[i].yPos < lowY then lowY = viableCells[i].yPos end
		if viableCells[i].yPos > highY then highY = viableCells[i].yPos end

		if viableCells[i].zPos < lowZ then lowZ = viableCells[i].zPos end
		if viableCells[i].zPos > highZ then highZ = viableCells[i].zPos end
	end

	local xRange, yRange, zRange = math.abs(highX - lowX), math.abs(highY - lowY), math.abs(highZ - lowZ)

	local xzPlaneArea = xRange * zRange
	local xyPlaneArea = xRange * yRange
	local yzPlaneArea = yRange * zRange

	if xyPlaneArea > xzPlaneArea then
		return true
	end

	if yzPlaneArea > xzPlaneArea then
		return true
	end

	return false
end

function destroySelection()
	if currSelectionUpdate then
		currSelectionUpdate = nil
	end
	if currSelectionDestroy then
		currSelectionDestroy()
		currSelectionDestroy = nil
	end
end

function moveTowardsGoal(direction, currPos, goalPos, currCell)
	if currPos ~= goalPos then
		if currPos < goalPos then
			if direction == "X" then
				currCell = Vector3.new(currCell.X + 1, currCell.Y,currCell.Z)
			elseif direction == "Y" then
				currCell = Vector3.new(currCell.X, currCell.Y + 1,currCell.Z)
			elseif direction == "Z" then
				currCell = Vector3.new(currCell.X, currCell.Y,currCell.Z + 1)
			end
		elseif currPos > goalPos then
			if direction == "X" then
				currCell = Vector3.new(currCell.X - 1, currCell.Y,currCell.Z)
			elseif direction == "Y" then
				currCell = Vector3.new(currCell.X, currCell.Y - 1,currCell.Z)
			elseif direction == "Z" then
				currCell = Vector3.new(currCell.X, currCell.Y,currCell.Z - 1)
			end
		end
	end

	return currCell
end

function interpolateOneDim(direction, currPos, goalPos, currCell)
	if currPos ~= goalPos then
		currCell = moveTowardsGoal(direction,currPos,goalPos,currCell)
		paint(currCell)
	end

	return currCell
end

function paintBetweenPoints( lastCell,newCell)
	local currCell = lastCell

	while currCell.X ~= newCell.X or currCell.Z ~= newCell.Z or currCell.Y ~= newCell.Y do
		currCell = interpolateOneDim("X",currCell.X,newCell.X,currCell)
		currCell = interpolateOneDim("Z",currCell.Z,newCell.Z,currCell)
		currCell = interpolateOneDim("Y",currCell.Y,newCell.Y,currCell)
	end
end

function On()
	if not c then
		return
	end
	self:Activate(true)
	toolbarbutton:SetActive(true)

	dragBar.Visible = true
	on = true
end

function Off()
	toolbarbutton:SetActive(false)

	destroySelection()
	dragBar.Visible = false
	on = false
end

------
--GUI-
------

--screengui
local g = Instance.new("ScreenGui", game:GetService("CoreGui"))
g.Name = "MaterialPainterGui"

dragBar, containerFrame, helpFrame, closeEvent = RbxGui.CreatePluginFrame("Material Brush",UDim2.new(0,163,0,285),UDim2.new(0,0,0,0),false,g)
dragBar.Visible = false

-- End dragging if it goes over the gui frame.
containerFrame.MouseEnter:connect(function() dragging = false end)
dragBar.MouseEnter:connect(function() dragging = false end)

helpFrame.Size = UDim2.new(0,200,0,250)
helpFrame.ZIndex = 3
local textHelp = Instance.new("TextLabel",helpFrame)
textHelp.Name = "TextHelp"
textHelp.Font = Enum.Font.ArialBold
textHelp.FontSize = Enum.FontSize.Size12
textHelp.TextColor3 = Color3.new(1,1,1)
textHelp.Size = UDim2.new(1,-6,1,-6)
textHelp.Position = UDim2.new(0,3,0,3)
textHelp.TextXAlignment = Enum.TextXAlignment.Left
textHelp.TextYAlignment = Enum.TextYAlignment.Top
textHelp.Position = UDim2.new(0,4,0,4)
textHelp.BackgroundTransparency = 1
textHelp.TextWrap = true
textHelp.ZIndex = 4
textHelp.Text = [[Changes existing terrain blocks to the specified material.  Simply hold the mouse down and drag to 'paint' the terrain (only cells inside the selection box will be affected).

Size: 
The size of the brush we paint terrain with.

Brush Type: 
The shape we paint terrain with inside of our selection box.

Material Selection: 
The terrain material we will paint.]]

closeEvent.Event:connect(function() Off() end)

terrainSelectorGui, terrainSelected, forceTerrainSelection = RbxGui.CreateTerrainMaterialSelector(UDim2.new(1, -10, 0, 184),UDim2.new(0, 5, 1, -190))
terrainSelectorGui.Parent = containerFrame
terrainSelectorGui.BackgroundTransparency = 1
terrainSelectorGui.BorderSizePixel = 0
terrainSelected.Event:connect(function ( newMaterial )
	currentMaterial = newMaterial
end)

-- Purpose:
-- Retrive the size text to display for a given radius, where 1 == 1 block and 2 == 3 blocks, etc.
function SizeText(radius)
	return 'Size: ' .. (((radius-1) * 2) + 1)
end

--current radius display label
radiusLabel = Instance.new("TextLabel", containerFrame)
radiusLabel.Name = "RadiusLabel"
radiusLabel.Position = UDim2.new(0, 10, 0, 5)
radiusLabel.Size = UDim2.new(1, -3, 0, 14)
radiusLabel.Text = SizeText(radius)
radiusLabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
radiusLabel.Font = Enum.Font.ArialBold
radiusLabel.FontSize = Enum.FontSize.Size14
radiusLabel.BorderColor3 = Color3.new(0, 0, 0)
radiusLabel.BackgroundTransparency = 1
radiusLabel.TextXAlignment = Enum.TextXAlignment.Left

--radius slider
radSliderGui, radSliderPosition = RbxGui.CreateSlider(6, 0, UDim2.new(0, 0, 0, 18))
radSliderGui.Size = UDim2.new(1,-2,0,20)
radSliderGui.Position = UDim2.new(0,0,0,24)
radSliderGui.Parent = containerFrame
radBar = radSliderGui:FindFirstChild("Bar")
radBar.Size = UDim2.new(1, -20, 0, 5)
radBar.Position = UDim2.new(0,10,0.5,-3)
radSliderPosition.Value = radius
radSliderPosition.Changed:connect(function()
	radius = radSliderPosition.Value
	radiusLabel.Text = SizeText(radius)
	destroySelection()
end)

local brushTypeChanged = function(newBrush)
	brushType = newBrush
end
-- brush type drop-down
local brushDropDown, forceSelection = RbxGui.CreateDropDownMenu(brushTypes, brushTypeChanged)
forceSelection("Square")
brushDropDown.Size = UDim2.new(1,-10,0.01,25)
brushDropDown.Position = UDim2.new(0,5,0,65)
brushDropDown.Parent = containerFrame

local brushLabel = radiusLabel:Clone()
brushLabel.Name = "BrushLabel"
brushLabel.Text = "Brush Type"
brushLabel.Position = UDim2.new(0,10,0,50)
brushLabel.Parent = containerFrame

self.Deactivation:connect(function()
	Off()
end)

--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
