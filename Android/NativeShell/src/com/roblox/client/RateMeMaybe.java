package com.roblox.client;

// License: MIT
// https://github.com/Kopfgeldjaeger/RateMeMaybe/blob/master/code/src/com/kopfgeldjaeger/ratememaybe
//
// Has ROBLOX modifications.


import android.app.Activity;
import android.app.FragmentManager;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.text.format.DateUtils;
import android.util.Log;
import android.widget.Toast;

import com.roblox.client.RateMeMaybeFragment.RMMFragInterface;

public class RateMeMaybe implements RMMFragInterface {
	private static final String TAG = "RateMeMaybe";

	private Activity mActivity;
	private SharedPreferences mPreferences;

	private String mDialogTitle;
	private String mDialogMessage;
	private String mPositiveBtn;
	private String mNeutralBtn;
	private String mNegativeBtn;
	private int mIcon;

	private int mMinLaunchesUntilInitialPrompt = 0;
	private int mMinDaysUntilInitialPrompt = 0;

	private int mMinLaunchesUntilNextPrompt = 0;
	private int mMinDaysUntilNextPrompt = 0;

	private Boolean mHandleCancelAsNeutral = true;

	private Boolean mRunWithoutPlayStore = false;

	public interface OnRMMUserChoiceListener {
		void handlePositive();

		void handleNeutral();

		void handleNegative();
	}

	private OnRMMUserChoiceListener mListener;

	public RateMeMaybe(Activity activity) {
		mActivity = activity;
		mPreferences = mActivity.getSharedPreferences(PREF.NAME, 0);
	}

	/**
	 * Sets the title of the dialog shown to the user
	 * 
	 * @param dialogTitle
	 */
	public void setDialogTitle(String dialogTitle) {
		mDialogTitle = dialogTitle;
	}

	public String getDialogTitle() {
		if (mDialogTitle == null) {
			return "Rate " + getApplicationName();
		} else {
			return mDialogTitle;
		}
	}

	/**
	 * Sets the message shown to the user. %totalLaunchCount% will be replaced
	 * with total launch count.
	 * 
	 * @param dialogMessage
	 *            The message shown
	 */
	public void setDialogMessage(String dialogMessage) {
		mDialogMessage = dialogMessage;
	}

	public String getDialogMessage() {
		if (mDialogMessage == null) {
			return "If you like using "
					+ this.getApplicationName()
					+ ", it would be great"
					+ " if you took a moment to rate it in the Play Store. Thank you!";
		} else {
			return mDialogMessage.replace("%totalLaunchCount%", String
					.valueOf(mPreferences.getInt(PREF.TOTAL_LAUNCH_COUNT, 0)));
		}
	}

	public String getPositiveBtn() {
		if (mPositiveBtn == null) {
			return "Rate it";
		} else {
			return mPositiveBtn;
		}
	}

	/**
	 * Sets name of button that opens Play Store entry
	 * 
	 * @param positiveBtn
	 */
	public void setPositiveBtn(String positiveBtn) {
		mPositiveBtn = positiveBtn;
	}

	public String getNeutralBtn() {
		if (mNeutralBtn == null) {
			return "Not now";
		} else {
			return mNeutralBtn;
		}
	}

	/**
	 * Sets name of neutral button
	 * 
	 * @param neutralBtn
	 */
	public void setNeutralBtn(String neutralBtn) {
		mNeutralBtn = neutralBtn;
	}

	public String getNegativeBtn() {
		if (mNegativeBtn == null) {
			return "Never";
		} else {
			return mNegativeBtn;
		}
	}

	/**
	 * Sets name of button that makes the prompt never show again
	 * 
	 * @param negativeBtn
	 */
	public void setNegativeBtn(String negativeBtn) {
		mNegativeBtn = negativeBtn;
	}

	/**
	 * @param customIcon
	 *            Drawable id of custom icon
	 */
	public void setIcon(int customIcon) {
		mIcon = customIcon;
	}

	public int getIcon() {
		return mIcon;
	}

