package com.roblox.client;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.jirbo.adcolony.AdColony;
import com.roblox.client.components.RbxAnimHelper;
import com.roblox.client.components.RbxButton;
import com.roblox.client.components.RbxTextView;
import com.roblox.client.http.CookieConsistencyChecker;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetRequest;
import com.roblox.client.http.RbxHttpPostRequest;
import com.roblox.client.http.WebkitCookieManagerProxy;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.RbxReportingManager;
import com.roblox.client.managers.SessionManager;

import org.json.JSONException;
import org.json.JSONObject;

public class ActivitySplash extends RobloxActivity implements NotificationManager.Observer {
    private RbxTextView mProgressText = null;
    final Activity mActivityRef = this;
    private ProgressBar mProgressSpinner = null;
    private View mBackground = null;
    private boolean enableTransitionAnimation = true;
    private Handler mUIThreadHandler = null;
    private RbxHttpGetRequest settingsReq = null;

    private long startTime = 0;
    private long retryLoginStartTime = 0;
    private long deviceInitResponseTime = 0;
    private long eventsRequestResponseTime = 0;
    // TODO: Remove when auth cookie analytics are finished
    int preNumAuthCookiesPresent = 0;
    int preLengthOfFirstAuthCookie = 0;
    int postNumAuthCookiesPresent = 0;
    int postLengthOfFirstAuthCookie = 0;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        /**** TODO: Will remove after data gathered ****/
        // No IF here, flags haven't loaded yet
        int[] result = Utils.getNumberAndLengthOfAuthCookies();
        preNumAuthCookiesPresent = result[0];
        preLengthOfFirstAuthCookie = result[1];
        /***** END TODO *****/

        startTime = System.currentTimeMillis();

