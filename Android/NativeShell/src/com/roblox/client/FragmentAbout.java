package com.roblox.client;

import android.app.Fragment;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.roblox.client.managers.SessionManager;

public class FragmentAbout extends Fragment {
    public final static String FRAGMENT_TAG = "FragmentAbout";

	private View mBackgroundView = null;
	private View mDialogueView = null;
	private TextView mTermsLicenscingPrivacy = null;
	private TextView mUserAgent = null;
	private TextView mBaseUrl = null;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View view = null;
		if( RobloxSettings.isPhone() )
		{
			view = inflater.inflate(R.layout.fragment_about_phone, container, false);
		}
		else
		{
			view = inflater.inflate(R.layout.fragment_about, container, false);
		}		
		
		mBackgroundView = view.findViewById(R.id.fragment_about_background);
		mDialogueView = view.findViewById(R.id.fragment_about_dialog_bg);
		mTermsLicenscingPrivacy = (TextView)view.findViewById(R.id.fragment_about_terms_licenscing_privacy);
		mUserAgent = (TextView)view.findViewById(R.id.fragment_about_user_agent);
		mBaseUrl = (TextView)view.findViewById(R.id.fragment_about_baseURL);
		
		mBackgroundView.setOnClickListener(new View.OnClickListener() {
			public void onClick(View x) {
				closeDialog();
			}
		});

		// This prevents a click being sent to the mBackgroundView if it hit the mDialogueView first.
		if( mDialogueView != null )
		{
			mDialogueView.setOnClickListener(new View.OnClickListener() {
				public void onClick(View x) {
				}
			});
		}
		
		String res = getString(R.string.TermsLicensingPrivacy);
		Utils.makeTextViewHtml(getActivity(), mTermsLicenscingPrivacy, res);
		
		mUserAgent.setText( RobloxSettings.userAgent() );
	
		mBaseUrl.setText( RobloxSettings.baseUrl() );
	
		return view;
	}

	@Override
	public void onResume()
	{
		super.onResume();
		if( getActivity() != null )
		{
	        Utils.sendAnalyticsScreen( "AboutScreen" );
		}
	}

	public void closeDialog() {
		if (!SessionManager.getInstance().getIsLoggedIn())
			RbxAnalytics.fireButtonClick("about", "close");

		FragmentTransaction ft = getFragmentManager().beginTransaction();
		ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
		ft.remove(this);
		ft.commit();
	}
}

