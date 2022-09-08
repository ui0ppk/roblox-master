package com.roblox.client;

import android.util.Log;

import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetRequest;

import org.json.JSONObject;

public class AndroidAppSettings {
    private final static String TAG = "AndroidAppSettings";
    private static JSONObject mAppSettingsJson = null;
    private static boolean settingsLoaded = false;
    public static boolean wereSettingsLoaded() { return settingsLoaded; }

    public interface FetchSettingsCallback {
        void onFinished(boolean success, HttpResponse responseData);
    }

    public static RbxHttpGetRequest fetchFromServer(final FetchSettingsCallback callback) {
        RbxHttpGetRequest settingsReq = new RbxHttpGetRequest(RobloxSettings.appSettingsUrl(), new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                if (response != null && !response.responseBodyAsString().isEmpty()) {
                    try {
                        JSONObject j = new JSONObject(response.responseBodyAsString());
                        AndroidAppSettings.setAppSettingsJson(j); // store here so the rest of the app has access
                        if (callback != null)
                            callback.onFinished(true, response);
                        settingsLoaded = true;
                    } catch (Exception e) {
                        Log.e("SettingsRequest", "Failed to parse settings!");
                        if (callback != null)
                            callback.onFinished(false, response);
                    }
                } else {
                    Log.e("SettingsRequest", "Failed to retrieve settings!");
                    if (callback != null)
                        callback.onFinished(false, response);
                }
            }
        });
        settingsReq.execute();

        return settingsReq;
    }

    private static void setAppSettingsJson(JSONObject j) {
        try {
            mAppSettingsJson = j;
            googleAdTagUrl = mAppSettingsJson.optString("GoogleVideoAdUrl", GoogleAdTagUrl());
            adColonyZoneId = mAppSettingsJson.optString("AdColonyZoneId", AdColonyZoneId());
            adColonyAppId = mAppSettingsJson.optString("AdColonyAppId", AdColonyAppId());
            enableRbxAnalytics = mAppSettingsJson.optBoolean("EnableRbxAnalytics", EnableRbxAnalytics());
            useNewWebGamesPage = mAppSettingsJson.optBoolean("UseNewWebGamesPage", UseNewWebGamesPage());
            enableSponsoredZoom = mAppSettingsJson.optBoolean("EnableSponsoredZoom", EnableSponsoredZoom());
            disableCookieDomainTrimming = mAppSettingsJson.optBoolean("DisableCookieDomainTrimming", DisableCookieDomainTrimming());
            enableRotationGestureFix = mAppSettingsJson.optBoolean("EnableRotationGestureFix", EnableRotationGestureFix());
            gigyaPrefix = mAppSettingsJson.optString("GigyaPrefix", GigyaPrefix());
            enableSponsoredZoom = mAppSettingsJson.optBoolean("EnableSponsoredZoom", EnableSponsoredZoom());
            enableUtilsAlertFix = mAppSettingsJson.optBoolean("EnableUtilsAlertFix", EnableUtilsAlertFix());
            enableFacebookAuth = mAppSettingsJson.optBoolean("EnableFacebookAuth", EnableFacebookAuth());
            enableGameStartFix = mAppSettingsJson.optBoolean("EnableGameStartFix", EnableGameStartFix());
            enableGoogleAnalyticsChange = mAppSettingsJson.optBoolean("EnableGoogleAnalyticsChange", EnableGoogleAnalyticsChange());
            enableCookieConsistencyChecks = mAppSettingsJson.optBoolean("EnableCookieConsistencyChecks", EnableCookieConsistencyChecks());
            enableRbxReportingManager = mAppSettingsJson.optBoolean("EnableRbxReportingManager", EnableRbxReportingManager());
            enableInfluxV2 = mAppSettingsJson.optBoolean("EnableInfluxV2", EnableInfluxV2());
            influxUrl = mAppSettingsJson.optString("InfluxUrl");
            influxDatabase = mAppSettingsJson.optString("InfluxDatabase");
            influxUser = mAppSettingsJson.optString("InfluxUser");
            influxPassword = mAppSettingsJson.optString("InfluxPassword");
            influxThrottleRate = mAppSettingsJson.optInt("InfluxThrottleRate", InfluxThrottleRate());
            enableNeonBlocker = mAppSettingsJson.optBoolean("EnableNeonBlocker", EnableNeonBlocker());
            enableLoginFailureExactReason = mAppSettingsJson.optBoolean("EnableLoginFailureExactReason", EnableLoginFailureExactReason());
            enableLoginWriteOnSuccessOnly = mAppSettingsJson.optBoolean("EnableLoginWriteOnSuccessOnly", EnableLoginWriteOnSuccessOnly());
            enableXBOXSignupRules = mAppSettingsJson.optBoolean("EnableXBOXSignupRules", EnableXBOXSignupRules());
            enableInputListenerActivePointerNullFix = mAppSettingsJson.optBoolean("EnableInputListenerActivePointerNullFix", EnableInputListenerActivePointerNullFix());
            enableWelcomeAnimation = mAppSettingsJson.optBoolean("EnableWelcomeAnimation", EnableWelcomeAnimation());
            enableShellLogoutOnWebViewLogout = mAppSettingsJson.optBoolean("EnableShellLogoutOnWebViewLogout", EnableShellLogoutOnWebViewLogout());
            enableSetWebViewBlankOnLogout = mAppSettingsJson.optBoolean("EnableSetWebViewBlankOnLogout", EnableSetWebViewBlankOnLogout());
            enableLoginLogoutSignupWithApi = mAppSettingsJson.optBoolean("EnableLoginLogoutSignupWithApi", EnableLoginLogoutSignupWithApi());
            enableSessionLogin = mAppSettingsJson.optBoolean("EnableSessionLogin", EnableSessionLogin());
            enableInferredCrashReporting = mAppSettingsJson.optBoolean("AndroidInferredCrashReporting", EnableInferredCrashReporting());
            enableAuthCookieAnalytics = mAppSettingsJson.optBoolean("EnableAuthCookieAnalytics", EnableAuthCookieAnalytics());
        } catch (Exception e) {
            Log.e(TAG, "Error retrieving values from AndroidAppSettings: " + e.toString());
        }
    }

    private static String googleAdTagUrl = null;
    private static String adColonyZoneId = null;
    private static String adColonyAppId = null;
    private static boolean disableCookieDomainTrimming = true;
    private static boolean enableRotationGestureFix = true;
    private static boolean useNewWebGamesPage = true;
    private static boolean enableValuePages = false;
    private static String gigyaPrefix = "PROD_"; // PROD for production, ST1 for sitetest1, etc.
    private static boolean enableSponsoredZoom = true;
    private static boolean enableUtilsAlertFix = true;
    private static boolean enableRbxAnalytics = true;
    private static boolean enableFacebookAuth = false;
    private static boolean enableGameStartFix = true;
    private static boolean enableGoogleAnalyticsChange = true;
    private static boolean enableCookieConsistencyChecks = true;
    private static boolean enableRbxReportingManager = true;
    private static boolean enableInfluxV2 = false;
    private static String influxUrl = null;
    private static String influxDatabase = null;
    private static String influxUser = null;
    private static String influxPassword = null;
    private static int influxThrottleRate = 10;
    private static boolean enableNeonBlocker = true;
    private static boolean enableLoginFailureExactReason = true;
    private static boolean enableLoginWriteOnSuccessOnly = true;
    private static boolean enableXBOXSignupRules = true;
    private static boolean enableInputListenerActivePointerNullFix = true;
    private static boolean enableWelcomeAnimation = false;
    private static boolean enableShellLogoutOnWebViewLogout = true;
    private static boolean enableSetWebViewBlankOnLogout = true;
    private static boolean enableLoginLogoutSignupWithApi = false;
    private static boolean enableSessionLogin = false;
    private static boolean enableInferredCrashReporting = false;
    private static boolean enableAuthCookieAnalytics = false;

    public static String GoogleAdTagUrl() {
        return googleAdTagUrl;
    }

    public static String AdColonyZoneId() {
        return adColonyZoneId;
    }

    public static String AdColonyAppId() {
        return adColonyAppId;
    }

    public static String GigyaPrefix() {
        return gigyaPrefix;
    }

    public static boolean EnableInfluxV2() {
        return enableInfluxV2;
    }

    public static boolean DisableCookieDomainTrimming() {
        return disableCookieDomainTrimming;
    }

    public static boolean EnableRotationGestureFix() {
        return enableRotationGestureFix;
    }

    public static boolean UseNewWebGamesPage() {
        return useNewWebGamesPage;
    }

    public static boolean EnableValuePages() {
        return enableValuePages;
    }

    public static boolean EnableSponsoredZoom() {
        return enableSponsoredZoom;
    }

    public static boolean EnableUtilsAlertFix() {
        return enableUtilsAlertFix;
    }

    public static boolean EnableRbxAnalytics() {
        return enableRbxAnalytics;
    }

    public static boolean EnableFacebookAuth() {
        return enableFacebookAuth;
    }

    public static boolean EnableGameStartFix() {
        return enableGameStartFix;
    }

    public static boolean EnableGoogleAnalyticsChange() {
        return enableGoogleAnalyticsChange;
    }

    public static boolean EnableCookieConsistencyChecks() {
        return enableCookieConsistencyChecks;
    }

    public static boolean EnableRbxReportingManager() { return enableRbxReportingManager; }

    public static String InfluxUrl() {
        return influxUrl;
    }

    public static String InfluxDatabase() {
        return influxDatabase;
    }

    public static String InfluxUser() {
        return influxUser;
    }

    public static String InfluxPassword() {
        return influxPassword;
    }

    public static int InfluxThrottleRate() {
        return influxThrottleRate;
    }

    public static boolean EnableNeonBlocker() {
        return enableNeonBlocker;
    }

    public static boolean EnableLoginFailureExactReason() {
        return enableLoginFailureExactReason;
    }

    public static boolean EnableLoginWriteOnSuccessOnly() {
        return enableLoginWriteOnSuccessOnly;
    }

    public static boolean EnableXBOXSignupRules() { return enableXBOXSignupRules; }

    public static boolean EnableInputListenerActivePointerNullFix() { return enableInputListenerActivePointerNullFix; }

    public static boolean EnableWelcomeAnimation() { return enableWelcomeAnimation; }

    public static boolean EnableShellLogoutOnWebViewLogout() { return enableShellLogoutOnWebViewLogout; }

    public static boolean EnableSetWebViewBlankOnLogout() { return enableSetWebViewBlankOnLogout; }

    public static boolean EnableLoginLogoutSignupWithApi(){ return enableLoginLogoutSignupWithApi; }

    public static boolean EnableSessionLogin(){ return enableSessionLogin; }

    public  static boolean EnableInferredCrashReporting() { return  enableInferredCrashReporting; }

    public static boolean EnableAuthCookieAnalytics() { return enableAuthCookieAnalytics; }
}
