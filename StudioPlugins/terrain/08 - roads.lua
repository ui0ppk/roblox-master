-----------------
--DEFAULT VALUES-
-----------------
loaded = false
x1 = 200
y1 = 200
x2 = 300
y2 = 300
h = 20
on = false
mode = 0


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
toolbarbutton = toolbar:CreateButton("", "Roads: Click once to set the starting point and again to set the endpoint of the road.", "roads.png")
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

--makes a column of blocks from 0 up to height at location (x,z) in cluster c
function coordHeight(x, z, height)
	c = game.Workspace.Terrain
	for h = 1, height do
		c:SetCell(x, h, z, 1, 0, 0)
	end
end

function coordCheck(x, z, height)
	c = game.Workspace.Terrain
	for h = height, 0, -1 do
		material, wedge, rotation = c:GetCell(x, h, z)
		if material.Value > 0 then
			return true
		elseif height == 0 then
			return false
		end
	end
end

function dist(x1, y1, x2, y2)
	return math.sqrt(math.pow(x2-x1, 2) + math.pow(y2-y1, 2))
end

function dist3d(x1, y1, z1, x2, y2, z2)
	return math.sqrt(math.pow(dist(x1, y1, x2, y2), 2) + math.pow(z2-z1, 2))
end

--create a path between coordinates (x1,z1) and (x2,z2) at height h in cluster c
--a path is a road with height of 3 instead of 1, and it builds a bridge if there is no existing land under it
--if you want path to come from x direction, make it start at the place
--if you want it to come from z direction, make it end at the place
--if p is true, turns on pillars, otherwise pillars are off
function makePath(x1, z1, x2, z2, h, p)
	c = game.Workspace.Terrain
	if x2 < x1 then
		incx = -1
		n = 1
	else 
		incx = 1
		n = -1
	end
	for x = x1, x2+n, incx do
		c:SetCell(x, h+1, z1, 0, 0, 0)
		c:SetCell(x, h+1, z1-1, 0, 0, 0)
		c:SetCell(x, h+1, z1+1, 0, 0, 0)
		c:SetCell(x, h+2, z1, 0, 0, 0)
		c:SetCell(x, h+2, z1-1, 0, 0, 0)
		c:SetCell(x, h+2, z1+1, 0, 0, 0)
		c:SetCell(x, h+3, z1, 0, 0, 0)
		c:SetCell(x, h+3, z1-1, 0, 0, 0)
		c:SetCell(x, h+3, z1+1, 0, 0, 0)
		c:SetCell(x, h, z1, 1, 0, 0)
		c:SetCell(x, h, z1-1, 1, 0, 0)
		c:SetCell(x, h, z1+1, 1, 0, 0)
	end
	if p then
		for x = x1, x2+n, 16*incx do
			if coordCheck(x, z1, h-1) then
				coordHeight(x, z1, h-1)
			end
			if coordCheck(x, z1-1, h-1) then
				coordHeight(x, z1-1, h-1)
			end
			if coordCheck(x, z1+1, h-1) then
				coordHeight(x, z1+1, h-1)
			end
		end
	end
	if z2 < z1 then
		incz = -1
		m = 1
		n = 2
	else
		incz = 1
		m = -1
		n = -2
	end
	for z = z1+m, z2+n, incz do
		c:SetCell(x2, h+1, z, 0, 0, 0)
		c:SetCell(x2-1, h+1, z, 0, 0, 0)
		c:SetCell(x2+1, h+1, z, 0, 0, 0)
		c:SetCell(x2, h+2, z, 0, 0, 0)
		c:SetCell(x2-1, h+2, z, 0, 0, 0)
		c:SetCell(x2+1, h+2, z, 0, 0, 0)
		c:SetCell(x2, h+3, z, 0, 0, 0)
		c:SetCell(x2-1, h+3, z, 0, 0, 0)
		c:SetCell(x2+1, h+3, z, 0, 0, 0)
		c:SetCell(x2, h, z, 1, 0, 0)
		c:SetCell(x2-1, h, z, 1, 0, 0)
		c:SetCell(x2+1, h, z, 1, 0, 0)
	end
	if p then
		for z = z1+m, z2+n, 16*incz do
			if coordCheck(x2, z, h-1) then
				coordHeight(x2, z, h-1)
			end
			if coordCheck(x2-1, z, h-1) then
				coordHeight(x2-1, z, h-1)
			end
			if coordCheck(x2+1, z, h-1) then
				coordHeight(x2+1, z, h-1)
			end
		end
	end
end

function onClicked (mouse)
	if on then
	
		c = game.Workspace.Terrain
		
		local cellPos = c:WorldToCellPreferSolid(Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		local x = cellPos.x
		local y = cellPos.y
		local z = cellPos.z
		
		if mode == 0 then
			x1 = x
			y1 = z
			h = y
			mode = 1
		elseif mode == 1 then
			x2 = x
			y2 = z
			makePath(x1, y1, x2, y2, h, true, c)
			mode = 0
		else
		end
		
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	mode = 0
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	on = false
end




------
--GUI-
------

--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))




--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Roads Plugin Loaded")
