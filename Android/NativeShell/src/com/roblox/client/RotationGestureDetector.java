package com.roblox.client;

import android.graphics.Point;
import android.view.MotionEvent;

public class RotationGestureDetector 
{
    private static final int INVALID_POINTER_ID = -1;
    private Point lastFinger1Pos = new Point();
    private Point lastFinger2Pos = new Point();
    private Point lastVector = new Point();
    private int ptrID1, ptrID2;
    private float mAngle = 0.0f;
    private int mState;
    
    private OnRotationGestureListener mListener;

    public float getAngle() {
        return mAngle;
    }
    
    public int getState() {
    	return mState;
    }
    
    public Point getPoint1() {
    	if(ptrID1 != INVALID_POINTER_ID)
    	{
    		return lastFinger1Pos;
    	}

    	return new Point();
    }
    
    public Point getPoint2() {
    	if(ptrID2 != INVALID_POINTER_ID)
    	{
    		return lastFinger2Pos;
    	}
    	
    	return new Point();
    }

    public RotationGestureDetector(OnRotationGestureListener listener){
        mListener = listener;
        mState = -1;
        ptrID1 = INVALID_POINTER_ID;
        ptrID2 = INVALID_POINTER_ID;
    }

    public boolean onTouchEvent(MotionEvent event){
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
            	if (ptrID1 == INVALID_POINTER_ID)
            	{
            		ptrID1 = event.getPointerId(event.getActionIndex());
            		ptrID2 = INVALID_POINTER_ID;
            		mState = 0;
            		mAngle = 0;
            	}
                break;
            case MotionEvent.ACTION_POINTER_DOWN:
            	if (ptrID2 == INVALID_POINTER_ID)
            	{
                    if (AndroidAppSettings.EnableRotationGestureFix()) {
                        ptrID2 = event.getPointerId(event.getActionIndex());

                        int ptrInd1 = event.findPointerIndex(ptrID1);
                        int ptrInd2 = event.findPointerIndex(ptrID2);

                        if (ptrInd1 != -1 && ptrInd1 < event.getPointerCount()) {
                            lastFinger1Pos.x = (int) event.getX(event.findPointerIndex(ptrID1));
                            lastFinger1Pos.y = (int) event.getY(event.findPointerIndex(ptrID1));
                        }

                        if (ptrInd2 != -1 && ptrInd2 < event.getPointerCount()) {
                            lastFinger2Pos.x = (int) event.getX(event.findPointerIndex(ptrID2));
                            lastFinger2Pos.y = (int) event.getY(event.findPointerIndex(ptrID2));
                        }

                        lastVector.x = (int) (lastFinger2Pos.x - lastFinger1Pos.x);
                        lastVector.y = (int) (lastFinger2Pos.y - lastFinger1Pos.y);
                    } else {
                        ptrID2 = event.getPointerId(event.getActionIndex());

                        lastFinger1Pos.x = (int) event.getX(event.findPointerIndex(ptrID1));
                        lastFinger1Pos.y = (int) event.getY(event.findPointerIndex(ptrID1));

                        lastFinger2Pos.x = (int) event.getX(event.findPointerIndex(ptrID2));
                        lastFinger2Pos.y = (int) event.getY(event.findPointerIndex(ptrID2));

                        lastVector.x = (int) (lastFinger2Pos.x - lastFinger1Pos.x);
                        lastVector.y = (int) (lastFinger2Pos.y - lastFinger1Pos.y);

                        if (ptrID1 != INVALID_POINTER_ID)
                            mListener.OnRotation(this);
                    }

	                
	                if (ptrID1 != INVALID_POINTER_ID)
	                	mListener.OnRotation(this);
            	}
                break;
            case MotionEvent.ACTION_MOVE:
                if(ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID){
                	mState = 1;

                    if (AndroidAppSettings.EnableRotationGestureFix()) {
                        int ptrInd1 = event.findPointerIndex(ptrID1);
                        int ptrInd2 = event.findPointerIndex(ptrID2);
                        float nsX = -1, nsY = -1, nfX = -1, nfY = -1;

                        if (ptrInd1 != -1 && ptrInd1 < event.getPointerCount()) {
                            nsX = event.getX(event.findPointerIndex(ptrID1));
                            nsY = event.getY(event.findPointerIndex(ptrID1));
                        }

                        if (ptrInd2 != -1 && ptrInd2 < event.getPointerCount()) {
                            nfX = event.getX(event.findPointerIndex(ptrID2));
                            nfY = event.getY(event.findPointerIndex(ptrID2));
                        }

                        if (nfX != -1 && nfY != -1 && nsX != -1 && nsY != -1)
                            angleBetweenLines(nfX, nfY, nsX, nsY);
                    } else {
                        float nsX = event.getX(event.findPointerIndex(ptrID1));
                        float nsY = event.getY(event.findPointerIndex(ptrID1));
                        float nfX = event.getX(event.findPointerIndex(ptrID2));
                        float nfY = event.getY(event.findPointerIndex(ptrID2));

                        angleBetweenLines(nfX, nfY, nsX, nsY);
                    }

                    if (mListener != null) {
                        mListener.OnRotation(this);
                    }
                }
                break;
            case MotionEvent.ACTION_UP:
            	mState = 2;
            	if(ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID){
            		mListener.OnRotation(this);
            	}
            	mAngle = 0.0f;
                ptrID1 = INVALID_POINTER_ID;
                break;
            case MotionEvent.ACTION_POINTER_UP:
            	mState = 2;
            	if(ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID){
            		mListener.OnRotation(this);
            	}
            	mAngle = 0.0f;
                ptrID2 = INVALID_POINTER_ID;
                break;
        }
        return true;
    }

    private void angleBetweenLines (float nfX, float nfY, float nsX, float nsY)
    {
    	Point newVector = new Point();
    	newVector.x = (int) (nfX - nsX);
    	newVector.y = (int) (nfY - nsY);
    	
    	lastVector.x = (int) (lastFinger2Pos.x - lastFinger1Pos.x);
    	lastVector.y = (int) (lastFinger2Pos.y - lastFinger1Pos.y);
    	
    	float dotProductDelta = (lastVector.x * newVector.x) + (lastVector.y * newVector.y);
    	
    	double angleDelta = Math.atan2( ((newVector.x * lastVector.y) - (newVector.y * lastVector.x )),
    			dotProductDelta);
     
        lastFinger2Pos.x = (int) nfX;
        lastFinger2Pos.y = (int) nfY;
        
        lastFinger1Pos.x = (int) nsX;
        lastFinger1Pos.y = (int) nsY;
        
        mAngle -= (float) angleDelta;
    }

    public static interface OnRotationGestureListener {
        public void OnRotation(RotationGestureDetector rotationDetector);
    }
}