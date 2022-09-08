package com.roblox.client;

import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.gigya.socialize.GSObject;
import com.gigya.socialize.GSResponse;
import com.gigya.socialize.GSResponseListener;
import com.gigya.socialize.android.GSAPI;
import com.gigya.socialize.android.GSSession;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetRequest;
import com.roblox.client.http.RbxHttpPostRequest;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.RbxReportingManager;
import com.roblox.client.managers.SessionManager;
import com.roblox.models.FacebookSignupData;

import org.json.JSONException;
import org.json.JSONObject;

import java.net.URLEncoder;

public class SocialManager {
    private static String TAG = "SocialManager";

    public SocialManager() {
        if (RobloxApplication.getInstance() != null) {
            GSAPI.getInstance().initialize(RobloxApplication.getInstance(),
                    RobloxApplication.getInstance().getResources().getString(R.string.GigyaApiKey));
        } else {
            GSAPI.getInstance().initialize(SessionManager.mCurrentActivity,
                    "3_OsvmtBbTg6S_EUbwTPtbbmoihFY5ON6v6hbVrTbuqpBs7SyF_LQaJwtwKJ60sY1p");
            new InfluxBuilderV2("AndroidNullContext").fireReport();
        }
        readKeyValues();
    }

    private void readKeyValues() {
        SharedPreferences data = RobloxSettings.getKeyValues();
        mUidSignature = data.getString(UIDSIG_KEY, "");
        mSignatureTimestamp = data.getString(SIGTIME_KEY, "");
        mUid = data.getString(UID_KEY, "");
        mLoginProvider = data.getString(LOGINPROV_KEY, "");
        mLoginProviderUid = data.getString(LOGINPROVUID_KEY, "");
        mProvider = data.getString(PROV_KEY, "");
        wasLoggedOut = data.getBoolean(WASLOGGEDOUT_KEY, false);
    }

    private void writeKeyValues() {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        editor.putString(UIDSIG_KEY, mUidSignature);
        editor.putString(SIGTIME_KEY, mSignatureTimestamp);
        editor.putString(UID_KEY, mUid);
        editor.putString(LOGINPROV_KEY, mLoginProvider);
        editor.putString(LOGINPROVUID_KEY, mLoginProviderUid);
        editor.putString(PROV_KEY, mProvider);
        editor.putBoolean(WASLOGGEDOUT_KEY, wasLoggedOut); // did the user manually log out on the last run?
        editor.apply();
    }

    static private SocialManager mInstance;
    static public SocialManager getInstance() { return (mInstance == null ? mInstance = new SocialManager() : mInstance); }
    static private final String UIDSIG_KEY = "UIDSignature",
            SIGTIME_KEY = "signatureTimestamp",
            UID_KEY = "UID",
            PROV_KEY = "provider",
            LOGINPROV_KEY = "loginProvider",
            LOGINPROVUID_KEY = "loginProviderUID",
            WASLOGGEDOUT_KEY = "wasLoggedOut",
            PROVIDER_FACEBOOK = "facebook";
    static public final String FACEBOOK_DATA_KEY = "facebookData";
    static private String mUidSignature = "", mSignatureTimestamp = "", mUid = "", mProvider = "", mLoginProvider = "", mLoginProviderUid = "";
    public static boolean   isConnectedFacebook = false, wasLoggedOut = false;
    private static FacebookSignupData mLastAttemptedFBSignup = null;
    private boolean wasLoginAutomatic = false;