	/**
	 * Sets requirements for when to prompt the user.
	 * 
	 * @param minLaunchesUntilInitialPrompt
	 *            Minimum of launches before the user is prompted for the first
	 *            time. One call of .run() counts as launch.
	 * @param minDaysUntilInitialPrompt
	 *            Minimum of days before the user is prompted for the first
	 *            time.
	 * @param minLaunchesUntilNextPrompt
	 *            Minimum of launches before the user is prompted for each next
	 *            time. One call of .run() counts as launch.
	 * @param minDaysUntilNextPrompt
	 *            Minimum of days before the user is prompted for each next
	 *            time.
	 */
	public void setPromptMinimums(int minLaunchesUntilInitialPrompt,
			int minDaysUntilInitialPrompt, int minLaunchesUntilNextPrompt,
			int minDaysUntilNextPrompt) {
		this.mMinLaunchesUntilInitialPrompt = minLaunchesUntilInitialPrompt;
		this.mMinDaysUntilInitialPrompt = minDaysUntilInitialPrompt;
		this.mMinLaunchesUntilNextPrompt = minLaunchesUntilNextPrompt;
		this.mMinDaysUntilNextPrompt = minDaysUntilNextPrompt;
	}

	/**
	 * @param handleCancelAsNeutral
	 *            Standard is true. If set to false, a back press (or other
	 *            things that lead to the dialog being cancelled), will be
	 *            handled like a negative choice (click on "Never").
	 */
	public void setHandleCancelAsNeutral(Boolean handleCancelAsNeutral) {
		this.mHandleCancelAsNeutral = handleCancelAsNeutral;
	}

	/**
	 * Sets an additional callback for when the user has made a choice.
	 * 
	 * @param listener
	 */
	public void setAdditionalListener(OnRMMUserChoiceListener listener) {
		mListener = listener;
	}

	/**
	 * Standard is false. Whether the run method is executed even if no Play
	 * Store is installed on device.
	 * 
	 * @param runWithoutPlayStore
	 */
	public void setRunWithoutPlayStore(Boolean runWithoutPlayStore) {
		mRunWithoutPlayStore = runWithoutPlayStore;
	}

	/**
	 * Reset the launch logs
	 */
	public static void resetData(Activity activity) {
		activity.getSharedPreferences(PREF.NAME, 0).edit().clear().commit();
		Log.d(TAG, "Cleared RateMeMaybe shared preferences.");
	}

	/**
	 * Actually show the dialog (if it is not currently shown)
	 */
	private void showDialog() {
		FragmentManager fm = mActivity.getFragmentManager();
		
		if (fm.findFragmentByTag(
				"rmmFragment") != null) {
			// the dialog is already shown to the user
			return;
		}
		RateMeMaybeFragment frag = new RateMeMaybeFragment();
		frag.setData(getIcon(), getDialogTitle(), getDialogMessage(),
				getPositiveBtn(), getNeutralBtn(), getNegativeBtn(), this);
		try {
			frag.show(fm, "rmmFragment");
		}
		catch( IllegalStateException e )
		{
			// Very rare crash showing up in Analytics.
		}

	}

	/**
	 * Forces the dialog to show, even if the requirements are not yet met. Does
	 * not affect prompt logs. Use with care.
	 */
	public void forceShow() {
		showDialog();
	}

