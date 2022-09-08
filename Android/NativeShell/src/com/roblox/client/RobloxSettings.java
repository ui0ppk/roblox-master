package com.roblox.client;

import java.io.File;
import java.io.IOException;
import java.util.Calendar;
import java.util.List;
import java.util.TimeZone;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Point;
import android.os.Build;
import android.util.Log;

import com.roblox.client.managers.SessionManager;
import com.roblox.iab.Purchase;

public class RobloxSettings {
    private static final String TAG = "roblox.config";
    private static Context mContext = null; // From RobloxApplication
    private static String mVersion = "version missing";

    // From roblox_settings.json:
    private static boolean mFakeUserAgent = true;
    private static String  mFakeUserAgentString = null;
    private static String  mActualUserAgentString = null;
    private static String  mUserAgentSuffix = null;
    private static boolean isSw500dp = false;
    private static String  mBaseUrl = null;
    private static String  mBaseMobileUrl = null;
    private static boolean mEnableBreakpad = false;
    private static boolean mCleanupBreakpadDumps = true;
    private static String  mWebViewURLOverride = null;
    private static boolean mUseWebURLOverride = false;
    private static String  mCacheDirectory = null;
    private static String  mFilesDirectory = null;
    private static int     mNDKProfilerFrequency = 0;
    private static String  mDeviceNotSupported = null;
    private static boolean mDeviceNotSupportedSkippable = true;
    private static File    mRobloxCookiesTmpFile = null;
    private static boolean mIsInternalBuild = false;

    private static boolean mGooglePlayServicesAvailable = false;

    static SharedPreferences mKeyValues = null;

    public static SharedPreferences getKeyValues() {
        return mKeyValues;
    }

    public static String version()            			{ return mVersion; }
    public static boolean isInternalBuild()            { return mIsInternalBuild; }
    public static boolean isTablet()					{ return isSw500dp; }
    public static boolean isPhone()					{ return !isSw500dp; }
    public static String  baseUrl()					{ return "http://"  + (isTablet() ? mBaseUrl : mBaseMobileUrl); }
    public static String  baseUrlWWW()          { return "http://" + mBaseUrl; }
    public static String    baseUrlSecure()				{ return "https://"  + (isTablet() ? mBaseUrl : mBaseMobileUrl); }
    public static String  baseUrlSecureWWW()			{ return "https://"  +  mBaseUrl ; }
    public static String  baseUrlAPI()          { return mBaseUrl.replace("www.", "https://api."); }
    static String  baseUrlInternalDebug()       { return "http://" + mKeyValues.getString("internalDebugUrl", "failed"); }
    public static boolean isTestSite()
    {
        String bUrl = (isTablet() ? mBaseUrl : mBaseMobileUrl);
        if(bUrl.indexOf(".robloxlabs.com") != -1)
            return true;
        else
            return false;
    }

    static String  baseURLNoHttp()				{ return mBaseUrl; }
    static void	   setBaseUrlDebug(String newUrl)	{ mBaseUrl = newUrl; updateSharedPrefs("BaseUrl", newUrl); }
    static void    setBaseMobileUrlDebug(String newUrl) { mBaseMobileUrl = newUrl; updateSharedPrefs("BaseMobileUrl", newUrl); }
    static boolean enableBreakpad()				{ return mEnableBreakpad; }
    static boolean cleanupBreakpadDumps()		{ return mCleanupBreakpadDumps; }
    static String  webViewURLOverride()			{ return mWebViewURLOverride; }
    static boolean useWebURLOverride()			{ return mUseWebURLOverride; }
    static String  userAgentSuffix()			{ return mUserAgentSuffix; }
    static void    updateSharedPrefs(String key, String value) {
        try {
            SharedPreferences.Editor editor = mKeyValues.edit();
            editor.putString(key, value);
            editor.putString("internalDebugUrl", value);
            editor.apply();
        }
        catch (Exception e)
        {
            Log.e(TAG, "Error setting SharedPrefs: " + e.toString());
        }
    }

