package com.roblox.client;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.util.PriorityQueue;
import java.util.Queue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.SynchronousQueue;

/**
 * Created by scritelli on 3/3/16.
 * Built because either Android or InfluxDB .10 can't handle the volume of reports
 * we're sending and some reports are getting lost. This queue will ensure we only fire
 * one request to InfluxDB at a time.
 */
public class InfluxV2Queue {
    // Singleton (thread-safe)
    private static class Holder {
        static final InfluxV2Queue INSTANCE = new InfluxV2Queue();
    }

    public static InfluxV2Queue GetInstance() {
        return Holder.INSTANCE;
    }
    private final static String TAG = "InfluxV2Queue";
    private enum STATE {IDLE, PROCESSING};
    Queue<InfluxBuilderV2> mReportQueue;
    private STATE mCurrentState = STATE.IDLE;

    private Handler mThreadHandler;

    public InfluxV2Queue() {
        mReportQueue = new ArrayBlockingQueue<InfluxBuilderV2>(20);

        if (Looper.myLooper() == null)
            Looper.prepare();

        mThreadHandler = new Handler();
    }

    public void addToQueue(InfluxBuilderV2 ib) {
        try {
            mReportQueue.add(ib);
        } catch (IllegalStateException ise) {
            Log.e(TAG, "Queue full! Cannot add more reports.");
        }
        processQueue();
    }

    private void processQueue() {
        if (mCurrentState == STATE.IDLE) {
            mCurrentState = STATE.PROCESSING;
            if (AndroidAppSettings.InfluxUrl() != null) {
                InfluxBuilderV2 nextIb = mReportQueue.poll();
                if (nextIb != null)
                    nextIb.internalFireReport(finishedListener);
                else {
                    mCurrentState = STATE.IDLE;
                }
            } else { // we haven't loaded flags yet - wait to send data
                mCurrentState = STATE.IDLE;
                mThreadHandler.removeCallbacks(looperRunnable); // make sure we don't duplicate the runnable
                if (mReportQueue.size() > 0) {
                    // Start a timer to make sure we don't forget to send any reports
                    mThreadHandler.postDelayed(looperRunnable, 10000);
                }
            }
        }
    }

    private InfluxV2StateListener finishedListener = new InfluxV2StateListener() {
        @Override
        public void onReportFinished() {
            mCurrentState = STATE.IDLE;
            processQueue();
        }
    };

    private Runnable looperRunnable = new Runnable() {
        @Override
        public void run() {
            processQueue();
        }
    };
}
