package com.roblox.client;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.widget.ImageView;

import com.roblox.client.components.RbxTextView;

import java.lang.ref.WeakReference;

public class WelcomeAnimation {
    private static WelcomeAnimationListener mListener = null;
    private static float startingTop, startingBottom = 0.0f;
    private static View mViewRoot = null;
    private static RbxTextView topLine = null;
    private static ImageView bottomLine = null;

    /*
        Gets the screen to the first key frame (just the red patterened background).
        Intended to be called from a different activity than the rest of the animation, hence
        the separate method.
     */
    public static void fadeInBackground(final Activity activity, final WelcomeAnimationListener listener) {
        if (!AndroidAppSettings.EnableWelcomeAnimation()) {
            if (listener != null)
                listener.onAnimationFinished();

            return;
        }

        final ViewGroup vg = (ViewGroup) (activity.getWindow().getDecorView().getRootView());
        LayoutInflater inflater = LayoutInflater.from(activity);
        inflater.inflate(R.layout.welcome_animation, vg);

        ValueAnimator anim = ValueAnimator.ofFloat(0f, 1.0f);
        anim.setDuration(600);
        anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                vg.setAlpha((Float) animation.getAnimatedValue());
            }
        });
        anim.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                if (listener != null)
                    listener.onAnimationFinished();
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        anim.setInterpolator(new AccelerateInterpolator());
        anim.start();


    }

    /*
        Initializes everything the animation will need through it's life.
        Gets to second key frame - both lines meeting in the center of the screen
     */
    public static void start(final Activity activity, final WelcomeAnimationListener listener) {
        if (!AndroidAppSettings.EnableWelcomeAnimation()) {
            if (listener != null)
                listener.onAnimationFinished();

            return;
        }

        mListener = listener;

        ViewGroup vg = (ViewGroup) (activity.getWindow().getDecorView().getRootView());
        LayoutInflater inflater = LayoutInflater.from(activity);
        inflater.inflate(R.layout.welcome_animation, vg);
        mViewRoot = vg;
        topLine = (RbxTextView) mViewRoot.findViewById(R.id.fragment_create_username_overlay_first);
        bottomLine = (ImageView) mViewRoot.findViewById(R.id.fragment_create_username_overlay_second);
        Handler mUIThreadHandler = new Handler(Looper.getMainLooper());

        // I have no idea why, but this must be run on the main thread
        // if it's not, mViewRoot has an empty rect, so all the positions are wrong
        mUIThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                // both lines start in the center of the screen, need to move them off screen
                // before animation starts
                startingTop = topLine.getX() + mViewRoot.getWidth();
                startingBottom = bottomLine.getX() - mViewRoot.getWidth();
                topLine.setX(startingTop);
                bottomLine.setX(startingBottom);

                ValueAnimator anim = ValueAnimator.ofFloat(0, mViewRoot.getWidth());
                anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        float newX = startingTop + -((Float) animation.getAnimatedValue());
                        float newX2 = startingBottom + (Float) animation.getAnimatedValue();
                        topLine.setX(newX);
                        bottomLine.setX(newX2);
                    }
                });
                anim.addListener(new Animator.AnimatorListener() {
                    @Override
                    public void onAnimationStart(Animator animation) {

                    }

                    @Override
                    public void onAnimationEnd(Animator animation) {
                        animateSlowSpread();
                    }

                    @Override
                    public void onAnimationCancel(Animator animation) {

                    }

                    @Override
                    public void onAnimationRepeat(Animator animation) {

                    }
                });


                anim.setDuration(900);
                anim.setInterpolator(new AccelerateDecelerateInterpolator());
                anim.start();
                topLine.setVisibility(View.VISIBLE);
                bottomLine.setVisibility(View.VISIBLE);

            }
        });
    }

    /*
        Both lines start from the center and slowly move apart, in the same direction they
        originally entered the screen
     */
    private static void animateSlowSpread() {
        ValueAnimator animWaiting = ValueAnimator.ofFloat(0, mViewRoot.getWidth() / 8);
        final float newStartingTop = topLine.getX();
        final float newStartingBottom = bottomLine.getX();
        animWaiting.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float newX = newStartingTop - (Float) animation.getAnimatedValue();
                float newX2 = newStartingBottom + (Float) animation.getAnimatedValue();
                topLine.setX(newX);
                bottomLine.setX(newX2);
            }
        });
        animWaiting.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                animateSlideOutOfView();
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        animWaiting.setDuration(3500);
        animWaiting.setInterpolator(new LinearInterpolator());
        animWaiting.start();
    }

    private static void animateSlideOutOfView() {
        ValueAnimator animOffScreen = ValueAnimator.ofFloat(0, mViewRoot.getWidth());
        final float newStartingTop = topLine.getX();
        final float newStartingBottom = bottomLine.getX();
        animOffScreen.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float newX = newStartingTop - (Float) animation.getAnimatedValue();
                float newX2 = newStartingBottom + (Float) animation.getAnimatedValue();
                topLine.setX(newX);
                bottomLine.setX(newX2);
            }
        });
        animOffScreen.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                fadeOut();
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        animOffScreen.setDuration(500);
        animOffScreen.setInterpolator(new LinearInterpolator());
        animOffScreen.start();
    }

    private static void fadeOut() {
        ValueAnimator anim = ValueAnimator.ofFloat(1.0f, 0);
        final View internalView = mViewRoot.findViewById(R.id.fragment_create_username_overlay_text_container);
        if (internalView == null) {
            internalView.setVisibility(View.GONE);
            return;
        }
        anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                internalView.setAlpha((Float) animation.getAnimatedValue());
            }
        });
        anim.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {

            }

            @Override
            public void onAnimationEnd(Animator animation) {
                internalView.setVisibility(View.GONE);
                if (mListener != null) {
                    mListener.onAnimationFinished();
                    cleanup();
                }
            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        anim.setDuration(400);
        anim.setInterpolator(new AccelerateInterpolator());
        anim.start();
    }

    private static void cleanup() {
        mListener = null;
        mViewRoot = null;
        topLine = null;
        bottomLine = null;
    }
}
