package com.roblox.client.components;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.support.v4.view.animation.FastOutLinearInInterpolator;
import android.support.v4.view.animation.LinearOutSlowInInterpolator;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.DecelerateInterpolator;

import com.roblox.client.R;

public class RbxRipple {
    private View mParentView = null;

    // cx = latest center X coord of ripple
    // cy = latest center Y coord of ripple
    // radius = current radius of ripple
    private float cx, cy, radius, startRadius;
    // mStartAlpha = current mStartAlpha of ripple
    private int mStartAlpha, mEndAlpha, mAlpha, mExpandDuration, mFadeDuration, mStartColor, mColor, mEndColor;
    private boolean rippleActive = false;
    private ValueAnimator mRadiusAnimation = null;
    private ValueAnimator mAlphaAnimation = null;
    private ValueAnimator mColorAnimation = null;

    private Rect hitbox = null;

    public RbxRipple(View parent, AttributeSet attrs) {
        if (parent != null)
            mParentView = parent;
        else
            return;

        if (attrs != null) {
            TypedArray a = mParentView.getContext().obtainStyledAttributes(attrs, R.styleable.RbxRipple);
            radius = a.getFloat(R.styleable.RbxRipple_startingRadius, DEFAULT_STARTING_RADIUS);
            startRadius = radius;
            mExpandDuration = a.getInt(R.styleable.RbxRipple_expandDuration, DEFAULT_EXPAND_DURATION);
            mFadeDuration = a.getInt(R.styleable.RbxRipple_fadeDuration, DEFAULT_FADE_DURATION);
            mStartAlpha = a.getInt(R.styleable.RbxRipple_startAlpha, DEFAULT_START_ALPHA);
            mEndAlpha = a.getInt(R.styleable.RbxRipple_endAlpha, DEFAULT_END_ALPHA);
            mStartColor = a.getColor(R.styleable.RbxRipple_startingColor, DEFAULT_COLOR);
            mEndColor = a.getColor(R.styleable.RbxRipple_endColor, DEFAULT_COLOR);
            a.recycle();
        }
        else {
            radius = DEFAULT_STARTING_RADIUS;
            startRadius = radius;
            mExpandDuration = DEFAULT_EXPAND_DURATION;
            mFadeDuration = DEFAULT_FADE_DURATION;
            mStartAlpha = DEFAULT_START_ALPHA;
            mEndAlpha = DEFAULT_END_ALPHA;
            mStartColor = DEFAULT_COLOR;
            mEndColor = DEFAULT_COLOR;
        }
    }

    public void onTouch(MotionEvent event) {
        if (hitbox == null) {
            hitbox = new Rect();
            mParentView.getDrawingRect(hitbox);
        }

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                rippleActive = true;
                cx = event.getX();
                cy = event.getY();
                startRipple();
                break;
            case MotionEvent.ACTION_MOVE:
                // need to use raw coords to get world/device coordinates
                if (!isViewContains((int)event.getRawX(), (int)event.getRawY()))
                    endRipple();
                break;
            case MotionEvent.ACTION_UP:
                endRipple();
                break;
            case MotionEvent.ACTION_CANCEL:
                endRipple();
                break;
        }
    }

    private boolean isViewContains(int rx, int ry) {
        int[] l = new int[2];
        mParentView.getLocationInWindow(l);
        int x = l[0];
        int y = l[1];
        int w = mParentView.getWidth();
        int h = mParentView.getHeight();

        if (rx < x || rx > x + w || ry < y || ry > y + h) {
            return false;
        }
        return true;
    }

    public void draw(Canvas canvas) {
        if (rippleActive)
        {
            Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setColor(mColor);
            paint.setAlpha(mAlpha);
            canvas.drawCircle(cx, cy, radius, paint);
        }
    }

    private void startRipple() {
        if (anyAnimationRunning())
            cancelAllAnimations();
        radius = startRadius;
        mAlpha = mStartAlpha;
        mColor = mStartColor; // doesn't animate currently

        mRadiusAnimation = ValueAnimator.ofFloat(radius, calcEndRadius((int)cx, (int)cy)); // okay to lose precision
        mRadiusAnimation.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                radius = (Float) animation.getAnimatedValue();
                mParentView.invalidate();
            }
        });
        mRadiusAnimation.setDuration(mExpandDuration);
        mRadiusAnimation.setInterpolator(new LinearOutSlowInInterpolator());
        mRadiusAnimation.start();


        mAlphaAnimation = ValueAnimator.ofInt(mAlpha, mEndAlpha);
        mAlphaAnimation.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                mAlpha = (Integer) animation.getAnimatedValue();
            }
        });
        mAlphaAnimation.setDuration(mExpandDuration);
        mAlphaAnimation.setInterpolator(new FastOutLinearInInterpolator());
        mAlphaAnimation.start();
    }

    private void endRipple() {
        if (anyAnimationRunning())
            cancelAllAnimations();

        mAlphaAnimation = ValueAnimator.ofInt(mAlpha, 0);
        mAlphaAnimation.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                mAlpha = (Integer)animation.getAnimatedValue();
                mParentView.invalidate();
            }
        });

        mAlphaAnimation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                rippleActive = false;
                cancelAllAnimations();
            }
        });
        mAlphaAnimation.setDuration(mFadeDuration);
        mAlphaAnimation.setInterpolator(new DecelerateInterpolator());
        mAlphaAnimation.start();
    }

    private float calcEndRadius(int touchXPos, int touchYPos) {
        int xDif = 0, yDif = 0;

        if (hitbox != null) {
            if ((touchXPos - hitbox.left) > (hitbox.right - touchXPos))
                xDif = touchXPos - hitbox.left;
            else
                xDif = hitbox.right - touchXPos;

            if ((touchYPos - hitbox.top) > (hitbox.bottom - touchYPos))
                yDif = touchYPos - hitbox.top;
            else
                yDif = hitbox.bottom - touchYPos;
        }

        return (yDif > xDif ? yDif : xDif) + 300.0f; // add a buffer just to be safe
    }

    private void cancelAllAnimations() {
        if (mRadiusAnimation != null)
        {
            mRadiusAnimation.cancel();
            mRadiusAnimation.removeAllListeners();
        }
        if (mAlphaAnimation != null)
        {
            mAlphaAnimation.cancel();
            mAlphaAnimation.removeAllListeners();
        }
        if (mColorAnimation != null)
        {
            mColorAnimation.cancel();
            mColorAnimation.removeAllListeners();
        }
    }

    private boolean anyAnimationRunning() {
        return (mRadiusAnimation != null && mRadiusAnimation.isRunning())
                && (mAlphaAnimation != null && mAlphaAnimation.isRunning())
                && (mColorAnimation != null && mColorAnimation.isRunning());
    }

    public void setStartColor(int newStartColor) {
        mStartColor = newStartColor;
    }

    public void setEndColor(int newEndColor) {
        mEndColor = newEndColor;
    }

    public void manualStartRipple() {
        rippleActive = true;
        startRipple();
    }


    // Constants
    private final int DEFAULT_START_ALPHA = 60,
        DEFAULT_END_ALPHA = 10,
        DEFAULT_COLOR = 0xB8B8B8, // colors.xml:RbxGray3
        DEFAULT_EXPAND_DURATION = 3500,
        DEFAULT_FADE_DURATION = 300;
    private final float DEFAULT_STARTING_RADIUS = 50.0f;
}
