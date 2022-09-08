package com.roblox.ima;

import com.google.ads.interactivemedia.v3.api.AdDisplayContainer;
import com.google.ads.interactivemedia.v3.api.AdErrorEvent;
import com.google.ads.interactivemedia.v3.api.AdEvent;
import com.google.ads.interactivemedia.v3.api.AdsLoader;
import com.google.ads.interactivemedia.v3.api.AdsManager;
import com.google.ads.interactivemedia.v3.api.AdsManagerLoadedEvent;
import com.google.ads.interactivemedia.v3.api.AdsRequest;
import com.google.ads.interactivemedia.v3.api.CompanionAdSlot;
import com.google.ads.interactivemedia.v3.api.ImaSdkFactory;
import com.google.ads.interactivemedia.v3.api.ImaSdkSettings;
import com.roblox.client.ActivityGlView;

import android.content.Context;
import android.util.Log;
import android.view.View;

import java.util.ArrayList;

public class AdPlayerController {

    private AdDisplayContainer mAdDisplayContainer;
    private AdsLoader mAdsLoader; // used to request and load ads
    private AdsManager mAdsManager; // controls ad playback and listens to events
    private ImaSdkFactory mImaSdkFactory;
    private AdPlayer mAdPlayer; // the video player that will play our ads
    private String mCurrentAdTagUrl;
    private boolean mIsAdPlaying;
    private String TAG = "RbxIMA_Controller";

    private class AdsLoadedListener implements AdsLoader.AdsLoadedListener {

        private String TAG = "AdsLoadedListener";

        @Override
        public void onAdsManagerLoaded(AdsManagerLoadedEvent adsManagerLoadedEvent) {
            Log.i(TAG, "in loader");
            mAdsManager = adsManagerLoadedEvent.getAdsManager();
            mAdsManager.addAdErrorListener(new AdErrorEvent.AdErrorListener() {
                @Override
                public void onAdError(AdErrorEvent adErrorEvent) {
                    Log.i(TAG, "Error loading ad: " + adErrorEvent.getError());
                    resumeContent();
                }
            });

            mAdsManager.addAdEventListener(new AdEvent.AdEventListener() {
                @Override
                public void onAdEvent(AdEvent adEvent) {
                    Log.i(TAG, "Ad event: " + adEvent.getType());

                    switch (adEvent.getType()) {
                        case LOADED:
                            mAdsManager.start();
                            break;
                        case CONTENT_PAUSE_REQUESTED:
                            pauseContent();
                            break;
                        case CONTENT_RESUME_REQUESTED:
                            resumeContent();
                            break;
                        case PAUSED:
                            mIsAdPlaying = false;
                            break;
                        case RESUMED:
                            mIsAdPlaying = true;
                            break;
                        case ALL_ADS_COMPLETED:
                            if (mAdsManager != null) {
                                mAdsManager.destroy();
                                mAdsManager = null;
                            }
                            resumeContent();
                            break;
                        default:
                            break;
                    }
                }
            });

            mAdsManager.init();
        }
    }

    public AdPlayerController(Context context, AdPlayer adPlayer, String language) {
        mAdPlayer = adPlayer;
        mIsAdPlaying = false;

        ImaSdkSettings imaSdkSettings = new ImaSdkSettings();
        imaSdkSettings.setLanguage(language);
        mImaSdkFactory = ImaSdkFactory.getInstance();
        mAdsLoader = mImaSdkFactory.createAdsLoader(context, imaSdkSettings);

        mAdsLoader.addAdErrorListener(new AdErrorEvent.AdErrorListener() {
            @Override
            public void onAdError(AdErrorEvent adErrorEvent) {
                Log.i(TAG, "Error loading ad: " + adErrorEvent.getError());
                resumeContent();
            }
        });

        mAdsLoader.addAdsLoadedListener(new AdsLoadedListener());


    }

    private void pauseContent() {
        mAdPlayer.pauseContentForAdPlayback();
        mIsAdPlaying = true;
    }

    private void resumeContent() {
        mAdPlayer.resumeContentAfterAdPlayback();
        mIsAdPlaying = false;
    }


    public void requestAndPlayAds() {
        if (mCurrentAdTagUrl == null || mCurrentAdTagUrl == "") {
            Log.i(TAG, "No VAST ad tag URL specified");
            resumeContent();
            return;
        }

        // Since we're switching to a new video, tell the SDK the previous video is finished.
        if (mAdsManager != null) {
            mAdsManager.destroy();
        }
        mAdsLoader.contentComplete();

        mAdDisplayContainer = mImaSdkFactory.createAdDisplayContainer();
        mAdDisplayContainer.setPlayer(mAdPlayer.getVideoAdPlayer());
        mAdDisplayContainer.setAdContainer(mAdPlayer.getAdUiContainer());

        // Create the ads request.
        AdsRequest request = mImaSdkFactory.createAdsRequest();
        request.setAdTagUrl(mCurrentAdTagUrl);
        request.setAdDisplayContainer(mAdDisplayContainer);
        request.setContentProgressProvider(mAdPlayer.getContentProgressProvider());

        // Request the ad. After the ad is loaded, onAdsManagerLoaded() will be called.
        mAdsLoader.requestAds(request);
    }

    public void setAdTagUrl(String adTagUrl) {
        mCurrentAdTagUrl = adTagUrl;
    }

}
