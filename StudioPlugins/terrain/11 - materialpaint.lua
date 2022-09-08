-----------------------------
--LOCAL FUNCTION DEFINITIONS-
-----------------------------
local c = game.Workspace.Terrain
local SetCell = c.SetCell
local GetCell = c.GetCell
local WorldToCellPreferSolid = c.WorldToCellPreferSolid

-----------------
--DEFAULT VALUES-
-----------------
loaded = false
on = false
local materialToImageMap = {}
local materialNames = {"Grass","Sand", "Brick", "Granite"}
local currentMaterial = 1
local changedDragging = 0

local debounce = false
local dragging = false
local r = 5

for i,v in pairs(materialNames) do
	materialToImageMap[v] = {}
	if v == "Grass" then
		materialToImageMap[v].Regular = "http://www.roblox.com/asset/?id=56563112"
	elseif v == "Sand" then
		materialToImageMap[v].Regular = "http://www.roblox.com/asset/?id=62356652"
	elseif v == "Brick" then
		materialToImageMap[v].Regular = "http://www.roblox.com/asset/?id=65961537"
		materialToImageMap[v].Over = "http://www.roblox.com/asset/?id=65961537"
	elseif v == "Granite" then
		materialToImageMap[v].Regular = "http://www.roblox.com/asset/?id=65961588"
		materialToImageMap[v].Over = "http://www.roblox.com/asset/?id=65961588"
	end
end

for i,v in pairs(materialNames) do
	 game:GetService("ContentProvider"):Preload(materialToImageMap[v].Regular)
end

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
toolbarbutton = toolbar:CreateButton("", "Material Brush", "materialBrush.png")
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

function paint(mouse)
	if mouse and mouse.Hit and c and not debounce then
		debounce = true
		local count = 0
		local cellPos = WorldToCellPreferSolid(c, mouse.Hit.p)
		for x = cellPos.x - r, cellPos.x + r do
			for y = cellPos.y - r, cellPos.y + r do
				for z = cellPos.z - r, cellPos.z + r do
					local tempCellPos = Vector3.new(x, y, z)
					local distSq = (tempCellPos - cellPos):Dot(tempCellPos - cellPos)
					if (distSq < r*r) then
						local oldMaterial, oldType, oldOrientation = GetCell(c, x, y, z)
						if oldMaterial.Value > 0 then SetCell(c, x, y, z, currentMaterial, oldType, oldOrientation) end
					end
					count = count + 1
				end
			end
			if count > 10000 then count = 0 wait() end
		end
		debounce = false
	end
end

function mouseDown(mouse)
	changedDragging = tick()
	if on and mouse.Target == game.Workspace.Terrain then
		dragging = true
		tweeningFrame = true
		frame:TweenPosition(UDim2.new(0,-245,0,0),Enum.EasingDirection.Out,Enum.EasingStyle.Quad,0.5,true,function()
			hintFrame.Visible = true
			tweeningFrame = false
		end)
		paint(mouse)
	end
end



function mouseUp(mouse)
	dragging = false
	changedDragging = tick()
	delay(5,function()
		local timeDiff = (changedDragging + 5) - tick()
		if timeDiff < 0.2 then
			tweeningFrame = true
			hintFrame.Visible = false
			frame:TweenPosition(UDim2.new(0,0,0,0),Enum.EasingDirection.Out,Enum.EasingStyle.Quad,0.5,true,function()
				tweeningFrame = false
			end)
		end
	end)
		
end


function mouseMove(mouse)
	if on and dragging then
		paint(mouse)
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	
	local gui = game:GetService("CoreGui"):FindFirstChild("MaterialPainterGui",true)

	frame.Position = UDim2.new(0,0,0,0)
	frame.Visible = true
	hintFrame.Visible = false
	halfFrame.Parent = gui
	title.Text = "Material Painter"
	radl.Text = "Radius: "..r
	
	on = true
end

function Off()
	toolbarbutton:SetActive(false)

	title.Text = ""
	radl.Text = ""
	frame.Visible = false
	hintFrame.Visible = false
	halfFrame.Parent = nil
	on = false
end


function updateMaterialChoice(choice)
	if choice == "Grass" then
		currentMaterial = 1
	elseif choice == "Sand" then
		currentMaterial = 2
	elseif choice == "Erase" then
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

--load library for with sliders
local RbxGui = LoadLibrary("RbxGui")
local tweeningFrame = false

--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))
g.Name = "MaterialPainterGui"

local selectedButton = nil

--frame
frame = Instance.new("Frame", g)
frame.Size = UDim2.new(0, 245, 0, 230)
frame.Position = UDim2.new(0, 0, 0, 0)
frame.BackgroundTransparency = 0.5
frame.BorderSizePixel = 0
frame.BackgroundColor3 = Color3.new(0,0,0)
frame.Visible = false
frame.Active = true

hintFrame = Instance.new("Frame",g)
hintFrame.BackgroundTransparency = 0.5
hintFrame.BackgroundColor3 = Color3.new(0,0,0)
hintFrame.Size = UDim2.new(0,15,0,230)
hintFrame.Visible = false

