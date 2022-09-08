package com.roblox.client.http;

import android.content.SharedPreferences;
import android.util.Log;
import android.webkit.CookieManager;

import com.roblox.client.AndroidAppSettings;
import com.roblox.client.RobloxSettings;
import com.roblox.client.Utils;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class CookieConsistencyChecker {
    private static final String BTID_KEY = "RbxBTID";
    private static final String GA_CATEGORY = "CookieConsistency";
    private static final String ACTION_OLDNOTFOUND = "KeyNotFound";
    private static final String ACTION_CURRENTNOTFOUND = "CookieNotFound";
    private static final String ACTION_FIRSTSTAGEOK = "FirstStageOk";
    private static final String ACTION_FIRSTSTAGEFAIL = "FirstStageFailure";
    private static final String ACTION_SECONDSTAGEOK = "SecondStageOk";
    private static final String ACTION_SECONDSTAGEFAIL = "SecondStageFailure";

    private static String FIRST_STAGE_RESULT = "";

    public static void firstStageCheck() {
        if (!AndroidAppSettings.EnableCookieConsistencyChecks()) return;

        SharedPreferences keyValues = RobloxSettings.getKeyValues();
        String oldBTID = keyValues.getString(BTID_KEY, "");

        if (oldBTID.isEmpty()) {
            Utils.sendAnalytics(GA_CATEGORY, ACTION_OLDNOTFOUND);
            return;
        } else {
            String currBtid = getCurrentBtid();
            if (currBtid == null) {
                Utils.sendAnalytics(GA_CATEGORY, ACTION_CURRENTNOTFOUND);
            } else {
                if (oldBTID.equals(getCurrentBtid()))
                    Utils.sendAnalytics(GA_CATEGORY, ACTION_FIRSTSTAGEOK);
                else {
                    Utils.sendAnalytics(GA_CATEGORY, ACTION_FIRSTSTAGEFAIL);
                    writeBtidToDisk();
                }
            }
        }
    }

    private static void writeBtidToDisk() {
        SharedPreferences keyValues = RobloxSettings.getKeyValues();
        SharedPreferences.Editor edit = keyValues.edit();
        edit.putString(BTID_KEY, getCurrentBtid());
        edit.commit(); // commit to ensure this gets written before the deviceId call comes back
    }

    public static void secondStageCheck() {
        if (!AndroidAppSettings.EnableCookieConsistencyChecks()) return;

        SharedPreferences keyValues = RobloxSettings.getKeyValues();
        String oldBtid = keyValues.getString(BTID_KEY, "");
        String currBtid = getCurrentBtid();

        if ((FIRST_STAGE_RESULT.equals(ACTION_OLDNOTFOUND) || FIRST_STAGE_RESULT.equals(ACTION_CURRENTNOTFOUND)) && !currBtid.isEmpty())
            Utils.sendAnalytics(GA_CATEGORY, ACTION_SECONDSTAGEOK);

        if (oldBtid.equals(currBtid)) Utils.sendAnalytics(GA_CATEGORY, ACTION_SECONDSTAGEOK);
        else Utils.sendAnalytics(GA_CATEGORY, ACTION_SECONDSTAGEFAIL);

        if (!currBtid.isEmpty()) writeBtidToDisk();
    }

    public static String getCurrentBtid() {
        String cookie = android.webkit.CookieManager.getInstance().getCookie(Utils.stripSubDomain(RobloxSettings.baseUrl()));
        if (cookie != null) {
            Log.i(GA_CATEGORY, "Cookie = " + cookie);

            Pattern pattern = Pattern.compile("&browserid=(\\d*);");
            Matcher match = pattern.matcher(cookie);

            if (match.find()) return match.group(1);
            else return "";
        }
        else return "";
    }
}
