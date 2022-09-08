-- Local function definitions
local c = game.Workspace.Terrain
local SetCell = c.SetCell
local GetCell = c.GetCell
local WorldToCellPreferSolid = c.WorldToCellPreferSolid
local CellCornerToWorld = c.CellCornerToWorld
local WorldToCell = c.WorldToCell
local AutoWedge = c.AutowedgeCells

-----------------
--DEFAULT VALUES-
-----------------
loaded = false
on = false
materialNames = {"Empty", "Grass", "Sand", "Granite", "Brick"}
local currentMaterial = 1

local debounce = false
local startCell = nil
local endCell = nil
local regionSelected = false
local dragging = false
local cloning = false
local cloneHitCell = nil
local boxSize = Vector3.new(0, 0, 0)

-- for region resizing
local handles = Instance.new("Handles")
local handlesWereDragged = false
local dragStartX = 0
local dragStartY = 0
local dragStartZ = 0
local dragEndX = 0
local dragEndY = 0
local dragEndZ = 0



local selectionBox = Instance.new("Part")
selectionBox.Anchored = true
selectionBox.CanCollide = false
selectionBox.formFactor = "Custom"
selectionBox.BottomSurface = "Smooth"
selectionBox.TopSurface = "Smooth"
selectionBox.BrickColor = BrickColor.new("Bright blue")
selectionBox.Transparency = .7

local selectionLasso = Instance.new("SelectionBox")
selectionLasso.Adornee = selectionBox

local cloneBox = Instance.new("Part")
cloneBox.Anchored = true
cloneBox.CanCollide = false
cloneBox.formFactor = "Custom"
cloneBox.BottomSurface = "Smooth"
cloneBox.TopSurface = "Smooth"
cloneBox.BrickColor = BrickColor.new("Deep orange")
cloneBox.Transparency = .5

local cloneLasso = Instance.new("SelectionBox")
cloneLasso.Color = BrickColor.new("Deep orange")
cloneLasso.Adornee = cloneBox


---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
mouse = self:GetMouse()
mouse.Button1Down:connect(function() mouseDown(mouse) end)
mouse.Button1Up:connect(function() mouseUp(mouse) end)
mouse.Move:connect(function() mouseMove(mouse) end)

self.Deactivation:connect(function()
	Off()
end)
toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("", "RegionEdit", "")
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

function putCloneDown()
	startX = math.min(startCell.x, endCell.x)
	startY = math.min(startCell.y, endCell.y)
	startZ = math.min(startCell.z, endCell.z)
	
	endX = math.max(startCell.x, endCell.x)
	endY = math.max(startCell.y, endCell.y)
	endZ = math.max(startCell.z, endCell.Z)

	centerX = math.floor((startX + endX)/2)
	centerY = math.floor((startY + endY)/2)
	centerZ = math.floor((startZ + endZ)/2)

	xOffset = cloneHitCell.x - centerX
	yOffset = cloneHitCell.y - centerY
	zOffset = cloneHitCell.z - centerZ

	xStep = 1
	yStep = 1
	zStep = 1

	if xOffset > 0 then
		-- go backwards on x axis
		tempVal = startX
		startX = endX
		endX = tempVal

		xStep = -1
	end
	if yOffset > 0 then
		-- go backwards on y axis
		tempVal = startY
		startY = endY
		endY = tempVal

		yStep = -1
	end
	if zOffset > 0 then
		-- go backwards on z axis
		tempVal = startZ
		startZ = endZ
		endZ = tempVal

		zStep = -1
	end

	local count = 0
	for x = startX, endX, xStep do
		for y = startY, endY, yStep do
			for z = startZ, endZ, zStep do
				local mat, typ, orient = GetCell(c, x, y, z)
				SetCell(c, x+xOffset, y+yOffset, z+zOffset, mat, typ, orient)
				count = count + 1
			end
		end
		if count > 10000 then count = 0 wait() end
	end

	-- switch the selected region to the new one :)
	startCell = Vector3.new(startX+xOffset, startY+yOffset, startZ+zOffset)
	endCell = Vector3.new(endX+xOffset, endY+yOffset, endZ+zOffset)

	-- make the clone button look deselected now, if it's not
	if clone then
		clone.BorderColor3 = Color3.new(0, 0, 0)
		clone.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
		clone.BackgroundTransparency = 0.5
	end
