-- Local function definitions
local c = game.Workspace.Terrain
local SetCell = c.SetCell
local GetCell = c.GetCell
local SetCells = c.SetCells
local AutoWedge = c.AutowedgeCells


-----------------
--DEFAULT VALUES-
-----------------
loaded = false
xpos = 0
zpos = 0
width = 512
length = 512
a = 30
f = 8
on = false




---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
self.Deactivation:connect(function()
	Off()
end)
toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("", "Terrain Generator", "terrain.png")
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

--makes a column of blocks from 1 up to height at location (x,z) in cluster c
function coordHeight(x, z, height)
	SetCells(c, Region3int16.new(Vector3int16.new(x, 1, z), Vector3int16.new(x, height, z)), 1, 0, 0)
end

--makes a heightmap for a layer of mountains (width x depth)
--with a width frequency wf and depthfrequency df (width should be divisible by wf, depth should be divisible by df) (for unsquished results, width/wf = depth/df)
--with a range of amplitudes between 0 and a
function mountLayer(width, depth, wf, df, a)
	local heightmap = {}
	for i = 0, width-1 do
		heightmap[i] = {}
		for k = 0, depth-1 do
			heightmap[i][k] = 0
		end
	end
	math.randomseed(tick())
	local corners = {}
	for i = 0,wf do
		corners[i] = {}
		for k = 0, df do
			corners[i][k] = a*math.random()
		end
	end
	for i = 0, wf do
		corners[i][0] = 0
		corners[i][math.floor(df)] = 0
	end
	for k = 0, df do
		corners[0][k]=0
		corners[math.floor(wf)][k]=0
	end

	for i = 0, width-(width/wf), width/wf do
		for k = 0, depth-(depth/df), depth/df do
			local c1 = corners[i/(width/wf)][k/(depth/df)]
			local c2 = corners[i/(width/wf)][(k+depth/df)/(depth/df)]
			local c3 = corners[(i+width/wf)/(width/wf)][k/(depth/df)]
			local c4 = corners[(i+width/wf)/(width/wf)][(k+depth/df)/(depth/df)]
			for x = i, i+(width/wf)-1 do
				for z = k, k+(depth/df)-1 do
					local avgc1c3 = (math.abs(x-i)*c3 + math.abs(x-(i+width/wf))*c1)/(width/wf)
					local avgc2c4 = (math.abs(x-i)*c4 + math.abs(x-(i+width/wf))*c2)/(width/wf)
					local avg = math.floor((math.abs(z-k)*avgc2c4 + math.abs(z-(k+depth/df))*avgc1c3)/(depth/df))
					if (avg > 100) then
						print(avg)
						avg = 1
					end
					heightmap[x][z]= avg
				end
			end
		end
	end
	return heightmap
end

--makes a shell around block at coordinate x, z using heightmap
function makeShell(x, z, heightmap, shellheightmap)
	local originalheight = heightmap[x][z]
	for i = x - 1, x + 1 do
		for k = z - 1, z + 1 do
			if shellheightmap[i][k] < originalheight then
				for h = originalheight, shellheightmap[i][k] - 2, -1 do
					if h > 0 then
						SetCell(c, i, h, k, 1, 0, 0)
					end
				end
				shellheightmap[i][k] = originalheight
			end
		end
	end
	return shellheightmap
end

function setCamera()
	local currCamera = game.Workspace.CurrentCamera
	currCamera.CoordinateFrame = CFrame.new(0, 400, 1600)
	currCamera.Focus = CFrame.new(0, 255, 0)
end


