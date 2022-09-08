package com.roblox.client;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

public class RbxKeyboard extends EditText {

	long currentTextBox;
	
	public RbxKeyboard(Context context) {
		super(context);
	}
	public RbxKeyboard(Context context, AttributeSet attrs)
    {
        super(context, attrs);
    }
	
	public void setCurrentTextBox(long newTextBox)
	{
		currentTextBox = newTextBox;
	}

	@Override
	public boolean onKeyPreIme(int keyCode, KeyEvent event) {
		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && 
				event.getAction() == KeyEvent.ACTION_UP) {
			
			ActivityGlView.releaseFocus(currentTextBox);
			currentTextBox = 0;
			
			this.setVisibility(View.GONE);
	    	this.setText("");
	    	
	    	InputMethodManager imm = (InputMethodManager) this.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
	    	imm.hideSoftInputFromWindow(this.getWindowToken(), 0);

		}
		return super.dispatchKeyEvent(event);
	}
}