package com.roblox.client;

import android.graphics.Point;
import android.view.MotionEvent;

public class ScaleGestureDetector 
{
    private static final int INVALID_POINTER_ID = -1;
    
    private Point initFinger1Pos = new Point();
    private Point initFinger2Pos = new Point();
    private Point currFinger1Pos = new Point();
    private Point currFinger2Pos = new Point();
    
    private int ptrID1, ptrID2;
    private int mState = -1;
    private float mScale = 1.0f;
    
    private OnScaleGestureListener mListener;
    
    public int getState() {
    	return mState;
    }
    
    public float getScale() {
    	return mScale;
    }
    
    public Point getPoint1() {
    	if(ptrID1 != INVALID_POINTER_ID)
    	{
    		return currFinger1Pos;
    	}

    	return new Point();
    }
    
    public Point getPoint2() {
    	if(ptrID2 != INVALID_POINTER_ID)
    	{
    		return currFinger2Pos;
    	}
    	
    	return new Point();
    }

    public ScaleGestureDetector(OnScaleGestureListener listener){
        mListener = listener;
        mState = -1;
        mScale = 1.0f;
        ptrID1 = INVALID_POINTER_ID;
        ptrID2 = INVALID_POINTER_ID;
    }

    public boolean onTouchEvent(MotionEvent event){
    	try {
	        switch (event.getActionMasked()) {
	            case MotionEvent.ACTION_DOWN:
	            	if (ptrID1 == INVALID_POINTER_ID)
	            	{
	            		ptrID1 = event.getPointerId(event.getActionIndex());
	            		ptrID2 = INVALID_POINTER_ID;
	            		mState = 0;
	            	}
	                break;
	            case MotionEvent.ACTION_POINTER_DOWN:
	            	if (ptrID2 == INVALID_POINTER_ID)
	            	{
		                ptrID2 = event.getPointerId(event.getActionIndex());
		                
		                initFinger1Pos.x = (int) event.getX(event.findPointerIndex(ptrID1));
		                initFinger1Pos.y = (int) event.getY(event.findPointerIndex(ptrID1));
		                
		                initFinger2Pos.x = (int) event.getX(event.findPointerIndex(ptrID2));
		                initFinger2Pos.y = (int) event.getY(event.findPointerIndex(ptrID2));
		                
		                
		                
		                currFinger1Pos.x = (int) event.getX(event.findPointerIndex(ptrID1));
		                currFinger1Pos.y = (int) event.getY(event.findPointerIndex(ptrID1));
		                
		                currFinger2Pos.x = (int) event.getX(event.findPointerIndex(ptrID2));
		                currFinger2Pos.y = (int) event.getY(event.findPointerIndex(ptrID2));
		                
		                mState = 0;
		                
		                if (ptrID1 != INVALID_POINTER_ID)
		                	mListener.OnScale(this);
	            	}
	                break;
	            case MotionEvent.ACTION_MOVE:
	                if(ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID){
	                	mState = 1;
	                	
	                	currFinger1Pos.x = (int) event.getX(event.findPointerIndex(ptrID1));
	                	currFinger1Pos.y = (int) event.getY(event.findPointerIndex(ptrID1));
		               
	                	currFinger2Pos.x = (int) event.getX(event.findPointerIndex(ptrID2));
	                	currFinger2Pos.y = (int) event.getY(event.findPointerIndex(ptrID2));
	
	                    scaleBetweenPoints();
	
	                    if (mListener != null) {
	                        mListener.OnScale(this);
	                    }
	                }
	                break;
	            case MotionEvent.ACTION_UP:
	            	mState = 2;
	            	if(ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID){
	            		mListener.OnScale(this);
	            	}
	            	mScale = 1.0f;
	                ptrID1 = INVALID_POINTER_ID;
	                break;
	            case MotionEvent.ACTION_POINTER_UP:
	            	mState = 2;
	            	if(ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID){
	            		mListener.OnScale(this);
	            	}
	            	mScale = 1.0f;
	                ptrID2 = INVALID_POINTER_ID;
	                break;
	        }
    	}
    	catch( IllegalArgumentException e )
    	{
    		// Very rare crash in Google Analytics.  Probably some sketchy device feeding in bad events.
    	}
        return true;
    }

    private void scaleBetweenPoints()
    {
    	float initXDiff = initFinger2Pos.x - initFinger1Pos.x;
    	float initYDiff = initFinger2Pos.y - initFinger1Pos.y;
    	
    	double initLength = Math.sqrt( Math.pow(initXDiff, 2) + Math.pow(initYDiff, 2) );
    	
    	float currXDiff = currFinger2Pos.x - currFinger1Pos.x;
    	float currYDiff = currFinger2Pos.y - currFinger1Pos.y;
    	
    	double currLength = Math.sqrt( Math.pow(currXDiff, 2) + Math.pow(currYDiff, 2) );
    	
    	if (initLength != 0.0f)
    		mScale = (float) (currLength/initLength);
    	else
    		mScale = 1.0f;

    }

    public static interface OnScaleGestureListener {
        public void OnScale(ScaleGestureDetector rotationDetector);
    }
}