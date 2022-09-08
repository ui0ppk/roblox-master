--[[
				// NoActionOverlay.lua

				// Creates an overlay where the user cannot take an action
				// to remove.

				// This is used when we detect something wrong with input or the active user
				// being lost
]]

local DATAMODEL_TYPE = {
	APP_SHELL = 0;
	GAME = 1;
}

local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")
local PlatformService = nil
pcall(function() PlatformService = game:GetService('PlatformService') end)

local BaseOverlay = require(Modules:FindFirstChild('BaseOverlay'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local createNoActionOverlay = function(errorType)
	local this = BaseOverlay()

	local title = errorType.Title
	local message = errorType.Msg
	local errorCode = errorType.Code

	local iconImage = Utility.Create'ImageLabel'
	{
		Name = "IconImage";
		Size = UDim2.new(0, 416, 0, 416);
		BackgroundTransparency = 1;
		ZIndex = this.BaseZIndex;
		Image = 'rbxasset://textures/ui/Shell/Icons/AlertIcon.png';
	}
	Utility.CalculateAnchor(iconImage, UDim2.new(0.5, 0, 0.5, 0), Utility.Enum.Anchor.Center)
	this:SetImage(iconImage)

	local titleText = Utility.Create'TextLabel'
	{
		Name = "TitleText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, this.RightAlign, 0, 136);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.HeaderSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = title;
		TextXAlignment = Enum.TextXAlignment.Left;
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}

	local descriptionText = Utility.Create'TextLabel'
	{
		Name = "DescriptionText";
		Size = UDim2.new(0, 762, 0, 304);
		Position = UDim2.new(0, this.RightAlign, 0, titleText.Position.Y.Offset + 62);
		BackgroundTransparency = 1;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextWrapped = true;
		Text = message;
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}
	if errorCode then
		descriptionText.Text = string.format(Strings:LocalizedString('ErrorMessageAndCodePrase'), message, errorCode)
	end

	function this:GetPriority()
		return GlobalSettings.ImmediatePriority
	end

	--[[ Public API ]]--
	-- override
	function this:Focus()
		-- DO NOTHING
	end
	function this:RemoveFocus()
		-- DO NOTHING
	end


	local baseShow = this.Show
	function this:Show()
		ContextActionService:BindCoreAction("StopControllerInput", function() end, false, Enum.UserInputType.Gamepad1)
		baseShow(self)
	end

	local baseHide = this.Hide
	function this:Hide()
		baseHide(self)
		ContextActionService:UnbindCoreAction("StopControllerInput")
	end

	local baseFocus = this.Focus
	function this:Focus()
		baseFocus()
		-- NOTE: This might want to be:
		-- `not PlatformService or`
		if PlatformService and PlatformService.DatamodelType == DATAMODEL_TYPE.APP_SHELL then
			GuiService.SelectedCoreObject = nil
		end
	end

	return this
end

return createNoActionOverlay
