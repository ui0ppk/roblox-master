var webTimeout = 2;
var loginTimeout = 15;

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
        loginButton.tap()
    }
}

function verifyWebViewStaticText(webView, textToVerify)
{
    if (webView.staticTexts()[textToVerify].isValid())
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




// Select the test environment (GT1)
wheel.selectValue("http://www.gametest1.robloxlabs.com/");
UIALogger.logMessage("Test Environment: " + wheel.value());

startTest("Open About Page");
aboutButtoniPad.tap();
var navBar = app.navigationBar();

if (navBar.name() == "About")
    passTest("Open About Page");
else
    failTest("About Page", "About Page failed to open.");

// Close About page
startTest("Close About Page");
navBar.buttons()["Close"].tap()
target.delay(1);

if (window.scrollViews()[0].staticTexts()["Login"].isValid())
    passTest("Close About Page");
else
    failTest("Close About Page", "About Page failed to close.");



// Attempt to login to the game
startTest("Login Test");

login("mobiletest", "hackproof12");
target.delay(loginTimeout);

if (!window.scrollViews()[0].staticTexts()["Login"].isValid())
    passTest("Login Test");
else
    failTest("Login Test", "Game failed to log in.");