function onGenerate()
	generate.BorderColor3 = Color3.new(0, 0, 0)
	generate.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	generate.BackgroundTransparency = 0.5
	frame.Visible = false
	toolbarbutton:SetActive(false)
	on = false
	if g:FindFirstChild("gload") == nil then
		gload = Instance.new("Frame", g)
		gload.Name = "gload"
		gload.Position = UDim2.new(0.4, 0, 0.025, 0)
		gload.Size = UDim2.new(0.2, 0, 0.05, 0)
		gload.BackgroundTransparency = 0.5
		gfill = Instance.new("Frame", gload)
		gfill.Name = "gfill"
		gfill.Position = UDim2.new(0, 0, 0, 0)
		gfill.Size = UDim2.new(0, 0, 1, 0)
		gfill.BackgroundColor3 = Color3.new(1, 0.25, 0)
		gfill.BackgroundTransparency = 0.5
		gloadstatus = Instance.new("TextLabel", gload)
		gloadstatus.Position = UDim2.new(0.4, 0, 0.4, 0)
		gloadstatus.Size = UDim2.new(0.2, 0, 0.2, 0)
		gloadstatus.Text = ""
		gloadstatus.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
		gloadstatus.TextColor3 = Color3.new(1, 1, 1)
		gloadstatus.Font = Enum.Font.ArialBold
		gloadstatus.FontSize = Enum.FontSize.Size14
		gloadstatus.BorderColor3 = Color3.new(0, 0, 0)
		gloadstatus.BackgroundTransparency = 1
	else
		gload = g:FindFirstChild("gload")
		gfill = gload:FindFirstChild("gfill")
		gload.Visible = 1
		gfill.Visible = 1
		gfill.Size = UDim2.new(0, 0, 1, 0)
		gloadstatus.Text = ""
	end

	--Generate Terrain
	-- offset terrain additionally by whatever the smallest cell is
	--xpos2 = xpos + game.Workspace.Terrain.MaxExtents.Min.X
	--zpos2 = zpos + game.Workspace.Terrain.MaxExtents.Min.Z
	xpos2 = xpos - width/2
	zpos2 = zpos - length/2

	-- make sure we can see all the terrain
	if game.Workspace.CurrentCamera.CoordinateFrame.p.Y < 255 then 
		setCamera()
	end

	--make 3 layers of mountains (you can change the frequency and amplitude of each layer and add or remove layers as you see fit (but don't forget to add the new layers to the loop below)
	a1 = mountLayer(width, length, f*width/512, f*length/512, 3/5*a)
	a2 = mountLayer(width, length, 2*f*width/512, 2*f*length/512, 2/5*a)
	heightmap = {}
	for x = 0, width - 1 do
		heightmap[x + xpos2] = {}
		for z = 0, length - 1 do
			heightmap[x + xpos2][z + zpos2] = a1[x][z] + a2[x][z]
		end
	end
	shellheightmap = {}
	for x = 0, width - 1 do
		shellheightmap[x + xpos2] = {}
		for z = 0, length - 1 do
			shellheightmap[x + xpos2][z + zpos2] = heightmap[x + xpos2][z + zpos2]
		end
	end
	gprogress = 0
	gloadstatus.Text = "Generating Terrain Shape..."
	k = 1 + zpos2

	local waitCount = 0

	while k < length - 1 + zpos2 do
		for x = 1 + xpos2, width - 2 + xpos2 do
			coordHeight(x, k, heightmap[x][k])
			shellheightmap = makeShell(x, k, heightmap, shellheightmap)
		end
		k = k + 1
		gprogress = gprogress + 2/(length * 3)
		gfill.Size = UDim2.new(gprogress, 0, 1, 0)
		if waitCount > 5 then waitCount = 0 wait(0.01) else waitCount = waitCount + 1 end
	end
	gloadstatus.Text = "Smoothing Terrain..."
	k = 1 + zpos2
	waitCount = 0
	local maxHeight = -1
	local oldK = k
	while k < length - 1 + zpos2 do
		for x = 1 + xpos2, width - 2 + xpos2 do
			height = shellheightmap[x][k]
			if height == nil then
				height = -1
			end
			if height > maxHeight then
				maxHeight = height
			end
		end
		k = k + 1
		gprogress = gprogress + 1/(length * 3)
		gfill.Size = UDim2.new(gprogress, 0, 1, 0)
		if waitCount > 10 then
			waitCount = 0
			AutoWedge(c, Region3int16.new(Vector3int16.new(1 + xpos2, 0, oldK), Vector3int16.new(width - 2 + xpos2, maxHeight, k)))
			oldK = k+1
			maxHeight = -1
			wait()
		else waitCount = waitCount + 1 end
		if k == length - 2 + zpos2 then
			AutoWedge(c, Region3int16.new(Vector3int16.new(1 + xpos2, 0, oldK), Vector3int16.new(width - 2 + xpos2, maxHeight, k)))

			gfill.Parent.Visible = false
			gfill.Visible = false
		end
	end

	--Generate Terrain End
end

function onReset()
	reset.BorderColor3 = Color3.new(0, 0, 0)
	reset.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	reset.BackgroundTransparency = 0.5
	frame.Visible = false
	toolbarbutton:SetActive(false)
	on = false
	
	if g:FindFirstChild("rload") == nil then
		rload = Instance.new("Frame", g)
		rload.Name = "rload"
		rload.Position = UDim2.new(0.4, 0, 0.025, 0)
		rload.Size = UDim2.new(0.2, 0, 0.05, 0)
		rload.BackgroundTransparency = 0.5
		rfill = Instance.new("Frame", rload)
		rfill.Name = "rfill"
		rfill.Position = UDim2.new(0, 0, 0, 0)
		rfill.Size = UDim2.new(0, 0, 1, 0)
		rfill.BackgroundColor3 = Color3.new(1, 0.25, 0)
		rfill.BackgroundTransparency = 0.5
	else
		rload = g:FindFirstChild("rload")
		rfill = rload:FindFirstChild("rfill")
		rload.Visible = 1
		rfill.Visible = 1
		rfill.Size = UDim2.new(0, 0, 1, 0)
	end
	--Erase Terrain
	
	local xMax = c.MaxExtents.Max.X
	local yMax = c.MaxExtents.Max.Y
	local zMax = c.MaxExtents.Max.Z

	local xMin = c.MaxExtents.Min.X
	local yMin = c.MaxExtents.Min.Y
	local zMin = c.MaxExtents.Min.Z


	rprogress = 0
	
	local waitCount = 0

	z = zMax
	while z >= zMin do
		SetCells(c, Region3int16.new(Vector3int16.new(xMin, yMin, z), Vector3int16.new(xMax, yMax, z)), 0, 0, 0)
		z = z - 1

		rprogress = rprogress + 1/(zMax - zMin + 1)
		rfill.Size = UDim2.new(rprogress, 0, 1, 0)
		if z == zMin then
			reset.BorderColor3 = Color3.new(0, 0, 0)
			rfill.Parent.Visible = false
			rfill.Visible = false
		end
		if waitCount > 5 then waitCount = 0 wait(0.001) else waitCount = waitCount + 1 end
	end
	
	--Erase Terrain End
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	frame.Visible = true
	for w = 0, 0.6, 0.1 do
		frame.Size = UDim2.new(w, 0, w + 0.2, 0)
		wait(0.0000001)
	end
	title.Text = "Terrain Generation"
	xposl.Text = "X-Offset: "..xpos
	zposl.Text = "Z-Offset: "..zpos
	widthl.Text = "Width: "..width
	lengthl.Text = "Length: "..length
	ampl.Text = "Amplitude: "..a
	freql.Text = "Frequency: "..f
	generate.Text = "Generate"
	reset.Text = "Reset"
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	title.Text = ""
	xposl.Text = ""
	zposl.Text = ""
	widthl.Text = ""
	lengthl.Text = ""
	ampl.Text = ""
	freql.Text = ""
	generate.Text = ""
	reset.Text = ""
	for w = 0.6, 0, -0.1 do
		frame.Size = UDim2.new(w, 0, w + 0.2, 0)
		wait(0.0000001)
	end
	frame.Visible = false
	on = false
end




------
--GUI-
------

--load library for with sliders
local RbxGui = LoadLibrary("RbxGui")

--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))

