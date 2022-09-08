package com.roblox.client;

import java.util.ArrayList;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;


public class InternalBuildLayout extends LinearLayout {
	
	//private static final String TAG = "internalBuildView";
    
	InternalBuildLayout(Context context) {
		super(context);
		
		this.setGravity(Gravity.BOTTOM);
		
		// Drop down menu for server selection
		{
			Spinner spinner = new Spinner(context);
			ArrayList<String> spinnerArray = getServerURLs();
			ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_dropdown_item, spinnerArray);
			spinner.setAdapter(spinnerArrayAdapter);
			spinner.setOnItemSelectedListener( new OnItemSelectedListener() {
	            public void onItemSelected(AdapterView<?> parent, View view, int position, long i)  {
	            	RobloxSettings.setBaseUrlDebug( parent.getItemAtPosition(position).toString() );
	            }
	
				@Override
				public void onNothingSelected(AdapterView<?> parent) {}
			});
			this.addView(spinner);
		}
	}
	
	private ArrayList<String> getServerURLs() {
		ArrayList<String> array = new ArrayList<String>();
		
		boolean isTablet = RobloxSettings.isTablet();
	    String m = isTablet ? "" : "m.";
	    String www = isTablet ? "www." : "m.";
	    
	    // PROD
	    array.add( Utils.format("%sroblox.com/", www) );
		   
		// Web Test Env
	    array.add( String.format("%ssitetest1.robloxlabs.com/", www) );
	    array.add( String.format("%ssitetest2.robloxlabs.com/", www) );
	    array.add( String.format("%ssitetest3.robloxlabs.com/", www) );
	    array.add( String.format("%ssitetest4.robloxlabs.com/", www) );
		
		// Web Personal Test Env
	    array.add( String.format("%sallen.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%santhony.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%sguru.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%smedhora.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%srosemary.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%ssairam.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%sshannon.sitetest3.robloxlabs.com/", m) );
	    array.add( String.format("%svlad.sitetest3.robloxlabs.com/", m) );
		
		// Client Test Env
	    array.add( String.format("%sgametest5.robloxlabs.com/", www) );
	    array.add( String.format("%sgametest4.robloxlabs.com/", www) );
	    array.add( String.format("%sgametest3.robloxlabs.com/", www) );
	    array.add( String.format("%sgametest2.robloxlabs.com/", www) );
	    array.add( String.format("%sgametest1.robloxlabs.com/", www) );

	    return array;
	}
}

