package com.roblox.client;

import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.widget.ImageView;

class ImageViewHttpLoader extends AsyncTask<String, Void, Bitmap> {
	public interface TaskTracking
	{
		// If the current index changes from when submitted the task is considered
		// late and aborted.
		public int getCurrentIndex();

		@SuppressWarnings("rawtypes")
		public void setDone( AsyncTask task );
	}
	
    public ImageView mImageView = null;
    public String mCacheKey = null;
    private TaskTracking mLateTaskCheck = null;
    public int mLateTaskCheckIndex = -1;

    // Central registry for controlling cached images.  UI thread only.
	static private Map<String, Bitmap> mBitmapCache = new HashMap<String, Bitmap>();
    
    public ImageViewHttpLoader(ImageView v, String key ) {
    	mImageView = v;
        mCacheKey = key;
    }

    // A mechanism to check if applying the bitmap to the ImageView is still relevant.
    public ImageViewHttpLoader(ImageView v, String key, TaskTracking lateTaskCheck, int lateTaskCheckIndex) {
    	mImageView = v;
        mCacheKey = key;
        mLateTaskCheck = lateTaskCheck;
        mLateTaskCheckIndex = lateTaskCheckIndex;
    }
    
	static public Bitmap checkBitmapCache(String key) {
		return mBitmapCache.get(key);
    }
	
    protected Bitmap doInBackground(String... urls) {
        Bitmap bm = null;
        try {
            InputStream s = new java.net.URL(urls[0]).openStream();
            bm = BitmapFactory.decodeStream(s);
        } catch (Exception e) {
        	// This actually crashed twice in the wild.
        	//   NullPointerException (@ImageViewHttpLoader:doInBackground:55) {AsyncTask #2}
        	// Log.e("ImageViewHttpLoader", e.getMessage());
        }
        return bm;
    }

    protected void onPostExecute(Bitmap result) {
    	if( mCacheKey != null )
    	{
    		mBitmapCache.put(mCacheKey, result);
    	}

    	if( mLateTaskCheck == null )
    	{
    		mImageView.setImageBitmap(result);
    		return;
    	}
    	
    	if( mLateTaskCheck.getCurrentIndex() == mLateTaskCheckIndex )
		{
    		mImageView.setImageBitmap(result);
    		mLateTaskCheck.setDone( this ); // release self.
		}
    }
}