--frame
frame = Instance.new("Frame", g)
frame.Size = UDim2.new(0.6, 0, 0.8, 0)
frame.Position = UDim2.new(0.2, 0, 0.1, 0)
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

--current xoffset display label
xposl = Instance.new("TextLabel", frame)
xposl.Position = UDim2.new(0.05, 0, 0.2, 0)
xposl.Size = UDim2.new(0.1, 0, 0.05, 0)
xposl.Text = ""
xposl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
xposl.TextColor3 = Color3.new(1, 1, 1)
xposl.Font = Enum.Font.ArialBold
xposl.FontSize = Enum.FontSize.Size14
xposl.BorderColor3 = Color3.new(0, 0, 0)
xposl.BackgroundTransparency = 1

--xoffset slider
xposSliderGui, xposSliderPosition = RbxGui.CreateSlider(128, 0, UDim2.new(0.2, 0, 0.22, 0))
xposSliderGui.Parent = frame
xposBar = xposSliderGui:FindFirstChild("Bar")
xposBar.Size = UDim2.new(0.75, 0, 0, 5)
xposSliderPosition.Value = (xpos+252)/4 + 1
xposSliderPosition.Changed:connect(function()
	xpos = (xposSliderPosition.Value - 1) * 4 - 252
	xposl.Text = "X-Offset: "..xpos
end)

