package com.roblox.client;

import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.os.*;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.google.android.gms.analytics.GoogleAnalytics;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.managers.SessionManager;
import com.roblox.ima.FragmentVideo;


import java.io.FileOutputStream;
import java.io.IOException;
import java.util.concurrent.CountDownLatch;

// ***
// *** C++ DEBUGGING REQUIRES EXTRA STEPS:  See how_to_debug_cpp.txt.
// ***

// NOT A RobloxActivity because this is in a different process.
public class ActivityGlView extends RobloxActivity implements SurfaceHolder.Callback {
    public static final boolean ALLOW_SAME_PROCESS_DEBUGGING = false;

    public static final int SURFACE_INVALID = -1;
    public static final int SURFACE_NOT_READY = 0;
    public static final int SURFACE_READY_NOT_CREATED = 1;
    public static final int SURFACE_CREATED = 2;
    public static final int SURFACE_READY_TO_DESTROY = 3;
    public static final int SURFACE_DESTROYED = 4;

    private static final int SHOW_VIDEO_AD_REQUEST = 1;

    private static final String TAG = "ActivityGlView";

	// The OS can try to multiply instantiate this class.  Even though it never should.
	private static ActivityGlView mSingleton = null;

    private RbxKeyboard mGlEditTextView = null;
    private SurfaceView mSurfaceView = null;
    private boolean mDifferentProcess = false;
    private Handler mUIThreadHandler = null;
    private long mCurrentTextBox = 0;
    private long mMTBFStartTimeMilliseconds = 0;
    private boolean mAlreadyCreated = false;
    private boolean mAlreadyDestroyed = false;
	private boolean mAllowPauseResume = true;
	private boolean mPausedForDialog = false;
    private InputListener mInputListener = null;

    private static FragmentManager mFragmentManager = null;

    // WARNING:  All native code is gated by mSurfaceState to ensure coherent calling states.
    // Should only ever increase through the SURFACE_* states.
    private int mSurfaceState = SURFACE_INVALID;

    // ------------------------------------------------------------------------
    // Activity

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // --------------------------------------------------------------------

        // First off check if the debugger needs to be attached.

        Log.i(TAG, "ActivityGlView onCreate()");

        SessionManager.mCurrentActivity = this;

        Intent incomingIntent = getIntent();
        if (incomingIntent == null) {
            Log.e(TAG, "Launching GL Activity without Intent.");
            finish();
            return;
        }

        // Required to make games launch in landscape on phone
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        // Check if there is a need to wait for the debugger.
        int launcherPid = incomingIntent.getIntExtra("roblox_launcher_pid", -1);
        boolean launcherDebuggerAttached = incomingIntent.getBooleanExtra("roblox_launcher_debugger_attached", false);
        int myPid = android.os.Process.myPid();

        // Shutdown is done differently depending on whether a separate process is in play.
        mDifferentProcess = myPid != launcherPid;

        String msg = Utils.format("Incoming Intent ActivityGlView Pid:%d Debuger:%s", launcherPid, launcherDebuggerAttached ? "attached" : "none");
        Log.i(TAG, msg);

        if (launcherDebuggerAttached && mDifferentProcess) {
            Log.w(TAG, "ActivityGlView waiting for debugger");
            Debug.waitForDebugger();
        }

        // Try to accept this instance as the single instance that is allowed.
        if (mSingleton != null) {
            Log.e(TAG, "*** Trying to Create twice. ***");
            finish();
            return;
        }
        mSingleton = this;

        if (!mDifferentProcess) {
            if (ALLOW_SAME_PROCESS_DEBUGGING) {
                Utils.alertUnexpectedError("Add android:process=\"glview\" to manifest and turn off ALLOW_SAME_PROCESS_DEBUGGING.");
            } else {
                // This is not cool but happens spuriously on certain devices, needs
                // to be silently ignored.  The combination of
                // android:launchMode="singleInstance" and android:process="glview"
                // should make this impossible.
                Log.e(TAG, "ActivityGlView launched in same process.");
                finish();
                return;
            }
        }

        // --------------------------------------------------------------------
        // Write "crash guard" for MTBF calculation

        // The crash guard updates on the mUIThreadHandler.
        mUIThreadHandler = new Handler(Looper.getMainLooper());
        doCrashGuardSetup();

