-- we'll load this later; workaround for strange crash
local RbxStamper = nil

local stampControl = nil
local stampingCon = nil


-----------------
--DEFAULT VALUES-
-----------------
local on = false
local loaded = false
local selectionConnection = nil
local myModel = nil
local whichStep = 1

local backEnabled = false
local continueEnabled = false

local bbPreviewCoroutine = nil
local facePreviewCoroutine = nil

local bbPreview = Instance.new("Part")
bbPreview.Name = "SideSelectionBox"
bbPreview.Transparency = .7
bbPreview.formFactor = "Custom"
bbPreview.Size = Vector3.new(4, 4, 4)
bbPreview.Transparency = 1
bbPreview.CanCollide = false
bbPreview.Anchored = true
bbPreview.CFrame = CFrame.new(0, 0, 0)
bbPreview.TopSurface = "Smooth"
bbPreview.BottomSurface = "Smooth"

local alignmentVect = Vector3.new(0, 0, 0)
local alignToFaceText = "[None]"
local unstampableFacesText = "[None]"
local unstampableFaces = {false, false, false, false, false, false}
local unjoinableFacesText = "[None]"
local unjoinableFaces = {false, false, false, false, false, false}

local SURFACE_FROM_NUMBER = {Enum.NormalId.Back, Enum.NormalId.Bottom, Enum.NormalId.Front, Enum.NormalId.Left, Enum.NormalId.Right, Enum.NormalId.Top}
local ALIGN_TO_FACE_MAPPING = {1, nil, 3, 2, 0, nil}
local UNSTAMPABLE_FACE_MAPPING = {"3", "-2", "-3", "-1", "1", "2"}
local SURFACE_TO_NUMBER = {}
local REVERSE_ALIGN_TO_FACE_MAPPING = {}
local REVERSE_UNSTAMPABLE_FACE_MAPPING = {}

for i = 1, #SURFACE_FROM_NUMBER do
	SURFACE_TO_NUMBER[SURFACE_FROM_NUMBER[i]] = i
end
for i = 1, #ALIGN_TO_FACE_MAPPING do
	if ALIGN_TO_FACE_MAPPING[i] then REVERSE_ALIGN_TO_FACE_MAPPING[ALIGN_TO_FACE_MAPPING[i]] = SURFACE_FROM_NUMBER[i] end
end
for i = 1, #UNSTAMPABLE_FACE_MAPPING do
	REVERSE_UNSTAMPABLE_FACE_MAPPING[tonumber(UNSTAMPABLE_FACE_MAPPING[i])] = i
end
---------------
--PLUGIN SETUP-
---------------

self = PluginManager():CreatePlugin()
mouse = self:GetMouse()
mouse.Button1Down:connect(function() onClicked(mouse) end)
self.Deactivation:connect(function()
	Off()
end)
toolbar = self:CreateToolbar("Utilities")
toolbarbutton = toolbar:CreateButton("", "Stamper Wizard: Make models you can stamp out!", "stampwizard.png")
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

function roundToGrid(vect)
	return Vector3.new(math.ceil(vect.X/4)*4, math.ceil(vect.Y/4)*4, math.ceil(vect.Z/4)*4)
end

function toAlignmentString(vect)
	local aString = ""
	aString = aString .. tostring(math.floor(vect.X*100 + .5) / 100) .. ", "
	aString = aString .. tostring(math.floor(vect.Y*100 + .5) / 100) .. ", "
	aString = aString .. tostring(math.floor(vect.Z*100 + .5) / 100)
	return aString
end

