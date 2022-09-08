package com.roblox.client;

// License: MIT
// https://github.com/Kopfgeldjaeger/RateMeMaybe/blob/master/code/src/com/kopfgeldjaeger/ratememaybe
//
// Has ROBLOX modifications.

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;

public class RateMeMaybeFragment extends DialogFragment implements
			DialogInterface.OnClickListener, DialogInterface.OnCancelListener {

	private RMMFragInterface mInterface;

	private String title;
	private String message;
	private int customIcon;
	private String positiveBtn;
	private String neutralBtn;
	private String negativeBtn;

	public interface RMMFragInterface {
		void _handlePositiveChoice();

		void _handleNeutralChoice();

		void _handleNegativeChoice();

		void _handleCancel();
	}

	public void setData(int customIcon, String title, String message,
			String positiveBtn, String neutralBtn, String negativeBtn,
			RMMFragInterface myInterface) {
		this.customIcon = customIcon;
		this.title = title;
		this.message = message;
		this.positiveBtn = positiveBtn;
		this.neutralBtn = neutralBtn;
		this.negativeBtn = negativeBtn;
		this.mInterface = myInterface;
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		// Fragment including variables will survive orientation changes
		this.setRetainInstance(true);
	}

	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {

		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

		if (customIcon != 0) {
			builder.setIcon(customIcon);
		}
		builder.setTitle(title);
		builder.setMessage(message);
		builder.setPositiveButton(positiveBtn, this);
		builder.setNeutralButton(neutralBtn, this);
		builder.setNegativeButton(negativeBtn, this);

		AlertDialog alert = builder.create();
		return alert;
	}

	@Override
	public void onCancel(DialogInterface dialog) {
		mInterface._handleCancel();
	}

	@Override
	public void onClick(DialogInterface dialog, int choice) {
		switch (choice) {
		case DialogInterface.BUTTON_POSITIVE:
			mInterface._handlePositiveChoice();
			break;
		case DialogInterface.BUTTON_NEUTRAL:
			mInterface._handleNeutralChoice();
			break;
		case DialogInterface.BUTTON_NEGATIVE:
			mInterface._handleNegativeChoice();
			break;
		}
	}

	@Override
	public void onDestroyView() {
		// Work around bug:
		// http://code.google.com/p/android/issues/detail?id=17423
		Dialog dialog = getDialog();

		if ((dialog != null) && getRetainInstance()) {
			dialog.setDismissMessage(null);
		}

		super.onDestroyView();
	}

}