	/**
	 * Normal way to update the launch logs and show the user prompt if the
	 * requirements are met.
	 */
	public void run() {
		if (mPreferences.getBoolean(PREF.DONT_SHOW_AGAIN, false)) {
			return;
		}

		if (!isPlayStoreInstalled()) {
			Log.d(TAG, "No Play Store installed on device.");
			if (!mRunWithoutPlayStore) {
				return;
			}
		}

		Editor editor = mPreferences.edit();

		int totalLaunchCount = mPreferences.getInt(PREF.TOTAL_LAUNCH_COUNT, 0) + 1;
		editor.putInt(PREF.TOTAL_LAUNCH_COUNT, totalLaunchCount);

		long currentMillis = System.currentTimeMillis();

		long timeOfAbsoluteFirstLaunch = mPreferences.getLong(
				PREF.TIME_OF_ABSOLUTE_FIRST_LAUNCH, 0);
		if (timeOfAbsoluteFirstLaunch == 0) {
			// this is the first launch!
			timeOfAbsoluteFirstLaunch = currentMillis;
			editor.putLong(PREF.TIME_OF_ABSOLUTE_FIRST_LAUNCH,
					timeOfAbsoluteFirstLaunch);
		}

		long timeOfLastPrompt = mPreferences.getLong(PREF.TIME_OF_LAST_PROMPT,
				0);

		int launchesSinceLastPrompt = mPreferences.getInt(
				PREF.LAUNCHES_SINCE_LAST_PROMPT, 0) + 1;
		editor.putInt(PREF.LAUNCHES_SINCE_LAST_PROMPT, launchesSinceLastPrompt);

		if (totalLaunchCount >= mMinLaunchesUntilInitialPrompt
				&& ((currentMillis - timeOfAbsoluteFirstLaunch)) >= (mMinDaysUntilInitialPrompt * DateUtils.DAY_IN_MILLIS)) {
			// requirements for initial launch are met
			if (timeOfLastPrompt == 0 /* user was not yet shown a prompt */
					|| (launchesSinceLastPrompt >= mMinLaunchesUntilNextPrompt && ((currentMillis - timeOfLastPrompt) >= (mMinDaysUntilNextPrompt * DateUtils.DAY_IN_MILLIS)))) {
				editor.putLong(PREF.TIME_OF_LAST_PROMPT, currentMillis);
				editor.putInt(PREF.LAUNCHES_SINCE_LAST_PROMPT, 0);
				editor.commit();
				showDialog();
			} else {
				editor.commit();
			}
		} else {
			editor.commit();
		}

	}

	@Override
	public void _handleCancel() {
		if (mHandleCancelAsNeutral) {
			_handleNeutralChoice();
		} else {
			_handleNegativeChoice();
		}
	}

	public void _handleNegativeChoice() {
		Editor editor = mPreferences.edit();
		editor.putBoolean(PREF.DONT_SHOW_AGAIN, true);
		editor.commit();
		if (mListener != null) {
			mListener.handleNegative();
		}
	}

	public void _handleNeutralChoice() {
		if (mListener != null) {
			mListener.handleNeutral();
		}
	}

	public void _handlePositiveChoice() {
		Editor editor = mPreferences.edit();
		editor.putBoolean(PREF.DONT_SHOW_AGAIN, true);
		editor.commit();

		try {
			mActivity
					.startActivity(new Intent(Intent.ACTION_VIEW, Uri
							.parse("market://details?id="
									+ mActivity.getPackageName())));
		} catch (ActivityNotFoundException e) {
			Toast.makeText(mActivity, "Could not launch Play Store!",
					Toast.LENGTH_SHORT).show();
		}

		if (mListener != null) {
			mListener.handlePositive();
		}

	}

	/**
	 * @return the application name of the host activity
	 */
	private String getApplicationName() {
		final PackageManager pm = mActivity.getApplicationContext()
				.getPackageManager();
		ApplicationInfo ai;
		String appName;
		try {
			ai = pm.getApplicationInfo(mActivity.getPackageName(), 0);
			appName = (String) pm.getApplicationLabel(ai);
		} catch (final NameNotFoundException e) {
			appName = "(unknown)";
		}
		return appName;
	}

	/**
	 * @return Whether Google Play Store is installed on device
	 */
	private Boolean isPlayStoreInstalled() {
		PackageManager pacman = mActivity.getPackageManager();
		try {
			pacman.getApplicationInfo("com.android.vending", 0);
			return true;
		} catch (NameNotFoundException e) {
			return false;
		}
	}

	static class PREF {
		public static final String NAME = "rate_me_maybe";

		private static final String DONT_SHOW_AGAIN = "PREF_DONT_SHOW_AGAIN";
		/**
		 * How many times the app was launched in total
		 */
		public static final String TOTAL_LAUNCH_COUNT = "PREF_TOTAL_LAUNCH_COUNT";
		/**
		 * Timestamp of when the app was launched for the first time
		 */
		public static final String TIME_OF_ABSOLUTE_FIRST_LAUNCH = "PREF_TIME_OF_ABSOLUTE_FIRST_LAUNCH";
		/**
		 * How many times the app was launched since the last prompt
		 */
		public static final String LAUNCHES_SINCE_LAST_PROMPT = "PREF_LAUNCHES_SINCE_LAST_PROMPT";
		/**
		 * Timestamp of the last user prompt
		 */
		public static final String TIME_OF_LAST_PROMPT = "PREF_TIME_OF_LAST_PROMPT";
	}

}
