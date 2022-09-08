package com.roblox.client.http;

import android.util.Log;

import java.io.IOException;
import java.net.CookieManager;
import java.net.CookiePolicy;
import java.net.CookieStore;
import java.net.URI;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.roblox.client.AndroidAppSettings;
import com.roblox.client.RobloxSettings;
import com.roblox.client.managers.RbxReportingManager;
import com.roblox.client.managers.SessionManager;


//
// Taken from:
//
//	http://stackoverflow.com/questions/12731211/pass-cookies-from-httpurlconnection-java-net-cookiemanager-to-webview-android
//
//	Implementation of java.net.CookieManager which forwards all requests to the WebViews' webkit android.webkit.CookieManager.
//
public class WebkitCookieManagerProxy extends CookieManager 
{
    private android.webkit.CookieManager webkitCookieManager;
    private static final String TAG = "roblox.webkitCookieMgr";

    public WebkitCookieManagerProxy()
    {
        this(null, null);
    }

    WebkitCookieManagerProxy(CookieStore store, CookiePolicy cookiePolicy)
    {
        super(null, cookiePolicy);
        this.webkitCookieManager = android.webkit.CookieManager.getInstance();
    }

    @Override
    public void put(URI uri, Map<String, List<String>> responseHeaders) throws IOException
    {
        // make sure our args are valid
        if ((uri == null) || (responseHeaders == null)) return;

        // save our url once
        String domain = uri.getHost();
        if (!AndroidAppSettings.DisableCookieDomainTrimming()) {
            domain = domain.replace("m.", "");
            domain = domain.replace("www.", "");
            domain = domain.replace("api.", "");
            domain = domain.replace("web.", "");
        }


        // go over the headers
        for (String headerKey : responseHeaders.keySet()) 
        {
            // ignore headers which aren't cookie related
            if ((headerKey == null) || !(headerKey.equalsIgnoreCase("Set-Cookie2") || headerKey.equalsIgnoreCase("Set-Cookie"))) continue;

            // process each of the headers
            for (String headerValue : responseHeaders.get(headerKey))
            {
                if (headerValue.contains(".ROBLOSECURITY"))
                    getAndSetLatestCookieExpiration(headerValue);

                /**** TODO: Will remove after data gathered ****/
                // No IF, flags might not have been loaded yet
                if (headerValue.contains(".ROBLOSECURITY=;")) {
                    RbxReportingManager.fireAuthCookieFlush(System.currentTimeMillis(), uri.toString());
                }
                /**** TODO END ****/

                this.webkitCookieManager.setCookie(domain, headerValue);
                //Log.i(TAG, "Cookie For Domain: " + domain + " Cookie: " + headerValue);
            }
        }

        RobloxSettings.saveRobloxCookies(domain, android.webkit.CookieManager.getInstance().getCookie(domain));
    }

    @Override
    public Map<String, List<String>> get(URI uri, Map<String, List<String>> requestHeaders) throws IOException 
    {
        // make sure our args are valid
        if ((uri == null) || (requestHeaders == null)) throw new IllegalArgumentException("Argument is null");

        // save our url once
        String url = uri.toString();

        // prepare our response
        Map<String, List<String>> res = new java.util.HashMap<String, List<String>>();

        // get the cookie
        String cookie = this.webkitCookieManager.getCookie(url);
        //Log.i(TAG, "All Cookies For Domain: " + url + "Cookie: " + cookie);

        if (cookie != null) res.put("Cookie", Arrays.asList(cookie));
        return res;
    }

    @Override
    public CookieStore getCookieStore() 
    {
        // we don't want anyone to work with this cookie store directly
        throw new UnsupportedOperationException();
    }

    private static void getAndSetLatestCookieExpiration(String cookie) {
        if (cookie != null) {
            Log.i(TAG, "Cookie = " + cookie);

            Pattern pattern = Pattern.compile("([0-9]+-\\w+-[0-9][0-9][0-9][0-9]\\s[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\sGMT);");
            Matcher match = pattern.matcher(cookie);

            if (match.find()) {
                // Sample: 01-Apr-2016 03:59:32 GMT
                SimpleDateFormat f = new SimpleDateFormat("dd-MMM-yyyy HH:mm:ss z");
                try {
                    Date d = f.parse(match.group(1));
                    SessionManager.getInstance().setAuthCookieExpiration(d.getTime());
                } catch (ParseException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}