        if(AndroidAppSettings.EnableInferredCrashReporting()) {
            SharedPreferences preferences = RobloxSettings.getKeyValues();
            boolean crash = preferences.getBoolean("ROBLOXCrash", false);

            new InfluxBuilderV2("Android-RobloxPlayer-SessionReport-Inferred")
                    .addField("Session", crash ? "Crash" : "Success")
                    .fireReport();

            RbxHttpPostRequest report = new RbxHttpPostRequest(RobloxSettings.ephemeralCounterUrl() +
                    RobloxSettings.ephemeralCounterParams(crash ? "Android-ROBLOXPlayer-Session-Inferred-Crash" : "Android-ROBLOXPlayer-Session-Inferred-Success", 1),
                    null, null, new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    // don't need to do anything
                }
            });
            report.execute();
        }

        Log.i("ActivitySplash", "ActivitySplash onCreate");
        setContentView(R.layout.activity_splash);

        mUIThreadHandler = new Handler(Looper.getMainLooper());

        LayoutInflater inflater = LayoutInflater.from(this);
        LinearLayout container = (LinearLayout)findViewById(R.id.splash_start_container);
        if (container != null) {
            inflater.inflate(R.layout.activity_start, container);
        }

        View startLogo = findViewById(R.id.activity_start_logo);
        if (startLogo != null)
            startLogo.setVisibility(View.INVISIBLE);

        mProgressText = (RbxTextView)findViewById(R.id.splash_progress_text);
        mProgressSpinner = (ProgressBar)findViewById(R.id.splash_progress_bar);
        mBackground = findViewById(R.id.splash_background);
        RobloxSettings.finishedFirstLaunch();

        if (!Utils.isNetworkConnected()) {
            launchStartActivity();
            return;
        }

        // If loading takes longer than 15s, we'll just take the user to the landing page
        mUIThreadHandler.postDelayed(timeoutToLanding, 15000);
        CookieConsistencyChecker.firstStageCheck();

        RobloxSettings.mDeviceId = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);

        // We need to call this endpoint first to ensure all future requests have the proper
        // cookies from the web
        RbxHttpPostRequest deviceIdReq = new RbxHttpPostRequest(RobloxSettings.deviceIDUrl(),
                "mobileDeviceId=" + RobloxSettings.mDeviceId, null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                try {
                    /**** TODO: Will remove after data gathered ****/
                    // No IF, flags haven't loaded yet
                    int[] result = Utils.getNumberAndLengthOfAuthCookies();
                    postNumAuthCookiesPresent = result[0];
                    postLengthOfFirstAuthCookie = result[1];
                    /**** TODO END ****/

                    RobloxSettings.mBrowserTrackerId = new JSONObject(response.responseBodyAsString()).optString("browserTrackerId", "");
                    CookieConsistencyChecker.secondStageCheck();
                } catch (JSONException e) {
                    e.printStackTrace();
                }
                // Moving this here so that we're guaranteed the first RbxEvent has a BTID
                RbxAnalytics.fireAppLaunch("appLaunch");
                // We need to save this because we can't fire an Influx event yet - we load the DB credentials via FFlags
                deviceInitResponseTime = response.responseTime();
                launchSecondStep(response.responseTime());
            }
        });
        deviceIdReq.execute();

        if (RobloxSettings.eventsData == null) {
            // Loading this data here to ensure it's ready by the time we open the More page
            // For the rare cases it fails to load (or if you're on a test site and hit the cookie constraint)
            // we attempt to load the data (if null) in WebviewInterface
            RbxHttpGetRequest eventsReq = new RbxHttpGetRequest(RobloxSettings.eventsUrl(), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    eventsRequestResponseTime = response.responseTime();
                    if (!response.responseBodyAsString().isEmpty()) {
                        try {
                            String nResponse = "{\"Data\":" + response.responseBodyAsString() + "}"; // returned as a JSONArray, need to wrap
                            // it in an object so we can attempt to parse
                            JSONObject j = new JSONObject(nResponse);
                            if (j != null)
                                RobloxSettings.eventsData = j.toString();
                        } catch (Exception e) {
                            Log.i("EventsRequest", e.toString());
                            RobloxSettings.eventsData = null;
                        }
                    }
                }
            });
            eventsReq.execute();
        }

        SessionManager.mCurrentActivity = this;
        NotificationManager.getInstance().addObserver(this);

        TextView mFinePrintTextView = (TextView)findViewById(R.id.tvStartFinePrint);

        if (mFinePrintTextView != null)
            Utils.makeTextViewHtml(this, mFinePrintTextView, getString(R.string.TermsLicensingPrivacy));


    }

    @Override
    protected void onDestroy() {
        NotificationManager.getInstance().removerObserver(this);
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void launchSecondStep(final long firstStepTime) {
        settingsReq = AndroidAppSettings.fetchFromServer(new AndroidAppSettings.FetchSettingsCallback() {
            @Override
            public void onFinished(boolean success, HttpResponse responseData) {
                if (success) {
                    hideOrShowGigya();
                    doAfterFetchAppSettings(responseData.responseTime()); // calling this after we get the response to ensure we can set the ZoneId
                } else {
                    doAfterFetchAppSettings(responseData.responseTime());
                }
            }
        });
    }

    private void hideOrShowGigya() {
        RbxButton gigyaButton = (RbxButton)findViewById(R.id.gigya_facebook_login);

        // Bit hacky, but this is only temporary - need to make sure the fake Landing page
        // we load in the bg looks the same as the real one
        // TODO: Remove this once FB is live for a few weeks
        if (AndroidAppSettings.EnableFacebookAuth()) {
            gigyaButton.setVisibility(View.VISIBLE);
            findViewById(R.id.gigya_facebook_icon).setVisibility(View.VISIBLE);
            gigyaButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    SocialManager.getInstance().facebookLoginStart();
                }
            });
        } else {
            findViewById(R.id.activity_start_facebook_container).setVisibility(View.GONE);
        }
    }

    // -----------------------------------------------------------------------
    private void doAfterFetchAppSettings(long settingsResponseTime) {
        RbxReportingManager.fireAppStartupEvent(RbxReportingManager.INFLUX_V_REQNAME_FETCHEVENTS, eventsRequestResponseTime);
        RbxReportingManager.fireAppStartupEvent(RbxReportingManager.INFLUX_V_REQNAME_DEVICEINIT, deviceInitResponseTime); // see note in deviceInit callback
        RbxReportingManager.fireAppStartupEvent(RbxReportingManager.INFLUX_V_REQNAME_FETCHAPPSETTINGS, settingsResponseTime);

        /**** TODO: Will remove after data gathered ****/
        if (AndroidAppSettings.EnableAuthCookieAnalytics()) {
            if (preNumAuthCookiesPresent != postNumAuthCookiesPresent || preLengthOfFirstAuthCookie != postLengthOfFirstAuthCookie) {
                RbxReportingManager.fireAuthCookieAnalytics(preNumAuthCookiesPresent, preLengthOfFirstAuthCookie, RbxReportingManager.INFLUX_V_ENDPOINT_DEVICEINIT_PRE);
                RbxReportingManager.fireAuthCookieAnalytics(postNumAuthCookiesPresent, postLengthOfFirstAuthCookie, RbxReportingManager.INFLUX_V_ENDPOINT_DEVICEINIT_POST);
            }
        }
        /**** TODO END ****/

        // Stop the timeout runner
        mUIThreadHandler.removeCallbacks(timeoutToLanding);

        // Read current version name
        String versionName = "";
        try {
            versionName = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (PackageManager.NameNotFoundException e) {

        }

        String adColonyZoneId = (AndroidAppSettings.AdColonyZoneId() == null ? getResources().getString(R.string.AdColonyZoneId) : AndroidAppSettings.AdColonyZoneId());
        String adColonyAppId = (AndroidAppSettings.AdColonyAppId() == null ? getResources().getString(R.string.AdColonyAppId) : AndroidAppSettings.AdColonyAppId());

        // Initialize AdColony
        String options = "version:" + versionName + ",store:google";
        AdColony.configure(this, options, adColonyAppId, adColonyZoneId);

        SessionManager.getInstance().retryLogin(this);
        mUIThreadHandler.postDelayed(timeoutToLanding, 15000); // timeout for login attempt

        // this block will run if login is not started...
        Handler handler = new Handler(Looper.getMainLooper());
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (!SessionManager.getInstance().willStartLogin() && !mLoginInProgress) {
                    mUIThreadHandler.removeCallbacks(timeoutToLanding);
                    if (enableTransitionAnimation)
                        startAnimationToLandingScreen();
                    else {
                        launchStartActivity();
                    }
                }
            }
        }, 1500);
    }

    @Override
    public void handleNotification(int notificationId, Bundle userParams) {
        String socialNetwork = "";

        if (userParams != null) socialNetwork = userParams.getString("socialNetwork");

        switch (notificationId) {
            case NotificationManager.EVENT_USER_LOGIN_STARTED: {
                retryLoginStartTime = System.currentTimeMillis();
                mLoginInProgress = true;
                onLoginStarted(socialNetwork);
                break;
            }
            case NotificationManager.EVENT_USER_LOGIN: {
                influxRetryLoginHelper();
                mUIThreadHandler.removeCallbacks(timeoutToLanding);
                if (enableTransitionAnimation)
                    startAnimationToLandingScreen();
                else {
                    launchMainActivity();
                }
                break;
            }
            case NotificationManager.EVENT_USER_LOGIN_STOPPED: {
                influxRetryLoginHelper();
                if (enableTransitionAnimation)
                    startAnimationToLandingScreen();
                else
                    launchStartActivity();
                break;
            }
        }
    }

    private final int ANIMATION_DURATION = 200; // in ms
    private boolean mLoginInProgress = false;
    private void onLoginStarted(String socialNetwork) {
        final String newText = (socialNetwork.isEmpty() ?
                getString(R.string.LoggingInSpinnerText) :
                getString(R.string.LoggingIn) + " with " + socialNetwork);
        // Fade out old text
        if (mProgressText != null) {
            AlphaAnimation fadeOut = new AlphaAnimation(1.0f, 0.0f);
            fadeOut.setDuration(ANIMATION_DURATION);
            fadeOut.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    final AlphaAnimation fadeIn = new AlphaAnimation(0.0f, 1.0f);
                    fadeIn.setDuration(ANIMATION_DURATION);

                    // Fade in new text
                    mProgressText.setText(newText);
                    mProgressText.setTextSize(Utils.pixelToDp(getResources().getDimensionPixelSize(R.dimen.splashFacebookTextSize)));
                    mProgressText.startAnimation(fadeIn);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });
            mProgressText.startAnimation(fadeOut);
        }
    }

    private void startAnimationToLandingScreen() {
        AlphaAnimation alpha = RbxAnimHelper.fadeOut(mProgressText, 400);
        alpha.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation animation) {

            }

            @Override
            public void onAnimationEnd(Animation animation) {
                mProgressText.setAlpha(0.0f);
                mProgressSpinner.setAlpha(0.0f);

                Utils.DoProtocolRegistrationCheck(getIntent(), new ProtocolLaunchCheckHandler() {
                    @Override
                    public void onParseFinished(String placeId) {
                        if (!placeId.isEmpty()) {
                            Intent intent = new Intent(ActivitySplash.this, ActivityNativeMain.class);
                            intent.putExtra("roblox_placeid", placeId);
                            intent.putExtra("launchWithProtocol", true);

                            startActivity(intent);
                            finish();
                        } else {
                            if (SessionManager.getInstance().getIsLoggedIn()) {
                                launchMainActivity();
                            } else {
                                animateBackgroundOut();
                            }
                        }
                    }
                });
            }

            @Override
            public void onAnimationRepeat(Animation animation) {

            }
        });
        mProgressText.startAnimation(alpha);
        mProgressSpinner.startAnimation(alpha);
    }

    private void animateBackgroundOut() {
        ValueAnimator animator = ValueAnimator.ofFloat(0.0f, -mBackground.getHeight());
        animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                mBackground.setY((Float) animation.getAnimatedValue());
            }
        });

        animator.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                if (!SessionManager.getInstance().getIsLoggedIn()) {
                    launchStartActivity();
                } else {
                    launchMainActivity();
                }
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        animator.setDuration(500);
        animator.setInterpolator(new AccelerateInterpolator());
        animator.start();
    }

    private void launchStartActivity() {
        // Send report to InfluxDb
        influxStartupFinishedHelper();
        Intent intent = new Intent(mActivityRef, ActivityStart.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        startActivity(intent);
        finish();
        overridePendingTransition(0, 0);
    }

    private void launchMainActivity() {
        influxStartupFinishedHelper();

        Intent intent = new Intent(mActivityRef, ActivityNativeMain.class);
        startActivity(intent);
        finish();
    }

    private void influxStartupFinishedHelper() {
        // Send report to InfluxDb
        RbxReportingManager.fireAppStartupEvent(RbxReportingManager.INFLUX_V_REQNAME_STARTUPFINISHED, System.currentTimeMillis() - startTime);
    }

    private void influxRetryLoginHelper() {
        RbxReportingManager.fireAppStartupEvent(RbxReportingManager.INFLUX_V_REQNAME_FETCHUSERINFO, System.currentTimeMillis() - retryLoginStartTime);
    }

    private final Runnable timeoutToLanding = new Runnable() {
        public void run()
        {
            // if we don't stop this request, the response could return after we've already
            // called finish() on the activity - and bad things could happen
            if (settingsReq != null && settingsReq.getStatus() != AsyncTask.Status.FINISHED)
                settingsReq.cancel(true);

            // we don't know if we've started a login at this point, but we'll attempt to
            // stop the requests if we did
            SessionManager.getInstance().stopLoginRequest();
            SocialManager.getInstance().stopLoginRequest();

            hideOrShowGigya();

            if (enableTransitionAnimation)
                startAnimationToLandingScreen();
            else
                launchStartActivity();
        }
    };
}
