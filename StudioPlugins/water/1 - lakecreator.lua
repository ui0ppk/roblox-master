--[[
This tool is designed to create bodies of water in terrain, below is an explanation of all the parameters that are adjustable (also accessible from the help menu in the tool)

Length: Adjusts the z-axis size of the lake
Width: Adjusts the x-axis size of the lake
Depth: Adjusts the y-axis size of the lake, this size is always from the currently selected terrain cell down

Curvature: 
Approximates a spherical or ellipsoid shape that curves the lake inward. 100% is the maximum amount of curve (like a perfectly round sphere), 0% is no curve (like an above-ground swimming pool)
Water Level:
The percentage to the top of the lake that is full of water (this is a percentage of the depth slider, not volume of the lake)

Lake Types:
Elliptical: draws a lake that approximates an ellpsoid
Circular: the same thing as Elliptical, except it locks the length and width sliders so they are always the same.
Rectangular: creates a lake to the exact shape of what is selected, which is always rectangular

Water Type:
These are directions that the water force will be applied to.  All cardinal directions are allows (x,y,z and their negatives)

Water Force: 
The amount of force to be applied to the specified water type.  Current choices are None, Small, Medium, Strong, and Max.

Create Walls: 
builds walls around the the lake when created, so all water is encapsulated on its sides by a material (material selected by Material Selection scrollbar)

Remove Above: 
Removes cells that are above the lake to be generated. The cells to be removed are the ones inside the red selection box.

Material Selection: 
The material used to create walls, if this option is selected.
]]

-- Local function definitions
local c = game.Workspace.Terrain
local SetCell = c.SetCell
local SetWaterCell = c.SetWaterCell
local SetCells = c.SetCells
local GetCell = c.GetCell
local WorldToCellPreferSolid = c.WorldToCellPreferSolid
local CellCenterToWorld = c.CellCenterToWorld
local AutoWedge = c.AutowedgeCell
local AutoWedgeCells = c.AutowedgeCells
local maxYExtents = c.MaxExtents.Max.Y


-- UI variables
local dragBar = nil
local containerFrame = nil
local buttonDownTick = nil
local doubleClickTime = 0.4
local didCancelCreate = false
local loadingFrame, updateLoadingGuiPercent, cancelLoadingPressed = nil
local currentSelection = nil
local groundSelection = nil
local shapes = {"Elliptical", "Circular", "Rectangular"}
local waterTypes = {"NegX","X","NegY","Y","NegZ","Z"}
local waterForces = {"None", "Small", "Medium", "Strong", "Max"}

-----------------
--DEFAULT VALUES-
-----------------
local loaded = false
local on = false
local debounce = false
local lastCell = nil

local zeroVector = Vector3.new(0,0,0)
local selectionUpdateFunc, destroyFunc = nil
local selectionGroundUpdateFunc, destroyGroundFunc = nil

-- values exposed to user
local currMaterial = Enum.CellMaterial.Grass
local radius = 15
local depth = 15
local xWidth = 15
local zWidth = 15
local waterLevel = 1.01
local percentOfSphere = 1.01
local createWalls = true
local removeAbove = true
local currLakeShape = nil
local currWaterType = 0
local currWaterForce = 0

--load libraries
local RbxGui = LoadLibrary("RbxGui")
local RbxUtil = LoadLibrary("RbxUtility")

---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
self.Deactivation:connect(function() Off() end)

mouse = self:GetMouse()
mouse.Button1Down:connect(function() onMouseButtonDown() end)
mouse.Button1Up:connect(function() onMouseButtonUp() end)
mouse.Move:connect(function() mouseMove() end)

toolbar = self:CreateToolbar("Water")
toolbarbutton = toolbar:CreateButton("", "Lake Creator", "lake.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	elseif loaded then
		On()
	end
end)

-----------------------
--FUNCTION DEFINITIONS-
-----------------------

