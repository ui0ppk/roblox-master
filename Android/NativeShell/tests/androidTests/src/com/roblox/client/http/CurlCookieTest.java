package com.roblox.client.http;

import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import com.roblox.client.RobloxSettings;
import com.roblox.client.test.CurlTestHelper;
import com.roblox.client.test.TestHelperActivity;

/**
 * Created on 11/10/15.
 */
public class CurlCookieTest extends ActivityInstrumentationTestCase2<TestHelperActivity> {

    private final String TAG = "InstrumentationTest";

    public CurlCookieTest(){
        super(TestHelperActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        // Called before every test execution
        Log.v(TAG, "CurlCookieTest.setUp()");

        // Reset cookies for each test
        android.webkit.CookieManager manager = android.webkit.CookieManager.getInstance();
        manager.removeAllCookie();
    }

    public void testCurlCookies() throws Exception {

        // Get a cookie from initial request
        CookieTest cTest = new CookieTest();
        cTest.doSingleRequest(getActivity());

        String browserTrackerId = cTest.getBrowserTrackerIdInCookieManager();
        assertNotNull(browserTrackerId);

        // NOTE: typically this is done in RobloxSettings.initConfig() but there's a problem
        // loading the resource files so we'll just skip to the critical part for now.
        String cacheDir = RobloxSettings.initCacheDirectory(getActivity());
        RobloxSettings.initCookiesTempFile(cacheDir);

        // initialization for native stuff
        CurlTestHelper cHelper = new CurlTestHelper();
        cHelper.init();

        // check that browser tracker id from curl
        String cookieString = cHelper.getCookieString();
        assertTrue(cookieString.contains("browserid=" + browserTrackerId));

        // perform a request from curl
        cHelper.doCurlRequest("http://www.roblox.com");

        // check that browser tracker id from curl after request is still unchanged
        cookieString = cHelper.getCookieString();
        assertTrue(cookieString.contains("browserid=" + browserTrackerId));

        Log.v(TAG, "CurlCookieTest.testCurlCookies() cookies:" + cookieString);

    }
}
