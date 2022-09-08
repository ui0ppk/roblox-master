-----------------
--DEFAULT VALUES-
-----------------
on = false
loaded = false


---------------
--PLUGIN SETUP-
---------------
self = PluginManager():CreatePlugin()
mouse = self:GetMouse()
mouse.Button1Down:connect(function() eventl.Text = "Button1Down" end)
mouse.Button1Up:connect(function() eventl.Text = "Button1Up" end)
mouse.Button2Down:connect(function() eventl.Text = "Button2Down" end)
mouse.Button2Up:connect(function() eventl.Text = "Button2Up" end)
mouse.WheelForward:connect(function() eventl.Text = "WheelForward" end)
mouse.WheelBackward:connect(function() eventl.Text = "WheelBackward" end)
mouse.KeyDown:connect(function(key) eventl.Text = "KeyDown("..key..")" end)
mouse.KeyUp:connect(function(key) eventl.Text = "KeyUp("..key..")" end)
mouse.Move:connect(function()
	hitl.Text = mouse.Hit.p.x..", "..mouse.Hit.p.y..", "..mouse.Hit.p.z
	terrain = game.Workspace.Terrain
	if terrain then
		cellPos = terrain:WorldToCellPreferSolid(mouse.Hit.p)
		if cellPos then
			terrainl.Text = cellPos.x..", "..cellPos.y..", "..cellPos.z
		else
			terrainl.Text = ""
		end
	end
	screenl.Text = mouse.X..", "..mouse.Y
end)
self.Deactivation:connect(function()
	Off()
end)
toolbar = self:CreateToolbar("Utilities")
toolbarbutton = toolbar:CreateButton("", "Input Info: Check coordinates of where you click and see input events triggered.", "inputinfo.png")
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
function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	frame.Visible = true
	eventt.Text = ""
	eventl.Text = ""
	hitt.Text = ""
	hitl.Text = ""
	terraint.Text = ""
	terrainl.Text = ""
	screent.Text = ""
	screenl.Text = ""
	for w = 0, 0.6, 0.12 do
		frame.Size = UDim2.new(w, 0, w/4, 0)
		wait(0.0000001)
	end
	eventt.Text = "Last Input Event: "
	hitt.Text = "World Coordinates: "
	terraint.Text = "Terrain Coordinates: "
	screent.Text = "Screen Coordinates: "
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	eventt.Text = ""
	eventl.Text = ""
	hitt.Text = ""
	hitl.Text = ""
	terraint.Text = ""
	terrainl.Text = ""
	screent.Text = ""
	screenl.Text = ""
	for w = 0.6, 0, -0.12 do
		frame.Size = UDim2.new(w, 0, w/4, 0)
		wait(0.0000001)
	end
	frame.Visible = false
	on = false
end




------
--GUI-
------
--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))

--frame
frame = Instance.new("Frame", g)
frame.Position = UDim2.new(0.2, 0, 0.8, 0)
frame.Size = UDim2.new(0.6, 0, 0.2, 0)
frame.BackgroundTransparency = 0.5
frame.Visible = false

--last event title
eventt = Instance.new("TextLabel", frame)
eventt.Position = UDim2.new(0.1, 0, 0.125, 0)
eventt.Size = UDim2.new(0.2, 0, 0.15, 0)
eventt.Text = ""
eventt.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
eventt.TextColor3 = Color3.new(0.95, 0.95, 0.95)
eventt.Font = Enum.Font.ArialBold
eventt.FontSize = Enum.FontSize.Size14
eventt.BorderColor3 = Color3.new(0, 0, 0)
eventt.BackgroundTransparency = 1

--last event display label
eventl = Instance.new("TextLabel", frame)
eventl.Position = UDim2.new(0.4, 0, 0.125, 0)
eventl.Size = UDim2.new(0.5, 0, 0.15, 0)
eventl.Text = ""
eventl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
eventl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
eventl.Font = Enum.Font.ArialBold
eventl.FontSize = Enum.FontSize.Size14
eventl.BorderColor3 = Color3.new(0, 0, 0)
eventl.BackgroundTransparency = 1

--current world coordinates title
hitt = Instance.new("TextLabel", frame)
hitt.Position = UDim2.new(0.1, 0, 0.325, 0)
hitt.Size = UDim2.new(0.2, 0, 0.15, 0)
hitt.Text = ""
hitt.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
hitt.TextColor3 = Color3.new(0.95, 0.95, 0.95)
hitt.Font = Enum.Font.ArialBold
hitt.FontSize = Enum.FontSize.Size14
hitt.BorderColor3 = Color3.new(0, 0, 0)
hitt.BackgroundTransparency = 1

--current world coordinates display label
hitl = Instance.new("TextLabel", frame)
hitl.Position = UDim2.new(0.4, 0, 0.325, 0)
hitl.Size = UDim2.new(0.5, 0, 0.15, 0)
hitl.Text = ""
hitl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
hitl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
hitl.Font = Enum.Font.ArialBold
hitl.FontSize = Enum.FontSize.Size14
hitl.BorderColor3 = Color3.new(0, 0, 0)
hitl.BackgroundTransparency = 1

--current terrain coordinates title
terraint = Instance.new("TextLabel", frame)
terraint.Position = UDim2.new(0.1, 0, 0.525, 0)
terraint.Size = UDim2.new(0.2, 0, 0.15, 0)
terraint.Text = ""
terraint.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
terraint.TextColor3 = Color3.new(0.95, 0.95, 0.95)
terraint.Font = Enum.Font.ArialBold
terraint.FontSize = Enum.FontSize.Size14
terraint.BorderColor3 = Color3.new(0, 0, 0)
terraint.BackgroundTransparency = 1

--current terrain coordinates display label
terrainl = Instance.new("TextLabel", frame)
terrainl.Position = UDim2.new(0.4, 0, 0.525, 0)
terrainl.Size = UDim2.new(0.5, 0, 0.15, 0)
terrainl.Text = ""
terrainl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
terrainl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
terrainl.Font = Enum.Font.ArialBold
terrainl.FontSize = Enum.FontSize.Size14
terrainl.BorderColor3 = Color3.new(0, 0, 0)
terrainl.BackgroundTransparency = 1

--current screen coordinates title
screent = Instance.new("TextLabel", frame)
screent.Position = UDim2.new(0.1, 0, 0.725, 0)
screent.Size = UDim2.new(0.2, 0, 0.15, 0)
screent.Text = ""
screent.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
screent.TextColor3 = Color3.new(0.95, 0.95, 0.95)
screent.Font = Enum.Font.ArialBold
screent.FontSize = Enum.FontSize.Size14
screent.BorderColor3 = Color3.new(0, 0, 0)
screent.BackgroundTransparency = 1

--current screen coordinates display label
screenl = Instance.new("TextLabel", frame)
screenl.Position = UDim2.new(0.4, 0, 0.725, 0)
screenl.Size = UDim2.new(0.5, 0, 0.15, 0)
screenl.Text = ""
screenl.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
screenl.TextColor3 = Color3.new(0.95, 0.95, 0.95)
screenl.Font = Enum.Font.ArialBold
screenl.FontSize = Enum.FontSize.Size14
screenl.BorderColor3 = Color3.new(0, 0, 0)
screenl.BackgroundTransparency = 1


--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Input Info Plugin Loaded")
