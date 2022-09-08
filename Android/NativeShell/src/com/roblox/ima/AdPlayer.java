package com.roblox.ima;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.RelativeLayout;

import com.google.ads.interactivemedia.v3.api.player.ContentProgressProvider;
import com.google.ads.interactivemedia.v3.api.player.VideoAdPlayer;
import com.google.ads.interactivemedia.v3.api.player.VideoProgressUpdate;
import com.roblox.client.ActivityGlView;
import com.roblox.client.R;

import java.util.ArrayList;
import java.util.List;

public class AdPlayer extends RelativeLayout {
    private String TAG = "RbxIMA_AdPlayer";

    public interface OnContentCompleteListener {
        public void onContentComplete();
    }

    private VideoPlayer mVideoPlayer;
    private ViewGroup mAdUiContainer;
    private boolean mIsAdDisplayed;
    private OnContentCompleteListener mOnContentCompleteListener;
    private AdPlayer mAdPlayer;
    private VideoAdPlayer mVideoAdPlayer;
    private ContentProgressProvider mContentProgressProvider;
    private final List<VideoAdPlayer.VideoAdPlayerCallback> mAdCallbacks = new ArrayList<VideoAdPlayer.VideoAdPlayerCallback>(1);

    public AdPlayer(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public AdPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public AdPlayer(Context context){
        super(context);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        init();
    }

    private void init()
    {
        mIsAdDisplayed = false;
        mVideoPlayer = (VideoPlayer)this.getRootView().findViewById(R.id.videoPlayer);
        mAdUiContainer = (ViewGroup)this.getRootView().findViewById(R.id.adUiContainer);

        mVideoAdPlayer = new VideoAdPlayer() {
            @Override
            public void playAd() {
                mIsAdDisplayed = true;
                mVideoPlayer.play();
            }

            @Override
            public void loadAd(String url) {
                mIsAdDisplayed = true;
                mVideoPlayer.setVideoPath(url);
            }

            @Override
            public void stopAd() { mVideoPlayer.stopPlayback(); }

            @Override
            public void pauseAd() { mVideoPlayer.pause(); }

            @Override
            public void resumeAd() { playAd(); }

            @Override
            public void addCallback(VideoAdPlayerCallback videoAdPlayerCallback) {
                mAdCallbacks.add(videoAdPlayerCallback);
            }

            @Override
            public void removeCallback(VideoAdPlayerCallback videoAdPlayerCallback) {
                mAdCallbacks.remove(videoAdPlayerCallback);
            }

            @Override
            public VideoProgressUpdate getAdProgress() {
                if (!mIsAdDisplayed || mVideoPlayer.getDuration() <= 0)
                    return VideoProgressUpdate.VIDEO_TIME_NOT_READY;

                return new VideoProgressUpdate(mVideoPlayer.getCurrentPosition(), mVideoPlayer.getDuration());
            }
        };

        mContentProgressProvider = new ContentProgressProvider() {
            @Override
            public VideoProgressUpdate getContentProgress() {
                if (mIsAdDisplayed || mVideoPlayer.getDuration() <= 0)
                    return VideoProgressUpdate.VIDEO_TIME_NOT_READY;

                return new VideoProgressUpdate(mVideoPlayer.getCurrentPosition(), mVideoPlayer.getDuration());
            }
        };

        mVideoPlayer.addPlayerCallback(new VideoPlayer.PlayerCallback() {
            @Override
            public void onPlay() {
                if (mIsAdDisplayed) {
                    for (VideoAdPlayer.VideoAdPlayerCallback callback : mAdCallbacks) {
                        callback.onPlay();
                    }
                }
            }

            @Override
            public void onPause() {
                if (mIsAdDisplayed) {
                    for (VideoAdPlayer.VideoAdPlayerCallback callback : mAdCallbacks) {
                        callback.onPause();
                    }
                }
            }

            @Override
            public void onResume() {
                if (mIsAdDisplayed) {
                    for (VideoAdPlayer.VideoAdPlayerCallback callback : mAdCallbacks) {
                        callback.onResume();
                    }
                }
            }

            @Override
            public void onError() {
                if (mIsAdDisplayed) {
                    for (VideoAdPlayer.VideoAdPlayerCallback callback : mAdCallbacks) {
                        callback.onError();
                    }
                }
            }

            @Override
            public void onCompleted() {
                if (mIsAdDisplayed) {
                    for (VideoAdPlayer.VideoAdPlayerCallback callback : mAdCallbacks) {
                        callback.onEnded();
                    }
                } else {
                    // Alert an external listener that our content video is complete.
                    if (mOnContentCompleteListener != null) {
                        mOnContentCompleteListener.onContentComplete();
                    }
                }
            }
        });
    }

    public void pauseContentForAdPlayback() {
        mVideoPlayer.disablePlaybackControls();
        mVideoPlayer.stopPlayback();
    }

    /**
     * Resume the content video from its previous playback progress position after
     * an ad finishes playing. Re-enables the media controller.
     */
    public void resumeContentAfterAdPlayback() {
        mIsAdDisplayed = false;
        ActivityGlView.removeGoogleAd();
    }

    /**
     * Returns the UI element for rendering video ad elements.
     */
    public ViewGroup getAdUiContainer() {
        return mAdUiContainer;
    }

    /**
     * Returns an implementation of the SDK's VideoAdPlayer interface.
     */
    public VideoAdPlayer getVideoAdPlayer() {
        return mVideoAdPlayer;
    }

    public ContentProgressProvider getContentProgressProvider() {
        return mContentProgressProvider;
    }


}