    static String  userAgentNotFaked()          {
        if( mActualUserAgentString != null )
        {
            return mActualUserAgentString;
        }

        long memory = Utils.getDeviceTotalMegabytes();
        Point screenSize = Utils.getScreenSize( mContext );
        Point dpi = Utils.getScreenDpi( mContext );
        Point dpsz = Utils.getScreenDpSize( mContext );
        String deviceName = Utils.getDeviceName();
        String androidVersion = Build.VERSION.RELEASE;
        String webKitVersion = Utils.getWebkitVersion( mContext );
        String phoneOrTablet = (Utils.isDevicePhone( mContext ) ? "Phone" : "Tablet");

        mActualUserAgentString = Utils.format( "Mozilla/5.0 (%dMB; %dx%d; %dx%d; %dx%d; %s; %s) %s (KHTML, like Gecko)  ROBLOX Android App %s %s Hybrid()",
                memory,
                screenSize.x,
                screenSize.y,
                dpi.x,
                dpi.y,
                dpsz.x,
                dpsz.y,
                deviceName,
                androidVersion,
                webKitVersion,
                mVersion,
                phoneOrTablet);

        return mActualUserAgentString;
    }
    public static String  userAgent() {
        if( mFakeUserAgent ) {
            return mFakeUserAgentString;
        } else {
            return userAgentNotFaked();
        }
    }
    static public String  loginUrl()					{ return baseUrlSecureWWW() + "mobileapi/login/"; }
    static public String  loginApiUrl()					{ return baseUrlAPI() + "login/v1"; }
    static public String  logoutUrl()					{ return baseUrlSecureWWW() + "mobileapi/logout"; }
    static public String  logoutApiUrl()		        { return baseUrlAPI() + "sign-out/v1"; }
    static public String  userInfoUrl()				{ return Utils.format("%smobileapi/userinfo", baseUrlSecure()); }
    static public String  accountInfoApiUrl()          { return baseUrlAPI() + "users/account-info"; }
    static public String  balanceApiUrl()          { return baseUrlAPI() + "my/balance"; }
    static public String  loginCaptchaValidateUrl()               { return baseUrlAPI() + "captcha/validate/login"; }
    static public String  signupCaptchaValidateUrl()               { return baseUrlAPI() + "captcha/validate/signup"; }

    // --- FragmentSignUp
    static public String  signUpUrl()					{ return baseUrlSecureWWW() + "mobileapi/securesignup"; }
    static public String  signUpApiUrl()               { return baseUrlAPI() + "signup/v1"; }
    static public String  signUpUrlArgs(String userName, String password, String gender, String dateOfBirth, String email) {
        return Utils.format("userName=%s&password=%s&gender=%s&dateOfBirth=%s&email=%s",userName, password, gender, dateOfBirth, email ); }

    static String  usernameCheckUrl(String userName) { return Utils.format("%sUserCheck/checkifinvalidusernameforsignup?username=%s", baseUrlSecureWWW(), userName); }
    static String  usernameCheckUrlXBOX(String userName) { return Utils.format("%ssignup/is-username-valid?username=%s", baseUrlAPI(), userName); }

    static String  passwordCheckUrl()			         { return baseUrlSecureWWW() + "UserCheck/validatepasswordforsignup"; }
    static String  passwordCheckUrlArgs(String userName, String password) { return Utils.format("password=%s&username=%s", password, userName); }
    static String  passwordCheckUrlXBOX(String userName, String password) { return Utils.format("%ssignup/is-password-valid?username=%s&password=%s", baseUrlAPI(), userName, password); }

    static String  recommendUsernameUrl(String userName) { return Utils.format("%sUserCheck/getrecommendedusername?usernameToTry=%s", baseUrlSecureWWW(), userName); }

