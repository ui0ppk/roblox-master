package com.roblox.client.managers;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.roblox.client.RobloxActivity;
import com.roblox.client.RobloxApplication;
import com.roblox.client.RobloxService;
import com.roblox.client.Utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 * NotificationManager
 *
 * Custom implementation of the observer/signal-slot patter
 * Notifications are always delivered in the main thread
 */
public class NotificationManager {

    static private final String TAG = "NotificationManager";

    static private final String NOTIFICATION_MANAGER_POST_ACTION = "com.roblox.android.notificationmanager.POST";

    // EVENTS
    //  Notifications of events
    static public final int EVENT_USER_LOGIN = 1;
    static public final int EVENT_USER_LOGOUT = 2;
    static public final int EVENT_USER_INFO_UPDATED = 3;
    static public final int EVENT_USER_LOGIN_STARTED = 4;
    static public final int EVENT_USER_LOGIN_STOPPED = 5;
    static public final int EVENT_USER_CAPTCHA_SOLVED = 6;
    static public final int EVENT_USER_CAPTCHA_REQUESTED = 7;
    static public final int EVENT_USER_CREATE_USERNAME_REQUESTED = 8;
    static public final int EVENT_USER_CREATE_USERNAME_SUCCESSFUL = 9;
    static public final int EVENT_USER_FACEBOOK_CONNECTED = 10;
    static public final int EVENT_USER_FACEBOOK_DISCONNECTED = 11;
    static public final int EVENT_USER_FACEBOOK_INFO_UPDATED = 12;
    static public final int EVENT_USER_FACEBOOK_CONNECT_STARTED = 13;
    static public final int EVENT_USER_FACEBOOK_DISCONNECT_STARTED = 14;
    static public final int EVENT_USER_FACEBOOK_CONNECT_STOPPED = 15;
    static public final int EVENT_USER_FACEBOOK_DISCONNECT_STOPPED = 16;
    static public final int EVENT_USER_CAPTCHA_SOCIAL_SOLVED = 17;
    static public final int EVENT_USER_CAPTCHA_SOCIAL_REQUESTED = 18;
    static public final int EVENT_TEST = 999;

    // REQUESTS
    //  Request to another entity to perform an action
    static public final int REQUEST_START_PLACEID = 101;
    static public final int REQUEST_USER_SEARCH = 102;
    static public final int REQUEST_OPEN_LOGIN = 103;
    static public final int REQUEST_GAME_SEARCH = 104;
    static public final int REQUEST_CATALOG_SEARCH = 105;


    // ------------------------------------------------------------------------
    //  Singleton (thread-safe)
    private static class Holder {
        static final NotificationManager INSTANCE = new NotificationManager();
    }
    static public NotificationManager getInstance() {
        return Holder.INSTANCE;
    }

    // ------------------------------------------------------------------------
    public interface Observer {
        public abstract void handleNotification(int notificationId, Bundle userParams);
    }

    // ------------------------------------------------------------------------
    private final Handler mHandler = new Handler( Looper.getMainLooper() );
    private final ArrayList<Observer> mObservers = new ArrayList<Observer>();
    private List<Message> mPendingMessages = new ArrayList<Message>();
    private Messenger mRobloxService = null;
    private Messenger mMessenger = new Messenger(new IncomingHandler());

    // ------------------------------------------------------------------------
    public NotificationManager() {
        bindService();
    }

    // ------------------------------------------------------------------------
    public void addObserver(final Observer observer) {
        mHandler.post( new Runnable() {
            @Override
            public void run() {
                mObservers.add(observer);
            }
        });
    }

    // ------------------------------------------------------------------------
    public void removerObserver(final Observer observer) {
        mHandler.post( new Runnable() {
            @Override
            public void run() {
                while( mObservers.remove(observer) );
            }
        });
    }

    // ------------------------------------------------------------------------
    public void postNotification(final int notificationId) {
        this.postNotification(notificationId, null);
    }

    // ------------------------------------------------------------------------
    public void postNotification(final int notificationId, final Bundle userParams) {
        postLocalNotification(notificationId, userParams);
        postRemoteNotification(notificationId, userParams);
    }

