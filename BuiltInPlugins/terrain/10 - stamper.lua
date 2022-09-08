while game == nil do
	wait(1/30)
end

---------------
--PLUGIN SETUP-
---------------
local on = false
local loaded = false

self	= PluginManager():CreatePlugin()

toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("Stamper", "Part Stamper - Toggle List (Shift + F)", "stamp.png")
toolbarbutton.Click:connect(function()
	if on then
		Off()
	elseif loaded then
		On()
	end
end)

game:WaitForChild("Workspace")
game.Workspace:WaitForChild("Terrain")

-----------------
--DEFAULT VALUES-
-----------------
local partButtonOn = true

local currStampGui = nil
local currStampId = nil
local recentsFrame = nil
local waterTypeChangedEvent = nil
local waterForceAndDirection = {"None","NegX"}

local setPanelVisibility = nil
local getPanelVisibility = nil

local stampControl = nil
local lastStampModel = nil

local keyCon = nil

-- ids of users we want to load sets in from
local userSetIds = {11744447,18881789,18881808,18881829,18881853,18881866}
local recentButtonStack = {}

-- mouse management
local mouse = nil

-- Libraries
local RbxStamper = nil
local RbxGui	 = nil

Spawn(function()
	RbxGui = LoadLibrary("RbxGui")
	RbxStamper = LoadLibrary("RbxStamper")
end)

local BaseUrl = game:GetService("ContentProvider").BaseUrl:lower()

-----------------------
--FUNCTION DEFINITIONS-
-----------------------

function getRbxGui()
	if not RbxGui then
		print("teh new rbxgui")
		RbxGui = LoadLibrary("RbxGui")
	end
	return RbxGui
end

function getRbxStamper()
	if not RbxStamper then
		print("teh new stamper")
		RbxStamper = LoadLibrary("RbxStamper")
	end
	return RbxStamper
end

function showLoadingDialog()
	currStampGui.LoadingFrame.LoadingText:TweenPosition(UDim2.new(0,150,0,0), Enum.EasingDirection.Out, Enum.EasingStyle.Quad, 2, true)
	currStampGui.LoadingFrame.Visible = true
end

function hideLoadingDialog()
	currStampGui.LoadingFrame.LoadingText:TweenPosition(UDim2.new(0,0,0,0), Enum.EasingDirection.Out, Enum.EasingStyle.Quad, 0.1, true)
	currStampGui.LoadingFrame.Visible = false
end

local partSelected = function(name, id, terrainShape)
	if not id then
		return
	end
	if not name then
		return
	end
	
	currStampId = id

	if stampControl then
		stampControl.Destroy()
	end
	if stampCon then
		stampCon:disconnect()
		stampCon = nil
	end

	setPanelVisibility(false)
	
	showLoadingDialog()
		lastStampModel = getRbxStamper().GetStampModel(id, terrainShape)
		updateRecentParts(name, id, terrainShape)
	hideLoadingDialog()

	if lastStampModel.Name == "MegaClusterCube" then
		local clusterTag = lastStampModel:FindFirstChild("ClusterMaterial")
		-- we are going to stamp water, send info to stamper about this
		if clusterTag and clusterTag:isA("Vector3Value") and clusterTag.Value.X == 17 then
			local waterForceTag = Instance.new("StringValue",lastStampModel)
			waterForceTag.Name = "WaterForceTag"
			waterForceTag.Value = waterForceAndDirection[1]

			local waterForceDirectionTag = Instance.new("StringValue",lastStampModel)
			waterForceDirectionTag.Name = "WaterForceDirectionTag"
			waterForceDirectionTag.Value = waterForceAndDirection[2]
		end
	end
	
	setupStamper(lastStampModel, mouse)
end