-- srs translation from region3 to region3int16
function Region3ToRegion3int16(region3)
	local theLowVec = region3.CFrame.p - (region3.Size/2) + Vector3.new(2,2,2)
	local lowCell = WorldToCellPreferSolid(c,theLowVec)

	local theHighVec = region3.CFrame.p + (region3.Size/2) - Vector3.new(2,2,2)
	local highCell = WorldToCellPreferSolid(c, theHighVec)

	local highIntVec = Vector3int16.new(highCell.x,highCell.y,highCell.z)
	local lowIntVec = Vector3int16.new(lowCell.x,lowCell.y,lowCell.z)

	return Region3int16.new(lowIntVec,highIntVec)
end

function cancelLakeCreate()
	loadingFrame.Visible = false
	didCancelCreate = true
end

function makeRectangularLake(x,y,z)
	-- first, find region we need to cut out
	local selectionToCutout = currentSelection
	local selectionToFill = groundSelection
	local innerRegion = Region3.new(( selectionToFill.CFrame.p - selectionToFill.Size/2) + Vector3.new(4,4,4),
					( selectionToFill.CFrame.p + selectionToFill.Size/2) - Vector3.new(4,0,4))


	-- first, clear out all cells around selection if user specified so
	if removeAbove then
		SetCells(c,Region3ToRegion3int16(selectionToCutout),0,0,0)
	end

	-- Make walls around lake if we specified so
	if createWalls then
		-- now fill in all cells with our border material
		SetCells(c,Region3ToRegion3int16(selectionToFill), currMaterial,0,0)
		-- cutout the cells in the lake
		SetCells(c,Region3ToRegion3int16(innerRegion), 0,0,0)
	else
		SetCells(c,Region3ToRegion3int16(innerRegion),0,0,0)
	end

	-- last, fill with water to the appropriate level
	local waterHeight = ((selectionToFill.CFrame.p.y + selectionToFill.Size.y/2) - (selectionToFill.CFrame.p.y - selectionToFill.Size.y/2)) * waterLevel

	local lowerVec,higherVec = nil
	if createWalls then
		lowerVec = innerRegion.CFrame.p - innerRegion.Size/2
		higherVec = innerRegion.CFrame.p + innerRegion.Size/2
	else
		lowerVec = selectionToFill.CFrame.p - selectionToFill.Size/2
		higherVec = selectionToFill.CFrame.p + selectionToFill.Size/2
	end

	higherVec = Vector3.new(higherVec.x, lowerVec.y + waterHeight - 4, higherVec.z)
	local waterRegion = Region3ToRegion3int16(Region3.new(lowerVec,higherVec))
	-- todo: make this call do force/direction when can be mass set
	SetCells(c, waterRegion, Enum.CellMaterial.Water, 0, 0)
	for k = lowerVec.z + 2,higherVec.z - 2,4 do
		for j = lowerVec.y + 2,higherVec.y - 2,4 do
			for i = lowerVec.x + 2, higherVec.x - 2,4 do
				local cellPos = WorldToCellPreferSolid(c,Vector3.new(i,j,k))
				SetWaterCell(c,cellPos.x,cellPos.y,cellPos.z,currWaterForce,currWaterType)
			end
		end
	end
end

function drillDownColumn(x,y,z, percentInEllipsoid, lowY, absoluteWaterLine)
	local waterOrEmptyCells = 0
	if percentInEllipsoid <= 1 then
		waterOrEmptyCells = (math.pow(percentInEllipsoid,2) * depth * percentOfSphere) + (math.pow(percentInEllipsoid,2) * percentOfSphere)
		waterOrEmptyCells = math.max(depth - waterOrEmptyCells,0)
	end
	local waterCells = math.max(absoluteWaterLine - (y - waterOrEmptyCells),0)
	local emptyCells = waterOrEmptyCells - waterCells

	if waterCells > 0 then
		local emptyCellsLowY = math.max(y - emptyCells - waterCells + 1,2)
		-- todo: make this call do force/direction when can be mass set
		SetCells(c,Region3int16.new(Vector3int16.new(x,emptyCellsLowY,z),Vector3int16.new(x, y - emptyCells, z)),Enum.CellMaterial.Water,0,0)
		for i = emptyCellsLowY, y - emptyCells do
			SetWaterCell(c,x,i,z,currWaterForce,currWaterType)
		end
	end
	if emptyCells > 0 then
		local emptyCellsLowY = math.max(y - emptyCells + 1,2)
		SetCells(c,Region3int16.new(Vector3int16.new(x,emptyCellsLowY,z),Vector3int16.new(x,y,z)),Enum.CellMaterial.Empty,0,0)
	end
	if createWalls then
		local materialCells = math.max(depth - waterOrEmptyCells,0)
		if materialCells > 0 then
			SetCells(c,Region3int16.new(Vector3int16.new(x,math.max(lowY - 1,1),z),Vector3int16.new(x,lowY + (materialCells - 1),z)),currMaterial,0,0)
		end
	end