end

function dragRegion(face, dist)
	startX = math.min(startCell.x, endCell.x)
	startY = math.min(startCell.y, endCell.y)
	startZ = math.min(startCell.z, endCell.z)
	
	endX = math.max(startCell.x, endCell.x)
	endY = math.max(startCell.y, endCell.y)
	endZ = math.max(startCell.z, endCell.Z)

	if face == Enum.NormalId.Front then
		dragStartZ = -math.floor(dist)
	elseif face == Enum.NormalId.Back then
		dragEndZ = math.floor(dist)
	elseif face == Enum.NormalId.Bottom then 
		dragStartY = -math.floor(dist)
	elseif face == Enum.NormalId.Top then
		dragEndY = math.floor(dist)
	elseif face == Enum.NormalId.Left then
		dragStartX = -math.floor(dist)
	elseif face == Enum.NormalId.Right then
		dragEndX = math.floor(dist)
	else return end

	-- update the box display
	local newStartX = math.min(startCell.x + dragStartX, endCell.x + dragEndX)
	local newStartY = math.min(startCell.y + dragStartY, endCell.y + dragEndY)
	local newStartZ = math.min(startCell.z + dragStartZ, endCell.z + dragEndZ)
	local newEndX = math.max(startCell.x + dragStartX, endCell.x + dragEndX)
	local newEndY = math.max(startCell.y + dragStartY, endCell.y + dragEndY)
	local newEndZ = math.max(startCell.z + dragStartZ, endCell.z + dragEndZ)

	local boxStart = CellCornerToWorld(c, newStartX, newStartY, newStartZ)
	local boxEnd = CellCornerToWorld(c, newEndX + 1, newEndY + 1, newEndZ + 1)
	boxSize = boxEnd - boxStart
	local boxCenter = (boxEnd + boxStart)/2

	selectionBox.Size = boxSize
	selectionBox.CFrame = CFrame.new(boxCenter)

	handlesWereDragged = true
end


function mouseDown(mouse)
	if on and not debounce and mouse.Target then
		startCell = nil
		endCell = nil
		regionSelected = false
		handles.Visible = false
		selectionBox.Parent = nil
		selectionLasso.Visible = false
		
		local tempCell = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		if GetCell(c, tempCell.x, tempCell.y, tempCell.z).Value == 0 then return end

		dragging = true

		startCell = tempCell
	elseif on and cloning and cloneHitCell and regionSelected then
		regionSelected = false
		handles.Visible = false
		print("Cloning")
		putCloneDown()
		
		-- update selection box to new coords
		startX = math.min(startCell.x, endCell.x)
		startY = math.min(startCell.y, endCell.y)
		startZ = math.min(startCell.z, endCell.z)
		endX = math.max(startCell.x, endCell.x)
		endY = math.max(startCell.y, endCell.y)
		endZ = math.max(startCell.z, endCell.Z)
		local boxStart = CellCornerToWorld(c, startX, startY, startZ)
		local boxEnd = CellCornerToWorld(c, endX + 1, endY + 1, endZ + 1)
		boxSize = boxEnd - boxStart
		local boxCenter = (boxEnd + boxStart)/2
		selectionBox.Size = boxSize
		selectionBox.CFrame = CFrame.new(boxCenter)

		--startCell = nil
		--endCell = nil
		--selectionLasso.Visible = false
		handles.Visible = true
		regionSelected = true
		cloneLasso.Visible = false
		cloning = false
		debounce = false
	end
end