function updateWaterInfo()
	if stampControl then
		stampControl.Destroy()
	end
	if stampCon then
		stampCon:disconnect()
		stampCon = nil
	end

	showLoadingDialog()
		lastStampModel = getRbxStamper().GetStampModel(currStampId)
	hideLoadingDialog()

	if lastStampModel.Name == "MegaClusterCube" then
		local clusterTag = lastStampModel:FindFirstChild("ClusterMaterial")
		-- we are going to stamp water, send info to stamper about this
		if clusterTag and clusterTag.Value.X == 17 then
			local waterForceTag = Instance.new("StringValue",lastStampModel)
			waterForceTag.Name = "WaterForceTag"
			waterForceTag.Value = waterForceAndDirection[1]

			local waterForceDirectionTag = Instance.new("StringValue",lastStampModel)
			waterForceDirectionTag.Name = "WaterForceDirectionTag"
			waterForceDirectionTag.Value = waterForceAndDirection[2]
		end
	end

	setupStamper(lastStampModel, mouse)
end


local dialogClosed = function()
	if lastStampModel then
		if stampControl then
			stampControl.Destroy()
		end
		setupStamper(lastStampModel, mouse)
	end
end

function pickPart()
	if stampControl then
		stampControl.Destroy()
	end
	setPanelVisibility(true)
end

function keyHandler(key)
	if key == 'f' then
		handlePartShow()
	end
end

function partOn()
	pickPart()
	partButtonOn = true
end

function partOff()
	setPanelVisibility(false)
	if lastStampModel then
		if stampControl then
			stampControl.Destroy()
		end
		setupStamper(lastStampModel, mouse)
	end
	partButtonOn = false
end

function handlePartShow()
	if getPanelVisibility() then
		partOff()
	else
		partOn()
	end
end

function On()
	if not game.Workspace.Terrain then
		return
	end
	
	if self then
		self:Activate(true)
		mouse = self:GetMouse()
	end

	if toolbarbutton then
		toolbarbutton:SetActive(true)
	end
	
	if not currStampGui then -- first load, lets make the gui
		createGui()
	end
	
	if setPanelVisibility then
		setPanelVisibility(true)
	end
	
	if recentsFrame then
		recentsFrame.Visible = true
	end
	
	if keyHandler then
		keyCon = mouse.KeyDown:connect(keyHandler)
	end
	
	on = true
end

function Off()

	if toolbarbutton then
		toolbarbutton:SetActive(false)
	end

	if stampControl then
		stampControl.Destroy()
	end
	
	if keyCon then
		keyCon:disconnect()
		keyCon = nil
	end

	if currStampGui and currStampGui:FindFirstChild("WaterFrame") then
		currStampGui.WaterFrame.Visible = false
	end
	
	if lastStampModel then
		lastStampModel:Destroy()
	end
	
	if setPanelVisibility then
		setPanelVisibility(false)
	end
	
	if recentsFrame then
		recentsFrame.Visible = false
	end
	
	on = false
end
------------------------------------------------------------------------------------------
-- Connect to deactivation event once the handler is defined
------------------------------------------------------------------------------------------
self.Deactivation:connect(function()
	Off()
end)

function setupStamper(model, mouse)
	if model then
		stampControl = getRbxStamper().SetupStamperDragger(model,mouse)
		if stampControl then
			stampCon = stampControl.Stamped.Changed:connect(function()
					if stampControl.Stamped.Value then
						stampControl.ReloadModel()
					end
			end)
		end
	end
end

function updateRecentParts(newName, newId, newTerrainShape)
	if newId then		
		for i = 1, #recentButtonStack do
			if recentButtonStack[i].Id == newId then -- already have item, nothing to do
				return
			end
		end
		for i = #recentButtonStack - 1, 1, -1 do
			recentButtonStack[i + 1].Id = recentButtonStack[i].Id
			recentButtonStack[i + 1].Name = recentButtonStack[i].Name
			recentButtonStack[i + 1].TerrainShape = recentButtonStack[i].TerrainShape
			
			recentButtonStack[i + 1].Button.Image = recentButtonStack[i].Button.Image
		end
		
		recentButtonStack[1].Id = newId
		recentButtonStack[1].Name = newName
		recentButtonStack[1].TerrainShape = newTerrainShape
		recentButtonStack[1].Button.Image = BaseUrl .. "Game/Tools/ThumbnailAsset.ashx?fmt=png&wd=75&ht=75&aid=" .. tostring(newId)
	end