end

function makeEllipsoidLake(x,y,z)
	local startTime = tick()

	-- check to see if we should remove terrain above
	local topSelection = currentSelection
	local bottomSelection = groundSelection
	if topSelection ~= bottomSelection then
		local removeRegion = Region3.new(bottomSelection.CFrame.p + Vector3.new(-bottomSelection.Size.x/2,bottomSelection.Size.y/2,-bottomSelection.Size.z/2), topSelection.CFrame.p + topSelection.Size/2)
		SetCells(c, Region3ToRegion3int16(removeRegion), 0, 0, 0)
	end

	-- reset our processing gui
	updateLoadingGuiPercent(0)
	-- calculate how many steps we will use for processing (For updating processing gui)
	local totalSteps = (xWidth*2 + 3) * (zWidth*2 + 3)
	totalSteps = totalSteps * 2
	local currStep = 0
	local percentDone = 0

	-- some variables we can precompute before O(n^2) loop
	local lowY = y - (depth - 1)
	local absoluteWaterLine = y - (depth * (1 - waterLevel))

	-- loop thru xz plane to determine what cells to nuke/fill
	for k = z - (zWidth + 1), z + (zWidth + 1) do
		-- used in inner loop, but set here for effeciency
		local zEllipsoidComponent = math.pow((z - k)/zWidth,2)
		for i = x - (xWidth + 1), x + (xWidth + 1) do
			-- check to see if we have canceled, if not keep going
			if didCancelCreate then 
				didCancelCreate = false
				return 
			end

			-- using ellipsoid equation to detemine if cell should be deleted
			local percentInEllipsoid = math.pow((x - i)/xWidth,2) + zEllipsoidComponent

			-- at each xz coordinate, drill down depth and set cells in larger chunks
			drillDownColumn(i,y,k, percentInEllipsoid, lowY, absoluteWaterLine)

			-- update our processing gui
			currStep = currStep + 1
			percentDone = currStep/totalSteps
			updateLoadingGuiPercent(percentDone)

			-- do a calculated wait every once and awhile (stops studio from hanging)
			currStep = currStep + 1
			local first, fraction = math.modf(currStep/1000) 
			if fraction == 0 then wait() end
		end
		if not loadingFrame.Visible then
			loadingFrame.Visible = (tick() - startTime) > 0.5 and percentDone < 0.4
		end
	end
end

-- makes a crater at point (x, y, z) in cluster c
function makeCrater(x, y, z)
	-- we need to autowedge a bit outside of selection, allows for smoother terain around lakes
	local autoWedgeSelection = Region3.new( (groundSelection.CFrame.p - groundSelection.Size/2) - Vector3.new(8,8,8), 
											(groundSelection.CFrame.p + groundSelection.Size/2) + Vector3.new(8,8,8))

	disablePreview()

	
	if currLakeShape == "Rectangular" then -- simply cut out the selection box for the lake
		makeRectangularLake(x, y, z)
	else -- we want something to approximate curves for our lake
		makeEllipsoidLake(x, y, z)
	end

	AutoWedgeCells(c,Region3ToRegion3int16(autoWedgeSelection))

	loadingFrame.Visible = false
end

function disablePreview()
	if destroyFunc then
		destroyFunc()
		selectionUpdateFunc = nil
		destroyGroundFunc()
		selectionGroundUpdateFunc = nil
	end
end


