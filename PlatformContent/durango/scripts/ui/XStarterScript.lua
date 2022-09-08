local scriptContext = game:GetService("ScriptContext")
local rbxGui = Game:GetService("CoreGui"):FindFirstChild("RobloxGui")

scriptContext:AddCoreScriptLocal("CoreScripts/AppHome", rbxGui)

-- TODO: Clean this up so it doesn't get created anymore, but for now we should remove it
spawn(function()
	local controlFrame = rbxGui:WaitForChild('ControlFrame')
	controlFrame:Destroy()
end)
