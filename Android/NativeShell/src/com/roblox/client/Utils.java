package com.roblox.client;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.FragmentTransaction;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Point;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.URLSpan;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.WindowManager;
import android.view.WindowManager.BadTokenException;
import android.view.inputmethod.InputMethodManager;
import android.webkit.WebView;
import android.widget.EditText;
import android.widget.TextView;

import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.HitBuilders.EventBuilder;
import com.google.android.gms.analytics.Tracker;
import com.roblox.client.managers.SessionManager;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.io.Reader;
import java.io.StringWriter;
import java.io.Writer;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Scanner;

public class Utils {
    private static final String TAG = "roblox.utils";

    public static final String CRASH_GUARD = "crash_guard";
    public static final String CRASH_GUARD_OK = "crash_guard_ok";
    public static final String CRASH_GUARD_CHANNEL = "MTBF_CRASH_GUARD_3";    // 3_ is a version number.

    private static AlertDialog mExclusiveDialog = null;
    private static int mExclusiveDialogContext = 0;
    private static RateMeMaybe mRateMeMaybe = null;

    public static DisplayMetrics mDeviceDisplayMetrics;

    // /proc/meminfo may be unavailable.
    public static int getDeviceTotalMegabytes() {
        RandomAccessFile reader = null;
        String s = null;
        try {
            reader = new RandomAccessFile("/proc/meminfo", "r");
            s = reader.readLine();
        } catch (IOException ex) {
        } finally {
            try {
                reader.close();
            } catch (IOException e) {
            }
        }

        if (s == null) {
            return 0;
        }

        String[] tokens = s.split("\\s+");
        if (tokens.length < 2) {
            return 0;
        }

        long mb = 0;
        try {
            long kb = Long.parseLong(tokens[1]);
            mb = kb / (1024);
        } catch (NumberFormatException ex) {
        }
        return (int) mb;
    }

    public static boolean getDeviceHasNEON() {
        List<String> lines = readTextFile("/proc/cpuinfo");
        for (String line : lines) {
            if (line.contains("neon")) {
                return true;
            }
        }
        new InfluxBuilderV2("NeonNotFound").addField("cpuinfo", lines.toString()).fireReport();
        return false;
    }

    public static Point getScreenSize(Context c) {
        WindowManager wm = (WindowManager) c.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        return size;
    }

    public static Point getScreenDpi(Context c) {
        DisplayMetrics metrics = c.getResources().getDisplayMetrics();
        Point dpi = new Point();
        dpi.x = (int) metrics.xdpi;
        dpi.y = (int) metrics.ydpi;
        return dpi;
    }

    public static Point getScreenDpSize(Context c) {
        DisplayMetrics metrics = c.getResources().getDisplayMetrics();
        Point dp = new Point();
        dp.x = (int) ((float) metrics.widthPixels / metrics.density);
        dp.y = (int) ((float) metrics.heightPixels / metrics.density);
        return dp;
    }

    public static String getDeviceName() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;

        String name;
        if (model.startsWith(manufacturer)) {
            name = model;
        } else {
            name = manufacturer + " " + model;
        }