function updateBBPreview()
	while bbPreviewBox.Visible do
		myModel:SetIdentityOrientation()
		local myModelSize = myModel:GetModelSize()
		bbPreview.Size = roundToGrid(myModelSize)
		bbPreview.CFrame = myModel:GetModelCFrame() + alignmentVect*(bbPreview.Size - myModelSize)*.5
		if whichStep == 3 then selectedLabel.Text = "Size: " .. tostring(bbPreview.Size)
		elseif whichStep == 4 then selectedLabel.Text = "Alignment: " .. toAlignmentString(alignmentVect)
		elseif whichStep == 5 then selectedLabel.Text = alignToFaceText
		elseif whichStep == 6 then selectedLabel.Text = unjoinableFacesText
		elseif whichStep == 7 then selectedLabel.Text = unstampableFacesText end
		wait(.1)
	end
	bbPreviewCoroutine = nil
end

function getFaceArrayText(array)
	local currText = ""
	local foundOne = false
	for i = 1, 6 do
		if array[i] then
			if foundOne then currText = currText .. ", " end
			currText = currText .. string.gsub(tostring(SURFACE_FROM_NUMBER[i]), "Enum.NormalId.", "")
			foundOne = true
		end
	end
	if not foundOne then return "[None]" else return currText end
end


function loadMetaData(myModel)
	if not myModel or not myModel:IsA("Model") then return end
	
	if myModel.PrimaryPart then myPrimaryPart = myModel.PrimaryPart end

	local jTag = myModel:FindFirstChild("Justification")
	local aTag = myModel:FindFirstChild("AutoAlignToFace")
	local usTag = myModel:FindFirstChild("UnstampableFaces")
	local ujTag = myModel:FindFirstChild("UnjoinableFaces")
	if jTag then 
		alignmentVect = -jTag.Value + Vector3.new(1, 1, 1)
	end
	if aTag then 
		alignToFaceSelection.TargetSurface = REVERSE_ALIGN_TO_FACE_MAPPING[aTag.Value]
		alignToFaceText = string.gsub(tostring(alignToFaceSelection.TargetSurface), "Enum.NormalId.", "")
	end
	if usTag then
		local uString = usTag.Value
		for i = 1, 6 do unstampableFaces[i] = false end

		for w in string.gmatch(uString, "[^,]*") do
			if tonumber(w) then unstampableFaces[REVERSE_UNSTAMPABLE_FACE_MAPPING[tonumber(w)]] = true end
		end
		unstampableFacesText = getFaceArrayText(unstampableFaces)
	end
	if ujTag then
		local uString = ujTag.Value
		for i = 1, 6 do unjoinableFaces[i] = false end

		for w in string.gmatch(uString, "[^,]*") do
			if tonumber(w) then unjoinableFaces[REVERSE_UNSTAMPABLE_FACE_MAPPING[tonumber(w)]] = true end
		end
		unjoinableFacesText = getFaceArrayText(unjoinableFaces)
	end
end

function setupStamper(stampModel)
	if stampControl then stampControl.Destroy() end
	if stampingCon then
		stampingCon:disconnect()
		stampingCon = nil
	end

	stampControl = RbxStamper.SetupStamperDragger(stampModel, mouse)
	stampingCon = stampControl.Stamped.Changed:connect(function()
		if stampControl and stampControl.Stamped.Value then
			stampControl.ReloadModel()
		end
	end)
end

local stampTesting = false
function testStamperModel()
	if stampTesting then
		stopStampTest()
	else
		startStampTest()
	end
end

local stampedModelListener = nil
local stampedModels = {}


function listenForStampedModels(childAdded)
	if childAdded and childAdded.Parent == game.Workspace and childAdded:FindFirstChild("RobloxStamper", true) then
		table.insert(stampedModels, childAdded)
	end
end

function insertStamperTags(parentModel)
	local stampTag = parentModel:FindFirstChild("RobloxStamper")
	if not stampTag then 
		stampTag = Instance.new("BoolValue")
		stampTag.Name = "RobloxStamper"
		stampTag.Parent = parentModel
	end

	local modelTag = parentModel:FindFirstChild("RobloxModel")
	if not modelTag then
		modelTag = Instance.new("BoolValue")
		modelTag.Name = "RobloxModel"
		modelTag.Parent = parentModel
	end
