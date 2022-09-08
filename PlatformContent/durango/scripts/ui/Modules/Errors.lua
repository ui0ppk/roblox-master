--[[
			// Errors.lua

			// Global error codes
]]
local CoreGui = Game:GetService("CoreGui")
local GuiRoot = CoreGui:FindFirstChild("RobloxGui")
local Modules = GuiRoot:FindFirstChild("Modules")

local Strings = require(Modules:FindFirstChild('LocalizedStrings'))

local Errors =
{
	Default = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("DefaultErrorPhrase"), Code = 0 };

	GameJoin =
	{
		-- index mapped to error code returned from c++
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("AlreadyRunningPhrase"), Code = 101 };
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("WebServerConnectFailPhrase"), Code = 102 };
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("AccessDeniedByWeb"), Code = 103 };
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("InstanceNotFound"), Code = 104 };
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("GameFullPhrase"), Code = 105 };
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("FollowUserFailed"), Code = 106 };
		{ Title = Strings:LocalizedString("UnableToJoinTitle"), Msg = Strings:LocalizedString("DefaultJoinFailPhrase"), Code = 107 };
	};

	Vote =
	{
		FloodCheckThresholdMet = { Title = Strings:LocalizedString("CannotVoteTitle"), Msg = Strings:LocalizedString("VoteFloodPhrase"), Code = 201 };
		PlayGame = { Title = Strings:LocalizedString("CannotVoteTitle"), Msg = Strings:LocalizedString("VotePlayGamePhrase"), Code = 202 };
	};

	Favorite =
	{
		Failed = { Title = Strings:LocalizedString("CannotFavoriteTitle"), Msg = Strings:LocalizedString("DefaultErrorPhrase"), Code = 301 };
		FloodCheck = { Title = Strings:LocalizedString("CannotFavoriteTitle"), Msg = Strings:LocalizedString("FavoriteFloodPhrase"), Code = 302 };
	};

	Test =
	{
		CannotJoinGame = { Title = "An Error Occured", Msg = "Cannot join games from studio.", Code = 401 };
		StillInDev = { Title = "An Error Occured", Msg = "This feature is still in development.", Code = 402 };
		FeatureNotAvailableInStudio = { Title =  "An Error Occured", Msg = "This feature is not available in Roblox Studio.", Code = 403 };
	};

	PackageEquip =
	{
		Default = { Title = Strings:LocalizedString("UnableToEquipTitle"), Msg = Strings:LocalizedString("UnableToEquipPhrase"), Code = 501 };
	};

	OutfitEquip =
	{
		Default = { Title = Strings:LocalizedString("UnableToWearOufitTitle"), Msg = Strings:LocalizedString("UnableToWearOufitPhrase"), Code = 601 };
	};

	PackagePurchase =
	{
		{ Title = Strings:LocalizedString("UnableToDoPurchaseTitle"), Msg = Strings:LocalizedString("UnableToDoPurchasePhrase"), Code = 701 };
	};

	RobuxPurchase =
	{
		{ Title = Strings:LocalizedString("UnableToDoRobuxPurchaseTitle"), Msg = Strings:LocalizedString("UnableToDoRobuxPurchasePhrase"), Code = 801 };
	};

	Authentication =
	{
		-- index mapped to int error code from c++
		[-1] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("AuthErrorPhrase"), Code = 901 };
		-- ["0"]; This is success
		[1] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("AuthInProgressPhrase"), Code = 902 };
		[2] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("AuthAccountUnlinkedPhrase"), Code = 903 };
		[3] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("AuthMissingGamePadPhrase"), Code = 904 };
		[4] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("AuthNoUserDetectedPhrase"), Code = 905 };
		[5] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("AuthHttpErrorDetected"), Code = 906 };
		[6] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkSignUpDisabled"), Code = 907 };
		[7] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkFlooded"), Code = 908 };
		[8] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkLeaseLocked"), Code = 909 };
		[9] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkAccountLinkingDisabled"), Code = 910 };
		[10] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkInvalidRobloxUser"), Code = 911 };
		[11] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkRobloxUserAlreadyLinked"), Code = 912 };
		[12] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkXboxUserAlreadyLinked"), Code = 913 };
		[13] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkIllegalChildAccountLinking"), Code = 914 };
		[14] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkInvalidPassword"), Code = 915 };
		[15] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkUsernamePasswordNotSet"), Code = 916 };
		[16] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkUsernameAlreadyTaken"), Code = 917 };
		[17] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("LinkInvalidCredentials"), Code = 918 };
	};

	Reauthentication =
	{
		-- index mapped to int error code from c++, you must index into this with a string
		[0] = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("ReauthUnknownPhrase"), Code = 1001 };
		[1] = { Title = Strings:LocalizedString("ReauthSignedOutTitle"), Msg = Strings:LocalizedString("ReauthSignedOutPhrase"), Code = 1002 };
		[2] = { Title = Strings:LocalizedString("ReauthRemovedTitle"), Msg = Strings:LocalizedString("ReauthRemovedPhrase"), Code = 1003 };
		[3] = { Title = Strings:LocalizedString("ReauthSignedOutTitle"), Msg = Strings:LocalizedString("ReauthInvalidSessionPhrase"), Code = 1004 };
		[4] = { Title = Strings:LocalizedString("ReauthUnlinkTitle"), Msg = Strings:LocalizedString("ReauthUnlinkPhrase"), Code = 1005 };
		[5] = { Title = Strings:LocalizedString("ReauthRemovedTitle"), Msg = Strings:LocalizedString("ReauthRemovedPhrase"), Code = 1006 };
		[6] = { Title = Strings:LocalizedString("ReauthRemovedTitle"), Msg = Strings:LocalizedString("ReauthRemovedPhrase"), Code = 1007 };
		[7] = { Title = Strings:LocalizedString("ReauthRemovedTitle"), Msg = Strings:LocalizedString("ReauthRemovedPhrase"), Code = 1008 };
	};

	SignIn =
	{
		["Invalid Username"] = { Title = Strings:LocalizedString("InvalidUsernameTitle"), Msg = Strings:LocalizedString("InvalidUsernamePhrase"), Code = 1101 };
		InvalidPassword = { Title = Strings:LocalizedString("InvalidPasswordTitle"), Msg = Strings:LocalizedString("InvalidPasswordPhrase"), Code = 1102 };
		["Already Taken"] = { Title = Strings:LocalizedString("AlreadyTakenTitle"), Msg = Strings:LocalizedString("AlreadyTakenPhrase"), Code = 1103 };
		["Invalid Characters Used"] = { Title = Strings:LocalizedString("InvalidUsernameTitle"), Msg = Strings:LocalizedString("InvalidCharactersUsedPhrase"), Code = 1104 };
		["Username Cannot Contain Spaces"] = { Title = Strings:LocalizedString("InvalidUsernameTitle"), Msg = Strings:LocalizedString("UsernameCannotContainSpacesPhrase"), Code = 1105 };
		NoUsernameEntered = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("NoUsernameEnteredPhrase"), Code = 1106 };
		NoUsernameOrPasswordEntered = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("NoUsernameOrPasswordEnteredPhrase"), Code = 1107 };
		["ConnectionFailed"] = { Title = Strings:LocalizedString("AuthenticationErrorTitle"), Msg = Strings:LocalizedString("WebServerConnectFailPhrase"), Code = 1108 };
	};

	PlatformError =
	{
		PopupPartyUI = { Title = Strings:LocalizedString("ErrorOccurredTitle"), Msg = Strings:LocalizedString("PopupPartyUIErrorPhrase"), Code = 1201 };
	};
}

return Errors
