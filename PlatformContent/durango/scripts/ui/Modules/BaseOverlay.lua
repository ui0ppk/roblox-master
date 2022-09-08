--[[
			// BaseOverlay.lua

			// Implements a base overlay for overlay screens.
			// Any other overlay classes should require this module
			// first, then implement its own logic
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local ScrollingTextBox = require(Modules:FindFirstChild('ScrollingTextBox'))
local Utility = require(Modules:FindFirstChild('Utility'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))

local FADE_TIME = 0.25

local createBaseOverlay = function()
	local this = {}

	local OVERLAY_TRANSPARENCY = GlobalSettings.ModalBackgroundTransparency

	this.RightAlign = 776
	this.BaseZIndex = 8

	local modalOverlay = Utility.Create'Frame'
	{
		Name = "ModalOverlay";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
		BackgroundColor3 = Color3.new();
		BorderSizePixel = 0;
		ZIndex = this.BaseZIndex;
	}
	local container = Utility.Create'Frame'
	{
		Name = "Container";
		Size = UDim2.new(1, 0, 0, 668);
		Position = UDim2.new(0, 0, 0, 227);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.OverlayColor;
		ZIndex = this.BaseZIndex;
		Parent = modalOverlay;
	}
	local imageContainer = Utility.Create'Frame'
	{
		Name = "ImageContainer";
		Size = UDim2.new(0, 576, 0, 642);
		Position = UDim2.new(0, 100, 0.5, -321);
		BorderSizePixel = 0;
		BackgroundTransparency = 1;
		BackgroundColor3 = Color3.new();
		ZIndex = this.BaseZIndex + 1;
		Parent = container;
	}

	this.Container = container

	--[[ Public API ]]--
	function this:SetImageBackgroundTransparency(value)
		imageContainer.BackgroundTransparency = value
	end

	function this:SetImageBackgroundColor(value)
		imageContainer.BackgroundColor3 = value
	end

	function this:SetImage(guiImage)
		guiImage.Position = UDim2.new(0.5, -guiImage.Size.X.Offset/2, 0.5, -guiImage.Size.Y.Offset/2)
		guiImage.Parent = imageContainer
	end

	function this:GetOverlaySound()
		return 'OverlayOpen'
	end

	function this:SetDropShadow()
		local dropShadow = AssetManager.CreateShadow(3)
		dropShadow.Parent = imageContainer
	end

	function this:GetPriority()
		return GlobalSettings.DefaultPriority
	end

	function this:Show()
		modalOverlay.Parent = ScreenManager:GetScreenGuiByPriority(self:GetPriority())
		local overlayTweenIn = Utility.PropertyTweener(modalOverlay, "BackgroundTransparency",
			1, OVERLAY_TRANSPARENCY, FADE_TIME, Utility.EaseInOutQuad, nil)
		SoundManager:Play(self:GetOverlaySound())

		-- Show the modalOverlay when we are shown
		modalOverlay.Visible = true
	end

	function this:Hide()
		local overlayTweenOut = Utility.PropertyTweener(modalOverlay, "BackgroundTransparency",
			OVERLAY_TRANSPARENCY, 1, FADE_TIME, Utility.EaseInOutQuad, true,
			function()
				modalOverlay:Destroy()
			end)
		container.Parent = nil
		container:Destroy()
	end

	function this:Focus()
		ContextActionService:BindCoreAction("CloseOverlay",
			function(actionName, inputState, inputObject)
				if inputState == Enum.UserInputState.End then
					ScreenManager:CloseCurrent()
				end
			end,
		false, Enum.KeyCode.ButtonB)
		GuiService:AddSelectionParent("Overlay", container)

		-- Don't show overlays when not focused
		modalOverlay.Visible = true
	end

	function this:RemoveFocus()
		ContextActionService:UnbindCoreAction("CloseOverlay")
		GuiService:RemoveSelectionGroup("Overlay")

		-- Don't show overlays when not focused
		modalOverlay.Visible = false
	end

	function this:Close()
		if ScreenManager:GetTopScreen() == self then
			SoundManager:Play('ButtonPress')
			ScreenManager:CloseCurrent()
			return true
		end
		return false
	end

	return this
end

return createBaseOverlay
