package com.roblox.client;

import java.io.File;

import android.graphics.Color;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

public class ActivityCurlTest extends RobloxActivity {
    private static String TAG = "roblox.activitycurltest";
    private TextView textView;
    
	static {
		System.loadLibrary("curl");
	    System.loadLibrary("roblox");
	}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "onCreate()");
        
        setContentView(R.layout.activity_curltest);
        textView = (TextView)findViewById(R.id.textView);
        textView.setMovementMethod(new ScrollingMovementMethod());
        textView.setTextColor(Color.BLACK);
        textView.setBackgroundColor(Color.WHITE);
    }
    
    public void onURLGo(View view)
    {
    	EditText urlEdit = (EditText)findViewById(R.id.url);
    	String url = urlEdit.getText().toString();
    	textView.setText("Loading " + url + "....\n");
    	String html = nativeGetURL(url);
//    	String response = nativePostAnalytics("Game", "GetURL", 0, "none");
//    	textView.append("Analytics: " + response + "\n");
    	textView.append("HTML: " + html + "\n");
    }

    @Override
    protected void onStart() {
        super.onStart();
        nativeOnStart();

        File cacheDir = getCacheDir();
        String cacheDirName = cacheDir.getAbsolutePath();
        Log.i(TAG, "cacheDirName = " + cacheDirName);
    }

    @Override
    protected void onResume() {
        super.onResume();
        nativeOnResume();
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        nativeOnPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        nativeOnStop();
    }

    @Override
    public void onTrimMemory( int level) {
        super.onTrimMemory( level );
        RobloxApplication.logTrimMemory( TAG, level );
    }    
    
    public static native void nativeOnStart();
    public static native void nativeOnResume();
    public static native void nativeOnPause();
    public static native void nativeOnStop();
    
    public static native String nativeGetURL(String url);
    public static native String nativePostAnalytics(String category, String action, int value, String label);
}
