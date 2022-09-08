//Script for testing basic Roblox iOS App fuctionality on iPhone/iPod Touch on GT2 (CI build)


var target = UIATarget.localTarget();

var app = target.frontMostApp();

var window = app.mainWindow();


//Initial screen elements

var loginButton = target.frontMostApp().mainWindow().scrollViews()[0].staticTexts()["Login"];

var aboutButton = target.frontMostApp().mainWindow().scrollViews()[0].buttons()[3];

var nameField = target.frontMostApp().mainWindow().scrollViews()[0].textFields()[0];

var passwordField = target.frontMostApp().mainWindow().scrollViews()[0].secureTextFields()[0];
var webTimeout = 20;
var loginTimeout = 15;


//-=FUNCTIONS=-


//Login Function

function login(username, password)
	{
		if (loginButton.isValid())
		{
			UIALogger.logMessage("Setting username as '" + username + "'");
			target.frontMostApp().mainWindow().scrollViews()[0].textFields()[0].setValue(username);
			UIALogger.logMessage("Setting password as '" + password + "'");
			target.frontMostApp().mainWindow().scrollViews()[0].secureTextFields()[0].setValue(password);
			loginButton.tap()
		}
	}





//-=TESTING SCRIPT=-


//Create elements log:
target.logElementTree();


//Go to About Screen:
var testName = "About Screen test"
UIALogger.logStart(testName);
 aboutButton.tap(); //works for ipod	


//Verifying that 'About' screen is working
var barname = target.frontMostApp().navigationBar().name();

if (barname == "About")
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//returning to the main screen
var testName = "Back to main screen";
UIALogger.logStart(testName);
target.frontMostApp().navigationBar().leftButton().tap();

if (target.frontMostApp().mainWindow().scrollViews()[0].staticTexts()["Login"].isValid())

	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//Normal login attempt

//Start logging
var testName = "Normal login test";
login ("mobiletest","hackproof12");

target.delay(loginTimeout);

//if login screen is still active
if (target.frontMostApp().mainWindow().scrollViews()[0].staticTexts()["Login"].isValid())

	{
		UIALogger.logFail(testName); //then test failed
	}
else
	{
		UIALogger.logPass(testName); //otherwise test passed (app switched to another screen)
	}




//=====

//Go to Catalog page
var testName = "Go to Catalog Page";
target.frontMostApp().mainWindow().buttons()["Catalog"].tap();
//Validate
target.delay(webTimeout);
if (target.frontMostApp().toolbar().buttons()["gridNavigation"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}
//Go back to the Main Page
var testName = "Go to Main Page";
target.frontMostApp().toolbar().buttons()["gridNavigation"].tap();
//Validate
if (target.frontMostApp().mainWindow().buttons()["Games"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//====

//Go to Inventory page
var testName = "Go to Inventory Page";
target.frontMostApp().mainWindow().buttons()["Inventory"].tap();
//Validate
target.delay(webTimeout);
if (target.frontMostApp().toolbar().buttons()["gridNavigation"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}
//Go back to the Main Page
var testName = "Go to Main Page";
target.frontMostApp().toolbar().buttons()["gridNavigation"].tap();
//Validate
if (target.frontMostApp().mainWindow().buttons()["Games"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//====


//Go to BC page 
var testName = "Go to Builders Club Page";
target.frontMostApp().mainWindow().buttons()[5].tap();
//Validate
target.delay(webTimeout);
if (target.frontMostApp().toolbar().buttons()["gridNavigation"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}
//Go back to the Main Page
var testName = "Go to Main Page";
target.frontMostApp().toolbar().buttons()["gridNavigation"].tap();
//Validate
if (target.frontMostApp().mainWindow().buttons()["Games"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//====


//Go to Profile page
var testName = "Go to Profile Page";
target.frontMostApp().mainWindow().buttons()["Profile"].tap();
//Validate
target.delay(webTimeout);
if (target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].staticTexts()["My status:"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}
//Go back to the Main Page
var testName = "Go to Main Page";
target.frontMostApp().toolbar().buttons()["gridNavigation"].tap();
//Validate
if (target.frontMostApp().mainWindow().buttons()["Games"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//====


//Go to Messages page
var testName = "Go to Messages Page";
target.frontMostApp().mainWindow().buttons()["Messages"].tap();
//Validate
target.delay(webTimeout);
if (target.frontMostApp().toolbar().buttons()["gridNavigation"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}
//Go back to the Main Page
var testName = "Go to Main Page";
target.frontMostApp().toolbar().buttons()["gridNavigation"].tap();
//Validate
if (target.frontMostApp().mainWindow().buttons()["Games"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//====

//Go to Games page
var testName = "Go to Games Page";
target.frontMostApp().mainWindow().buttons()["Games"].tap();
//Validate
target.delay(webTimeout);
if (target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].staticTexts()["Featured Games"].isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//Show all games ("See more")
target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()[0].tap();

target.delay(5);

//Launch "Sound Replication" place

target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["\nSound Replication Test\n"].tap();

//Waiting 15 sec for game to launch
target.delay(25);

//Verifying that game is running (keyboard pops up after pressing chat button)
var testName = "Game launched";
target.frontMostApp().mainWindow().images()[0].images()[1].tapWithOptions({tapOffset:{x:0.23, y:0.06}}); //open chat (works!)
target.delay(1);

//Check if keyboard is active
if (target.frontMostApp().keyboard().isValid())
	{
		UIALogger.logPass(testName);
	}
else
	{
		UIALogger.logFail(testName);
	}

//Type something


target.delay(1);

target.frontMostApp().keyboard().typeString("Test is over\n"); 

target.delay(5);
