package com.roblox.client;

import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.roblox.client.components.OnRbxClicked;
import com.roblox.client.components.RbxButton;
import com.roblox.client.components.RbxEditText;
import com.roblox.client.components.RbxProgressButton;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.SessionManager;

/**
 * Created by roblox on 11/21/14.
 */
public class FragmentLogin extends DialogFragment implements NotificationManager.Observer {

    private EditText mUsernameEditText, mPasswordEditText = null;
    private RbxEditText mUsernameField, mPasswordField = null;

    private RbxButton mCancelButton, mSignupButton = null;
    private RbxProgressButton mLoginButton = null;

    private static String ctx = "login";
    private View mViewRef = null;

    public static final String FRAGMENT_TAG = "login_window";

    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


        if (!RobloxSettings.isPhone())
            this.setStyle(DialogFragment.STYLE_NO_TITLE, this.getTheme());
        else
            this.setStyle(DialogFragment.STYLE_NORMAL, android.R.style.Theme_Black_NoTitleBar_Fullscreen);
    }

    // -----------------------------------------------------------------------
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view, cardContainer, cardContents = null;
        LinearLayout swapContainer, innerContainer;

        int containerId = (RobloxSettings.isPhone() ? R.layout.fragment_login_card_phone : R.layout.fragment_login_card_tablet);

        view = inflater.inflate(R.layout.fragment_login_new, container, false);
        swapContainer = (LinearLayout)view.findViewById(R.id.fragment_login_swap_container);

        cardContainer = inflater.inflate(containerId, swapContainer);

        innerContainer = (LinearLayout)cardContainer.findViewById(R.id.fragment_login_card_inner_container);
        cardContents = inflater.inflate(R.layout.fragment_login_card_common, innerContainer);

        // This stops touches from falling through to the layout behind the signup form
        LinearLayout bg = (LinearLayout)view.findViewById(R.id.fragment_login_background);
        bg.setOnClickListener(null);

        SessionManager sessionManager = SessionManager.getInstance();

        mUsernameField = (RbxEditText)view.findViewById(R.id.fragment_login_username);
        mUsernameEditText = mUsernameField.getTextBox();

        mUsernameEditText.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        mUsernameEditText.setText( sessionManager.getUsername() );
        mUsernameEditText.requestFocus();
        mViewRef = view;

        Utils.showKeyboard(mViewRef, mUsernameEditText);


        mPasswordField = (RbxEditText)view.findViewById(R.id.fragment_login_password);
        mPasswordEditText = mPasswordField.getTextBox();

        mPasswordEditText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        mPasswordEditText.setText(sessionManager.getPassword());
        mPasswordEditText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_GO || actionId == EditorInfo.IME_ACTION_DONE) {
                    onLoginButtonClick();
                    return true;
                }
                return false;
            }
        });

        mCancelButton = (RbxButton)view.findViewById(R.id.fragment_login_btn_cancel);
        mCancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                closeDialog();
            }
        });

        mLoginButton = (RbxProgressButton)view.findViewById(R.id.fragment_login_btn_login);
        mLoginButton.setOnRbxClickedListener(new OnRbxClicked() {
            @Override
            public void onClick(View v) {
                onLoginButtonClick();
            }
        });

        mSignupButton = (RbxButton)view.findViewById(R.id.fragment_login_btn_signup);
        mSignupButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switchToSignup();
            }
        });


        return view;
    }

    // -----------------------------------------------------------------------
    @Override
    public void onStart() {
        super.onStart();

        NotificationManager.getInstance().addObserver(this);
    }

    // -----------------------------------------------------------------------
    @Override
    public void onStop() {
        super.onStop();

        NotificationManager.getInstance().removerObserver(this);
    }

    // -----------------------------------------------------------------------
    private void onLoginButtonClick() {
        mUsernameEditText.clearFocus();
        mPasswordEditText.clearFocus();
        mLoginButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.SignupValidating);

        Utils.hideKeyboard(mViewRef);
        lockFields();

        final Handler mUIThreadHandler = new Handler(Looper.getMainLooper());

        mUIThreadHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                RbxAnalytics.fireButtonClick(ctx, "submit");
                String username = mUsernameEditText.getText().toString();
                if(username.isEmpty())
                {
                    unlockFields();
                    mUsernameField.showErrorText(R.string.SignupNoUsername);
                    return;
                }
                mUsernameField.reset();

                String password = mPasswordEditText.getText().toString();
                if(password.isEmpty())
                {
                    unlockFields();
                    mPasswordField.showErrorText(R.string.SignupNoPassword);
                    mPasswordEditText.requestFocus();
                    Utils.showKeyboard(mViewRef, mPasswordEditText);

                    return;
                }
                mPasswordField.reset();


                // Trigger login
                SessionManager.getInstance().doLoginFromStart(username, password, (RobloxActivity)getActivity());
            }
        }, 500);
    }

    // -----------------------------------------------------------------------
    public void closeDialog() {
        if (!SessionManager.getInstance().getIsLoggedIn())
            RbxAnalytics.fireButtonClick(ctx, "close");
        Utils.hideKeyboard(mViewRef);

        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        ft.remove(this);
        ft.commit();
    }

    // -----------------------------------------------------------------------
    @Override
    public void handleNotification(int notificationId, Bundle userParams) {
        switch (notificationId) {
            case NotificationManager.EVENT_USER_LOGIN:
                if(SessionManager.getInstance().getIsLoggedIn()) {
                    closeDialog();
                }
                break;
            case NotificationManager.EVENT_USER_LOGIN_STOPPED:
                stopLoginActivity();
                break;
//            case NotificationManager.EVENT_USER_CAPTCHA_SOLVED: {
//                onLoginCaptchaSolved();
//            }
            default:
                break;
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        if (!SessionManager.getInstance().getIsLoggedIn())
            RbxAnalytics.fireButtonClick(ctx, "close");
    }

    public void showLoginActivity() {
        mLoginButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.LoggingIn);
    }

    public void stopLoginActivity() {
        mLoginButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
        unlockFields();
    }

    public void lockFields() {
        mUsernameField.lock();
        mPasswordField.lock();
    }

    private void unlockFields() {
        mLoginButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
        mUsernameField.unlock();
        mPasswordField.unlock();
    }

//    public void onLoginCaptchaSolved() {
//        // Find captcha web fragment
//        Fragment fragWeb = getFragmentManager().findFragmentByTag(RobloxWebFragment.FRAGMENT_TAG_CAPTCHA);
//        FragmentTransaction ft = getFragmentManager().beginTransaction();
//
//        if (fragWeb != null && fragWeb.isVisible())
//        {
//            // Remove fragment
//            ft.remove(fragWeb);
//            ft.commit();
//
//            // Trigger another login
//            SessionManager.getInstance().doLoginFromStart(
//                    mUsernameEditText.getText().toString(),
//                    mPasswordEditText.getText().toString(),
//                    (RobloxActivity)getActivity()
//            );
//        }
//    }

    private void switchToSignup() {
        RbxAnalytics.fireButtonClick(ctx, "signup");

        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_right, R.animator.slide_out_to_left);
        FragmentSignUp fragment = new FragmentSignUp();
        ft.hide(this);
        ft.replace(Utils.getCurrentActivityId(getActivity()), fragment, FragmentSignUp.FRAGMENT_TAG);
        ft.commit();
    }
}
