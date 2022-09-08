$('a').on('tap', function(e){
	e.preventDefault();
});

$( function() {
	// Make sure this runs first, otherwise we can't see the page outside the app
	$("#moreContainer").fadeIn(500);
	//$("#eventsContainer").fadeIn(500);

	//interface.LogMessage("inside the more page");
	var raw = interface.getInitSettings();
	var settings = JSON.parse(raw);
	//console.log("raw settings: " + settings);

	interface.fireScreenLoaded()
	// Make sure the app's action bar is reset only when coming
	// back to the More page from one of its children
	if (!settings.isFirstLaunch && !settings.reloadMore)
		interface.transitionToColor("More", "black");


	if (settings.isMobile)
	{
		$("#lnkCatalog").attr("href", settings.catalogUrl);
		$("#lnkCatalog").on('click', function(){
			interface.transitionToColor("Catalog", "green");
		});
	}

	// Updates the action bar as we move between pages
	$("#lnkProfile").attr("href", settings.profileUrl);
	$("#btnProfile").on('click', function(){
		interface.transitionToColor("Profile", "orange");
	});
	if (!settings.isMobile)
	{
		$("#lnkCharacter").attr("href", settings.characterUrl);
		$("#btnCharacter").on('click', function(){
			interface.transitionToColor("Character", "orange");
		});	
	}
	$("#lnkInventory").attr("href", settings.inventoryUrl);
	$("#btnInventory").on('click', function(){
		interface.transitionToColor("Inventory", "orange");
	});
	if (!settings.isMobile)
	{
		$("#lnkTrade").attr("href", settings.tradeUrl);
		$("#btnTrade").on('click', function(){
			interface.transitionToColor("Trade", "green");
		});
	}
	$("#lnkGroups").attr("href", settings.groupsUrl);
	$("#btnGroups").on('click', function(){
		interface.transitionToColor("Groups", "green");
	});
	if (!settings.isMobile)
	{
		$("#lnkForum").attr("href", settings.forumUrl);
		$("#btnForum").on('click', function(){
			interface.transitionToColor("Forum", "purple");
		});
	}
	$("#lnkBlog").attr("href", settings.blogUrl);
	$("#btnBlog").on('click', function(){
		interface.transitionToColor("Blog", "blue");
	});
	$("#lnkHelp").attr("href", settings.helpUrl);
	$("#btnHelp").on('click', function(){
		interface.transitionToColor("Help", "purple");
	});
	$("#lnkSettings").attr("href", "#");
	$("#btnSettings").on('click', function(){
		interface.showSettingsDialog();
	});

    if (settings.isEmailNotificationEnabled)
        $("#btnSettingsImg").attr("src", "img/icon_settings_notification.png");

	// Set up the events page
	// We aren't guaranteed to have /any/ events, so we wait to fade in the events div
	// until we know for sure
	if ("eventsData" in settings)
	{
		var events = JSON.parse(settings.eventsData);
		//console.log("Num events: " + events.length);
		//console.log("settings.eventsData = " + events[0]);
		//console.log("LogoImageURL? = " + events[0].LogoImageURL);
		//console.log("eventsData is present: " + settings.eventsData);
		parseEvents(settings.baseUrl, events, settings.isMobile, settings.useCompatibility);
	}
	else
	{
		$("#eventsContainer").hide();
		console.log("no eventsData :(");
	}
});

function parseEvents(baseUrl, events, isMobile, useCompatibility) {
	if (isMobile && events.length > 2)
	{
		var specClass = "";
		if (useCompatibility)
			specClass = " compatibility";

		var cnt = 0;
		var appendOpening = "<div class=\"longRow\">";
		var appendClosing = "</div>";
		var inner = "";
		var logStr = "";
		for (var i = 0; i < events.length; i++)
		{
			inner += "<a href=\"" + baseUrl + events[i].PageUrl + "\"><div class=\"halfBtn\"><img class=\"" + specClass + "\" src=\"" + events[i].LogoImageURL + "\" /></div></a>";
			cnt++;
			if (cnt == 2)
			{
				logStr = appendOpening + inner + appendClosing;
				//console.log("cnt: " + cnt + " | i: " + i + " | logStr: " + logStr);
				$("#eventsContainer").append(logStr);
				inner = "";
				cnt = 0;
			}
		}
		if (cnt == 1)
			$("#eventsContainer").append(appendOpening + inner + appendClosing);

		//console.log("FINAL: " + logStr);
		//console.log("innerHTML: " + $("#eventsContainer").innerHTML());
	}
	else { // on tablets, we only have 1 button per row
		var specClass = "";
		if (useCompatibility)
			specClass = "halfBtnCompat fullBtnCompat";
		else
			specClass = "halfBtn fullBtn";

		for (var i = 0; i < events.length; i++)
		{
			var row = "<div class=\"longRow\"><a href=\"" + baseUrl + events[i].PageUrl + "\"><div class=\"" + specClass + "\"><img src=\"" + events[i].LogoImageURL + "\" /></div></div></a>";
			$("#eventsContainer").append(row);	
		}
	}

	// Fade in the events after everything is loaded
	$("#eventsContainer").fadeIn(500);
}

function testFunction(obj) {
	console.log("inside testFunction, param: " + obj);
}