package com.roblox.client;

import android.content.Intent;
import android.os.Bundle;

import com.jirbo.adcolony.AdColony;
import com.jirbo.adcolony.AdColonyAd;
import com.jirbo.adcolony.AdColonyAdListener;
import com.jirbo.adcolony.AdColonyVideoAd;


public class ActivityAdColony extends RobloxActivity implements AdColonyAdListener {

    @Override
    public void onCreate(Bundle savedInstance) {
        super.onCreate(savedInstance);
        if (savedInstance != null)
            finish();
    }

    @Override
    protected void onStart() {
        super.onStart();

        AdColony.resume(this);
        this.showVideoAd();
    }
    
    @Override
    protected void onPause() {
        super.onPause();
		
        AdColony.pause();
    }

	private void showVideoAd() {
    	AdColonyVideoAd ad = new AdColonyVideoAd().withListener(this);
    	ad.show();
	}

	@Override
	public void onAdColonyAdStarted(AdColonyAd ad) {
		
	}
	
	@Override
	public void onAdColonyAdAttemptFinished(AdColonyAd ad) {
        Intent intent = new Intent(this, ActivityGlView.class);
        intent.putExtra("shown", ad.shown());
        setResult(1, intent);
		finish();
	}
}