function mouseUp(mouse)
	dragging = false
	if not startCell or not endCell then return end
	if cloning then return end
	regionSelected = true
	print("Region Selected: "..startCell.x..", "..startCell.y..", "..startCell.z.." to "..endCell.x..", "..endCell.y..", "..endCell.z)

	-- enforce ordering on startCell and endCell
	startX = math.min(startCell.x, endCell.x)
	startY = math.min(startCell.y, endCell.y)
	startZ = math.min(startCell.z, endCell.z)
	
	endX = math.max(startCell.x, endCell.x)
	endY = math.max(startCell.y, endCell.y)
	endZ = math.max(startCell.z, endCell.Z)

	startCell = Vector3.new(startX, startY, startZ)
	endCell = Vector3.new(endX, endY, endZ)

	-- handle handle    [lulz]
	handles.Visible = true

	if handlesWereDragged then
		newStartX = math.min(startCell.x + dragStartX, endCell.x + dragEndX)
		newStartY = math.min(startCell.y + dragStartY, endCell.y + dragEndY)
		newStartZ = math.min(startCell.z + dragStartZ, endCell.z + dragEndZ)
		newEndX = math.max(startCell.x + dragStartX, endCell.x + dragEndX)
		newEndY = math.max(startCell.y + dragStartY, endCell.y + dragEndY)
		newEndZ = math.max(startCell.z + dragStartZ, endCell.z + dragEndZ)
		startCell = Vector3.new(newStartX, newStartY, newStartZ)
		endCell = Vector3.new(newEndX, newEndY, newEndZ)
		handlesWereDragged = false
	end

	dragStartX = 0
	dragStartY = 0
	dragStartZ = 0
	dragEndX = 0
	dragEndY = 0
	dragEndZ = 0
end