-- creates selection boxes around the areas that will be affected by lake creator
function createSelections()
	selectionUpdateFunc, destroyFunc = RbxUtil.SelectTerrainRegion(Region3.new(zeroVector,zeroVector), 
																					BrickColor.new("Really red"),
																					true,
																					game.CoreGui)
	selectionGroundUpdateFunc, destroyGroundFunc = RbxUtil.SelectTerrainRegion( Region3.new(zeroVector,zeroVector), 
																			BrickColor.new("Toothpaste"),
																			true,
																			game.CoreGui)
end

-- moves selections boxes to the current cell region
function updateSelections()
	if not selectionUpdateFunc then
		createSelections()
		return
	end

	if not lastCell then
		lastCell = WorldToCellPreferSolid(c, mouse.Hit.p)
	end

	local selectionBelowMinYExtents = lastCell.y - depth + 1 <= 0

	local startPos = CellCenterToWorld(c, lastCell.x,lastCell.y,lastCell.z)

	local minOverallVec = nil
	if createWalls then
		minOverallVec = startPos - Vector3.new(xWidth*4,(depth - 1) *4,zWidth*4) - Vector3.new(2,6,2)
	else
		minOverallVec = startPos - Vector3.new(xWidth*4,(depth - 1) *4,zWidth*4) - Vector3.new(2,2,2)
	end
	local minDeletionVec = startPos - Vector3.new(xWidth*4,0,zWidth*4) - Vector3.new(2,-2.5,2)
	local maxOverallVec = startPos + Vector3.new(xWidth*4,(depth)*4,zWidth*4) + Vector3.new(2,2,2)
	local maxGroundVec = startPos + Vector3.new(xWidth*4,0,zWidth*4) + Vector3.new(2,2,2)

	if selectionBelowMinYExtents then

		local yMin = WorldToCellPreferSolid(c, Vector3.new(0,1,0)) + Vector3.new(2,2,2)
		minOverallVec = Vector3.new(minOverallVec.x,yMin,minOverallVec.z)
	end

	if removeAbove then
		currentSelection = Region3.new(minDeletionVec, maxOverallVec)
		selectionUpdateFunc(currentSelection)
		groundSelection = Region3.new(minOverallVec, maxGroundVec)
		selectionGroundUpdateFunc(groundSelection)
	else
		currentSelection = Region3.new(zeroVector,zeroVector)
		selectionUpdateFunc(currentSelection)
		groundSelection = Region3.new(minOverallVec, maxGroundVec)
		selectionGroundUpdateFunc(groundSelection)
	end
end

function onMouseButtonDown()
	buttonDownTick = tick()
end

function onMouseButtonUp()
	if tick() - buttonDownTick < doubleClickTime then
		onClicked()
	end
end


function onClicked()
	if on and not debounce then
		debounce = true
			local cellPos = WorldToCellPreferSolid(c, mouse.Hit.p)
			local x = cellPos.x
			local y = cellPos.y
			local z = cellPos.z

			makeCrater(x, y, z)
		debounce = false
	end
end

function mouseMove()
	if on then
		if not mouse.Target then disablePreview() return end
		if debounce then return end

		local newCell = WorldToCellPreferSolid(c, mouse.Hit.p)
		if newCell == lastCell then return end
		lastCell = newCell

		updateSelections()
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	dragBar.Visible = true
	didCancelCreate = false
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	dragBar.Visible = false
	disablePreview()
	cancelLakeCreate()
	on = false
end

function createMenuButton(size,position,text,fontsize,name,parent)
	local button = Instance.new("TextButton",parent)
	button.AutoButtonColor = false
	button.Name = name
	button.BackgroundTransparency = 1
	button.Position = position
	button.Size = size
	button.Font = Enum.Font.ArialBold
	button.FontSize = fontsize
	button.Text =  text
	button.TextColor3 = Color3.new(1,1,1)
	button.BorderSizePixel = 0
	button.BackgroundColor3 = Color3.new(20/255,20/255,20/255)

	button.MouseEnter:connect(function ( )
		if button.Selected then return end
		button.BackgroundTransparency = 0
	end)
	button.MouseLeave:connect(function ( )
		if button.Selected then return end
		button.BackgroundTransparency = 1
	end)

	return button

end