    static public String captchaUrl()                 { return baseUrlWWW() + "mobile-captcha";}
    static public String captchaFailedUrl()           { return captchaUrl() + "?"; }
    static public String captchaSolvedUrl()           { return baseUrlWWW() + "mobile-captcha-solved"; }
    // --- ActivityNativeMain
    static String  homeUrl()                    { return baseUrlSecureWWW() + "home"; } // WWW on both phone & tablet
    static String  gamesUrl()					{ return baseUrlWWW() + "games"; }
    static String  gamesUrlBroken()				{ return baseUrlWWW() + (AndroidAppSettings.UseNewWebGamesPage() ? "games" : "games/list"); } // this is just here for temporary analytics. we're not proud of it.
    static String  catalogUrl()					{ return baseUrl() + ( isTablet() ? "catalog/" : "catalog/" ); }
    static String  characterUrl()				{ return baseUrl() + "My/Character.aspx"; }
    static String  tradeUrl()                   { return baseUrl() + "My/Money.aspx"; }
    static String  forumUrl()                   { return "http://www.roblox.com/Forum/default.aspx"; }
    static String  blogUrl()                    { return "http://blog.roblox.com/"; }
    static String  helpUrl()                    { return "https://en.help.roblox.com/hc/en-us"; }
    static String  settingsUrl()                { return baseUrlSecureWWW() + "my/account"; }
    static String  buildersClubUrl()			{ return baseUrl() + "mobile-app-upgrades/"; }
    static String  robuxOnlyUrl()               { return baseUrlSecureWWW() + "mobile-app-upgrades/native-ios/robux"; }
    static String  buildersClubOnlyUrl()        { return baseUrlSecureWWW() + "mobile-app-upgrades/native-ios/bc"; }
    static String  messagesUrl()				{ return baseUrl() + ( isTablet() ? "my/messages/#!/inbox" : "inbox" ); }
    static String  groupsUrl()					{ return baseUrl() + ( isTablet() ? "My/Groups.aspx" : "my-groups" ); }
    static String  leaderboardsUrl()			{ return baseUrl() + ( "leaderboards" ); }
    static String  inventoryUrl()				{ return getUsersPageUrl("inventory"); }
    static String  profileUrl()					{ return getUsersPageUrl("profile"); }
    static String  friendsUrl()                 { return getUsersPageUrl("friends"); }
    static String  searchUsersUrl(String term)  { return baseUrl() + ( isTablet() ? "users/search?keyword=" : "people?search=") + term; }
    static String  searchGamesUrl(String q)     { return baseUrlWWW() + "games/?Keyword=" + q;}
    //static String  searchCatalogUrl(String q)   { return baseUrl() + ( isTablet() ? "catalog/browse.aspx?CatalogContext=1&Keyword=": "catalog?search=") + q; }
    static String  searchCatalogUrl(String q)   { return baseUrl() + "catalog/browse.aspx?CatalogContext=1&Keyword=" + q; }
    // --- Upgrade URL
    static String  upgradeCheckUrl()			{ return baseUrlWWW() + "mobileapi/check-app-version?appVersion=" + (android.os.Build.BRAND.equals("Amazon") ? "AppAmazonV" : "AppAndroidV") + version(); }

    static String getUsersPageUrl(String page)   {
        int userId = SessionManager.getInstance().getUserId();
        if(userId != -1){
            return baseUrlSecureWWW() + "users/" + userId + "/" + page;
        }
        else {
            return baseUrlSecureWWW() + "users/" + page;
        }
    }

    // --- Login Web URLs
    static String  loginWebUrl()                { return baseUrl() + "Login"; }
    static String  WWWLoginWebUrl()             { return baseUrlWWW() + "Login"; }
    static String  secureLoginWebUrl()          { return baseUrlSecure() + "Login"; }
    static String  newLoginWebUrl()             { return baseUrl() + "newlogin"; }
    static String  secureWWWNewLoginWebUrl()    { return baseUrlSecureWWW() + "NewLogin"; }

