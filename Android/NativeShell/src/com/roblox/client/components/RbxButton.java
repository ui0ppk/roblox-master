package com.roblox.client.components;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.AdapterView;
import android.widget.Button;

import com.roblox.client.R;

public class RbxButton extends Button {
    RbxButton refThis = null;
    RbxRipple ripple = null;


    public RbxButton(Context context) {
        super(context);
        refThis = this;
    }

    public RbxButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        RbxFontHelper.setCustomFont(this, context, attrs);

        init(attrs);
    }

    public RbxButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        RbxFontHelper.setCustomFont(this, context, attrs);

        init(attrs);
    }

    private void init(AttributeSet attrs) {
        refThis = this;
        ripple = new RbxRipple(this, attrs);
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


}
