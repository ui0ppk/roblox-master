package com.roblox.client.components;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

public class RbxTextView extends TextView {
    public RbxTextView(Context context) {
        super(context);
    }

    public RbxTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        RbxFontHelper.setCustomFont(this, context, attrs);

        //init(attrs);
    }

    public RbxTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        RbxFontHelper.setCustomFont(this, context, attrs);

        //init(attrs);
    }
}