------
--GUI-
------
--screengui
local g = Instance.new("ScreenGui", game:GetService("CoreGui"))
g.Name = "LakeToolGui"

dragBar, containerFrame, helpFrame, closeEvent = RbxGui.CreatePluginFrame("Lake Creator",UDim2.new(0,163,0,400),UDim2.new(0,0,0,0),true,g)
dragBar.Visible = false
closeEvent.Event:connect(function (  )
	Off()
end)

helpFrame.Size = UDim2.new(0,300,0,552)

local helpFirstText = Instance.new("TextLabel",helpFrame)
helpFirstText.Name = "HelpText"
helpFirstText.Font = Enum.Font.ArialBold
helpFirstText.FontSize = Enum.FontSize.Size12
helpFirstText.TextColor3 = Color3.new(227/255,227/255,227/255)
helpFirstText.TextXAlignment = Enum.TextXAlignment.Left
helpFirstText.TextYAlignment = Enum.TextYAlignment.Top
helpFirstText.Position = UDim2.new(0,4,0,4)
helpFirstText.Size = UDim2.new(1,-8,0,177)
helpFirstText.BackgroundTransparency = 1
helpFirstText.TextWrap = true
helpFirstText.Text = "Length: Adjusts the z-axis size of the lake\nWidth: Adjusts the x-axis size of the lake\nDepth: Adjusts the y-axis size of the lake, this size is always from the currently selected terrain cell down\n\nCurvature: \nApproximates a spherical or ellipsoid shape that curves the lake inward. 100% is the maximum amount of curve (like a perfectly round sphere), 0% is no curve (like an above-ground swimming pool)\nWater Level:\nThe percentage to the top of the lake that is full of water (this is a percentage of the depth slider, not volume of the lake)"

local helpSecondText = helpFirstText:clone()
helpSecondText.Name = "HelpSecondText"
helpSecondText.Position = UDim2.new(0,0,1,0)
helpSecondText.Size = UDim2.new(1,0,0,212)
helpSecondText.Text = "Lake Types:\nElliptical: draws a lake that approximates an ellpsoid\nCircular: the same thing as Elliptical, except it locks the length and width sliders so they are always the same.\nRectangular: creates a lake to the exact shape of what is selected, which is always rectangular\n\nWater Type:\nThese are directions that the water force will be applied to.  All cardinal directions are allows (x,y,z and their negatives)\n\nWater Force: \nThe amount of force to be applied to the specified water type.  Current choices are None, Small, Medium, Strong, and Max."
helpSecondText.Parent = helpFirstText

local helpThirdText = helpSecondText:clone()
helpThirdText.Size = UDim2.new(1,0,0,159)
helpThirdText.Name = "HelpThirdText"
helpThirdText.Text = "Create Walls: \nbuilds walls around the the lake when created, so all water is encapsulated on its sides by a material (material selected by Material Selection scrollbar)\n\nRemove Above: \nRemoves cells that are above the lake to be generated. The cells to be removed are the ones inside the red selection box.\n\nMaterial Selection: \nThe material used to create walls, if this option is selected."
helpThirdText.Parent = helpSecondText

local linkImage = Instance.new("ImageLabel",containerFrame)
linkImage.Name = "LinkImage"
linkImage.BackgroundTransparency = 1
linkImage.Visible = false
linkImage.Size = UDim2.new(0,15,0,15)
linkImage.Position = UDim2.new(0,120,0,51)
game:GetService("ContentProvider"):Preload("http://gametest.roblox.com/asset/?id=129724452")
linkImage.Image = "http://gametest.roblox.com/asset/?id=129724452" --todo: put this in a better spot

--current width display label
local lengthLabel = Instance.new("TextLabel", containerFrame)
lengthLabel.Name = "LengthTextLabel"
lengthLabel.Position = UDim2.new(0, 8, 0, 10)
lengthLabel.Size = UDim2.new(0, 67, 0, 14)
lengthLabel.Text = ""
lengthLabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
lengthLabel.Font = Enum.Font.ArialBold
lengthLabel.FontSize = Enum.FontSize.Size14
lengthLabel.TextXAlignment = Enum.TextXAlignment.Left
lengthLabel.BackgroundTransparency = 1


