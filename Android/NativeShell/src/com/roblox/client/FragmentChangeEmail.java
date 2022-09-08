package com.roblox.client;

import android.app.DialogFragment;
import android.app.FragmentTransaction;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.roblox.client.components.OnRbxClicked;
import com.roblox.client.components.OnRbxFocusChanged;
import com.roblox.client.components.RbxButton;
import com.roblox.client.components.RbxEditText;
import com.roblox.client.components.RbxProgressButton;
import com.roblox.client.components.RbxTextView;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpPostRequest;

import org.json.JSONException;
import org.json.JSONObject;

public class FragmentChangeEmail extends DialogFragment {
    private String TAG = "FragmentChangeEmail";

    private Bundle mArgs = null;
    private RbxTextView mTitleText = null;
    private RbxEditText mNewEmailField, mCurrentPasswordField = null;
    private RbxButton mCancelBtn = null;
    private RbxProgressButton mSaveBtn = null;
    private View mViewRef = null;
    private boolean leavingNewEmailField = false;
    public static final String FRAGMENT_TAG = "change_email_window";

    private String ctx = "changeEmail";
    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        RbxAnalytics.fireScreenLoaded(ctx);
        if (!RobloxSettings.isPhone())
            this.setStyle(DialogFragment.STYLE_NO_TITLE, this.getTheme());
        else
            this.setStyle(DialogFragment.STYLE_NORMAL, android.R.style.Theme_Black_NoTitleBar_Fullscreen);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        int containerId = (RobloxSettings.isPhone() ? R.layout.fragment_change_email_card_phone : R.layout.fragment_change_email_card_tablet);
        View view, cardContainer, cardContents = null;
        LinearLayout swapContainer, innerContainer;
        view = inflater.inflate(R.layout.fragment_change_email_new, container, false);
        swapContainer = (LinearLayout)view.findViewById(R.id.fragment_change_email_swap_container);

        cardContainer = inflater.inflate(containerId, swapContainer);

        innerContainer = (LinearLayout)cardContainer.findViewById(R.id.fragment_change_email_card_inner_container);
        cardContents = inflater.inflate(R.layout.fragment_change_email_card_common, innerContainer);

        mViewRef = view;

        mTitleText = (RbxTextView)view.findViewById(R.id.fragment_change_email_header);
        mNewEmailField = (RbxEditText)view.findViewById(R.id.fragment_change_email_new_email);
        mCurrentPasswordField = (RbxEditText)view.findViewById(R.id.fragment_change_email_current_password);
        mCancelBtn = (RbxButton)view.findViewById(R.id.fragment_change_email_cancel_btn);
        mSaveBtn = (RbxProgressButton)view.findViewById(R.id.fragment_change_email_save_btn);

        if (!RobloxSettings.getUserEmail().isEmpty()) {
            mNewEmailField.setTextBoxText(RobloxSettings.getSanitizedUserEmail());
            if (RobloxSettings.isUserUnder13) {
                mTitleText.setText(R.string.ChangeEmailTitleUnder13);
                mNewEmailField.setHintText(R.string.ChangeEmailNewEmailUnder13);
            } else {
                mTitleText.setText(R.string.ChangeEmailTitle);
                mNewEmailField.setHintText(R.string.ChangeEmailNewEmail);
            }
        }
        else {
            if (RobloxSettings.isUserUnder13) {
                mTitleText.setText(R.string.ChangeEmailAddTitleUnder13);
                mNewEmailField.setHintText(R.string.ChangeEmailNewEmailUnder13);
            }
            else {
                mTitleText.setText(R.string.ChangeEmailAddTitle);
                mNewEmailField.setHintText(R.string.ChangeEmailNewEmail);
            }
        }

        if (RobloxSettings.isEmailNotificationEnabled() && RobloxSettings.getUserEmail().isEmpty()) {
            view.findViewById(R.id.fragment_change_email_value).setVisibility(View.VISIBLE);
            view.findViewById(R.id.fragment_change_email_spacer).setVisibility(View.GONE);
            if (RobloxSettings.isUserUnder13) mNewEmailField.showErrorText(R.string.ChangeEmailNewEmailUnder13);
            else mNewEmailField.showErrorText(R.string.ChangeEmailNewEmail);
        }
        else {
            //mTitleText.setPadding(0, 0, 0, 10);
            ((RbxTextView)view.findViewById(R.id.fragment_change_email_value_text)).setHeight((int) Utils.pixelToDp(20));
            view.findViewById(R.id.fragment_change_email_value_icon).setVisibility(View.GONE);
            view.findViewById(R.id.fragment_change_email_spacer).setVisibility(View.INVISIBLE);
        }

        mNewEmailField.requestFocus();
        Utils.showKeyboard(mViewRef, mNewEmailField.getTextBox());