end

function startStampTest()
	stampTesting = true
	testButton.Text = "Stop testing"

	-- start listening for all models stamped via testing mechanism
	stampedModelListener = game.Workspace.ChildAdded:connect(listenForStampedModels)

	local cloneInstance = myModel:Clone()
	finishModel(cloneInstance)
	insertStamperTags(cloneInstance)
	cloneInstance:ResetOrientationToIdentity()
	cloneInstance:BreakJoints()
	setupStamper(cloneInstance)
end

function stopStampTest()
	stampTesting = false
	testButton.Text = "Test it!"

	if stampControl then
		stampControl.Destroy()
	end
	
	-- remove all models stamped via testing mechanism
	if stampedModelListener then stampedModelListener:disconnect() end
	for i = 1, #stampedModels do stampedModels[i]:Remove() end
	stampedModels = {}
end


-- just returns the first ancestor of type "Model", but nil if that model is a direct parent of the game (like game.Workspace)
function getModelFromPart(part)
	if not part then return {} end
	if part.Parent == game then return {} end
	if part:IsA("Model") then
		return {part}
	else
		return getModelFromPart(part.Parent)
	end
end

function onClicked(mouse)
	if surfaceSelection.Visible then
		if whichStep == 5 then
			if alignToFaceSelection.Visible and alignToFaceSelection.TargetSurface == surfaceSelection.TargetSurface then
				-- turn off that face
				alignToFaceText = "[None]"
				alignToFaceSelection.Visible = false
			else
				alignToFaceSelection.TargetSurface = surfaceSelection.TargetSurface
				alignToFaceSelection.Visible = true
				alignToFaceText = string.gsub(tostring(alignToFaceSelection.TargetSurface), "Enum.NormalId.", "")
			end
		elseif whichStep == 6 then
			local whichFace = SURFACE_TO_NUMBER[surfaceSelection.TargetSurface]
			local unjoinableFaceSelector = unjoinableFaceSelections[whichFace]
			if unjoinableFaceSelector.Visible then
				unjoinableFaceSelector.Visible = false
				unjoinableFaces[whichFace] = false
				unjoinableFacesText = getFaceArrayText(unjoinableFaces)
			else
				unjoinableFaceSelector.Visible = true
				unjoinableFaces[whichFace] = true
				unjoinableFacesText = getFaceArrayText(unjoinableFaces)
			end
		elseif whichStep == 7 then
			local whichFace = SURFACE_TO_NUMBER[surfaceSelection.TargetSurface]
			local unstampableFaceSelector = unstampableFaceSelections[whichFace]
			if unstampableFaceSelector.Visible then
				unstampableFaceSelector.Visible = false
				unstampableFaces[whichFace] = false
				unstampableFacesText = getFaceArrayText(unstampableFaces)
			else
				unstampableFaceSelector.Visible = true
				unstampableFaces[whichFace] = true
				unstampableFacesText = getFaceArrayText(unstampableFaces)
			end
		end
	elseif whichStep == 1 then
		-- allow them to click directly on the model they want to choose
		if mouse.Target then game.Selection:Set(getModelFromPart(mouse.Target)) end
	elseif whichStep == 2 then
		-- allow them to click directly on the part they want to choose
		if mouse.Target then game.Selection:Set({mouse.Target}) end
	end
end

