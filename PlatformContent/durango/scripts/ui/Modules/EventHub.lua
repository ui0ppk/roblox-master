-- Written by Kyler Mulherin, Copyright ROBLOX 2015
local listeners = {}
--listeners is a table that holds arrays of listener objects
--Ex - listeners["login"] = { Listener , Listener, Listener }

local function createListener(idString, callbackFunction)
	local Listener = {  id = idString , callback = callbackFunction };
	return Listener;
end

--Initialize all the functions for the EventHub
local EventHub = {}
do
	function EventHub:addEventListener(eventString, objectIDString, callbackFunction)
		--print ('Adding Listener with ID : ' .. objectIDString)
		if (listeners[eventString] == nil) then
			listeners[eventString] = {}
		end

		table.insert(listeners[eventString], createListener(objectIDString, callbackFunction))
	end
	function EventHub:removeEventListener(eventString, objectIDString)
		--print ('Removing Listener with ID : ' .. objectIDString)
		if (listeners[eventString] == nil) then return end

		--iterate through the listeners for an event string, remove all of the listeners with the provided objectIDString
		for key, value in ipairs(listeners[eventString]) do
			local listener = value
			if (listener ~= nil) then
				if (listener.id == objectIDString) then
					--print ('-Removing listener with id : ' .. listener.id)
					table.remove(listeners[eventString], key)
				end
			end
		end
	end
	function EventHub:removeCallbackFromEvent(eventString, objectIDString, callbackFunction)
		--NOTE- Will not work with anonymous functions
		--print ('Removing Listener with ID : ' .. objectIDString)
		if (listeners[eventString] == nil) then return end

		--iterate through the listeners for an event string, remove the one with the provided objectIDString and callback function
		for key, value in ipairs(listeners[eventString]) do
			local listener = value
			if (listener ~= nil) then
				if (listener.id == objectIDString) and (listener.callback == callbackFunction) then
					table.remove(listeners[eventString], key)
					break
				end
			end
		end
	end
	function EventHub:dispatchEvent(eventString, ...)
		--print ('Dispatching Event : ' .. eventString .. ' with data : ' .. tostring(data))
		if (listeners[eventString] == nil) then
			return
		end

		--loop through all the listeners and call the callback function
		for key, value in ipairs(listeners[eventString]) do
			value.callback(...)
		end
	end


	--A comprehensive list of notification strings to read from
	EventHub.Notifications = {
		-- Authentication
		AuthenticationSuccess = "rbxNotificationAuthenticationSuccess";
		GameJoin = "rbxNotificationGameJoin";

		-- Game Notifications
		OpenGames = 		"rbxNotificationOpenGames";
		OpenGameDetail = 	"rbxNotificationOpenGameDetail";
		OpenGameGenre = 	"rbxNotificationOpenGameGenre";
		OpenBadgeScreen = 	"rbxNotificationOpenBadgeScreen";

		-- Unlink Account Notification
		UnlinkAccountConfirmation = "rbxNotificationUnlinkAccountConfirmation";

		-- Overscan Notifications
		OpenOverscanScreen = "rbxNotificationOpenOverscanScreen";

		-- Engagement Screen Notifcations
		OpenEngagementScreen = "rbxNotificationOpenEngagementScreen";

		--Social Notifications
		OpenSocialScreen = "rbxNotificationOpenSocialScreen";

		--Settings Notifications
		OpenSettingsScreen = "rbxNotificationOpenSettingsScreen";

		--Avatar Screen Notifications
		NavigateToEquippedAvatar = "rbxNotificationNavigateToEquippedAvatar";

		-- Robux Screen Notification
		NavigateToRobuxScreen = "rbxNotificationNavigateToRobuxScreen";
		RobuxCatalogPurchaseInitiated = "rbxRobuxCatalogPurchaseInitiated";
		--

		-- Achievement Related Events
		TestXButtonPressed = "rbxTestXButtonPressed";
		DonnedDifferentPackage = "rbxDonnedDifferentPackage";
		VotedOnPlace = "rbxVotedOnPlace";

		-- Hero Stats Related Events
		AvatarEquipped = "rbxAvatarEquipped";
		-- JoinedParty = "rbxJoinedParty";

		AvatarEquipBegin = "rbxAvatarEquipBegin";

		DonnedDifferentOutfit = "rbxDonnedDifferentOutfit";
	};

end


return EventHub;
--print 'Event Hub initialized'


-- [[ TESTING STUFF - include somewhere the EventHub has been initialized ]]--
--[[
	-- TEST #1 -- removing entire listeners from events
	print '\nAdding Event Listeners...';
	EventHub:addEventListener( "test0", "testID1", function (data) print('Test 1 Received test0 : ' .. data) end);
	EventHub:addEventListener( "test0", "testID2", function (data) print('Test 2 Received test0 : ' .. data) end);
	EventHub:addEventListener( "test0", "testID3", function (data) print('Test 3 Received test0 : ' .. data) end);
	EventHub:addEventListener( "test0", "testID4", function (data) print('Test 4 Received test0 : ' .. data) end);

	print '\nDispatching Events...';
	EventHub:dispatchEvent("test0", "var");
	--SHOULD SEE ALL 4 LISTENERS REPORT

	print '\nDispatching Events...';
	EventHub:dispatchEvent("test0", "var2");

	print '\nRemoving TestID2 and 3s Listeners...';
	EventHub:removeEventListener("test0", "testID2");
	EventHub:removeEventListener("test0", "testID3");

	print '\nDispatching Events...';
	EventHub:dispatchEvent("test0", "var3");
	--SHOULD ONLY SEE testID1 AND testID4 REPORT






	-- TEST #2  -- selective removal of listeners
	print '\nAdding Generic Function Event Listeners...';
	local function test1(data) print('Generic function 1. Received test1 : ' .. data) end;
	local function test2(data) print('Generic function 2. Received test1 : ' .. data) end;
	local testFunc1 = test1;
	local testFunc2 = test2;

	EventHub:addEventListener( "test1", "testID1", testFunc1);
	EventHub:addEventListener( "test1", "testID1", testFunc2);
	EventHub:addEventListener( "test1", "testID2", testFunc1);

	print '\nDispatching Events...';
	EventHub:dispatchEvent("test1", "foo");
	--SHOULD SEE 3 REPORTS

	print '\nRemoving Generic Function 1 Callback of testID1';
	EventHub:removeCallbackFromEvent("test1", "testID1", testFunc1);

	print '\nDispatching Events...';
	EventHub:dispatchEvent("test1", "foo3");
	--SHOULD SEE THAT testID1 NO LONGER REPORTS FROM GENERIC FUNCTION 1


]]--
