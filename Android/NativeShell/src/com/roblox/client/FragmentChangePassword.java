package com.roblox.client;

import android.app.DialogFragment;
import android.app.FragmentTransaction;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
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
import com.roblox.client.managers.SessionManager;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class FragmentChangePassword extends DialogFragment {
    private String TAG = "FragmentChangePassword";

    private Bundle mArgs = null;

    private RbxButton mCancelBtn = null;
    private RbxProgressButton mSaveBtn = null;
    private RbxEditText mOldPasswordField, mNewPasswordField, mConfirmPasswordField = null;
    private RbxTextView mTitleText = null;

    private View mViewRef = null;

    private String ctx = "changePassword";
    private boolean leavingOldPasswordField = false;

    protected static final String PASSWORD_KEY = "password";
    public static final String FRAGMENT_TAG = "change_password_window";

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
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        int containerId = (RobloxSettings.isPhone() ? R.layout.fragment_change_password_card_phone : R.layout.fragment_change_password_card_tablet);

        View view, cardContainer, cardContents = null;
        LinearLayout swapContainer, innerContainer;
        view = inflater.inflate(R.layout.fragment_change_password_new, container, false);
        swapContainer = (LinearLayout)view.findViewById(R.id.fragment_change_password_swap_container);

        cardContainer = inflater.inflate(containerId, swapContainer);

        innerContainer = (LinearLayout)cardContainer.findViewById(R.id.fragment_change_password_card_inner_container);
        cardContents = inflater.inflate(R.layout.fragment_change_password_card_common, innerContainer);

        mViewRef = view;

        mCancelBtn = (RbxButton)view.findViewById(R.id.fragment_change_password_cancel_btn);
        mSaveBtn = (RbxProgressButton)view.findViewById(R.id.fragment_change_password_save_btn);
        mOldPasswordField = (RbxEditText)view.findViewById(R.id.fragment_change_password_old_password);
        mNewPasswordField = (RbxEditText)view.findViewById(R.id.fragment_change_password_new_password);
        mConfirmPasswordField = (RbxEditText)view.findViewById(R.id.fragment_change_password_confirm_password);
        mTitleText = (RbxTextView)view.findViewById(R.id.fragment_change_password_header);


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

        view.findViewById(R.id.fragment_change_password_background).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                return;
            }
        });

        mOldPasswordField.getTextBox().setImeOptions(EditorInfo.IME_ACTION_NEXT);
        mOldPasswordField.getTextBox().setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    onOldPasswordNext(true);

                    return true;
                }

                return false;
            }
        });
        mOldPasswordField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!hasFocus)
                    onOldPasswordNext(false);
            }
        });

        mNewPasswordField.getTextBox().setImeOptions(EditorInfo.IME_ACTION_NEXT);
        mNewPasswordField.getTextBox().setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    onNewPasswordNext(true);

                    return true;
                }

                return false;
            }
        });
        mNewPasswordField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!hasFocus) {
                    if (leavingOldPasswordField) return;
                    onNewPasswordNext(false);
                }
                else
                    leavingOldPasswordField = false;
            }
        });

        mConfirmPasswordField.getTextBox().setImeOptions(EditorInfo.IME_ACTION_DONE);
        mConfirmPasswordField.getTextBox().setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    if (mConfirmPasswordField.getText().isEmpty())
                        mConfirmPasswordField.showErrorText(R.string.ChangePasswordNoNewPassword);
                    else {
                        Utils.hideKeyboard(mViewRef, mConfirmPasswordField.getTextBox());
                        mConfirmPasswordField.getTextBox().clearFocus();
                        mNewPasswordField.showSuccessText(null);
                        mConfirmPasswordField.showSuccessText(null);
                        onButtonClicked();
                    }

                    return true;
                }

                return false;
            }
        });

        if (!RobloxSettings.userHasPassword) {
            mTitleText.setText(R.string.ChangePasswordAddTitle);
            (view.findViewById(R.id.fragment_change_password_value)).setVisibility(View.VISIBLE);
            mOldPasswordField.setVisibility(View.GONE);
            view.findViewById(R.id.fragment_change_password_value_icon).setVisibility(View.VISIBLE);
            ((RbxTextView)view.findViewById(R.id.fragment_change_password_value_text)).setTextColor(getResources().getColor(R.color.RbxRed1));

            // Force the keyboard to open on the Old Password field
            mNewPasswordField.getTextBox().requestFocus();
            Utils.showKeyboard(mViewRef, mNewPasswordField.getTextBox());
        }
        else {
            // Force the keyboard to open on the Old Password field
            mOldPasswordField.getTextBox().requestFocus();
            Utils.showKeyboard(mViewRef, mOldPasswordField.getTextBox());
        }

        return view;
    }

    public void closeDialog() {
        RbxAnalytics.fireButtonClick(ctx, "close");

        Utils.hideKeyboard(mViewRef);

        FragmentTransaction ft = getActivity().getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_left, R.animator.slide_out_to_right);
        FragmentSettings frag = new FragmentSettings();
        ft.replace(Utils.getCurrentActivityId(getActivity()), frag, FragmentSettings.FRAGMENT_TAG);
        ft.commit();
    }

    public void onButtonClicked() {
        RbxAnalytics.fireButtonClick(ctx, "submit");

        lockFields();

        Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
        mUIThreadHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                final String userOldPassword = mOldPasswordField.getText();
                final String userNewPassword = mNewPasswordField.getText();
                final String userConfirmPassword = mConfirmPasswordField.getText();

                boolean missing = false;

                if (userOldPassword == null || userOldPassword.equals(""))
                    mOldPasswordField.showErrorText(R.string.ChangePasswordNoOldPassword);
                if (userNewPassword == null || userNewPassword.equals(""))
                    mNewPasswordField.showErrorText(R.string.ChangePasswordNoNewPassword);
                if (userConfirmPassword == null || userConfirmPassword.equals(""))
                    mConfirmPasswordField.showErrorText(R.string.ChangePasswordNoConfirmPassword);

                if (missing) {
                    unlockFields();
                    return;
                }

                SharedPreferences keyValues = RobloxSettings.getKeyValues();

                final String savedUsername = keyValues.getString("username", "");

                if (!userNewPassword.equals(userConfirmPassword)) // Mismatched passwords
                {
                    mNewPasswordField.showErrorText(R.string.ChangePasswordNoMatch);

                    mOldPasswordField.clearFocus();
                    mNewPasswordField.requestFocus();
                    mConfirmPasswordField.clearFocus();

                    mConfirmPasswordField.showErrorText(null);
                    unlockFields();
                } else if (validatePassword(userNewPassword, true, true)) // Everything looks okay, send to server
                {
                    String params = RobloxSettings.changePasswordParams(userOldPassword, userNewPassword, userConfirmPassword);
                    RbxHttpPostRequest changePasswordRequest = new RbxHttpPostRequest(RobloxSettings.changePasswordUrl(), params, null,
                            new OnRbxHttpRequestFinished() {
                                @Override
                                public void onFinished(HttpResponse response) {
                                    onPasswordChangeFinished(response.responseBodyAsString(), savedUsername, userNewPassword);
                                } // end onFinished
                            }); // end HTTPPostRequest
                    changePasswordRequest.execute();
                }
            }
        }, 1000);
    }

    private boolean validatePassword(String userNewPassword, boolean showErrorNewPass, boolean showErrorConfirmPass) {
        if( AndroidAppSettings.EnableXBOXSignupRules() ) {
            if (userNewPassword.length() < 8) {
                if (showErrorNewPass)
                    mNewPasswordField.showErrorText(R.string.SignupXBOXPasswordTooShort);
                if (showErrorConfirmPass)
                    mConfirmPasswordField.showErrorText(null);
            }
                return true;
        } else {
            Pattern p = Pattern.compile(".*[0-9].*[0-9].*");
            Matcher m = p.matcher(userNewPassword);
            boolean matches = m.matches();

            if (userNewPassword.length() < 4) {
                if (showErrorNewPass)
                    mNewPasswordField.showErrorText(R.string.SignupPasswordTooShort);
                if (showErrorConfirmPass)
                    mConfirmPasswordField.showErrorText(null);
            } else if (userNewPassword.length() > 20) {
                if (showErrorNewPass)
                    mNewPasswordField.showErrorText(R.string.SignupPasswordTooLong);
                if (showErrorConfirmPass)
                    mConfirmPasswordField.showErrorText(null);
            } else if (!matches) {
                if (showErrorNewPass)
                    mNewPasswordField.showErrorText(R.string.SignupPasswordNoNumbers);
                if (showErrorConfirmPass)
                    mConfirmPasswordField.showErrorText(null);
            } else
                return true;
        }

        return false;
    }

    private void lockFields() {
        mSaveBtn.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.Working);
        mOldPasswordField.lock();
        mNewPasswordField.lock();
        mConfirmPasswordField.lock();
    }

    private void unlockFields() {
        mSaveBtn.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
        mOldPasswordField.unlock();
        mNewPasswordField.unlock();
        mConfirmPasswordField.unlock();
    }

    private void onPasswordChangeFinished(String response, String savedUsername, String userNewPassword) {
        Log.i(TAG, "Change password response: " + response);
        // Check the response to see if we were successful or not
        JSONObject mJson = null;
        boolean success = false;
        String message = "Request failed. Your password was not changed.";
        try {
            mJson = new JSONObject(response);
            if (mJson != null) {
                success = mJson.getBoolean("Success");
                message = mJson.getString("Message");
            }
        } catch (JSONException e) {
            e.printStackTrace();
            unlockFields();
        }

        if (success) {
            if (!RobloxSettings.userHasPassword && RobloxSettings.isPasswordNotificationEnabled()) {
                RobloxSettings.userHasPassword = true;
                RobloxSettings.disablePasswordNotification();
                ((ActivityNativeMain)getActivity()).clearSettingsNotification();
            }

            Utils.alert("You have successfully changed your password.");
            SessionManager sm = SessionManager.getInstance();
            sm.doLoginFromStart(savedUsername, userNewPassword, (RobloxActivity)getActivity());

            closeDialog();
        }
        else
        {
            processErrorResponse(message);
            unlockFields();
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        RbxAnalytics.fireButtonClick(ctx, "close");
    }

    private void processErrorResponse(String msg) {
        if (msg.contains("Password must contain at least 2")) {
            mNewPasswordField.showErrorText("Invalid password!");
            mConfirmPasswordField.showErrorText(null);
            mOldPasswordField.showSuccessText(null);
        }
        else if (msg.contains("Your password could not be changed")) {
            mOldPasswordField.showErrorText("Incorrect password!");
            mNewPasswordField.showSuccessText(null);
            mConfirmPasswordField.showSuccessText(null);
        }
    }

    private void onOldPasswordNext(boolean forceFocus) {
        if (mOldPasswordField.getText().isEmpty()) {
            mOldPasswordField.showErrorText(R.string.ChangePasswordNoOldPassword);
            if (forceFocus) {
                mOldPasswordField.requestFocus();
                mNewPasswordField.clearFocus();
                mConfirmPasswordField.clearFocus();
            }
        }
        else {
            leavingOldPasswordField = true;
            mOldPasswordField.showSuccessText(null);
            mNewPasswordField.requestFocus();
        }
    }

    private void onNewPasswordNext(boolean forceFocus) {
        if (mNewPasswordField.getText().isEmpty()) {
            mNewPasswordField.showErrorText(R.string.ChangePasswordNoNewPassword);
            if (forceFocus) {
                mOldPasswordField.clearFocus();
                mNewPasswordField.requestFocus();
                mConfirmPasswordField.clearFocus();
            }
        } else if (validatePassword(mNewPasswordField.getText(), true, false)) {
            mNewPasswordField.showSuccessText(null);
            mConfirmPasswordField.getTextBox().requestFocus();
        }
    }
}