function updateFacePreview()
	while bbPreview.Parent do
		if mouse.Target == bbPreview and (whichStep ~= 5 or (mouse.TargetSurface ~= Enum.NormalId.Top and mouse.TargetSurface ~= Enum.NormalId.Bottom)) then
			surfaceSelection.TargetSurface = mouse.TargetSurface
			surfaceSelection.Visible = true
			if (whichStep == 5 and alignToFaceSelection.Visible and alignToFaceSelection.TargetSurface == surfaceSelection.TargetSurface) then surfaceSelection.Color = BrickColor.new("Toothpaste")
			elseif (whichStep == 6 and unjoinableFaces[SURFACE_TO_NUMBER[surfaceSelection.TargetSurface]]) then surfaceSelection.Color = BrickColor.new("New Yeller")
			elseif (whichStep == 7 and unstampableFaces[SURFACE_TO_NUMBER[surfaceSelection.TargetSurface]]) then surfaceSelection.Color = BrickColor.new("Really red")
			else surfaceSelection.Color = BrickColor.new("Light stone grey") end
		else
			surfaceSelection.Visible = false
		end

		wait(.1)
	end
	facePreviewCoroutine = nil
end

function alignmentTruncate(num)
	if num < -1 then return -1
	elseif num > 1 then return 1
	else return num end
end

function dragBB(face, dist)
	if face == Enum.NormalId.Front then
		alignmentVect = Vector3.new(alignmentVect.X, alignmentVect.Y, alignmentTruncate(math.ceil(-dist/20)))
	elseif face == Enum.NormalId.Back then
		alignmentVect = Vector3.new(alignmentVect.X, alignmentVect.Y, alignmentTruncate(math.floor(dist/20)))
	elseif face == Enum.NormalId.Bottom then 
		alignmentVect = Vector3.new(alignmentVect.X, alignmentTruncate(math.ceil(-dist/20)), alignmentVect.Z)
	elseif face == Enum.NormalId.Top then
		alignmentVect = Vector3.new(alignmentVect.X, alignmentTruncate(math.floor(dist/20)), alignmentVect.Z)
	elseif face == Enum.NormalId.Left then
		alignmentVect = Vector3.new(alignmentTruncate(math.ceil(-dist/20)), alignmentVect.Y, alignmentVect.Z)
	elseif face == Enum.NormalId.Right then
		alignmentVect = Vector3.new(alignmentTruncate(math.floor(dist/20)), alignmentVect.Y, alignmentVect.Z)
	else return end
end


