package com.roblox.client;

import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetRequest;

public class RbxAnalytics {
    private static final String TAG = "RbxAnalytics";

    public static void fireAppLaunch(String ctx) {
        if (!AndroidAppSettings.EnableRbxAnalytics()) return;

        String url = RobloxSettings.evtAppLaunchUrl(ctx);
        RbxHttpGetRequest req = new RbxHttpGetRequest(url, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
            }
        });
        req.execute();
    }

    public static void fireScreenLoaded(String ctx) {
        if (!AndroidAppSettings.EnableRbxAnalytics()) return;

        String url = RobloxSettings.evtScreenLoadedUrl(ctx);
        RbxHttpGetRequest req = new RbxHttpGetRequest(url, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
            }
        });
        req.execute();
    }

    public static void fireButtonClick(String ctx, String btn) {
        if (!AndroidAppSettings.EnableRbxAnalytics()) return;

        fireButtonClickCommon(RobloxSettings.evtButtonClickUrl(ctx, btn));
    }

    public static void fireButtonClick(String ctx, String btn, String custom) {
        if (!AndroidAppSettings.EnableRbxAnalytics()) return;

        fireButtonClickCommon(RobloxSettings.evtButtonClickUrl(ctx, btn, custom));
    }

    private static void fireButtonClickCommon(String url) {
        RbxHttpGetRequest req = new RbxHttpGetRequest(url, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
            }
        });
        req.execute();
    }

    public static void fireFormFieldValidation(String ctx, String input, boolean vis) {
        if (!AndroidAppSettings.EnableRbxAnalytics()) return;

        fireFormFieldCommon(RobloxSettings.evtFormFieldUrl(ctx, input, vis));
    }

    public static void fireFormFieldValidation(String ctx, String input, String msg, boolean vis) {
        if (!AndroidAppSettings.EnableRbxAnalytics()) return;

        fireFormFieldCommon(RobloxSettings.evtFormFieldUrl(ctx, input, vis, msg));
    }

    private static void fireFormFieldCommon(String url) {
        RbxHttpGetRequest req = new RbxHttpGetRequest(url, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
            }
        });
        req.execute();
    }

    public static void fireClientSideError(String ctx, String error) {
        fireClientSideErrorCommon(RobloxSettings.evtClientSideError(ctx, error));
    }

    public static void fireClientSideError(String ctx, String error, String custom) {
        fireClientSideErrorCommon(RobloxSettings.evtClientSideError(ctx, error, custom));
    }

    private static void fireClientSideErrorCommon(String url) {
        RbxHttpGetRequest req = new RbxHttpGetRequest(url, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
            }
        });
        req.execute();
    }
}
