package com.roblox.client.components;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ArgbEvaluator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.renderscript.Sampler;
import android.support.v4.view.animation.LinearOutSlowInInterpolator;
import android.util.AttributeSet;
import android.util.Log;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.widget.LinearLayout;

import com.roblox.client.R;
import com.roblox.client.managers.NotificationManager;

public class RbxLinearLayout extends LinearLayout {
    private RbxRipple mRipple = null;

    public RbxLinearLayout(Context context) {
        super(context);
        mRipple = new RbxRipple(this, null);
    }

    public RbxLinearLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mRipple = new RbxRipple(this, attrs);
        mRipple.setStartColor(getResources().getColor(R.color.RbxGreen2));
    }

    public RbxLinearLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mRipple = new RbxRipple(this, attrs);
        mRipple.setStartColor(getResources().getColor(R.color.RbxGreen2));
    }

    public float getXFraction() {
        return getX() / getWidth();
    }

    public void setXFraction(float xFraction) {
        final int width = getWidth();
        setX((width > 0) ? (xFraction * width) : -9999);
    }

    public float getYFraction() {
        return getY() / getHeight();
    }

    public void setYFraction(float yFraction) {
        final int height = getHeight();
        setY((height > 0) ? (yFraction * height) : -9999);
    }

    private float mRadius = 20;
    private boolean mWipeActive = false;
    private float cx, cy;
    public void startWipe(float x, float y) {
        curColor = getResources().getColor(R.color.RbxGreen2);
        mWipeActive = true;
        cx = x;
        cy = y;
        //mRipple.manualStartRipple();
        ValueAnimator mRadiusAnimation = ValueAnimator.ofFloat(mRadius, 2000); // okay to lose precision
        mRadiusAnimation.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                mRadius = (Float) animation.getAnimatedValue();
                invalidate();
            }
        });
        mRadiusAnimation.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                ValueAnimator colorAnim = ValueAnimator.ofObject(new ArgbEvaluator(), curColor, getResources().getColor(R.color.RbxRed1));
                colorAnim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        curColor = (Integer) animation.getAnimatedValue();
                        invalidate();
                    }
                });
                colorAnim.addListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        super.onAnimationEnd(animation);
                        NotificationManager.getInstance().postNotification(NotificationManager.EVENT_TEST);
                    }
                });
                colorAnim.setDuration(300);
                colorAnim.setInterpolator(new AccelerateInterpolator());
                colorAnim.start();
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        mRadiusAnimation.setDuration(1000);
        mRadiusAnimation.setInterpolator(new AccelerateInterpolator());
        mRadiusAnimation.start();


    }

    private Integer curColor = 0;
    @Override
    public void draw(Canvas canvas) {
        if (mWipeActive) {
            Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setColor(curColor);
            //paint.setAlpha(100);
            canvas.drawCircle(cx, cy, mRadius, paint);
        }
        super.draw(canvas);

//        mRipple.draw(canvas);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
    }
}