    public static boolean isLoginRequiredWebUrl(String url){
        // Check if URL is a known "user not logged in" URL
        boolean isLoginUrl = url.startsWith(RobloxSettings.loginWebUrl()) ||
                url.startsWith(RobloxSettings.WWWLoginWebUrl()) ||
                url.startsWith(RobloxSettings.secureLoginWebUrl()) ||
                url.startsWith(RobloxSettings.newLoginWebUrl()) ||
                url.startsWith(RobloxSettings.secureWWWNewLoginWebUrl());

        // sitetest access requires user input at Login/FulfillConstraint.aspx
        isLoginUrl &= !url.contains("Login/FulfillConstraint.aspx");

        return isLoginUrl;
    }

    // --- Device Blacklist URL
    static String  appSettingsUrl()				{ return "http://clientsettings.api." + baseURLNoHttp().replace("www.", "") + "Setting/QuietGet/AndroidAppSettings/?apiKey=76E5A40C-3AE1-4028-9F10-7C62520BD94F"; }

    public static void setUserEmail(String newEmail) { mUserEmail = newEmail; }

    // --- In-App Purchasing URL
    static String  verifyPurchaseReceiptUrlForGoogle(Purchase p, boolean isRetry )	{
        return baseUrlSecure() + "mobileapi/google-purchase?packageName=" + p.getPackageName()
                + "&productId=" + p.getSku()
                + "&orderId=" + p.getOrderId()
                + "&isRetry=" + isRetry
                + "&token=" + p.getToken();
    }
    static String  verifyPurchaseReceiptUrlForAmazon(String receiptId, String userId, boolean isRetry)	{
        return baseUrlSecure() + "mobileapi/amazon-purchase?receiptId=" + receiptId
                + "&amazonUserId=" + userId
                + "&isRetry=" + isRetry;
    }
    static String  validatePurchaseUrl()                { return baseUrlSecureWWW() + "mobileapi/validate-mobile-purchase"; }
    static String  validatePurchaseParams(String productId) { return Utils.format("productId=%s", productId); }
    // --- More Page static data
    public static String eventsUrl()                   { return baseUrlWWW() + "sponsoredpage/list-json"; }
    static String eventsData = null;

    // --- Change Password
    static String changePasswordUrl()           { return baseUrlSecureWWW() + "account/changepassword"; }
    static String changePasswordParams(String oldPw, String newPw, String confirmPw) {
        return Utils.format("oldPassword=%s&newPassword=%s&confirmNewPassword=%s", oldPw, newPw, confirmPw); }

    // --- Change Email
    static String changeEmailUrl()              { return baseUrlSecureWWW() + "account/changeemail"; }
    static String changeEmailParams(String newEmail, String currPassword) {
        return Utils.format("emailAddress=%s&password=%s", newEmail, currPassword); }
    public static String userAgeBracketUrl()           { return baseUrlSecureWWW() + "my/account/json"; }
    public static boolean isUserUnder13 = false;

    // --- Account Notifications
    public static String accountNotificationsUrl() { return baseUrlAPI() + "notifications/account"; }
    public static void setAccountNotificationSettings(JSONObject json) {
        mEmailNotificationEnabled = json.optBoolean("EmailNotificationEnabled", false);
        mPasswordNotificationEnabled = json.optBoolean("PasswordNotificationEnabled", false);
    }

    private static boolean mEmailNotificationEnabled = false;
    private static boolean mPasswordNotificationEnabled = false;
    public static boolean isEmailNotificationEnabled() { return mEmailNotificationEnabled; }
    public static void disableEmailNotification() { mEmailNotificationEnabled = false; }
    public static boolean isPasswordNotificationEnabled() { return mPasswordNotificationEnabled; }
    public static void disablePasswordNotification()    { mPasswordNotificationEnabled = false; }

    // --- Email Notification
    private static String mUserEmail = null;
    public static String getUserEmail() {
        if (mUserEmail == null || mUserEmail.equals("null")) return "";
        else return mUserEmail;
    }
    public static String getSanitizedUserEmail() {
        return Utils.sanitizeEmailAddress(getUserEmail());
    }