    public void facebookLoginStart() {
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STARTED);
        GSObject params = new GSObject();
        params.put(PROV_KEY, "facebook");
        try {
            final long startTime = System.currentTimeMillis();
            GSAPI.getInstance().login(params, new GSResponseListener() {
                @Override
                public void onGSResponse(String s, GSResponse gsResponse, Object o) {
                    if (gsResponse != null && gsResponse.getInt("errorCode", 1) == 0) {
                        facebookLoginStartPostLogin(gsResponse.getData());
                    } else {
                        closeLandingSpinner();
                        RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_GIGYASTART,
                                gsResponse.getInt("errorCode", 1), true, wasLoginAutomatic, "GSAPI.login",
                                gsResponse.getData().toString(), "", System.currentTimeMillis() - startTime);

                    }
                }
            }, false, new Object());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /*
        Second stage of the FB login/signup flow. Contacts the RBX server
     */
    private RbxHttpPostRequest loginReq = null;
    private void facebookLoginStartPostLogin(GSObject data) {
//        if (true) {
//            facebookLoginHandlePostLoginResponse(null, null);
//            return;
//        }
        String realName = "", profileUrl = "";
        final long startTime = System.currentTimeMillis();
        try {
            mUidSignature = data.getString(UIDSIG_KEY);
            mSignatureTimestamp = data.getString(SIGTIME_KEY);
            mUid = data.getString(UID_KEY);
            mLoginProvider = data.getString(LOGINPROV_KEY);
            mLoginProviderUid = data.getString(LOGINPROVUID_KEY);

            realName = data.getString("firstName", "Welcome");
            profileUrl = data.getString("photoURL", "");

            GSObject temp = new GSObject(data.getArray("identities").getString(0));
            mProvider = temp.getString(PROV_KEY);
            String email = temp.getString("email", "");
            String gender = temp.getString("gender", "");
            String birthDay = temp.getString("birthDay", "");
            String birthMonth = temp.getString("birthMonth", "");
            String birthYear = temp.getString("birthYear", "");

            // Allows us to retry log-ins
            writeKeyValues();

            final FacebookSignupData fbd = new FacebookSignupData();
            fbd.profileUrl = profileUrl;
            fbd.realName = realName;
            fbd.email = email;
            fbd.gender = (gender.equals("f") ? "Female" : "Male");
            Log.i(TAG, "UID = " + mUid + ", UIDSignature = " + mUidSignature);
            Log.i(TAG, "EncodedUID = " + URLEncoder.encode(mUid, "UTF-8") + ", EncodedUIDSignature = " + URLEncoder.encode(mUidSignature, "UTF-8"));
            String params = RobloxSettings.socialLoginParams(URLEncoder.encode(mUidSignature, "UTF-8"), mSignatureTimestamp,
                    URLEncoder.encode(mUid, "UTF-8"), mProvider, mLoginProvider, mLoginProviderUid,
                    birthDay, birthMonth, birthYear);
            loginReq = new RbxHttpPostRequest(RobloxSettings.socialLoginUrl(), params, null, new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    facebookLoginHandlePostLoginResponse(response, fbd);
                }
            });
            loginReq.execute();

        } catch (Exception e) {
            e.printStackTrace();
            closeLandingSpinner();
            RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_GIGYAKEYMISSING, 0, true,
                    wasLoginAutomatic, "GSAPI.postLogin", data.toString(), "",
                    System.currentTimeMillis() - startTime);
            wasLoginAutomatic = false;
        }
    }

    private void closeLandingSpinner() {
        if (SessionManager.mCurrentActivity instanceof ActivityStart)
            ((ActivityStart)SessionManager.mCurrentActivity).closeSpinner();
    }

    private void facebookLoginHandlePostLoginResponse(HttpResponse response, FacebookSignupData fbd) {
//        if (true) {
//            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CREATE_USERNAME_REQUESTED, null);
//            return;
//        }

        int responseCode = response.responseCode();
        String status = "";
        Log.i(TAG, "response: " + response.toString());
        try {
            status = new JSONObject(response.responseBodyAsString()).getString("status");
        } catch (JSONException e) {
            e.printStackTrace();
        }

        if (responseCode == 200) {
            if (status.equals("currentUser")) {
                // take them inside the app
                onLoginSuccess(responseCode);
            }
            else if (status.equals("newUser")) {
                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
                // pull the necessary details, then open the CreateUserUsername fragment
                Bundle args = new Bundle();
                try {
                    JSONObject data = new JSONObject(response.responseBodyAsString());
                    if (fbd == null) fbd = new FacebookSignupData();

                    fbd.birthday = data.optString("birthday", "");
                    fbd.gigyaUid = data.optString("gigyaUid", "");

                    args.putParcelable("facebookData", fbd);
                } catch (JSONException e) {
                    e.printStackTrace();
                }

                if (!args.isEmpty())
                    NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CREATE_USERNAME_REQUESTED, args);
            }
        }
        else if (responseCode == 403) {
            if (status.equals("alreadyAuthenticated")) {
                onLoginSuccess(responseCode);

            } else if (status.equals("moderatedUser")) {
                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
                if (SessionManager.mCurrentActivity != null) {
                    Bundle args = new Bundle();
                    try {
                        JSONObject punishmentInfo = new JSONObject(response.responseBodyAsString()).getJSONObject("PunishmentInfo");
                        args.putString("PunishmentType", punishmentInfo.getString("PunishmentType"));
                        args.putString("ModeratorNote", punishmentInfo.getString("MessageToUser"));
                        args.putString("ReviewDate", punishmentInfo.getString("BeginDateString"));
                        args.putString("EndDate", punishmentInfo.getString("EndDateString"));
                        SessionManager.mCurrentActivity.showBannedAccountMessage(args);
                    } catch (JSONException e) {
                        Utils.alertExclusively(R.string.NeedsExternalLogin);
                    }
                } else {
                    Utils.alertExclusively(R.string.NeedsExternalLogin);
                }
                RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_NOTAPPROVED,
                        responseCode, true, wasLoginAutomatic, response.url(),
                        response.responseBodyAsString(), mUid, response.responseTime());
            } else {
                RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_POSTLOGINUNSPECIFIED,
                        responseCode, true, wasLoginAutomatic, response.url(),
                        response.responseBodyAsString(), mUid, response.responseTime());
                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
            }
        }
        else {
            // fatal error - display message to the user, stop flow
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
            RbxReportingManager.fireLoginFailure(RbxReportingManager.ACTION_F_POSTLOGINUNSPECIFIED, responseCode,
                    true, wasLoginAutomatic, response.url(), response.responseBodyAsString(),
                    mUid, response.responseTime());
            Utils.alert(R.string.FailedContactCS);
        }
        wasLoginAutomatic = false;
    }

    private void onLoginSuccess(int responseCode) {
        // already have an account, take them into the app
        SessionManager.getInstance().setIsLoggedIn();
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN);
        facebookGetUserInfoFinalStage(null, null);
        SessionManager.getInstance().requestUserInfoUpdate();
        RbxReportingManager.fireLoginSuccess(responseCode, true);
        wasLoggedOut = false;
        writeKeyValues();
    }

    private static int retryCount = 0;
    private static final int MAX_LOGIN_RETRY = 1;
    /***
     * Attempts a FB social login without opening the FB app for auth.
     */
    public void facebookLoginHeadless() {
        if (!AndroidAppSettings.EnableFacebookAuth()) return;

        if (GSAPI.getInstance().getSession() != null && GSAPI.getInstance().getSession().isValid() && !wasLoggedOut) {
            wasLoginAutomatic = true;

            Bundle b = new Bundle();
            b.putString("socialNetwork", "Facebook");
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STARTED, b);

            GSObject params = new GSObject();
            params.put("UID", getGigyaUID());
            params.put("enabledProviders", "facebook");
            params.put("format", "json");
            GSAPI.getInstance().sendRequest("socialize.getUserInfo", params, new GSResponseListener() {
                @Override
                public void onGSResponse(String s, GSResponse gsResponse, Object o) {
                    if (gsResponse.getInt("errorCode", 1) == 0) {// non-zero = error
                        try {
                            // if the user has no identities, this will throw an exception
                            // we're only requesting facebook identities, so we don't have to check any other elements
                            GSObject temp = new GSObject(gsResponse.getArray("identities", null).getString(0));
                            if (temp.getString("provider", "").equals("facebook")) {
                                SocialManager.isConnectedFacebook = true;
                                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_INFO_UPDATED);

                                facebookLoginStartPostLogin(gsResponse.getData());
                            }
                        } catch (Exception e) {
                            SocialManager.isConnectedFacebook = false;
                        } finally {
                            Log.i("facebookGetUserInfo", "isConnectedFacebook=" + SocialManager.isConnectedFacebook);
                            wasLoginAutomatic = false;
                        }
                    }
                    else {
                        GSAPI.getInstance().setSession(null);
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN_STOPPED);
                        Toast.makeText(SessionManager.mCurrentActivity, "Problem contacting Facebook - please login again.", Toast.LENGTH_LONG).show();
                        wasLoginAutomatic = false;
                    }
                }
            }, null);
        }
    }

    /***
     * Starts the FB-RBX signup process. Bundle should have the following keys:
     *      userName
     *      gigyaUid
     *      birthday
     *      gender
     *      email
     * @param data
     */
    public void facebookSignupStart(final Bundle data) {
//        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CREATE_USERNAME_SUCCESSFUL);
        FacebookSignupData fbd = null;
        if (data != null) {
            fbd = data.getParcelable(FACEBOOK_DATA_KEY);
            mLastAttemptedFBSignup = fbd;
        } else if (mLastAttemptedFBSignup != null) {
            fbd = mLastAttemptedFBSignup;
            mLastAttemptedFBSignup = null;
        } else {
            RbxReportingManager.fireSocialSignupFailure(RbxReportingManager.ACTION_F_MISSINGDATA, 0, "", "", "", 0);
            return;
        }

        String params = RobloxSettings.socialSignupParams(
                fbd.rbxUsername,
                fbd.gigyaUid,
                fbd.birthday,
                fbd.gender,
                fbd.email
        );

        final String username = fbd.rbxUsername;
        Log.i(TAG, "Signup params = " + params);
        RbxHttpPostRequest signupReq = new RbxHttpPostRequest( RobloxSettings.socialSignupUrl(), params, null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                int responseCode = response.responseCode();
                String status = "";
                try {
                    status = new JSONObject(response.responseBodyAsString()).getString("status");
                    if (responseCode == 200 && status.equals("success")) {
                        try {
                            int uId = new JSONObject(response.responseBodyAsString()).getInt("userId");
                            SessionManager.getInstance().setUserId(uId);
                            SessionManager.getInstance().requestUserInfoUpdate();

                            SessionManager.getInstance().setIsLoggedIn();
                            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CREATE_USERNAME_SUCCESSFUL);

                            // being explicit to be safe - covers case of logging out then creating a new account
                            wasLoggedOut = false;
                            writeKeyValues();

                            facebookGetUserInfoFinalStage(null, null);

                            RbxReportingManager.fireSocialSignupSuccess(responseCode);
                        } catch (JSONException e) {
                            e.printStackTrace();
                            RbxReportingManager.fireSocialSignupFailure(RbxReportingManager.ACTION_F_INCOMPLETEJSON,
                                    responseCode, response.url(), response.responseBodyAsString(),
                                    username, response.responseTime());
                        }
                    } else if (responseCode == 403 && status.equals("alreadyAuthenticated")){
                        // already have an account, take them into the app
                        onLoginSuccess(responseCode);
                        RbxReportingManager.fireSocialSignupFailure(RbxReportingManager.ACTION_ALREADYAUTHED, responseCode,
                                response.url(), response.responseBodyAsString(), username,
                                response.responseTime());
                    } else if (responseCode == 403 && status.equals("captcha")) {
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_CAPTCHA_SOCIAL_REQUESTED, data);
                        RbxReportingManager.fireSocialSignupFailure(RbxReportingManager.ACTION_F_CAPTCHA, responseCode,
                                response.url(), response.responseBodyAsString(), username,
                                response.responseTime());
                    }
                    else {
                        Utils.alert(R.string.FailedContactCS);
                        clearKeyValues();
                        RbxReportingManager.fireSocialSignupFailure(RbxReportingManager.ACTION_F_UNKNOWNERROR, responseCode,
                                response.url(), response.responseBodyAsString(), username,
                                response.responseTime());
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                    RbxReportingManager.fireSocialSignupFailure(RbxReportingManager.ACTION_F_JSONPARSE,
                            responseCode, response.url(), response.responseBodyAsString(), username,
                            response.responseTime());
                }
            }
        });
        signupReq.execute();

    }

    public void facebookConnectStart() {
        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STARTED);

        GSObject params = new GSObject();
        params.put("provider", "facebook");
        final long startTime = System.currentTimeMillis();
        // Important that we call addConnection (and NOT login), before we hit the RBX server
        GSAPI.getInstance().addConnection(params, new GSResponseListener() {
            @Override
            public void onGSResponse(String s, GSResponse gsResponse, Object o) {
                if (gsResponse != null && gsResponse.getInt("errorCode", 1) == 0) {
                    Log.i(TAG, gsResponse.toString());
                    facebookConnectUpdateInfo();
                } else {
                    NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STOPPED);
                    Utils.alert(R.string.GigyaConnectFailedGeneric);
                    RbxAnalytics.fireClientSideError("settings", "GigyaReturnedError", "connectFacebook");

                    String tempBody = "";
                    if (gsResponse != null)
                        tempBody = gsResponse.getResponseText();

                    RbxReportingManager.fireSocialConnectFailure(RbxReportingManager.ACTION_F_GIGYA_GENERIC,
                            0, "GSAPI.addConnection", tempBody,
                            SessionManager.getInstance().getUsername(), System.currentTimeMillis() - startTime, PROVIDER_FACEBOOK);
                }
            }
        }, null);
    }

    private void facebookConnectUpdateInfo() {
        RbxHttpPostRequest connectReq = new RbxHttpPostRequest(RobloxSettings.socialConnectUrl(), "", null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                int responseCode = response.responseCode();
                if (responseCode == 200) {
                    Utils.alert(R.string.FacebookConnectSuccess);
                    SocialManager.isConnectedFacebook = true;
                    NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECTED);

                    RbxReportingManager.fireSocialConnectSuccess(responseCode);
                }
                else if (responseCode == 403) {
                    try {
                        JSONObject json = new JSONObject(response.responseBodyAsString());
                        if (json.optString("status", "").equals("failed")) {
                            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STOPPED);
                            Utils.alertExclusively(json.optString("message",
                                    SessionManager.mCurrentActivity.getResources().getString(R.string.FailedContactCS)));

                            RbxReportingManager.fireSocialConnectFailure(RbxReportingManager.ACTION_F_FAILED,
                                    responseCode, response.url(), response.responseBodyAsString(),
                                    SessionManager.getInstance().getUsername(), response.responseTime(), PROVIDER_FACEBOOK);
                        }
                    } catch (Exception e) {
                        Log.i(TAG, e.toString());
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STOPPED);
                        Utils.alertExclusively(SessionManager.
                                mCurrentActivity.
                                getResources().
                                getString(R.string.FailedContactCS));
                        RbxAnalytics.fireClientSideError("settings", "EndpointReturnedError", "connectFacebook");

                        RbxReportingManager.fireSocialConnectFailure(RbxReportingManager.ACTION_F_UNKNOWNERROR,
                                responseCode, response.url(), response.responseBodyAsString(),
                                SessionManager.getInstance().getUsername(), response.responseTime(), PROVIDER_FACEBOOK);
                    }

                    facebookDisconnectStart(true); // clean up unused connection
                }
                else {
                    NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STOPPED);
                    Utils.alertExclusively(SessionManager.
                            mCurrentActivity.
                            getResources().
                            getString(R.string.FailedContactCS));

                    RbxAnalytics.fireClientSideError("settings", "EndpointReturnedError", "connectFacebookUnknown403");

                    RbxReportingManager.fireSocialConnectFailure(RbxReportingManager.ACTION_F_UNEXPECTEDRESPONSECODE,
                            responseCode, response.url(), response.responseBodyAsString(),
                            SessionManager.getInstance().getUsername(), response.responseTime(), PROVIDER_FACEBOOK);

                    facebookDisconnectStart(true); // clean up unused connection
                }
            }
        });
        connectReq.execute();
    }

    public void facebookDisconnectStart(final boolean silentDisconnect) {
        if (!silentDisconnect)
            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STARTED);

        GSObject params = new GSObject();
        params.put("provider", "facebook");
        final long startTime = System.currentTimeMillis();
        GSAPI.getInstance().removeConnection(params, new GSResponseListener() {
            @Override
            public void onGSResponse(String s, GSResponse gsResponse, Object o) {
                Log.i(TAG, gsResponse.toString());
                if (!silentDisconnect) {
                    if (gsResponse != null && gsResponse.getInt("errorCode", 1) == 0) {
                        facebookDisconnectSecondStage();
                    } else {
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STOPPED);
                        Utils.alertExclusively(R.string.GigyaDisconnectFailedGeneric);
                        RbxAnalytics.fireClientSideError("settings", "GigyaReturnedError", "disconnectFacebook");

                        String tempBody = "";
                        if (gsResponse != null)
                            tempBody = gsResponse.getResponseText();

                        RbxReportingManager.fireSocialDisconnectFailure(RbxReportingManager.ACTION_F_GIGYA_GENERIC,
                                0, "GSAPI.removeConnection", tempBody,
                                SessionManager.getInstance().getUsername(), System.currentTimeMillis() - startTime, PROVIDER_FACEBOOK);
                    }
                }
            }
        }, null);
    }

    private void facebookDisconnectSecondStage() {
        final String params = RobloxSettings.socialDisconnectParams("facebook");
        RbxHttpPostRequest disconnectReq = new RbxHttpPostRequest(RobloxSettings.socialDisconnectUrl(), params, null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                        int responseCode = response.responseCode();
                        if (responseCode == 200) {
                            Utils.alert(R.string.FacebookDisconnectSuccess);
                            SocialManager.isConnectedFacebook = false;
                            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECTED);

                            RbxReportingManager.fireSocialDisconnectSuccess(responseCode);
                        }
                        else if (responseCode == 403) {
                            try {
                                JSONObject json = new JSONObject(response.responseBodyAsString());
                                if (json.getString("status").equals("failed")) {
                                    NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STOPPED);
                                    Utils.alertExclusively(json.optString("message", SessionManager.mCurrentActivity.getString(R.string.GigyaDisconnectFailedGeneric)));

                                    RbxReportingManager.fireSocialDisconnectFailure(RbxReportingManager.ACTION_F_FAILED,
                                            responseCode, response.url(), response.responseBodyAsString(),
                                            SessionManager.getInstance().getUsername(), response.responseTime(), PROVIDER_FACEBOOK);
                                }
                            } catch (JSONException e) {
                                e.printStackTrace();
                                NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STOPPED);
                                Utils.alert(R.string.FacebookDisconnectFailedNoPassword);
                                RbxAnalytics.fireClientSideError("settings", "EndpointReturnedError", "disconnectFacebook");

                                RbxReportingManager.fireSocialDisconnectFailure(RbxReportingManager.ACTION_F_UNKNOWNERROR,
                                        responseCode, response.url(), response.responseBodyAsString(),
                                        SessionManager.getInstance().getUsername(), response.responseTime(), PROVIDER_FACEBOOK);
                            }
                        }
                        else {
                            NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STOPPED);
                            Utils.alertExclusively(R.string.GigyaDisconnectFailedGeneric);
                            RbxAnalytics.fireClientSideError("settings", "EndpointReturnedError", "disconnectFacebookUnkown403");

                            RbxReportingManager.fireSocialDisconnectFailure(RbxReportingManager.ACTION_F_UNEXPECTEDRESPONSECODE,
                                    responseCode, response.url(), response.responseBodyAsString(),
                                    SessionManager.getInstance().getUsername(), response.responseTime(), PROVIDER_FACEBOOK);
                        }
            }
        });
        disconnectReq.execute();
    }

    public void facebookGetUserInfoStart(final OnRbxGetUserInfo listener) {
        if (GSAPI.getInstance().getSession() == null || !GSAPI.getInstance().getSession().isValid()) {
            RbxHttpGetRequest getAuthDataReq = new RbxHttpGetRequest(RobloxSettings.socialAuthDataUrl(), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    try {
                        JSONObject json = new JSONObject(response.responseBodyAsString());
                        if (json.getString("status").equals("success")) {
                            long timestamp = json.getLong("timestamp");
                            String signature = json.getString("uidSignature");

                            if (timestamp != 0 && !signature.isEmpty())
                                facebookGetUserInfoSecondStage(timestamp, signature, listener);
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            });
            getAuthDataReq.execute();
        } else {
            facebookGetUserInfoFinalStage(null, listener);
        }
    }

    private void facebookGetUserInfoSecondStage(long timestamp, String signature, final OnRbxGetUserInfo listener) {
        GSObject loginParams = new GSObject();
        loginParams.put("UIDTimestamp", timestamp);
        loginParams.put("siteUID", getGigyaUID());
        loginParams.put("UIDSig", signature);
        GSAPI.getInstance().sendRequest("socialize.notifyLogin", loginParams, new GSResponseListener() {
            @Override
            public void onGSResponse(String s, GSResponse gsResponse, Object o) {
                if (gsResponse.getInt("errorCode", 1) == 0) {
//                    Log.i("facebookGetUserInfo", "RbxGigyaUID = " + getGigyaUID() + ", GGigyaUID = " + gsResponse.getString("UID", ""));
                    String sessionToken = gsResponse.getString("sessionToken", "");
                    String sessionSecret = gsResponse.getString("sessionSecret", "");

                    if (!sessionSecret.isEmpty() && !sessionToken.isEmpty())
                        GSAPI.getInstance().setSession(new GSSession(sessionToken, sessionSecret));

                    GSObject params = new GSObject();
                    params.put("UID", gsResponse.getString("UID", ""));
                    params.put("enabledProviders", "facebook");
                    params.put("format", "json");

                    facebookGetUserInfoFinalStage(params, listener);
                }
            }
        }, null);
    }

    private void facebookGetUserInfoFinalStage(GSObject params, final OnRbxGetUserInfo listener) {
        if (params == null) {
            params = new GSObject();
            params.put("UID", getGigyaUID());
            params.put("enabledProviders", "facebook");
            params.put("format", "json");
        }

        GSAPI.getInstance().sendRequest("socialize.getUserInfo", params, new GSResponseListener() {
            @Override
            public void onGSResponse(String s, GSResponse gsResponse, Object o) {
//                Log.i("facebookGetUserInfo", "GigyaResponse=" + gsResponse.getData().toString());
                if (gsResponse.getInt("errorCode", 1) == 0) {// non-zero = error
                    try {
                        // if the user has no identities, this will throw an exception
                        // we're only requesting facebook identities, so we don't have to check any other elements
                        GSObject temp = new GSObject(gsResponse.getArray("identities", null).getString(0));
                        if (temp.getString("provider", "").equals("facebook"))
                            SocialManager.isConnectedFacebook = true;
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_INFO_UPDATED);
                    } catch (Exception e) {
                        SocialManager.isConnectedFacebook = false;
                    } finally {
                        Log.i("facebookGetUserInfo", "isConnectedFacebook=" + SocialManager.isConnectedFacebook);
                        if (listener != null)
                            listener.onResponse();
                    }
                } else if (gsResponse.getInt("errorCode", 1) == 403005) {   // 403005 is 'unauthorized user', which means the Gigya
                                                                            // session is not set properly for the current user
                    GSAPI.getInstance().setSession(null);
                    facebookGetUserInfoStart(listener);
                }
            }
        }, null);
    }

    public void facebookConnectOrDisconnectStart(final String ctx) {
        facebookGetUserInfoStart(new OnRbxGetUserInfo(SocialManager.isConnectedFacebook) {
            @Override
            public void onResponse() {
                if (this.wasFbConnected == SocialManager.isConnectedFacebook) {
                    if (SocialManager.isConnectedFacebook) {
                        if (RobloxSettings.userHasPassword) {
                            RbxAnalytics.fireButtonClick(ctx, "disconnect");
                            SocialManager.getInstance().facebookDisconnectStart(false);
                        } else {
                            Utils.alert("Your account needs a password before you can disconnect from Facebook!");
                        }
                    } else {
                        RbxAnalytics.fireButtonClick(ctx, "connect");
                        SocialManager.getInstance().facebookConnectStart();
                    }
                } else {
                    if (SocialManager.isConnectedFacebook) {
                        Utils.alert(R.string.GigyaConnectDuplicateGeneric);
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_CONNECTED);
                    } else {
                        Utils.alert(R.string.GigyaDisconnectedDuplicateGeneric);
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_FACEBOOK_DISCONNECTED);
                    }
                }
            }
        });
    }

    private String getGigyaUID() {
        if (mUid != null && !mUid.isEmpty())
            return mUid;
        else
            return AndroidAppSettings.GigyaPrefix() + SessionManager.getInstance().getUserId();
    }

    public void gigyaLogout() {
        if (!AndroidAppSettings.EnableFacebookAuth()) return;

        clearKeyValues();

        GSAPI.getInstance().setSession(null);
        GSObject params = new GSObject();
        params.put("UID", getGigyaUID());
        params.put("format", "json");
        GSAPI.getInstance().sendRequest("socialize.logout", params, new GSResponseListener() {
            @Override
            public void onGSResponse(String s, GSResponse gsResponse, Object o) {
                if (gsResponse.getInt("errorCode", 1) == 0) {
                    Log.i(TAG, gsResponse.getData().toString());
                }
            }
        }, null);
    }

    private void clearKeyValues() {
        mUidSignature = "";
        mSignatureTimestamp = "";
        mUid = "";
        mProvider = "";
        mLoginProvider = "";
        mLoginProviderUid = "";
        wasLoggedOut = true; // can also be trigged by a failed signup, but either way we don't want
                                // to attempt an automatic login
        writeKeyValues();
    }

    public boolean willStartLogin() {
        if (!AndroidAppSettings.EnableFacebookAuth()) return false;

        if (GSAPI.getInstance().getSession() != null && GSAPI.getInstance().getSession().isValid()) return true;
        else return false;
    }

    public void stopLoginRequest() {
        if (loginReq != null && loginReq.getStatus() != AsyncTask.Status.FINISHED) {
            loginReq.cancel(true);
            Toast.makeText(SessionManager.mCurrentActivity, "Login took too long - please check your internet connection.", Toast.LENGTH_LONG).show();
        }
    }
}
