package com.roblox.client.components;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RelativeLayout;

/**
 * Created by Sam on 9/8/2015.
 */
public class RbxRelativeLayout extends RelativeLayout {
    public RbxRelativeLayout(Context context) {
        super(context);
    }

    public RbxRelativeLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
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
}
