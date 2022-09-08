package com.roblox.client;


import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ArgbEvaluator;
import android.animation.ValueAnimator;
import android.app.AlertDialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.widget.SearchView;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TabHost;
import android.widget.TextView;

import com.jirbo.adcolony.AdColony;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetRequest;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.SessionManager;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class ActivityNativeMain extends RobloxActivity implements NotificationManager.Observer, TabHost.OnTabChangeListener {

    TextView mRobuxBalanceTextView = null;
    TextView mTicketsBalanceTextView = null;

    private ReclickableTabHost mTabHost = null;
    private int mTabRequestedByUser = -1;
    private int mActiveTab = 0;
    private ArrayList<Fragment> mTabContents = new ArrayList<Fragment>();
    private Menu mMenu = null;
    private boolean mHideSearchIcon = false;

    private String TAG = "ActivityNativeMain";
    private boolean mIsForeground = false;

    private MenuItem mSearchMenuItem = null;
    private MenuItem mLogoutMenuItem = null;

    private boolean mUseSpecificIcons = false;
    private Integer mPrevColor = 0;

    private ValueAnimator mColorAnimation = null;

    private String mMoreCurrentHeader = null;
    private Integer mMoreCurrentColor = null;

    private boolean mHasFinishedSetup = false;
    private static String ctx = "nativeMain";
    public static String latestMorePage = "";

    private int mMoreIconOffId = R.drawable.icon_more2, mMoreIconOnId = R.drawable.icon_more2_on;

    private Toolbar mToolbar;

    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SessionManager.mCurrentActivity = this;

        // If the app crashes Android may restart it to this activity
        // in that case, we won't be properly logged in, so we should attempt a login if we have credentials
        if (!SessionManager.getInstance().getIsLoggedIn() && SessionManager.getInstance().willStartLogin())
            SessionManager.getInstance().retryLogin(this);

        if (!AndroidAppSettings.wereSettingsLoaded())
            AndroidAppSettings.fetchFromServer(null);


        if (RobloxSettings.isPhone())
            setContentView(R.layout.activity_main_phone);
        else
            setContentView(R.layout.activity_main);

        mToolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(mToolbar);

        launchOnStartupRequests();

        mTabHost = (ReclickableTabHost)findViewById(android.R.id.tabhost);
        mTabHost.setActivityRef(this);
        mTabHost.setOnTabChangedListener(this);
        mTabHost.setup();

        //if (SessionManager.getInstance().getIsLoggedIn())
        if (!AndroidAppSettings.EnableValuePages())
            createLoggedInTabs();
        else
            createGuestTabs();

        mMoreCurrentHeader = "More";
        mMoreCurrentColor = getResources().getColor(R.color.black);

        // Initialize action bar
        getSupportActionBar().setDisplayShowHomeEnabled(true);
        setToolbarIcon(R.drawable.ic_launcher_transparent);
        if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_home);
        getSupportActionBar().setTitle("Home");

        // Set title text color
        setToolbarTitleTextColor(getResources().getColor(R.color.white));
        mPrevColor = getResources().getColor(R.color.RbxGreen1);

        if (SessionManager.getInstance().getIsLoggedIn())
        {
            mTabHost.setCurrentTab(0);
            mTabRequestedByUser = 0;
        }
        else
        {
            mTabHost.setCurrentTab(1);
            mTabRequestedByUser = 1;

            getSupportActionBar().setTitle(R.string.GameWord);
            resetMenuButtons(false, false, true, false);
            changeSearchButton(2);
        }

        updateTabsStatus();
        setTabIndicators();

        mHasFinishedSetup = true;

        onTabChanged(mTabHost.getCurrentTabTag());

        // For now, we don't need the callback for anything
        WelcomeAnimation.start(this, null);
    }

    private void launchOnStartupRequests() {
        RbxHttpGetRequest notificationsRequest = new RbxHttpGetRequest(RobloxSettings.accountNotificationsUrl(), new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                if (!response.responseBodyAsString().isEmpty()) {
                    try {
                        JSONObject json = new JSONObject(response.responseBodyAsString());
                        RobloxSettings.setAccountNotificationSettings(json);
                        if ((RobloxSettings.isEmailNotificationEnabled() && RobloxSettings.getUserEmail().isEmpty())
                                || (RobloxSettings.isPasswordNotificationEnabled() && !RobloxSettings.userHasPassword)) {
                            mMoreIconOffId = R.drawable.icon_more2_notification;
                            mMoreIconOnId = R.drawable.icon_more2_on_notification;
                            updateIcons();
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
        notificationsRequest.execute();

        if (AndroidAppSettings.EnableFacebookAuth()) {
            RbxHttpGetRequest hasPasswordRequest = new RbxHttpGetRequest(RobloxSettings.userHasPasswordUrl(), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                    if (!response.responseBodyAsString().isEmpty()) {
                        try {
                            JSONObject json = new JSONObject(response.responseBodyAsString());
                            if (response.responseCode() == 200 && json.getString("status").equals("success"))
                                RobloxSettings.userHasPassword = json.optBoolean("result", true);
                        } catch (Exception e) {
                            Log.i(TAG, e.toString());
                        }
                    }
                }
            });
            hasPasswordRequest.execute();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        Utils.mDeviceDisplayMetrics = getResources().getDisplayMetrics();
    }

    // bit of a stub - expand if we ever need to implement more notifications
    private void updateIcons() {
        View tab;
        int iconId = 0, tabId = 0;

        if (RobloxSettings.isPhone()) tabId = 4;
        else tabId = 5;

        tab = mTabHost.getTabWidget().getChildAt(tabId); // more
        iconId = ( mActiveTab == tabId ? mMoreIconOnId : mMoreIconOffId);

        ((ImageView)tab.findViewById(android.R.id.icon)).setImageResource(iconId);
    }

    public void clearSettingsNotification () {
        if (!RobloxSettings.isEmailNotificationEnabled() && !RobloxSettings.isPasswordNotificationEnabled()) {
            mMoreIconOffId = R.drawable.icon_more2;
            mMoreIconOnId = R.drawable.icon_more2_on;

            updateIcons();

            // Get the more page - for now, we're guaranteed to be on the More page
            ((RobloxWebFragment) mTabContents.get(mActiveTab)).getJavascriptInterface().clearSettingsNotification();
        }
    }

    OnRbxHttpRequestFinished sessionCheckHandler = new OnRbxHttpRequestFinished(){
        @Override
        public void onFinished(HttpResponse response) {
            if(response.responseCode() != 200){
                SessionManager.getInstance().doLogout(false);
            }
        }
    };

    // -----------------------------------------------------------------------
    @Override
    protected void onResume() {
        super.onResume();
        UpgradeCheckHelper.checkForUpdate(this);
        if(SessionManager.getInstance().getIsLoggedIn()){
            // check if user's session is still valid
            SessionManager.getInstance().doSessionCheck(sessionCheckHandler);
        }
        NotificationManager.getInstance().addObserver(this);

        SessionManager.mCurrentActivity = this;

        // Check if there was a crash and report it.
        {
            File file = new File(getCacheDir(), RobloxSettings.exceptionReasonFilename());
            if (file.exists())
            {
                List<String> lines = Utils.readTextFile(file.toString());
                file.delete();
                Utils.alertUnexpectedError(lines.get(0));
            }
        }

        Utils.onRateMeMaybe(this);

        // AdColony
        AdColony.resume(this);

    	// Give the GL Activity time to shut down before killing it.
    	// Hopefully this will reduce the crash guard reports.
        mIsForeground = true;
        {
            final Handler handler = new Handler(getMainLooper());
            handler.postDelayed(new Runnable() {
                public void run() {
                    if (mIsForeground) {
                        Utils.killBackgroundProcesses(ActivityNativeMain.this);
                    }
                }
            }, 2000);
        }

        Intent incomingIntent = getIntent();

        if (incomingIntent != null && incomingIntent.getBooleanExtra("launchWithProtocol", false) && incomingIntent.getStringExtra("roblox_placeid") != null)
        {
            int placeId = 0;
            try {
                placeId = Integer.parseInt(incomingIntent.getStringExtra("roblox_placeid"));
            }
            finally {
                if(placeId > 0) {
                    Bundle params = new Bundle();
                    params.putInt("placeId", placeId);
                    params.putInt("requestType", 0);
                    launchGame(params);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    @Override
    protected void onPause() {
        super.onPause();

        NotificationManager.getInstance().removerObserver(this);

        // AdColony
        AdColony.pause();

        mIsForeground = false;
    }

    private void createGuestTabs() {
        // Home/games pages are the same on both tablet & phone
        createValueSectionTab(FragmentValuePage.PAGE.HOME, "Home", R.string.HomeWord, R.drawable.icon_home);
        createWebSectionTab(RobloxSettings.gamesUrl(), "Games", R.string.GameWord, R.drawable.icon_game);
        Utils.sendAnalytics("GamesUrl", RobloxSettings.gamesUrlBroken()); // this is just here for temporary analytics. we're not proud of it.
        if (RobloxSettings.isPhone()) {
            createValueSectionTab(FragmentValuePage.PAGE.FRIENDS, "Friends", R.string.FriendsWord, R.drawable.icon_friends);
            createValueSectionTab(FragmentValuePage.PAGE.MESSAGES, "Messages", R.string.MessagesWordPhone, R.drawable.icon_messages);
            createValueSectionTab(FragmentValuePage.PAGE.MORE, "More", R.string.MoreWord, mMoreIconOffId);
        }
        else {
            createValueSectionTab(FragmentValuePage.PAGE.CATALOG, "Catalog", R.string.CatalogWord, R.drawable.icon_catalog);
            createValueSectionTab(FragmentValuePage.PAGE.FRIENDS, "Friends", R.string.FriendsWord, R.drawable.icon_friends);
            createValueSectionTab(FragmentValuePage.PAGE.MESSAGES, "Messages", R.string.MessagesWord, R.drawable.icon_messages);
            createValueSectionTab(FragmentValuePage.PAGE.MORE, "More", R.string.MoreWord, mMoreIconOffId);
        }
    }

    private void createLoggedInTabs() {
        // Home/games pages are the same on both tablet & phone
        createWebSectionTab(RobloxSettings.homeUrl(), "Home", R.string.HomeWord, R.drawable.icon_home);
        createWebSectionTab(RobloxSettings.gamesUrl(), "Games", R.string.GameWord, R.drawable.icon_game);
        Utils.sendAnalytics("GamesUrl", RobloxSettings.gamesUrlBroken()); // this is just here for temporary analytics. we're not proud of it.
        // Create webviews
        if (RobloxSettings.isPhone()) {
            createWebSectionTab(RobloxSettings.friendsUrl(), "Friends", R.string.FriendsWord, R.drawable.icon_friends);
            createWebSectionTab(RobloxSettings.messagesUrl(), "Messages", R.string.MessagesWordPhone, R.drawable.icon_messages);
            createLocalWebSectionTab("more_phone.html", "More", R.string.MoreWord, mMoreIconOffId);
        }
        else {
            createWebSectionTab(RobloxSettings.catalogUrl(), "Catalog", R.string.CatalogWord, R.drawable.icon_catalog);
            createWebSectionTab(RobloxSettings.friendsUrl(), "Friends", R.string.FriendsWord, R.drawable.icon_friends);
            createWebSectionTab(RobloxSettings.messagesUrl(), "Messages", R.string.MessagesWord, R.drawable.icon_messages);
            createLocalWebSectionTab("more.html", "More", R.string.MoreWord, mMoreIconOffId);
        }
    }

    // -----------------------------------------------------------------------
    private void createWebSectionTab(String url, String tag, int title_id, int icon_id) {

        RobloxWebFragment fragment = new RobloxWebFragment();
        mTabContents.add(fragment);

        fragment.setDefaultUrl(url);
        fragment.loadURL(url);

        // Setup the tab indicator
        View indicator = createTabIndicator(tag, title_id, icon_id);
    }

    private void createValueSectionTab(FragmentValuePage.PAGE page, String tag, int title_id, int icon_id) {
        FragmentValuePage fragment = new FragmentValuePage();
        Bundle args = new Bundle();
        args.putSerializable("page", page);
        fragment.setArguments(args);
        mTabContents.add(fragment);

        // Setup the tab indicator
        createTabIndicator(tag, title_id, icon_id);
    }

    private void createLocalWebSectionTab(String url, String tag, int title_id, int icon_id) {
        RobloxWebFragment fragment = new RobloxWebFragment();
        Bundle args = new Bundle();
        args.putBoolean("enablePullToRefresh", false);
        fragment.setArguments(args);
        mTabContents.add(fragment);

        fragment.setDefaultUrl("file:///android_asset/html/" + url);
        fragment.loadURL("file:///android_asset/html/" + url);

        // Setup the tab indicator
        createTabIndicator(tag, title_id, icon_id);
    }

    private View createTabIndicator(String tag, int title_id, int icon_id){

        // Setup the tab indicator
        View button = getLayoutInflater().inflate(R.layout.tab_button_layout, null, false);

        ImageView icon = (ImageView) button.findViewById(android.R.id.icon);
        icon.setImageResource(icon_id);

        TextView textView = (TextView) button.findViewById(android.R.id.title);
        textView.setText(title_id);

        // TO-DO: REMOVE THIS HACKY CRAP
        if (getResources().getDisplayMetrics().density < 2.0)
            textView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 8);

        // Add the tab to the tab widget
        TabHost.TabSpec spec = mTabHost.newTabSpec(tag)
                .setIndicator(button)
                .setContent( new TabHost.TabContentFactory() {
                    @Override
                    public View createTabContent(String s) {
                        // Dummy content
                        return new View(ActivityNativeMain.this);
                    }
                });

        mTabHost.addTab(spec);

        return button;
    }

    // -----------------------------------------------------------------------
    private void updateTabsStatus() {
        if(SessionManager.getInstance().getIsLoggedIn()) {
            if(mTabRequestedByUser > 0) {
                mTabHost.setCurrentTab(mTabRequestedByUser);
                mTabRequestedByUser = -1;
            }
        } else {
            if(mTabHost.getCurrentTab() == 0) {
                mTabHost.setCurrentTab(1);
            }
        }
    }

    // -----------------------------------------------------------------------
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Hang onto this reference for later
        mMenu = menu;
        if (SessionManager.getInstance().getIsLoggedIn()) {
            resetMenuButtons(true, true, true, false);
            changeSearchButton(1);
        }
        else {
            resetMenuButtons(false, false, true, false);
            changeSearchButton(2);
        }
        return super.onCreateOptionsMenu(menu);
    }

    private void setTabIndicators() {
        View tab = mTabHost.getTabWidget().getChildAt(0); // home
        tab.setBackgroundResource(R.drawable.tab_indicator_blue_dark);

        tab = mTabHost.getTabWidget().getChildAt(1); // games
        tab.setBackgroundResource(R.drawable.tab_indicator_green);

        if (RobloxSettings.isPhone())
        {
            tab = mTabHost.getTabWidget().getChildAt(2); // messages
            tab.setBackgroundResource(R.drawable.tab_indicator_blue);
        }
        else
        {
            tab = mTabHost.getTabWidget().getChildAt(2); // catalog
            tab.setBackgroundResource(R.drawable.tab_indicator_green);
        }

        tab = mTabHost.getTabWidget().getChildAt(3); // friends
        tab.setBackgroundResource(R.drawable.tab_indicator_blue);

        if (RobloxSettings.isPhone()) {
            tab = mTabHost.getTabWidget().getChildAt(4); // more
            tab.setBackgroundResource(R.drawable.tab_indicator_black);
        }
        else
        {
            tab = mTabHost.getTabWidget().getChildAt(4); // messages
            tab.setBackgroundResource(R.drawable.tab_indicator_blue);

            tab = mTabHost.getTabWidget().getChildAt(5); // more
            tab.setBackgroundResource(R.drawable.tab_indicator_black);
        }
    }

    // -----------------------------------------------------------------------
    private void showSignupDialog() {
        Bundle args = new Bundle();
        args.putBoolean("isActivityMain", true);
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        FragmentSignUp fragment = new FragmentSignUp();
        fragment.setArguments(args);
        ft.add(Utils.getCurrentActivityId(this), fragment, FragmentSignUp.FRAGMENT_TAG);
        ft.commit();
    }

    // -----------------------------------------------------------------------
    private void showLogoutDialog() {
        RbxAnalytics.fireScreenLoaded("logout");

        AlertDialog dialog = new AlertDialog.Builder(this).setMessage(R.string.LogoutConfirmation)
                .setPositiveButton(R.string.LogoutWord, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        RbxAnalytics.fireButtonClick("logout", "yes");
                        dialog.dismiss();
                        SessionManager.getInstance().doLogout();
                    }
                }).setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        RbxAnalytics.fireButtonClick("logout", "no");
                        dialog.dismiss();
                    }
                }).setOnCancelListener(new DialogInterface.OnCancelListener() {

                    @Override
                    public void onCancel(DialogInterface dialog) {
                        RbxAnalytics.fireButtonClick("logout", "no");
                        dialog.dismiss();
                    }
                }).create();
        dialog.show();
    }

    // -----------------------------------------------------------------------
    private void showRobuxDialog() {
        RobloxWebFragment fragment = new RobloxWebFragment();

        Bundle args = new Bundle();
        int abHeight = getSupportActionBar().getHeight();
        int tabHeight = mTabHost.getTabWidget().getHeight();
        args.putBoolean("showRobux", true);
        args.putInt("dialogHeight", getResources().getDisplayMetrics().heightPixels - (abHeight + tabHeight));

        fragment.setArguments(args);
        fragment.loadURL(RobloxSettings.robuxOnlyUrl());
        fragment.setStyle(DialogFragment.STYLE_NORMAL, R.style.Theme_Roblox_WebDialogCenteredTitle);
        fragment.show(getFragmentManager(), "dialog");

        fireButtonClick("robux");
    }

    private void showBuildersClubDialog() {
        RobloxWebFragment fragment = new RobloxWebFragment();

        Bundle args = new Bundle();
        int abHeight = getSupportActionBar().getHeight();
        int tabHeight = mTabHost.getTabWidget().getHeight();
        args.putBoolean("showBC", true);
        args.putInt("dialogHeight", getResources().getDisplayMetrics().heightPixels - (abHeight + tabHeight));
        fragment.setArguments(args);

        fragment.loadURL(RobloxSettings.buildersClubOnlyUrl());
        fragment.setStyle(DialogFragment.STYLE_NO_TITLE, fragment.getTheme());
        fragment.show(getFragmentManager(), "dialog");

        fireButtonClick("buildersClub");
    }

    public void showSettingsDialog() {
        fireButtonClick("settings");

        if (AndroidAppSettings.EnableFacebookAuth())
            SocialManager.getInstance().facebookGetUserInfoStart(null);

        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        FragmentSettings fragment = new FragmentSettings();
        ft.add(Utils.getCurrentActivityId(this), fragment, FragmentSettings.FRAGMENT_TAG);
        ft.commit();
    }


    // -----------------------------------------------------------------------
    private void launchGame(Bundle params) {
        RobloxSettings.dontReloadMorePage = true;

        int pid = android.os.Process.myPid();
        boolean debuggerAttached = Debug.isDebuggerConnected();

        int placeId = params.getInt("placeId", 0);


        Log.i(TAG, Utils.format("Launching PlaceId:%s Pid:%d Debuger:%s", placeId, pid, debuggerAttached ? "attached" : "none"));
        Intent intent = new Intent(this, ActivityGlView.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);

        // Standard set of parameters - some will be empty depending on the join type
        intent.putExtra("roblox_placeId", placeId);
        intent.putExtra("roblox_userId", params.getInt("userId", 0));
        intent.putExtra("roblox_accessCode", params.getString("accessCode", ""));
        intent.putExtra("roblox_gameId", params.getString("gameId", ""));
        intent.putExtra("roblox_joinRequestType", params.getInt("requestType", -1));

        intent.putExtra("roblox_launcher_pid", pid);
        intent.putExtra("roblox_launcher_debugger_attached", debuggerAttached);

        startActivity(intent);
    }

    // -----------------------------------------------------------------------
    public void onTabChanged(String s) {
        SessionManager sm = SessionManager.getInstance();
        // This method gets called multiple times as we're setting up the tab widget
        // If we don't check mHasFinishedSetup, we'll end up showing the signup dialog when
        // a guest hits 'Play Now'

        // Don't even bother with this until setup complete and we know mHasFinishedSetup is true.
        // We will manually call onTabChanged() when ready
        if(!mHasFinishedSetup){
            return;
        }

        if(mTabHost.getCurrentTab() != 1 && !sm.getIsLoggedIn() && !AndroidAppSettings.EnableValuePages()) {
            // Force the user to log in
            mTabRequestedByUser = mTabHost.getCurrentTab();

            this.showSignupDialog();

            mTabHost.setCurrentTab(1);
            mActiveTab = 1;
        } else {
            resetPrevTab();

            // Make sure the search is closed before we change tabs
            hideKeyboardNoView();
            if (mMenu != null)
            {
                MenuItem searchMenuItem = mMenu.findItem(R.id.action_search);
                if (searchMenuItem != null)
                {
                    SearchView sv = (SearchView) MenuItemCompat.getActionView(searchMenuItem);
                    //mMenu.findItem(R.id.action_search).collapseActionView();
                    sv.onActionViewCollapsed();
                }
            }

            // Swap tabs
            if (AndroidAppSettings.EnableValuePages()) {
                FragmentTransaction transaction = getFragmentManager().beginTransaction();
                transaction.setCustomAnimations(R.animator.slide_in_from_right, R.animator.slide_out_to_left);
                Fragment oldFragment = mTabContents.get(mActiveTab);
                //transaction.setCustomAnimations(R.animator.slide_out_to_left, R.animator.slide_in_from_right);
                transaction.hide(oldFragment);

                int currentTab = mTabHost.getCurrentTab();
                Fragment newFragment = mTabContents.get(currentTab);
                if (!newFragment.isAdded()) {
                    transaction.add(R.id.realtabcontent, newFragment);
                } else
                    transaction.show(newFragment);
                //transaction.replace(Utils.getCurrentActivityId(this), newFragment);

                transaction.commit();

                setActiveTabOptions(currentTab);

                mActiveTab = mTabHost.getCurrentTab();
            }
            else {
                // Swap tabs
                FragmentTransaction transaction = getFragmentManager().beginTransaction();

                Fragment oldFragment = mTabContents.get(mActiveTab);
                transaction.hide(oldFragment);

                int currentTab = mTabHost.getCurrentTab();
                Fragment newFragment = mTabContents.get(currentTab);
                if(!newFragment.isAdded()) {
                    transaction.add(R.id.realtabcontent, newFragment);
                }
                transaction.show(newFragment);
                transaction.commit();

                setActiveTabOptions(currentTab);

                mActiveTab = mTabHost.getCurrentTab();
            }
        }
        RbxAnalytics.fireButtonClick(ctx, getCurrentTabName(), (sm.getIsLoggedIn() ? "isLoggedIn" : "isGuest"));
    }

    private void setActiveTabOptions(int currentTab) {
        ImageView icon = (ImageView)mTabHost.getCurrentTabView().findViewById(android.R.id.icon);
        TextView title = (TextView)mTabHost.getCurrentTabView().findViewById(android.R.id.title);

        switch(currentTab)
        {
            case 0: // home button
                icon.setImageResource(R.drawable.icon_home_on);
                title.setTextColor(getResources().getColor(R.color.RbxBlue3));
                mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_blue_dark);
                getSupportActionBar().setTitle("Home");
                if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_home);

                resetMenuButtons(true, true, true, false);
                changeSearchButton(1);

                startNewTransition(getResources().getColor(R.color.RbxBlue3));

                break;
            case 1: // games
                icon.setImageResource(R.drawable.icon_game_on);
                title.setTextColor(getResources().getColor(R.color.RbxGreen1));
                mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_green);
                getSupportActionBar().setTitle("Games");
                if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_game);

                if (SessionManager.getInstance().getIsLoggedIn())
                    resetMenuButtons(true, true, true, false);
                else
                    resetMenuButtons(false, false, true, false);

                changeSearchButton(2);

                startNewTransition(getResources().getColor(R.color.RbxGreen1));

                break;
            case 2: // friends (phone) or catalog (tablet)
                if (RobloxSettings.isPhone())
                {
                    icon.setImageResource(R.drawable.icon_friends_on);
                    title.setTextColor(getResources().getColor(R.color.RbxBlue1));
                    mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_blue);
                    getSupportActionBar().setTitle("Friends");
                    if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_friends);

                    resetMenuButtons(true, true, false, false);

                    startNewTransition(getResources().getColor(R.color.RbxBlue1));
                }
                else
                {
                    icon.setImageResource(R.drawable.icon_catalog_on);
                    title.setTextColor(getResources().getColor(R.color.RbxGreen1));
                    mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_green);
                    getSupportActionBar().setTitle("Catalog");
                    if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_catalog);

                    resetMenuButtons(true, true, true, false);
                    changeSearchButton(3);

                    startNewTransition(getResources().getColor(R.color.RbxGreen1));
                }

                break;
            case 3: // messages (phone) or friends (tablet)
                if (RobloxSettings.isPhone())
                {
                    icon.setImageResource(R.drawable.icon_messages_on);
                    title.setTextColor(getResources().getColor(R.color.RbxBlue1));
                    mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_blue);
                    getSupportActionBar().setTitle("Messages");
                    if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_messages);

                    resetMenuButtons(true, true, false, false);

                    startNewTransition(getResources().getColor(R.color.RbxBlue1));
                }
                else
                {
                    icon.setImageResource(R.drawable.icon_friends_on);
                    title.setTextColor(getResources().getColor(R.color.RbxBlue1));
                    mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_blue);
                    getSupportActionBar().setTitle("Friends");
                    if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_friends);

                    resetMenuButtons(true, true, false, false);

                    startNewTransition(getResources().getColor(R.color.RbxBlue1));
                }

                break;
            case 4: // more (phone) or messages (tablet)
                if (RobloxSettings.isPhone())
                {
                    icon.setImageResource(mMoreIconOnId);
                    title.setTextColor(getResources().getColor(R.color.black));
                    mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_black);
                    getSupportActionBar().setTitle(mMoreCurrentHeader);
                    if (mUseSpecificIcons) setToolbarIcon(mMoreIconOnId);

                    resetMenuButtons(true, true, false, true);
                    startNewTransition(mMoreCurrentColor);
                }
                else
                {
                    icon.setImageResource(R.drawable.icon_messages_on);
                    title.setTextColor(getResources().getColor(R.color.RbxBlue1));
                    mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_blue);
                    getSupportActionBar().setTitle("Messages");
                    if (mUseSpecificIcons) setToolbarIcon(R.drawable.icon_messages);

                    resetMenuButtons(true, true, false, false);

                    startNewTransition(getResources().getColor(R.color.RbxBlue1));
                }
                break;
            case 5: // more (tablet only)
                icon.setImageResource(mMoreIconOnId);
                title.setTextColor(getResources().getColor(R.color.black));
                mTabHost.getCurrentTabView().setBackgroundResource(R.drawable.tab_indicator_black);
                getSupportActionBar().setTitle(mMoreCurrentHeader);
                if (mUseSpecificIcons) setToolbarIcon(mMoreIconOnId);

                resetMenuButtons(true, true, false, true);
                startNewTransition(mMoreCurrentColor);

                break;
        }


    }

    private void resetMenuButtons(boolean showRobux, boolean showBC, boolean showSearch, boolean showLogout)
    {
        if (mMenu != null)
        {
            mMenu.removeItem(R.id.action_logout);
            mMenu.removeItem(R.id.action_robux);
            mMenu.removeItem(R.id.action_search);
            mMenu.removeItem(R.id.action_tickets);

            MenuInflater inflater = null;

            if (showSearch)
            {
                inflater = getMenuInflater();
                inflater.inflate(R.menu.menu_button_search, mMenu);
                mSearchMenuItem = mMenu.findItem(R.id.action_search);
            }

            if (showRobux)
                addRobuxButton();
            if (showBC)
                addBCButton();
            if (showLogout)
                addLogoutButton();
        }
    }

    private void addRobuxButton() {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_button_robux, mMenu);

        MenuItem robuxItem = mMenu.findItem(R.id.action_robux);
        View robux_button = MenuItemCompat.getActionView(robuxItem);
        robux_button.setOnClickListener( new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ActivityNativeMain.this.showRobuxDialog();
            }
        });

        // We have to re-get this view every time we recreate the menu
        mRobuxBalanceTextView = (TextView) robux_button.findViewById(R.id.robux_balance);
        mRobuxBalanceTextView.setText("" + Utils.formatRobuxBalance(SessionManager.getInstance().getRobuxBalance()));
    }

    private void addBCButton(){
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_button_bc, mMenu);

        MenuItem ticketsItem = mMenu.findItem(R.id.action_tickets);
        View tickets_button = MenuItemCompat.getActionView(ticketsItem);
        tickets_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ActivityNativeMain.this.showBuildersClubDialog();
            }
        });
    }

    private void addLogoutButton() {
        if (mMenu != null && mMenu.findItem(R.id.action_logout) == null)
        {
            MenuInflater inflater = getMenuInflater();
            inflater.inflate(R.menu.menu_button_logout, mMenu);
            mLogoutMenuItem = mMenu.findItem(R.id.action_logout);

            View logout_button = MenuItemCompat.getActionView(mLogoutMenuItem);
            logout_button.setOnClickListener( new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    ActivityNativeMain.this.showLogoutDialog();
                }
            });
        }
    }

    public void startNewTransition(final Integer newColor) {
        if (mColorAnimation != null && mColorAnimation.isRunning())
            mColorAnimation.cancel();

        int duration = 0;
        if (newColor.equals(mPrevColor))
        {
            if (mPrevColor == getResources().getColor(R.color.RbxBlue1))
                mColorAnimation = ValueAnimator.ofObject(new ArgbEvaluator(), mPrevColor, getResources().getColor(R.color.RbxBlueLight));
            else if (mPrevColor == getResources().getColor(R.color.RbxGreen1))
                mColorAnimation = ValueAnimator.ofObject(new ArgbEvaluator(), mPrevColor, getResources().getColor(R.color.RbxGreenLight));
            else if (mPrevColor == getResources().getColor(R.color.black))
                mColorAnimation = ValueAnimator.ofObject(new ArgbEvaluator(), mPrevColor, getResources().getColor(R.color.RbxGray2));

            duration = 350;
        }
        else
        {
            mColorAnimation = ValueAnimator.ofObject(new ArgbEvaluator(), mPrevColor, newColor);
            duration = 700;
        }

        mColorAnimation.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator valueAnimator) {
                if(mToolbar != null) {
                    mToolbar.setBackgroundColor((Integer) valueAnimator.getAnimatedValue());
                }
            }

        });

        mColorAnimation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                if (newColor.equals(mPrevColor))
                {
                    if (mPrevColor == getResources().getColor(R.color.RbxBlue1))
                    {
                        mPrevColor = getResources().getColor(R.color.RbxBlueLight);
                        startNewTransition(newColor);
                    }
                    else if (mPrevColor == getResources().getColor(R.color.RbxGreen1))
                    {
                        mPrevColor = getResources().getColor(R.color.RbxGreenLight);
                        startNewTransition(newColor);
                    }
                    else if (mPrevColor == getResources().getColor(R.color.black))
                    {
                        mPrevColor = getResources().getColor(R.color.RbxGray2);
                        startNewTransition(newColor);
                    }
                }
                else
                    mPrevColor = newColor;
            }
        });

        mColorAnimation.setDuration(duration);
        mColorAnimation.start();
    }

    private void changeSearchButton(int type) {
        if (mMenu != null) {

            int notification = -1;
            int stringId = -1;

            if (type == 1) {
                notification = NotificationManager.REQUEST_USER_SEARCH;
                stringId = R.string.SearchUsersWord;
            } else if (type == 2) {
                notification = NotificationManager.REQUEST_GAME_SEARCH;
                stringId = R.string.SearchGamesWord;
            } else if (type == 3) {
                notification = NotificationManager.REQUEST_CATALOG_SEARCH;
                stringId = R.string.SearchCatalogWord;
            }

            if (notification != -1 && stringId != -1) {
                final int id = notification; // necessary
                final int fType = type;
                MenuItem searchItem = mMenu.findItem(R.id.action_search);
                final SearchView searchView = (SearchView) MenuItemCompat.getActionView(searchItem);
                searchView.setVisibility(View.VISIBLE);
                mSearchMenuItem.setEnabled(true);
                mHideSearchIcon = false;

                // The icon that appears before opening the search bar
                ImageView v = (ImageView) searchView.findViewById(android.support.v7.appcompat.R.id.search_button);
                v.setImageResource(R.drawable.icon_search_32x32);

                v.setOnTouchListener(new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        if (event.getAction() == MotionEvent.ACTION_DOWN)
                        {
                            switch(fType){
                                case 1:
                                    RbxAnalytics.fireButtonClick(ctx, "searchOpen", "users");
                                    break;
                                case 2:
                                    RbxAnalytics.fireButtonClick(ctx, "searchOpen", "games");
                                    break;
                                case 3:
                                    RbxAnalytics.fireButtonClick(ctx, "searchOpen", "catalog");
                                    break;
                            }
                        }
                        return false;
                    }
                });

                // Set search text to white (this is the text that the user enters)
                EditText searchText = (EditText) searchView.findViewById(android.support.v7.appcompat.R.id.search_src_text);
                searchText.setTextColor(Color.WHITE);
                searchText.setHintTextColor(Color.WHITE);

                // Change search plate drawable
                View searchPlate = searchView.findViewById(android.support.v7.appcompat.R.id.search_plate);
                //searchPlate.setBackgroundColor(Color.DKGRAY);
                searchPlate.setBackgroundResource(R.drawable.textfield_searchview_holo_light);

                // Inner close button
                ImageView searchCloseView = (ImageView)searchView.findViewById(android.support.v7.appcompat.R.id.search_close_btn);
                searchCloseView.setImageResource(R.drawable.icon_close_14x14);

                searchCloseView.setOnTouchListener(new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        if (event.getAction() == MotionEvent.ACTION_DOWN)
                        {
                            switch(fType){
                                case 1:
                                    RbxAnalytics.fireButtonClick(ctx, "searchClose", "users");
                                    break;
                                case 2:
                                    RbxAnalytics.fireButtonClick(ctx, "searchClose", "games");
                                    break;
                                case 3:
                                    RbxAnalytics.fireButtonClick(ctx, "searchClose", "catalog");
                                    break;
                            }
                        }
                        return false;
                    }
                });

                searchView.setQueryHint(getResources().getString(stringId));
                searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
                    @Override
                    public boolean onQueryTextSubmit(String s) {
                        Bundle gameData = new Bundle();
                        gameData.putString("query", s);
                        NotificationManager.getInstance().postNotification(id, gameData);
                        return true;
                    }

                    @Override
                    public boolean onQueryTextChange(String s) {
                        return false;
                    }
                });

                searchText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
                    @Override
                    public void onFocusChange(View view, boolean hasFocus) {
                        if (!hasFocus)
                        {
                            hideKeyboardNoView();
                            MenuItem searchItem = mMenu.findItem(R.id.action_search);
                            SearchView sv = (SearchView) MenuItemCompat.getActionView(searchItem);
                            //mMenu.findItem(R.id.action_search).collapseActionView();
                            sv.onActionViewCollapsed();
                        }
                    }
                });
            }
            else
                Log.e(TAG, "ERROR: Unknown type in changeSearchButton.");
        }

    }

    // Resets the tab we are leaving to black icon/text
    private void resetPrevTab() {
        ImageView icon = (ImageView)mTabHost.getTabWidget().getChildAt(mActiveTab).findViewById(android.R.id.icon);
        TextView title = (TextView)mTabHost.getTabWidget().getChildAt(mActiveTab).findViewById(android.R.id.title);
        title.setTextColor(Color.parseColor("#000000")); // always reset to black
        switch (mActiveTab)
        {
            case 0: // home
                icon.setImageResource(R.drawable.icon_home);
                break;
            case 1: // games
                icon.setImageResource(R.drawable.icon_game);
                break;
            case 2: // catalog (tablet) or friends (phone)
                if (RobloxSettings.isPhone())
                    icon.setImageResource(R.drawable.icon_friends);
                else
                    icon.setImageResource(R.drawable.icon_catalog);
                break;
            case 3: // friends (tablet) or messages (phone)
                if (RobloxSettings.isPhone())
                    icon.setImageResource(R.drawable.icon_messages);
                else
                    icon.setImageResource(R.drawable.icon_friends);
                break;
            case 4: // messages (tablet) or more (phone)
                if (RobloxSettings.isPhone())
                    icon.setImageResource(mMoreIconOffId);
                else
                    icon.setImageResource(R.drawable.icon_messages);
                break;
            case 5: // more (tablet only)
                icon.setImageResource(mMoreIconOffId);
                break;
        }
    }

    public void updateMoreProperties(String newTitle, Integer newColor) {
        mMoreCurrentColor = newColor;
        mMoreCurrentHeader = newTitle;
        getSupportActionBar().setTitle(newTitle);
    }

    // -----------------------------------------------------------------------
    private void hideKeyboard() {
        View viewWithFocus = getCurrentFocus();
        if(viewWithFocus != null) {
            InputMethodManager inputManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            inputManager.hideSoftInputFromWindow(viewWithFocus.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
        }
    }

    private void hideKeyboardNoView() {
        InputMethodManager inputManager = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        inputManager.hideSoftInputFromWindow(findViewById(android.R.id.content).getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
    }

    // -----------------------------------------------------------------------
    @Override
    public void onBackPressed() {
        if (!closeOpenFrags()) {
            Fragment currentFrag = mTabContents.get(mTabHost.getCurrentTab());
            boolean promptLogOut = true;
            if(currentFrag instanceof RobloxWebFragment){
                promptLogOut &= !((RobloxWebFragment) currentFrag).goBack();
            }
            if (promptLogOut) {
                if (!SessionManager.getInstance().getIsLoggedIn()) {
                    launchActivityStart();
                } else
                    showLogoutDialog();
            }
        }
    }

    /***
     * Checks for open fragments and closes them.
     * @return True if a fragment was closed, otherwise false.
     */
    private boolean closeOpenFrags() {
        FragmentManager fm = getFragmentManager();
        Fragment frag = fm.findFragmentByTag(FragmentSignUp.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentSignUp)frag).closeDialog();
            return true;
        }

        frag = fm.findFragmentByTag(FragmentChangeEmail.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentChangeEmail)frag).closeDialog();
            return true;
        }

        frag = fm.findFragmentByTag(FragmentChangePassword.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentChangePassword)frag).closeDialog();
            return true;
        }

        frag = fm.findFragmentByTag(FragmentSettings.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentSettings)frag).closeDialog();
            return true;
        }

        return false;
    }

    private void launchActivityStart() {
        this.finish();
        Intent intent = new Intent(this, ActivityStart.class);
        intent.putExtra("returningFromNativeMain", true);
        startActivity(intent);
    }

    // -----------------------------------------------------------------------
    @Override
    public void handleNotification(int notificationId, Bundle userParam) {
        switch (notificationId) {
            case NotificationManager.REQUEST_START_PLACEID: {
                launchGame(userParam);
                break;
            }

            case NotificationManager.REQUEST_USER_SEARCH: {
                hideKeyboard();

                String query = userParam.getString("query");
                String url = RobloxSettings.searchUsersUrl(query);

                RobloxWebFragment webView = (RobloxWebFragment) mTabContents.get( mTabHost.getCurrentTab() );
                webView.loadURL(url);

                break;
            }

            case NotificationManager.REQUEST_GAME_SEARCH: {
                hideKeyboard();

                String query = userParam.getString("query");
                String url = RobloxSettings.searchGamesUrl(query);

                RobloxWebFragment webView = (RobloxWebFragment) mTabContents.get( mTabHost.getCurrentTab() );
                webView.loadURL(url);

                break;
            }

            case NotificationManager.REQUEST_CATALOG_SEARCH: {
                hideKeyboard();

                String query = userParam.getString("query");
                String url = RobloxSettings.searchCatalogUrl(query);

                RobloxWebFragment webView = (RobloxWebFragment) mTabContents.get( mTabHost.getCurrentTab() );
                webView.loadURL(url);

                break;
            }

            case NotificationManager.EVENT_USER_LOGIN: {
                // If the Sign Up window is open when the user logs in, this will close it
                FragmentManager m = getFragmentManager();
                Fragment f = m.findFragmentByTag(FragmentSignUp.FRAGMENT_TAG);
                if (f != null)
                    m.beginTransaction().remove(f).commit();

                updateTabsStatus();
                break;
            }

            case NotificationManager.EVENT_USER_LOGOUT: {
                FragmentLogin login = (FragmentLogin)getFragmentManager().findFragmentByTag(FragmentLogin.FRAGMENT_TAG);
                if (login == null || !login.isVisible())
                {
                    launchActivityStart();
                    break;
                }

            }

            case NotificationManager.EVENT_USER_INFO_UPDATED: {
                SessionManager sm = SessionManager.getInstance();

                if( sm.getIsLoggedIn() ) {
                    getSupportActionBar().show();

                    if (sm != null && mRobuxBalanceTextView != null)
                        mRobuxBalanceTextView.setText( "" + Utils.formatRobuxBalance(sm.getRobuxBalance()) );
                }
                break;
            }

            case NotificationManager.REQUEST_OPEN_LOGIN: {

            }
            case NotificationManager.EVENT_USER_CAPTCHA_SOLVED: {
                onLoginCaptchaSolved();
                break;
            }
            case NotificationManager.EVENT_USER_FACEBOOK_CONNECTED: {
                onFacebookConnectionUpdate();
                onFacebookConnectionUpdateStop();
                break;
            }
            case NotificationManager.EVENT_USER_FACEBOOK_DISCONNECTED: {
                onFacebookConnectionUpdate();
                onFacebookConnectionUpdateStop();
                break;
            }
            case NotificationManager.EVENT_USER_FACEBOOK_INFO_UPDATED: {
                onFacebookConnectionUpdate();
                break;
            }
            case NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STARTED:
                onFacebookConnectionUpdateStart("Connecting...");
                break;
            case NotificationManager.EVENT_USER_FACEBOOK_CONNECT_STOPPED:
                onFacebookConnectionUpdateStop();
                break;
            case NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STARTED:
                onFacebookConnectionUpdateStart("Disconnecting...");
                break;
            case NotificationManager.EVENT_USER_FACEBOOK_DISCONNECT_STOPPED:
                onFacebookConnectionUpdateStop();
                break;
            default:
                break;
        }
    }

    private void onFacebookConnectionUpdate() {
        Fragment frag = getFragmentManager().findFragmentByTag(FragmentSettings.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentSettings)frag).updateFacebookButton();
        }
    }

    private void onFacebookConnectionUpdateStart(String message) {
        Fragment frag = getFragmentManager().findFragmentByTag(FragmentSettings.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentSettings)frag).showSpinner(message);
        }
    }

    private void onFacebookConnectionUpdateStop() {
        Fragment frag = getFragmentManager().findFragmentByTag(FragmentSettings.FRAGMENT_TAG);
        if (frag != null) {
            ((FragmentSettings) frag).closeSpinner();
        }
    }


    // RbxAnalytics Helpers
    private void fireButtonClick(String buttonName) {
        String cstm = getCurrentTabName();
        RbxAnalytics.fireButtonClick(ctx, buttonName, cstm);
    }

    private String getCurrentTabName() {
        int currentTab = mTabHost.getCurrentTab();
        boolean isPhone = RobloxSettings.isPhone();
        String cstm = "";
        switch (currentTab) {
            case 0: // home
                cstm = "tabHome";
                break;
            case 1: // games
                cstm = "tabGames";
                break;
            case 2: // friends (phone) or catalog (tablet)
                if (isPhone)
                    cstm = "tabFriends";
                else
                    cstm = "tabCatalog";
                break;
            case 3: // messages (phone) or friends (tablet)
                if (isPhone)
                    cstm = "tabMessages";
                else
                    cstm = "tabFriends";
                break;
            case 4: // more (phone) or messages (tablet)
                if (isPhone)
                    cstm = "tabMore";
                else
                    cstm = "tabMessages";
                break;
            case 5: // more (tablet only)
                cstm = "tabMore";
                break;
        }

        // This allows us to know which More Sub-Page they are on when they click the Robux/BC buttons
        if (cstm.equals("tabMore") && !latestMorePage.isEmpty())
            cstm = latestMorePage;

        return cstm;
    }

    public ArrayList<Fragment> getTabContents()
    {
        return mTabContents;
    }

    /**
     * Set the toolbar logo icon and add some padding to the ImageView in the toolbar
     */
    private void setToolbarIcon(int res){
        Drawable logo = getResources().getDrawable(res);
        mToolbar.setLogo(logo);
        for (int i = 0; i < mToolbar.getChildCount(); i++) {
            View child = mToolbar.getChildAt(i);
            if (child != null) {
                if (child.getClass() == ImageView.class) {
                    ImageView iv2 = (ImageView) child;
                    if (iv2.getDrawable() == logo) {
                        int padding = (int) (5 * getResources().getDisplayMetrics().density);
                        iv2.setPadding(0, padding, 0, padding);
                        break;
                    }
                }
            }
        }
    }

    /**
     * Set the toolbar title text color. Note: this should be handled in theme, but some older
     * devices still not showing the text in the correct color
     */
    private void setToolbarTitleTextColor(int color){
        for (int i = 0; i < mToolbar.getChildCount(); i++) {
            View child = mToolbar.getChildAt(i);
            if (child != null) {
                if (child.getClass() == TextView.class) {
                    TextView tv = (TextView) child;
                    tv.setTextColor(color);
                }
            }
        }
    }
}
