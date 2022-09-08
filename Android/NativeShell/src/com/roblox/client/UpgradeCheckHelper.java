package com.roblox.client;

import org.json.JSONObject;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;

import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetRequest;

public class UpgradeCheckHelper {
	
	private static final String TAG = "UpgradeCheckHelper";
	
	enum UpgradeStatus
	{
		UnKnown,
		Recommended,
		Required,
		NotRequired
	}
	
	static private UpgradeStatus upgradeStatus;
	static private RobloxActivity mActivity = null;
    static private AlertDialog alert = null;
	
	public static void checkForUpdate(final RobloxActivity activity)
	{
        alert = null;
		upgradeStatus = UpgradeStatus.UnKnown;
        RbxHttpGetRequest upgradeCheck = new RbxHttpGetRequest(RobloxSettings.upgradeCheckUrl(), new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                evaluateResponse(response, activity);
            }
        });
        upgradeCheck.execute();
		//(new ForceUpgradeCheck(activity)).execute();
	}

    private static void evaluateResponse(HttpResponse response, RobloxActivity activity) {
        try
        {
            JSONObject mJson = new JSONObject(response.responseBodyAsString());
            JSONObject dataJsonObj = mJson.getJSONObject("data");
            if(dataJsonObj == null)
                return;

            String status = dataJsonObj.getString("UpgradeAction");
            if( status.equals("Required") )
            {
                upgradeStatus = UpgradeStatus.Required;
            }
            else if( status.equals("Recommended") )
            {
                upgradeStatus = UpgradeStatus.Recommended;
            }
            else
                upgradeStatus = UpgradeStatus.NotRequired;

            if (activity != null)
                UpgradeCheckHelper.showUpdateDialogIfRequired(activity);
        }
        catch (Exception e)
        {
            // Do nothing we already check the UpgradeStatus to Unknown on Startup
            // Let them continue to Play
        }
    }

    public static UpgradeStatus showUpdateDialogIfRequired(RobloxActivity activity) {
        if (upgradeStatus == UpgradeStatus.Required && alert == null) {
            mActivity = activity;
            alert = new AlertDialog.Builder(activity)
                    .setTitle("ROBLOX Upgrade")
                    .setMessage(R.string.UpgradeBodyString)
                    .setPositiveButton("Upgrade", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            final String appPackageName = mActivity.getPackageName();
                            try {
                                mActivity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + appPackageName)));
                            } catch (android.content.ActivityNotFoundException anfe) {
                                mActivity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("http://play.google.com/store/apps/details?id=" + appPackageName)));
                            }
                        }
                    })
                    .setCancelable(false)
                    .show();
        } else if (upgradeStatus == UpgradeStatus.Recommended && alert == null) {
            mActivity = activity;
            alert = new AlertDialog.Builder(activity)
                    .setTitle("ROBLOX Upgrade")
                    .setMessage(R.string.UpgradeBodyString)
                    .setPositiveButton("Upgrade", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            final String appPackageName = mActivity.getPackageName();
                            try {
                                mActivity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + appPackageName)));
                            } catch (android.content.ActivityNotFoundException anfe) {
                                mActivity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("http://play.google.com/store/apps/details?id=" + appPackageName)));
                            }
                        }
                    })
                    .setNegativeButton("Not Now", new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                        }
                    })
                    .show();
        } else if (alert != null && !alert.isShowing())
            alert.show();

        return upgradeStatus;
    }
}
