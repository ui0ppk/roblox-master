package com.roblox.hybrid.modules;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.roblox.hybrid.RBHybridCommand;
import com.roblox.hybrid.RBHybridModule;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Created by mcritelli on 11/10/15.
 */
public class RBHybridModuleChat extends RBHybridModule {
    private static final String MODULE_ID = "Chat";

    // @see NotificationManager.java
    private static final int NOTIFICATION_MANAGER_NEW_UNREAD_MESSAGE = 19;
    private static final int NOTIFICATION_MANAGER_REQUEST_ACTION_BAR_HEIGHT = 107;
    private static final int NOTIFICATION_MANAGER_REQUEST_KEYBOARD_HEIGHT = 108;
    private LocalBroadcastManager localBroadcastManager;

    public RBHybridModuleChat(LocalBroadcastManager localBroadcastManager) {
        super(MODULE_ID);
        this.localBroadcastManager = localBroadcastManager;

        this.registerFunction("newMessageNotification", new NewMessageNotification());
        this.registerFunction("getTopBarHeight", new GetTopBarHeight());
        this.registerFunction("getKeyboardHeight", new GetKeyboardHeight());
    }

    private class NewMessageNotification implements ModuleFunction {
        @Override
        public void execute(RBHybridCommand command) {
            JSONObject params = command.getParams();
            if (params != null) {
                Bundle bParam = new Bundle();

                try {
                    Object numUnreadMessages = params.get("numUnreadMessages");
                    if (numUnreadMessages instanceof String)
                        bParam.putInt("numUnreadMessages", Integer.parseInt((String)numUnreadMessages));
                    else if (numUnreadMessages instanceof Integer)
                        bParam.putInt("numUnreadMessages", params.getInt("numUnreadMessages"));
                    else
                        bParam.putInt("numUnreadMessages", 0);

                    Intent broadcast = new Intent();
                    broadcast.setAction(NOTIFICATION_MANAGER_POST_ACTION);
                    broadcast.putExtra("notificationId", NOTIFICATION_MANAGER_NEW_UNREAD_MESSAGE);
                    broadcast.putExtra("userParams", bParam);
                    localBroadcastManager.sendBroadcast(broadcast);

                    command.executeCallback(true, null);

                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private class GetTopBarHeight implements ModuleFunction {
        @Override
        public void execute(final RBHybridCommand command) {
            Log.i("SAM", "Inside GetTopBarHeight Java");
            Intent broadcast = new Intent();
            broadcast.setAction(NOTIFICATION_MANAGER_POST_ACTION);
            broadcast.putExtra("notificationId", NOTIFICATION_MANAGER_REQUEST_ACTION_BAR_HEIGHT);
            localBroadcastManager.sendBroadcast(broadcast);

            localBroadcastManager.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    Bundle data = intent.getBundleExtra("returnData");
                    int height = 0;
                    if (data != null) {
                        height = data.getInt("topBarHeight", 0);
                    }
                    JSONObject j = new JSONObject();
                    try {
                        j.put("topBarHeight", height);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    command.executeCallback(true, j);
                }
            }, new IntentFilter(NOTIFICATION_MANAGER_CALLBACK_ACTION + CALLBACK_ACTION_TOP_BAR));
        }
    }

    private class GetKeyboardHeight implements ModuleFunction {
        @Override
        public void execute(RBHybridCommand command) {
            Intent broadcast = new Intent();
            broadcast.setAction(NOTIFICATION_MANAGER_POST_ACTION);
            broadcast.putExtra("notificationId", NOTIFICATION_MANAGER_REQUEST_KEYBOARD_HEIGHT);
            localBroadcastManager.sendBroadcast(broadcast);
            JSONObject j = new JSONObject();
            try {
                j.put("keyboardHeight", 50);
            } catch (JSONException e) {
                e.printStackTrace();
            }

            command.executeCallback(true, j);
        }
    }
}