local lengthSliderPosition, widthSliderPosition, widthSliderGui, lengthSliderGui = nil

--length slider
lengthSliderGui, lengthSliderPosition = RbxGui.CreateSlider(128, 0, UDim2.new(0,0,0,0))
lengthSliderGui.Parent = containerFrame
lengthSliderGui.Position = UDim2.new(0,1,0,26)
lengthSliderGui.Size = UDim2.new(0,160,0,20)
local lengthBar = lengthSliderGui:FindFirstChild("Bar")
lengthBar.Size = UDim2.new(1, -20, 0, 5)
lengthBar.Position = UDim2.new(0,10,0.5,-2)
lengthSliderPosition.Changed:connect(function()
	zWidth = lengthSliderPosition.Value
	lengthLabel.Text = "Length: ".. zWidth
	if currLakeShape == "Circular" then
		widthSliderPosition.Value = zWidth
	end
end)
lengthSliderPosition.Value = zWidth

--current width display label
local widthLabel = Instance.new("TextLabel", containerFrame)
widthLabel.Name = "WidthTextLabel"
widthLabel.Position = UDim2.new(0, 8, 0, 51)
widthLabel.Size = UDim2.new(0, 59, 0, 14)
widthLabel.Text = ""
widthLabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
widthLabel.Font = Enum.Font.ArialBold
widthLabel.FontSize = Enum.FontSize.Size14
widthLabel.TextXAlignment = Enum.TextXAlignment.Left
widthLabel.BackgroundTransparency = 1

--width slider
widthSliderGui, widthSliderPosition = RbxGui.CreateSlider(128,0, UDim2.new(0,0,0,0))
widthSliderGui.Parent = containerFrame
widthSliderGui.Position = UDim2.new(0,1,0,67)
widthSliderGui.Size = UDim2.new(0,160,0,20)
local widthBar = widthSliderGui:FindFirstChild("Bar")
widthBar.Size = UDim2.new(1, -20, 0, 5)
widthBar.Position = UDim2.new(0,10,0.5,-2)
widthSliderPosition.Changed:connect(function()
	xWidth = widthSliderPosition.Value
	widthLabel.Text = "Width: " .. xWidth
	if currLakeShape == "Circular" then
		lengthSliderPosition.Value = xWidth
	end
end)
widthSliderPosition.Value = xWidth

--current depth factor display label
local depthlabel = Instance.new("TextLabel", containerFrame)
depthlabel.Name = "DepthTextLabel"
depthlabel.Size = UDim2.new(0, 61, 0, 14)
depthlabel.Text = ""
depthlabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
depthlabel.Font = Enum.Font.ArialBold
depthlabel.FontSize = Enum.FontSize.Size14
depthlabel.Position = UDim2.new(0,8,0,90)
depthlabel.Size = UDim2.new(0,61,0,14)
depthlabel.TextXAlignment = Enum.TextXAlignment.Left
depthlabel.BackgroundTransparency = 1

--depth factor slider
local dfSliderGui, dfSliderPosition = RbxGui.CreateSlider(maxYExtents,0, UDim2.new(0,0,0,0))
dfSliderGui.Position = UDim2.new(0,1,0,106)
dfSliderGui.Size = UDim2.new(0,160,0,20)
dfSliderGui.Parent = containerFrame
local dfBar = dfSliderGui:FindFirstChild("Bar")
dfBar.Size = UDim2.new(1,-20,0,5)
dfBar.Position = UDim2.new(0,10,0.5,-2)
dfSliderPosition.Changed:connect(function()
	depth = dfSliderPosition.Value
	depthlabel.Text = "Depth: ".. depth
end)
dfSliderPosition.Value = depth

-- curvature text label
local curvatureLabel = Instance.new("TextLabel", containerFrame)
curvatureLabel.Name = "CurvatureTextLabel"
curvatureLabel.Size = UDim2.new(0, 106, 0, 14)
curvatureLabel.Text = ""
curvatureLabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
curvatureLabel.Font = Enum.Font.ArialBold
curvatureLabel.FontSize = Enum.FontSize.Size14
curvatureLabel.Position = UDim2.new(0,8,0,141)
curvatureLabel.Size = UDim2.new(0,106,0,14)
curvatureLabel.TextXAlignment = Enum.TextXAlignment.Left
curvatureLabel.BackgroundTransparency = 1

