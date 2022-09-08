package com.roblox.hybrid;

import android.util.Log;

import org.json.JSONObject;

import java.util.HashMap;

/**
 * Created by roblox on 3/5/15.
 */
public class RBHybridModule {

    static final String TAG = "RBHybridModule";
    protected static final String NOTIFICATION_MANAGER_POST_ACTION = "com.roblox.android.notificationmanager.POST";
    protected static final String NOTIFICATION_MANAGER_CALLBACK_ACTION = "com.roblox.hybrid.broadcastreceiver.RESPONSE";
    protected static final String CALLBACK_ACTION_TOP_BAR = ".getTopBarHeight";

    protected interface ModuleFunction {
        abstract void execute(final RBHybridCommand command);
    }

    private String mModuleID;
    private HashMap<String, ModuleFunction> mFunctions;

    public RBHybridModule(String moduleID) {
        mModuleID = moduleID;

        mFunctions = new HashMap<String, ModuleFunction>();

        this.registerFunction("supports", new SupportsFunction());
    }

    public String getModuleID() {
        return mModuleID;
    }

    protected void registerFunction(final String name, ModuleFunction function) {
        mFunctions.put(name, function);
    }

    protected boolean hasFunction(final String functionName) {
        return mFunctions.containsKey(functionName);
    }

    // Implement this method in your module
    public void execute(final RBHybridCommand command) {
        String functionName = command.getFunctionName();

        ModuleFunction function = mFunctions.get(functionName);
        if(function != null) {
            function.execute(command);
        } else {
            Log.e(TAG, "Cannot find function " + functionName + " in module " + mModuleID);
            command.executeCallback(false, null);
        }
    }

    protected void broadcastEvent(final String eventName, final JSONObject eventParams) {

    }

    private class SupportsFunction implements ModuleFunction {
        public void execute(final RBHybridCommand command) {
            String functionName = command.getParams().optString("functionName", "");
            boolean functionExists = RBHybridModule.this.hasFunction(functionName);
            command.executeCallback(functionExists, null);
        }
    }
}
