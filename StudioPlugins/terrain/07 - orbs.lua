-----------------
--DEFAULT VALUES-
-----------------
loaded = false
on = false
r = 20
d = 50




---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
mouse = self:GetMouse()
mouse.Button1Down:connect(function() onClicked(mouse) end)
self.Deactivation:connect(function()
	Off()
end)
toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("", "Orb", "orbs.png")
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

--find height at coordinate x, z
function getHeight(x, z)
	c = game.Workspace.Terrain
	h = 0
	material, wedge, rotation = c:GetCell(x, h + 1, z)
	while material.Value > 0 do
		h = h + 1
		material, wedge, rotation = c:GetCell(x, h + 1, z)
	end
	return h
end

--makes a wedge at location x, y, z using info from heightmap instead of GetCell
--sets cell x, y, z to default material if parameter is provided, if not sets cell x, y, z to be whatever material it previously was
--returns true if made a wedge, false if the cell remains a block
function MakeWedge(x, y, z, heightmap, defaultmaterial)
	local c = game.Workspace.Terrain
	--gather info about all the cells around x, y, z
	local surroundings = {} --surroundings is a 3 x 3 x 3 array of the cells adjacent to x, y, z (1 if a cell is there, 0 if not)
	for i = x - 1, x + 1 do
		surroundings[i] = {}
		for j = y - 1, y + 1 do
			surroundings[i][j] = {}
			for k = z - 1, z + 1 do
				heightmapi = heightmap[i]
				height = nil
				if heightmapi then
					height = heightmapi[k]
				end
				if height and height >= j then
					surroundings[i][j][k] = 1
				else
					surroundings[i][j][k] = 0
				end
			end
		end
	end
	--make some useful arrays and counters
	local sides = {} --sides is an array of the material of the 4 adjacent sides
	sides[0] = surroundings[x - 1][y][z]
	sides[1] = surroundings[x][y][z + 1]
	sides[2] = surroundings[x + 1][y][z]
	sides[3] = surroundings[x][y][z - 1]
	local adjacentSides = 0
	for n = 0, 3 do
		if sides[n] > 0 then
			adjacentSides = adjacentSides + 1
		end
	end
	local sidesAbove = {} --sides is an array of the material of the 4 adjacent sides 1 height above
	sidesAbove[0] = surroundings[x - 1][y + 1][z]
	sidesAbove[1] = surroundings[x][y + 1][z + 1]
	sidesAbove[2] = surroundings[x + 1][y + 1][z]
	sidesAbove[3] = surroundings[x][y + 1][z - 1]
	local adjacentSidesAbove = 0
	for n = 0, 3 do
		if sidesAbove[n] > 0 then
			adjacentSidesAbove = adjacentSidesAbove + 1
		end
	end
	local corners = {} --corners is an array of the material of the 4 adjacent corners
	corners[0] = surroundings[x - 1][y][z - 1]
	corners[1] = surroundings[x - 1][y][z + 1]
	corners[2] = surroundings[x + 1][y][z + 1]
	corners[3] = surroundings[x + 1][y][z - 1]
	local adjacentCorners = 0
	for n = 0, 3 do
		if corners[n] > 0 then
			adjacentCorners = adjacentCorners + 1
		end
	end
	local cornersAbove = {} --corners is an array of the material of the 4 adjacent corners 1 height above
	cornersAbove[0] = surroundings[x - 1][y + 1][z - 1]
	cornersAbove[1] = surroundings[x - 1][y + 1][z + 1]
	cornersAbove[2] = surroundings[x + 1][y + 1][z + 1]
	cornersAbove[3] = surroundings[x + 1][y + 1][z - 1]
	local adjacentCornersAbove = 0
	for n = 0, 3 do
		if cornersAbove[n] > 0 then
			adjacentCornersAbove = adjacentCornersAbove + 1
		end
	end
	--determine what type of wedge to make
	local material = nil
	local wedge = nil
	local rotation = nil
	if defaultmaterial then
		material = defaultmaterial
	else
		material, wedge, rotation = c:GetCell(x, y, z) --start with the existing material, wedge, and rotation
	end
	wedge = 0 --default wedge is a block
	rotation = 0 --default rotation is 0
	--type 1: 45 degree ramp //must not have a block on top and must have a block under, and be surrounded by 1 side; or 3 sides and the 2 corners between them
	if surroundings[x][y + 1][z] == 0 and surroundings[x][y - 1][z] > 0 then
		if adjacentSides == 1 then
			for n = 0, 3 do
				if sides[n] > 0 then
					wedge = 1
					rotation = (n + 1) % 4
					c:SetCell(x, y, z, material, wedge, rotation)
					return true
				end
			end
		elseif  adjacentSides == 3 then
			for n = 0, 3 do
				if sides[n] > 0 and corners[(n + 1) % 4] > 0 and sides[(n + 1) % 4] > 0 and corners[(n + 2) % 4] > 0 and sides[(n + 2) % 4] > 0 then
					wedge = 1
					rotation = (n + 2) % 4
					c:SetCell(x, y, z, material, wedge, rotation)
					return true
				end
			end
		end
	end
	--type 2: 45 degree corner //must not have a block on top and must have a block under, and be surrounded by 2 sides and the 1 corner between them; or 3 sides and 1 corner between 2 of them (facing towards that corner)
	if surroundings[x][y + 1][z] == 0 and surroundings[x][y - 1][z] > 0 then
		for n = 0, 3 do
			if sides[n] > 0 and corners[(n + 1) % 4] > 0 and sides[(n + 1) % 4] > 0 and (adjacentSides == 2 or (adjacentSides == 3 and (corners[(n + 3) % 4] > 0 or (sides[(n + 2) % 4] > 0 and corners[(n + 2) % 4] > 0) or (sides[(n + 3) % 4] > 0 and corners[n] > 0)))) then
				wedge = 2
				rotation = (n + 2) % 4
				c:SetCell(x, y, z, material, wedge, rotation)
				return true
			end
		end
	end
	--type 3: 45 degree inverse corner //surrounded by three sides or 4 sides and 3 corners, with nothing above or else a block on top surrounded on 2 sides and the corner between them
	if adjacentSides == 3 and surroundings[x][y + 1][z] > 0 then
		if adjacentCorners > 1 then
			for n = 0, 3 do
				if (corners[n] == 0 or cornersAbove[n] == 0) and (sides[(n - 1) % 4] == 0 or sides[n] == 0) and (sidesAbove[n] == 0 and sidesAbove[(n + 1) % 4] > 0 and sidesAbove[(n + 2) % 4] > 0 and sidesAbove[(n + 3) % 4] == 0) then
					wedge = 3
					rotation = (n + 3) % 4
					c:SetCell(x, y, z, material, wedge, rotation)
					return true
				end
			end
		end
	elseif adjacentSides == 4 and adjacentCorners == 3 then
		for n = 0, 3 do
			if corners[n] == 0 and (surroundings[x][y + 1][z] == 0 or (sidesAbove[n] == 0 and sidesAbove[(n + 1) % 4] > 0 and cornersAbove[(n + 2) % 4] > 0 and sidesAbove[(n + 2) % 4] > 0 and sidesAbove[(n + 3) % 4] == 0)) then
				wedge = 3
				rotation = (n + 3) % 4
				c:SetCell(x, y, z, material, wedge, rotation)
				return true
			end
		end
	end
	--type 4: half a cube, as if it were cut diagonally from front to back //surrounded by 2 sides
	if adjacentSides == 2 and adjacentCorners < 4 then
		for n = 0, 3 do
			if sides[n] == 0 and sides[(n + 1) % 4] == 0 and (surroundings[x][y + 1][z] == 0 or (sidesAbove[n] == 0 and sidesAbove[(n + 1) % 4] == 0 and sidesAbove[(n + 2) % 4] > 0 and sidesAbove[(n + 3) % 4] > 0)) then
				wedge = 4
				rotation = n
				c:SetCell(x, y, z, material, wedge, rotation)
				return true
			end
		end
	end
	c:SetCell(x, y, z, material, wedge, rotation)
	return false