function refreshPage(whichPage)
	print("refresh")
	if whichPage == 1 then
		-- Select Model
		continueButton.Text = "Continue"
		infoBox.Text = "Select the model you want to turn into a stamper part."
		if myModel then selectedLabel.Text = myModel.Name setContinueEnabled(true) if #game.Selection:Get() ~= 1 or game.Selection:Get()[1] ~= myModel then game.Selection:Set({myModel}) end else selectedLabel.Text = "[None]" setContinueEnabled(false) end
	elseif whichPage == 2 then
		-- Select Primary Part
		bbPreviewBox.Visible = false
		infoBox.Text = "Select one part inside your model to be the \"key part\"."
		if myPrimaryPart then selectedLabel.Text = myPrimaryPart.Name setContinueEnabled(true) if #game.Selection:Get() ~= 1 or game.Selection:Get()[1] ~= myPrimaryPart then game.Selection:Set({myPrimaryPart}) end else selectedLabel.Text = "[None]" setContinueEnabled(false) end
	elseif whichPage == 3 then
		-- Create Bounding Box
		alignmentHandles.Visible = false
		myModel.PrimaryPart = myPrimaryPart
		infoBox.Text = "Rotate and adjust the model so the green bounding box looks good."
		if #game.Selection:Get() ~= 1 or game.Selection:Get()[1] ~= myModel then game.Selection:Set({myModel}) end

		setContinueEnabled(true)
	elseif whichPage == 4 then
		-- Set Alignment Within Bounding Box
		infoBox.Text = "Move the green bounding box so your model is aligned correctly inside it."
		alignToFaceSelection.Visible = false
		surfaceSelection.Visible = false
		alignmentHandles.Visible = true
		if #game.Selection:Get() ~= 1 or game.Selection:Get()[1] ~= myModel then game.Selection:Set({myModel}) end
		bbPreview.Parent = nil
		setContinueEnabled(true)		
	elseif whichPage == 5 then
		-- AutoAlignToFace
		alignmentHandles.Visible = false
		if (alignToFaceText ~= "[None]") then alignToFaceSelection.Visible = true end
		for i = 1, 6 do unjoinableFaceSelections[i].Visible = false end
		infoBox.Text = "[Optional Step]: Select a side to be \"always facing the wall\"."
		if #game.Selection:Get() ~= 0 then game.Selection:Set({}) end
		setContinueEnabled(true)		
	elseif whichPage == 6 then
		-- Unjoinable Faces
		alignToFaceSelection.Visible = false
		for i = 1, 6 do unjoinableFaceSelections[i].Visible = unjoinableFaces[i] end
		for i = 1, 6 do unstampableFaceSelections[i].Visible = false end
		infoBox.Text = "[Optional Step]: Select any sides you don't want things to join to when stamping."
		if #game.Selection:Get() ~= 0 then game.Selection:Set({}) end
		setContinueEnabled(true)
	elseif whichPage == 7 then
		-- Unstampable Faces
		for i = 1, 6 do unstampableFaceSelections[i].Visible = unstampableFaces[i] end
		for i = 1, 6 do unjoinableFaceSelections[i].Visible = false end
		infoBox.Text = "[Optional Step]: Select any sides you don't want to be stampable or have things stamp to."
		continueButton.Text = "Continue"
		testButton.Visible = false
		if #game.Selection:Get() ~= 0 then game.Selection:Set({}) end
		setContinueEnabled(true)
		stopStampTest()
	elseif whichPage == 8 then
		-- Test it out!
		for i = 1, 6 do unstampableFaceSelections[i].Visible = false end
		bbPreview.Parent = nil
		selectedLabel.Text = myModel.Name
		infoBox.Text = "Test it out!  Click \"Finish\" when it's ready to publish!"
		continueButton.Text = "Finish"
		testButton.Visible = true
		setContinueEnabled(true)
	end

	if whichPage >=3 and whichPage <= 7 then
		bbPreviewBox.Visible = true

		if not bbPreviewCoroutine then
			bbPreviewCoroutine = coroutine.create(updateBBPreview)
			coroutine.resume(bbPreviewCoroutine)
		end

		if whichPage > 4 then
			bbPreview.Parent = game.Workspace
			if not facePreviewCoroutine then
				facePreviewCoroutine = coroutine.create(updateFacePreview)
				coroutine.resume(facePreviewCoroutine)
			end
		end
	end

	stepText.Text = "Step " .. tostring(whichPage) .. " of 8"
end


function setContinueEnabled(isEnabled)
	if isEnabled then
		continueButton.TextColor3 = Color3.new(1, 1, 1)
	else
		continueButton.TextColor3 = Color3.new(.5, .5, .5)
	end
	continueEnabled = isEnabled
end

function setBackEnabled(isEnabled)
	if isEnabled then
		backButton.TextColor3 = Color3.new(1, 1, 1)
	else
		backButton.TextColor3 = Color3.new(.5, .5, .5)
	end
	backEnabled = isEnabled
end

function onSelect()
	print("onSelect")
	local selected = game.Selection:Get()

	if whichStep == 1 then -- looking for a single model to be selected
		if #selected == 1 and selected[1] == myModel then return end

		if #selected == 1 and selected[1]:IsA("Model") and selected[1] ~= game.Workspace then
			myModel = selected[1]
			myPrimaryPart = nil
		else
			myModel = nil
			myPrimaryPart = nil
		end
		refreshPage(1)
	elseif whichStep == 2 then
		if #selected == 1 and selected[1] == myPrimaryPart then return end

		if #selected == 1 and selected[1]:IsA("BasePart") and selected[1].Parent == myModel then
			myPrimaryPart = selected[1]
		else
			myPrimaryPart = nil
		end
		refreshPage(2)
	end
end

