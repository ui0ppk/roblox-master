package com.roblox.client.managers;

import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.util.Log;
import android.widget.Toast;

import com.roblox.client.AndroidAppSettings;
import com.roblox.client.ConfigEncryption;
import com.roblox.client.SocialManager;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.R;
import com.roblox.client.RbxAnalytics;
import com.roblox.client.RobloxActivity;
import com.roblox.client.http.RbxHttpGetRequest;
import com.roblox.client.RobloxSettings;
import com.roblox.client.Utils;
import com.roblox.client.http.RbxHttpPostRequest;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

/**
 * Created by roblox on 11/24/14.
 */
public class SessionManager {
    protected static final String TAG = "SessionManager";

    public static final String USERNAME_KEY = "username";
    public static final String USERID_KEY = "userid";
    protected static final String PASSWORD_KEY = "password";
    protected static final String PASSWORD_CHECKBOX_KEY = "password_checkbox";
    protected static final String USER_LOGGED_IN_TIME_KEY = "user_logged_in_time";
    public static final String LAST_AUTH_COOKIE_EXPIR_KEY = "last_auth_cookie_expir_key";
    protected static final String THUMBNAIL_KEY = "thumbnail";
    protected static final String ROBUX_KEY = "robux";
    protected static final String TICKETS_KEY = "tickets";
    protected static final String BUILDERS_CLUB_KEY = "builders_club";
    public static RobloxActivity mCurrentActivity = null;

    private String mUsername = null;
    private String mPassword = null;
    private int    mUserId = -1;
    private boolean mRememberPassword = true;
    boolean mLoggedIn = false; // As of last interaction with web server, not checking cookies.
    String mThumbnailUrl = null;
    int mRobuxBalance = 0;
    int mTicketsBalance = 0;
    boolean mIsAnyBuildersClubMember = false;
    private boolean wasLoginAutomatic = false;

    // ------------------------------------------------------------------------
    // Singleton (thread-safe)
    private static class Holder {
        static final SessionManager INSTANCE = new SessionManager();
    }

    public static SessionManager getInstance() {
        return Holder.INSTANCE;
    }

    // ------------------------------------------------------------------------

    public SessionManager() {
        readLoginKeyValues();
    }

    // ------------------------------------------------------------------------
    // Getters

    public String getUsername() { return mUsername; }
    public void setUsername(String username) { mUsername = username; }
    public String getPassword() { return mPassword; }
    public int      getUserId() { return mUserId; }
    public void     setUserId(int userId) { mUserId = userId; }
    public boolean getRememberPassword() { return mRememberPassword; }
    public String getThumbnailURL() { return mThumbnailUrl; }
    public int getRobuxBalance() { return mRobuxBalance; }
    public int getTicketsBalance() { return mTicketsBalance; }
    public boolean getIsLoggedIn() { return mLoggedIn; }
    public void     setIsLoggedIn() { mLoggedIn = true; }
    // -----------------------------------------------------------------------