--curvature factor slider
local curveSliderGui, curveSliderPosition = RbxGui.CreateSlider(101,0, UDim2.new(0,0,0,0))
curveSliderGui.Position = UDim2.new(0,1,0,157)
curveSliderGui.Size = UDim2.new(0,160,0,20)
curveSliderGui.Parent = containerFrame
local curveBar = curveSliderGui:FindFirstChild("Bar")
curveBar.Size = UDim2.new(1,-20,0,5)
curveBar.Position = UDim2.new(0,10,0.5,-2)
curveSliderPosition.Changed:connect(function()
	percentOfSphere = (curveSliderPosition.Value - 1)/100
	curvatureLabel.Text = "Curvature: ".. tostring(percentOfSphere) * 100 .. "%"
end)
curveSliderPosition.Value = percentOfSphere * 100

-- water level text label
local waterLevelLabel = Instance.new("TextLabel", containerFrame)
waterLevelLabel.Name = "WaterLevelTextLabel"
waterLevelLabel.Size = UDim2.new(0.2, 0, 0.35, 0)
waterLevelLabel.Text = ""
waterLevelLabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
waterLevelLabel.Font = Enum.Font.ArialBold
waterLevelLabel.FontSize = Enum.FontSize.Size14
waterLevelLabel.Position = UDim2.new(0,8,0,180)
waterLevelLabel.Size = UDim2.new(0,118,0,14)
waterLevelLabel.TextXAlignment = Enum.TextXAlignment.Left
waterLevelLabel.BackgroundTransparency = 1

--depth factor slider
local waterLevelSliderGui, waterLevelSliderPosition = RbxGui.CreateSlider(101,0, UDim2.new(0,0,0,0))
waterLevelSliderGui.Position = UDim2.new(0,1,0,196)
waterLevelSliderGui.Size = UDim2.new(0,160,0,20)
waterLevelSliderGui.Parent = containerFrame
local waterLevelBar = waterLevelSliderGui:FindFirstChild("Bar")
waterLevelBar.Size = UDim2.new(1,-20,0,5)
waterLevelBar.Position = UDim2.new(0,10,0.5,-2)
waterLevelSliderPosition.Changed:connect(function()
	waterLevel = (waterLevelSliderPosition.Value - 1)/100
	waterLevelLabel.Text = "Water Level: ".. tostring(waterLevel) * 100 .. "%"
end)
waterLevelSliderPosition.Value = waterLevel * 100

local lakeShapeSelectedFunc = function ( item )
	currLakeShape = item
	if currLakeShape == "Circular" then
		local maxSize = math.max(xWidth,zWidth)
		lengthSliderPosition.Value = maxSize
		widthSliderPosition.Value = maxSize
		linkImage.Visible = true
	else
		linkImage.Visible = false
	end
end

local lakeShape, onShapeSelected = RbxGui.CreateDropDownMenu(shapes,lakeShapeSelectedFunc)
onShapeSelected("Elliptical")
lakeShape.ZIndex = 2
lakeShape.Size = UDim2.new(0,150,0,32)
lakeShape.Position = UDim2.new(0,5,0,244)
lakeShape.Parent = containerFrame

local lakeTypeLabel = curvatureLabel:clone()
lakeTypeLabel.Name = "LakeTypeLabel"
lakeTypeLabel.Text = "Lake Type"
lakeTypeLabel.Position = UDim2.new(0, 8, 0, 229)
lakeTypeLabel.Size = UDim2.new(0,64,0,14)
lakeTypeLabel.Parent = containerFrame

local waterTypeLabel = lakeTypeLabel:clone()
waterTypeLabel.Name = "WaterTypeLabel"
waterTypeLabel.Text = "Water Type"
waterTypeLabel.Position = UDim2.new(0, 8, 0, 278)
waterTypeLabel.Size = UDim2.new(0, 98, 0, 14)
waterTypeLabel.Parent = containerFrame