end


------
--GUI-
------

function createGui()

	--Insert Panel
	currStampGui, setPanelVisibility, getPanelVisibility, waterTypeChangedEvent = 
		getRbxGui().CreateSetPanel(userSetIds, partSelected, dialogClosed, UDim2.new(0.8, 0, 0.9, 0), UDim2.new(0.1,0,0.05,0), true)
	setPanelVisibility(false)
	
	currStampGui.Parent = game:GetService("CoreGui")

	waterTypeChangedEvent.Event:connect(function(waterTable)
		waterForceAndDirection = waterTable
		updateWaterInfo()
	end)

	-- Loading Gui
	local loadingFrame = Instance.new("Frame")
	loadingFrame.Name = "LoadingFrame"
	loadingFrame.Style = Enum.FrameStyle.RobloxRound
	loadingFrame.Size = UDim2.new(0,350,0,60)
	loadingFrame.Visible = false
	loadingFrame.Position = UDim2.new(0.5,-175,0.5,-30)

	local loadingText = Instance.new("TextLabel")
	loadingText.Name = "LoadingText"
	loadingText.BackgroundTransparency = 1
	loadingText.Size = UDim2.new(0,155,1,0)
	loadingText.Font = Enum.Font.ArialBold
	loadingText.FontSize = Enum.FontSize.Size36
	loadingText.Text = "Loading..."
	loadingText.TextColor3 = Color3.new(1,1,1)
	loadingText.TextStrokeTransparency = 0
	loadingText.Parent = loadingFrame

	loadingFrame.Parent = currStampGui

	-- Recents Stack Gui
	recentsFrame = Instance.new("Frame")
	recentsFrame.BackgroundTransparency = 0.5
	recentsFrame.Name = "RecentsFrame"
	recentsFrame.BackgroundColor3 = Color3.new(0,0,0)
	recentsFrame.Size = UDim2.new(0,50,0,150)
	recentsFrame.Visible = false
	recentsFrame.Parent = currStampGui

	local recentButtonOne = Instance.new("ImageButton")
	recentButtonOne.Style = Enum.ButtonStyle.RobloxButton
	recentButtonOne.Name = "RecentButtonOne"
	recentButtonOne.Size = UDim2.new(0,50,0,50)
	recentButtonOne.Parent = recentsFrame

	local recentButtonTwo = recentButtonOne:clone()
	recentButtonTwo.Name = "RecentButtonTwo"
	recentButtonTwo.Position = UDim2.new(0,0,0,50)
	recentButtonTwo.Parent = recentsFrame

	local recentButtonThree = recentButtonOne:clone()
	recentButtonThree.Name = "RecentButtonThree"
	recentButtonThree.Position = UDim2.new(0,0,0,100)
	recentButtonThree.Parent = recentsFrame

	for i = 1, 3 do
		recentButtonStack[i] = {}
		recentButtonStack[i].Name = nil
		recentButtonStack[i].Id = nil
		recentButtonStack[i].TerrainShape = nil
	end

	recentButtonStack[1].Button = recentButtonOne
	recentButtonStack[2].Button = recentButtonTwo
	recentButtonStack[3].Button = recentButtonThree

	local buttonClicked = false

	for i = 1, #recentButtonStack do
		recentButtonStack[i].Button.MouseButton1Click:connect(function()
			if buttonClicked then return end
			buttonClicked = true
			partSelected(recentButtonStack[i].Name, recentButtonStack[i].Id, recentButtonStack[i].TerrainShape)
			buttonClicked = false
		end)
	end
end

--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
