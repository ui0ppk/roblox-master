package com.roblox.client.components;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.roblox.client.R;

public class RbxGenderPicker extends LinearLayout {

    public RbxGenderPicker(Context context) {
        super(context);
        init(context);
    }

    public RbxGenderPicker(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
        RbxFontHelper.setCustomFont((TextView)findViewById(R.id.rbxGenderText), context, attrs);
    }

    private RbxButton mBtnMale, mBtnFemale = null;
    private ImageView mBtnMaleBg, mBtnFemaleBg = null;
    private LinearLayout mContainer = null;
    private int mValue = 0;

    private void init (Context c) {
        LayoutInflater.from(c).inflate(R.layout.rbx_gender_picker, (ViewGroup)(this.getRootView()));

        mBtnMale = (RbxButton)findViewById(R.id.rbxGenderBtnMale);
        mBtnFemale = (RbxButton)findViewById(R.id.rbxGenderBtnFemale);
        mBtnMaleBg = (ImageView)findViewById(R.id.rbxGenderBtnMaleBg);
        mBtnFemaleBg = (ImageView)findViewById(R.id.rbxGenderBtnFemaleBg);
        mContainer = (LinearLayout)findViewById(R.id.rbxGenderContainer);

        this.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                resetGender();
            }
        });

        mBtnMale.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mValue != 1) {
                    mBtnMaleBg.setImageResource(R.drawable.icon_male_on);
                    mBtnFemaleBg.setImageResource(R.drawable.icon_female);
                    mValue = 1;

                    mContainer.setBackgroundResource(R.drawable.rbx_bg_gender_full_success);
                }
                else resetGender();
            }
        });

        mBtnFemale.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mValue != 2) {
                    mBtnMaleBg.setImageResource(R.drawable.icon_male);
                    mBtnFemaleBg.setImageResource(R.drawable.icon_female_on);
                    mValue = 2;

                    mContainer.setBackgroundResource(R.drawable.rbx_bg_gender_full_success);
                }
                else resetGender();
            }
        });
    }

    private void resetGender() {
        mValue = 0;
        mBtnFemaleBg.setImageResource(R.drawable.icon_female);
        mBtnMaleBg.setImageResource(R.drawable.icon_male);
        mContainer.setBackgroundResource(R.drawable.rbx_bg_gender_full);
    }

    public void setError() { mContainer.setBackgroundResource(R.drawable.rbx_bg_gender_full_error); }

    public int getValue() { return mValue; }

    public void lock() {
        AlphaAnimation alpha = RbxAnimHelper.lockAnimation(this);
        this.startAnimation(alpha);
        View.OnTouchListener consumeTouch = new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                return true;
            }
        };

        mBtnFemale.setOnTouchListener(consumeTouch);
        mBtnMale.setOnTouchListener(consumeTouch);
    }

    public void unlock() {
        AlphaAnimation alpha = RbxAnimHelper.unlockAnimation(this);
        this.startAnimation(alpha);

        mBtnFemale.setOnTouchListener(null);
        mBtnMale.setOnTouchListener(null);
    }
}