function mouseMove(mouse)
	if on and cloning and regionSelected then
		cloneHitCell = WorldToCell(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		
		startX = math.min(startCell.x, endCell.x)
		startY = math.min(startCell.y, endCell.y)
		startZ = math.min(startCell.z, endCell.z)
	
		endX = math.max(startCell.x, endCell.x)
		endY = math.max(startCell.y, endCell.y)
		endZ = math.max(startCell.z, endCell.Z)

		centerX = math.floor((startX + endX)/2)
		centerY = math.floor((startY + endY)/2)
		centerZ = math.floor((startZ + endZ)/2)

		xOffset = cloneHitCell.x - centerX
		yOffset = cloneHitCell.y - centerY
		zOffset = cloneHitCell.z - centerZ

		cloneBox.CFrame = selectionBox.CFrame + 4 * Vector3.new(xOffset, yOffset, zOffset) -- since cells are 4x4x4
		cloneLasso.Visible = true
		return
	end


	if not startCell or not dragging or debounce then return end

	endCell = WorldToCellPreferSolid(c, Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
	startX = math.min(startCell.x, endCell.x)
	startY = math.min(startCell.y, endCell.y)
	startZ = math.min(startCell.z, endCell.z)
	
	endX = math.max(startCell.x, endCell.x)
	endY = math.max(startCell.y, endCell.y)
	endZ = math.max(startCell.z, endCell.Z)

	local boxStart = CellCornerToWorld(c, startX, startY, startZ)
	local boxEnd = CellCornerToWorld(c, endX + 1, endY + 1, endZ + 1)
	boxSize = boxEnd - boxStart
	local boxCenter = (boxEnd + boxStart)/2

	selectionBox.Size = boxSize
	selectionBox.CFrame = CFrame.new(boxCenter)
	--selectionBox.Parent = game.Workspace
	selectionLasso.Visible = true
end

function onFill(newMaterial)
	if debounce then
		print("Cannot perform that operation until the previous one is complete.")
		return
	elseif not regionSelected then
		print("Please select a region first.")
		return
	else
		debounce = true
		print("Filling region.")
		startX = math.min(startCell.x, endCell.x)
		startY = math.min(startCell.y, endCell.y)
		startZ = math.min(startCell.z, endCell.z)
	
		endX = math.max(startCell.x, endCell.x)
		endY = math.max(startCell.y, endCell.y)
		endZ = math.max(startCell.z, endCell.Z)

		--c:SetCells(Region3int16.new(Vector3int16.new(startX, startY, startZ), Vector3int16.new(endX-startX, endY-startY, endZ-startZ)), newMaterial, 0, 0)
		local count = 0
		for x = startX, endX do
			for y = startY, endY do
				for z = startZ, endZ do
					SetCell(c, x, y, z, newMaterial, 0, 0)
					count = count + 1
				end
			end
			if count > 10000 then count = 0 wait() end
		end

		debounce = false
	end
end

function onMaterialChange(newMaterial)
	if debounce then
		print("Cannot perform that operation until the previous one is complete.")
		return
	elseif not regionSelected then
		print("Please select a region first.")
		return
	elseif newMaterial == 0 then
		print("Cannot change material to Empty.  Use Fill button to erase a region.")
		return
	else
		debounce = true
		print("Changing material of region.")
		startX = math.min(startCell.x, endCell.x)
		startY = math.min(startCell.y, endCell.y)
		startZ = math.min(startCell.z, endCell.z)
	
		endX = math.max(startCell.x, endCell.x)
		endY = math.max(startCell.y, endCell.y)
		endZ = math.max(startCell.z, endCell.Z)

		local count = 0
		for x = startX, endX do
			for y = startY, endY do
				for z = startZ, endZ do
					local mat, typ, orient = GetCell(c, x, y, z)
					if mat.Value > 0 then SetCell(c, x, y, z, newMaterial, typ, orient) end
					count = count + 1
				end
			end
			if count > 10000 then count = 0 wait() end
		end

		debounce = false
	end
end

function onRotate()
	if debounce then
		print("Cannot perform that operation until the previous one is complete.")
		return
	elseif not regionSelected then
		print("Please select a region first.")
		return
	else
		debounce = true
		print("Rotating region.")
		startX = math.min(startCell.x, endCell.x)
		startY = math.min(startCell.y, endCell.y)
		startZ = math.min(startCell.z, endCell.z)
	
		endX = math.max(startCell.x, endCell.x)
		endY = math.max(startCell.y, endCell.y)
		endZ = math.max(startCell.z, endCell.Z)

		local xCenter = (startX+endX)/2
		local zCenter = (startZ+endZ)/2
		local xSize = (endX - startX)
		local zSize = (endZ - startZ)
		local xHalfSize = math.floor(xSize / 2)
		local zHalfSize = math.floor(zSize / 2)
		local newXCenter = math.floor(xCenter) - zHalfSize + zSize/2
		local newZCenter = math.floor(zCenter) - xHalfSize + xSize/2

		for y = startY, endY do
			-- extract the y-slice

			local cellTable = {}
			

			for x = startX, endX do
				xIndex = x - xCenter
				if (startX+endX)%2 == 1 then
					if xIndex > 0 then xIndex = math.ceil(xIndex)
					else xIndex = math.floor(xIndex) end
				end

				cellTable[xIndex] = {}
				for z = startZ, endZ do
					local mat, typ, orient = GetCell(c, x, y, z)
					zIndex = z - zCenter
					if (startZ+endZ)%2 == 1 then
						if zIndex > 0 then zIndex = math.ceil(zIndex)
						else zIndex = math.floor(zIndex) end
					end
					
					-- store in our table
					cellTable[xIndex][zIndex] = Vector3.new(mat.Value, typ.Value, orient.Value)
					
					-- and clear the y-slice, so we don't get duplication errors
					SetCell(c, x, y, z, 0, 0, 0)
				end
			end
			wait()

			-- put it back, rotated into the y-slice
			for x = math.floor(xCenter) - zHalfSize, math.floor(xCenter) - zHalfSize + zSize  do
				for z = math.floor(zCenter) - xHalfSize, math.floor(zCenter) - xHalfSize + xSize do
					xIndex = x - newXCenter
					zIndex = z - newZCenter
					if zSize%2 == 1 then
						if xIndex > 0 then xIndex = math.ceil(xIndex)
						else xIndex = math.floor(xIndex) end
					end
					if xSize%2 == 1 then
						if zIndex > 0 then zIndex = math.ceil(zIndex)
						else zIndex = math.floor(zIndex) end
					end					
					SetCell(c, x, y, z, cellTable[zIndex][-xIndex].x, cellTable[zIndex][-xIndex].y, (cellTable[zIndex][-xIndex].z + 3)%4)
				end
			end
			wait()
		end

		-- update selection to follow rotation
		startCell = Vector3.new(math.floor(xCenter) - zHalfSize, startY, math.floor(zCenter) - xHalfSize)
		endCell = Vector3.new(math.floor(xCenter) - zHalfSize + zSize, endY, math.floor(zCenter) - xHalfSize + xSize)

		local boxStart = CellCornerToWorld(c, startCell.X, startCell.Y, startCell.Z)
		local boxEnd = CellCornerToWorld(c, endCell.X + 1, endCell.Y + 1, endCell.Z + 1)
		boxSize = boxEnd - boxStart
		local boxCenter = (boxEnd + boxStart)/2

		selectionBox.Size = boxSize
		selectionBox.CFrame = CFrame.new(boxCenter)
		selectionLasso.Visible = true
		debounce = false
	end
end

function onAutoWedge()
	if debounce then
		print("Cannot perform that operation until the previous one is complete.")
		return
	elseif not regionSelected then
		print("Please select a region first.")
		return
	elseif newMaterial == 0 then
		print("Cannot change material to Empty.  Use Fill button to erase a region.")
		return
	else
		debounce = true
		print("Smoothing region.")
		startX = math.min(startCell.x, endCell.x)
		startY = math.min(startCell.y, endCell.y)
		startZ = math.min(startCell.z, endCell.z)
	
		endX = math.max(startCell.x, endCell.x)
		endY = math.max(startCell.y, endCell.y)
		endZ = math.max(startCell.z, endCell.Z)

		local count = 0
		local numCellsInLayer = (endY - startY)*(endX - startX)
		for z = startZ, endZ do
			AutoWedge(c, Region3int16.new(Vector3int16.new(startX, startY, z), Vector3int16.new(endX, endY, z)))
			if count > 10000 then count = 0 wait() else count = count + numCellsInLayer end
		end

		debounce = false
	end
end


function onClone()
	if debounce then
		print("Cannot perform that operation until the previous one is complete.")
		return
	elseif not regionSelected then
		print("Please select a region first.")
		return
	else
		debounce = true
		cloneHitCell = nil
		cloning = true
		cloneBox.Size = boxSize
	end
end



function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)

	frame.Visible = true
	for w = 0, 0.2, 0.1 do
		frame.Size = UDim2.new(w, 0, w, 0)
		wait(0.0000001)
	end
	title.Text = "Region Editing"
	--freql.Text = materialString
	fill.Text = "Fill"
	matChange.Text = "Set Material"
	clone.Text = "Clone"
	autoWedge.Text = "Smooth"
	rotate.Text = "Rotate"

	on = true
end

function Off()
	selectionLasso.Visible = false
	cloneLasso.Visible = false
	startCell = nil
	endCell = nil
	regionSelected = false
	handles.Visible = false
	selectionBox.Parent = nil

	-- reset variables
	currentMaterial = 1
	startCell = nil
	endCell = nil
	regionSelected = false
	dragging = false
	cloning = false
	cloneHitCell = nil
	boxSize = Vector3.new(0, 0, 0)
	debounce = false

	-- for region resizing
	handlesWereDragged = false
	dragStartX = 0
	dragStartY = 0
	dragStartZ = 0
	dragEndX = 0
	dragEndY = 0
	dragEndZ = 0

	toolbarbutton:SetActive(false)

	title.Text = ""
	fill.Text = ""
	clone.Text = ""
	rotate.Text = ""
	matChange.Text = ""
	autoWedge.Text = ""
	for w = 0.2, 0, -0.1 do
		frame.Size = UDim2.new(w, 0, w, 0)
		wait(0.0000001)
	end
	frame.Visible = false
	on = false
end


function updateMaterialChoice(choice)
	if choice == "Grass" then
		currentMaterial = 1
	elseif choice == "Sand" then
		currentMaterial = 2
	elseif choice == "Empty" then
		currentMaterial = 0
	elseif choice == "Brick" then
		currentMaterial = 3
	elseif choice == "Granite" then
		currentMaterial = 4
	end
end


------
--GUI-
------

--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))
selectionLasso.Visible = false
selectionLasso.Parent = g
cloneLasso.Visible = false
cloneLasso.Parent = g
handles.Visible = false
handles.Adornee = selectionBox
handles.Parent = g
handles.MouseDrag:connect(dragRegion)