    // --- Password Notification
    public static boolean userHasPassword = true;
    public static String userHasPasswordUrl() { return baseUrlAPI() + "social/has-user-set-password"; }

    // --- RbxAnalytics
    public static String   deviceIDUrl() { return baseUrlAPI() + "device/initialize"; }
    private static boolean mFirstLaunch = true;
    public static boolean isFirstLaunch()      { return mFirstLaunch; }
    public static void finishedFirstLaunch()      { mFirstLaunch = false; }
    public static String mDeviceId = "";
    public static String mBrowserTrackerId = "";
    private static String   rbxAnalyticsUrl()                                       { return "http://ecsv2.roblox.com/pe.png?t=mobile&lt=" +
            String.format("%tFT%<tT.%<tLZ", Calendar.getInstance(TimeZone.getTimeZone("Z"))) +
            "&mdid=" + mDeviceId; }
    private static String   rbxAnalyticsTestUrl()                                   { return "http://ecsv2.sitetest3.robloxlabs.com/pe.png?t=mobile&lt=" +
            String.format("%tFT%<tT.%<tLZ", Calendar.getInstance(TimeZone.getTimeZone("Z"))) +
            "&mdid=" + mDeviceId; }
    public static String    evtAppLaunchUrl(String ctx)             { return rbxAnalyticsUrl() + "&evt=appLaunch&ctx=" + ctx + "&appStoreSource=" + (BuildConfig.USE_AMAZON_PURCHASING ? "amazon" : "google"); }
    public static String    evtScreenLoadedUrl(String ctx)          { return rbxAnalyticsUrl() + "&evt=screenLoaded&ctx=" + ctx; }
    public static String    evtButtonClickUrl(String ctx, String btn)
    { return rbxAnalyticsUrl() + "&evt=buttonClick&ctx=" + ctx + "&btn=" + btn; }
    public static String    evtButtonClickUrl(String ctx, String btn, String custom)
    { return rbxAnalyticsUrl() + "&evt=buttonClick&ctx=" + ctx + "&btn=" + btn + "&cstm=" + custom; }
    public static String    evtFormFieldUrl(String ctx, String input, boolean vis)
    { return rbxAnalyticsUrl() + "&evt=formValidation&ctx=" + ctx + "&input=" + input; }
    public static String    evtFormFieldUrl(String ctx, String input, boolean vis, String msg)
    { return rbxAnalyticsUrl() + "&evt=formValidation&ctx=" + ctx + "&input=" + input + "&vis=" + vis + "&msg=" + msg; }
    public static String    evtClientSideError(String ctx, String error)
    { return rbxAnalyticsUrl() + "&evt=clientSideError&ctx=" + ctx + "&error=" + error; }
    public static String    evtClientSideError(String ctx, String error, String cstm)
    { return rbxAnalyticsUrl() + "&evt=clientSideError&ctx=" + ctx + "&error=" + error + "&cstm=" + cstm; }

    public static int mDeviceDensity = 0;

    // --- Social signup
    public static String    socialLoginUrl() {
        return baseUrlAPI() + "social/postlogin?";
    }
    public static String    socialLoginParams(String UIDSignature, String signatureTimestamp,
                                              String UID, String provider, String loginProvider,
                                              String loginProviderUID, String birthDay, String birthMonth,
                                              String birthYear) {
        return "UIDSignature=" + UIDSignature +
                "&signatureTimestamp=" + signatureTimestamp +
                "&UID=" + UID +
                "&provider=" + provider +
                "&loginProvider=" + loginProvider +
                "&loginProviderUID=" + loginProviderUID +
                "&birthDay=" + birthDay +
                "&birthMonth=" + birthMonth +
                "&birthYear=" + birthYear;
    }
    public static String    socialSignupUrl()   { return baseUrlAPI() + "social/signup?"; }
    public static String    socialSignupParams(String userName, String gigyaUid,
                                               String birthday, String gender,
                                               String email) {
        return "username=" + userName +
                "&gigyaUid=" + gigyaUid +
                "&birthday=" + birthday +
                "&gender=" + gender +
                "&email=" + email;
    }
    public static String    socialConnectUrl()  { return baseUrlSecureWWW() + "social/update-info"; }
    public static String    socialDisconnectUrl()   { return baseUrlSecureWWW() + "social/disconnect"; }
    public static String    socialDisconnectParams(String network) { return "provider=" + network; }
    public static String    socialAuthDataUrl() { return baseUrlAPI() + "social/get-uidsignature-timestamp"; }


