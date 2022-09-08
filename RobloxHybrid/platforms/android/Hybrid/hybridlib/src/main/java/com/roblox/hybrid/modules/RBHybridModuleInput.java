package com.roblox.hybrid.modules;

import android.content.Intent;
import android.graphics.Rect;
import android.support.v4.content.LocalBroadcastManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewTreeObserver;

import com.roblox.hybrid.RBHybridEvent;
import com.roblox.hybrid.RBHybridModule;
import com.roblox.hybrid.RBHybridWebView;

import org.json.JSONException;
import org.json.JSONObject;

public class RBHybridModuleInput extends RBHybridModule{
    private static final String TAG = "HybridInput";

    private static final String MODULE_ID = "Input";
    private LocalBroadcastManager localBroadcastManager;

    private static final String ON_KEYBOARD_SHOW_EVENT_ID = "onKeyboardShow";
    private static final String ON_KEYBOARD_HIDE_EVENT_ID = "onKeyboardHide";
    static public final int NOTIFICATION_MANAGER_REQUEST_HIDE_NAV_BAR = 109;
    static public final int NOTIFICATION_MANAGER_REQUEST_SHOW_NAV_BAR = 110;

    public RBHybridModuleInput(RBHybridWebView hybridView, LocalBroadcastManager localBroadcastManager) {
        super(MODULE_ID);
        this.localBroadcastManager = localBroadcastManager;

        initKeyboardListener(hybridView);
    }

    private static boolean isKeyboardShowing = false;

    private void initKeyboardListener(RBHybridWebView hybridView) {
        final View rootView = hybridView.getRootView();
        hybridView.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                /* 128dp = 32dp * 4, minimum button height 32dp and generic 4 rows soft keyboard */
                final int SOFT_KEYBOARD_HEIGHT_DP_THRESHOLD = 128;

                Rect r = new Rect();
                rootView.getWindowVisibleDisplayFrame(r);
                DisplayMetrics dm = rootView.getResources().getDisplayMetrics();

                /* heightDiff = rootView height - status bar height (r.top) - visible frame height (r.bottom - r.top) */
                int heightDiff = rootView.getBottom() - r.bottom;

                /* Threshold size: dp to pixels, multiply with display density */
                boolean didKeyboardChange = heightDiff > SOFT_KEYBOARD_HEIGHT_DP_THRESHOLD * dm.density;
                RBHybridEvent event = new RBHybridEvent();
                event.setModuleName(MODULE_ID);
                JSONObject j = new JSONObject();
                try {
                    j.put("keyboardHeight", heightDiff);
                    event.setParams(j);
                } catch (JSONException e) {

                }


                if (!didKeyboardChange && isKeyboardShowing) {
                    isKeyboardShowing = false;
                    event.setName(ON_KEYBOARD_HIDE_EVENT_ID);

                    RBHybridWebView.broadcastEvent(event);
                } else if (didKeyboardChange && !isKeyboardShowing) {
                    isKeyboardShowing = true;
                    event.setName(ON_KEYBOARD_SHOW_EVENT_ID);

                    RBHybridWebView.broadcastEvent(event);
                }
//                Log.v(TAG, "isKeyboardShown ? " + isKeyboardShown + ", heightDiff:" + heightDiff + ", density:" + dm.density
//                        + "root view height:" + rootView.getHeight() + ", rect:" + r);
            }
        });
    }

}