halfFrame = Instance.new("Frame",g)
halfFrame.Name = "HalfFrame"
halfFrame.Size = UDim2.new(0,123,0,230)
halfFrame.BackgroundTransparency = 1
halfFrame.MouseEnter:connect(function()
	if not dragging and frame.Position ~= UDim2.new(0, 0, 0, 0) and not tweeningFrame then
		tweeningFrame = true
		hintFrame.Visible = false
		frame:TweenPosition(UDim2.new(0,0,0,0),Enum.EasingDirection.Out,Enum.EasingStyle.Quad,0.2,true,function() tweeningFrame = false end)
	end
end)

--title
title = Instance.new("TextLabel", frame)
title.Position = UDim2.new(0, 0, 0, 3)
title.Size = UDim2.new(1, 0, 0, 24)
title.Text = ""
title.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
title.TextColor3 = Color3.new(1, 1, 1)
title.Font = Enum.Font.ArialBold
title.FontSize = Enum.FontSize.Size18
title.BorderColor3 = Color3.new(0, 0, 0)
title.BackgroundTransparency = 1

--current radius display label
radl = Instance.new("TextLabel", frame)
radl.Position = UDim2.new(0, 10, 1, -20)
radl.Size = UDim2.new(0.2, 0, 0, 14)
radl.Text = ""
radl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
radl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
radl.Font = Enum.Font.Arial
radl.FontSize = Enum.FontSize.Size14
radl.BorderColor3 = Color3.new(0, 0, 0)
radl.BackgroundTransparency = 1

--radius slider
radSliderGui, radSliderPosition = RbxGui.CreateSlider(10, 0, UDim2.new(0, 75, 1, -15))
radSliderGui.Parent = frame
radBar = radSliderGui:FindFirstChild("Bar")
radBar.Size = UDim2.new(0.65, 0, 0, 5)
radSliderPosition.Value = r - 1
radSliderPosition.Changed:connect(function()
	r = radSliderPosition.Value
	radl.Text = "Radius: "..r
end)

local scrollFrame, scrollUp, scrollDown, recalculateScroll = RbxGui.CreateScrollingFrame(nil,"grid")
scrollFrame.Size = UDim2.new(0,240,0,170)
scrollFrame.Position = UDim2.new(0.5,-scrollFrame.Size.X.Offset/2,0,30)
scrollFrame.Parent = frame

scrollUp.Parent = frame
scrollUp.Visible = false
scrollUp.Position = UDim2.new(1,-19,0,30)

scrollDown.Parent = frame
scrollDown.Visible = false
scrollDown.Position = UDim2.new(1,-19,0,30 - 17 + scrollFrame.Size.Y.Offset)

scrollFrame.ChildRemoved:connect(function()
	if #scrollFrame:GetChildren() < 35 then
		scrollUp.Visible = false
		scrollDown.Visible = false
	end
end)

scrollFrame.ChildAdded:connect(function()
	if #scrollFrame:GetChildren() >= 35 then
		scrollUp.Visible = true
		scrollDown.Visible = true
	end
end)

function createMaterialButton(name)	
	local buttonWrap = Instance.new("TextButton")
	buttonWrap.Text = ""
	buttonWrap.Size = UDim2.new(0,32,0,32)
	buttonWrap.BackgroundColor3 = Color3.new(1,1,1)
	buttonWrap.BorderSizePixel = 0
	buttonWrap.BackgroundTransparency = 1
	buttonWrap.AutoButtonColor = false
	buttonWrap.Name = tostring(name) .. "Button"
	
	local imageButton = Instance.new("ImageButton")
	imageButton.AutoButtonColor = false
	imageButton.BackgroundTransparency = 1
	imageButton.Size = UDim2.new(0,30,0,30)
	imageButton.Position = UDim2.new(0,1,0,1)
	imageButton.Name = tostring(name) .. "Button"
	imageButton.Parent = buttonWrap
	imageButton.Image = materialToImageMap[name].Regular
	
	imageButton.MouseEnter:connect(function()
		buttonWrap.BackgroundTransparency = 0
	end)
	imageButton.MouseLeave:connect(function()
		if selectedButton ~= buttonWrap then
			buttonWrap.BackgroundTransparency = 1
		end
	end)
	imageButton.MouseButton1Click:connect(function() 
		updateMaterialChoice(name)
		if selectedButton ~= buttonWrap then
			buttonWrap.BackgroundTransparency = 0
			selectedButton.BackgroundTransparency = 1
			selectedButton = buttonWrap
		end
	end)
	
	return buttonWrap
end

for i = 1, #materialNames do
	local imageButton = createMaterialButton(materialNames[i])
	
	if materialNames[i] == "Grass" then
		selectedButton = imageButton
		imageButton.BackgroundTransparency = 0
	end
	
	imageButton.Parent = scrollFrame
end

recalculateScroll()


--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true