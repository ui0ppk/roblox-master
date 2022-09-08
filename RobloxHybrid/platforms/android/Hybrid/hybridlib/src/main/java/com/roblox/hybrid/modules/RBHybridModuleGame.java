package com.roblox.hybrid.modules;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;

import com.roblox.hybrid.RBHybridCommand;
import com.roblox.hybrid.RBHybridModule;

import org.json.JSONObject;

/**
 * Created by roblox on 3/6/15.
 */
public class RBHybridModuleGame extends RBHybridModule {

    private static final String MODULE_ID = "Game";


    // @see NotificationManager.REQUEST_START_PLACEID == 101
    private static final int NOTIFICATION_MANAGER_REQUEST_START_PLACEID = 101;
    private static final int NOTIFICATION_MANAGER_REQUEST_START_PLACEID_PARTY = 106;
    private LocalBroadcastManager localBroadcastManager;

    public RBHybridModuleGame(LocalBroadcastManager localBroadcastManager) {
        super(MODULE_ID);
        this.localBroadcastManager = localBroadcastManager;

        this.registerFunction("launchPartyForPlaceId", new LaunchGamePartyJoin());
        this.registerFunction("startWithPlaceID", new StartWithPlaceId());

    }

    private class StartWithPlaceId implements ModuleFunction {
        @Override
        public void execute(RBHybridCommand command) {
            String strPlaceID = command.getParams().optString("placeID");
            if(strPlaceID != null) {
                Bundle userParams = new Bundle();
                userParams.putInt("placeId", Integer.parseInt(strPlaceID));
                userParams.putInt("requestType", 0);

                Intent broadcast = new Intent();
                broadcast.setAction(NOTIFICATION_MANAGER_POST_ACTION);
                broadcast.putExtra("notificationId", NOTIFICATION_MANAGER_REQUEST_START_PLACEID);
                broadcast.putExtra("userParams", userParams);
                localBroadcastManager.sendBroadcast(broadcast);

                command.executeCallback(true, null);
            } else {
                command.executeCallback(false, null);
            }
        }
    };

    private class LaunchGamePartyJoin implements ModuleFunction {
        @Override
        public void execute(RBHybridCommand command) {
            JSONObject params = command.getParams();

            String placeId = params.optString("placeId", "");
            Bundle bParam = new Bundle();
            bParam.putString("placeId", placeId);
            bParam.putInt("requestType", 4);

            Intent broadcast = new Intent();
            broadcast.setAction(NOTIFICATION_MANAGER_POST_ACTION);
            broadcast.putExtra("notificationId", NOTIFICATION_MANAGER_REQUEST_START_PLACEID_PARTY);
            broadcast.putExtra("userParams", bParam);
            localBroadcastManager.sendBroadcast(broadcast);

            command.executeCallback(true, null);
        }
    };
}