--current zoffset display label
zposl = Instance.new("TextLabel", frame)
zposl.Position = UDim2.new(0.05, 0, 0.3, 0)
zposl.Size = UDim2.new(0.1, 0, 0.05, 0)
zposl.Text = ""
zposl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
zposl.TextColor3 = Color3.new(1, 1, 1)
zposl.Font = Enum.Font.ArialBold
zposl.FontSize = Enum.FontSize.Size14
zposl.BorderColor3 = Color3.new(0, 0, 0)
zposl.BackgroundTransparency = 1

--zoffset slider
zposSliderGui, zposSliderPosition = RbxGui.CreateSlider(128, 0, UDim2.new(0.2, 0, 0.32, 0))
zposSliderGui.Parent = frame
zposBar = zposSliderGui:FindFirstChild("Bar")
zposBar.Size = UDim2.new(0.75, 0, 0, 5)
zposSliderPosition.Value = (zpos+252)/4 + 1
zposSliderPosition.Changed:connect(function()
	zpos = (zposSliderPosition.Value - 1) * 4 - 252
	zposl.Text = "Z-Offset: "..zpos
end)

--current width display label
widthl = Instance.new("TextLabel", frame)
widthl.Position = UDim2.new(0.05, 0, 0.4, 0)
widthl.Size = UDim2.new(0.1, 0, 0.05, 0)
widthl.Text = ""
widthl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
widthl.TextColor3 = Color3.new(1, 1, 1)
widthl.Font = Enum.Font.ArialBold
widthl.FontSize = Enum.FontSize.Size14
widthl.BorderColor3 = Color3.new(0, 0, 0)
widthl.BackgroundTransparency = 1

--width slider
widthSliderGui, widthSliderPosition = RbxGui.CreateSlider(4, 0, UDim2.new(0.2, 0, 0.42, 0))
widthSliderGui.Parent = frame
widthbar = widthSliderGui:FindFirstChild("Bar")
widthbar.Size = UDim2.new(0.75, 0, 0, 5)
widthSliderPosition.Value = 4
widthSliderPosition.Changed:connect(function()
	width = math.pow(2, widthSliderPosition.Value + 5)
	widthl.Text = "Width: "..width
end)

--current length display label
lengthl = Instance.new("TextLabel", frame)
lengthl.Position = UDim2.new(0.05, 0, 0.5, 0)
lengthl.Size = UDim2.new(0.1, 0, 0.05, 0)
lengthl.Text = ""
lengthl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
lengthl.TextColor3 = Color3.new(1, 1, 1)
lengthl.Font = Enum.Font.ArialBold
lengthl.FontSize = Enum.FontSize.Size14
lengthl.BorderColor3 = Color3.new(0, 0, 0)
lengthl.BackgroundTransparency = 1

--length slider
lengthSliderGui, lengthSliderPosition = RbxGui.CreateSlider(4, 0, UDim2.new(0.2, 0, 0.52, 0))
lengthSliderGui.Parent = frame
lengthbar = lengthSliderGui:FindFirstChild("Bar")
lengthbar.Size = UDim2.new(0.75, 0, 0, 5)
lengthSliderPosition.Value = 4
lengthSliderPosition.Changed:connect(function()
	length = math.pow(2, lengthSliderPosition.Value + 5)
	lengthl.Text = "Length: "..length
end)

