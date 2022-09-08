package com.roblox.client.http;

import android.content.Context;
import android.provider.Settings;
import android.test.InstrumentationTestCase;
import android.util.Log;

import com.roblox.client.RobloxSettings;

import org.json.JSONException;
import org.json.JSONObject;

import java.net.URI;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created on 10/30/15
 */
public class CookieTest extends InstrumentationTestCase {

    private final String TAG = "InstrumentationTest";

    private static TestRequestData testGetData1;
    private static TestRequestData testGetData2;
    private static TestRequestData testPutData1;
    private static final String testBrowserTrackerId = "1234567890";
    private static final String testPutData1Cookie = "RBXEventTrackerV2=CreateDate=10/30/2015 4:43:45 PM&rbxid=&browserid=" + testBrowserTrackerId;

    private Pattern idPattern = Pattern.compile("RBXEventTrackerV2=.*?&browserid=(\\d+)");

    /**
     * Convenience class for holding test URI and request header information
     */
    private static class TestRequestData {
        private URI uri;
        private Map<String, List<String>> requestHeaders = new LinkedHashMap<String, List<String>>();

        public TestRequestData(String uri){
            setUri(uri);
        }

        public TestRequestData(URI uri){
            setUri(uri);
        }

        public URI getUri() {
            return uri;
        }

        public void setUri(URI uri) {
            this.uri = uri;
        }

        public void setUri(String uri) {
            this.uri = URI.create(uri);
        }

        public Map<String, List<String>> getRequestHeaders() {
            return requestHeaders;
        }

        public void addRequestHeader(String key, String val){
            List<String> headerValues = new ArrayList<String>();
            headerValues.add(val);
            requestHeaders.put(key, headerValues);
        }
    }

