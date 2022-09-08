package com.roblox.client;


import android.animation.Animator;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings.Secure;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.webkit.WebView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.TextView;

import com.facebook.appevents.AppEventsLogger;
import com.google.android.gms.ads.identifier.AdvertisingIdClient;
import com.google.android.gms.ads.identifier.AdvertisingIdClient.Info;
import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.common.GooglePlayServicesRepairableException;
import com.jirbo.adcolony.AdColony;
import com.mobileapptracker.MobileAppTracker;
import com.roblox.client.components.RbxAnimHelper;
import com.roblox.client.components.RbxButton;
import com.roblox.client.components.RbxLinearLayout;
import com.roblox.client.http.CookieConsistencyChecker;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.SessionManager;

import java.io.File;
import java.io.IOException;

public class ActivityStart extends RobloxActivity implements NotificationManager.Observer, View.OnClickListener {

    public static WebView mSiteAlertWebView = null;
    MobileAppTracker mobileAppTracker = null;
    private String TAG = "ActivityStart";

    TextView mRobuxBalanceTextView = null;
    TextView mTicketsBalanceTextView = null;

    TextView mFinePrintTextView = null;

    private static ProgressDialog mProgressSpinner = null;

    // Analytics
    private static String ctx = "landing";
    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


//        if (RobloxSettings.isFirstLaunch()) {
//            RbxAnalytics.fireAppLaunch("appLaunch");
//            RobloxSettings.finishedFirstLaunch();
//            // We need to call this endpoint first to ensure all future requests have the proper
//            // cookies from the web
//            RbxHttpPostRequest deviceIdReq = new RbxHttpPostRequest(RobloxSettings.deviceIDUrl(),
//                    "mobileDeviceId=" + RobloxSettings.mDeviceId, null, new OnRbxHttpRequestFinished() {
//                @Override
//                public void onFinished(HttpResponse response) {
//                    try {
//                        RobloxSettings.mBrowserTrackerId = new JSONObject(response.responseBodyAsString()).optString("browserTrackerId", "");
//                        CookieConsistencyChecker.secondStageCheck();
//                    } catch (JSONException e) {
//                        e.printStackTrace();
//                    }
//                    launchSecondStep();
//                }
//            });
//            deviceIdReq.execute();
//        }
//        else
            RbxAnalytics.fireScreenLoaded(ctx);

        setContentView(R.layout.activity_start);

        mProgressSpinner = new ProgressDialog(this);
        mProgressSpinner.setMessage(getString(R.string.LoggingInSpinnerText));

        // Facebook integration for Ads attribution
        //com.facebook.AppEventsLogger.activateApp(this);
        AppEventsLogger.activateApp(this); // FBSDK 4.x

        // Mobile App Tracking
        this.initializeMAT();

        Utils.mDeviceDisplayMetrics = getResources().getDisplayMetrics();

        mFinePrintTextView = (TextView)findViewById(R.id.tvStartFinePrint);

        if (mFinePrintTextView != null)
            Utils.makeTextViewHtml(this, mFinePrintTextView, getString(R.string.TermsLicensingPrivacy));

