package com.roblox.ima;

import android.content.Context;
import android.media.MediaPlayer;
import android.util.AttributeSet;
import android.widget.MediaController;
import android.widget.VideoView;

import java.util.ArrayList;
import java.util.List;

public class VideoPlayerImpl extends VideoView implements VideoPlayer {
    private enum PlaybackState {
        STOPPED, PAUSED, PLAYING
    }

    private MediaController mMediaController;
    private PlaybackState mPlaybackState;
    private final List<PlayerCallback> mVideoPlayerCallbacks = new ArrayList<PlayerCallback>(1);

    public VideoPlayerImpl(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    public VideoPlayerImpl(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public VideoPlayerImpl(Context context) {
        super(context);
        init();
    }

    private void init() {
        mPlaybackState = PlaybackState.STOPPED;
        mMediaController = new MediaController(getContext());
        mMediaController.setAnchorView(this);
        enablePlaybackControls();

        // Set OnCompletionListener to notify our callbacks when the video is completed.
        super.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {

            @Override
            public void onCompletion(MediaPlayer mediaPlayer) {
                // Reset the MediaPlayer.
                // This prevents a race condition which occasionally results in the media
                // player crashing when switching between videos.
                disablePlaybackControls();
                mediaPlayer.reset();
                mediaPlayer.setDisplay(getHolder());
                enablePlaybackControls();
                mPlaybackState = PlaybackState.STOPPED;

                for (PlayerCallback callback : mVideoPlayerCallbacks) {
                    callback.onCompleted();
                }
            }
        });

        // Set OnErrorListener to notify our callbacks if the video errors.
        super.setOnErrorListener(new MediaPlayer.OnErrorListener() {

            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                mPlaybackState = PlaybackState.STOPPED;
                for (PlayerCallback callback : mVideoPlayerCallbacks) {
                    callback.onError();
                }

                // Returning true signals to MediaPlayer that we handled the error. This will
                // prevent the completion handler from being called.
                return true;
            }
        });
    }

    @Override
    public void setOnCompletionListener(MediaPlayer.OnCompletionListener listener) {
        // The OnCompletionListener can only be implemented by SampleVideoPlayer.
        throw new UnsupportedOperationException();
    }

    @Override
    public void setOnErrorListener(MediaPlayer.OnErrorListener listener) {
        // The OnErrorListener can only be implemented by SampleVideoPlayer.
        throw new UnsupportedOperationException();
    }

    // Methods implementing the VideoPlayer interface.
    @Override
    public void play() {
        start();
    }

    @Override
    public void start() {
        super.start();
        PlaybackState oldPlaybackState = mPlaybackState;
        mPlaybackState = PlaybackState.PLAYING;
        switch (oldPlaybackState) {
            case STOPPED:
                for (PlayerCallback callback : mVideoPlayerCallbacks) {
                    callback.onPlay();
                }
                break;
            case PAUSED:
                for (PlayerCallback callback : mVideoPlayerCallbacks) {
                    callback.onResume();
                }
                break;
            default:
                // Already playing; do nothing.
                break;
        }
    }

    @Override
    public void pause() {
        super.pause();
        mPlaybackState = PlaybackState.PAUSED;
        for (PlayerCallback callback : mVideoPlayerCallbacks) {
            callback.onPause();
        }
    }

    @Override
    public void stopPlayback() {
        super.stopPlayback();
        mPlaybackState = PlaybackState.STOPPED;
    }

    @Override
    public void disablePlaybackControls() {
        setMediaController(null);
    }

    @Override
    public void enablePlaybackControls() {
        setMediaController(mMediaController);
    }

    @Override
    public void addPlayerCallback(PlayerCallback callback) {
        mVideoPlayerCallbacks.add(callback);
    }

    @Override
    public void removePlayerCallback(PlayerCallback callback) {
        mVideoPlayerCallbacks.remove(callback);
    }
}
