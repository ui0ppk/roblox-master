--[[
			// ReportOverlay.lua
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")
local GuiService = game:GetService('GuiService')
local ContextActionService = game:GetService("ContextActionService")

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))
local BaseOverlay = require(Modules:FindFirstChild('BaseOverlay'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Http = require(Modules:FindFirstChild('Http'))

local ReportOverlay = {}

ReportOverlay.ReportType = {
	REPORT_GAME = 0;
}

local REPORT_COMMENT = "Game reported from the Xbox App.";

function ReportOverlay:CreateReportOverlay(reportType, assetId)
	local this = BaseOverlay()

	local submitButton = Utility.Create'TextButton'
	{
		Name = "SubmitButton";
		Size = UDim2.new(0, 320, 0, 66);
		Position = UDim2.new(0, 776, 1, -100 - 66);
		BorderSizePixel = 0;
		BackgroundColor3 = GlobalSettings.BlueButtonColor;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextColor3 = GlobalSettings.TextSelectedColor;
		Text = string.upper(Strings:LocalizedString("SubmitWord"));
		ZIndex = this.BaseZIndex;
		Parent = this.Container;

		SoundManager:CreateSound('MoveSelection');
	}
	local titleText = Utility.Create'TextLabel'
	{
		Name = "TitleText";
		Size = UDim2.new(0, 0, 0, 0);
		Position = UDim2.new(0, submitButton.Position.X.Offset, 0, 136);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.HeaderSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = Strings:LocalizedString("ReportGameWord");
		TextXAlignment = Enum.TextXAlignment.Left;
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}
	local descriptionText = Utility.Create'TextLabel'
	{
		Name = "DescriptionText";
		Size = UDim2.new(0, 762, 0, 304);
		Position = UDim2.new(0, titleText.Position.X.Offset, 0, titleText.Position.Y.Offset + 62);
		BackgroundTransparency = 1;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextYAlignment = Enum.TextYAlignment.Top;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.TitleSize;
		TextColor3 = GlobalSettings.WhiteTextColor;
		TextWrapped = true;
		Text = Strings:LocalizedString("ReportPhrase");
		ZIndex = this.BaseZIndex;
		Parent = this.Container;
	}

	local reportIcon = Utility.Create'ImageLabel'
	{
		Name = "ReportIcon";
		Position = UDim2.new(0, 226, 0, 204);
		BackgroundTransparency = 1;
		ZIndex = this.BaseZIndex;
	}
	AssetManager.LocalImage(reportIcon, 'rbxasset://textures/ui/Shell/Icons/ErrorIconLargeCopy',
		{['720'] = UDim2.new(0,214,0,176); ['1080'] = UDim2.new(0,321,0,264);})
	this:SetImage(reportIcon)

	submitButton.MouseButton1Click:connect(function()
		if this:Close() then
			if assetId then
				spawn(function()
					local result = Http.ReportAbuseAsync("Asset", assetId, 7, REPORT_COMMENT)
				end)
			end
		end
	end)

	function this:GetPriority()
		return GlobalSettings.ElevatedPriority
	end

	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(this)
		GuiService.SelectedCoreObject = submitButton
	end

	return this
end

return ReportOverlay
