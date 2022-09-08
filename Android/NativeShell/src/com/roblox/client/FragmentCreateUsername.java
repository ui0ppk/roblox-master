package com.roblox.client;

import android.app.DialogFragment;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.Nullable;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.roblox.client.components.OnRbxClicked;
import com.roblox.client.components.RbxButton;
import com.roblox.client.components.RbxEditText;
import com.roblox.client.components.RbxLinearLayout;
import com.roblox.client.components.RbxProgressButton;
import com.roblox.client.components.RbxRipple;
import com.roblox.client.components.RbxTextView;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpBitmapRequestFinished;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpGetImageRequest;
import com.roblox.client.http.RbxHttpGetRequest;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.SessionManager;
import com.roblox.models.FacebookSignupData;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.regex.Pattern;

public class FragmentCreateUsername extends DialogFragment  {

    public static final String FRAGMENT_TAG = "create_username_window";
    private String mUsername;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.setStyle(DialogFragment.STYLE_NO_TITLE, this.getTheme());
    }

    private RbxProgressButton mCreateUsernameButton = null;
    private RbxEditText mUsernameField = null;
    private EditText mUsernameEditText = null;
    private enum ValidationState { NONE, VALID, INVALID };
    private ValidationState mValidationState = ValidationState.NONE;
    private final String ctx = "socialSignUp";
    private RbxLinearLayout mViewRoot = null;
    private RbxRipple mRipple = null;
    private View mViewRef = null;

    FacebookSignupData fbd = null;

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        int containerId = (RobloxSettings.isPhone() ? R.layout.fragment_create_username_card_phone : R.layout.fragment_create_username_card_tablet);
        View view, cardContainer, cardContents;
        LinearLayout swapContainer, innerContainer;

        view = inflater.inflate(R.layout.fragment_create_username, container, false);
        swapContainer = (LinearLayout)view.findViewById(R.id.fragment_create_username_swap_container);

        cardContainer = inflater.inflate(containerId, swapContainer);

        innerContainer = (LinearLayout)cardContainer.findViewById(R.id.fragment_create_username_card_inner_container);
        inflater.inflate(R.layout.fragment_create_username_card_common, innerContainer);

        mCreateUsernameButton = (RbxProgressButton)view.findViewById(R.id.fragment_create_username_create_button);
        RbxButton cancelButton = (RbxButton)view.findViewById(R.id.fragment_create_username_cancel_btn);
        mUsernameField = (RbxEditText)view.findViewById(R.id.fragment_create_username_username);
        mViewRoot = (RbxLinearLayout)view.findViewById(R.id.fragment_create_username_overlay);
        mUsernameEditText = mUsernameField.getTextBox();

        view.findViewById(R.id.fragment_create_username_background).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                return; // stops touches from falling through to fragments behind this one
            }
        });

        mCreateUsernameButton.setOnRbxClickedListener(new OnRbxClicked() {
            @Override
            public void onClick(View v) {
                onCreateUsernameButtonClicked();
            }
        });

        mUsernameEditText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        mUsernameEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    mUsernameEditText.clearFocus();
                    onCreateUsernameButtonClicked();

                    View focusView = getActivity().getCurrentFocus();
                    if (focusView != null) {
                        InputMethodManager inputManager = (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
                        inputManager.hideSoftInputFromWindow(focusView.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
                    }
                    return true;
                }

                return false;
            }
        });

        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                closeDialog();
            }
        });


        Bundle argsForSignup = getArguments();
        final View viewRef = view;
        if (argsForSignup != null) {
            fbd = argsForSignup.getParcelable("facebookData");
            if (!fbd.realName.isEmpty()) {
                mUsername = fbd.realName;
                launchUsernameSuggestion(true);
                ((RbxTextView) view.findViewById(R.id.fragment_create_username_header)).setText(fbd.realName + ", you're almost done!");
            } else
            {
                ((RbxTextView) view.findViewById(R.id.fragment_create_username_header)).setText("You're almost done!");
            }

            if (!fbd.profileUrl.isEmpty()) {
                RbxHttpGetImageRequest test = new RbxHttpGetImageRequest(fbd.profileUrl, new OnRbxHttpBitmapRequestFinished() {
                    @Override
                    public void onFinished(Bitmap b) {
                        ((ImageView) viewRef.findViewById(R.id.fragment_create_username_profile_image)).setImageBitmap(b);
                    }
                });
                test.execute();
            }
        }
        else {
            ((RbxTextView) view.findViewById(R.id.fragment_create_username_header)).setText("You're almost done!");
        }

        mViewRef = view;

        RbxAnalytics.fireScreenLoaded(ctx);
        return view;
    }

    public void closeDialog() {
        RbxAnalytics.fireButtonClick(ctx, "cancel");

        Utils.hideKeyboard(mViewRef);

        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        ft.remove(this);
        ft.commit();
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    private void validateUsername() {
        mCreateUsernameButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, "Validating...");
        mUsernameField.lock();

        final Handler mUIThreadHandler = new Handler(Looper.getMainLooper());
        mUIThreadHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                final String field = "username";
                String error = "";
                mUsername = mUsernameEditText.getText().toString();
                if (AndroidAppSettings.EnableXBOXSignupRules()) {
                    if (mUsername.isEmpty()) {
                        error = "Empty";
                        mValidationState = ValidationState.INVALID;
                        mUsernameField.showErrorText(R.string.CreateUsernameEmpty);
                    } else if (mUsername.length() < 3) {
                        error = "TooShort";
                        mValidationState = ValidationState.INVALID;
                        mUsernameField.showErrorText(R.string.SignupXBOXUsernameTooShort);
                    } else if (mUsername.length() > 20) {
                        error = "TooLong";
                        mValidationState = ValidationState.INVALID;
                        mUsernameField.showErrorText(R.string.SignupXBOXUsernameTooLong);
                    } else if (!Pattern.compile("([A-Z]|[a-z]|[0-9]|_)*").matcher(mUsername).matches()) {
                        error = "InvalidCharacters";
                        mValidationState = ValidationState.INVALID;
                        mUsernameField.showErrorText(R.string.SignupXBOXUsernameInvalidCharacters);
                    } else if (mUsername.charAt(0) == '_' || mUsername.charAt(mUsername.length() - 1) == '_') {
                        error = "InvalidFirstOrLastCharacter";
                        mValidationState = ValidationState.INVALID;
                        mUsernameField.showErrorText(R.string.SignupXBOXUsernameInvalidFirstOrLastCharacter);
                    } else if (mUsername.contains("__")) {
                        error = "InvalidUsernameDoubleUnderscore";
                        mValidationState = ValidationState.INVALID;
                        mUsernameField.showErrorText(R.string.SignupXBOXUsernameInvalidDoubleUnderscore);
                    } else {
                        launchRemoteUsernameCheck(mUIThreadHandler);
                    }
                } else {
                    if (mUsername.isEmpty()) {
                        mValidationState = ValidationState.INVALID;
                        error = "Empty";

                        mUsernameField.showErrorText(R.string.CreateUsernameEmpty);
                    } else if (mUsername.length() < 3) {
                        mValidationState = ValidationState.INVALID;
                        error = "TooShort";

                        mUsernameField.showErrorText(R.string.CreateUsernameTooShort);
                    } else if (mUsername.length() > 20) {
                        mValidationState = ValidationState.INVALID;
                        error = "TooLong";

                        mUsernameField.showErrorText(R.string.CreateUsernameTooLong);
                    } else {
                        launchRemoteUsernameCheck(mUIThreadHandler);
                    }
                }

                if (!error.isEmpty()) {
                    unlockForm();
                    mUsernameEditText.requestFocus();
                    RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
                }
            }
        }, 1000);
    }

    private void launchRemoteUsernameCheck(final Handler uiThreadHandler) {
        final String field = "username";

        try {
            mUsername = URLEncoder.encode(mUsername, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            RbxAnalytics.fireFormFieldValidation(ctx, field, "NotUTF8", true);
            mUsernameField.showErrorText(R.string.InvalidCharactersUsed);
        }

        if(AndroidAppSettings.EnableXBOXSignupRules()) {
            final RbxHttpGetRequest validationReq = new RbxHttpGetRequest(RobloxSettings.usernameCheckUrlXBOX(mUsername), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                if (!response.responseBodyAsString().isEmpty()) {
                    try {
                        JSONObject j = new JSONObject(response.responseBodyAsString());
                        boolean isValid = j.optBoolean("IsValid", false);
                        if (isValid) {
                            mValidationState = ValidationState.VALID;
                            mUsernameField.showSuccessText(null);
                            onCreateUsernameButtonClicked();
                        } else {
                            uiThreadHandler.postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    launchUsernameSuggestion(false);
                                }
                            }, 1000);
                        }
                    } catch (JSONException e) {
                        RbxAnalytics.fireFormFieldValidation(ctx, field, "ValidationJSONException", true);
                        mUsernameField.showErrorText("Server response failed. Please try again later.");
                        unlockForm();
                    }
                } else {
                    RbxAnalytics.fireFormFieldValidation(ctx, field, "NoResponse", true);
                    mUsernameField.showErrorText("Could not contact server. Please try again later.");
                    unlockForm();
                }
                }
            });
            validationReq.execute();
        } else {
            final RbxHttpGetRequest validationReq = new RbxHttpGetRequest(RobloxSettings.usernameCheckUrl(mUsername), new OnRbxHttpRequestFinished() {
                @Override
                public void onFinished(HttpResponse response) {
                if (!response.responseBodyAsString().isEmpty()) {
                    try {
                        JSONObject j = new JSONObject(response.responseBodyAsString());
                        if (j.getInt("data") != 0) {
                            uiThreadHandler.postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    launchUsernameSuggestion(false);
                                }
                            }, 1000);
                        } else {
                            mValidationState = ValidationState.VALID;
                            mUsernameField.showSuccessText(null);
                            onCreateUsernameButtonClicked();
                        }

                    } catch (JSONException e) {
                        RbxAnalytics.fireFormFieldValidation(ctx, field, "ValidationJSONException", true);
                        mUsernameField.showErrorText("Server response failed. Please try again later.");
                        unlockForm();
                    }
                } else {
                    RbxAnalytics.fireFormFieldValidation(ctx, field, "NoResponse", true);
                    mUsernameField.showErrorText("Could not contact server. Please try again later.");
                    unlockForm();
                }
                }
            });
            validationReq.execute();
        }
    }

    private void onCreateUsernameButtonClicked() {
//        if (true) {
//            onCreateSuccess();
//            return;
//        }
        RbxAnalytics.fireButtonClick(ctx, "signup");
        if (mValidationState == ValidationState.VALID) {
            String progressText = getResources().getString(R.string.CreatingUsernameProgress);
            mCreateUsernameButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, progressText);
            mUsernameField.lock();

            if (fbd != null) {
                fbd.rbxUsername = mUsername;
                Bundle b = new Bundle();
                b.putParcelable(SocialManager.FACEBOOK_DATA_KEY, fbd);
                SocialManager.getInstance().facebookSignupStart(b);
            }
        }
        else
            validateUsername();
    }

    public void onCreateSuccess() {
        SharedPreferences.Editor editor = RobloxSettings.getKeyValues().edit();
        editor.putString(SessionManager.getInstance().USERNAME_KEY, mUsername);
        editor.apply();

        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_USER_LOGIN);