    static {
        testGetData1 = new TestRequestData("https://api.roblox.com/device/initialize");
        testGetData1.addRequestHeader("null", "POST /device/initialize HTTP/1.1");
        testGetData1.addRequestHeader("Accept-Encoding", "gzip");
        testGetData1.addRequestHeader("Connection", "Keep-Alive");
        testGetData1.addRequestHeader("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
        testGetData1.addRequestHeader("Host", "api.roblox.com");
        testGetData1.addRequestHeader("User-Agent", "Mozilla/5.0 (1805MB; 1920x1104; 320x322; 960x552; Asus Nexus 7; 4.4.4) AppleWebKit/537.36 (KHTML, like Gecko)  ROBLOX Android App 2.99.99 Tablet Hybrid()");

        testPutData1 = new TestRequestData("https://api.roblox.com/device/initialize");
        testPutData1.addRequestHeader("null", "HTTP/1.1 200 OK");
        testPutData1.addRequestHeader("Cache-Control", "private");
        testPutData1.addRequestHeader("Content-Encoding", "gzip");
        testPutData1.addRequestHeader("Content-Length", "66");
        testPutData1.addRequestHeader("Content-Type", "application/json; charset=utf-8");
        testPutData1.addRequestHeader("Date", "Fri, 30 Oct 2015 21:43:44 GMT");
        testPutData1.addRequestHeader("P3P", "CP=\"CAO DSP COR CURa ADMa DEVa OUR IND PHY ONL UNI COM NAV INT DEM PRE\"");
        testPutData1.addRequestHeader("Server", "Microsoft-IIS/8.5");
        testPutData1.addRequestHeader("Set-Cookie", testPutData1Cookie + "; domain=roblox.com; expires=Tue, 17-Mar-2043 21:43:45 GMT; path=/");
        testPutData1.addRequestHeader("Vary", "Accept-Encoding");
        testPutData1.addRequestHeader("X-AspNet-Version", "4.0.30319");
        testPutData1.addRequestHeader("X-AspNetMvc-Version", "5.2");
        testPutData1.addRequestHeader("X-Powered-By", "ASP.NET");

        testGetData2 = new TestRequestData("https://www.roblox.com/mobileapi/check-app-version?appVersion=AppAndroidV2.99.99");
        testGetData2.addRequestHeader("null", "GET /mobileapi/check-app-version?appVersion=AppAndroidV2.99.99 HTTP/1.1");
        testGetData2.addRequestHeader("Accept-Encoding", "gzip");
        testGetData2.addRequestHeader("Connection", "Keep-Alive");
        testGetData2.addRequestHeader("Host", "www.roblox.com");
        testGetData2.addRequestHeader("User-Agent", "Mozilla/5.0 (1805MB; 1920x1104; 320x322; 960x552; Asus Nexus 7; 4.4.4) AppleWebKit/537.36 (KHTML, like Gecko)  ROBLOX Android App 2.99.99 Tablet Hybrid()");
    }

    @Override
    public void setUp() throws Exception {
        // Called before every test execution
        Log.v(TAG, "CookieTest.setUp()");

        // Reset cookies for each test
        android.webkit.CookieManager manager = android.webkit.CookieManager.getInstance();
        manager.removeAllCookie();
    }

    /**
     * Test basic getting and putting into the Cookie Manager. Cookie Manager is mostly the platform code we shouldn't expect any problems
     */
    public void testGetPut() throws Exception {
        Log.v(TAG, "CookieTest.testGetPut()");

        // Init cookie manager
        WebkitCookieManagerProxy cookieManager = new WebkitCookieManagerProxy(null, java.net.CookiePolicy.ACCEPT_ALL);

        // Check that cookie manager is empty
        Map<String, List<String>> cookieMap = cookieManager.get(testGetData1.getUri(), testGetData1.getRequestHeaders());

        List<String> cookieList = cookieMap.get("Cookie");
        Log.v(TAG, "CookieTest.testGetPut() get:" + cookieList);
        assertNull(cookieList);

        // Store a cookie
        cookieManager.put(testPutData1.getUri(), testPutData1.getRequestHeaders());
        Log.v(TAG, "CookieTest.testGetPut() put:" + testPutData1Cookie);

        cookieMap = cookieManager.get(testGetData2.getUri(), testGetData2.getRequestHeaders());

        // Retrieve the cookie we just stored
        cookieList = cookieMap.get("Cookie");
        Log.v(TAG, "CookieTest.testGetPut() get:" + cookieList);
        assertNotNull(cookieList);
        String cookieVal = cookieList.get(0);
        assertEquals(cookieVal, testPutData1Cookie);
    }

    /**
     * Test getting cookie from the server
     */
    public void testSingleRequest() throws Exception {
        doSingleRequest(getInstrumentation().getContext());
    }

    public void doSingleRequest(Context context) throws Exception {
        Log.v(TAG, "CookieTest.testSingleRequest()");

        // Init HttpAgent to hook up cookie manager
        HttpAgent.onCreate(context);

        // Get Device ID for our request to the server
        String deviceId = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
        String params = "mobileDeviceId=" + deviceId;

        RbxHttpPostRequest deviceIdReq = new RbxHttpPostRequest(RobloxSettings.deviceIDUrl(), params, null,
                new OnRbxHttpRequestFinished() { @Override public void onFinished(HttpResponse response) {} });

        // Do network call synchronous
        HttpResponse response = deviceIdReq.doInBackground();

        // Get Browser Tracker ID
        String browserTrackerId = null;
        try {
            browserTrackerId = new JSONObject(response.responseBodyAsString()).optString("browserTrackerId", "");
            Log.v(TAG, "CookieTest.testSingleRequest() response:" + response.responseBodyAsString());
            Log.v(TAG, "CookieTest.testSingleRequest() id:" + browserTrackerId);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        assertNotNull(browserTrackerId);

        // Check cookie manager has stored Browser Tracker ID
        WebkitCookieManagerProxy cookieManager = new WebkitCookieManagerProxy(null, java.net.CookiePolicy.ACCEPT_ALL);

        Map<String, List<String>> cookieMap = cookieManager.get(testGetData2.getUri(), testGetData2.getRequestHeaders());

        assertNotNull(cookieMap);
        List<String> cookieList = cookieMap.get("Cookie");
        assertNotNull(cookieList);
        assertTrue(cookieList.get(0).contains("browserid=" + browserTrackerId));
        Log.v(TAG, "CookieTest.testSingleRequest() Cookie:" + cookieList);
    }

    /**
     * Test getting cookie from the server over multiple requests
     */
    public void testTrackerIdOverMultipleRequest() throws Exception {
        Log.v(TAG, "CookieTest.testTrackerIdOverMultipleRequest()");

        // Get a cookie from initial request
        testSingleRequest();

        String browserTrackerId = getBrowserTrackerIdInCookieManager();
        assertNotNull(browserTrackerId);

        // Test a sequence of requests and check that the cookie is persisted
        for(int i=1; i<=5; i++) {
            Log.v(TAG, "CookieTest.testTrackerIdOverMultipleRequest() req:" + i);
            RbxHttpGetRequest eventsReq = new RbxHttpGetRequest(RobloxSettings.eventsUrl(),
                    new OnRbxHttpRequestFinished() { @Override public void onFinished(HttpResponse response) {} });

            // Do network call synchronous
            eventsReq.doInBackground();

            assertEqualsBrowserTrackerIdInCookieManager(browserTrackerId);
        }
    }

    /**
     * Get Browser Tracker ID from cookie manager
     */
    public String getBrowserTrackerIdInCookieManager() throws Exception {

        WebkitCookieManagerProxy cookieManager = new WebkitCookieManagerProxy(null, java.net.CookiePolicy.ACCEPT_ALL);

        Map<String, List<String>> cookieMap = cookieManager.get(testGetData2.getUri(), testGetData2.getRequestHeaders());

        if(cookieMap != null) {
            List<String> cookieList = cookieMap.get("Cookie");
            if(cookieList != null && cookieList.get(0) != null){
                String browserTrackerCookie = cookieList.get(0);

                Matcher m = idPattern.matcher(browserTrackerCookie);
                if(m.find()) {
                    return m.group(1);
                }
            }
        }
        return null;
    }

    /**
     * Check that browserTrackerId is the same as stored in cookie manager
     */
    private void assertEqualsBrowserTrackerIdInCookieManager(String browserTrackerId) throws Exception {
        String storedId = getBrowserTrackerIdInCookieManager();
        assertEquals(storedId, browserTrackerId);
    }
}