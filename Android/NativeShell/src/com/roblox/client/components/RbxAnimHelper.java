package com.roblox.client.components;

import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;

public class RbxAnimHelper {

    private static final float lockUnlockStartAlpha = 1.0f;
    private static final float lockUnlockToAlpha = 0.35f;
    private static final int lockUnlockDuration = 200;

    private static AlphaAnimation standardFieldLockUnlock(final View field, final float startingAlpha, final float endAlpha, final int time) {
        AlphaAnimation alpha = new AlphaAnimation(startingAlpha, endAlpha);
        alpha.setDuration(time);
        alpha.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation animation) {
                field.setAlpha(startingAlpha);
            }

            @Override
            public void onAnimationEnd(Animation animation) {
                field.setAlpha(endAlpha);
            }

            @Override
            public void onAnimationRepeat(Animation animation) {

            }
        });

        return alpha;
    }

    public static AlphaAnimation lockAnimation(final View field) {
        return standardFieldLockUnlock(field, lockUnlockStartAlpha, lockUnlockToAlpha, lockUnlockDuration);
    }

    public static AlphaAnimation unlockAnimation(final View field) {
        return standardFieldLockUnlock(field, lockUnlockToAlpha, lockUnlockStartAlpha, lockUnlockDuration);
    }

    public static AlphaAnimation fadeOut(final View field, int duration) {
        return standardFieldLockUnlock(field, 1.0f, 0.0f, duration);
    }

    public static AlphaAnimation fadeIn(final View field, int duration) {
        return standardFieldLockUnlock(field, 0.0f, 1.0f, duration);
    }
}
