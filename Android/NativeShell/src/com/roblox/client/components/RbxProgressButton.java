package com.roblox.client.components;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.roblox.client.R;

public class RbxProgressButton extends RelativeLayout {
    public RbxProgressButton(Context context) {
        super(context);
    }

    public RbxProgressButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        //RbxFontHelper.setCustomFont(this, context, attrs);

        init(attrs);
    }

    public RbxProgressButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        //RbxFontHelper.setCustomFont(this, context, attrs);

        init(attrs);
    }

    private RbxButton mButton = null;
    private TextView mProgressText = null;
    private ProgressBar mProgressSpinner = null;
    private LinearLayout mContainer = null;
    private OnRbxClicked mOnRbxClickedListener = null;
    public enum STATE_COMMAND { SHOW_BUTTON, SHOW_PROGRESS};
    public enum STATE { BUTTON, ANIMATING, PROGRESS;};
    private STATE mViewState = STATE.BUTTON;
    private STATE_COMMAND mQueuedCommand = null;

    private String mNewProgressText = null;
    private String DEFAULT_BUTTON_TEXT, DEFAULT_PROGRESS_TEXT = null;
    private final int DEFAULT_HIDE_DURATION = 150, DEFAULT_REVEAL_DURATION = 200,
            DEFAULT_PROGRESS_TEXT_SIZE = 25, DEFAULT_PROGRESS_TEXT_COLOR = 0xFF0000,
            DEFAULT_BUTTON_TEXT_SIZE = 25, DEFAULT_BUTTON_TEXT_COLOR = 0xFF0000;

    RbxRipple ripple = null;

    private void init(AttributeSet attrs) {
        inflate(getContext(), R.layout.rbx_button_progress, this);

        TypedArray a = getContext().obtainStyledAttributes(attrs, R.styleable.RbxProgressButton);

        DEFAULT_BUTTON_TEXT = a.getString(R.styleable.RbxProgressButton_defButtonText);
        DEFAULT_PROGRESS_TEXT = a.getString(R.styleable.RbxProgressButton_defProgressText);

        if (DEFAULT_BUTTON_TEXT == null)
            DEFAULT_BUTTON_TEXT = "NO BUTTON TEXT";
        if (DEFAULT_PROGRESS_TEXT == null)
            DEFAULT_PROGRESS_TEXT = "Working";


        mButton = (RbxButton)findViewById(R.id.rbxProgressBtnRbxBtn);
        mProgressText = (TextView)findViewById(R.id.rbxProgressBtnProgressText);
        mProgressSpinner = (ProgressBar)findViewById(R.id.rbxProgressBtnProgressBar);
        mContainer = (LinearLayout)findViewById(R.id.rbxProgressBtnProgressContainer);

        mButton.setText(DEFAULT_BUTTON_TEXT);
        mProgressText.setText(DEFAULT_PROGRESS_TEXT);

        RbxFontHelper.setCustomFont(mProgressText, getContext(), attrs);
        mProgressText.setTextSize(TypedValue.COMPLEX_UNIT_PX, a.getDimension(R.styleable.RbxProgressButton_progressTextSize, DEFAULT_PROGRESS_TEXT_SIZE));
        mProgressText.setTextColor(a.getColor(R.styleable.RbxProgressButton_progressTextColor, DEFAULT_PROGRESS_TEXT_COLOR));

        RbxFontHelper.setCustomFont(mButton, getContext(), attrs);
        mButton.setTextSize(TypedValue.COMPLEX_UNIT_PX, a.getDimension(R.styleable.RbxProgressButton_buttonTextSize, DEFAULT_BUTTON_TEXT_SIZE));
        mButton.setTextColor(a.getColor(R.styleable.RbxProgressButton_buttonTextColor, DEFAULT_BUTTON_TEXT_COLOR));

        mButton.setContentDescription(getContentDescription());

        mContainer.setVisibility(INVISIBLE);

        ripple = new RbxRipple(this, attrs);
        ripple.setStartColor(a.getColor(R.styleable.RbxProgressButton_rippleStartColor, getResources().getColor(R.color.RbxGray4)));

        // Need to set both backgrounds to ensure a seamless transition
        mButton.setBackgroundResource(a.getResourceId(R.styleable.RbxProgressButton_backgroundResource, R.drawable.rbx_drawable_flat_button_progress_blue));
        this.setBackgroundResource(a.getResourceId(R.styleable.RbxProgressButton_backgroundResource, R.drawable.rbx_drawable_flat_button_progress_blue));

        setButtonClickListener();

        a.recycle();
    }


    public STATE getCurrentState() { return mViewState; }

    public void toggleState(STATE_COMMAND newState) {
        switch (newState) {
            case SHOW_BUTTON:
                animateToDefaultButton();
                break;
            case SHOW_PROGRESS:
                animateToDefaultProgress();
                break;
        }
    }

    public void toggleState(STATE_COMMAND newState, int newStringId) {
        switch (newState) {
            case SHOW_BUTTON:
                animateToCustomButton(newStringId);
                break;
            case SHOW_PROGRESS:
                animateToCustomProgress(newStringId);
                break;
        }
    }

    /**
     * Change the ProgressButton to the newState. If newState is SHOW_BUTTON, newString will
     * replace the existing button string. If newState is SHOW_PROGRESS, newString will replace
     * the existing progress string.
     * @param newState
     * @param newString
     */
    public void toggleState(STATE_COMMAND newState, String newString) {
        switch (newState) {
            case SHOW_BUTTON:
                animateToCustomButton(newString);
                break;
            case SHOW_PROGRESS:
                animateToCustomProgress(newString);
                break;
        }
    }

    private void animateToCustomButton(String newString) {
        mButton.setText(newString);
        animateToButton();
    }

    private void animateToCustomButton(int newStringId) {
        mButton.setText(newStringId);
        animateToButton();
    }

    private void animateToDefaultButton() {
        mButton.setText(DEFAULT_BUTTON_TEXT);
        animateToButton();
    }

    private void animateToButton() {
        if (mViewState == STATE.PROGRESS) {
            mViewState = STATE.ANIMATING;

            final AlphaAnimation buttonReveal = new AlphaAnimation(0.0f, 1.0f);
            buttonReveal.setDuration(DEFAULT_REVEAL_DURATION);
            buttonReveal.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) { // this is the final code to be run
                    mViewState = STATE.BUTTON;
                    setButtonClickListener();

                    processQueuedCommand();
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });

            AlphaAnimation progressHide = new AlphaAnimation(1.0f, 0.0f);
            progressHide.setDuration(DEFAULT_HIDE_DURATION);
            mContainer.startAnimation(progressHide);

            progressHide.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    mContainer.setVisibility(INVISIBLE);
                    mButton.setVisibility(VISIBLE);

                    mButton.startAnimation(buttonReveal);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });
        }
        else if (mViewState == STATE.ANIMATING) {
            queueStateChange(STATE_COMMAND.SHOW_BUTTON);
        }
    }

    private void queueStateChange(STATE_COMMAND newCommand) {
        mQueuedCommand = newCommand;
    }

    private void processQueuedCommand() {
        if (mQueuedCommand != null) {
            switch (mQueuedCommand) {
                case SHOW_BUTTON:
                    animateToButton();
                    break;
                case SHOW_PROGRESS:
                    animateToProgress();
                    break;
            }
            mQueuedCommand = null; // so we don't repeat
        }
    }


    private void animateToCustomProgress(int newStringId) {
        if (mViewState == STATE.PROGRESS)
            mNewProgressText = getResources().getString(newStringId);
        else
            mProgressText.setText(newStringId);

        if (mNewProgressText == null) mNewProgressText = getResources().getString(R.string.Working);

        animateToProgress();
    }

    private void animateToCustomProgress(String newString) {
        if (mViewState == STATE.PROGRESS)
            mNewProgressText = newString;
        else
            mProgressText.setText(newString);

        animateToProgress();
    }

    private void animateToDefaultProgress() {
        if (mViewState == STATE.PROGRESS)
            mNewProgressText = DEFAULT_PROGRESS_TEXT;
        else
            mProgressText.setText(DEFAULT_PROGRESS_TEXT);

        animateToProgress();
    }

    private void animateToProgress() {
        if (mViewState == STATE.BUTTON) {
            mViewState = STATE.ANIMATING;

            final AlphaAnimation progressReveal = new AlphaAnimation(0.0f, 1.0f);
            progressReveal.setDuration(DEFAULT_REVEAL_DURATION);
            progressReveal.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) { // this is the final code to be run
                    mViewState = STATE.PROGRESS;
                    setContainerClickListener();

                    processQueuedCommand();
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });

            AlphaAnimation buttonHide = new AlphaAnimation(1.0f, 0.0f);
            buttonHide.setDuration(DEFAULT_HIDE_DURATION);
            mButton.startAnimation(buttonHide);

            buttonHide.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    mButton.setVisibility(INVISIBLE);
                    mContainer.setVisibility(VISIBLE);
                    mContainer.startAnimation(progressReveal);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });
        }
        else if (mViewState == STATE.PROGRESS) { // Only animate the ProgressText
            mViewState = STATE.ANIMATING;

            final AlphaAnimation textReveal = new AlphaAnimation(0.0f, 1.0f);
            textReveal.setDuration(DEFAULT_REVEAL_DURATION);
            textReveal.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    mViewState = STATE.PROGRESS;
                    setContainerClickListener();

                    processQueuedCommand();
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });

            AlphaAnimation textHide = new AlphaAnimation(1.0f, 0.0f);
            textHide.setDuration(DEFAULT_HIDE_DURATION);
            mProgressText.startAnimation(textHide);

            textHide.setAnimationListener(new Animation.AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {

                }

                @Override
                public void onAnimationEnd(Animation animation) {
                    mProgressText.setText(mNewProgressText != null ? mNewProgressText : DEFAULT_PROGRESS_TEXT);
                    mNewProgressText = null; // clear for next animation
                    mProgressText.startAnimation(textReveal);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {

                }
            });
        }
        else if (mViewState == STATE.ANIMATING) {
            if (mProgressText != null && mNewProgressText != null && !mNewProgressText.equals(mProgressText)) // skip animation if we're transitioning to the same text
                queueStateChange(STATE_COMMAND.SHOW_PROGRESS);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        ripple.onTouch(event);
        return super.onTouchEvent(event);
    }


    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        ripple.draw(canvas);
    }

    public void setOnRbxClickedListener(OnRbxClicked l) {
        mOnRbxClickedListener = l;
    }

    private void setButtonClickListener() {
        // just to make sure we don't fire on the hidden elements
        mContainer.setOnClickListener(null);
        mProgressText.setOnClickListener(null);

        mButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mOnRbxClickedListener != null) {
                    mOnRbxClickedListener.onClick(v);
                }
            }
        });
    }

    private void setContainerClickListener() {
        // just to make sure we don't fire on the hidden elements
        mButton.setOnClickListener(null);

        mContainer.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mOnRbxClickedListener != null) {
                    mOnRbxClickedListener.onClick(v);
                }
            }
        });

        mProgressText.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mOnRbxClickedListener != null) {
                    mOnRbxClickedListener.onClick(v);
                }
            }
        });
    }
}