        char capital = Character.toUpperCase(name.charAt(0));
        return capital + name.substring(1, name.length());
    }

    public static String getWebkitVersion(Context c) {
        // Yes this happened.
        String userAgent = new WebView(c).getSettings().getUserAgentString();

        String[] tokens = userAgent.split("\\s+");
        for (int i = 0; i < tokens.length; ++i) {
            if (tokens[i].startsWith("AppleWebKit")) {
                return tokens[i];
            }
        }
        return "AppleWebKit/Unknown";
    }

    public static boolean isDevicePhone(Context c)
    {
        if (new WebView(c).getSettings().getUserAgentString().contains("Mobile"))
            return true;
        else
            return false;
    }

    public static void createDirectory(File path) {
        path.mkdirs();
    }

    public static void writeToFile(File path, String data) {
        BufferedWriter writer = null;
        try {
            writer = new BufferedWriter(new FileWriter(path));
            writer.write(data);
        } catch (final IOException e) {
            Log.e("Exception", "File write failed: " + e.toString());
        } finally {
            if (null != writer) {
                try {
                    writer.close();
                } catch (final IOException e) {
                }
            }
        }
    }

    static List<String> readTextFile(String path) {
        List<String> strings = new ArrayList<String>();
        File file = new File(path);
        Scanner scanner;
        try {
            scanner = new Scanner(file);
        } catch (FileNotFoundException e) {
            return strings;
        }

        try {
            while (scanner.hasNextLine()) {
                strings.add(scanner.nextLine());
            }
        } finally {
            scanner.close();
        }
        return strings;
    }

    static boolean checkForRawResource(Context context, String name) {
        int id = context.getResources().getIdentifier(name, "raw", context.getPackageName());
        return id != 0;
    }

    static JSONObject loadJson(Context context, int res) throws IOException {
        InputStream is = context.getResources().openRawResource(res);
        Writer writer = new StringWriter();
        char[] buffer = new char[1024];
        JSONObject json = null;
        try {
            Reader reader = new BufferedReader(new InputStreamReader(is, "UTF-8"));
            int n;
            while ((n = reader.read(buffer)) != -1) {
                writer.write(buffer, 0, n);
            }
            String jsonString = writer.toString();
            json = new JSONObject(jsonString);
        } catch (IOException e) {
            Log.e(TAG, "IOException loading JSON.");
            throw new IOException("IOException loading JSON resource # " + res);
        } catch (JSONException e) {
            Log.e(TAG, "Cannot parse JSON.");
            throw new IOException("Cannot parse JSON resource # " + res);
        } finally {
            try {
                is.close();
            } catch (IOException e) {
            }
        }

        return json;
    }

    public static void killBackgroundProcesses(Context c) {
        // Kill the GlView because it can be done cleanly from here.  Kills the shell as a side effect.
        String packageName = c.getPackageName();
        ActivityManager activityManager = (ActivityManager) c.getSystemService(Context.ACTIVITY_SERVICE);
        activityManager.killBackgroundProcesses(packageName);
    }

    // Specify Locale.ROOT so that URLs do not change with locale.
    public static String format(String format, Object... args) {
        return String.format(Locale.ROOT, format, args);
    }

    public static boolean isNetworkConnected() {
        Context context = RobloxApplication.getInstance();

        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        return cm.getActiveNetworkInfo() != null && cm.getActiveNetworkInfo().isConnected();
    }

    private static void setExclusiveDialog(AlertDialog alertDialog) {
        Context context = RobloxApplication.getInstance().getCurrentActivity();
        if (context != null) {
            mExclusiveDialog = alertDialog;
            mExclusiveDialogContext = System.identityHashCode(context);
            mExclusiveDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface arg0) {
                    mExclusiveDialog = null;
                    mExclusiveDialogContext = 0;
                    }
            });
        }
    }

    // Localization requires using R.string.*
    // Use "exclusive" variety as these could leak across an activity change.
    public static AlertDialog alert(int stringResource) {
        Context context = RobloxApplication.getInstance().getCurrentActivity();
        if (context != null) {
            String s = context.getResources().getString(stringResource);
            return alert(s);
        }
        else return alert("Critical error! For safety, please restart this app.");
    }

    // Use "exclusive" variety as these could leak across an activity change.
    protected static AlertDialog alert(String s) {
        Context context = RobloxApplication.getInstance().getCurrentActivity();

        TextView t = new TextView(context);
        t.setText(s);
        t.setTextAppearance(context, android.R.style.TextAppearance_DeviceDefault_Large);
        t.setTextSize(20.0f);
        t.setEllipsize(null);

        AlertDialog alertDialog = new AlertDialog.Builder(context).create();
        alertDialog.setView(t, 150, 100, 150, 100);
        alertDialog.setCanceledOnTouchOutside(true);
        try {
            alertDialog.show();
        } catch (BadTokenException e) {
            // This was a rare crash in Google Analytics.  Best guess, an alert is being triggered
            // with the RobloxApplication as a context which would be illegal here...  But I don't
            // know which one.
        }
        return alertDialog;
    }

    // Use "exclusive" variety as these could leak across an activity change.
    protected static AlertDialog alertFormatted(int stringResource, Object... args) {
        Context context = RobloxApplication.getInstance().getCurrentActivity();
        String format = context.getResources().getString(stringResource);
        String s = String.format(Locale.ROOT, format, args);
        return alert(s);
    }

    public synchronized static AlertDialog alertExclusively(int stringResource) {
        if (mExclusiveDialog == null) {
            setExclusiveDialog(alert(stringResource));
            return mExclusiveDialog;
        }
        return null;
    }

    public synchronized static AlertDialog alertExclusively(String s) {
        if (mExclusiveDialog == null) {
            setExclusiveDialog(alert(s));
            return mExclusiveDialog;
        }
        return null;
    }

    public synchronized static AlertDialog alertExclusivelyFormatted(int stringResource, Object... args) {
        if (mExclusiveDialog == null) {
            setExclusiveDialog(alertFormatted(stringResource, args));
            return mExclusiveDialog;
        }
        return null;
    }

    public synchronized static AlertDialog alertUnexpectedError(String err) {
        sendAnalytics("UnexpectedError", err);
        return alertFormatted(R.string.UnexpectedError, err);
    }

    public synchronized static void sendUnexpectedError(String err) {
        sendAnalytics("UnexpectedErrorSilent", err);
    }

    // returns true if not connected.
    public static boolean alertIfNetworkNotConnected() {
        boolean isNotConnected = !isNetworkConnected();
        if (isNotConnected) {
            alertExclusively(R.string.ConnectionError);
        }
        return isNotConnected;
    }

    public static void sendAnalytics(String category, String action) {
        if (AndroidAppSettings.EnableGoogleAnalyticsChange()) {
            sendAnalytics(category, action, null, null);
        } else {
            RobloxApplication app = RobloxApplication.getInstance();
            if (app.isGooglePlayServicesAvailable()) {
                Tracker t = app.getAndroidTracker();

                EventBuilder eb = new HitBuilders.EventBuilder().setCategory(category).setAction(action);
                t.send(eb.build());

                Log.i(TAG, format("sendAnalytics: %s %s", category, action));
            }
        }
    }

    public static void sendAnalytics(String category, String action, String label) {
        sendAnalytics(category, action, label, null);
    }

    public static void sendAnalytics(String category, String action, Long value) {
        sendAnalytics(category, action, null, value);
    }

    public static void sendAnalytics(String category, String action, String label, Long value) {
        // Category & action are required
        if ((category == null || category.isEmpty()) && (action == null || action.isEmpty())) return;

        RobloxApplication app = RobloxApplication.getInstance();
        if (app.isGooglePlayServicesAvailable()) {
            Tracker t = app.getAndroidTracker();

            EventBuilder eb = new HitBuilders.EventBuilder().setCategory(category).setAction(action);
            if (label != null && !label.isEmpty())
                eb.setLabel(label);
            else
                label = "NO_LABEL";
            if (value != null)
                eb.setValue(value);
            else
                value = 0l;

            t.send(eb.build());


            Log.i(TAG, format("sendAnalytics: %s %s %s %s", category, action, label, value.toString()));
        }
    }

    public static void sendTiming(String category, String action, long milliseconds) {
        RobloxApplication app = RobloxApplication.getInstance();
        if (app.isGooglePlayServicesAvailable()) {
            Tracker t = app.getAndroidTracker();

            EventBuilder eb = new HitBuilders.EventBuilder().setCategory(category).setAction(action);
            eb.setValue(milliseconds / 1000); // seconds
            t.send(eb.build());

            Log.i(TAG, format("sendTiming: %s %s %d", category, action, milliseconds / 1000));
        }
    }

    public static void sendAnalyticsScreen(String category) {
        RobloxApplication app = RobloxApplication.getInstance();
        if (app.isGooglePlayServicesAvailable()) {
            Tracker t = app.getAndroidTracker();
            t.setScreenName(category);
            t.send(new HitBuilders.EventBuilder().setCategory(category).setAction("ScreenView").build());
        }
    }

    public static void onRateMeMaybe(Activity a) {
        if (mRateMeMaybe == null) {
            RateMeMaybe.resetData(a);
            mRateMeMaybe = new RateMeMaybe(a);
        }

        // 		minLaunchesUntilInitialPrompt
        // 		minDaysUntilInitialPrompt
        // 		minLaunchesUntilNextPrompt
        // 		minDaysUntilNextPrompt
        mRateMeMaybe.setPromptMinimums(10, 1, 10, 2);

        mRateMeMaybe.setHandleCancelAsNeutral(false); // Assume insisting on a rating gets worse ratings.

        // mRateMeMaybe.setRunWithoutPlayStore(true);
        // rmm.setAdditionalListener(this); // extends SherlockFragmentActivity implements OnRMMUserChoiceListener

        String rateMeDialogTitle = a.getResources().getString(R.string.RateMeDialogTitle);
        String rateMeDialogMessage = a.getResources().getString(R.string.RateMeDialogMessage);
        String rateMeRateIt = a.getResources().getString(R.string.RateMeRateIt);
        String rateMeNotNow = a.getResources().getString(R.string.RateMeNotNow);
        String rateMeNever = a.getResources().getString(R.string.RateMeNever);

        mRateMeMaybe.setDialogTitle(rateMeDialogTitle);
        mRateMeMaybe.setDialogMessage(rateMeDialogMessage);
        mRateMeMaybe.setPositiveBtn(rateMeRateIt);
        mRateMeMaybe.setNeutralBtn(rateMeNotNow);
        mRateMeMaybe.setNegativeBtn(rateMeNever);

        mRateMeMaybe.run();
    }

    public static void chmod(String path, int mode) {
        try {
            File f = new File(path);
            @SuppressWarnings("rawtypes")
            Class fileUtils = Class.forName("android.os.FileUtils");
            @SuppressWarnings("unchecked")
            Method setPermissions = fileUtils.getMethod("setPermissions", String.class, int.class, int.class, int.class);
            setPermissions.invoke(null, f.getAbsolutePath(), mode, -1, -1);
        } catch (Exception e) {
            Log.e(TAG, "chmod: " + e.getLocalizedMessage());
        }
    }

    public static void makeTextViewHtml(Activity a, TextView tv, String s) {
        Spanned html = Html.fromHtml(s);
        tv.setText(html);
        tv.setMovementMethod(LinkMovementMethod.getInstance());

        CharSequence text = tv.getText();
        if (text instanceof Spannable) {
            Spannable sp = (Spannable) text;
            URLSpan[] urls = sp.getSpans(0, text.length(), URLSpan.class);
            SpannableStringBuilder style = new SpannableStringBuilder(text);
            style.clearSpans();//should clear old spans
            final Activity activity = a;
            for (URLSpan url : urls) {
                final URLSpan urlFinal = url;
                ClickableSpan click = new ClickableSpan() {
                    @Override
                    public void onClick(View widget) {
                        try {
                            Intent i = new Intent("android.intent.action.MAIN");
                            i.setComponent(ComponentName.unflattenFromString("com.android.chrome/com.android.chrome.Main"));
                            i.addCategory("android.intent.category.LAUNCHER");
                            i.setData(Uri.parse(urlFinal.getURL()));
                            activity.startActivity(i);
                        }
                        catch(ActivityNotFoundException e) { // In case Chrome isn't installed
                            Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(urlFinal.getURL()));
                            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            activity.startActivity(i);
                        }
                    }
                };
                style.setSpan(click, sp.getSpanStart(url), sp.getSpanEnd(url), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
            tv.setText(style);
        }
    }

    public static void doCrashGuardCheck(boolean alertIfCrashed) {
        Context context = RobloxApplication.getInstance();

        boolean wasOk = false;
        try {
            FileInputStream fis = context.openFileInput(CRASH_GUARD_OK);
            fis.close();
            wasOk = true;
        } catch (Exception e) {
        }

        try {
            FileInputStream fis = context.openFileInput(CRASH_GUARD);
            byte[] buffer = new byte[256];
            int nRead = fis.read(buffer);
            char[] chars = new char[nRead];
            for (int i = 0; i < nRead; ++i) {
                chars[i] = (char) buffer[i];
            }
            String str = String.copyValueOf(chars);

            if (!wasOk) {
                Log.e(TAG, "*** Found Crash Guard: " + str);
            }

            long milliseconds = Long.parseLong(str);
            long bucket = milliseconds / 1000;
            if (bucket > 0) {
                bucket = (long) Math.log((double) bucket);
                bucket = (long) Math.pow(2, (double) bucket);
            }

            if (wasOk) {
                sendTiming(CRASH_GUARD_CHANNEL, "TimeTeardownOk", milliseconds);
                sendTiming(CRASH_GUARD_CHANNEL, "TimeTeardownOk_" + bucket, milliseconds);
            } else {
                sendTiming(CRASH_GUARD_CHANNEL, "CrashFound", milliseconds);
                sendTiming(CRASH_GUARD_CHANNEL, "CrashFound_" + bucket, milliseconds);
            }
            sendTiming(CRASH_GUARD_CHANNEL, "Time", milliseconds);

            fis.close();
        } catch (FileNotFoundException e) {
            // This is desired.
        } catch (Exception e) {
            sendAnalytics(CRASH_GUARD_CHANNEL, "CrashFoundTimeReadFailure");
        }

        context.deleteFile(CRASH_GUARD);
        context.deleteFile(CRASH_GUARD_OK);
    }

    public static String formatRobuxBalance(int bal)
    {
        String balString = "";
        char mod = 'a';
        if (bal < 1000)
            return "" + bal;
        else if ( bal < 1000000) {
            mod = 'K';
            int rem = bal % 1000;
            bal = bal - rem;
            balString += bal;
            balString = balString.replace("000", "");
        }
        else {
            mod = 'M';
            int rem = bal % 1000000;
            bal = bal - rem;
            balString += bal;
            balString = balString.replace("000000", "");
        }

        return "" + balString + mod + (mod == 'a' ? "" : "+");
    }

    public static DisplayMetrics getDeviceDisplayMetrics()
    {
        return (mDeviceDisplayMetrics == null ?
                mDeviceDisplayMetrics = SessionManager.mCurrentActivity.getResources().getDisplayMetrics()
                : mDeviceDisplayMetrics);
    }

    public static int getCurrentActivityId(Activity a) { return a.findViewById(android.R.id.content).getId(); }

    public static float dpToPixel(int pixel) {
        return pixel * getDeviceDisplayMetrics().density;
    }

    public static float dpToPixel(float pixel) {
        return pixel * getDeviceDisplayMetrics().density;
    }

    public static float pixelToDp(float dp) {
        return dp / getDeviceDisplayMetrics().density;
    }

    public static float pixelToDp(int dp) {
        return dp / getDeviceDisplayMetrics().density;
    }

    public static void showKeyboard(final View viewRef, final EditText editRef) {
        if (viewRef != null && editRef != null) {
            Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
            mUIThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    final InputMethodManager imm = (InputMethodManager) viewRef.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.showSoftInput(editRef, InputMethodManager.SHOW_IMPLICIT);
                }
            });
        }
        else if (viewRef != null && editRef == null)
            showKeyboard(viewRef);
    }

    public static void showKeyboard(final View viewRef) {
        if (viewRef != null) {
            Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
            mUIThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    final InputMethodManager imm = (InputMethodManager) viewRef.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.showSoftInput(viewRef, InputMethodManager.SHOW_IMPLICIT);
                }
            });
        }
    }

    public static void hideKeyboard(final View viewRef, final EditText editRef) {
        if (viewRef != null && editRef != null) {
            Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
            mUIThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    InputMethodManager imm = (InputMethodManager) viewRef.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(viewRef.getWindowToken(), 0);
                }
            });
        }
        else if (viewRef != null && editRef == null)
            hideKeyboard(viewRef);
    }

    public static void hideKeyboard(final View viewRef) {
        if (viewRef != null) {
            Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
            mUIThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    final InputMethodManager imm = (InputMethodManager) viewRef.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(viewRef.getWindowToken(), 0);
                }
            });
        }
    }

    public static String sanitizeEmailAddress(String email) {
        if (email == null)
            return "";

        int pos = email.indexOf("@");
        StringBuilder newEm = new StringBuilder();
        if (pos != -1) {
            String sub = email.substring(0, pos);
            for (int i = 0; i < sub.length(); i++) {
                newEm.append("*");
            }
            newEm.append(email.substring(pos, email.length()));
        }

        return newEm.toString();
    }

    // -----------------------------------------------------------------------
    public static void DoProtocolRegistrationCheck(Intent incomingIntent, ProtocolLaunchCheckHandler handler)
    {
        Log.i(TAG, "in DoProtocolRegistrationCheck");
        if (handler == null) {
            Log.e(TAG, "Can't run ProtocolRegistrationCheck without a ProtocolCheckHandler!");
            return;
        }
        if( incomingIntent == null )
        {
            Log.e(TAG, "Launching Web View Activity without Intent.");
            return;
        }

        if( incomingIntent.getAction() == Intent.ACTION_VIEW )
        {
            RbxAnalytics.fireAppLaunch("protocolLaunch");
            String str = incomingIntent.getDataString();
            String placeID = str.replace("robloxmobile://", "").replace("placeID=", "");
            Log.i(TAG, "Launching from Protocol, Place ID: " + placeID);

            if(str.matches("robloxmobile://\\??(placeID=)?(\\d+)")) {
                handler.onParseFinished(placeID);
            }
        } else {
            handler.onParseFinished("");
        }
    }

    public static String stripSubDomain(String domain) {
        domain = domain.replace("m.", "");
        domain = domain.replace("www.", "");
        domain = domain.replace("api.", "");
        domain = domain.replace("web.", "");

        return domain;
    }

    public static int[] getNumberAndLengthOfAuthCookies() {
        int[] output = new int[2];
        // Grab count of auth cookies
        String cookie = android.webkit.CookieManager.getInstance().getCookie(RobloxSettings.baseUrl());
        if (cookie != null) {
            int lastIndex = 0;
            String cookieName = ".ROBLOSECURITY";
            while (lastIndex != -1) {

                lastIndex = cookie.indexOf(cookieName, lastIndex);

                if (lastIndex != -1) {
                    output[0]++;
                    lastIndex += cookieName.length();
                }
            }
            //Log.i(TAG, "# occurences: " + output[0]);
            // Grab length of first auth cookie
            if (output[0] > 0) {
                int start = cookie.indexOf(cookieName, 0);
                int length = cookie.indexOf(";", start + cookieName.length());
                if (length != -1) {
                    output[1] = length;
                } else { // this means the auth cookie is the last cookie listed and has no trailing semi-colon
                    output[1] = cookie.length() - start;
                }

            }
            //Log.i(TAG, "length of cookie: " + output[1]);
        }
        return output;
    }
}