--load library for with sliders
local RbxGui = LoadLibrary("RbxGui")

--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))

--frame
frame = Instance.new("Frame", g)
frame.Size = UDim2.new(0.2, 0, 0.2, 0)
frame.Position = UDim2.new(0, 0, 0, 0)
frame.BackgroundTransparency = 0.5
frame.Visible = false

--title
title = Instance.new("TextLabel", frame)
title.Position = UDim2.new(0.4, 0, 0.05, 0)
title.Size = UDim2.new(0.2, 0, 0.05, 0)
title.Text = ""
title.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
title.TextColor3 = Color3.new(1, 1, 1)
title.Font = Enum.Font.ArialBold
title.FontSize = Enum.FontSize.Size24
title.BorderColor3 = Color3.new(0, 0, 0)
title.BackgroundTransparency = 1

--[[current frequency display label
freql = Instance.new("TextLabel", frame)
freql.Position = UDim2.new(0.15, 0, 0.4, 0)
freql.Size = UDim2.new(0.1, 0, 0.05, 0)
freql.Text = ""
freql.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
freql.TextColor3 = Color3.new(1, 1, 1)
freql.Font = Enum.Font.ArialBold
freql.FontSize = Enum.FontSize.Size14
freql.BorderColor3 = Color3.new(0, 0, 0)
freql.BackgroundTransparency = 1

--frequency slider
freqSliderGui, freqSliderPosition = RbxGui.CreateSlider(3, 0, UDim2.new(0.2, 0, 0.72, 0))
freqSliderGui.Parent = frame
freqbar = freqSliderGui:FindFirstChild("Bar")
freqbar.Size = UDim2.new(0.75, 0, 0, 5)
freqSliderPosition.Value = 2
freqSliderPosition.Changed:connect(function()
	f = freqSliderPosition.Value
	if (f == 1) then materialString = "Material: Empty"
	elseif (f == 2) then materialString = "Material: Grass"
	elseif (f == 3) then materialString = "Material: Sand"
	else materialString = "Material: Undefined" end
	freql.Text = materialString
end)]]--

