package com.roblox.client;

import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.service.voice.VoiceInteractionService;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.roblox.client.components.RbxButton;
import com.roblox.client.managers.NotificationManager;

public class FragmentSettings extends DialogFragment {

    private String TAG = "FragmentSettings";
    public static final String FRAGMENT_TAG = "settings_window";
    private String ctx = "settings";
    private ProgressDialog mProgressSpinner;

    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.setStyle(DialogFragment.STYLE_NO_TITLE, this.getTheme());
    }

    private RbxButton connectFacebookBtn = null;
    private View mViewRef = null;
    private RbxButton mChangeEmailBtn = null, mChangePasswordBtn = null;
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        RbxAnalytics.fireScreenLoaded(ctx);

        int containerId = (RobloxSettings.isPhone() ? R.layout.fragment_settings_card_phone : R.layout.fragment_settings_card_tablet);

        View view, cardContainer = null;
        LinearLayout swapContainer, innerContainer;
        view = inflater.inflate(R.layout.fragment_settings_new2, container, false);
        swapContainer = (LinearLayout)view.findViewById(R.id.fragment_settings_swap_container);

        cardContainer = inflater.inflate(containerId, swapContainer);

        innerContainer = (LinearLayout)cardContainer.findViewById(R.id.fragment_settings_card_inner_container);
        inflater.inflate(R.layout.fragment_settings_card_common, innerContainer);

        view.findViewById(R.id.fragment_settings_background).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                return;
            }
        });


        connectFacebookBtn = (RbxButton)view.findViewById(R.id.fragment_settings_facebook_btn);
        mChangeEmailBtn = (RbxButton)view.findViewById(R.id.fragment_settings_email_btn);
        mChangePasswordBtn = (RbxButton)view.findViewById(R.id.fragment_settings_password_btn);
        RbxButton cancelBtn = (RbxButton)view.findViewById(R.id.fragment_settings_cancel_btn);

        mChangeEmailBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                FragmentTransaction ft = getFragmentManager().beginTransaction();
                ft.setCustomAnimations(R.animator.slide_in_from_right, R.animator.slide_out_to_left);
                FragmentChangeEmail fragment = new FragmentChangeEmail();
                ft.replace(Utils.getCurrentActivityId(getActivity()), fragment, FragmentChangeEmail.FRAGMENT_TAG);
                ft.commit();
            }
        });

        mChangePasswordBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                FragmentTransaction ft = getFragmentManager().beginTransaction();
                ft.setCustomAnimations(R.animator.slide_in_from_right, R.animator.slide_out_to_left);
                FragmentChangePassword fragment = new FragmentChangePassword();
                ft.replace(Utils.getCurrentActivityId(getActivity()), fragment, FragmentChangePassword.FRAGMENT_TAG);
                ft.commit();
            }
        });

        if (AndroidAppSettings.EnableFacebookAuth()) {
            view.findViewById(R.id.fragment_settings_facebook_container).setVisibility(View.VISIBLE);
            updateFacebookButton();

            connectFacebookBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    SocialManager.getInstance().facebookConnectOrDisconnectStart(ctx);
                }
            });
        } else {
            view.findViewById(R.id.fragment_settings_facebook_container).setVisibility(View.GONE);
        }

        mViewRef = view;

        final Fragment ref = this;
        cancelBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                closeDialog();
            }
        });

        try {
            mProgressSpinner = new ProgressDialog(getActivity());
        } catch (Exception e) {
            e.printStackTrace();
        }

        return view;
    }

    public void updateFacebookButton() {
        if (SocialManager.isConnectedFacebook)
            connectFacebookBtn.setText("Disconnect");
        else
            connectFacebookBtn.setText("Connect");
    }

    public void closeDialog()
    {
        RbxAnalytics.fireButtonClick(ctx, "close");
        getActivity().getFragmentManager().beginTransaction().setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom).remove(this).commit();
    }

    @Override
    public void onResume() {
        super.onResume();
        checkNotifications();
    }

    public void checkNotifications() {
        if (RobloxSettings.isEmailNotificationEnabled() && RobloxSettings.getUserEmail().isEmpty()) {
            mViewRef.findViewById(R.id.fragment_settings_email_alert).setVisibility(View.VISIBLE);
            mChangeEmailBtn.setTextColor(getResources().getColor(R.color.RbxRed1));
            mChangeEmailBtn.setBackgroundResource(R.drawable.rbx_bg_flat_button_red);
        } else {
            mViewRef.findViewById(R.id.fragment_settings_email_alert).setVisibility(View.INVISIBLE);
            mChangeEmailBtn.setTextColor(getResources().getColor(R.color.black));
            mChangeEmailBtn.setBackgroundResource(R.drawable.rbx_drawable_flat_button_white);
        }

        if (RobloxSettings.isPasswordNotificationEnabled() && !RobloxSettings.userHasPassword) {
            mViewRef.findViewById(R.id.fragment_settings_password_alert).setVisibility(View.VISIBLE);
            mChangePasswordBtn.setTextColor(getResources().getColor(R.color.RbxRed1));
            mChangePasswordBtn.setBackgroundResource(R.drawable.rbx_bg_flat_button_red);
        } else {
            mViewRef.findViewById(R.id.fragment_settings_password_alert).setVisibility(View.INVISIBLE);
            mChangePasswordBtn.setTextColor(getResources().getColor(R.color.black));
            mChangePasswordBtn.setBackgroundResource(R.drawable.rbx_drawable_flat_button_white);
        }
    }

    public void showSpinner(String message) {
        if (mProgressSpinner != null) {
            mProgressSpinner.setMessage(message);
            mProgressSpinner.show();
        }
    }

    public void closeSpinner() {
        if (mProgressSpinner != null) {
            mProgressSpinner.hide();
        }
    }
}