--[[
			// BaseScreen.lua

			// Creates a base screen with breadcrumbs and title. Do not use for a pane/tab
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local ContextActionService = game:GetService("ContextActionService")
local GuiService = game:GetService('GuiService')

local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local Utility = require(Modules:FindFirstChild('Utility'))

local function createBaseScreen()
	local this = {}

	local defaultSelectionObject = nil
	local lastParent = nil

	local container = Utility.Create'Frame'
	{
		Name = "Container";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = 1;
	}
	local backImage = Utility.Create'ImageLabel'
	{
		Name = "BackImage";
		BackgroundTransparency = 1;
		Parent = container;
	}
	AssetManager.LocalImage(backImage,
		'rbxasset://textures/ui/Shell/Icons/BackIcon', {['720'] = UDim2.new(0,32,0,32); ['1080'] = UDim2.new(0,48,0,48);})
	local backText = Utility.Create'TextLabel'
	{
		Name = "BackText";
		Size = UDim2.new(0, 0, 0, backImage.Size.Y.Offset);
		Position = UDim2.new(0, backImage.Size.X.Offset + 8, 0, 0);
		BackgroundTransparency = 1;
		Font = GlobalSettings.RegularFont;
		FontSize = GlobalSettings.ButtonSize;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = "";
		Parent = container
	}
	local titleText = Utility.Create'TextLabel'
	{
		Name = "TitleText";
		Size = UDim2.new(0, 0, 0, 35);
		Position = UDim2.new(0, 16, 0, backImage.Size.Y.Offset + 74);
		BackgroundTransparency = 1;
		Font = GlobalSettings.LightFont;
		FontSize = GlobalSettings.HeaderSize;
		TextXAlignment = Enum.TextXAlignment.Left;
		TextColor3 = GlobalSettings.WhiteTextColor;
		Text = "";
		Parent = container;
	}

	--[[ Public API ]]--
	this.Container = container

	function this:SetTitle(newTitle)
		titleText.Text = newTitle
	end
	function this:SetBackText(newText)
		backText.Text = newText
	end

	function this:GetDefaultSelectionObject()
		return defaultSelectionObject
	end

	function this:Destroy()
		self.Container:Destroy()
		self = nil
	end

	--[[ Public API - Screen Management ]]--
	function this:SetPosition(newPosition)
		self.Container.Position = newPosition
	end
	function this:SetParent(newParent)
		lastParent = newParent
		self.Container.Parent = newParent
	end

	function this:GetName()
		return titleText.Text
	end

	function this:Show()
		local prevScreen = ScreenManager:GetScreenBelow(self)
		if prevScreen and prevScreen.GetName then
			self:SetBackText(prevScreen:GetName())
		else
			self:SetBackText(string.upper(Strings:LocalizedString("BackWord")))
		end

		self.Container.Parent = lastParent
		self.TransisitionTweens = ScreenManager:DefaultFadeIn(self.Container)
		ScreenManager:PlayDefaultOpenSound()
	end
	function this:Hide()
		self.Container.Parent = nil
		ScreenManager:DefaultCancelFade(self.TransisitionTweens)
		self.TransisitionTweens = nil
	end
	function this:Focus()
		if self.SavedSelectedObject and self.SavedSelectedObject:IsDescendantOf(self.Container) then
			GuiService.SelectedCoreObject = self.SavedSelectedObject
		else
			GuiService.SelectedCoreObject = self:GetDefaultSelectionObject()
		end

		ContextActionService:BindCoreAction("ReturnFromScreen",
			function(actionName, inputState, inputObject)
				if inputState == Enum.UserInputState.End then
					ScreenManager:CloseCurrent()
					self:Destroy()
				end
			end,
			false, Enum.KeyCode.ButtonB)
	end
	function this:RemoveFocus()
		local selectedObject = GuiService.SelectedCoreObject
		if selectedObject and selectedObject:IsDescendantOf(self.Container) then
			self.SavedSelectedObject = selectedObject
			GuiService.SelectedCoreObject = nil
		end
		ContextActionService:UnbindCoreAction("ReturnFromScreen")
	end

	return this
end

return createBaseScreen
