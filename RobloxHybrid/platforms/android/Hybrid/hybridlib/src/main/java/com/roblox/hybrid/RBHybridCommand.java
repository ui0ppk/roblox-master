package com.roblox.hybrid;

import android.content.Context;

import org.json.JSONObject;

import java.lang.ref.WeakReference;

/**
 * Created by roblox on 3/5/15.
 */
public class RBHybridCommand {

    private WeakReference<RBHybridWebView> mOriginWebView;

    private String mModuleID;
    private String mFunctionName;
    private JSONObject mParams;
    private String mCallbackID;

    public RBHybridCommand(WeakReference<RBHybridWebView> origin) {
        mOriginWebView = origin;
    }

    public String getModuleID() { return mModuleID; }
    public void setModuleID(final String moduleID) { mModuleID = moduleID; }

    public String getFunctionName() { return mFunctionName; }
    public void setFunctionName(final String functionName) { mFunctionName = functionName; }

    public JSONObject getParams() { return mParams; }
    public void setParams(final JSONObject params) { mParams = params; }

    public String getCallbackID() { return mCallbackID; }
    public void setCallbackID(final String callbackID) { mCallbackID = callbackID; }

    public Context getContext() {
        return mOriginWebView.get().getContext();
    }

    public void executeCallback(boolean success, final JSONObject params) {
        mOriginWebView.get().executeNativeCallback(this.mCallbackID, success, params);
    }
}
