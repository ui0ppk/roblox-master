package com.roblox.client.managers;

import android.os.AsyncTask;
import android.util.Log;

import com.roblox.client.BuildConfig;

import com.roblox.client.AndroidAppSettings;
import com.roblox.client.InfluxBuilderV2;
import com.roblox.client.RobloxSettings;
import com.roblox.client.Utils;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpPostRequest;

/**
 * Created by Sam on 9/20/2015.
 */
public class RbxReportingManager {

    /*
        Google Analytics Category strings - first param of sendAnalytics
     */
    private final static String GA_LOGIN = "Login";
    private final static String GA_LOGIN_SOCIAL = "LoginSocial";
    private final static String GA_SOCIAL_SIGNUP = "SocialSignupAttempt";
    private final static String GA_SOCIAL_CONNECT = "SocialConnectAttempt";
    private final static String GA_SOCIAL_DISCONNECT = "SocialDisconnectAttempt";
    private final static String GA_SIGNUP = "SignupAttempt";
    /*
        Google Analytics Action strings, InfluxDb Status strings
     */
    public final static String ACTION_SUCCESS = "Success";
    public final static String ACTION_F_UNSUPPORTEDENCODING = "FailureUnsupportedEncoding";
    public final static String ACTION_F_JSON = "FailureJSON";
    public final static String ACTION_F_NORESPONSE = "FailureNoResponse";
    public final static String ACTION_F_INVALIDUSERSESSION = "FailureInvalidUserSession";
    public final static String ACTION_F_INVALIDUSERPASS = "FailureInvalidUsernamePassword";
    public final static String ACTION_F_INVALIDUSER = "FailureInvalidUsername";
    public final static String ACTION_F_INVALIDPASS = "FailureInvalidPassword";
    public final static String ACTION_F_INVALIDBIRTHDAY = "FailureInvalidBirthday";
    public final static String ACTION_F_INVALIDGENDER = "FailureInvalidGender";
    public final static String ACTION_F_MISSINGFIELD = "FailureMissingField";
    public final static String ACTION_F_SUCCESSFLOOD = "FailureSuccessFloodcheck";
    public final static String ACTION_F_FAILEDFLOOD = "FailureFailedFloodcheck";
    public final static String ACTION_F_USERFLOOD = "FailurePerUserFloodcheck";
    public final static String ACTION_F_INCOMPLETEJSON = "FailureIncompleteJSON";
    public final static String ACTION_F_NOTAPPROVED = "AccountNotApproved";
    public final static String ACTION_F_MISSINGUSERINFO = "MissingUserInfo";
    public final static String ACTION_F_GIGYASTART = "FailureGigyaLogin";
    public final static String ACTION_F_GIGYAKEYMISSING = "FailureGigyaKeyMissing";
    public final static String ACTION_F_POSTLOGINUNSPECIFIED = "FailurePostLoginUnspecified";
    public final static String ACTION_F_JSONPARSE = "FailureJSONParse";
    public final static String ACTION_F_CAPTCHA = "FailureCaptcha";
    public final static String ACTION_ALREADYAUTHED = "AlreadyAuthenticated";
    public final static String ACTION_F_MISSINGDATA = "FailureMissingData";
    public final static String ACTION_F_TWOSTEPVERIFICATION = "FailureTwoStepVerification";
    public final static String ACTION_F_PRIVILEGED = "FailurePrivileged";
    public final static String ACTION_F_UNKNOWNERROR = "FailureUnknownError";
    public final static String ACTION_TASKRUNNING = "TaskStillRunning";
    public final static String ACTION_F_ALREADYTAKEN = "FailureAlreadyTaken";
    public final static String ACTION_F_INVALIDCHAR = "FailureInvalidCharacters";
    public final static String ACTION_F_CONTAINSSPACE = "FailureContainsSpaces";
    public final static String ACTION_F_ACCOUNTCREATEFLOOD = "FailureAccountCreateFloodcheck";
    public final static String ACTION_F_BADCOOKIE = "FailureBadCookie";
    /*
        Web returns {"status":"failed", ...}
     */
    public final static String ACTION_F_FAILED = "FailureFailed";
    public final static String ACTION_F_UNEXPECTEDRESPONSECODE = "FailureUnexpectedResponseCode";
    public final static String ACTION_F_GIGYA_GENERIC = "FailureGigya";
    /*
        Diag counter names
     */
    private final static String DIAG_APP_LOGIN_SUCCESS = "Android-AppLogin-Success";
    private final static String DIAG_APP_LOGIN_FAILURE = "Android-AppLogin-Failure";
    private final static String DIAG_SOCIAL_LOGIN_SUCCESS = "Android-SocialLogin-Success";
    private final static String DIAG_SOCIAL_LOGIN_FAILURE =  "Android-SocialLogin-Failure";
    private final static String DIAG_SOCIAL_SIGNUP_SUCCESS = "Android-SocialSignup-Success";
    private final static String DIAG_SOCIAL_SIGNUP_FAILURE = "Android-SocialSignup-Failure";
    private final static String DIAG_APP_SIGNUP_SUCCESS = "Android-AppSignup-Success";
    private final static String DIAG_APP_SIGNUP_FAILURE = "Android-AppSignup-Failure";
    /*
        InfluxDb series names
     */
    private final static String INFLUX_S_LOGIN_FAILURE = "LoginFailure";
    private final static String INFLUX_S_SIGNUP_FAILURE = "SignupFailureAndroid";
    private final static String INFLUX_S_SOCIAL_CONNECT_FAILURE = "SocialConnectFailureAndroid";
    private final static String INFLUX_S_SOCIAL_DISCONNECT_FAILURE = "SocialDisconnectFailureAndroid";
    private final static String INFLUX_S_APP_STARTUP_TIME = "AppStartupTimeAndroid";
    private final static String INFLUX_S_AUTO_LOGIN_FAILURE = "AutoLoginFailures";
    private final static String INFLUX_S_AUTH_COOKIE_FAILURE = "AndroidAuthCookieFailureData";
    private final static String INFLUX_S_AUTH_COOKIE_FLUSH = "AndroidAuthCookieFlushData";
    /*
        InfluxDb key names
     */
    private final static String INFLUX_K_STATUS = "Status";
    private final static String INFLUX_K_REQUESTURL = "requestUrl";
    private final static String INFLUX_K_RESPONSEBODY = "responseBody";
    private final static String INFLUX_K_USERNAME = "username";
    private final static String INFLUX_K_RESPONSETIME = "responseTimeMs";
    private final static String INFLUX_K_RESPONSECODE = "httpResponseCode";
    private final static String INFLUX_K_LOGINTYPE = "loginType";
    private final static String INFLUX_K_SIGNUPTYPE = "signupType";
    private final static String INFLUX_K_PROVIDER = "provider";
    private final static String INFLUX_K_PLATFORM = "platform";
    private final static String INFLUX_K_REQUESTNAME = "requestName";
    private final static String INFLUX_K_COMPLETIONTIME = "completionTime";
    private final static String INFLUX_K_INITIALLOGINTIMESTAMP = "initialLoginTimestamp";
    private final static String INFLUX_K_COOKIEEXPIRTIMESTAMP = "cookieExpirationTimestamp";
    private final static String INFLUX_K_EXPECTEDEXPIRTIMESTAMP = "expectedCookieExpirationTimestamp";
    private final static String INFLUX_K_ENDPOINT_ID = "endpointIdentifier";
    private final static String INFLUX_K_LENGTHOFAUTH = "lengthOfFirstAuthCookie";
    private final static String INFLUX_K_NUMCOOKIES = "numAuthCookiesPresent";
    private final static String INFLUX_K_REQUESTTIMESTAMP = "requestTimestamp";

