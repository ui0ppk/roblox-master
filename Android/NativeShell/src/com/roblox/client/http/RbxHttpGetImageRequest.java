package com.roblox.client.http;

import android.graphics.Bitmap;
import android.os.AsyncTask;

public class RbxHttpGetImageRequest extends AsyncTask<Void, Void, Void>{
    private String TAG = "RbxHttpGetImageRequest";
    private Bitmap mResponse = null;
    private String mUrl = null;
    OnRbxHttpBitmapRequestFinished mRequestFinished = null;

    public RbxHttpGetImageRequest(String url, OnRbxHttpBitmapRequestFinished req) {
        mRequestFinished = req;
        mUrl = url;
    }

    @Override
    protected Void doInBackground(Void... arg0) {
        mResponse = HttpAgent.readUrlToBitmap(mUrl);
        return null;
    }

    @Override
    protected void onPostExecute(Void result) {
        super.onPostExecute(result);
        mRequestFinished.onFinished(mResponse);
    }
}

