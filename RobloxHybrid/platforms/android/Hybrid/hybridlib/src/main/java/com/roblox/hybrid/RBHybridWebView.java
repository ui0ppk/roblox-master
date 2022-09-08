package com.roblox.hybrid;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.support.v4.content.LocalBroadcastManager;
import android.util.AttributeSet;
import android.util.Log;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;

import com.roblox.hybrid.modules.RBHybridModuleAnalytics;
import com.roblox.hybrid.modules.RBHybridModuleChat;
import com.roblox.hybrid.modules.RBHybridModuleDialogs;
import com.roblox.hybrid.modules.RBHybridModuleGame;
import com.roblox.hybrid.modules.RBHybridModuleInput;
import com.roblox.hybrid.modules.RBHybridModuleSocial;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.LinkedList;

/**
 *
 * RBHybridWebView
 *
 */
public class RBHybridWebView extends WebView {

    private static final String TAG = "RBHybrid";

    private WeakReference<RBHybridWebView> mWeakSelf;
    static private LinkedList<WeakReference<RBHybridWebView>> mWebViewInstances = new LinkedList<>();

    // WebView settings
    private String mUserAgent;
    private Handler mMainThreadHandler;

    private LocalBroadcastManager localBroadcastManager = LocalBroadcastManager.getInstance(getContext());

    // List of modules. This list is common for all webviews
    static private HashMap<String, RBHybridModule> mModules;

    // JSBridge
    private class JSBridge {

        @JavascriptInterface
        public void executeRoblox(final String query) {

            // Run on main thread
            mMainThreadHandler.post( new Runnable() {
                @Override
                public void run() {
                    processCommand(query);
                }
            });

        }
    }

    public RBHybridWebView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr); // passing in defStyleRes is only available in API 21

        this.init();
    }

    public RBHybridWebView(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.init();
    }

    public RBHybridWebView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        this.init();
    }

    public RBHybridWebView(Context context) {
        super(context);
        this.init();
    }

    private void init() {
        // Register webview instance
        mWeakSelf = new WeakReference<RBHybridWebView>(this);
        mWebViewInstances.add(mWeakSelf);

        // Main thread handler
        mMainThreadHandler = new Handler( this.getContext().getMainLooper() );

        // Initialize modules
        this.registerModules();

        // Inject a JS interface into the client code
        JSBridge bridge = new JSBridge();
        this.addJavascriptInterface(bridge, "__globalRobloxAndroidBridge__");

        // TODO: Remove
        this.setWebChromeClient(new WebChromeClient());
    }

    @Override
    protected void finalize() throws Throwable {
        mWebViewInstances.remove(mWeakSelf);
        super.finalize();
    }

    private void registerModule(RBHybridModule module) {
        mModules.put(module.getModuleID(), module);
    }

    //
    private void registerModules() {
        if(mModules == null) {
            mModules = new HashMap<String, RBHybridModule>();

            registerModule( new RBHybridModuleSocial() );
            registerModule( new RBHybridModuleDialogs() );
            registerModule( new RBHybridModuleAnalytics());
            registerModule( new RBHybridModuleGame(localBroadcastManager) );
            registerModule(new RBHybridModuleChat(localBroadcastManager));
            registerModule( new RBHybridModuleInput(this, localBroadcastManager));
        }
    }

    // This function should always be executed on the main thread
    private void processCommand(final String query) {
        try {
            RBHybridCommand command = new RBHybridCommand( new WeakReference<RBHybridWebView>(this) );

            JSONObject jsonObject = new JSONObject(query);

            command.setModuleID( jsonObject.getString("moduleID") );
            command.setFunctionName(jsonObject.getString("functionName"));
            command.setParams( jsonObject.getJSONObject("params") );
            command.setCallbackID( jsonObject.optString("callbackID") );

            RBHybridModule module = mModules.get( command.getModuleID() );
            if(module != null) {
                module.execute(command);
            } else {
                Log.e(TAG, "Couldn't find module with ID: " + command.getModuleID());
            }
        } catch(JSONException jsonExecption) {
            Log.e(TAG, "There was an error parsing the JSON command: " + jsonExecption.getMessage());
        }
    }

    @Override
    public void loadUrl(java.lang.String url)
    {
        // Load the url
        //super.loadUrl("http://172.17.250.128:8080/index.html");
        super.loadUrl(url);
    }

    public void executeNativeCallback(final String callbackID, boolean success, JSONObject params)
    {
        String strParams = params != null ? params.toString() : "{}";

        final String jsExec = String.format( /*"window.Roblox.Hybrid.Bridge.nativeCallback('%s', %s, %s);",*/
                "if (window.Roblox.Hybrid &&" +
                        " window.Roblox.Hybrid.Bridge.nativeCallback &&" +
                        " typeof window.Roblox.Hybrid.Bridge.nativeCallback === \"function\") { window.Roblox.Hybrid.Bridge.nativeCallback('%s', %s, %s); }",
                callbackID,
                success ? "true" : "false",
                strParams
        );

        if( mMainThreadHandler.getLooper().getThread() == Thread.currentThread() ) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
                this.evaluateJavascript(jsExec, null);
            else
            {
                String js = "javascript:" + jsExec;
                this.loadUrl(js);
            }
        } else {
            mMainThreadHandler.post( new Runnable() {
                @Override
                public void run() {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
                        RBHybridWebView.this.evaluateJavascript(jsExec, null);
                    else {
                        String js = "javascript:" + jsExec;
                        RBHybridWebView.this.loadUrl(js);
                    }
                }
            });
        }
    }

    // Broadcast event to all the webviews
    static public void broadcastEvent(final RBHybridEvent event) {
        for(int i = 0; i < mWebViewInstances.size(); i++) {
            RBHybridWebView webview = mWebViewInstances.get(i).get();
            webview.emitEvent(event);
        }
    }

    // Fire an event in this webview
    public void emitEvent(final RBHybridEvent event) {

        String strParams = event.getParams() != null ? event.getParams().toString() : "{}";

        final String jsExec = String.format(/*"window.Roblox.Hybrid.Bridge.emitEvent('%s', '%s', %s);",(*/
                "if (window.Roblox.Hybrid && window.Roblox.Hybrid.Bridge.emitEvent && typeof window.Roblox.Hybrid.Bridge.emitEvent === \"function\") { window.Roblox.Hybrid.Bridge.emitEvent('%s', '%s', %s); }",
                event.getModuleName(),
                event.getName(),
                strParams
        );

        if( mMainThreadHandler.getLooper().getThread() == Thread.currentThread() ) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
                this.evaluateJavascript(jsExec, null);
            else
            {
                String js = "javascript:" + jsExec;
                this.loadUrl(js);
            }
        } else {
            mMainThreadHandler.post( new Runnable() {
                @Override
                public void run() {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
                        RBHybridWebView.this.evaluateJavascript(jsExec, null);
                    else {
                        String js = "javascript:" + jsExec;
                        RBHybridWebView.this.loadUrl(js);
                    }
                }
            });
        }
    }
}
