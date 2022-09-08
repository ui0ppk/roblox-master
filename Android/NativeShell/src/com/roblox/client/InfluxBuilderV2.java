package com.roblox.client;

import android.os.Build;
import android.util.Log;

import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpPostRequest;

import org.json.JSONArray;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * Created by scritelli on 3/2/16.
 */
public class InfluxBuilderV2 {
    private final static String TAG = "InfluxBuilder";
    private StringBuilder mDataInternal = null;
    private StringBuilder mTagsInternal = null;
    private String mMeasurementName = "";

    public enum TIMESTAMP_PRECISION {NANOSECONDS, MICROSECONDS, MILLISECONDS, SECONDS, MINUTES, HOURS};
    private TIMESTAMP_PRECISION mTimestampPrecision;
    private long mTimestamp = 0l;

    ArrayList<String> mBatchReports;

    public InfluxBuilderV2(String measurementName) {
        mBatchReports = new ArrayList<>();
        mMeasurementName = measurementName;
        mTimestampPrecision = TIMESTAMP_PRECISION.MILLISECONDS;
        mTagsInternal = new StringBuilder();
        mDataInternal = new StringBuilder();
    }

    // Tag values are always stored as Strings, so we don't need any overloads
    public InfluxBuilderV2 addTag(String key, Object value) {
        mTagsInternal.append(key).append('=').append(value.toString().replace(" ", "\\ ").replace(",", "\\,")).append(",");
        return this;
    }

    public InfluxBuilderV2 addField(String key, Object value) {
        mDataInternal.append(key).append("=").append('"').append(value).append('"').append(",");
        return this;
    }

    public InfluxBuilderV2 addField(String key, long value) {
        mDataInternal.append(key).append("=").append(value).append("i,");
        return this;
    }

    public InfluxBuilderV2 addField(String key, int value) {
        mDataInternal.append(key).append("=").append(value).append("i,");
        return this;
    }

    public InfluxBuilderV2 addField(String key, boolean value) {
        mDataInternal.append(key).append("=").append(value).append(", ");
        return this;
    }

    public InfluxBuilderV2 setTimestamp(long timestamp) {
        mTimestamp = timestamp;
        return this;
    }

    public InfluxBuilderV2 setTimestampPrecision(TIMESTAMP_PRECISION precision)
    {
        mTimestampPrecision = precision;
        return this;
    }

    public InfluxBuilderV2 fireReport() {
        InfluxV2Queue.GetInstance().addToQueue(this);
        return this;
    }

    protected void internalFireReport(final InfluxV2StateListener listener) {
        if (!canSend()) {
            if (listener != null)
                listener.onReportFinished();

            return;
        }


        if (mTimestamp == 0) {
            mTimestamp = System.currentTimeMillis();
            mTimestampPrecision = TIMESTAMP_PRECISION.MILLISECONDS;
        }

        StringBuilder url = new StringBuilder().
                append(AndroidAppSettings.InfluxUrl()).
                append("/write?db=").
                append(AndroidAppSettings.InfluxDatabase()).
                append("&u=").
                append(AndroidAppSettings.InfluxUser()).
                append("&p=").
                append(AndroidAppSettings.InfluxPassword()).
                append("&precision=").
                append(getTimestampPrecision());



        RbxHttpPostRequest req = new RbxHttpPostRequest(url.toString(), getDataString(), null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                if (listener != null) listener.onReportFinished();

                // Debug
//                Log.i("SAMINFLUX", finalDatastring);
//                if (response.responseCode() != 204) {
//                    Log.i("SAMINFLUX", response.toString());
//                }
            }
        });
        req.execute();
    }

    public InfluxBuilderV2 addBatchReport(InfluxBuilderV2 ib) {
        mBatchReports.add(ib.getDataString());

        return this;
    }

    protected String getDataString() {
        // Add standard tags
        this.addTag("appVersion", BuildConfig.VERSION_NAME);
        this.addTag("deviceType", Build.MODEL);
        this.addTag("deviceOSVersion", Build.VERSION.SDK_INT);
        if (RobloxSettings.mBrowserTrackerId != null && !RobloxSettings.mBrowserTrackerId.isEmpty())
            this.addTag("browserTrackerId", RobloxSettings.mBrowserTrackerId);
        this.addTag("platform", "Android");
        this.addTag("reporter", "App");

        if (mTimestamp == 0) {
            mTimestamp = System.currentTimeMillis();
            mTimestampPrecision = TIMESTAMP_PRECISION.MILLISECONDS;
        }

        StringBuilder batchReportsCombined = new StringBuilder();
        for (int i = 0; i < mBatchReports.size(); i++) {
            batchReportsCombined.append(mBatchReports.get(i)).append("\n");
        }

        return new StringBuilder().
                append(mMeasurementName).
                append(",").
                append(removeTrailingComma(mTagsInternal.toString())).
                append(" ").
                append(removeTrailingComma(mDataInternal.toString())).
                append(" ").
                append(mTimestamp).
                append("\n").
                append(batchReportsCombined).
                toString();
    }

    private String getTimestampPrecision() {
        switch (mTimestampPrecision) {
            case NANOSECONDS:
                return "n";
            case MICROSECONDS:
                return "u";
            case MILLISECONDS:
                return "ms";
            case SECONDS:
                return "s";
            case MINUTES:
                return "m";
            case HOURS:
                return "h";
            default:
                return "ms";
        }
    }

    private boolean canSend() {
        return (new Random().nextInt(99) + 1) < AndroidAppSettings.InfluxThrottleRate();
    }

    private String removeTrailingComma(String input) {
        int pos = input.lastIndexOf(",");
        if (pos != -1)
            return input.substring(0, pos);
        else
            return input;
    }
}