        // --------------------------------------------------------------------
        // WebView should only launch skippable warnings here.
//        String reason = RobloxSettings.deviceNotSupportedString();
//        if (reason != null) {
//            Utils.alertExclusivelyFormatted(R.string.UnsupportedDeviceSkippable, reason);
//        } else // Check if this device is on the blacklist
//        {
//            mDeviceBlacklistChecker = new DeviceBlacklistChecker(Build.MODEL, RobloxSettings.blacklistUrl(), (Activity) mSingleton);
//            mDeviceBlacklistChecker.execute();
//        }

        // --------------------------------------------------------------------
        // native .so's

        try {
            System.loadLibrary("fmod");
        } catch (final UnsatisfiedLinkError e) {
            // This is normal in Noopt.
            Log.i(TAG, e.getLocalizedMessage());
            System.loadLibrary("fmodL");
        }
        System.loadLibrary("roblox");

        // --------------------------------------------------------------------
        // ActivityGlView onCreate

        RobloxApplication app = (RobloxApplication) getApplication();
        if (app.checkShowCriticalError()) {
            Log.e(TAG, "Trying to create GLView after critcal error.");
            return;
        }

        setContentView(R.layout.activity_glview);

        HttpAgent.onCreate(this);
        RobloxSettings.updateNativeSettings();

        mGlEditTextView = (RbxKeyboard) findViewById(R.id.gl_edit_text);
        initGlEditTextView();


        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        initSurfaceView();
        mSurfaceState = SURFACE_NOT_READY;

        org.fmod.FMOD.init(this);