        // TODO: Remove this once FB is live for a few weeks
        RbxButton gigyaButton = (RbxButton)findViewById(R.id.gigya_facebook_login);
        if (AndroidAppSettings.EnableFacebookAuth()) {
            gigyaButton.setVisibility(View.VISIBLE);
            findViewById(R.id.gigya_facebook_icon).setVisibility(View.VISIBLE);
            gigyaButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                RbxAnalytics.fireButtonClick(ctx, "socialSignIn");
                SocialManager.getInstance().facebookLoginStart();
                }
            });
        } else {
            findViewById(R.id.activity_start_facebook_container).setVisibility(View.GONE);
        }

        Utils.alertIfNetworkNotConnected();
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onStart() {
        super.onStart();

        this.initializeMenu();
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onResume() {
        super.onResume();

        NotificationManager.getInstance().addObserver(this);

        SessionManager sm = SessionManager.getInstance();
        sm.mCurrentActivity = this;

        if (sm.getUsername().isEmpty() || sm.getPassword().isEmpty())
            UpgradeCheckHelper.checkForUpdate(this);

        // Mobile App Tracking
        // Get source of open for app re-engagement
        mobileAppTracker.setReferralSources(this);
        // MAT will not function unless the measureSession call is included
        mobileAppTracker.measureSession();

        // AdColony
        AdColony.resume(this);

        DoProtocolRegistrationCheck();
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onPause() {
        super.onPause();

        NotificationManager.getInstance().removerObserver(this);

        // AdColony
        AdColony.pause();
    }


    // -----------------------------------------------------------------------
    private void initializeMenu() {

        Button loginButton = (Button)findViewById(R.id.login_button);
        loginButton.setOnClickListener(this);

        Button signupButton = (Button)findViewById(R.id.signup_button);
        signupButton.setOnClickListener(this);

        Button playNowButton = (Button)findViewById(R.id.play_now_button);
        playNowButton.setOnClickListener(this);

        // Internal only
//        final Spinner spinnerEnvironment = (Spinner)findViewById(R.id.spinner_environment);
//        if (RobloxSettings.isInternalBuild()) {
//            // Populate spinner with environment list
//            ArrayAdapter<CharSequence> envAdapter = ArrayAdapter.createFromResource(this, (RobloxSettings.isTablet() ? R.array.EnvironmentArray : R.array.EnvironmentArrayMobile), android.R.layout.simple_spinner_item);
//            envAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
//            spinnerEnvironment.setAdapter(envAdapter);
//            // This forces the element to be set before we establish the listener -
//            // if this isn't done, the event below will fire every time this activity
//            // is created (which is every time we go back to the grid screen on phones).
//            spinnerEnvironment.setSelection(0, false);
//
//            spinnerEnvironment.post(new Runnable() {
//                public void run() {
//                    spinnerEnvironment.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
//                        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
//                            if (RobloxSettings.isTablet())
//                                RobloxSettings.setBaseUrlDebug(parent.getItemAtPosition(pos).toString());
//                            else {
//                                RobloxSettings.setBaseUrlDebug(parent.getItemAtPosition(pos).toString().replace("m.","www."));
//                                RobloxSettings.setBaseMobileUrlDebug(parent.getItemAtPosition(pos).toString());
//								}
//                        }
//
//                        public void onNothingSelected(AdapterView<?> parent) {
//                            // Stub
//                        }
//                    });
//                }
//            });
//        } else {
//            spinnerEnvironment.setVisibility(View.GONE);
//        }
    }

    // -----------------------------------------------------------------------
    private void initializeMAT() {
        MobileAppTracker.init(getApplicationContext(), "18714", "4316dbf38e776530b30b954d3786bd41");
        mobileAppTracker = MobileAppTracker.getInstance();

        // Only enable for testing purposes
        //mobileAppTracker.setDebugMode(true);
        //mobileAppTracker.setAllowDuplicates(true);

        // Check if the user ever was logged in.
        // HACK: Since there is no other trace to check,
        //       use a GoogleAnalytics to see if this is the first time the user opens the app.
        File googleAnalyticsTrace = new File(this.getApplicationContext().getFilesDir() + "/gaClientId");
        int lastVersionCode = RobloxSettings.getKeyValues().getInt(LAST_APP_VERSION_KEY, 0);
        boolean isExistingUser = googleAnalyticsTrace.exists() || lastVersionCode != 0;
        if (isExistingUser) {
            mobileAppTracker.setExistingUser(true);
        }

        // Collect Google Play Advertising ID
        new Thread(new Runnable() {
            @Override
            public void run() {
                // See sample code at http://developer.android.com/google/play-services/id.html
                try {
                    Info adInfo = AdvertisingIdClient.getAdvertisingIdInfo(getApplicationContext());
                    mobileAppTracker.setGoogleAdvertisingId(adInfo.getId(), adInfo.isLimitAdTrackingEnabled());
                } catch (IOException e) {
                    // Unrecoverable error connecting to Google Play services (e.g.,
                    // the old version of the service doesn't support getting AdvertisingId).
                    mobileAppTracker.setAndroidId(Secure.getString(getContentResolver(), Secure.ANDROID_ID));
                } catch (GooglePlayServicesNotAvailableException e) {
                    // Google Play services is not available entirely.
                    mobileAppTracker.setAndroidId(Secure.getString(getContentResolver(), Secure.ANDROID_ID));
                } catch (GooglePlayServicesRepairableException e) {
                    // Encountered a recoverable error connecting to Google Play services.
                    mobileAppTracker.setAndroidId(Secure.getString(getContentResolver(), Secure.ANDROID_ID));
                } catch (NullPointerException e) {
                    // getId() is sometimes null
                    mobileAppTracker.setAndroidId(Secure.getString(getContentResolver(), Secure.ANDROID_ID));
                }
            }
        }).start();
    }



    // -----------------------------------------------------------------------
    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.login_button:
                this.onLoginButtonClicked();
                break;
            case R.id.signup_button:
                this.onSignUpButtonClicked();
                break;
            case R.id.play_now_button:
                this.onPlayNowButtonClicked();
                break;
        }
    }

    // -----------------------------------------------------------------------
    private void onLoginButtonClicked() {
        RbxAnalytics.fireButtonClick(ctx, "login");
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        FragmentLogin fragment = new FragmentLogin();
        ft.add(R.id.activity_start, fragment, FragmentLogin.FRAGMENT_TAG);
        ft.commit();
    }

    // -----------------------------------------------------------------------
    private void onSignUpButtonClicked() {
        RbxAnalytics.fireButtonClick(ctx, "signup");
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        FragmentSignUp fragment = new FragmentSignUp();
        ft.add(R.id.activity_start, fragment, FragmentSignUp.FRAGMENT_TAG);
        ft.commit();
    }

    // -----------------------------------------------------------------------
    private void onPlayNowButtonClicked() {
        RbxAnalytics.fireButtonClick(ctx, "playNow");

        final Activity actRef = this;
        WelcomeAnimation.fadeInBackground(this, new WelcomeAnimationListener() {
            @Override
            public void onAnimationFinished() {
                Intent intent = new Intent(actRef, ActivityNativeMain.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
                startActivity(intent);
            }
        });
    }

    private void onCreateUsernameRequested(Bundle args) {
//        RbxAnalytics.fireButtonClick(ctx, "signup");
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        FragmentCreateUsername fragment = new FragmentCreateUsername();
        fragment.setArguments(args);
        ft.add(R.id.activity_start, fragment, FragmentCreateUsername.FRAGMENT_TAG);
        ft.commit();
    }

    // -----------------------------------------------------------------------
    public void onAbout(View view) {
        RbxAnalytics.fireButtonClick(ctx, "about");
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        FragmentAbout fragment = new FragmentAbout();
        ft.add(R.id.activity_start, fragment, FragmentAbout.FRAGMENT_TAG);
        ft.commit();

        Utils.sendAnalytics("ActivityNativeMain", "about");
    }

    // -----------------------------------------------------------------------
    @Override
    public void handleNotification(int notificationId, Bundle userParam) {
        switch (notificationId) {

            case NotificationManager.EVENT_USER_LOGIN: {
                this.onPlayNowButtonClicked();
                break;
            }
            case NotificationManager.EVENT_USER_LOGIN_STARTED: {
                if (mProgressSpinner != null) {
                    // We only want to show the spinner if we're doing an automatic login
                    Fragment f = getFragmentManager().findFragmentByTag(FragmentLogin.FRAGMENT_TAG);
                    if (f == null)
                        mProgressSpinner.show();
                }
                break;
            }
            case NotificationManager.EVENT_USER_LOGIN_STOPPED: {
                closeSpinner();
                break;
            }
            case NotificationManager.EVENT_USER_CAPTCHA_SOLVED:
                closeSpinner();
                hideCaptcha();
                break;
            case NotificationManager.EVENT_USER_CAPTCHA_REQUESTED:
                closeSpinner();
                showCaptcha(false);
                break;
            case NotificationManager.EVENT_USER_CREATE_USERNAME_REQUESTED:
                onCreateUsernameRequested(userParam);
                break;
            case NotificationManager.EVENT_TEST:
                testActivityChange();
                break;
            case NotificationManager.EVENT_USER_CREATE_USERNAME_SUCCESSFUL:
                onUsernameCreated();
                break;
            case NotificationManager.EVENT_USER_CAPTCHA_SOCIAL_REQUESTED:
                closeSpinner();
                showCaptcha(true);
                break;
            case NotificationManager.EVENT_USER_CAPTCHA_SOCIAL_SOLVED:
                closeSpinner();
                hideSocialCaptcha();
                break;
            default:
                break;
        }
    }

    public void closeSpinner() {
        if (mProgressSpinner != null) {
            mProgressSpinner.hide();
        }
    }

    private void onUsernameCreated() {
        Fragment fragCreateUsername = getFragmentManager().findFragmentByTag(FragmentCreateUsername.FRAGMENT_TAG);
        if (fragCreateUsername != null && fragCreateUsername.isVisible())
            ((FragmentCreateUsername)fragCreateUsername).onCreateSuccess();
    }

    private void testActivityChange() {
        Intent intent = new Intent(this, ActivityNativeMain.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
        startActivity(intent);
    }

    @Override
    public void onBackPressed() {
        Fragment fragSignup = getFragmentManager().findFragmentByTag(FragmentSignUp.FRAGMENT_TAG);
        Fragment fragLogin = getFragmentManager().findFragmentByTag(FragmentLogin.FRAGMENT_TAG);
        FragmentTransaction ft = getFragmentManager().beginTransaction();

        if (fragSignup != null && fragSignup.isVisible())
            ((FragmentSignUp)fragSignup).closeDialog();
        else if (fragLogin != null && fragLogin.isVisible())
            ((FragmentLogin)fragLogin).closeDialog();
        else {
            Fragment fragCreateUsername = getFragmentManager().findFragmentByTag(FragmentCreateUsername.FRAGMENT_TAG);
            if (fragCreateUsername != null && fragCreateUsername.isVisible())
                ((FragmentCreateUsername)fragCreateUsername).closeDialog();
        }
    }


    // -----------------------------------------------------------------------
    void DoProtocolRegistrationCheck()
    {
        Log.i(TAG, "in DoProtocolRegistrationCheck");
        Intent incomingIntent = getIntent();
        if( incomingIntent == null )
        {
            Log.e(TAG, "Launching Web View Activity without Intent.");
            return;
        }

        if( incomingIntent.getAction() == Intent.ACTION_VIEW )
        {
            RbxAnalytics.fireAppLaunch("protocolLaunch");
            String str = incomingIntent.getDataString();
            String placeID = str.replace("robloxmobile://", "").replace("placeID=", "");
            Log.i(TAG, "Launching from Protocol, Place ID: " + placeID);

            if(str.matches("robloxmobile://\\??(placeID=)?(\\d+)")) {
                Intent intent = new Intent(ActivityStart.this, ActivityNativeMain.class);
                intent.putExtra("roblox_placeid", placeID);
                intent.putExtra("launchWithProtocol", true);

                startActivity(intent);
                finish();
            }
        }
    }
}