--current amplitude display label
ampl = Instance.new("TextLabel", frame)
ampl.Position = UDim2.new(0.05, 0, 0.6, 0)
ampl.Size = UDim2.new(0.1, 0, 0.05, 0)
ampl.Text = ""
ampl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
ampl.TextColor3 = Color3.new(1, 1, 1)
ampl.Font = Enum.Font.ArialBold
ampl.FontSize = Enum.FontSize.Size14
ampl.BorderColor3 = Color3.new(0, 0, 0)
ampl.BackgroundTransparency = 1

--amplitude slider
ampSliderGui, ampSliderPosition = RbxGui.CreateSlider(62, 0, UDim2.new(0.2, 0, 0.62, 0))
ampSliderGui.Parent = frame
ampbar = ampSliderGui:FindFirstChild("Bar")
ampbar.Size = UDim2.new(0.75, 0, 0, 5)
ampSliderPosition.Value = a + 1
ampSliderPosition.Changed:connect(function()
	a = ampSliderPosition.Value + 1
	ampl.Text = "Amplitude: "..a
end)

--current frequency display label
freql = Instance.new("TextLabel", frame)
freql.Position = UDim2.new(0.05, 0, 0.7, 0)
freql.Size = UDim2.new(0.1, 0, 0.05, 0)
freql.Text = ""
freql.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
freql.TextColor3 = Color3.new(1, 1, 1)
freql.Font = Enum.Font.ArialBold
freql.FontSize = Enum.FontSize.Size14
freql.BorderColor3 = Color3.new(0, 0, 0)
freql.BackgroundTransparency = 1

--frequency slider
freqSliderGui, freqSliderPosition = RbxGui.CreateSlider(8, 0, UDim2.new(0.2, 0, 0.72, 0))
freqSliderGui.Parent = frame
freqbar = freqSliderGui:FindFirstChild("Bar")
freqbar.Size = UDim2.new(0.75, 0, 0, 5)
freqSliderPosition.Value = 4
freqSliderPosition.Changed:connect(function()
	f = math.pow(2, freqSliderPosition.Value - 1)
	freql.Text = "Frequency: "..f
end)

--generate button
generate = Instance.new("TextButton", frame)
generate.Position = UDim2.new(0.275, 0, 0.85, 0)
generate.Size = UDim2.new(0.2, 0, 0.1, 0)
generate.Text = ""
generate.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
generate.TextColor3 = Color3.new(1, 1, 1)
generate.Font = Enum.Font.ArialBold
generate.FontSize = Enum.FontSize.Size14
generate.BorderColor3 = Color3.new(0, 0, 0)
generate.BackgroundTransparency = 0.5
generate.MouseEnter:connect(function()
	generate.BorderColor3 = Color3.new(1, 1, 1)
	generate.BackgroundColor3 = Color3.new(0, 0, 0)
	generate.BackgroundTransparency = 0.2
end)
generate.MouseButton1Click:connect(onGenerate)
generate.MouseLeave:connect(function()
	generate.BorderColor3 = Color3.new(0, 0, 0)
	generate.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	generate.BackgroundTransparency = 0.5
end)

--reset button
reset = Instance.new("TextButton", frame)
reset.Position = UDim2.new(0.525, 0, 0.85, 0)
reset.Size = UDim2.new(0.2, 0, 0.1, 0)
reset.Text = ""
reset.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
reset.TextColor3 = Color3.new(1, 1, 1)
reset.Font = Enum.Font.ArialBold
reset.FontSize = Enum.FontSize.Size14
reset.BorderColor3 = Color3.new(0, 0, 0)
reset.BackgroundTransparency = 0.5
reset.MouseEnter:connect(function()
	reset.BorderColor3 = Color3.new(1, 1, 1)
	reset.BackgroundColor3 = Color3.new(0, 0, 0)
	reset.BackgroundTransparency = 0.2
end)
reset.MouseButton1Click:connect(onReset)
reset.MouseLeave:connect(function()
	reset.BorderColor3 = Color3.new(0, 0, 0)
	reset.BackgroundColor3 = Color3.new(0.5, 0.5, 0.5)
	reset.BackgroundTransparency = 0.5
end)




--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Terrain Plugin Loaded")
