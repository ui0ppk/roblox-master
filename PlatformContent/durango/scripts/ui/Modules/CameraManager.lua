-- Written by Kip Turner, Copyright ROBLOX 2015
-- CFrame animations by Tomarty :)


local CoreGui = game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local Utility = require(Modules:FindFirstChild('Utility'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local RunService = game:GetService("RunService")

local PlatformService;
pcall(function() PlatformService = game:GetService('PlatformService') end)
local UserInputService = game:GetService('UserInputService')

local function clerp(x)
	return (math.sin(math.pi * (x - 0.5)) + 1) * 0.5
end

local function clerp0(x)
	return clerp(x * 0.5)
end
local function clerp1(x)
	return clerp(x * 0.5 + 0.5)
end


local CameraManager = {}

function CameraManager:StartTransitionScreenEffect()
	if PlatformService then
		PlatformService.Brightness = GlobalSettings.SceneBrightness
		PlatformService.Contrast = GlobalSettings.SceneContrast
		PlatformService.GrayscaleLevel = GlobalSettings.SceneGrayscaleLevel
		PlatformService.TintColor = GlobalSettings.SceneTintColor
		PlatformService.BlurIntensity = GlobalSettings.SceneMotionBlurIntensity
	end
end

function CameraManager:EndTransitionScreenEffect( ... )
	-- if PlatformService then
	-- 	PlatformService.Brightness = GlobalSettings.SceneBrightness
	-- 	PlatformService.Contrast = GlobalSettings.SceneContrast
	-- 	PlatformService.GrayscaleLevel = GlobalSettings.SceneGrayscaleLevel
	-- 	PlatformService.TintColor = GlobalSettings.SceneTintColor
	-- 	PlatformService.BlurIntensity = GlobalSettings.SceneBlurIntensity
	-- end
end

local cameraConnection;


local camera = workspace.CurrentCamera
local function onCameraChanged()
	if workspace.CurrentCamera then
		camera = workspace.CurrentCamera
		camera.CameraType = 'Scriptable'
	end
end
workspace.Changed:connect(function()
	if prop == 'CurrentCamera' then
		onCameraChanged()
	end
end)
onCameraChanged()

local function CFrameBezierLerp(cframes, t)
	local cframes2 = {}
	for i = 1, #cframes - 1 do
		cframes2[i] = cframes[i]:lerp(cframes[i + 1], t)
	end
	if #cframes2 == 1 then
		return cframes2[1]
	end
	return CFrameBezierLerp(cframes2, t)
end

local cameraMoveCn = nil
local gamepadInput = Vector2.new(0, 0)
function CameraManager:EnableCameraControl()
	cameraMoveCn = Utility.DisconnectEvent(cameraMoveCn)
	cameraMoveCn = UserInputService.InputChanged:connect(function(input)
		if input.KeyCode == Enum.KeyCode.Thumbstick2 then
			gamepadInput = input.Position or gamepadInput
			gamepadInput = Vector2.new(gamepadInput.X, gamepadInput.Y)
		end
	end)
end
function CameraManager:DisableCameraControl()
	cameraMoveCn = Utility.DisconnectEvent(cameraMoveCn)
	gamepadInput = Vector2.new(0, 0)
end

local getGamepadInputCFrame; do
	local gamepadInputLerping = Vector2.new(0, 0)
	local timestamp0 = tick()
	function getGamepadInputCFrame()
		local timestamp1 = tick()
		local deltaTime = timestamp1 - timestamp0
		timestamp0 = timestamp1
		local unit = 0.125 ^ deltaTime
		gamepadInputLerping = gamepadInputLerping * unit + gamepadInput * (1 - unit)
		return CFrame.new(gamepadInputLerping.X/8, gamepadInputLerping.Y/8, 0) * CFrame.Angles(0, -gamepadInputLerping.X / 12, 0) * CFrame.Angles(gamepadInputLerping.Y / 12, 0, 0)
	end
end

local function lerpToCFrame(cframes, length, useVelocity, yieldEarlyLength)
	-- print("Lerpin")

	local name = "CameraScriptCutsceneLerp"

	yieldEarlyLength = yieldEarlyLength or 0
	yieldEarlyLength = 1 - yieldEarlyLength / length

	if useVelocity then
		-- TODO: estimate the total distance traveled
		--length = (cf0.p - cf1.p).magnitude / length
	end

	if cameraConnection then
		cameraConnection:disconnect()
		cameraConnection = nil
	end

	local timestamp0 = tick()

	local event = Instance.new("BindableEvent")

	cameraConnection = {
		disconnect = function()
			if event then
				event:Fire()
				event = nil
			end
			RunService:UnbindFromRenderStep(name)
		end
	}

	RunService:BindToRenderStep(name, Enum.RenderPriority.Camera.Value, function()

		local timestamp1 = tick()
		local t = (timestamp1 - timestamp0) / length

		if t >= 1 then
			t = 1
		end

		--print(t)

		local cam =	Workspace.CurrentCamera
		cam.CoordinateFrame = CFrameBezierLerp(cframes, t) * getGamepadInputCFrame()
		if t >= 1 then
			if event then
				event:Fire()
				event = nil
			end
			cameraConnection:disconnect()
		elseif t >= yieldEarlyLength then
			--local test = cf0:lerp(cf1, (t))
			--print(test)
			if event then
				event:Fire()
				event = nil
			end
		end
	end)

	event.Event:wait()

end


local function GetCameraParts(model)
	local parts = {}
	for i, part in pairs(model:GetChildren()) do
		parts[tonumber(part.Name:sub(4, -1))] = part
		part.Transparency = 1
	end
	return parts
end

local function PlayAnimationAsync(cameras, doTransition, isPan)

	if #cameras <= 1 then
		return
	end

	local cam =	Workspace.CurrentCamera
	local time = 1.7

	cam.CoordinateFrame = cameras[1].CFrame

	if (doTransition) then
		delay(time, function()
			if PlatformService then
				Utility.PropertyTweener(PlatformService, 'Contrast', PlatformService.Contrast, GlobalSettings.SceneContrast, time,  Utility.EaseInOutQuad, true)
				Utility.PropertyTweener(PlatformService, 'BlurIntensity', PlatformService.BlurIntensity, GlobalSettings.SceneMotionBlurIntensity, time,  Utility.EaseInOutQuad, true)
			end
		end)
	end


	local cframes = {}
	for i = 1, #cameras do
		cframes[i] = cameras[i].CFrame * CFrame.new(0, 0, -cameras[i].Size.Z / 2)
	end
	lerpToCFrame(cframes, isPan and 60 or 120, false, time + 0.1)
	-- lerpToCFrame(cframes, 15, false, time + 0.1)

	if PlatformService then
		Utility.PropertyTweener(PlatformService, 'Contrast', GlobalSettings.SceneContrast, -1, time,  Utility.EaseInOutQuad, true)
		Utility.PropertyTweener(PlatformService, 'BlurIntensity', GlobalSettings.SceneMotionBlurIntensity, 50, time,  Utility.EaseInOutQuad, true)
	end

	wait(time)

end

pcall(function()
	local function recurse(model)
		local children = model:GetChildren()
		for i = 1, #children do
			local child = children[i]
			if child:IsA("BasePart") then
				child.Locked = true
				child.Anchored = true
				child.CanCollide = false
			end
			recurse(child)
		end
	end
	recurse(workspace)
end)



local ZoneManager = require(script.Parent:WaitForChild("CameraManagerModules"):WaitForChild("CameraManager_ZoneManager"))

function CameraManager:CameraMoveToAsync( ... )
	local cameraSets = {}
	for k, f in pairs(workspace:WaitForChild("Cameras"):GetChildren()) do
		cameraSets[f.Name] = GetCameraParts(f)
	end

	local first = true
	while true do
		ZoneManager:SetZone("City")
		PlayAnimationAsync(cameraSets.City, not first, false)
		ZoneManager:SetZone("Space")
		PlayAnimationAsync(cameraSets.Space, true, true)
		ZoneManager:SetZone("Volcano")
		PlayAnimationAsync(cameraSets.Volcano, true, true)

		first = false
	end
end


return CameraManager


