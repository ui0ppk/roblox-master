package com.roblox.client.components;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.widget.TextView;

import com.roblox.client.R;

import java.util.Hashtable;

public class RbxFontHelper {

    private static final String DEFAULT_FONT = "SourceSansPro-Light.ttf";

    // Sets a font for our view based on the com.roblox.client:font attribute
    // Note: font is optional, defaults to SourceSansPro
    public static void setCustomFont(TextView view, Context context, AttributeSet attr) {
        TypedArray a = context.obtainStyledAttributes(attr, R.styleable.RbxFont);
        String font = a.getString(R.styleable.RbxFont_fontName);
        if (font == null)
            font = DEFAULT_FONT;
        setCustomFont(view, context, font);
        a.recycle();
    }

    public static void setCustomFont(TextView view, Context context, String font) {
        if (font != null) {
            Typeface tf = RbxFontCache.get(font, context);
            if (tf != null)
                view.setTypeface(tf);
        }
    }

    static class RbxFontCache {
        private static String TAG = "RbxFontCache";
        private static Hashtable<String, Typeface> fontCache = new Hashtable<String, Typeface>();

        public static Typeface get(String name, Context context) {
            Typeface tf = fontCache.get(name);
            if (tf == null)
            {
                try {
                    tf = Typeface.createFromAsset(context.getAssets(), "fonts/" + name);
                    fontCache.put(name, tf);
                }
                catch (Exception e) {
                    return null;
                }
            }
            return tf;
        }

    }
}
