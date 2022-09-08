package com.roblox.client;

import java.util.ArrayList;

import android.annotation.SuppressLint;
import android.app.Service;
import android.content.Intent;
import android.os.Debug;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;


public class RobloxService extends Service {
    private static String SERVICETAG = "robloxservice";

	public static final int MSG_REGISTER_CLIENT		 = 1;
    public static final int MSG_UNREGISTER_CLIENT	 = 2;
    public static final int MSG_NOTIFICATION		 = 3;

	private Messenger mMessenger = new Messenger(new IncomingHandler());
	private ArrayList<Messenger> mClients = new ArrayList<Messenger>();
	private boolean mWaitForDebugger = false;
	private boolean mAlreadyWaitedForDebugger = false;
	
	// ------------------------------------------------------------------------
	// Service
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		// *If* the service is killed the OS does not need to restart it.
		return Service.START_NOT_STICKY;
	}	
	
	@Override
    public IBinder onBind(Intent intent) {
		// Assume the service is always in a different process.
		
        boolean launcherDebuggerAttached = intent.getBooleanExtra( "roblox_launcher_debugger_attached", false );
        if( launcherDebuggerAttached )
        {
        	// Waiting now gets the service killed.
        	mWaitForDebugger = true;
        }
        return mMessenger.getBinder();
    }

	@Override
    public void onCreate() {
    	super.onCreate();
    	
    	Log.i(SERVICETAG, "RobloxService onCreate");

        // Start unpacking the assets, if necessary
        XAPKManager.unpackAssetsAsync(this);
    }

    @Override
    public void onDestroy() {
    	Log.w(SERVICETAG, "RobloxService onDestroy");
    	super.onDestroy();
    }

	// ------------------------------------------------------------------------
	// IncomingHandler

    @SuppressLint("HandlerLeak")
    private class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if( mWaitForDebugger && !mAlreadyWaitedForDebugger )
            {
               	Log.w(SERVICETAG, "RobloxService waiting for debugger");
               	Debug.waitForDebugger();
               	mAlreadyWaitedForDebugger = true;
            }
        	
            switch (msg.what)
            {
                case MSG_REGISTER_CLIENT:
                    mClients.add(msg.replyTo);
                    break;
                case MSG_UNREGISTER_CLIENT:
                    mClients.remove(msg.replyTo);
                    break;
                case MSG_NOTIFICATION:
                {
                    // Broadcast message to all the clients
                    for(Messenger messenger : mClients) {
                        // But do not send the message back to the sender
                        if( msg.replyTo.equals(messenger) ) continue;

                        try {
                            Message message = Message.obtain(msg);
                            messenger.send(message);
                        } catch (RemoteException e) {
                            Log.e(SERVICETAG, Utils.format("Remote exception: ." + e.getMessage()));
                        }
                    }
                    break;
                }
                default:
                    super.handleMessage(msg);
            }
        }
    }
}

