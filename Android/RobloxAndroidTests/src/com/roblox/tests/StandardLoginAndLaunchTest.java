package com.roblox.tests;

import java.io.File;

import com.android.uiautomator.core.UiDevice;
import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.core.UiScrollable;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;

/**
 * Created by mcritelli on 9/17/14.
 */
public class StandardLoginAndLaunchTest extends UiAutomatorTestCase {
	
	// Open the app
	public void testOpenApp() throws UiObjectNotFoundException {
		// Return to the home screen
		getUiDevice().pressHome();
		
		
		// Press the App button
		UiObject applications = new UiObject(new UiSelector().description("Applications"));
		if (!applications.exists())
			applications = new UiObject(new UiSelector().description("Apps"));
		
		applications.clickAndWaitForNewWindow();
		
		// Get reference to app page
		UiScrollable appPage = new UiScrollable(new UiSelector().className("android.view.View").scrollable(true));
		
		// Set scrolling direction
		appPage.setAsHorizontalList();

		// Test to see if Roblox app is on this page
		UiObject robloxApp = appPage.getChildByText(new UiSelector().className(android.widget.TextView.class.getName()), "ROBLOX", true); 
		
		if (robloxApp.exists())
			robloxApp.clickAndWaitForNewWindow();
	}
	
	public void testLogin() throws UiObjectNotFoundException {
		UiDevice device = getUiDevice();

        UiObject spinnerEnvironemnt = new UiObject(new UiSelector().resourceId("com.roblox.client:id/spinner_environment"));
        spinnerEnvironemnt.getChild(new UiSelector().resourceId("android:id/text1"));
        
        String env = getParams().getString("environment", "www.gametest2.robloxlabs.com/");
        new UiScrollable(new UiSelector().scrollable(true)).scrollIntoView(new UiSelector().text(env));
        new UiObject(new UiSelector().text(env)).click();

        String username = getParams().getString("username", "mobiletest");
        String password = getParams().getString("password", "hackproof12");
        
		UiObject tvUsername = new UiObject(new UiSelector().resourceId("com.roblox.client:id/username"));
        //UiObject tvUsername = new UiObject(new UiSelector().className("android.widget.EditText").index(1));
		tvUsername.setText(username);
		sleep(2000);
		// Press back button to close the keyboard
		device.pressBack();
		sleep(1000);
		UiObject tvPassword = new UiObject(new UiSelector().resourceId("com.roblox.client:id/password"));
		//UiObject tvPassword = new UiObject(new UiSelector().className("android.widget.EditText").index(2));
		tvPassword.setText(password);
		device.pressBack();
		sleep(2000);
		
		UiObject btnLogin = new UiObject(new UiSelector().resourceId("com.roblox.client:id/login"));
		btnLogin.click();
		
		// If we logged in properly, this TextView should exist
		UiObject usernameTextView = new UiObject(new UiSelector().resourceId("com.roblox.client:id/username_textview"));
		assertTrue("Login successful?", usernameTextView.exists());
	}
	
	// Open each WebView in the app and save a screenshot to our device
	public void testScreenshotAllWebViews() throws UiObjectNotFoundException {
		UiDevice device = getUiDevice();
		
		OpenAndScreenshot(device, "com.roblox.client:id/catalog", "catalog.png");

		// Return to grid screen
		UiObject btnGrid = new UiObject(new UiSelector().resourceId("com.roblox.client:id/webview_grid"));
		btnGrid.click();
		
		OpenAndScreenshot(device, "com.roblox.client:id/inventory", "inventory.png");
		btnGrid.click();
		
		OpenAndScreenshot(device, "com.roblox.client:id/builders_club", "builders_club.png");
		btnGrid.click();
		
		OpenAndScreenshot(device, "com.roblox.client:id/profile", "profile.png");
		btnGrid.click();
		
		OpenAndScreenshot(device, "com.roblox.client:id/messages", "messages.png");
		btnGrid.click();
		
		OpenAndScreenshot(device, "com.roblox.client:id/groups", "groups.png");
		btnGrid.click();
	}
	
	public void OpenAndScreenshot(UiDevice device, String resourceId, String Filename) throws UiObjectNotFoundException
	{
		UiObject currentButton = new UiObject(new UiSelector().resourceId(resourceId));
		currentButton.click();
		sleep(5000);
		device.click(10, 10);
		sleep(1500);
		device.takeScreenshot(new File("/sdcard/roblox/" + Filename));
	}

    public void testLaunchGame() throws UiObjectNotFoundException {
        UiDevice device = getUiDevice();

        UiObject tvPlacedId = new UiObject(new UiSelector().resourceId("com.roblox.client:id/editTextPlaceID"));
        tvPlacedId.setText("70381591");
        device.pressBack();

        UiObject btnLaunchPlace = new UiObject(new UiSelector().resourceId("com.roblox.client:id/buttonLaunchPlace"));
        btnLaunchPlace.click();

        sleep(5000);

        device.click(10,200);

        // Click chat button
        sleep(15000);
        device.click(236, 30);
        new UiObject(new UiSelector().resourceId("com.roblox.client:id/gl_edit_text")).setText("The test is complete.");
        device.click(2368,1205);
    }
}
