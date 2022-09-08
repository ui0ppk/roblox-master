//iPod game launching and verification on GT2 (internal build)


var target = UIATarget.localTarget();

var app = target.frontMostApp();

var window = app.mainWindow();

//Initial screen elements


var wheel = target.frontMostApp().mainWindow().scrollViews()[0].pickers()[0].wheels()[0]; //picker wheel

var loginButton = target.frontMostApp().mainWindow().scrollViews()[0].staticTexts()["Login"];

var aboutButton = target.frontMostApp().mainWindow().scrollViews()[0].buttons()[3];

var nameField = target.frontMostApp().mainWindow().scrollViews()[0].textFields()[0];

var passwordField = target.frontMostApp().mainWindow().scrollViews()[0].secureTextFields()[0];



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


//Select test environment
wheel.selectValue("http://m.gametest2.robloxlabs.com/"); //selecting test environment
UIALogger.logMessage("Test Environment: " + wheel.value()); //log current selected environment



//Normal login attempt

//Start logging
var testName = "Normal login test";
login ("mobiletest","hackproof12");

target.delay(5);

//if login screen is still active
if (target.frontMostApp().mainWindow().scrollViews()[0].staticTexts()["Login"].isValid())

	{
		UIALogger.logFail(testName); //then test failed
	}
else
	{
		UIALogger.logPass(testName); //otherwise test passed (app switched to another screen)
	}

//====

//Go to Games page
var testName = "Go to Games Page";
target.frontMostApp().mainWindow().buttons()["Games"].tap();
//Validate
target.delay(3);
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

target.delay(1);

//Launch "Sound Replication" place
target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0].links()["\nSound Replication Test\n"].tap();

//Waiting 10 sec for game to launch
target.delay(15);

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