    public void requestUserInfoUpdate() {
        if( mLoggedIn ) {
            RbxHttpGetRequest userInfoReq = new RbxHttpGetRequest(RobloxSettings.userInfoUrl(), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    JSONObject mJson = null;
                    try {
                        mJson = new JSONObject(response.responseBodyAsString());
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    if( mJson == null )
                    {
                        Utils.sendAnalytics("LoginAsyncTask", "NullJSON");
                        // The website may unilaterally invalidate your security token
                        // and at that point return HTML, not JSON.  To reach this point
                        // the user was logged in, log them out.
                        if( mRememberPassword && !mUsername.isEmpty() && !mPassword.isEmpty() )
                            startLogin(mCurrentActivity);
//                            (new LoginAsyncTask()).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, (Void[]) null);
                        else
                        {
                            doLogout();
                            Utils.alertExclusively(R.string.LoggedOut);
                        }
                    }
                    else if( onLogin( mJson ) )
                    {
                        Utils.sendAnalytics( "UserInfoAsyncTask", "OK" );
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_INFO_UPDATED);
                    }
                    else
                        Utils.sendAnalytics("LoginAsyncTask", response.responseBodyAsString());
                }
            });
            userInfoReq.execute();
        } else {
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_INFO_UPDATED);
        }
    }

    public void doLoginFromStart(String username, String password, final RobloxActivity ref)
    {
        // Write values
        mUsername = username;
        mPassword = password;
        mRememberPassword = true;
        if (!AndroidAppSettings.EnableLoginWriteOnSuccessOnly())
            writeLoginKeyValues();

        // Login
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STARTED);
        Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
        mUIThreadHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                startLogin(ref);
            }
        }, 1000);

    }

    public void retryLogin(final RobloxActivity ref) {
        retryLogin(ref, false);
    }

    public void retryLogin(final RobloxActivity ref, boolean fromCaptcha) {

        if(AndroidAppSettings.EnableSessionLogin() && !fromCaptcha){
            // check session not expired
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STARTED);

            Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
            mUIThreadHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    doSessionCheck(sessionCheckHandler);
                }
            }, 0);
        }
        else if (mUsername != null && !mUsername.isEmpty() && mPassword != null && !mPassword.isEmpty()) {
            wasLoginAutomatic = true;
            doLoginFromStart(mUsername, mPassword, ref);
        }
        else
            SocialManager.getInstance().facebookLoginHeadless();
    }

    public boolean willStartLogin() {
        if (mUsername != null && !mUsername.isEmpty() && mPassword != null && !mPassword.isEmpty()) return true;
        else return SocialManager.getInstance().willStartLogin();
    }

    int preNumAuthCookiesPresent = 0;
    int preLengthOfFirstAuthCookie = 0;
    public void doSessionCheck(final OnRbxHttpRequestFinished handler){
        if(AndroidAppSettings.EnableSessionLogin()) {
            /**** TODO: Will remove after data gathered ****/
            if (AndroidAppSettings.EnableAuthCookieAnalytics()) {
                int[] result = Utils.getNumberAndLengthOfAuthCookies();
                preNumAuthCookiesPresent = result[0];
                preLengthOfFirstAuthCookie = result[1];
            }
            /***** END TODO *****/

            RbxHttpGetRequest infoRequest = new RbxHttpGetRequest(RobloxSettings.accountInfoApiUrl(), handler);
            infoRequest.execute();
        }
    }

    private OnRbxHttpRequestFinished sessionCheckHandler = new OnRbxHttpRequestFinished(){
        @Override
        public void onFinished(HttpResponse response) {
            if(response.responseCode() != 200){
                long savedLoginTime = getSavedLoginTime();
                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
                if(savedLoginTime > 0) {
                    RbxReportingManager.fireLoginFailure(
                            RbxReportingManager.ACTION_F_INVALIDUSERSESSION,
                            response.responseCode(),
                            false, true,
                            response.url(),
                            Long.toString(System.currentTimeMillis() - savedLoginTime),
                            mUsername,
                            response.responseTime());

                    RbxReportingManager.fireAutoLoginFailure(getSavedLoginTime(), System.currentTimeMillis(), getSavedAuthCookieExpiration());

                    /**** TODO: Will remove after data gathered ****/
                    if (AndroidAppSettings.EnableAuthCookieAnalytics()) {
                        int[] result = Utils.getNumberAndLengthOfAuthCookies();
                        RbxReportingManager.fireAuthCookieAnalytics(preNumAuthCookiesPresent, preLengthOfFirstAuthCookie, RbxReportingManager.INFLUX_V_ENDPOINT_GETUSERINFO_PRE);
                        RbxReportingManager.fireAuthCookieAnalytics(result[0], result[1], RbxReportingManager.INFLUX_V_ENDPOINT_GETUSERINFO_POST);
                    }
                    /**** TODO END ****/

                    unsetLoginTime();
                    unsetAuthCookieExpiration();
                }
            } else {
                SessionManager.getInstance().setIsLoggedIn();
                SessionManager.getInstance().requestUserInfoUpdate();
                onAccountInfoReceived(response.responseBodyAsString());
                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN);
            }
        }
    };

    private void onAccountInfoReceived(String responseBody){
        // FORMAT:
        // {
        //     "Username": "robloxuser",
        //     "HasPasswordSet": true,
        //     "Email": {"Value":"user@roblox.com","IsVerified":false},
        //     "AgeBracket": 0
        // }
        if (responseBody != null && !responseBody.isEmpty()) {
            try {
                JSONObject json = new JSONObject(responseBody);
                // TODO get userid after webteam adds the field to json response (WEBAPI-197).
                // TODO save this information EVERYTIME user/account-info is called and not just after login calls.
                int ageBracket = json.optInt("AgeBracket", 1);
                if (ageBracket == 1) {
                    RobloxSettings.isUserUnder13 = true;
                } else {
                    RobloxSettings.isUserUnder13 = false;
                }
                JSONObject emailJson = json.optJSONObject("Email");
                if(emailJson != null) {
                    RobloxSettings.setUserEmail(emailJson.optString("Value", null));
                }
            } catch (JSONException e) {
                e.printStackTrace();
                RobloxSettings.isUserUnder13 = false;
            }
        } else {
            RobloxSettings.isUserUnder13 = false;
        }
    }

    // ------------------------------------------------------------------------

    public void doLogout() {
        doLogout(true);
    }

    public void doLogout(boolean callServer) {
        mRobuxBalance = 0;
        mTicketsBalance = 0;
        mThumbnailUrl = null;
        mIsAnyBuildersClubMember = false;

        mPassword = "";
        mLoggedIn = false;
        unsetLoginTime();
        unsetAuthCookieExpiration();

        writeLoginKeyValues();

        Utils.sendAnalytics("SessionManager", "logout");

        SocialManager.getInstance().gigyaLogout();

        if(callServer) {
            if (AndroidAppSettings.EnableLoginLogoutSignupWithApi()) {
                callLogoutWithApi();
            } else {
                callLogoutWithMobileApi();
            }
        }

        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGOUT);
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_INFO_UPDATED);
        wasLoginAutomatic = false; // safe to flip this off here, we know there's no reporting past this point
    }

    private void callLogoutWithApi(){
        RbxHttpPostRequest logoutReq = new RbxHttpPostRequest(RobloxSettings.logoutApiUrl(), null, null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                Log.i(TAG, "Logout: " + response);
                if (response.responseCode() != 200) {
                    // clear cookies to make sure user is properly logged out even though website returned error
                    android.webkit.CookieManager.getInstance().removeAllCookie();
                }
            }
        });
        logoutReq.execute();
    }

    private void callLogoutWithMobileApi(){
        RbxHttpPostRequest logoutReq = new RbxHttpPostRequest(RobloxSettings.logoutUrl(), "", null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                Log.i(TAG, "Logout: " + response);
                if (response.responseCode() != 200)
                    android.webkit.CookieManager.getInstance().removeAllCookie();
            }
        });
        logoutReq.execute();
    }

    // -----------------------------------------------------------------------

    private void readLoginKeyValues() {
        SharedPreferences keyValues = RobloxSettings.getKeyValues();

        mUsername = keyValues.getString( USERNAME_KEY, "" );
        mPassword = keyValues.getString(PASSWORD_KEY, "");
        mPassword = ConfigEncryption.decrypt(mPassword);
        mRememberPassword = keyValues.getBoolean( PASSWORD_CHECKBOX_KEY, false );
        mThumbnailUrl = keyValues.getString( THUMBNAIL_KEY, "" );
        mRobuxBalance = keyValues.getInt(ROBUX_KEY, -1 );
        mTicketsBalance = keyValues.getInt(TICKETS_KEY, -1 );
        mIsAnyBuildersClubMember = keyValues.getBoolean( BUILDERS_CLUB_KEY, false );
        mUserId = keyValues.getInt(USERID_KEY, -1);
    }

    // ------------------------------------------------------------------------

    // NOTE: Write an accessor in this class to modify mUsername/mPassword and then sync to disk.
    private void writeLoginKeyValues() {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();

        editor.putString(USERNAME_KEY, mUsername);
        if( mRememberPassword ) {
            String pw = ConfigEncryption.encrypt( mPassword );
            editor.putString(PASSWORD_KEY, pw);
        } else {
            editor.remove(PASSWORD_KEY);
        }

        editor.putBoolean(PASSWORD_CHECKBOX_KEY, mRememberPassword);
        editor.putString(THUMBNAIL_KEY, mThumbnailUrl);
        editor.putInt(ROBUX_KEY, mRobuxBalance);
        editor.putInt(TICKETS_KEY, mTicketsBalance);
        editor.putBoolean(BUILDERS_CLUB_KEY, mIsAnyBuildersClubMember);
        editor.putInt(USERID_KEY, mUserId);

        editor.apply();
    }

    // ------------------------------------------------------------------------

    private boolean onLogin( JSONObject json ) {
        boolean result = false;

        // Check for Gigya/FB info
        if (AndroidAppSettings.EnableFacebookAuth())
            SocialManager.getInstance().facebookGetUserInfoStart(null);

        if(AndroidAppSettings.EnableLoginLogoutSignupWithApi()){
            result = onLoginAfterApiLogin(json);
        }
        else {
            result = onLoginAfterMobileApiLogin(json);
        }

        return result;
    }

    private boolean onLoginAfterMobileApiLogin( JSONObject json ) {
        boolean result = false;
        try {
            JSONObject userInfo = json.optJSONObject("UserInfo");
            if( userInfo == null ) {
                userInfo = json; // UserInfoAsyncTask
            }

            RbxHttpGetRequest ageRequest = new RbxHttpGetRequest(RobloxSettings.userAgeBracketUrl(), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    if (!response.responseBodyAsString().isEmpty()) {
                        try {
                            JSONObject mJson = new JSONObject(response.responseBodyAsString());
                            int ageBracket = mJson.optInt("AgeBracket", 1);
                            if (ageBracket == 1) RobloxSettings.isUserUnder13 = true;
                            else if (ageBracket == 2) RobloxSettings.isUserUnder13 = false;

                            RobloxSettings.setUserEmail(mJson.optString("UserEmail", null));
                        } catch (JSONException e) {
                            e.printStackTrace();
                            RobloxSettings.isUserUnder13 = false;
                        }
                    }else {
                        RobloxSettings.isUserUnder13 = false;
                    }
                }
            });
            ageRequest.execute();

            mUsername					= userInfo.getString("UserName");
            mRobuxBalance				= userInfo.getInt("RobuxBalance");
            mTicketsBalance				= userInfo.getInt("TicketsBalance");
            mIsAnyBuildersClubMember	= userInfo.getBoolean("IsAnyBuildersClubMember");
            mThumbnailUrl				= userInfo.getString("ThumbnailUrl");
            mUserId                     = userInfo.getInt("UserID");

            mLoggedIn = true;

            writeLoginKeyValues();

            result = true;

            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN);
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_INFO_UPDATED);

        } catch(JSONException e) {
            Utils.alertUnexpectedError( "Missing User Info" );
            RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_MISSINGUSERINFO, 200,
                    false, wasLoginAutomatic, "loginUserInfo", json.toString(), mUsername, 0l);
            notifyLoginStopped();
            doLogout();
        }

        return result;
    }

    public boolean onLoginAfterApiLogin(JSONObject json) {
        boolean result = false;
        Log.i(TAG, "SM.onLoginAfterApiLogin() json:" + json);

        JSONObject userInfo = null;
        if(json != null) {
            userInfo = json.optJSONObject("UserInfo");
            if (userInfo == null) {
                userInfo = json; // UserInfoAsyncTask
            }
        }

        RbxHttpGetRequest infoRequest = new RbxHttpGetRequest(RobloxSettings.accountInfoApiUrl(), new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                onAccountInfoReceived(response.responseBodyAsString());
            }
        });

        RbxHttpGetRequest balanceRequest = new RbxHttpGetRequest(RobloxSettings.balanceApiUrl(), new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                // FORMAT:
                // {
                //      "robux":0,
                //      "tickets":40
                // }
                if (!response.responseBodyAsString().isEmpty()) {
                    try {
                        JSONObject json = new JSONObject(response.responseBodyAsString());
                        mRobuxBalance = json.optInt("robux");
                        mTicketsBalance	= json.optInt("tickets");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        if(userInfo != null) {
            mUserId = userInfo.optInt("UserID", mUserId);
            mUserId = userInfo.optInt("userId", mUserId); //alternate (login/v1)
        }
        Log.i(TAG, "SM.onLoginAfterApiLogin() mUsername:" + mUsername + " mUserId:" + mUserId);

        if(mUserId != -1) {
            mLoggedIn = true;
            setLoginTime();

            infoRequest.execute();
            balanceRequest.execute();

            writeLoginKeyValues();

            result = true;

            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN);
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_INFO_UPDATED);
        }
        else {
            // no user ID, probably an error, bail.
            Utils.alertUnexpectedError( "Missing User ID" );
            RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_MISSINGUSERINFO, 200,
                    false, wasLoginAutomatic, "loginUserId", json.toString(), mUsername, 0l);
            notifyLoginStopped();
            doLogout();
        }

        return result;
    }

    private void setLoginTime() {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        editor.putLong(USER_LOGGED_IN_TIME_KEY, System.currentTimeMillis()).apply();
    }

    private void unsetLoginTime(){
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        editor.remove(USER_LOGGED_IN_TIME_KEY).apply();
    }

    private long getSavedLoginTime(){
        SharedPreferences keyValues = RobloxSettings.getKeyValues();
        return keyValues.getLong(USER_LOGGED_IN_TIME_KEY, -1);
    }

    private long getSavedAuthCookieExpiration() {
        SharedPreferences keyValues = RobloxSettings.getKeyValues();
        return keyValues.getLong(LAST_AUTH_COOKIE_EXPIR_KEY, -1);
    }

    public void setAuthCookieExpiration(Long timestamp) {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        editor.putLong(SessionManager.LAST_AUTH_COOKIE_EXPIR_KEY, timestamp).apply();
    }

    private void unsetAuthCookieExpiration() {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        editor.remove(LAST_AUTH_COOKIE_EXPIR_KEY);
    }

    private RbxHttpPostRequest loginReq = null;
    private void startLogin(final RobloxActivity mActivityRef) {
        String encodedUsername = "";
        String encodedPassword = "";
        try {
            encodedUsername = URLEncoder.encode(mUsername, "UTF-8");
            encodedPassword = URLEncoder.encode(mPassword, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_UNSUPPORTEDENCODING, 0,
                    false, wasLoginAutomatic, "", "", mUsername, 0l);
        }

        if (encodedPassword.isEmpty() || encodedUsername.isEmpty()) {
            wasLoginAutomatic = false;
            return;
        }

        if(AndroidAppSettings.EnableLoginLogoutSignupWithApi()) {
            callLoginWithApi(encodedUsername, encodedPassword);
        }
        else {
            callLoginWithMobileApi(mActivityRef, encodedUsername, encodedPassword);
        }
    }

    /**
     * Login against api login/v1 endpoint
     *
     * @param encodedUsername
     * @param encodedPassword
     */
    private void callLoginWithApi(final String encodedUsername, final String encodedPassword){
        Log.i(TAG, "SM.callLoginWithApi() url:" + RobloxSettings.loginApiUrl());
        loginReq = new RbxHttpPostRequest(RobloxSettings.loginApiUrl(),
                Utils.format("username=%s&password=%s", encodedUsername, encodedPassword),
                null,
                new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                // if reportAction not null, there was an error
                String reportAction = null;
                boolean floodchecked = false;
                int code = response.responseCode();
                switch(code){
                    case 200:
                        Log.i(TAG, "SM.callLoginWithApi() 200");
                        RbxReportingManager.fireLoginSuccess(response.responseCode(), false);
                        setIsLoggedIn();
                        // make call to get UserInfo json
                        JSONObject jsonResponse = null;
                        try {
                            jsonResponse = new JSONObject(response.responseBodyAsString());
                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
                        onLogin(jsonResponse);
                        break;
                    case 400:
                        // error "Returned if username or password was null."
                        reportAction = onLoginInvalidUsernamePassword(RbxReportingManager.ACTION_F_INVALIDUSERPASS, false);
                        break;
                    case 403:
                        reportAction = RbxReportingManager.ACTION_F_UNKNOWNERROR;
                        String responseString = response.responseBodyAsString();
                        JSONObject responseJson;
                        String reason = "";
                        try {
                            responseJson = new JSONObject(responseString);
                            reason = responseJson.getString("message");
                        } catch (JSONException e) {
                            // error server returned no message...
                            Utils.alertExclusively("Unable to log in.");
                            reportAction = RbxReportingManager.ACTION_F_JSON;
                        }
                        if(reason.equalsIgnoreCase("Captcha")){
                            // "Captcha" - Captcha is required and the user has not passed it.
                            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CAPTCHA_REQUESTED);
                            RbxAnalytics.fireScreenLoaded("captcha");
                            reportAction = reason;
                            floodchecked = true;
                        }
                        else if(reason.equalsIgnoreCase("Credentials")){
                            // "Credentials" - The given username and password were incorrect.
                            reportAction = onLoginInvalidUsernamePassword(RbxReportingManager.ACTION_F_INVALIDUSERPASS, false);
                        }
                        else if(reason.equalsIgnoreCase("Privileged")){
                            // "Privileged" - The user is a privileged user and may not log in through this endpoint.
                            // Possible reason: Two-Step Verification not enabled on this endpoint
                            Utils.alertExclusively("Unable to log in.");
                            reportAction = RbxReportingManager.ACTION_F_PRIVILEGED;
                        }
                        else if(reason.equalsIgnoreCase("TwoStepVerification")){
                            // "TwoStepVerification" - Two step verification is required and the user has not passed it.
                            // NOTE not yet supported on client
                            Utils.alertExclusively("Unable to log in.");
                            reportAction = RbxReportingManager.ACTION_F_TWOSTEPVERIFICATION;
                        }
                        break;
                    case 404:
                        // error "The endpoint has been disabled."
                    case 500:
                        // error "An internal server error occurred."
                    case 503:
                        // error "The service was unavailable."
                        Utils.alertExclusively("Unable to log in.");
                        reportAction = RbxReportingManager.ACTION_F_UNKNOWNERROR;
                        break;
                }

                if (reportAction != null) {
                    Log.i(TAG, "SM.callLoginWithApi() code:" + response.responseCode() + " error:" + reportAction);
                    // report error
                    fireLoginFailure(reportAction, false, response);

                    if(!floodchecked){
                        notifyLoginStopped();
                        doLogout(false);
                    }
                }
            }
        });
        loginReq.execute();
    }

    /**
     * Login against mobileapi/login endpoint
     *
     * @param mActivityRef
     * @param encodedUsername
     * @param encodedPassword
     */
    private void callLoginWithMobileApi(final RobloxActivity mActivityRef, final String encodedUsername, final String encodedPassword) {
        loginReq = new RbxHttpPostRequest(RobloxSettings.loginUrl(),
                Utils.format("username=%s&password=%s", encodedUsername, encodedPassword), null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                if (response.responseBodyAsString().isEmpty()) {
                    RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_NORESPONSE, response.responseCode(),
                            false, wasLoginAutomatic, response.url(), "", mUsername, response.responseTime());
                    wasLoginAutomatic = false;

                    if (!Utils.alertIfNetworkNotConnected()) {
                        Utils.alertUnexpectedError("Login cannot get response");
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
                    }
                } else {
                    String reportAction = "UnknownError";
                    JSONObject mJson;
                    try {
                        mJson = new JSONObject(response.responseBodyAsString());
                    } catch (JSONException e) {
                        e.printStackTrace();
                        wasLoginAutomatic = false;
                        RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_JSON, response.responseCode(),
                                false, wasLoginAutomatic, response.url(),
                                response.responseBodyAsString(), mUsername, response.responseTime());
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
                        return;
                    }

                    boolean isOk = false;
                    boolean floodchecked = false;

                    try {
                        String status = mJson.getString("Status");
                        Utils.sendAnalytics("LoginAsyncTask", status);

                        switch (status) {
                            case "OK":
                                RbxReportingManager.fireLoginSuccess(response.responseCode(), false);
                                if (onLogin(mJson))
                                    isOk = true;
                                break;
                            case "InvalidUsername":
                                reportAction = onLoginInvalidUsernamePassword(status);
                                break;
                            case "InvalidPassword":
                                reportAction = onLoginInvalidUsernamePassword(status);
                                break;
                            case "MissingRequiredField":
                                reportAction = onLoginInvalidUsernamePassword(status);
                                break;
                            case "SuccessfulLoginFloodcheck":
                                reportAction = onLoginFloodcheck(status);
                                floodchecked = true;
                                break;
                            case "FailedLoginFloodcheck":
                                reportAction = onLoginFloodcheck(status);
                                floodchecked = true;
                                break;
                            case "FailedLoginPerUserFloodcheck":
                                reportAction = onLoginFloodcheck(status);
                                floodchecked = true;
                                break;
                            case "AccountNotApproved":
                                reportAction = onLoginAccountNotApproved(mActivityRef, mJson);
                                break;
                            case "BadCookieTryAgain":
                                reportAction = onBadCookie(mActivityRef);
                                isOk = true; // stops the password from being cleared out so that
                                                // we can make the second request, which should succeed
                                break;
                            default:
                                Utils.alertUnexpectedError(status);
                                break;
                        }
                    } catch (JSONException e) {
                        reportAction = onLoginJsonException();
                    } finally {
                        if (!isOk) {
                            RbxReportingManager.fireLoginFailure(reportAction, response.responseCode(),
                                    false, wasLoginAutomatic, response.url(), response.responseBodyAsString(),
                                    mUsername, response.responseTime());
                        }
                    }

                    if (!isOk && !floodchecked)
                        doLogout();
                }
            }
        });
        loginReq.execute();
    }

    private String onLoginInvalidUsernamePassword(String status) {
        return onLoginInvalidUsernamePassword(status, true);
    }

    private String onLoginInvalidUsernamePassword(String status, boolean notify) {
        int resId = R.string.InvalidUsernameOrPw;
        String reportAction = RbxReportingManager.ACTION_F_INVALIDUSERPASS;
        if(AndroidAppSettings.EnableLoginFailureExactReason()){
            switch (status) {
                case "InvalidUsername":
                    resId = R.string.LoginInvalidUsername;
                    reportAction = RbxReportingManager.ACTION_F_INVALIDUSER;
                    break;
                case "InvalidPassword":
                    resId = R.string.LoginInvalidPassword;
                    reportAction = RbxReportingManager.ACTION_F_INVALIDPASS;
                    break;
                case "MissingRequiredField":
                    resId = R.string.LoginMissingField;
                    reportAction = RbxReportingManager.ACTION_F_MISSINGFIELD;
                    break;
            }
        }
        Utils.alertExclusively(resId);
        if(notify) {
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
        }

        return reportAction;
    }

    private String onLoginFloodcheck(String status) {
        String reportAction = status;

        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CAPTCHA_REQUESTED);
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
        RbxAnalytics.fireScreenLoaded("captcha");
        switch (status) {
            case "SuccessfulLoginFloodcheck":
                reportAction = RbxReportingManager.ACTION_F_SUCCESSFLOOD;
                break;
            case "FailedLoginFloodcheck":
                reportAction = RbxReportingManager.ACTION_F_FAILEDFLOOD;
                break;
            case "FailedLoginPerUserFloodcheck":
                reportAction = RbxReportingManager.ACTION_F_USERFLOOD;
                break;
        }
        return reportAction;
    }

    private String onLoginAccountNotApproved(RobloxActivity mActivityRef, JSONObject mJson) {
        String reportAction = RbxReportingManager.ACTION_F_NOTAPPROVED;

        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
        if (mActivityRef != null) {
            Bundle args = new Bundle();
            try {
                JSONObject punishmentInfo = mJson.getJSONObject("PunishmentInfo");
                args.putString("PunishmentType", punishmentInfo.getString("PunishmentType"));
                args.putString("ModeratorNote", punishmentInfo.getString("MessageToUser"));
                args.putString("ReviewDate", punishmentInfo.getString("BeginDateString"));
                args.putString("EndDate", punishmentInfo.getString("EndDateString"));
                mActivityRef.showBannedAccountMessage(args);
            } catch (JSONException e) {
                reportAction = onLoginJsonException();
            }
        } else {
            Utils.alertExclusively(R.string.NeedsExternalLogin);
        }

        return reportAction;
    }

    @NonNull
    private String onLoginJsonException() {
        String reportAction;
        Utils.sendAnalytics("LoginAsyncTask", "IncompleteJSON");
        Utils.alertUnexpectedError("Login incomplete JSON");
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
        reportAction = RbxReportingManager.ACTION_F_INCOMPLETEJSON;
        return reportAction;
    }

    private static boolean wasRetried = false;
    public String onBadCookie(RobloxActivity ref) {
        if (!wasRetried) {
            retryLogin(ref);
            wasRetried = true;
        } else {
            Toast.makeText(SessionManager.mCurrentActivity, "An unknown error has occured. Please enter your login credentials again.", Toast.LENGTH_LONG).show();
            wasRetried = false;
        }
        return RbxReportingManager.ACTION_F_BADCOOKIE;
    }

    public void stopLoginRequest() {
        if (loginReq != null && loginReq.getStatus() != AsyncTask.Status.FINISHED) {
            loginReq.cancel(true);
            Toast.makeText(SessionManager.mCurrentActivity, "Login took too long - please check your internet connection.", Toast.LENGTH_LONG).show();
        }
    }

    /**
     * Convenience function for error reporting
     */
    private void fireLoginFailure(String action, boolean isSocial, HttpResponse response){
        RbxReportingManager.fireLoginFailure(
                action,
                response.responseCode(),
                isSocial,
                wasLoginAutomatic,
                response.url(),
                response.responseBodyAsString(),
                mUsername,
                response.responseTime());
    }

    private void notifyLoginStopped(){
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
    }
}
