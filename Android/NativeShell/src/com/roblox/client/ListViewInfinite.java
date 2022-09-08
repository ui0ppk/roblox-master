package com.roblox.client;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ListView;


public class ListViewInfinite extends ListView {
    public ListViewInfinite(Context context, AttributeSet attrs) {
        super(context, attrs );
    }

    public ListViewInfinite(Context context, AttributeSet attrs, int defStyle) {
    	super(context, attrs, defStyle);
    }

    // The normal onMeasure code wanted to create and then trash a lot of ImageViews every
    // frame just to determine the size of the list view.
    //
    // Which generated bogus HTTP traffic for the Games Page.
    //
    // The idea here is to just tell the layout code that the list view will take all available
    //space.  
    
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);
        setMeasuredDimension(widthSize , heightSize);
    }
}




