local materialSelect, externalMaterialSelectFunction = RbxGui.CreateDropDownMenu(materialNames, updateMaterialChoice)
materialSelect.Name = "MaterialSelect"
materialSelect.Position = UDim2.new(.05, 0, .2, 0)
materialSelect.Size = UDim2.new(.6, 0, .3, 0)
materialSelect.Parent = frame



--fill region button
fill = Instance.new("TextButton", frame)
fill.Position = UDim2.new(0.370, 0, 0.85, 0)
fill.Size = UDim2.new(0.15, 0, 0.1, 0)
fill.Text = ""
fill.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
fill.TextColor3 = Color3.new(1, 1, 1)
fill.Font = Enum.Font.ArialBold
fill.FontSize = Enum.FontSize.Size14
fill.BorderColor3 = Color3.new(0, 0, 0)
fill.BackgroundTransparency = 0.5
fill.MouseEnter:connect(function()
	fill.BorderColor3 = Color3.new(1, 1, 1)
	fill.BackgroundColor3 = Color3.new(0, 0, 0)
	fill.BackgroundTransparency = 0.2
end)
fill.MouseButton1Click:connect(function() onFill(currentMaterial) end)
fill.MouseLeave:connect(function()
	fill.BorderColor3 = Color3.new(0, 0, 0)
	fill.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	fill.BackgroundTransparency = 0.5
end)