        // Stops taps from passing through to 'More' page
        view.findViewById(R.id.fragment_change_email_background).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                return;
            }
        });

        mCancelBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                closeDialog();
            }
        });

        mSaveBtn.setOnRbxClickedListener(new OnRbxClicked() {
            @Override
            public void onClick(View v) {
                onButtonClicked();
            }
        });

        mNewEmailField.getTextBox().setImeOptions(EditorInfo.IME_ACTION_NEXT);
        mNewEmailField.getTextBox().setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    onNewEmailNext(true);

                    return true;
                }

                return false;
            }
        });
        mNewEmailField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!hasFocus)
                    onNewEmailNext(false);
            }
        });

        mCurrentPasswordField.getTextBox().setImeOptions(EditorInfo.IME_ACTION_DONE);
        mCurrentPasswordField.getTextBox().setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    if (mCurrentPasswordField.getText().isEmpty()) {
                        mCurrentPasswordField.showErrorText(R.string.ChangeEmailNoPassword);
                        mCurrentPasswordField.requestFocus();
                    }
                    else {
                        Utils.hideKeyboard(mViewRef, mCurrentPasswordField.getTextBox());
                        mCurrentPasswordField.getTextBox().clearFocus();
                        mNewEmailField.showSuccessText(null);
                        mCurrentPasswordField.showSuccessText(null);
                        onButtonClicked();
                    }

                    return true;
                }

                return false;
            }
        });

        return view;
    }

    private void onNewEmailNext(boolean forceFocus) {
        if (mNewEmailField.getText().isEmpty()) {
            mNewEmailField.showErrorText(R.string.ChangeEmailNoNewEmail);
            if (forceFocus) {
                mNewEmailField.requestFocus();
                mCurrentPasswordField.clearFocus();
            }
        } else if (mNewEmailField.getText().equals(RobloxSettings.getSanitizedUserEmail())) {
            mNewEmailField.showErrorText(R.string.ChangeEmailSameAsCurrent);
            if (forceFocus) {
                mNewEmailField.requestFocus();
                mCurrentPasswordField.clearFocus();
            }
        }
        else {
            leavingNewEmailField = true;
            mNewEmailField.showSuccessText(null);
            mCurrentPasswordField.requestFocus();
        }
    }

    public void closeDialog()
    {
        RbxAnalytics.fireButtonClick(ctx, "close");

        Utils.hideKeyboard(mViewRef);

        FragmentTransaction ft = getActivity().getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_left, R.animator.slide_out_to_right);
        FragmentSettings frag = new FragmentSettings();
        ft.replace(Utils.getCurrentActivityId(getActivity()), frag, FragmentSettings.FRAGMENT_TAG);
        ft.commit();
    }

    public void onButtonClicked()
    {
        RbxAnalytics.fireButtonClick(ctx, "submit");
        lockFields();

        final String userNewEmail = mNewEmailField.getText();
        final String userCurrPassword = mCurrentPasswordField.getText();

        boolean wasError = false;

        if (userNewEmail == null || userNewEmail.equals("")) {
            mNewEmailField.showErrorText(R.string.ChangeEmailNoNewEmail);
            mNewEmailField.requestFocus();
            Utils.showKeyboard(mViewRef, mNewEmailField.getTextBox());
            wasError = true;
        }
        else if (RobloxSettings.getUserEmail().equals(userNewEmail) || userNewEmail.equals(RobloxSettings.getSanitizedUserEmail())) {
            mNewEmailField.showErrorText(R.string.ChangeEmailSameAsCurrent);
            mNewEmailField.requestFocus();
            Utils.showKeyboard(mViewRef, mNewEmailField.getTextBox());
            wasError = true;
        }

        if (userCurrPassword == null || userCurrPassword.equals("")) {
            mCurrentPasswordField.showErrorText(R.string.ChangeEmailNoPassword);
            mCurrentPasswordField.requestFocus();
            Utils.showKeyboard(mViewRef, mCurrentPasswordField.getTextBox());
            wasError = true;
        }

        if (wasError) {
            unlockFields();
            return;
        }

        mCurrentPasswordField.showSuccessText(null);
        mCurrentPasswordField.clearFocus();
        Utils.hideKeyboard(mViewRef);

        // Everything looks okay, send to server
        String params = RobloxSettings.changeEmailParams(userNewEmail, userCurrPassword);
        RbxHttpPostRequest changeEmailRequest = new RbxHttpPostRequest(RobloxSettings.changeEmailUrl(), params, null,
                new OnRbxHttpRequestFinished() {
                    @Override
                    public void onFinished(HttpResponse response) {
                        onEmailChangeFinished(response.responseBodyAsString());
                    } // end first onFinished
                });
        changeEmailRequest.execute();
    }

    private void onEmailChangeFinished(String response) {
        Log.i(TAG, "Change email response: " + response);
        // Check the response to see if we were successful or not
        JSONObject mJson = null;
        boolean success = false;
        String message = "Request failed. Your email was not changed.";
        try {
            mJson = new JSONObject(response);
            if (mJson != null)
            {
                success = mJson.getBoolean("Success");
                message = mJson.getString("Message");
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }

        if (success) {
            onSuccess();
        }
        else
        {
            Utils.alert(message);
            unlockFields();
        }
    }

    private void onSuccess() {
        // Save the new email address
        RobloxSettings.setUserEmail(mNewEmailField.getText());

        // If we were showing notification icons before, we need to clear them
        if (RobloxSettings.isEmailNotificationEnabled() && RobloxSettings.getUserEmail().isEmpty()) {
            RobloxSettings.disableEmailNotification();
            // For now, safe to assume we're on ActivityNativeMain
            ((ActivityNativeMain)getActivity()).clearSettingsNotification();
        }

        Utils.alert("You have successfully changed your email address.");
        closeDialog();
    }

    private void lockFields() {
        mSaveBtn.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.Working);
        mNewEmailField.lock();
        mCurrentPasswordField.lock();
    }

    private void unlockFields() {
        mSaveBtn.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
        mNewEmailField.unlock();
        mCurrentPasswordField.unlock();
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        RbxAnalytics.fireButtonClick(ctx, "close");
    }
}