function On()
	self:Activate(true)
	toolbarbutton:SetActive(true)
	frame.Visible = true

	for w = 0, 0.6, 0.12 do
		frame.Size = UDim2.new(w, 0, 2*w/3, 0)
		wait()
	end

	on = true

	-- connect to the game's selection
	selectionConnection = game.Selection.SelectionChanged:connect(onSelect)

	refreshPage(whichStep)
end

function Off()
	toolbarbutton:SetActive(false)

	for w = 0.6, 0, -0.12 do
		frame.Size = UDim2.new(w, 0, 2*w/3, 0)
		wait()
	end

	frame.Visible = false
	on = false

	if selectionConnection then selectionConnection:disconnect() selectionConnection = nil end

	alignmentHandles.Visible = false
	surfaceSelection.Visible = false
	alignToFaceSelection.Visible = false
	for i = 1, 6 do unstampableFaceSelections[i].Visible = false end
	for i = 1, 6 do unjoinableFaceSelections[i].Visible = false end

	stopStampTest()

	if whichStep ~= 3 then bbPreviewBox.Visible = false end
	bbPreview.Parent = nil
end

function resetWizard()
	bbPreview.Parent = nil
	bbPreviewBox.Visible = false
	wait(.5) -- so that we can kill our coroutines safely

	myModel = nil
	myPrimaryPart = nil
	whichStep = 1
	for i = 1, 6 do unstampableFaces[i] = false end
	for i = 1, 6 do unjoinableFaces[i] = false end
	alignToFaceText = "[None]"
	alignmentVect = Vector3.new(0, 0, 0)
	testButton.Visible = false

	setBackEnabled(false)
	setContinueEnabled(false)
	
	alignmentHandles.Visible = false
	surfaceSelection.Visible = false
	alignToFaceSelection.Visible = false
	for i = 1, 6 do unstampableFaceSelections[i].Visible = false unstampableFaces[i] = false end
	for i = 1, 6 do unjoinableFaceSelections[i].Visible = false unjoinableFaces[i] = false end

	refreshPage(whichStep)
end

function finishModel(model)
	if alignmentVect.magnitude > .04 then
		local alignTag = model:FindFirstChild("Justification")
		if not alignTag then alignTag = Instance.new("Vector3Value") end
		alignTag.Name = "Justification"
		alignTag.Value = -alignmentVect + Vector3.new(1, 1, 1)
		alignTag.Parent = model
	elseif model:FindFirstChild("Justification") then model.Justification:Remove() end
	
	if alignToFaceText ~= "[None]" then
		local autoAlignToFaceTag = model:FindFirstChild("AutoAlignToFace")
		if not autoAlignToFaceTag then autoAlignToFaceTag = Instance.new("NumberValue") end
		autoAlignToFaceTag.Name = "AutoAlignToFace"
		autoAlignToFaceTag.Value = ALIGN_TO_FACE_MAPPING[SURFACE_TO_NUMBER[alignToFaceSelection.TargetSurface]]
		autoAlignToFaceTag.Parent = model
	elseif model:FindFirstChild("AutoAlignToFace") then model.AutoAlignToFace:Remove() end

	local unstampableFaceString = ""
	local foundOne = false
	for i = 1, #unstampableFaces do
		if unstampableFaces[i] then
			if foundOne then unstampableFaceString = unstampableFaceString .. ", " end
			unstampableFaceString = unstampableFaceString .. UNSTAMPABLE_FACE_MAPPING[i]
			foundOne = true
		end
	end
	if foundOne then
		local unstampableFaceTag = model:FindFirstChild("UnstampableFaces")
		if not unstampableFaceTag then unstampableFaceTag = Instance.new("StringValue") end
		unstampableFaceTag.Name = "UnstampableFaces"
		unstampableFaceTag.Value = unstampableFaceString
		unstampableFaceTag.Parent = model
	elseif model:FindFirstChild("UnstampableFaces") then model.UnstampableFaces:Remove() end

	local unjoinableFaceString = ""
	foundOne = false
	for i = 1, #unjoinableFaces do
		if unjoinableFaces[i] then
			if foundOne then unjoinableFaceString = unjoinableFaceString .. ", " end
			unjoinableFaceString = unjoinableFaceString .. UNSTAMPABLE_FACE_MAPPING[i]
			foundOne = true
		end
	end
	if foundOne then
		local unjoinableFaceTag = model:FindFirstChild("UnjoinableFaces")
		if not unjoinableFaceTag then unjoinableFaceTag = Instance.new("StringValue") end
		unjoinableFaceTag.Name = "UnjoinableFaces"
		unjoinableFaceTag.Value = unjoinableFaceString
		unjoinableFaceTag.Parent = model
	elseif model:FindFirstChild("UnjoinableFaces") then model.UnjoinableFaces:Remove() end
