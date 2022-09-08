package com.roblox.ima;

import android.app.DialogFragment;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import com.roblox.client.R;
import com.roblox.client.RobloxSettings;

public class FragmentVideo extends DialogFragment {
    private AdPlayerController mVideoPlayerController;
    private LinearLayout mVideoExampleLayout;
    private String mGoogleAdUrl = null;
    private final String TAG = "RbxIMA_FragmentVideo";
    @Override
    public void onActivityCreated(Bundle bundle) {
        super.onActivityCreated(bundle);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_video, container, false);
        initUi(rootView);

        Bundle args = getArguments();
        if (args != null)
            mGoogleAdUrl = args.getString("GoogleUrl");

        return rootView;
    }

    public void loadVideo() {
        String url = ((mGoogleAdUrl == null) || (mGoogleAdUrl.equals("")) ? getResources().getString(R.string.GoogleAdTagUrl) : mGoogleAdUrl);
        Log.i(TAG, "mGoogleAdUrl: " + mGoogleAdUrl + " | url: " + url);
        mVideoPlayerController.setAdTagUrl(url);
        mVideoPlayerController.requestAndPlayAds();
    }

    private void initUi(View rootView) {
        AdPlayer mVideoPlayerWithAdPlayback = (AdPlayer)
                rootView.findViewById(R.id.videoPlayerWithAdPlayback);
        //ViewGroup companionAdSlot = (ViewGroup) rootView.findViewById(R.id.companionAdSlot);

        mVideoPlayerController = new AdPlayerController(this.getActivity(), mVideoPlayerWithAdPlayback, "en");
    }

    /**
     * Shows or hides all non-video UI elements to make the video as large as possible.
     */
    public void makeFullscreen(boolean isFullscreen) {
        for (int i = 0; i < mVideoExampleLayout.getChildCount(); i++) {
            View view = mVideoExampleLayout.getChildAt(i);
            // If it's not the video element, hide or show it, depending on fullscreen status.
            if (view.getId() != R.id.videoContainer) {
                if (isFullscreen) {
                    view.setVisibility(View.GONE);
                } else {
                    view.setVisibility(View.VISIBLE);
                }
            }
        }
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
        loadVideo();
    }

}
