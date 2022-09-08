package com.roblox.client;

import android.app.Fragment;
import android.content.Context;
import android.util.AttributeSet;
import android.widget.TabHost;

public class ReclickableTabHost extends TabHost {

    private ActivityNativeMain mMainReference = null;

    public ReclickableTabHost(Context context) {
        super(context);
    }

    public ReclickableTabHost(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    // We created this wrapper class so we can capture every click on a tab button.
    // The default tab host only fires the listener if we click on a different tab
    // than the currently active one.
    @Override
    public void setCurrentTab(int index) {
        if (index == getCurrentTab()) {
            Fragment currentFrag = mMainReference.getTabContents().get(index);
            if(currentFrag instanceof RobloxWebFragment) {
                ((RobloxWebFragment) currentFrag).loadDefaultUrl();
                return;
            }
        }
        super.setCurrentTab(index);
    }

    public void setActivityRef(ActivityNativeMain ref) {
        mMainReference = ref;
    }
}