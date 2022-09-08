var webTimeout = 4;
var loginTimeout = 5;
var gameTimeout = 10;

var target = UIATarget.localTarget();
var webView = target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0];
var app = target.frontMostApp();
var window = app.mainWindow();
var innerWindow = window.scrollViews();

//Initial screen elements
var wheel = target.frontMostApp().mainWindow().pickers()[0].wheels()[0]; //picker wheel
var loginButton = target.frontMostApp().mainWindow().scrollViews()[0].staticTexts()["Login"];
var aboutButtoniPad = target.frontMostApp().mainWindow().scrollViews()[0].buttons()[4]; //for iPad
var nameField = target.frontMostApp().mainWindow().scrollViews()[0].textFields()[0];
var passwordField = target.frontMostApp().mainWindow().scrollViews()[0].secureTextFields()[0];

function login(username, password)
{
    if (loginButton.isValid())
    {
        UIALogger.logMessage("Setting username as '" + username + "'");
        innerWindow[0].textFields()[0].setValue(username);
        UIALogger.logMessage("Setting password as '" + password + "'");
        innerWindow[0].secureTextFields()[0].setValue(password);
        loginButton.tap();
    }
}

function handleCookieConstraint(wv)
{
    if (wv.staticTexts()["The site is currently offline for maintenance and upgrades. Please check back soon!"].isValid())
    {
        wv.secureTextFields()[0].tap();
        app.keyboard().typeString("rewards");
        wv.buttons()["O"].tap();
    }
}

function verifyWebViewStaticText(textToVerify)
{
    var wv = target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0];
    handleCookieConstraint(wv);

    if (wv.staticTexts()[textToVerify].isValid())
        return true;
    else
        return false;
}

function verifyWebViewLink(textToVerify)
{
    var wv = target.frontMostApp().mainWindow().scrollViews()[0].webViews()[0];
    handleCookieConstraint(wv);

    if (wv.links()[textToVerify].isValid())
        return true;
    else
        return false;
}

function startTest(testName)
{
    UIALogger.logStart(testName);
    UIALogger.logMessage("##teamcity[testStarted name='" + testName + "']");
}

function failTest(testName, message, details)
{
    if (!testName)
    {
        UIALogger.logMessage("ERROR: Test name not provided!");
        return;
    }
    if (!details)
        details = "No additional details provided.";
    if (!message)
        message = testName + "failed for unspecified reason.";

    UIALogger.logFail(testName);
    UIALogger.logMessage("##teamcity[testFailed name='" + testName + "' message='" + message + "' details='" + details +"']");
    UIALogger.logMessage("##teamcity[testFinished name='" + testName + "']");
}

function passTest(testName)
{
    UIALogger.logPass(testName);
    UIALogger.logMessage("##teamcity[testFinished name='" + testName + "']");
}

function resetVars()
{
    target = UIATarget.localTarget();
    app = target.frontMostApp();
    window = app.mainWindow();
    webView = window.scrollViews()[0].webViews()[0];
    innerWindow = window.scrollViews();
}


//UIALogger.logMessage("Version: " + window.scrollViews()[0].staticTexts()[4].label());

var versionNumber = window.scrollViews()[0].staticTexts()[4].label();
// Start test suite for this version
UIALogger.logMessage("##teamcity[testSuiteStarted name='Smoke Test for v" + versionNumber + "']");


// Select the test environment (GT1)
wheel.selectValue("http://www.sitetest2.robloxlabs.com/");
UIALogger.logMessage("Test Environment: " + wheel.value());


var testName = "Open About Page";
startTest(testName);
aboutButtoniPad.tap();
var navBar = app.navigationBar();

if (navBar.name() == "About")
    passTest(testName);
else
    failTest(testName, "About Page failed to open.");

// Close About page
testName = "Close About Page";
startTest(testName);
navBar.buttons()["Close"].tap()
target.delay(1);

if (window.scrollViews()[0].staticTexts()["Login"].isValid())
    passTest(testName);
else
    failTest(testName, "About Page failed to close.");



/*****************************************/
/* Begin Login Test */

// Attempt to login to the game
testName = "Login Test";
startTest(testName);

login("mobiletest", "hackproof12");
target.delay(loginTimeout);

if (!window.scrollViews()[0].staticTexts()["Login"].isValid())
    passTest(testName);
else
    failTest(testName, "Game failed to log in.");

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


/*****************************************/
/* Begin Catalog Page Test */

// Open the Catalog page
testName = "Open Catalog Page";
startTest(testName);

window.buttons()["Catalog"].tap();
target.delay(5);

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();

if (!app.toolbar().buttons()["gridNavigation"].isValid())
    failTest
(testName, "Failed to open Catalog screen.");

var cnt = 0;
while (cnt <= webTimeout)
{
    if (verifyWebViewLink("Catalog"))
    {
        passTest(testName);
        break;
    }
    else
        cnt++; 
}

if (cnt >= webTimeout)
    failTest(testName, "Failed to load Catalog web page.");


// Return to grid screen
app.toolbar().buttons()["gridNavigation"].tap();

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();

/* End Catalog Page Test */
/*****************************************/
/* Begin Inventory Page Test */