end

--makes an orb at point (x, y, z) in cluster c
--d is the depth factor, a percent of the depth of a perfect sphere
function makeBall(x, y, z, r, d)
	c = game.Workspace.Terrain
	local heightmap = {}
	for i = x - (r + 1), x + (r + 1) do
		heightmap[i] = {}
	end

	for j = 0, r + 1 do
		local cellschanged = false
		for i = x - (r + 1), x + (r + 1) do
			for k = z - (r + 1), z + (r + 1) do
				distance = math.sqrt(math.pow(dist(x, z, i, k), 2) + math.pow(j*(100/d), 2))
				if distance < r then
					c:SetCell(i, y - j, k, 1, 0, 1)
					c:SetCell(i, y + j, k, 1, 0, 1)
					cellschanged = true
					heightmap[i][k] = y + j
				elseif heightmap[i][k] == nil then
					heightmap[i][k] = getHeight(i, k)
				end
			end
		end
		if cellschanged == false then
			break
		end
		wait(0)
	end

	for ri = 0, r do
		wait(0)

		i = x - ri
		for k = z - r, z + r do
			height = heightmap[i][k]
			if height == nil then
				height = -1
			end
			for h = height, 0, -1 do
				if not MakeWedge(i, h, k, heightmap) then
					break
				end
			end
		end

		i = x + ri
		for k = z - r, z + r do
			height = heightmap[i][k]
			if height == nil then
				height = -1
			end
			for h = height, 0, -1 do
				if not MakeWedge(i, h, k, heightmap) then
					break
				end
			end
		end

	end