    public static String    ephemeralCounterParams(String counterName, int amount) { return "&counterName=" + counterName + "&amount=" + amount;}
    public static String    ephemeralCounterUrl() {
        return mBaseUrl.replace("www.", "http://ephemeralcounters.api.")
                + "v1.1/Counters/Increment/?apiKey=76E5A40C-3AE1-4028-9F10-7C62520BD94F";
    }

    // --- libroblox.so
    public static void    saveRobloxCookies(String domain, String cookies)
    {
        if (null == domain || null == cookies)
            return;

        //Log.i(TAG, "Setting ROBLOX cookies for domain " + domain + " to " + cookies);
        Utils.writeToFile(mRobloxCookiesTmpFile, domain + "\n" + cookies);
    }
    static String  exceptionReasonFilename()	{ return "exception_reason.txt"; }
    static String  deviceNotSupportedString()	{ return mDeviceNotSupported; }
    static boolean deviceNotSupportedSkippable(){ return mDeviceNotSupportedSkippable; }
    static boolean dontReloadMorePage = false;

    static void initConfig( Context c ) throws IOException
    {
        mContext = c;
        mKeyValues = mContext.getSharedPreferences( "prefs", Context.MODE_PRIVATE|Context.MODE_MULTI_PROCESS );
        try {
            JSONObject j = Utils.loadJson( c, R.raw.roblox_settings );
            mFakeUserAgent = j.getBoolean( "FakeUserAgent" );
            mFakeUserAgentString = j.getString( "FakeUserAgentString" );
            mUserAgentSuffix = j.getString( "UserAgentSuffix" );
            mBaseUrl = j.getString( "BaseUrl" );
            mBaseMobileUrl = j.getString( "BaseMobileUrl" );
            mEnableBreakpad = j.getBoolean( "EnableBreakpad" );
            mCleanupBreakpadDumps = j.optBoolean("CleanupBreakpadDumps", true);
            mWebViewURLOverride = j.optString( "WebViewURLOverride", null );
            mUseWebURLOverride = j.optBoolean( "UseURLOverride", false );
            mNDKProfilerFrequency = j.getInt( "NDKProfilerFrequency" );
            mIsInternalBuild = j.optBoolean( "InternalBuild", false );
        } catch (JSONException e) {
            throw new IOException( "Cannot parse JSON resource: res/raw/roblox_settings: " + e.getMessage() );
        }

        if (mIsInternalBuild)
        {
            mBaseUrl = mKeyValues.getString("internalDebugUrl", baseUrl());
        }

        // Version # Read from Manifest
        try
        {
            PackageInfo pinfo = ((Application)mContext).getPackageManager().getPackageInfo(mContext.getPackageName(), 0);
            mVersion = pinfo.versionName;
        }
        catch(PackageManager.NameNotFoundException e)
        {
            throw new RuntimeException( "Cannot Read Package Info for Version String: " + e.getMessage() );
        }
        mDeviceDensity = c.getResources().getDisplayMetrics().densityDpi;

        if( !Utils.getDeviceHasNEON() )
        {
            mDeviceNotSupported = "Requires NEON instructions";
            mDeviceNotSupportedSkippable = false;
        }
        else if( Utils.getScreenDpi(mContext).x < 180 )
        {
            mDeviceNotSupported = ""; // Do not explain this reasoning.
        }
        else if( Build.MODEL.equals("SM­-T210R") )
        {
            mDeviceNotSupported = "SM­-T210R";
        }

        isSw500dp = c.getResources().getBoolean(R.bool.sw500dp);

        mFilesDirectory = mContext.getFilesDir().getAbsolutePath();
        String cacheDir = initCacheDirectory(mContext);
        initCookiesTempFile(cacheDir);
    }

