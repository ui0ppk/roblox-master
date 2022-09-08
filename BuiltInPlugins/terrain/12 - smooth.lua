while game == nil do
	wait(1/30)
end

self = PluginManager():CreatePlugin()

toolbar = self:CreateToolbar("Terrain")
toolbarbutton = toolbar:CreateButton("Convert To Smooth", "Convert To Smooth", "smooth.png")
toolbarbutton.Click:connect(function()
	local RbxGui = LoadLibrary("RbxGui")
	local g = Instance.new("ScreenGui", game:GetService("CoreGui"))

	if not workspace.Terrain.IsSmooth then
		if workspace.Terrain:CountCells() > 0 then
			local result = nil
			local confirm = RbxGui.CreateMessageDialog("Convert To Smooth?",
				"You are converting voxel terrain to smooth voxel terrain. Some materials are not supported any more. This action can not be undone. Proceed?",
				{{Text="Convert", Function=function() result = true end}, {Text="Cancel", Function=function() result = false end}})

			confirm.Parent = g

			while result == nil do
				wait(0.1)
			end

			confirm:Destroy()

			if not result then
				return
			end
		end

		workspace.Terrain:ConvertToSmooth()
	end
end)