// Open the Inventory page
testName = "Open Inventory Page";
startTest(testName);

window.buttons()["Inventory"].tap();
target.delay(5);

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


if (!app.toolbar().buttons()["gridNavigation"].isValid())
    failTest(testName, "Failed to open Inventory screen.");

var cnt = 0;
while (cnt <= webTimeout)
{
    if (verifyWebViewStaticText("Character Customizer"))
    {
        passTest(testName);
        break;
    }
    else
        cnt++; 
}

if (cnt >= webTimeout)
    failTest(testName, "Failed to load Inventory web page.");

// Return to grid screen
app.toolbar().buttons()["gridNavigation"].tap();

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();



/* End Inventory Page Test */
/*****************************************/
/* Begin Builders Club Page Test */

// Open the Builders Club page
testName = "Open Builders Club Page";
startTest(testName);

window.buttons()["Builders Club"].tap();
target.delay(5);

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


if (!app.toolbar().buttons()["gridNavigation"].isValid())
    failTest(testName, "Failed to open Builders Club screen.");

var cnt = 0;
while (cnt <= webTimeout)
{
    if (verifyWebViewStaticText("UPGRADE"))
    {
        passTest(testName);
        break;
    }
    else
        cnt++; 
}

if (cnt >= webTimeout)
    failTest(testName, "Failed to load Builders Club web page.");


// Return to grid screen
app.toolbar().buttons()["gridNavigation"].tap();

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();



/* End Builders Club Page Test */
/*****************************************/
/* Begin Profile Page Test */

// Open the Inventory page
testName = "Open Profile Page";
startTest(testName);

// Why is this button named 'Crossroads'?
// Good question!
window.buttons()["Crossroads"].tap();
target.delay(5);

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


if (!app.toolbar().buttons()["gridNavigation"].isValid())
    failTest(testName, "Failed to open Profile screen.");

var cnt = 0;
while (cnt <= webTimeout)
{
    if (verifyWebViewStaticText("Your Profile"))
    {
        passTest(testName);
        break;
    }
    else
        cnt++; 
}

if (cnt >= webTimeout)
    failTest(testName, "Failed to load Profile web page.");


// Return to grid screen
app.toolbar().buttons()["gridNavigation"].tap();

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();



/* End Profile Page Test */
/*****************************************/
/* Begin Messages Page Test */

// Open the Messages page
testName = "Open Messages Page";
startTest(testName);

window.buttons()["Messages"].tap();
target.delay(5);

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


if (!app.toolbar().buttons()["gridNavigation"].isValid())
    failTest(testName, "Failed to open Messages screen.");

var cnt = 0;
while (cnt <= webTimeout)
{
    if (verifyWebViewStaticText("Inbox"))
    {
        passTest(testName);
        break;
    }
    else
        cnt++; 
}

if (cnt >= webTimeout)
    failTest(testName, "Failed to load Messages web page.");


// Return to grid screen
app.toolbar().buttons()["gridNavigation"].tap();


target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();



/* End Messages Page Test */
/*****************************************/
/* Begin Groups Page Test */

// Open the Groups page
testName = "Open Groups Page";
startTest(testName);

window.buttons()["Groups"].tap();
target.delay(5);


target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


if (!app.toolbar().buttons()["gridNavigation"].isValid())
    failTest(testName, "Failed to open Groups screen.");

var cnt = 0;
while (cnt <= webTimeout)
{
    if (verifyWebViewStaticText("Only Builders Club Members can create a new group. Receive all of the benefits of Builders Club by signing up"))
    {
        passTest(testName);
        break;
    }
    else
        cnt++; 
}

if (cnt >= webTimeout)
    failTest(testName, "Failed to load Groups web page.");


// Return to grid screen
app.toolbar().buttons()["gridNavigation"].tap();


target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();



/* End Groups Page Test */
/*****************************************/
/* Begin Game Launching Test */
// We verify a game has launched by the presence of the keyboard - 
// if the launch was successful, we should be able to tap the Chat button,
// which opens the keyboard

testName = "Game Launching";
startTest(testName);

// Launch game - 92786801 is 'mobiletest's Place
window.textFields()[2].setValue("92786801");
window.buttons()["Place Launch"].tap();

cnt = 0;
while (cnt <= gameTimeout)
{
    // Tap where the Chat button should be
    window.images()[0].images()[0].tapWithOptions({tapOffset:{x:0.12, y:0.03}});

    if (app.keyboard().isValid())
    {
        passTest(testName);
        break;
    }
    else
        cnt++;
}

if (cnt >= gameTimeout)
    failTest(testName, "Game failed to launch - keyboard did not open.")

//Waiting 25 sec for game to launch
//target.delay(25);

target = UIATarget.localTarget();
app = target.frontMostApp();
window = app.mainWindow();
webView = window.scrollViews()[0].webViews()[0];
innerWindow = window.scrollViews();


//Type a verification message
target.delay(1);
target.frontMostApp().keyboard().typeString("The test is over.\n"); 
target.delay(5);






UIALogger.logMessage("##teamcity[testSuiteFinished name='Smoke Test for v" + versionNumber + "']");