        mFragmentManager = getFragmentManager();
    }

    @Override
    protected void onDestroy() {
        if (mAlreadyDestroyed) {
            Log.e(TAG, "*** Trying to Destroy twice. ***");
            return;
        }
        mAlreadyDestroyed = true;

        doCrashGuardTeardown(true);
        org.fmod.FMOD.close();

        // Try to flush Analytics before calling killProcess.
        GoogleAnalytics.getInstance(getBaseContext()).dispatchLocalHits();

        if (mDifferentProcess) {
            // Kills process, other activities in same process re-launched.
            System.exit(0);
        } else {
            super.onDestroy();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.i(TAG, "onStart");


        RobloxSettings.enableNDKProfiler(true);
    }

    private final Runnable runnerExit = new Runnable() {
        public void run()
        {
            exitGameSilent();
        }
    };

    @Override
    protected void onStop() {
        super.onStop();

	    RobloxSettings.enableNDKProfiler(false);
        
        if (mAllowPauseResume && !mPausedForDialog)
            mUIThreadHandler.postDelayed(runnerExit, 30000);

        if (mSurfaceState < SURFACE_CREATED) {
            mSurfaceState = SURFACE_DESTROYED;
        } else if (mSurfaceState == SURFACE_CREATED && !allowPauseResume()) {
            mSurfaceState = SURFACE_READY_TO_DESTROY;
        }


        if (mInputListener != null) {
            mInputListener.stopSensorListening();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        SessionManager.mCurrentActivity = this;
        Utils.sendAnalyticsScreen("ActivityGlView");
        HttpAgent.onResume();

		// If we return to this activity before the timer executes,
        // we remove the runner from the queue
        mUIThreadHandler.removeCallbacks(runnerExit);		

        if (mInputListener != null) {
            mInputListener.startSensorListening(false);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        HttpAgent.onPause(getCacheDir(), null);
    }

    @Override
    public void onBackPressed() {
        nativeHandleBackPressed();
    }

    // onTrimMemory() is logged up in RobloxActivity but this is the only time the
    // running game is notified.
    @Override
    public void onLowMemory() {
        super.onLowMemory();
        // This should only be called when the RobloxView is alive.
        if (mSurfaceState == SURFACE_CREATED) {
            nativeOnLowMemory();
        }
    }

    @Override
    public void onNewIntent(Intent intent) {
        Log.w(TAG, "onNewIntent called");  // "Because Android."
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == SHOW_VIDEO_AD_REQUEST) {
            // Notify that the video ad is finished
            if (data != null) // can be null on phones - Android likes to launch our activity multiple times,
                                // which causes ads to play twice
                nativeVideoAdFinished(data.getBooleanExtra("shown", false));
            else
                nativeVideoAdFinished(false);

            mSingleton.mPausedForDialog = false;
        }
    }

    protected boolean allowPauseResume() {
        return mAllowPauseResume || mPausedForDialog;
    }

    // ------------------------------------------------------------------------

    private void initGlEditTextView() {
        mGlEditTextView.setVisibility(View.GONE);

        mGlEditTextView.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI);
        mGlEditTextView.setSingleLine(true);
        mGlEditTextView.setText("");

        mGlEditTextView.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT) {
                    String text = v.getText().toString();
                    if (mSurfaceState == SURFACE_CREATED) {
                        nativePassText(mCurrentTextBox, text, true, v.getSelectionStart());
                    } else {
                        Log.w(TAG, "nativePassText not ready");
                    }
                    mGlEditTextView.setCurrentTextBox(0);

                    v.setVisibility(View.GONE);

                    InputMethodManager imm = (InputMethodManager) mSurfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);

                    return true;
                }
                return false;
            }
        });
		
		mGlEditTextView.addTextChangedListener(new TextWatcher() {
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count, int after) {
				
			}
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				if (mSurfaceState == SURFACE_CREATED) {
					nativePassText(mCurrentTextBox, s.toString(), false, start + count);
				} else {
					Log.w(TAG, "nativePassText not ready");
				}
			}
			
			@Override
			public void afterTextChanged(Editable s) {
			
			}
		});
    }

    // ------------------------------------------------------------------------
    // SurfaceView/SurfaceHolder

    public static int getSurfaceState() {
        return mSingleton.mSurfaceState;
    }

    private void initSurfaceView() {
        // This is one half of a hack that is necessary to render the game correctly
        // on the Samsung Galaxy Tab 4 7.0. The other half is in JNIGLActivity.cpp:nativeStartGame and nativeStartupGraphics
        // I'm not entirely sure why this is necessary - best guess is that the device doesn't
        // report the screen size correctly in native code when we resize the buffer. I think the
        // ANativeWindow_setBuffersGeometry call uses the size from the SurfaceHolder, not the SurfaceView
        // so we need to set these manually before we pass them to the native layer. We also have to manually set the
        // height and width of the buffer in the native layer before we create it.
        if (Build.MODEL.equals("SM-T230NU")) {
            ViewGroup.LayoutParams lp = mSurfaceView.getLayoutParams();
            lp.width = 1280;
            lp.height = 800;
            mSurfaceView.setLayoutParams(lp);
            mSurfaceView.getHolder().setFixedSize(960, 600);
        }

        mSurfaceView.setFocusable(true);
        mSurfaceView.setFocusableInTouchMode(true);

        mSurfaceView.getHolder().addCallback(this);
    }

    // NOTE:  This method is always called at least once, after surfaceCreated().
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    }

    public void surfaceCreated(SurfaceHolder holder) {
        if (mSurfaceState == SURFACE_NOT_READY) {
            mSurfaceState = SURFACE_READY_NOT_CREATED;

            startGame();
        } else if (allowPauseResume() && mSurfaceState == SURFACE_CREATED) {
            DisplayMetrics metrics = getResources().getDisplayMetrics();
            Log.w(TAG, "*** nativeStartUpGraphics ***");
            Surface surface = mSurfaceView.getHolder().getSurface();

            nativeStartUpGraphics(surface, metrics.density, Build.MODEL);
        }

        // Restore this flag after all the initialization is done
        mPausedForDialog = false;
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        RobloxApplication app = (RobloxApplication) getApplication();
        if (app.checkShowCriticalError()) {
            Log.e(TAG, "Trying to shut down GL surface after critcal error.");
            return;
        }

        if (mSurfaceState != SURFACE_CREATED && mSurfaceState != SURFACE_READY_TO_DESTROY) {
            // Bad things are happening.  This instance will be killed.
            return;
        }

        if (allowPauseResume()) {
            Log.w(TAG, "*** nativeShutDownGraphics ***");
            nativeShutDownGraphics(mSurfaceView.getHolder().getSurface());
        } else {
            Log.i(TAG, ">>> Calling nativeStopGame");
            nativeStopGame();
            Log.i(TAG, "<<< Returned from nativeStopGame");

            mSurfaceState = SURFACE_DESTROYED;
        }
    }

    // ------------------------------------------------------------------------
    // Callback from service generated via surfaceCreated

    protected void startGame() {
        mSurfaceState = SURFACE_CREATED;

        RobloxApplication app = (RobloxApplication) getApplication();
        if (app.checkShowCriticalError()) {
            Log.e(TAG, "Trying to create GL surface after critcal error.");
            return;
        }

        Intent intent = this.getIntent();

        if (AndroidAppSettings.EnableGameStartFix()) {
            if (intent != null) {
                int placeId = intent.getIntExtra("roblox_placeId", 0);
                int userId = intent.getIntExtra("roblox_userId", 0);
                String accessCode = intent.getStringExtra("roblox_accessCode");
                String gameId = intent.getStringExtra("roblox_gameId");
                int joinRequestType = intent.getIntExtra("roblox_joinRequestType", -1);

                Log.i(TAG, "ActivityGLView Data: placeId = " + placeId + " | userId = " + userId + " | accessCode = " + accessCode + " | gameId = " + gameId + " | requestType = " + joinRequestType);

                final String assetPath = XAPKManager.unpackAssets(this);

                Surface surface = mSurfaceView.getHolder().getSurface();
                DisplayMetrics metrics = getResources().getDisplayMetrics();
                nativeStartGame(surface, placeId, userId, accessCode, gameId, joinRequestType, assetPath, metrics.density,
                        getPackageManager().hasSystemFeature("android.hardware.touchscreen"), "" + Build.VERSION.SDK_INT, Devices.getDeviceName(),
                        RobloxSettings.version(), Build.MODEL);

                mInputListener = new InputListener(this, mSurfaceView);
                mSurfaceView.setOnTouchListener(mInputListener);
            } else {
                Utils.alert("Game launched failed! Please try again.");
                finish();
            }
        } else {
            int placeId = intent.getIntExtra("roblox_placeId", 0);
            int userId = intent.getIntExtra("roblox_userId", 0);
            String accessCode = intent.getStringExtra("roblox_accessCode");
            String gameId = intent.getStringExtra("roblox_gameId");
            int joinRequestType = intent.getIntExtra("roblox_joinRequestType", -1);

            Log.i(TAG, "ActivityGLView Data: placeId = " + placeId + " | userId = " + userId + " | accessCode = " + accessCode + " | gameId = " + gameId + " | requestType = " + joinRequestType);

            final String assetPath = XAPKManager.unpackAssets(this);

            Surface surface = mSurfaceView.getHolder().getSurface();
            DisplayMetrics metrics = getResources().getDisplayMetrics();
            nativeStartGame(surface, placeId, userId, accessCode, gameId, joinRequestType, assetPath, metrics.density,
                    getPackageManager().hasSystemFeature("android.hardware.touchscreen"), "" + Build.VERSION.SDK_INT, Devices.getDeviceName(),
                    RobloxSettings.version(), Build.MODEL);

            mInputListener = new InputListener(this, mSurfaceView);
            mSurfaceView.setOnTouchListener(mInputListener);
        }

    }

    // ------------------------------------------------------------------------
    // CrashGuard   

    private void doCrashGuardSetup() {
        Utils.doCrashGuardCheck(false);

        mMTBFStartTimeMilliseconds = System.currentTimeMillis();
        doCrashGuardUpdate(true);
    }

    private void doCrashGuardUpdate(boolean firstTime) {
        if (mMTBFStartTimeMilliseconds == 0) {
            return;
        }

        long endTimeMilliseconds = System.currentTimeMillis();
        long milliseconds = endTimeMilliseconds - mMTBFStartTimeMilliseconds;
        if (milliseconds < 1) {
            milliseconds = 1;
        }
        char[] millisecondsChar = Long.toString(milliseconds).toCharArray();

        try {
            FileOutputStream fos = openFileOutput(Utils.CRASH_GUARD, Context.MODE_PRIVATE);
            for (char c : millisecondsChar) {
                fos.write((byte) c);
            }
            fos.flush();
            fos.getFD().sync();
            fos.close();
        } catch (IOException e) {
            Utils.sendAnalytics(Utils.CRASH_GUARD_CHANNEL, "FailedCreate");
            return;
        }

        if (firstTime) {
            Utils.sendAnalytics(Utils.CRASH_GUARD_CHANNEL, "CreateOK");
        }

        mUIThreadHandler.postDelayed(new Runnable() {
            public void run() {
                doCrashGuardUpdate(false);
            }
        }, 1000);
    }

    private void doCrashGuardTeardown(boolean onDestroy) {
        try {
            FileOutputStream fos = openFileOutput(Utils.CRASH_GUARD_OK, Context.MODE_PRIVATE);
            fos.write((byte) 'x');
            fos.flush();
            fos.getFD().sync();
            fos.close();
        } catch (IOException e) {
        }

        // Stop the update looper.
        mMTBFStartTimeMilliseconds = 0;
    }

    // ------------------------------------------------------------------------
    // Calls to JNI

    public static void releaseFocus(long textBoxInFocus) {
        if (mSingleton.mSurfaceState != SURFACE_CREATED) {
            Log.w(TAG, "releaseFocus() called unexpectedly: " + mSingleton.mSurfaceState);
            return;
        }

        nativeReleaseFocus(textBoxInFocus);
    }

    private static native void nativeStartGame(Surface surface, int placeId, int userId, String accessCode, String gameId, int joinRequestType,
                                               String assetFolder, float density, boolean isTouchDevice, String osVersion, String deviceName,
                                               String appVersion, String model);

    private static native void nativeStopGame();

    private static native void nativePassText(long textBoxInFocus, String text, boolean enterPressed, int cursorPosition);

    private static native void nativeReleaseFocus(long textBoxInFocus);

    private static native void nativeOnLowMemory();

    private static native void nativeHandleBackPressed();

    private static native void nativeShutDownGraphics(Surface surface);

    private static native void nativeStartUpGraphics(Surface surface, float density, String model);

    private static native void nativeInGamePurchaseFinished(boolean success, long player, String productId);

    private static native void nativeVideoAdFinished(boolean adShown);

    // NOTE: This is in JNIMain.cpp for now as it's associated with function
    //       marshaling and in this class solely to have access to mUIThreadHandler.
    private static native void nativeCallMessagesFromMainThread();

    // ------------------------------------------------------------------------
    // Callbacks from JNI

    static public void sendAppEvent(final boolean blockUntilSent) throws InterruptedException {
        final CountDownLatch appEventSentSignal = blockUntilSent ? (new CountDownLatch(1)) : null;

        mSingleton.mUIThreadHandler.postAtFrontOfQueue(new Runnable() {
            public void run() {
                nativeCallMessagesFromMainThread();
                if (blockUntilSent) {
                    appEventSentSignal.countDown();
                }
            }
        });

        if (blockUntilSent) {
            appEventSentSignal.await();
        }
    }

    static public void postAppEvent() {
        mSingleton.mUIThreadHandler.postAtFrontOfQueue(new Runnable() {
            public void run() {
                nativeCallMessagesFromMainThread();
            }
        });
    }

    static public void showGoogleAd(String url)
    {
        mSingleton.mPausedForDialog = true;
        Runnable showAdRunnable = new showGoogleAdRunner(mFragmentManager, url);
        mSingleton.mUIThreadHandler.post(showAdRunnable);
    }

    static public class showGoogleAdRunner implements Runnable {
        FragmentManager fm;
        String mUrl;

        public showGoogleAdRunner(FragmentManager f, String url)
        {
            fm = f;
            mUrl = url;
        }

        public void run() {
            FragmentTransaction ft = fm.beginTransaction();
            FragmentVideo fv = new FragmentVideo();
            Bundle args = new Bundle();
            args.putString("GoogleUrl", mUrl);
            fv.setArguments(args);
            ft.add(R.id.activity_glview, fv, "dialog_ad");
            ft.commit();
            mSingleton.mSurfaceView.setVisibility(View.GONE);
        }
    }

    static public void removeGoogleAd() {
        mSingleton.mPausedForDialog = false;
        Runnable removeAdRunnable = new removeGoogleAdRunner(mFragmentManager);
        mSingleton.mUIThreadHandler.post(removeAdRunnable);
    }

    static public class removeGoogleAdRunner implements Runnable {

        FragmentManager fm;

        public removeGoogleAdRunner(FragmentManager f){
            fm = f;
        }

        public void run() {
            Fragment frag = fm.findFragmentByTag("dialog_ad");
            if (frag != null)
            {
                fm.beginTransaction().remove(frag).commit();
                mSingleton.mSurfaceView.setVisibility(View.VISIBLE);
            }


        }
    }

    static public class showEditText implements Runnable {

        String param;

        public showEditText(String parameter) {
            param = parameter;
        }

        public void run() {
            mSingleton.mGlEditTextView.setVisibility(View.VISIBLE);

            CharSequence charText = (CharSequence) param;
            mSingleton.mGlEditTextView.setText(charText);

            mSingleton.mGlEditTextView.requestFocus();

            InputMethodManager imm = (InputMethodManager) mSingleton.mSurfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.showSoftInput(mSingleton.mGlEditTextView, InputMethodManager.SHOW_FORCED);
        }
	}
    
    static public void showKeyboard(long textBox, String textFromTextBox)
    {
    	mSingleton.mCurrentTextBox = textBox;
    	mSingleton.mGlEditTextView.setCurrentTextBox(textBox);
    	
    	Runnable showKeyBoardRunnable = new ActivityGlView.showEditText(textFromTextBox);
    	mSingleton.mUIThreadHandler.post(showKeyBoardRunnable);
    }

    static private class runnableHideKeyboard implements Runnable {
        // If we don't run these commands on the main thread,
        // we'll get a ViewRootImpl$CalledFromWrongThreadException

        public void run() {
            InputMethodManager imm = (InputMethodManager)mSingleton.mSurfaceView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(mSingleton.mGlEditTextView.getWindowToken(), 0); // 0 forces the keyboard to close when it was opened with SHOW_FORCED
                                                                                            // (not sure why)
            mSingleton.mGlEditTextView.setVisibility(View.GONE);
        }
    }

    static public void hideKeyboard() {
        mSingleton.mCurrentTextBox = 0;
        mSingleton.mGlEditTextView.setCurrentTextBox(0);

        Runnable hideKeyboardRunnable = new runnableHideKeyboard();
        mSingleton.mUIThreadHandler.post(hideKeyboardRunnable);
    }

    static public void listenToMotionEvents(String motionType) {
        mSingleton.mInputListener.startSensorListening(true);
    }

    static public void showAdColonyAd() {
        Intent intent = new Intent(mSingleton, ActivityAdColony.class);

        // Since the ad is shown in a different process and a different activity,
        // force the game to not be destroyed when this activity resumes.
        mSingleton.mPausedForDialog = true;

        mSingleton.startActivityForResult(intent, SHOW_VIDEO_AD_REQUEST);
    }

    static public void promptNativePurchase(long player, String productId, String username) {
        mSingleton.mStoreMgr.doInAppPurchaseForProduct(mSingleton, productId, username, player);
    }

    static public void inGamePurchaseFinished(boolean success, long player, String productId) {
        if ((mSingleton == null) || (mSingleton.mSurfaceState != SURFACE_CREATED)) {
            return;
        }
        nativeInGamePurchaseFinished(success, player, productId);
    }

	static public void exitGame()
    {
        mSingleton.mUIThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                SessionManager.getInstance().requestUserInfoUpdate();
            }
        });
		mSingleton.finish();
	}

    static private Runnable createRunnable(final String name)
    {
        return new Runnable() {
            public void run() {
                // The C++ layer will pass up the string name, as defined in strings.xml
                // We then need to get that string's Android resource ID so that we can pull the
                // full error message from strings.xml.
                int androidId = mSingleton.getResources().getIdentifier(name, "string", mSingleton.getPackageName());
                AlertDialog dialog = new AlertDialog.Builder(mSingleton).setMessage(androidId)
                        .setNegativeButton("Close", new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                                mSingleton.finish();
                            }
                        })
                        .setOnCancelListener(new DialogInterface.OnCancelListener() {

                            @Override
                            public void onCancel(DialogInterface dialog) {
                                dialog.dismiss();
                                mSingleton.finish();
                            }
                        }).create();
                dialog.show();
            }
        };
    }

    static public void exitGameWithError(String errorResId)
    {
        mSingleton.mUIThreadHandler.post(createRunnable(errorResId));
    }

    static public void exitGameSilent()
    {
        SharedPreferences.Editor editor = RobloxSettings.mKeyValues.edit();

        editor.putBoolean("returningFromExitGameSilent", true);
        editor.apply();
        editor.commit();

        mSingleton.finish();
    }

    static public String getApiUrl() {
        return RobloxSettings.baseUrlAPI();
    }

}