    // -----------------------------------------------------------------------
    private void postLocalNotification(final int notificationId, final Bundle userParams) {
        mHandler.post( new Runnable() {
            @Override
            public void run() {
                for(Observer observer : mObservers) {
                    observer.handleNotification(notificationId, userParams);
                }
            }
        });
    }

    // ------------------------------------------------------------------------
    private void postRemoteNotification(final int notificationId, final Bundle userParams) {
        Message message = Message.obtain();
        message.replyTo = mMessenger;
        message.what = RobloxService.MSG_NOTIFICATION;
        message.arg1 = notificationId;
        message.arg2 = 0;
        message.setData(userParams);

        if(mRobloxService == null) {
            mPendingMessages.add(message);
        } else {
            try {
                mRobloxService.send(message);
            } catch (RemoteException e) {
                Log.e(TAG, Utils.format("NotificationManager.postRemoteNotification failed service dead"));
            }
        }
    }

    // ------------------------------------------------------------------------
    protected void bindService() {
        Context context = RobloxApplication.getInstance();

        Intent intent = new Intent(context, RobloxService.class);

        boolean debuggerAttached = Debug.isDebuggerConnected();
        intent.putExtra("roblox_launcher_debugger_attached", debuggerAttached );

        boolean isOk = context.bindService(intent, mRobloxServiceConnection, Context.BIND_AUTO_CREATE);
        if( !isOk )
        {
            Log.e(TAG, Utils.format("NotificationManager.doBindService failed"));
        }

        LocalBroadcastManager.getInstance(context).registerReceiver(mMessageReceiver, new IntentFilter(NOTIFICATION_MANAGER_POST_ACTION));
    }

    // ------------------------------------------------------------------------
    protected void unbindService() {
        if (mRobloxService != null)
        {
            try
            {
                Message msg = Message.obtain(null, RobloxService.MSG_UNREGISTER_CLIENT);
                msg.replyTo = mMessenger;
                mRobloxService.send(msg);
            }
            catch (RemoteException e)
            {
                Log.e(TAG, Utils.format("NotificationManager.unbindService wtf service already dead."));
            }
            mRobloxService = null; // There is no callback.
        }

        LocalBroadcastManager.getInstance(RobloxApplication.getInstance()).unregisterReceiver(mMessageReceiver);

        Context context = RobloxApplication.getInstance();
        context.unbindService(mRobloxServiceConnection);
    }

    // ------------------------------------------------------------------------
    private ServiceConnection mRobloxServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            mRobloxService = new Messenger(service);

            try {
                Message msg = Message.obtain(null, RobloxService.MSG_REGISTER_CLIENT);
                msg.replyTo = mMessenger;
                mRobloxService.send(msg);

                Log.i(TAG, "NotificationManager.onServiceConnected success");

                // Deliver pending
                for( Message pendingMsg : mPendingMessages )
                {
                    try {
                        mRobloxService.send(pendingMsg);
                    } catch (RemoteException e) {
                        Log.e(TAG, Utils.format("NotificationManager.doNotifyService failed service dead"));
                    }
                }
                mPendingMessages.clear();

            } catch (RemoteException e) {
                Log.e(TAG, "NotificationManager.onServiceConnected failed");
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            // *** This never happens.  Only called when the service process exits.
            Log.i(TAG, "NotificationManager.onServiceDisconnected");
        }
    };

    // ------------------------------------------------------------------------
    // Events reflected off the service.
    private class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case RobloxService.MSG_NOTIFICATION:
                    // Send notification
                    final int notificationId = msg.arg1;
                    final Bundle userParams = msg.getData();
                    postLocalNotification(notificationId, userParams);
                    Log.i(TAG, String.format("NotificationManager.handleMessage remote %d %s", notificationId, userParams.toString()));
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Handler for received Intents,
    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // Extract data included in the Intent
            int notificationId = intent.getIntExtra("notificationId", -1);
            Log.v(TAG, Utils.format("NotificationManager > BroadcastReceiver.onReceive() notificationId:" + notificationId));

            if(notificationId == NotificationManager.REQUEST_START_PLACEID) {
                Bundle userParams = intent.getBundleExtra("userParams");
                postNotification(NotificationManager.REQUEST_START_PLACEID, userParams);
            }
        }
    };
}