local waterTypeSelectedFunc = function ( item )
	currWaterType = item
end
local waterType, forceWaterType = RbxGui.CreateDropDownMenu(waterTypes,waterTypeSelectedFunc)
forceWaterType("NegX")
waterType.Size = UDim2.new(0,150,0,32)
waterType.Position = UDim2.new(0,5,0,294)
waterType.Parent = containerFrame

local waterForceLabel = lakeTypeLabel:clone()
waterForceLabel.Name = "WaterForceLabel"
waterForceLabel.Text = "Water Force"
waterForceLabel.Position = UDim2.new(0, 8, 0, 326)
waterForceLabel.Size = UDim2.new(0, 98, 0, 14)
waterForceLabel.Parent = containerFrame

local waterForceSelectedFunc = function ( item )
	currWaterForce = item
end
local waterForce, forceWaterForce = RbxGui.CreateDropDownMenu(waterForces,waterForceSelectedFunc)
forceWaterForce("None")
waterForce.Size = UDim2.new(0,150,0,32)
waterForce.Position = UDim2.new(0,5,0,340)
waterForce.Parent = containerFrame

-- create walls toggle
local createWallsLabel = Instance.new("TextLabel", containerFrame)
createWallsLabel.Name = "CreateWallsTextLabel"
createWallsLabel.Size = UDim2.new(0.2, 0, 0.35, 0)
createWallsLabel.Text = "Create Walls"
createWallsLabel.TextColor3 = Color3.new(0.95, 0.95, 0.95)
createWallsLabel.Font = Enum.Font.ArialBold
createWallsLabel.FontSize = Enum.FontSize.Size14
createWallsLabel.Position = UDim2.new(0,40,0,391)
createWallsLabel.Size = UDim2.new(0,80,0,14)
createWallsLabel.TextXAlignment = Enum.TextXAlignment.Left
createWallsLabel.BackgroundTransparency = 1

local createWallsToggle = Instance.new("TextButton",createWallsLabel)
createWallsToggle.Name = "CreateWallsToggle"
createWallsToggle.Style = Enum.ButtonStyle.RobloxButton
createWallsToggle.Size = UDim2.new(0,32,0,32)
createWallsToggle.Position = UDim2.new(0,-34,0.5,-16)
createWallsToggle.Text = "X"
createWallsToggle.Font = Enum.Font.ArialBold
createWallsToggle.FontSize = Enum.FontSize.Size18
createWallsToggle.TextColor3 = Color3.new(1,1,1)
createWallsToggle.MouseButton1Click:connect(function()
	createWalls = not createWalls
	if createWalls then
		createWallsToggle.Text = "X"
	else
		createWallsToggle.Text = ""
	end
end)

local removeAboveLabel = createWallsLabel:clone()
removeAboveLabel.Name = "RemoveAboveLabel"
removeAboveLabel.Text = "Remove Above"
removeAboveLabel.Position = UDim2.new(0,40,0,424)
removeAboveLabel.Size = UDim2.new(0,93,0,14)
removeAboveLabel.CreateWallsToggle.Name = "RemoveAboveToggle"
removeAboveLabel.RemoveAboveToggle.MouseButton1Click:connect(function( )
	removeAbove = not removeAbove
	if removeAbove then
		removeAboveLabel.RemoveAboveToggle.Text = "X"
	else
		removeAboveLabel.RemoveAboveToggle.Text = ""
	end
	updateSelections()
end)
removeAboveLabel.Parent = containerFrame

local materialSelectionGui,newMaterialEvent,forceMaterialFunc = RbxGui.CreateTerrainMaterialSelector(UDim2.new(1,0,0,82),UDim2.new(0,0,0,450))
materialSelectionGui.BackgroundTransparency = 1
materialSelectionGui.Parent = containerFrame
newMaterialEvent.Event:connect(function(newMat)
	currMaterial = newMat
end)

loadingFrame, updateLoadingGuiPercent, cancelLoadingPressed = RbxGui.CreateLoadingFrame("Processing")
loadingFrame.Visible = false
loadingFrame.Parent = g

cancelLoadingPressed.Event:connect(function() cancelLakeCreate() end)
--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
