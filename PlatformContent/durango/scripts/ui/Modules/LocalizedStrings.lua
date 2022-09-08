-- Written by Kip Turner, Copyright ROBLOX 2015


local enUS =
{
	["HomeWord"] = "Home";
	["GameWord"] = "Games";
	["FriendsWord"] = "Friends";
	["CatalogWord"] = "ROBUX";
	["AvatarWord"] = "Avatar";
	["PlayWord"] = "Play";
	["FavoriteWord"] = "Favorite";
	["FavoritedWord"] = "Favorited";
	["LikedWord"] = "Liked";
	["DislikedWord"] = "Disliked";
	["BackWord"] = "Back";
	["SettingsWord"] = "Settings";
	["AccountWord"] = "Account";
	["OverscanWord"] = "Adjust Screen";
	["HelpWord"] = "Help";
	["SwitchProfileWord"] = "Switch Profile";

	["SearchWord"] = "Search";

	["EditAvatarPhrase"] = "Edit Avatar";
	["FriendActivityWord"] = "Friend Activity";
	["OnlineFriendsWords"] = "Online Friends";
	["RecentlyPlayedWithSortTitle"] = "Recently Played With";
	["StartPartyPhrase"] = "Snap Party App";

	["RecommendedSortTitle"] = "Recommended";
	["FavoritesSortTitle"] = "Favorites";
	["RecentlyPlayedSortTitle"] = "Recently Played";
	["MyRecentTitle"] = "My Recent";
	["TopEarningTitle"] = "Top Earning";
	["TopRatedTitle"] = "Top Rated";
	["PopularTitle"] = "Popular";
	["FeaturedTitle"] = "Featured";
	["MoreGamesPhrase"] = "More Games";

	["NoFriendsOnlinePhrase"] = "Your friends are not online";
	["JoinGameWord"] = "Join Game";
	["ViewGameDetailsWord"] = "View Game Details";
	["InviteToPartyWord"] = "Invite To Party";
	["ViewGamerCardWord"] = "View Gamer Card";

	["RatingDescriptionTitle"] = "Rating and Description";
	["GameImagesTitle"] = "Game Images";
	["GameBadgesTitle"] = "Game Badges";
	["FriendActivityTitle"] = "Friend Activity";
	["RelatedGamesTitle"] = "Related Games";
	["MoreDetailsTitle"] = "More Details";
	["GameBadgesTitle"] = "Game Badges";

	["SelectYourAvatarTitle"] = "Select Your Avatar";

	["LastUpdatedWord"] = "Last Updated";
	["CreationDateWord"] = "Creation Date";
	["CreatedByWord"] = "Created By";
	["ReportGameWord"] = "Report Game";
	["HaveBadgeWord"] = "You Have This Badge";
	["MaxPlayersWord"] = "Max Players";

	["ScreenSizeWord"] = "Screen Size";
	["ResizeScreenPrompt"] = "Use the right stick to adjust the edges of the white box until it is just off the screen";
	["ResizeScreenInputHint"] = "Resize Screen";
	["AcceptWord"] = "Accept";
	["ResetWord"] = "Reset";
	["CannotVoteWord"] = "You must play this game before rating";
	["FirstToRateWord"] = "Be the first to rate this game";
	["CustomAvatarPhrase"] = "This game uses custom Avatars";

	["EngagementScreenHint"] = "Press Any Button To Begin";

	["EquipWord"] = "Swap Avatar";
	["BuyWord"] = "Buy";
	["OkWord"] = "Ok";
	["FreeWord"] = "Free";
	["TakeWord"] = "Take";
	['GetRobuxPhrase'] = 'Get ROBUX';
	["AlreadyOwnedPhrase"] = "You already own this";
	["PurchasedThisPhrase"] = 'You purchased this for %s ROBUX';
	["RobuxBalanceTitle"] = 'My Balance';

	["RobuxBalanceOverlayTitle"] = "ROBUX Balance";
	["RobuxBalanceOverlayPhrase"] = "Only ROBUX purchased from the Xbox Store may be used on Xbox.";
	["PlatformBalanceTitle"] = "Available on Xbox:";
	["TotalBalanceTitle"] = "Total Balance:";

	["AvatarCatalogTitle"] = 'Catalog';
	["AvatarOutfitsTitle"] = 'My Collection';


	["PurchasingTitle"] = 'Purchasing...';
	["ConfirmPurchaseTitle"] = 'Confirm Purchase';
	["AreYouSurePhrase"] = 'Are you sure you want to buy "%s"?';
	["AreYouSureTakePhrase"] = 'Are you sure you want to take "%s"?';
	["AreYouSureWithPricePhrase"] = 'Are you sure you want to buy "%s" for %s?';
	["RemainingBalancePhrase"] = 'Your remaining balance will be %s ROBUX';
	["ConfirmWord"] = 'Confirm';
	["DeclineWord"] = 'Decline';

	["OnlineWord"] = "Online";
	["OfflineWord"] = "Offline";

	["SubmitWord"] = "Submit";
	["ReportPhrase"] = "You can send a report to our moderation team. We will review the game and take appropriate action.";


	["CurrencySymbol"] = "$";
	["RobuxStoreDescription"] = "Get ROBUX to buy great new looks for your Avatar, plus perks and abilities in games.";
	["RobuxStoreNoItemsPhrase"] = "There are no items for purchase right now, please try again later";
	["PercentMoreRobuxPhrase"] = "%s%% More";
	--["RobuxStoreError"] = "ROBUX items are inaccessable at this time.";

	["NoFriendsPhrase"] = "Your friends are not online. Play some games and make new friends!";

	-- Platform Service Errors
	["PopupPartyUIErrorPhrase"] = "There was an error trying to start a party. Please try again.";

	-- Auth Errors
	["AuthenticationErrorTitle"] = "Authentication Error";
	["AuthInProgressPhrase"] = "Authentication already in progress";
	["AuthAccountUnlinkedPhrase"] = "";	-- This is a special case handled by EngagementScreen
	["AuthMissingGamePadPhrase"] = "No gamepad detected. Please turn on a gamepad.";
	["AuthNoUserDetectedPhrase"] = "No Xbox Live user detected. Please sign in to an Xbox Live account.";
	["AuthHttpErrorDetected"] = "Trouble communicating with ROBLOX servers. Please check www.roblox.com/help/xbox for more info.";
	["AuthErrorPhrase"] = "Trouble communicating with ROBLOX servers. Please try again.";

	-- Reauth (Booted to engagement screen)
	["ReauthSignedOutTitle"] = "Signed Out";
	["ReauthSignedOutPhrase"] = "You have signed out of your Xbox Live account. Please sign in to continue.";
	["ReauthRemovedTitle"] = "User Changed";
	["ReauthRemovedPhrase"] = "We have detected a change in the active user. Please sign in again.";
	["ReauthInvalidSessionPhrase"] = "You have been signed out of all current ROBLOX sessons.";
	["ReauthUnlinkTitle"] = "Account Unlinked";
	["ReauthUnlinkPhrase"] = "You have successfully unlinked from your ROBLOX account. You can now choose to sign in again, or sign up for a new ROBLOX account.";
	["ReauthUnknownPhrase"] = "An error occurred and you have been signed out. Please sign in again to continue playing.";

	-- Sign In
	["NewUserPhrase"] = "Community Created Gaming. Limitless Possibilities.";
	["SignInPhrase"] = "Sign In";
	["PlayAsPhrase"] = "Sign Up Using %s";
	["UsernameWord"] = "Username";
	["PasswordWord"] = "Password";
	["UsernameRulePhrase"] = "3-20 characters, no spaces";
	["PasswordRulePhrase"] = "6 letters and 2 numbers minimum";
	["AccountSettingsTitle"] = "Account Settings";
	["PlatformLinkInfoTitle"] = "Welcome to ROBLOX!";
	["PlatformLinkInfoMessage"] = "We have created a new ROBLOX account for you. All game progress and purchases will be saved to this ROBLOX account. Now you can sign in on any platform and continue where you left off!";

	-- Sign In/Up Errors
	["InvalidUsernameTitle"] = "Invalid Username";
	["InvalidPasswordTitle"] = "Invalid Password";
	["InvalidUsernamePhrase"] = "Usernames must have 3-20 characters.";
	["InvalidPasswordPhrase"] = "Passwords must have at least 6 letters and 2 numbers";
	["AlreadyTakenTitle"] = "Username Taken";
	["AlreadyTakenPhrase"] = "That username is already taken, please try another.";
	["InvalidCharactersUsedPhrase"] = "The username you entered contains invalid characters. Only letters and numbers are allowed.";
	["UsernameCannotContainSpacesPhrase"] = "The username you entered contains spaces. Only letters and numbers are allowed.";
	["NoUsernameEnteredPhrase"] = "Username is required.";
	["NoUsernameOrPasswordEnteredPhrase"] = "You must enter a valid username and password to set up your ROBLOX account.";
	["LinkedAsPhrase"] = "%s is currently linked to your ROBLOX account, %s.";

	-- Linking Errors
	["LinkSignUpDisabled"] = "Sign up is currently disabled. Please try again later.";
	["LinkFlooded"] = "You have signed up too many times today. Please sign in with an existing ROBLOX account or try again later.";
	["LinkLeaseLocked"] = "Transaction in progress. Please wait.";
	["LinkAccountLinkingDisabled"] = "Account linking is currently disabled. Please try again later.";
	["LinkInvalidRobloxUser"] = "The ROBLOX username you entered is invalid. Please enter a valid ROBLOX username.";
	["LinkRobloxUserAlreadyLinked"] = "Your Xbox Live account is already linked to a ROBLOX account";
	["LinkXboxUserAlreadyLinked"] = "Your Xbox Live account is already linked to a ROBLOX account.";
	["LinkIllegalChildAccountLinking"] = "The accounts could not be linked. Please review your Xbox age settings.";
	["LinkInvalidPassword"] = "The password you entered is invalid. Please enter the correct password.";
	["LinkUsernamePasswordNotSet"] = "Username or password is empty. Please enter a username and password and try again.";
	["LinkUsernameAlreadyTaken"] = "That username is already taken. Please choose another username and try again.";
	["LinkInvalidCredentials"] = "The username or password is invalid. Please enter a valid username and password and try again.";
	["LinkUnknownError"] = "An unknown error has occurred. Please try again.";

	-- Linking
	["LinkAccountTitle"] = "Sign in to ROBLOX";
	["LinkAccountPhrase"] = "Sign in with an existing ROBLOX account to access your Avatar appearance and save game progress.\n\nYour ROBLOX account will be linked to your Xbox Live account, and the next time you play you will sign in automatically!\n\nYou can unlink from the Settings screen at any time.";

	-- Sign Up
	["SignUpWord"] = "Sign up";
	["SignUpTitle"] = "Create a ROBLOX Account";
	["SignUpPhrase"] = "A ROBLOX account is your access to ROBLOX on every platform. Simply set a username and password and you can sign in anywhere and continue where you left off!\n\nYour new ROBLOX account will be linked to your Xbox Live account, and the next time you play you will sign in automatically!\n\nYou can unlink from the Settings screen at any time.";

	-- Edge Case - have linked account but no credentials
	["SetCredentialsTitle"] = "Assign Username and Password";
	["SetCredentialsPhrase"] = "Looks like you were interrupted before you could finish signing up! Don't worry, you can finish setting up your ROBLOX account by choosing a username and password here.\n\nYour ROBLOX account will be linked to your Xbox Live account, and the next time you play you will sign in automatically!\n\nYou can unlink from the Settings screen at any time.";
	["SetCredentialsWord"] = "Assign";

	-- Unlink
	["UnlinkTitle"] = "Unlink Account";
	["UnlinkGamerTagPhrase"] = "Unlink %s";
	["UnlinkPhrase"] = "Are you sure you want to unlink this ROBLOX account from your Xbox account? Your save data and purchases are associated with this ROBLOX account and will be inaccessible until you sign back in!";

	-- Error Strings
	["UnableToJoinTitle"] = "Unable to Join";
	["ErrorOccurredTitle"] = "An Error Occurred";

	["DefaultErrorPhrase"] = "Could not connect to ROBLOX. Please try again later.";

	["DefaultJoinFailPhrase"]  = "Could not connect to ROBLOX. Please try again later.";
	["AlreadyRunningPhrase"] = "You are already in this ROBLOX game.";
	["WebServerConnectFailPhrase"] = "Could not connect to ROBLOX servers. Please try again later.";
	["AccessDeniedByWeb"] = "The ROBLOX game you are trying to join is currently not available.";
	["InstanceNotFound"] = "The ROBLOX game you are trying to join is currently not available.";
	["GameFullPhrase"] = "The ROBLOX game you are trying to join is currently full.";
	["FollowUserFailed"] = "The ROBLOX game you are trying to join is currently not available.";

	["UnableToEquipTitle"] = "Unable to Select";
	["UnableToEquipPhrase"] = "We were unable to select that Avatar. Please try again later.";

	["UnableToWearOufitTitle"] = "Unable to Select";
	["UnableToWearOufitPhrase"] = "We were unable to select that Outfit. Please try again later.";

	["UnableToDoPurchaseTitle"] = "Unable to Complete Purchase";
	["UnableToDoPurchasePhrase"] = "We were unable to complete your purchase. Please try again later.";

	["UnableToDoRobuxPurchaseTitle"] = "Unable to Complete Purchase";
	["UnableToDoRobuxPurchasePhrase"] = "We were unable to complete your purchase. Please try again later.";

	["CannotVoteTitle"] = "Cannot Vote";
	["VoteFloodPhrase"] = "You're voting too often. Come back later and try again.";
	["VotePlayGamePhrase"] = "You must play this game before voting.";

	["CannotFavoriteTitle"] = "Cannot Favorite Game";
	["FavoriteFloodPhrase"] = "You're favoriting games too often. Come back later and try again.";

	-- Controller connection errors
	["ControllerLostConnectionTitle"] = "Missing Controller";
	["ControllerLostConnectionPhrase"] = "Controller for user '%s' has been disconnected, please press 'A' on the controller you would like to continue with.";

	["ActiveUserLostConnectionTitle"] = "Active User Removed";
	["ActiveUserLostConnectionPhrase"] = "User '%s' has been logged out, please press 'A' on the controller you would like to continue with.";

	-- Terms of Service & Privacy
	["ToSPhrase"] = "View Terms...";
	["ToSInfoLinkPhrase"] = "Terms & Privacy Policy:\nwww.roblox.com/info/terms-of-service\nwww.roblox.com/info/Privacy.aspx";
	["PrivacyPhrase"] = "Privacy";

	-- Codes
	["VersionIdString"] = "Version: %s.%s.%s.%s";
	["ErrorMessageAndCodePrase"] = "%s\nError Code: %d";

	-- Play My Place
	["PlayMyPlaceMoreGamesTitle"] = "My Games";
	["PlayMyPlaceMoreGamesPhrase"] = "Sign in on ROBLOX.com to create more games and edit your existing creations!";
	["PrivateSessionPhrase"] = "In Private Game";
}




local this = {}


function this:GetLocale()
	return enUS
end

function this:LocalizedString(stringKey)
	local locale = self:GetLocale()
	local result = locale and locale[stringKey]
	if not result then
		print("LocalizedString: Could not find string for:" , stringKey , "using locale:" , locale)
		result = stringKey
	end
	return result
end


return this