    /*
        Other possible InfluxDb key values
     */
    public final static String INFLUX_V_REQNAME_FETCHAPPSETTINGS = "fetchAppSettings";
    public final static String INFLUX_V_REQNAME_DEVICEINIT = "deviceInitialize";
    public final static String INFLUX_V_REQNAME_FETCHUSERINFO = "fetchUserInfo";
    public final static String INFLUX_V_REQNAME_STARTUPFINISHED = "startupFinished";
    public final static String INFLUX_V_REQNAME_FETCHEVENTS = "fetchEventsInfo";
    public final static String INFLUX_V_ENDPOINT_DEVICEINIT_PRE = "pre_deviceInitialize";
    public final static String INFLUX_V_ENDPOINT_DEVICEINIT_POST = "post_deviceInitialize";
    public final static String INFLUX_V_ENDPOINT_GETUSERINFO_PRE = "pre_getUserInfo";
    public final static String INFLUX_V_ENDPOINT_GETUSERINFO_POST = "post_getUserInfo";

    public static void fireLoginSuccess(int responseCode, boolean isSocial) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        if (!isSocial) {
            Utils.sendAnalytics(GA_LOGIN, ACTION_SUCCESS, Integer.toString(responseCode));
            reportCounter(DIAG_APP_LOGIN_SUCCESS, 1);
        } else {
            Utils.sendAnalytics(GA_LOGIN_SOCIAL, ACTION_SUCCESS);
            reportCounter(DIAG_SOCIAL_LOGIN_SUCCESS, 1);
        }
    }

    public static void fireLoginFailure(String action, int responseCode, boolean isSocial, boolean wasAutomatic, String requestUrl, String responseBody, String username, long responseTime) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        if (action == null) action = "UnknownFailure";
        if (!isSocial) {
            Utils.sendAnalytics(GA_LOGIN, action, Integer.toString(responseCode));
            reportCounter(DIAG_APP_LOGIN_FAILURE, 1);
            reportInfluxLogin(action, responseCode, isSocial, wasAutomatic, requestUrl, responseBody, username, responseTime);
        } else {
            Utils.sendAnalytics(GA_LOGIN_SOCIAL, action, Integer.toString(responseCode));
            reportCounter(DIAG_SOCIAL_LOGIN_FAILURE, 1);
            reportInfluxLogin(action, responseCode, isSocial, wasAutomatic, requestUrl, responseBody, username, responseTime);
        }
    }

    public static void fireSocialSignupSuccess(int responseCode) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SOCIAL_SIGNUP, ACTION_SUCCESS, Integer.toString(responseCode));
        // Diag
         reportCounter(DIAG_SOCIAL_SIGNUP_SUCCESS, 1);
    }

    public static void fireSocialSignupFailure(String action, int responseCode, String requestUrl, String responseBody, String username, long responseTime) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SOCIAL_SIGNUP, action, Integer.toString(responseCode));
        // Diag
        reportCounter(DIAG_SOCIAL_SIGNUP_FAILURE, 1);
        // Influx
        reportInfluxSignupCommon(INFLUX_S_SIGNUP_FAILURE, action, responseCode, requestUrl, responseBody, username, responseTime, "social");
    }

    public static void fireSocialConnectSuccess(int responseCode) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SOCIAL_CONNECT, ACTION_SUCCESS, Integer.toString(responseCode));
    }

    public static void fireSocialConnectFailure(String action, int responseCode, String requestUrl, String responseBody, String username, long responseTime, String provider) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SOCIAL_CONNECT, action, Integer.toString(responseCode));
        // Influx
        reportInfluxSocialCommon(INFLUX_S_SOCIAL_CONNECT_FAILURE, action, responseCode, requestUrl, responseBody, username, responseTime, provider);
    }

    public static void fireSocialDisconnectSuccess(int responseCode) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SOCIAL_DISCONNECT, ACTION_SUCCESS, Integer.toString(responseCode));
    }

    public static void fireSocialDisconnectFailure(String action, int responseCode, String requestUrl, String responseBody, String username, long responseTime, String provider) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SOCIAL_DISCONNECT, action, Integer.toString(responseCode));
        // Influx
        reportInfluxSocialCommon(INFLUX_S_SOCIAL_DISCONNECT_FAILURE, action, responseCode, requestUrl, responseBody, username, responseTime, provider);
    }

    public static void fireSignupSuccess(int responseCode) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SIGNUP, ACTION_SUCCESS, Integer.toString(responseCode));
        // Diag
        reportCounter(DIAG_APP_SIGNUP_SUCCESS, 1);
    }

    public static void fireSignupFailure(String action, int responseCode, String requestUrl, String responseBody, String username, long responseTime) {
        if (!AndroidAppSettings.EnableRbxReportingManager()) return;

        // GA
        Utils.sendAnalytics(GA_SIGNUP, action, Integer.toString(responseCode));
        // Diag
        reportCounter(DIAG_APP_SIGNUP_FAILURE, 1);
        // Influx
        reportInfluxSignupCommon(INFLUX_S_SIGNUP_FAILURE, action, responseCode, requestUrl, responseBody, username, responseTime, "regular");
    }

    public static void fireAppStartupEvent(String requestName, long completionTime) {
        new InfluxBuilderV2(INFLUX_S_APP_STARTUP_TIME).
                addField(INFLUX_K_REQUESTNAME, requestName).
                addField(INFLUX_K_COMPLETIONTIME, completionTime).
                fireReport();
    }

    public static void fireAutoLoginFailure(long initialLoginTimestamp, long cookieExpirationTimestamp, long expectedCookieExpirationTimestamp) {
        new InfluxBuilderV2(INFLUX_S_AUTO_LOGIN_FAILURE).
                addField(INFLUX_K_INITIALLOGINTIMESTAMP, initialLoginTimestamp).
                addField(INFLUX_K_COOKIEEXPIRTIMESTAMP, cookieExpirationTimestamp).
                addField(INFLUX_K_EXPECTEDEXPIRTIMESTAMP, expectedCookieExpirationTimestamp).
                fireReport();
    }

    public static void fireAuthCookieAnalytics(int preNumAuthCookiesPresent, int preLengthOfFirstAuthCookie, String preEndpointIdentifier) {
        new InfluxBuilderV2(INFLUX_S_AUTH_COOKIE_FAILURE).
                addField(INFLUX_K_NUMCOOKIES, preNumAuthCookiesPresent).
                addField(INFLUX_K_LENGTHOFAUTH, preLengthOfFirstAuthCookie).
                addField(INFLUX_K_ENDPOINT_ID, preEndpointIdentifier).
                fireReport();
    }

    public static void fireAuthCookieFlush(long requestTimestamp, String requestUrl) {
        Log.i("SAMINFLUX", "INSIDE AUTHCOOKIEFLUSH");
        new InfluxBuilderV2(INFLUX_S_AUTH_COOKIE_FLUSH).
                addField(INFLUX_K_REQUESTURL, requestUrl).
                setTimestamp(requestTimestamp).
                setTimestampPrecision(InfluxBuilderV2.TIMESTAMP_PRECISION.MILLISECONDS).
                fireReport();
    }

    /*
        Helper methods
     */
    private static void reportInfluxLogin(String reason, int responseCode, boolean isSocial, boolean wasAutomatic, String requestUrl, String responseBody, String username, long responseTime) {
        String loginType = "";
        if (isSocial && wasAutomatic)
            loginType = "socialAuto";
        else if (isSocial && !wasAutomatic)
            loginType = "social";
        else if (!isSocial && wasAutomatic)
            loginType = "auto";
        else
            loginType = "manual";

        new InfluxBuilderV2(INFLUX_S_LOGIN_FAILURE).
                addField(INFLUX_K_STATUS, reason).
                addField(INFLUX_K_LOGINTYPE, loginType).
                addField(INFLUX_K_REQUESTURL, requestUrl).
                addField(INFLUX_K_RESPONSECODE, responseCode).
                addField(INFLUX_K_RESPONSEBODY, responseBody).
                addField(INFLUX_K_USERNAME, username).
                addField(INFLUX_K_RESPONSETIME, responseTime).
                fireReport();
    }

    // Updates an ephemeral counter (the things diag is built off of)
    private static void reportCounter(String counterName, int amount) {
        RbxHttpPostRequest report = new RbxHttpPostRequest(RobloxSettings.ephemeralCounterUrl() + RobloxSettings.ephemeralCounterParams(counterName, amount),
                null, null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                // don't need to do anything
            }
        });
        report.execute();
    }

    private static void reportInfluxSocialCommon(String series, String action, int responseCode, String requestUrl, String responseBody, String username, long responseTime, String provider) {
        new InfluxBuilderV2(series)
                .addField(INFLUX_K_STATUS, action)
                .addField(INFLUX_K_REQUESTURL, requestUrl)
                .addField(INFLUX_K_RESPONSEBODY, responseBody)
                .addField(INFLUX_K_USERNAME, username)
                .addField(INFLUX_K_RESPONSETIME, responseTime)
                .addField(INFLUX_K_RESPONSECODE, responseCode)
                .addField(INFLUX_K_PROVIDER, provider)
                .fireReport();
    }

    private static void reportInfluxSignupCommon(String series, String action, int responseCode, String requestUrl, String responseBody, String username, long responseTime, String signupType) {
        new InfluxBuilderV2(series)
                .addField(INFLUX_K_STATUS, action)
                .addField(INFLUX_K_REQUESTURL, requestUrl)
                .addField(INFLUX_K_RESPONSEBODY, responseBody)
                .addField(INFLUX_K_USERNAME, username)
                .addField(INFLUX_K_RESPONSETIME, responseTime)
                .addField(INFLUX_K_RESPONSECODE, responseCode)
                .addField(INFLUX_K_SIGNUPTYPE, signupType)
                .fireReport();
    }

}