matChange = Instance.new("TextButton", frame)
matChange.Position = UDim2.new(0.025, 0, 0.85, 0)
matChange.Size = UDim2.new(0.325, 0, 0.1, 0)
matChange.Text = ""
matChange.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
matChange.TextColor3 = Color3.new(1, 1, 1)
matChange.Font = Enum.Font.ArialBold
matChange.FontSize = Enum.FontSize.Size14
matChange.BorderColor3 = Color3.new(0, 0, 0)
matChange.BackgroundTransparency = 0.5
matChange.MouseEnter:connect(function()
	matChange.BorderColor3 = Color3.new(1, 1, 1)
	matChange.BackgroundColor3 = Color3.new(0, 0, 0)
	matChange.BackgroundTransparency = 0.2
end)
matChange.MouseButton1Click:connect(function() onMaterialChange(currentMaterial) end)
matChange.MouseLeave:connect(function()
	matChange.BorderColor3 = Color3.new(0, 0, 0)
	matChange.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	matChange.BackgroundTransparency = 0.5
end)



--clone region button
clone = Instance.new("TextButton", frame)
clone.Position = UDim2.new(0.5325, 0, 0.85, 0)
clone.Size = UDim2.new(0.2, 0, 0.1, 0)
clone.Text = ""
clone.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
clone.TextColor3 = Color3.new(1, 1, 1)
clone.Font = Enum.Font.ArialBold
clone.FontSize = Enum.FontSize.Size14
clone.BorderColor3 = Color3.new(0, 0, 0)
clone.BackgroundTransparency = 0.5
clone.MouseEnter:connect(function()
	clone.BorderColor3 = Color3.new(1, 1, 1)
	clone.BackgroundColor3 = Color3.new(0, 0, 0)
	clone.BackgroundTransparency = 0.2
end)
clone.MouseButton1Click:connect(onClone)
clone.MouseLeave:connect(function()
	clone.BorderColor3 = Color3.new(0, 0, 0)
	clone.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	clone.BackgroundTransparency = 0.5
end)

--autowedge button
autoWedge = Instance.new("TextButton", frame)
autoWedge.Position = UDim2.new(0.75, 0, 0.85, 0)
autoWedge.Size = UDim2.new(0.2, 0, 0.1, 0)
autoWedge.Text = ""
autoWedge.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
autoWedge.TextColor3 = Color3.new(1, 1, 1)
autoWedge.Font = Enum.Font.ArialBold
autoWedge.FontSize = Enum.FontSize.Size14
autoWedge.BorderColor3 = Color3.new(0, 0, 0)
autoWedge.BackgroundTransparency = 0.5
autoWedge.MouseEnter:connect(function()
	autoWedge.BorderColor3 = Color3.new(1, 1, 1)
	autoWedge.BackgroundColor3 = Color3.new(0, 0, 0)
	autoWedge.BackgroundTransparency = 0.2
end)
autoWedge.MouseButton1Click:connect(onAutoWedge)
autoWedge.MouseLeave:connect(function()
	autoWedge.BorderColor3 = Color3.new(0, 0, 0)
	autoWedge.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	autoWedge.BackgroundTransparency = 0.5
end)

--rotate button
rotate = Instance.new("TextButton", frame)
rotate.Position = UDim2.new(0.75, 0, 0.45, 0)
rotate.Size = UDim2.new(0.2, 0, 0.1, 0)
rotate.Text = ""
rotate.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
rotate.TextColor3 = Color3.new(1, 1, 1)
rotate.Font = Enum.Font.ArialBold
rotate.FontSize = Enum.FontSize.Size14
rotate.BorderColor3 = Color3.new(0, 0, 0)
rotate.BackgroundTransparency = 0.5
rotate.MouseEnter:connect(function()
	rotate.BorderColor3 = Color3.new(1, 1, 1)
	rotate.BackgroundColor3 = Color3.new(0, 0, 0)
	rotate.BackgroundTransparency = 0.2
end)
rotate.MouseButton1Click:connect(onRotate)
rotate.MouseLeave:connect(function()
	rotate.BorderColor3 = Color3.new(0, 0, 0)
	rotate.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	rotate.BackgroundTransparency = 0.5
end)

--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Region Editing Plugin Loaded")