end

function goBack()
	if backEnabled then
		whichStep = whichStep - 1
		if whichStep == 1 then setBackEnabled(false) end
		setContinueEnabled(true)
		refreshPage(whichStep)
	end
end

function goForwards()
	if continueEnabled then
		whichStep = whichStep + 1
		setContinueEnabled(false)
		setBackEnabled(true)
		if whichStep == 9 then
			stopStampTest()
			finishModel(myModel)
			resetWizard()
			--Off() -- if we want to auto-close the wizard after we finish 
		else
			if whichStep == 2 then
				-- upon selecting a model, we propagate all other values with the current model's values
				loadMetaData(myModel)
			end

			refreshPage(whichStep)
		end
	end
end



------
--GUI-
------
--screengui
g = Instance.new("ScreenGui", game:GetService("CoreGui"))

--frame
frame = Instance.new("Frame", g)
frame.Position = UDim2.new(0.2, 20, 0, 0)
frame.Size = UDim2.new(0.6, -40, .4, 0)
frame.Style = Enum.FrameStyle.RobloxRound
frame.BackgroundTransparency = 0.5
frame.Visible = false

titleText = Instance.new("TextLabel", frame)
titleText.Name = "Title"
titleText.BackgroundTransparency = 1
titleText.Font = Enum.Font.ArialBold
titleText.FontSize = Enum.FontSize.Size24
titleText.Size = UDim2.new(1, 0, 0, 24)
titleText.Text = "Stamper Model Maker"
titleText.TextColor3 = Color3.new(1,1,1)
titleText.TextYAlignment = Enum.TextYAlignment.Center

stepText = titleText:Clone()
stepText.Name = "StepsLabel"
stepText.Text = "Step 1 of 6"
stepText.Position = UDim2.new(0, -5, .81, 0)
stepText.Font = Enum.Font.Arial
stepText.Parent = frame

infoBox = Instance.new("TextLabel", frame)
infoBox.Name = "InfoBox"
infoBox.Font = Enum.Font.Arial
infoBox.FontSize = Enum.FontSize.Size24
infoBox.Size = UDim2.new(1,0,0,24)
infoBox.Position = UDim2.new(0, 0, 0.15, 0)
infoBox.BackgroundTransparency = 0.75
infoBox.TextColor3 = Color3.new(1,1,1)
infoBox.Text = "Select the model you want to turn into a stamper part."
infoBox.TextWrap = false
infoBox.TextXAlignment = Enum.TextXAlignment.Left
infoBox.TextYAlignment = Enum.TextYAlignment.Top
infoBox.Visible = true

modelInfoFrame = Instance.new("Frame", frame)
modelInfoFrame.Style = Enum.FrameStyle.RobloxRound
modelInfoFrame.Name = "ModelInfo"
modelInfoFrame.Position = UDim2.new(.395, 0, .325, 0)
modelInfoFrame.Size = UDim2.new(.2, 0, .4, 0)