    public static String initCacheDirectory(Context context){
        mCacheDirectory = context.getCacheDir().getAbsolutePath();
        return mCacheDirectory;
    }

    public static File initCookiesTempFile(String cacheDir){
        mRobloxCookiesTmpFile = new File(cacheDir, "2345sd-2345234-cookies.txt");
        return mRobloxCookiesTmpFile;
    }

    public static void enableNDKProfiler( boolean enable ) {
        if( enable )
        {
            boolean doesExist = nativeEnableNDKProfiler( mNDKProfilerFrequency );
            if( doesExist )
            {
                // Logging as error as this should not be shipped.
                Log.e(TAG, "Setting NDK Profiler frequency: "+mNDKProfilerFrequency );
            }
        }
        else
        {
            boolean doesExist = nativeEnableNDKProfiler( 0 );
            if( doesExist )
            {
                Utils.chmod( mFilesDirectory+"/gmon.out", 0755 );
            }
        }
    }

    // ------------------------------------------------------------------------
    // Native calls

    public static void updateNativeSettings()
    {
        nativeSetExceptionReasonFilename(exceptionReasonFilename());

        if( baseUrl() == null )
        {
            throw new NullPointerException( "Missing baseUrl()");
        }

        if (mIsInternalBuild)
            nativeSetBaseUrl( baseUrlInternalDebug() );
        else
            nativeSetBaseUrl( baseUrlWWW() );


        if(mCacheDirectory == null)
        {
            throw new NullPointerException( "Missing cacheDirectory");
        }

        nativeSetCacheDirectory( mCacheDirectory );
        nativeSetFilesDirectory( mFilesDirectory );

        // Set these before setting up breakpad, since breakpad will use
        // these values in the filename it uploads.
        nativeSetPlatformUserAgent(userAgent());
        nativeSetRobloxVersion(version());

        // Do this immediately after setting up the base url and cache directory.
        if (enableBreakpad())
        {
            nativeInitBreakpad(cleanupBreakpadDumps());
        }

        String proxyHost = System.getProperty("http.proxyHost", "");
        long proxyPort = Long.getLong("http.proxyPort", 0);
        nativeSetHttpProxy(proxyHost, proxyPort);

        if (null != mRobloxCookiesTmpFile)
        {
            List<String> lines = Utils.readTextFile(mRobloxCookiesTmpFile.toString());
            if (lines.size() > 0)
            {
                Log.i(TAG, "Setting ROBLOX cookies for domain " + lines.get(0) + " to " + lines.get(1));
                nativeSetCookiesForDomain(lines.get(0), lines.get(1));
            }
        }

        nativeInitFastLog();
    }

    private static native void nativeSetBaseUrl(String baseUrl);
    private static native void nativeSetCacheDirectory(String cacheDirectory);
    private static native void nativeSetFilesDirectory(String filesDirectory);
    private static native void nativeSetExceptionReasonFilename(String filename);
    private static native void nativeInitFastLog();
    private static native void nativeSetCookiesForDomain(String domain, String cookies);
    private static native void nativeSetHttpProxy(String proxyHost, long proxyPort);
    private static native void nativeInitBreakpad(boolean cleanup);
    private static native void nativeSetPlatformUserAgent(String userAgent);
    private static native void nativeSetRobloxVersion(String robloxVersion);
    private static native void nativeLocaleDecimalPoint(Byte decimalPoint);
    private static native boolean nativeEnableNDKProfiler(int frequency);
}
