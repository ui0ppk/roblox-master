--[[
			// SetAccountCredentialsScreen.lua
]]
local CoreGui = game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local ContextActionService = game:GetService('ContextActionService')
local GuiService = game:GetService('GuiService')

local AccountManager = require(Modules:FindFirstChild('AccountManager'))
local AssetManager = require(Modules:FindFirstChild('AssetManager'))
local BaseSignInScreen = require(Modules:FindFirstChild('BaseSignInScreen'))
local Errors = require(Modules:FindFirstChild('Errors'))
local ErrorOverlay = require(Modules:FindFirstChild('ErrorOverlay'))
local EventHub = require(Modules:FindFirstChild('EventHub'))
local GlobalSettings = require(Modules:FindFirstChild('GlobalSettings'))
local LoadingWidget = require(Modules:FindFirstChild('LoadingWidget'))
local ScreenManager = require(Modules:FindFirstChild('ScreenManager'))
local SoundManager = require(Modules:FindFirstChild('SoundManager'))
local Strings = require(Modules:FindFirstChild('LocalizedStrings'))
local TextBox = require(Modules:FindFirstChild('TextBox'))
local Utility = require(Modules:FindFirstChild('Utility'))
local UserData = require(Modules:FindFirstChild('UserData'))

local function createSetAccountCredentialsScreen(title, description, buttonText)
	local this = BaseSignInScreen()

	this:SetTitle(string.upper(title or ""))
	this:SetDescriptionText(description or "")
	this:SetButtonText(buttonText or "")

	local ModalOverlay = Utility.Create'Frame'
	{
		Name = "ModalOverlay";
		Size = UDim2.new(1, 0, 1, 0);
		BackgroundTransparency = GlobalSettings.ModalBackgroundTransparency;
		BackgroundColor3 = GlobalSettings.ModalBackgroundColor;
		BorderSizePixel = 0;
		ZIndex = 4;
	}

	local myUsername = nil
	local myPassword = nil

	local isUsernameValid = false
	local isPasswordValid = false

	local UserNameProcessingCount = 0
	local PasswordProcessingCount = 0

	this.UsernameObject:SetDefaultText(Strings:LocalizedString("UsernameWord").." ("..
		Strings:LocalizedString("UsernameRulePhrase")..")")
	this.UsernameObject:SetKeyboardTitle(Strings:LocalizedString("UsernameWord"))
	this.UsernameObject:SetKeyboardDescription(Strings:LocalizedString("UsernameRulePhrase"))
	local usernameChangedCn = nil

	this.PasswordObject:SetDefaultText(Strings:LocalizedString("PasswordWord").." ("..
		Strings:LocalizedString("PasswordRulePhrase")..")")
	this.PasswordObject:SetKeyboardTitle(Strings:LocalizedString("PasswordWord"))
	this.PasswordObject:SetKeyboardDescription(Strings:LocalizedString("PasswordRulePhrase"))
	this.PasswordObject:SetKeyboardType(Enum.XboxKeyBoardType.Password)
	local passwordChangedCn = nil

	local function createAndSetCredentialsAsync()
		local result = nil

		local function signInAsync()
			-- check linked account status
			result = AccountManager:HasLinkedAccountAsync()

			-- linked, set credentials
			if result == AccountManager.AuthResults.Success then
				result = AccountManager:SetRobloxCredentialsAsync(myUsername, myPassword)
			-- unlinked, create new account and set credentials
			elseif result == AccountManager.AuthResults.AccountUnlinked then
				result = AccountManager:GenerateAccountAsync(myUsername, myPassword)
			end
		end

		local loader = LoadingWidget(
			{ Parent = ModalOverlay }, { signInAsync } )

		-- set up full screen loader
		ModalOverlay.Parent = GuiRoot
		ContextActionService:BindCoreAction("BlockB", function() end, false, Enum.KeyCode.ButtonB)
		local selectedObject = GuiService.SelectedCoreObject
		GuiService.SelectedCoreObject = nil

		-- call loader
		loader:AwaitFinished()

		-- clean up
		loader:Cleanup()
		loader = nil
		GuiService.SelectedCoreObject = selectedObject
		ContextActionService:UnbindCoreAction("BlockB")
		ModalOverlay.Parent = nil

		if result == AccountManager.AuthResults.Success then
			EventHub:dispatchEvent(EventHub.Notifications["AuthenticationSuccess"])
		else
			local err = result and Errors.Authentication[result] or Errors.Default
			ScreenManager:OpenScreen(ErrorOverlay(err), false)
		end
	end

	local function validatePassword(silent)
		PasswordProcessingCount = PasswordProcessingCount + 1

		local reason = nil
		isPasswordValid, reason = AccountManager:IsValidPasswordAsync(myUsername or "", myPassword)
		if isPasswordValid then
			if myUsername and #myUsername > 0 then
				GuiService.SelectedCoreObject = this.SignInButton
			else
				GuiService.SelectedCoreObject = this.UsernameSelection
			end
		elseif isPasswordValid == false then
			if not silent then
				-- web returns long strings on password error. Lets create our own error type
				GuiService.SelectedCoreObject = this.PasswordSelection
				local err = Errors.Default;
				if reason then
					err = Errors.SignIn.InvalidPassword
					err.Msg = reason
				end
				ScreenManager:OpenScreen(ErrorOverlay(err), false)
			end
		else -- Http failure

		end

		PasswordProcessingCount = PasswordProcessingCount - 1
	end

	local function validateUsername(silent)
		UserNameProcessingCount = UserNameProcessingCount + 1

		-- 1. Check if valid user name
		local reason = nil
		isUsernameValid, reason = AccountManager:IsValidUsernameAsync(myUsername)
		if isUsernameValid then
			-- 2. if password set, need to recheck password rules
			if myPassword and #myPassword > 0 then
				validatePassword()
			else
				GuiService.SelectedCoreObject = this.PasswordSelection
			end
		elseif isUsernameValid == false then
			if not silent then
				GuiService.SelectedCoreObject = this.UsernameSelection
				-- NOTE: Web has changed username rules and the result of the ErrorMessage in the endpoint call.
				-- We key check reason vs. our current error table, if we don't find a key, we create an error out
				-- of the reason returned. If there is no reason, use default. This covers both old and new behavior
				local err = nil
				if Errors.SignIn[reason] then
					err = Errors.SignIn[reason]
				elseif reason then
					err = { Title = Strings:LocalizedString("InvalidUsernameTitle"), Msg = reason }
				else
					err = Errors.Default
				end
				ScreenManager:OpenScreen(ErrorOverlay(err), false)
			end
		else -- Http Request failed

		end

		UserNameProcessingCount = UserNameProcessingCount - 1
	end

	local function onUsernameChanged(text)
		myUsername = text
		if #myUsername > 0 then
			validateUsername()
		else
			GuiService.SelectedCoreObject = this.UsernameSelection
			isUsernameValid = false
		end
	end

	local function onPasswordChanged(text)
		myPassword = text
		if #myPassword > 0 then
			validatePassword()
		else
			GuiService.SelectedCoreObject = this.PasswordSelection
			isPasswordValid = false
		end
	end


	local isSettingCredentials = false
	this.SignInButton.MouseButton1Click:connect(function()
		if isSettingCredentials then return end
		isSettingCredentials = true

		local function stillValidatingUserInfo()
			return UserNameProcessingCount > 0 or PasswordProcessingCount > 0
		end
		local function awaitValidatingUserInfo()
			while stillValidatingUserInfo() do wait() end
		end

		SoundManager:Play('ButtonPress')


		local processingFunctions = nil
		-- Wait for current validation to finish
		if stillValidatingUserInfo() then

			processingFunctions = { awaitValidatingUserInfo }

		-- Retry our username and password validation
		elseif isUsernameValid == nil or isPasswordValid == nil then

			processingFunctions = {}
			if isUsernameValid == nil then
				table.insert(processingFunctions, function() validateUsername(true) awaitValidatingUserInfo() end)
			end
			if isPasswordValid == nil then
				table.insert(processingFunctions, function() validatePassword(true) awaitValidatingUserInfo() end)
			end

		end

		if processingFunctions then
			local processingLoader = LoadingWidget(
				{ Parent = ModalOverlay },
				processingFunctions)

			-- NOTE: may need to get a separate overlay for this spinner
			-- Also should we disable input while overlay is active?
			ModalOverlay.Parent = GuiRoot

			processingLoader:AwaitFinished()
			processingLoader:Cleanup()
			processingLoader = nil

			ModalOverlay.Parent = nil
		end

		if isUsernameValid and isPasswordValid then
			createAndSetCredentialsAsync()
		elseif isUsernameValid == false or isPasswordValid == false then
			local err = Errors.SignIn.NoUsernameOrPasswordEntered
			ScreenManager:OpenScreen(ErrorOverlay(err), false)
		else -- Http failed to validate your password or username
			local err = Errors.SignIn.ConnectionFailed
			ScreenManager:OpenScreen(ErrorOverlay(err), false)
		end
		isSettingCredentials = false
	end)


	--[[ Public API ]]--
	-- override
	local baseFocus = this.Focus
	function this:Focus()
		baseFocus(self)
		usernameChangedCn = this.UsernameObject.OnTextChanged:connect(onUsernameChanged)
		passwordChangedCn = this.PasswordObject.OnTextChanged:connect(onPasswordChanged)
	end

	-- override
	local baseRemoveFocus = this.RemoveFocus
	function this:RemoveFocus()
		baseRemoveFocus(self)
		Utility.DisconnectEvent(usernameChangedCn)
		Utility.DisconnectEvent(passwordChangedCn)
	end

	return this
end

return createSetAccountCredentialsScreen
