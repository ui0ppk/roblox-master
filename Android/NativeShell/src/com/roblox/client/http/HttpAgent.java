package com.roblox.client.http;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Array;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;

import com.roblox.client.RobloxSettings;

//
// Handles HTTP communication using same cookies as WebView. Configuration data goes in Config.java.
//
// Browser header check:  "http://pgl.yoyo.org/http/browser-headers.php"
//

public class HttpAgent {
	private static final String TAG = "roblox.httpagent";
	private static final String XSRF_HEADER_NAME = "X-CSRF-TOKEN";
	public static final String XSRF_ERROR_MESSAGE = "XSRF Token Validation Failed";
	private static String mLatestXSRFToken = null;

	static public class HttpHeader
	{
		public String header;
		public String value;
	}
	
	static public void onCreate(Context context) {
        CookieSyncManager.createInstance(context);
        CookieManager.getInstance().setAcceptCookie(true);       
        CookieSyncManager.getInstance().startSync();
        
        // Install a proxy which directs HttpURLConnection to use the WebView cookies. 
        WebkitCookieManagerProxy coreCookieManager = new WebkitCookieManagerProxy(null, java.net.CookiePolicy.ACCEPT_ALL);
        java.net.CookieHandler.setDefault(coreCookieManager);
    }

	static public void onPause(File cacheDir, String url) {
		CookieSyncManager.getInstance().stopSync();
	}	

	static public void onResume() {
		CookieSyncManager.getInstance().startSync();
	}

    static public void onStop() {
        CookieSyncManager.getInstance().sync();
    }

	static public HttpURLConnection urlToConnection( String url, String post,  HttpHeader[] headerList ) throws IOException {
		return urlToConnection(url, post, headerList, false);
	}

	/**
	 * @param isXsrfRetry
	 * 		If true, this request is a XSRF retry attempt, do not attempt another retry.
	 */
	static public HttpURLConnection urlToConnection( String url, String post,  HttpHeader[] headerList, boolean isXsrfRetry ) throws IOException {
		HttpURLConnection httpConnection = null;

		URL urlObj = new URL(url);
		httpConnection = (HttpURLConnection)urlObj.openConnection();

		// Setup Header
		if(headerList != null && Array.getLength(headerList) > 0)
		{
			for(int i = 0; i < headerList.length; ++i)
			{
				HttpHeader header = headerList[i];
				httpConnection.setRequestProperty(header.header, header.value );
			}
		}

		httpConnection.setRequestProperty("User-Agent", RobloxSettings.userAgent() );

		// 1 minute timeout seems like long enough.
		httpConnection.setConnectTimeout(1000*60);
		httpConnection.setReadTimeout(1000*60);
		
		if( post != null )
		{
            httpConnection.setRequestMethod("POST");
			httpConnection.setDoOutput(true);
			httpConnection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
			
			// XSRF Protection: Post a message that modifies the state of the server using a safety ROBLOX token
			if(mLatestXSRFToken != null)
			{
				httpConnection.setRequestProperty(XSRF_HEADER_NAME, mLatestXSRFToken);
			}

			OutputStreamWriter out = new OutputStreamWriter(httpConnection.getOutputStream());
			out.write(post);
			out.close();
		}

		int status = httpConnection.getResponseCode();
		if(status == 403 && XSRF_ERROR_MESSAGE.equals(httpConnection.getResponseMessage())){
			// If the status code is 403 (XSRF Token Validation Failed),
			// the request should be reposted with the new token provided in the response
			String xsrfToken = httpConnection.getHeaderField(XSRF_HEADER_NAME);
			if(!isXsrfRetry && xsrfToken != null){
				Log.i(TAG, "XSRF: got token. retrying");
				mLatestXSRFToken = xsrfToken;
				return HttpAgent.urlToConnection(url, post, headerList, true);
			}
			else if(isXsrfRetry){
				Log.e(TAG, "XSRF Error: retry already attempted. Will not retry");
			}
			else{
				Log.e(TAG, "XSRF Error: token not present in response. Will not retry");
			}
		}

        if( status != 200 ) {
            Log.i(TAG, "User-Agent:" + httpConnection.getRequestProperty("User-Agent"));
            Log.i(TAG, "URL:" + url);
            Log.i(TAG, "HTTP Status:" + status);
        }
        else
            CookieSyncManager.getInstance().sync(); // we want to save the cookies to disk any time there is a successful response

		return httpConnection;
	}

	static public HttpResponse readUrlToBytes( String url, String post, HttpHeader[] headerList ) {
		HttpURLConnection httpConnection = null;
        HttpResponse response = new HttpResponse();
        long startTime = System.currentTimeMillis();

        if (post == null) response.requestType = HttpResponse.RequestType.GET;
        else response.requestType = HttpResponse.RequestType.POST;
        response.mUrl = url;

		try {
			httpConnection = urlToConnection(url, post, headerList );
            response.mResponseCode = httpConnection.getResponseCode();

            ByteArrayOutputStream baos = new ByteArrayOutputStream();

		    byte[] buffer = new byte[1024];
		    int length = 0;
			while ((length = httpConnection.getInputStream().read(buffer)) != -1) {
				baos.write(buffer, 0, length);
			}

            response.setResponseBody(baos);

		} catch (IOException e) {
            Log.i(TAG, "readUrlToBytes:" + e.toString() );
            Log.i(TAG, "url:" + url );
            if( post != null )
            {
                Log.i(TAG, "post:" + post );
            }
			try {
				// If there wasn't an input stream, there's a good chance there's an error stream
				ByteArrayOutputStream baos = new ByteArrayOutputStream();
				byte[] buffer = new byte[1024];
				int length = 0;
				while ((length = httpConnection.getErrorStream().read(buffer)) != -1) {
					baos.write(buffer, 0, length);
				}
				response.setResponseBody(baos);
			} catch (Exception e1){
				Log.i(TAG, "readUrlToBytes also failed to get errorStream, " + e1.toString());
			}
		} finally {
			if (httpConnection != null) {
				httpConnection.disconnect();
			}
		}

        response.mResponseTime = System.currentTimeMillis() - startTime;
		return response;
	}

	static public String readUrlToString( String url, String post, HttpHeader[] headerList ) {
        return readUrlToBytes(url, post, headerList).responseBodyAsString();
	}

    static public HttpResponse readUrl( String url, String post, HttpHeader[] headerList) {
        return readUrlToBytes(url, post, headerList);
    }

    static public Bitmap readUrlToBitmap( String url ) {
        Bitmap b = Bitmap.createBitmap(200, 150, Bitmap.Config.ARGB_8888);
        try {
            b = BitmapFactory.decodeStream(urlToConnection(url, null, null).getInputStream());
        } catch (IOException e) {
            e.printStackTrace();
        }
        return b;
    }

    static public HttpHeader getRobloxHeader(String url) {
        HttpAgent.HttpHeader header = new HttpAgent.HttpHeader();
        header.header = "Cookie";

		Uri uri = Uri.parse(url);
        header.value = android.webkit.CookieManager.getInstance().getCookie(uri.getHost());
        return header;
    }
}



























