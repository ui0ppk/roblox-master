package com.roblox.client.components;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.text.InputType;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.roblox.client.R;
import com.roblox.client.RobloxSettings;

public class RbxEditText extends LinearLayout {

    private EditText mTextBox = null;
    private TextView mBottomLabel = null;
    private AttributeSet mAttrs = null;
    private String mDefaultErrorStr = null;
    private String mDefaultSuccessStr = null;
    private String mDefaultHintStr = null;
    private String mDefaultLongStr = null;
    private OnRbxFocusChanged mFocusChangedListener = null;

    private float mDefTextSize = 22.0f;
    public RbxEditText(Context context)
    {
        super(context);
        init();
    }

    public RbxEditText(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        mAttrs = attrs;

        init();
    }

    private void init()
    {
        inflate(getContext(), R.layout.rbx_edittext, this);

        mTextBox = (EditText)findViewById(R.id.rbxEditTextTextBox);
        mBottomLabel = (TextView)findViewById(R.id.rbxEditTextBottomLabel);

        TypedArray a = getContext().obtainStyledAttributes(mAttrs, R.styleable.RbxEditText);
        mDefaultHintStr = a.getString(R.styleable.RbxEditText_hintText);
        mDefaultErrorStr = a.getString(R.styleable.RbxEditText_errorText);
        mDefaultSuccessStr = a.getString(R.styleable.RbxEditText_successText);

        String inputType = a.getString(R.styleable.RbxEditText_inputType);
        if (inputType != null)
            setTextBoxInput(inputType);

        String mDefaultLongStr = a.getString(R.styleable.RbxEditText_hintTextLong);
        RbxFontHelper.setCustomFont(mBottomLabel, getContext(), mAttrs);
        RbxFontHelper.setCustomFont(mTextBox, getContext(), mAttrs);

        mTextBox.setHint(mDefaultHintStr);
        mTextBox.setHintTextColor(getResources().getColor(R.color.RbxGray3));

        mTextBox.setContentDescription(getContentDescription());

        if (mDefaultLongStr != null)
            mBottomLabel.setText(mDefaultLongStr);
        else
            mBottomLabel.setText(mDefaultHintStr);

        mBottomLabel.setVisibility(TextView.INVISIBLE);
        mBottomLabel.setTextColor(getResources().getColor(R.color.RbxGray2));

        mTextBox.setOnFocusChangeListener(new OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (mFocusChangedListener != null)
                    mFocusChangedListener.focusChanged(v, hasFocus);
            }
        });

        a.recycle();
    }

    private void setTextBoxInput(String inputType) {
        switch (inputType) {
            case "email":
                mTextBox.setInputType(InputType.TYPE_CLASS_TEXT |
                    InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
                break;
            case "number":
                mTextBox.setInputType(InputType.TYPE_CLASS_NUMBER |
                    InputType.TYPE_NUMBER_VARIATION_NORMAL);
                break;
            case "date":
                mTextBox.setInputType(InputType.TYPE_CLASS_DATETIME |
                    InputType.TYPE_DATETIME_VARIATION_DATE);
                break;
            case "uri":
                mTextBox.setInputType(InputType.TYPE_CLASS_TEXT |
                    InputType.TYPE_TEXT_VARIATION_URI);
                break;
            case "password":
                mTextBox.setInputType(InputType.TYPE_CLASS_TEXT |
                        InputType.TYPE_TEXT_VARIATION_PASSWORD);
                break;
        }
    }

    /**
     * Show red error text underneath field based on a string resource.
     * @param resId
     */
    public void showErrorText(int resId) {
        showErrorText(getResources().getString(resId));
    }

    /**
     * Show red error text underneath field based on a string.
     * @param error
     */
    public void showErrorText(String error) {
        if (error != null) {
            // don't show the label again if it's already visible
            if (error.equals(mBottomLabel.getText())) return;
            mBottomLabel.setText(error);
        }
        else
            mBottomLabel.setText(mDefaultErrorStr);
        showErrorText();
    }

    private void showErrorText() {
        if (mBottomLabel.getText().length() != 0) {
            mBottomLabel.setTextColor(getResources().getColor(R.color.RbxRed2));

            if(mBottomLabel.getVisibility() != VISIBLE){
                animateLabelToVisible();
            }

            // adjustment for LDPI devices
            if (RobloxSettings.mDeviceDensity == DisplayMetrics.DENSITY_LOW)
                mTextBox.setTextSize(15);
        }

        mTextBox.setBackgroundResource(R.drawable.rbx_bg_flat_edittext_error);
    }

    public void showSuccessText(int resId) {
        showSuccessText(getResources().getString(resId));
    }

    public void showSuccessText(String msg) {
        if (msg != null) {
            // don't show the label again if it's already visible
            if (msg.equals(mBottomLabel.getText())) return;
            mBottomLabel.setText(msg);
        }
        else
            mBottomLabel.setText(mDefaultSuccessStr);

        showSuccessText();
    }

    private void showSuccessText() {
        if (mBottomLabel.getText().length() != 0) {
            mDefTextSize = mTextBox.getTextSize();
            mBottomLabel.setTextColor(getResources().getColor(R.color.RbxGreen1));

            if(mBottomLabel.getVisibility() != VISIBLE){
                animateLabelToVisible();
            }

            // adjustment for LDPI devices
            if (RobloxSettings.mDeviceDensity == DisplayMetrics.DENSITY_LOW)
                mTextBox.setTextSize(15);
        }
        else {
            mBottomLabel.setVisibility(INVISIBLE);
        }

        mTextBox.setBackgroundResource(R.drawable.rbx_bg_flat_edittext_success);
    }

    public void hideSuccessText() {
        hideErrorText();
    }

    public void setTextBoxText(String newText) {
        mTextBox.setText(newText);
    }

    public void hideErrorText() {
            AlphaAnimation alpha = new AlphaAnimation(1.0f, 0.0f);
            alpha.setDuration(250);
            alpha.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    mBottomLabel.setVisibility(INVISIBLE);
                    mTextBox.setBackgroundResource(R.drawable.rbx_bg_flat_edittext);

                    // only need to reset on LDPI devices
                    if (RobloxSettings.mDeviceDensity == DisplayMetrics.DENSITY_LOW)
                        mTextBox.setTextSize(mDefTextSize);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });
        mBottomLabel.startAnimation(alpha);
    }

    public void setHintText(int resId) {
        setHintText(getResources().getString(resId));
    }

    public void setHintText(String newHint) {
        mTextBox.setHint(newHint);
        mBottomLabel.setText(newHint);
    }

    public void setLongHintText(int resId) { mBottomLabel.setText(resId); }

    public void setLongHintText(String newHint) { mBottomLabel.setText(newHint); }

    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);

        if (gainFocus)
            mTextBox.requestFocus();
    }

    /**
     * Returns the internet text box to this component.
     * @return The Android EditText object.
     */
    public EditText getTextBox() {
        return mTextBox;
    }

    public String getText() { return mTextBox.getText().toString(); }

    /**
     * Use this to listen for when this component's focus changes. The listener will be executed
     * after the default hint show/hide logic.
     * @param l Listener to be executed when this RbxEditText has a change of focus.
     */
    public void setRbxFocusChangedListener(OnRbxFocusChanged l) {
        mFocusChangedListener = l;
    }

    public void lock() {
        AlphaAnimation alpha = RbxAnimHelper.lockAnimation(this);
        this.startAnimation(alpha);

        View.OnTouchListener consumeTouch = new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                return true;
            }
        };

        mTextBox.setOnTouchListener(consumeTouch);
    }

    public void unlock() {
        AlphaAnimation alpha = RbxAnimHelper.unlockAnimation(this);
        this.startAnimation(alpha);

        mTextBox.setOnTouchListener(null);
    }

    public void reset() {
        mTextBox.setBackgroundResource(R.drawable.rbx_bg_flat_edittext);
        mBottomLabel.setVisibility(INVISIBLE);
    }

    private void animateLabelToVisible() {
        animateLabel(VISIBLE, 0.0f, 1.0f);
    }

    private void animateLabel(final int invisible, float fromAlpha, float toAlpha) {
        AlphaAnimation alpha = new AlphaAnimation(fromAlpha, toAlpha);
        alpha.setDuration(200);
        mBottomLabel.setAnimation(alpha);
        mBottomLabel.setVisibility(invisible);
    }
}