selectedLabel = stepText:Clone()
selectedLabel.Name = "SelectionLabel"
selectedLabel.Text = "[None]"
selectedLabel.Position = UDim2.new(0, 0, .35, 0)
selectedLabel.Parent = modelInfoFrame		

backButton = Instance.new("TextButton", frame)
backButton.Name = "Back"
backButton.Font = Enum.Font.Arial
backButton.FontSize = Enum.FontSize.Size18
backButton.Size = UDim2.new(0, 200, 0, 50)
backButton.Position = UDim2.new(0, 1, 1, -55)
backButton.BackgroundTransparency = 1
backButton.Style = Enum.ButtonStyle.RobloxButtonDefault
backButton.Text = "Back"
backButton.TextColor3 = Color3.new(0.5, 0.5, 0.5)
backButton.MouseButton1Click:connect(goBack)

continueButton = backButton:Clone()
continueButton.Name = "Continue"
continueButton.Text = "Continue"
continueButton.Position = UDim2.new(1, -200, 1, -55)
continueButton.MouseButton1Click:connect(goForwards)
continueButton.Parent = frame

testButton = backButton:Clone()
testButton.Name = "Test"
testButton.Text = "Test it!"
testButton.TextColor3 = Color3.new(1, 1, 1)
testButton.Position = UDim2.new(1, -200, 1, -110)
testButton.MouseButton1Click:connect(testStamperModel)
testButton.Visible = false
testButton.Parent = frame

bbPreviewBox = Instance.new("SelectionBox")
bbPreviewBox.Color = BrickColor.new("Lime green")
bbPreviewBox.Adornee = bbPreview
bbPreviewBox.Visible = false
bbPreviewBox.Parent = frame

alignmentHandles = Instance.new("Handles")
alignmentHandles.Visible = false
alignmentHandles.Adornee = bbPreview
alignmentHandles.Parent = frame
alignmentHandles.MouseDrag:connect(dragBB)
alignmentHandles.Color = BrickColor.new("Lime green")

surfaceSelection = Instance.new("SurfaceSelection")
surfaceSelection.Name = "SurfaceSelection"
surfaceSelection.Color = BrickColor.new("White")
surfaceSelection.Transparency = .8
surfaceSelection.Visible = false
surfaceSelection.Adornee = bbPreview

alignToFaceSelection = surfaceSelection:Clone()
alignToFaceSelection.Name = "AlignToFaceSelection"
alignToFaceSelection.Color = BrickColor.new("Cyan")

unstampableFaceSelections = {}
for i = 1, 6 do
	local newUnstampableFaceSelection = alignToFaceSelection:Clone()
	newUnstampableFaceSelection.Name = "UnstampableFaceSelection"
	newUnstampableFaceSelection.Color = BrickColor.new("Bright red")
	newUnstampableFaceSelection.TargetSurface = SURFACE_FROM_NUMBER[i]
	newUnstampableFaceSelection.Parent = frame
	table.insert(unstampableFaceSelections, newUnstampableFaceSelection)
end

unjoinableFaceSelections = {}
for i = 1, 6 do
	local newUnjoinableFaceSelection = alignToFaceSelection:Clone()
	newUnjoinableFaceSelection.Name = "UnjoinableFaceSelection"
	newUnjoinableFaceSelection.Color = BrickColor.new("Deep orange")
	newUnjoinableFaceSelection.TargetSurface = SURFACE_FROM_NUMBER[i]
	newUnjoinableFaceSelection.Parent = frame
	table.insert(unjoinableFaceSelections, newUnjoinableFaceSelection)
end

alignToFaceSelection.Parent = frame
surfaceSelection.Parent = frame


-- load the lib'ary!
RbxStamper = LoadLibrary("RbxStamper")

if not RbxStamper then
	print("Error: RbxStamper Library Load Fail! Returning")
	return nil
end


--------------------------
--SUCCESSFUL LOAD MESSAGE-
--------------------------
loaded = true
print("StamperWizard Plugin Loaded")
