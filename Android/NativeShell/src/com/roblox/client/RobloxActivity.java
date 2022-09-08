package com.roblox.client;

import android.app.Activity;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

import com.android.debug.hv.ViewServer;
import com.flurry.android.FlurryAgent;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.managers.SessionManager;

public class RobloxActivity extends AppCompatActivity {
    protected static final String TAG = "RobloxActivity";
    protected static final String WEBVIEW_URL_KEY = "webview_url";
    protected static final String LAST_APP_VERSION_KEY = "last_version_code";
    protected static int referenceCount = 0;

    protected StoreManager mStoreMgr = null;

    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (RobloxSettings.isPhone())
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
        else
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        HttpAgent.onCreate(this);

        if (AndroidAppSettings.EnableUtilsAlertFix())
            RobloxApplication.getInstance().setCurrentActivity(this);

        if(BuildConfig.DEBUG) {
            ViewServer.get(this).addWindow(this);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(BuildConfig.DEBUG) {
            ViewServer.get(this).removeWindow(this);
        }
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onStart() {
        super.onStart();

        if (referenceCount++ == 0)
        {
            SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
            editor.putBoolean("ROBLOXCrash", true);
            editor.apply();
        }
        mStoreMgr = StoreManager.getStoreManager(this);

        try {
            FlurryAgent.init(this, "3QYHHVKPXQPP3BJV7CTP");
            FlurryAgent.onStartSession(this);
        } catch (NullPointerException e) {
        }

        this.writeLastVersionCode();

        Intent intent = new Intent(this, RobloxService.class);
        startService(intent);
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onStop() {
        try {
            FlurryAgent.onEndSession(this);
        } catch (NullPointerException e) {
        }

        HttpAgent.onStop();
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        if (--referenceCount == 0) {
            editor.remove("ROBLOXCrash");
            editor.apply();
        }

        super.onStop();
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onPause() {
        String url = RobloxSettings.mKeyValues.getString(WEBVIEW_URL_KEY, "");
        HttpAgent.onPause(getCacheDir(), url);
        // Cause getIntent to return null signaling that onResume was called without an Activity transition.
        setIntent(null);

        super.onPause();
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onResume() {
        super.onResume();

        if (!AndroidAppSettings.EnableUtilsAlertFix())
            RobloxApplication.getInstance().setCurrentActivity(this);

        HttpAgent.onResume();
        mStoreMgr.handleActivityResume();

        if(BuildConfig.DEBUG){
            ViewServer.get(this).setFocusedWindow(this);
        }
    }

    // -----------------------------------------------------------------------
    // Allow getIntent() to return the last Intent passed, not the first.
    @Override
    public void onNewIntent(Intent n) {
        setIntent(n);
        super.onNewIntent(n);
    }

    // -----------------------------------------------------------------------
    // *** onLowMemory() is implemented in and sent down by the ActivityGlView. ***
    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        RobloxApplication.logTrimMemory(TAG, level);
    }

    // -----------------------------------------------------------------------
    // protected
    protected void writeLastVersionCode() {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();

        int versionCode = -1;
        try {
            versionCode = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
        } catch (NameNotFoundException e) {

        }
        editor.putInt(LAST_APP_VERSION_KEY, versionCode);

        editor.apply();
    }

    public void onLoginCaptchaSolved() {
        // Find captcha web fragment
        Fragment fragWeb = getFragmentManager().findFragmentByTag(RobloxWebFragment.FRAGMENT_TAG_CAPTCHA);
        FragmentTransaction ft = getFragmentManager().beginTransaction();

        if (fragWeb != null && fragWeb.isVisible())
        {
            // Remove fragment
            ft.remove(fragWeb);
            ft.commit();

            // Trigger another login
            SessionManager.getInstance().retryLogin(this, true);
        }
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // Pass on the activity result to the store Manager for handling
        if (!mStoreMgr.handleActivityResult(requestCode, resultCode, data)) {
            // not handled, so handle it ourselves (here's where you'd
            // perform any handling of activity results not related to in-app
            // billing...
            if(requestCode == ReCaptchaActivity.ACTIVITY_REQUEST_CODE){
                if(resultCode == Activity.RESULT_OK) {
                    hideCaptcha();
                }
                else {
                    // update the fragment UI
                    FragmentLogin login = (FragmentLogin) getFragmentManager().findFragmentByTag(FragmentLogin.FRAGMENT_TAG);
                    if(login != null){
                        login.stopLoginActivity();
                    }
                }
            }
            else {
                super.onActivityResult(requestCode, resultCode, data);
            }
        } else {
            Log.d(TAG, "onActivityResult handled by Store Manager");
        }

        // Purchase finalized. Update user info (after a delay)
        final Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                SessionManager.getInstance().requestUserInfoUpdate();
            }
        }, 4000);
    }

    // Storing this here so any activity can display the window,
    // although for now only ActivityStart and ActivityNativeMain attempt
    // a login
    public void showBannedAccountMessage(Bundle args)
    {
        FragmentBannedAccount fragment = new FragmentBannedAccount();
        fragment.setArguments(args);
        fragment.show(getFragmentManager(), FragmentBannedAccount.FRAGMENT_TAG);
    }

    // -----------------------------------------------------------------------
    public StoreManager getStoreManager() {
        return mStoreMgr;
    }

    public void showWebCaptcha(boolean isSocial) {
        RobloxWebFragment fragment = new RobloxWebFragment();

        Bundle args = new Bundle();
        args.putBoolean("showCaptcha", true);
        args.putBoolean("isSocial", isSocial);
        fragment.setArguments(args);

        fragment.loadURL(RobloxSettings.captchaUrl());
        fragment.setStyle(DialogFragment.STYLE_NO_TITLE, fragment.getTheme());
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.add(fragment, RobloxWebFragment.FRAGMENT_TAG_CAPTCHA);
        ft.commitAllowingStateLoss();
    }

    public void showCaptcha(boolean isSocial) {
        if(AndroidAppSettings.EnableLoginLogoutSignupWithApi()) {
            Intent intent = new Intent(this, ReCaptchaActivity.class);
            intent.putExtra(ReCaptchaActivity.USERNAME, SessionManager.getInstance().getUsername());
            intent.putExtra(ReCaptchaActivity.ACTION, ReCaptchaActivity.ACTION_LOGIN);
            startActivityForResult(intent, ReCaptchaActivity.ACTIVITY_REQUEST_CODE);
        }
        else {
            showWebCaptcha(isSocial);
        }
    }

    private void hideCaptchaCommon() {
        // Find captcha web fragment
        Fragment fragWeb = getFragmentManager().findFragmentByTag(RobloxWebFragment.FRAGMENT_TAG_CAPTCHA);
        FragmentTransaction ft = getFragmentManager().beginTransaction();

        if (fragWeb != null && fragWeb.isVisible()) {
            // Remove fragment
            ft.remove(fragWeb); // need to call remove, not hide - otherwise, an empty FrameLayout never gets removed
            ft.commitAllowingStateLoss();
        }
    }

    public void hideCaptcha() {
        hideCaptchaCommon();

        // Trigger another login
        SessionManager.getInstance().retryLogin(this, true);
    }

    public void hideSocialCaptcha() {
        hideCaptchaCommon();
        SocialManager.getInstance().facebookSignupStart(null);
    }

}

