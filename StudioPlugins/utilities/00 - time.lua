-----------------
--DEFAULT VALUES-
-----------------
loaded = false
on = false
t = 720




---------------
--PLUGIN SETUP-
---------------

--make internal representation of plugin (needed to call functions like CreateToolbar() and Activate())
self = PluginManager():CreatePlugin()

--Deactivation event is sent when another plugin calls Activate(), turn off when Deactivation event is received
self.Deactivation:connect(function()
	Off()
end)

--make a new toolbar, "Utilities" is the name of it, if another plugin also uses a toolbar named "Utilities" they will both create buttons on the same one
toolbar = self:CreateToolbar("Utilities")

--make a button on the toolbar, the first parameter is the text on the button, second is the tooltip, and last is the icon
toolbarbutton = toolbar:CreateButton("", "Time of Day", "time.png")

--make button turn plugin on and off
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
--converts time in minutes into a string of the format "hours:minutes:seconds" (seconds are always 00)
function timestring(t)
	local tsTemp = "%0.2d:%0.2d:00"
	return tsTemp:format(math.floor(t/60), math.floor(t%60))
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	frame.Visible = true
	for w = 0, 0.8, 0.16 do
		frame.Size = UDim2.new(w, 0, w/9, 0)
		wait(0.0000001)
	end
	title.Text = "Time of Day: "..timestring(t)
	on = true
end

function Off()
	toolbarbutton:SetActive(false)
	title.Text = ""
	for w = 0.8, 0, -0.16 do
		frame.Size = UDim2.new(w, 0, w/9, 0)
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
frame.Position = UDim2.new(0.1, 0, 0.85, 0)
frame.Size = UDim2.new(0.8, 0, 0.1, 0)
frame.BackgroundTransparency = 0.7
frame.Visible = false

--title
title = Instance.new("TextLabel", frame)
title.Position = UDim2.new(0.4, 0, 0.25, 0)
title.Size = UDim2.new(0.2, 0, 0.05, 0)
title.Text = ""
title.BackgroundColor3 = Color3.new(0.4, 0.4, 0.4)
title.TextColor3 = Color3.new(1, 1, 1)
title.Font = Enum.Font.ArialBold
title.FontSize = Enum.FontSize.Size24
title.BorderColor3 = Color3.new(0, 0, 0)
title.BackgroundTransparency = 1

--slider
local RbxGui = LoadLibrary("RbxGui")
sliderGui, sliderPosition = RbxGui.CreateSlider(1440, frame.AbsoluteSize.x * 0.95, UDim2.new(0.025, 0, 0.7, 0))
sliderGui.Parent = frame
bar = sliderGui:FindFirstChild("Bar")
bar.Size = UDim2.new(0.95, 0, 0, 5)
sliderPosition.Value = t
sliderPosition.Changed:connect(function()
	t = sliderPosition.Value
	ts = timestring(t)
	title.Text = "Time of Day: "..ts
	title.Text = "Time of Day: "..ts
	game:GetService("Lighting").TimeOfDay = ts
end)






--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("Time of Day Plugin Loaded")
