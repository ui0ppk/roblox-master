package com.roblox.client;

import com.crashlytics.android.Crashlytics;
import io.fabric.sdk.android.Fabric;
import java.io.IOException;
import java.lang.ref.WeakReference;

import android.app.Application;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.support.multidex.MultiDex;
import android.util.Log;

import com.crashlytics.android.ndk.CrashlyticsNdk;
import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.Tracker;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesUtil;

public class RobloxApplication extends Application {
    private static String TAG = "roblox.app";

    static WeakReference<RobloxApplication> mInstance = null;
    WeakReference<RobloxActivity> mCurrentActivity = null;
    boolean mGooglePlayServicesAvailable = false;

    // ------------------------------------------------------------------------
    public RobloxApplication() {
        mInstance = new WeakReference<RobloxApplication>(this);
    }

    public static RobloxApplication getInstance() {
        return mInstance != null ? mInstance.get() : null;
    }

    // ------------------------------------------------------------------------
    public RobloxActivity getCurrentActivity() {
        return (mCurrentActivity == null ? null : mCurrentActivity.get());
    }

    public void setCurrentActivity(RobloxActivity activity) {
        mCurrentActivity = new WeakReference<RobloxActivity>(activity);
    }

    // ------------------------------------------------------------------------
    // CriticalError
    
	// A way to stop the App from becoming further corrupted/crashing while
	// remaining in a debuggable state.  Also to tell us nicely.
	static String mCriticalError = null;
	
	public void setCriticalErrorOccured( String msg )
	{
		mCriticalError = msg;
		Log.e(TAG, "************************************************************" );
		Log.e(TAG, "***  CriticalError: " + mCriticalError );
		Log.e(TAG, "************************************************************" );

		throw new RuntimeException( msg );
	}
	
	// Cannot be called during startup, only from an Activity.
	public boolean checkShowCriticalError()
	{
		// Seems that there is no reliable time to show an alert.  Just spew
		// this message so it stands out from the rest of the noise.
		if( mCriticalError != null )
		{
			Log.e(TAG, "***  CriticalError: " + mCriticalError );
		}
		return mCriticalError != null;
	}
	
    public void onCreate() {
        super.onCreate();
        Fabric.with(this, new Crashlytics(), new CrashlyticsNdk());

        try {
			RobloxSettings.initConfig( getApplicationContext() );
		} catch (IOException e) {
			setCriticalErrorOccured( e.getMessage() );
		}

        int res = GooglePlayServicesUtil.isGooglePlayServicesAvailable(this);
        if (res != ConnectionResult.SUCCESS) {
            Log.w(TAG, "isGooglePlayServicesAvailable: No");
        } else {
            mGooglePlayServicesAvailable = true;
        }

        String ua = RobloxSettings.userAgent();
        Log.i( TAG, ua );
    }

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        MultiDex.install(this);
    }

    // ------------------------------------------------------------------------
    // Gooooooogle analytics

    // -----------------------------------------------------------------------
    protected boolean isGooglePlayServicesAvailable() {
        return mGooglePlayServicesAvailable;
    }

    private Tracker mAndroidTracker = null;
    synchronized Tracker getAndroidTracker()
    {
    	if (mAndroidTracker == null)
    	{
    		GoogleAnalytics analytics = GoogleAnalytics.getInstance(this);
    		mAndroidTracker = analytics.newTracker(R.xml.android_tracker);
            mAndroidTracker.setSampleRate(5);
    	}
    	return mAndroidTracker;
	}    

    // ------------------------------------------------------------------------
    // Memory Tracking
    
    static void logTrimMemory( String TAG, int level )
    {
    	switch( level )
    	{
    	case ComponentCallbacks2.TRIM_MEMORY_COMPLETE:			Log.w(TAG, "TRIM_MEMORY_COMPLETE");			break;
		case ComponentCallbacks2.TRIM_MEMORY_MODERATE:			Log.w(TAG, "TRIM_MEMORY_MODERATE");			break;
		case ComponentCallbacks2.TRIM_MEMORY_BACKGROUND:		Log.w(TAG, "TRIM_MEMORY_BACKGROUND");		break;
		case ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN:			Log.w(TAG, "TRIM_MEMORY_UI_HIDDEN");		break;
		
		// (only available in API LEVEL >= 16)
		//case ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL:	Log.w(TAG, "TRIM_MEMORY_RUNNING_CRITICAL");	break;
		//case ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW:		Log.w(TAG, "TRIM_MEMORY_RUNNING_LOW");		break;
		//case ComponentCallbacks2.TRIM_MEMORY_RUNNING_MODERATE:	Log.w(TAG, "TRIM_MEMORY_RUNNING_MODERATE");	break;
    	}
    }
}