end


function dist(x1, y1, x2, y2)
	return math.sqrt(math.pow(x2-x1, 2) + math.pow(y2-y1, 2))
end

local debounce = false
function onClicked(mouse)
	if on and not debounce then
		debounce = true
		c = game.Workspace.Terrain

		local cellPos = c:WorldToCellPreferEmpty(Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		local x = cellPos.x
		local y = cellPos.y
		local z = cellPos.z

		makeBall(x, y, z, r, d)
		print("Orb created at: "..x..", "..y..", "..z)
		debounce = false
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	frame.Visible = true
	for w = 0, 0.3, 0.06 do
		frame.Size = UDim2.new(w, 0, w/2, 0)
		wait(0)
	end
	radl.Text = "Radius: "..r
	dfl.Text = "Height: "..d.."%"
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	radl.Text = ""
	dfl.Text = ""
	for w = 0.3, 0, -0.06 do
		frame.Size = UDim2.new(w, 0, w/2, 0)
		wait(0)
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
frame.Position = UDim2.new(0.35, 0, 0.8, 0)
frame.Size = UDim2.new(0.3, 0, 0.15, 0)
frame.BackgroundTransparency = 0.5
frame.Visible = false

--current radius display label
radl = Instance.new("TextLabel", frame)
radl.Position = UDim2.new(0.05, 0, 0.1, 0)
radl.Size = UDim2.new(0.2, 0, 0.35, 0)
radl.Text = ""
radl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
radl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
radl.Font = Enum.Font.ArialBold
radl.FontSize = Enum.FontSize.Size14
radl.BorderColor3 = Color3.new(0, 0, 0)
radl.BackgroundTransparency = 1

--radius slider
radSliderGui, radSliderPosition = RbxGui.CreateSlider(128, 0, UDim2.new(0.3, 0, 0.26, 0))
radSliderGui.Parent = frame
radBar = radSliderGui:FindFirstChild("Bar")
radBar.Size = UDim2.new(0.65, 0, 0, 5)
radSliderPosition.Value = r
radSliderPosition.Changed:connect(function()
	r = radSliderPosition.Value
	radl.Text = "Radius: "..r
end)

--current depth factor display label
dfl = Instance.new("TextLabel", frame)
dfl.Position = UDim2.new(0.05, 0, 0.55, 0)
dfl.Size = UDim2.new(0.2, 0, 0.35, 0)
dfl.Text = ""
dfl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
dfl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
dfl.Font = Enum.Font.ArialBold
dfl.FontSize = Enum.FontSize.Size14
dfl.BorderColor3 = Color3.new(0, 0, 0)
dfl.BackgroundTransparency = 1

--depth factor slider
dfSliderGui, dfSliderPosition = RbxGui.CreateSlider(100, 0, UDim2.new(0.3, 0, 0.71, 0))
dfSliderGui.Parent = frame
dfBar = dfSliderGui:FindFirstChild("Bar")
dfBar.Size = UDim2.new(0.65, 0, 0, 5)
dfSliderPosition.Value = d
dfSliderPosition.Changed:connect(function()
	d = dfSliderPosition.Value
	dfl.Text = "Height: "..d.."%"
end)




--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Orbs Plugin Loaded")
