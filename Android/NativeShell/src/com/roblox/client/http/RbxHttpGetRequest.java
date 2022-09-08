package com.roblox.client.http;

import android.os.AsyncTask;

public class RbxHttpGetRequest extends AsyncTask<Void, Void, HttpResponse>{
    private String TAG = "RobloxHTTPGetRequest";
    private HttpResponse mResponse = null;
    private String mUrl = null;
    OnRbxHttpRequestFinished mRequestFinished = null;

    public RbxHttpGetRequest(String url, OnRbxHttpRequestFinished req) {
        mRequestFinished = req;
        mUrl = url;
    }

    @Override
    protected HttpResponse doInBackground(Void... arg0) {
        mResponse = HttpAgent.readUrl(mUrl, null, null);
        return mResponse;
    }

    @Override
    protected void onPostExecute(HttpResponse result) {
        super.onPostExecute(result);
        mRequestFinished.onFinished(mResponse);
    }

    public void execute(){
        // execute tasks in parallel
        this.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
}

