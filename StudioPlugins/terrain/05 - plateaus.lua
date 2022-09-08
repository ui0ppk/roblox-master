-----------------
--DEFAULT VALUES-
-----------------
loaded = false
on = false




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
toolbarbutton = toolbar:CreateButton("", "Plateau", "plateaus.png")
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

--makes a plateau starting at point (x, y, z)
function makePlateau(x, y, z)
	c = game.Workspace.Terrain
	q = {}
	index = 0
	q[index] = x
	q[index+1] = y
	q[index+2] = z
	insertindex = 3
	c:SetCell(q[index], q[index+1], q[index+2], 0, 0, 0)
	while q[index] do
		
		insertindex = plateauHelper(insertindex, 0, 1, 0)
		insertindex = plateauHelper(insertindex, -1, 0, -1)
		insertindex = plateauHelper(insertindex, -1, 0, 0)
		insertindex = plateauHelper(insertindex, -1, 0, 1)
		insertindex = plateauHelper(insertindex, 0, 0, -1)
		insertindex = plateauHelper(insertindex, 0, 0, 1)
		insertindex = plateauHelper(insertindex, 1, 0, -1)
		insertindex = plateauHelper(insertindex, 1, 0, 0)
		insertindex = plateauHelper(insertindex, 1, 0, 1)

		q[index] = nil
		q[index + 1] = nil
		q[index + 2] = nil
		index = index + 3
		if index % 1000 == 0 then
			wait()
		end
	end
end

function plateauHelper(insertindex, xoffset, yoffset, zoffset)
	material, wedge, rotation = c:GetCell(q[index] + xoffset, q[index+1] + yoffset, q[index+2] + zoffset)
	if material.Value > 0 then
		q[insertindex] = q[index] + xoffset
		q[insertindex+1] = q[index+1] + yoffset
		q[insertindex+2] = q[index+2] + zoffset
		c:SetCell(q[index] + xoffset, q[index+1] + yoffset, q[index+2] + zoffset, 0, 0, 0)
		insertindex = insertindex + 3
	end
	return insertindex
end


function dist(x1, y1, x2, y2)
	return math.sqrt(math.pow(x2-x1, 2) + math.pow(y2-y1, 2))
end

function dist3d(x1, y1, z1, x2, y2, z2)
	return math.sqrt(math.pow(dist(x1, y1, x2, y2), 2) + math.pow(z2-z1, 2))
end

function onClicked(mouse)
	if on then
	
		c = game.Workspace.Terrain
		
		local cellPos = c:WorldToCellPreferSolid(Vector3.new(mouse.Hit.x, mouse.Hit.y, mouse.Hit.z))
		local x = cellPos.x
		local y = cellPos.y
		local z = cellPos.z
		
		print("Plateau formed at: "..x..", "..y..", "..z)
		makePlateau(x, y, z)	
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
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
print("Plateaus Plugin Loaded")