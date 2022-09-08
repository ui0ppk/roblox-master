package com.roblox.client.http;

import android.os.AsyncTask;

public class RbxHttpPostRequest extends AsyncTask<Void, Void, HttpResponse>{
    private String TAG = "RobloxHTTPPostRequest";
    HttpResponse mResponse = null;
    private String mUrl = null;
    private String mPostParams = null;
    private HttpAgent.HttpHeader[] mHeaderList = null;
    OnRbxHttpRequestFinished mRequestFinished = null;

    public RbxHttpPostRequest(String url, String postParams, HttpAgent.HttpHeader[] headerList, OnRbxHttpRequestFinished req) {
        mRequestFinished = req;
        mUrl = url;
        mPostParams = postParams;
        mHeaderList = headerList;
        if (postParams == null) mPostParams = ""; // if it's null, HttpAgent won't set the request type as POST
    }

    @Override
    protected HttpResponse doInBackground(Void... arg0) {
//        mResponse = HttpAgent.readUrlToString( mUrl, mPostParams, mHeaderList ); // POST not GET?
        mResponse = HttpAgent.readUrl(mUrl, mPostParams, mHeaderList);
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