//        float cx = mCreateUsernameButton.getX() + (mCreateUsernameButton.getWidth() / 2);
//        float cy = mCreateUsernameButton.getY() + (mCreateUsernameButton.getHeight() / 2);
//        mViewRoot.startWipe(cx, cy);
//        mViewRoot.setBackgroundColor(getResources().getColor(R.color.RbxTransparent));
    }

    private void launchUsernameSuggestion(final boolean onLaunch) {
        RbxHttpGetRequest suggestReq = new RbxHttpGetRequest(RobloxSettings.recommendUsernameUrl(mUsername), new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                String error = "";
                String field = "username";
                if (!response.responseBodyAsString().isEmpty()) {
                    if (mUsername.equals(response.responseBodyAsString())) {
                        mUsernameField.showErrorText("This username is not allowed! Please try another.");
                        error = "UsernameNotAllowed";
                    }
                    else {
                        mUsernameField.setTextBoxText(response.responseBodyAsString());
                        if (onLaunch)
                            mUsernameField.showSuccessText("Try this username!");
                        else
                            mUsernameField.showSuccessText("That username was already taken! Try this one instead.");

                        error = "UsernameTaken";
                    }
                }
                else {
                    error = "NoResponseSuggestion";
                    mUsernameField.showErrorText("Could not contact server. Please try again later.");
                }

                if (!error.isEmpty()) {
                    mValidationState = ValidationState.INVALID;
                    unlockForm();
                    RbxAnalytics.fireFormFieldValidation(ctx, field, true);
                }
                else {
                    mValidationState = ValidationState.VALID;
                    onCreateUsernameButtonClicked();

                    RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
                }
            }
        });
        suggestReq.execute();
    }

    private void unlockForm() {
        mCreateUsernameButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
        mUsernameField.unlock();
    }
}
