package com.roblox.client;

import android.app.Activity;
import android.app.DialogFragment;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.format.DateUtils;
import android.util.Log;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.roblox.client.components.OnRbxClicked;
import com.roblox.client.components.OnRbxDateChanged;
import com.roblox.client.components.OnRbxFocusChanged;
import com.roblox.client.components.RbxBirthdayPicker;
import com.roblox.client.components.RbxButton;
import com.roblox.client.components.RbxEditText;
import com.roblox.client.components.RbxGenderPicker;
import com.roblox.client.components.RbxProgressButton;
import com.roblox.client.components.RbxTextView;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpAgent.HttpHeader;
import com.roblox.client.managers.NotificationManager;
import com.roblox.client.managers.RbxReportingManager;
import com.roblox.client.managers.SessionManager;
import com.roblox.client.signup.SignUpApiTask;
import com.roblox.client.signup.SignUpResult;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class FragmentSignUp extends DialogFragment implements NotificationManager.Observer {
	
	private enum ValidationOp { USERNAME, PASSWORD };
	private enum Validation { BLANK, VALID, INVALID };

	private RbxButton mCancelButton = null;
	private EditText mUsernameEditText = null;
    private RbxEditText mUsernameField = null;
    private RbxEditText mPasswordField = null;
	private EditText mPasswordEditText = null;
	private EditText mPasswordVerifyEditText = null;
    private RbxEditText mPasswordVerifyField = null;
    private RbxGenderPicker mGenderPicker = null;
    private RbxBirthdayPicker mBirthdayPicker = null;
    private RbxEditText mEmailField = null;
	private EditText mEmailEditText = null;
	private RbxProgressButton mSignUpButton = null;
	private TextView mAgreementTextView = null;
    private RbxButton mLoginButton = null;
    private LinearLayout mCardBackground = null;

	// Read by getStringsFromUi
	private String mUsername = null;
	private String mPassword = null;	
	private String mPasswordVerify = null;
	private String mEmail = null;


	int mYear = -1, mMonth = -1, mDay = -1;
	boolean mDateSelected = false;

	private int mGender = 0;
	private boolean mIsCanceling = false;
	private Validation mUsernameValidation       = Validation.BLANK;
	private Validation mPasswordValidation       = Validation.BLANK;
	private Validation mPasswordVerifyValidation = Validation.BLANK;
	private Validation mEmailVerifyValidation    = Validation.BLANK;
    private Validation mBirthdayValidation       = Validation.INVALID;
	
	private String s3 = "Z^#q";
	private String s1 = "Fu.*mJ";
	private String s4 = "l%=f~RIW";
	private String s2 = "L65HQ,v?K";
	private String s5 = "hC39$";
	private String s7 = "qb@Wl";
	private String s8 = "Av=M";
	private String s10 = "B7YpO";
	private String s6 = "jEda0J~i";
	private String s9 = "HZmfcyG9,F";

    private int mDialogWidth = 0;

    private static String ctx = "signup"; // RbxAnalytics
    private View mViewRef = null;

    public static final String FRAGMENT_TAG = "signup_window";

    private long taskStartTime;

    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.setStyle(DialogFragment.STYLE_NO_TITLE, this.getTheme());
    }

    // -----------------------------------------------------------------------
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		//View cardContainer = new View(getActivity().getApplicationContext());
        View view, cardContainer, cardContents = null;
        LinearLayout swapContainer, innerContainer;

        int containerId = (RobloxSettings.isPhone() ? R.layout.fragment_sign_up_card_phone : R.layout.fragment_sign_up_card_tablet);

        view = inflater.inflate(R.layout.fragment_signup_new, container, false);
        swapContainer = (LinearLayout)view.findViewById(R.id.fragment_sign_up_swap_container);

        cardContainer = inflater.inflate(containerId, swapContainer);

        innerContainer = (LinearLayout)cardContainer.findViewById(R.id.fragment_sign_up_card_inner_container);
        cardContents = inflater.inflate(R.layout.fragment_sign_up_card_common, innerContainer);

        // This stops touches from falling through to the layout behind the signup form
        LinearLayout bg = (LinearLayout)view.findViewById(R.id.fragment_sign_up_background);
        bg.setOnClickListener(null);

		mCancelButton = (RbxButton)view.findViewById(R.id.fragment_sign_up_cancel_btn);
        mUsernameField = (RbxEditText)view.findViewById(R.id.fragment_sign_up_username);
		mUsernameEditText = mUsernameField.getTextBox();
        mPasswordField = (RbxEditText)view.findViewById(R.id.fragment_sign_up_password);
		mPasswordEditText = mPasswordField.getTextBox();
		mPasswordVerifyField = (RbxEditText)view.findViewById(R.id.fragment_sign_up_password_verify);
        mPasswordVerifyEditText = mPasswordVerifyField.getTextBox();
        mGenderPicker = (RbxGenderPicker)view.findViewById(R.id.fragment_sign_up_gender_picker);
        mBirthdayPicker = (RbxBirthdayPicker)view.findViewById(R.id.fragment_sign_up_birthday_picker);
        mEmailField = (RbxEditText)view.findViewById(R.id.fragment_sign_up_email);
        mEmailEditText = mEmailField.getTextBox();
		mSignUpButton = (RbxProgressButton)view.findViewById(R.id.fragment_sign_up_submit_btn);
        mSignUpButton.setVisibility(View.VISIBLE);
		mAgreementTextView = (TextView)view.findViewById(R.id.fragment_sign_up_agreement);
        mLoginButton = (RbxButton)view.findViewById(R.id.fragment_sign_up_login_btn);
        mCardBackground = (LinearLayout)view.findViewById(R.id.fragment_sign_up_inner_background);

        if(AndroidAppSettings.EnableLoginLogoutSignupWithApi()){
            // sign-up/v1 doesn't accept email field
            mEmailField.setVisibility(View.GONE);
        }

        mViewRef = view;

        // Should cause the username box to get focus and open the keyboard
        mUsernameEditText.requestFocus();
        Utils.showKeyboard(mViewRef, mUsernameEditText);

        mUsernameEditText.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        mUsernameEditText.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    mPasswordField.requestFocus();

                    return true;
                }

                return false;
            }
        });
        mUsernameField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!mIsCanceling && !hasFocus)
                    doValidationTask(ValidationOp.USERNAME);
            }
        });

        mPasswordEditText.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        mPasswordEditText.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    mPasswordVerifyField.requestFocus();

                    return true;
                }
                return false;
            }
        });
        mPasswordField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!mIsCanceling && !hasFocus)
                    doValidationTask(ValidationOp.PASSWORD);
            }
        });

        // email field is hidden when EnableLoginLogoutSignupWithApi is true
        mPasswordVerifyEditText.setImeOptions(AndroidAppSettings.EnableLoginLogoutSignupWithApi() ? EditorInfo.IME_ACTION_DONE : EditorInfo.IME_ACTION_NEXT);
        mPasswordVerifyEditText.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_NEXT) {
                    mEmailField.requestFocus();

                    return true;
                }
                else if (actionId == EditorInfo.IME_ACTION_DONE) {
                    mPasswordVerifyEditText.clearFocus();
                    Utils.hideKeyboard(mViewRef);
                    return true;
                }

                return false;
            }
        });
        mPasswordVerifyField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!mIsCanceling && !hasFocus)
                    doPasswordVerifyValidation(true);
            }
        });

        mEmailEditText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        mEmailEditText.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    mEmailEditText.clearFocus();

                    Utils.hideKeyboard(mViewRef);
                    return true;
                }

                return false;
            }
        });
        mEmailField.setRbxFocusChangedListener(new OnRbxFocusChanged() {
            @Override
            public void focusChanged(View v, boolean hasFocus) {
                if (!mIsCanceling && !hasFocus)
                    doEmailValidation();
            }
        });

        mBirthdayPicker.setRbxDateChangedListener(new OnRbxDateChanged() {
            @Override
            public void dateChanged(int field, int newValue) {
                switch(field) {
                    case OnRbxDateChanged.DAY:
                        mDay = newValue;
                        break;
                    case OnRbxDateChanged.MONTH:
                        mMonth = newValue;
                        break;
                    case OnRbxDateChanged.YEAR:
                        mYear = newValue;
                        break;
                }
                onDateSet();
            }
        });



        mLoginButton.setOnClickListener(new View.OnClickListener() {
                public void onClick(View x){
                switchToLogin();
        }
        });
//
//		mBackgroundView.setOnClickListener(new View.OnClickListener() {
//			public void onClick(View x) {
//			}
//		});
//
		mCancelButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View x) {
                mIsCanceling = true;
                RbxAnalytics.fireButtonClick(ctx, "close");
                closeDialog();
			}
		});

		mSignUpButton.setOnRbxClickedListener(new OnRbxClicked() {
            public void onClick(View v) {
                onSignUpClicked(true);
            }
        });

		Utils.makeTextViewHtml(getActivity(), mAgreementTextView, getString(R.string.SignupFinePrintAndroid));
        mAgreementTextView.setTextIsSelectable(false);

        Bundle args = getArguments();
        if (args != null) {
            mDialogWidth = args.getInt("dialogWidth", 0);
            if (args.getBoolean("isActivityMain")) {
                // we need to set the top padding to 0 so the 'SIGNUP' text doesn't get cut off
                View tCont = view.findViewById(R.id.fragment_sign_up_inner_background);
                RbxTextView signupText = (RbxTextView)view.findViewById(R.id.fragment_signup_header_txt);

                tCont.setPadding(tCont.getPaddingLeft(), tCont.getPaddingTop() / 2, tCont.getPaddingRight(), tCont.getPaddingBottom());

                float size = signupText.getTextSize() * 0.9f;
                signupText.setTextSize(TypedValue.COMPLEX_UNIT_SP, Utils.pixelToDp(size)); // need to convert back to dp
            }
        }

        RbxAnalytics.fireScreenLoaded(ctx);

		return view;
	}

    private void switchToLogin() {
        RbxAnalytics.fireButtonClick(ctx, "login");

        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_left, R.animator.slide_out_to_right);
        FragmentLogin fragment = new FragmentLogin();
        ft.hide(this);
        ft.replace(Utils.getCurrentActivityId(getActivity()), fragment, FragmentLogin.FRAGMENT_TAG);
        ft.commit();
    }

    // -----------------------------------------------------------------------
	private String computeHash(String input) throws NoSuchAlgorithmException, UnsupportedEncodingException{
	    MessageDigest digest = MessageDigest.getInstance("SHA-256");
	    digest.reset();

	    byte[] byteData = digest.digest(input.getBytes("UTF-8"));
	    StringBuffer sb = new StringBuffer();

	    for (int i = 0; i < byteData.length; i++){
	      sb.append(Integer.toString((byteData[i] & 0xff) + 0x100, 16).substring(1));
	    }
	    return sb.toString();
	}

    // -----------------------------------------------------------------------
	@Override
	public void onResume()
	{
		super.onResume();
		if( getActivity() != null )
		{
	        Utils.sendAnalyticsScreen( "AboutScreen" );
		}
	}

    // -----------------------------------------------------------------------
	@Override
	public void onDestroy() {
		super.onDestroy();
	}

	// ------------------------------------------------------------------------
	// UI callbacks

	public void onDateSet()
	{
        if(mYear == -1 || mMonth == -1 || mDay == -1){
            // incomplete birthday
            return;
        }
        else {
            mBirthdayPicker.clearError();
        }

		Calendar cal = new GregorianCalendar(mYear, mMonth, mDay);
		String date = DateUtils.formatDateTime( null, cal.getTimeInMillis(), DateUtils.FORMAT_SHOW_DATE|DateUtils.FORMAT_ABBREV_ALL );
		mDateSelected = true;

        //TODO: make birthday box green?

        Date d = new Date();
        long diff = (d.getTime() - cal.getTimeInMillis()) / 1000;
        long yearSec = 31556952000L / 1000;
        if (diff / yearSec < 13)
        {
            mEmailField.setHintText(R.string.EmailRequirementsUnder13);
            mEmailField.setLongHintText(R.string.SignupUnder13Warning);
            mEmailEditText.setText("");
            //Utils.alert(R.string.SignupUnder13Warning);
        }
        else
            mEmailField.setHintText(R.string.EmailWord);
	}

    private void setFieldError(RbxEditText field, int resId) {
        field.showErrorText(resId);
    }

    private void setFieldError(RbxEditText field, String error) {
        field.showErrorText(error);
    }

    private void setFieldSuccess(RbxEditText field, int resId) {
        field.showSuccessText(resId);
    }

    private void setFieldSuccess(RbxEditText field, String msg) {
        field.showSuccessText(msg);
    }


    // -----------------------------------------------------------------------
	public void onSignUpClicked(boolean isUserAction) {
        if(isUserAction) {
            RbxAnalytics.fireButtonClick(ctx, "submit");
        }
        getStringsFromUi();

        mGender = mGenderPicker.getValue();

        mSignUpButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.SignupValidating);
        lockFields();

        final Handler mUIThreadHandler = new Handler(Looper.getMainLooper());

        mUIThreadHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                String error = ""; // RbxAnalytics
                String field = "";
                if (mUsername.equals("")) {
                    error = "Empty";
                    field = "username";
                    setFieldError(mUsernameField, R.string.SignupNoUsername);

                }
                if (mPassword.equals("")) {
                    error = "Empty";
                    field = "password";
                    setFieldError(mPasswordField, R.string.SignupNoPassword);

                }

                if (!error.isEmpty() && !field.isEmpty()) {
                    RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
                    unlockFields();
                    return;
                }

                doPasswordVerifyValidation(true);
                doEmailValidation();
                doBirthdayValidation();

                if (mUsernameValidation != Validation.VALID
                        || mBirthdayValidation != Validation.VALID
                        || mPasswordValidation != Validation.VALID
                        || mPasswordVerifyValidation != Validation.VALID
                        || mEmailVerifyValidation == Validation.INVALID) {
                    unlockFields();
                    return;
                }
                mUIThreadHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mSignUpButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.RegisteringWord);
                        taskStartTime = System.currentTimeMillis();
                        if(AndroidAppSettings.EnableLoginLogoutSignupWithApi()) {
                            new SignUpApiTask(
                                    mGender,
                                    mYear,
                                    mMonth,
                                    mDay,
                                    mEmail,
                                    mUsername,
                                    mPassword,
                                    signUpApiListener).execute();
                        }
                        else {
                            new SignUpAsyncTask().execute();
                        }
                    }
                }, 1000);
            }
        }, 1000);

    }

    // -----------------------------------------------------------------------
	public void getStringsFromUi()
	{
		mUsername = mUsernameEditText.getText().toString();
		mPassword = mPasswordEditText.getText().toString();	
		mPasswordVerify = mPasswordVerifyEditText.getText().toString();
		mEmail = mEmailEditText.getText().toString();	
	}

    // -----------------------------------------------------------------------
	public void doValidationTask( ValidationOp op)
	{
		getStringsFromUi();
        String field = "";
        String error = "";
		switch( op )
		{
			case USERNAME:
                field = "username";
                if(AndroidAppSettings.EnableXBOXSignupRules()) {
                    if(mUsername.isEmpty()) {
                        setFieldError(mUsernameField, R.string.SignupNoUsername);
                        error = "Empty";
                    } else if(mUsername.length() < 3) {
                        setFieldError(mUsernameField, R.string.SignupXBOXUsernameTooShort);
                        error = "TooShort";
                    } else if(mUsername.length() > 20) {
                        setFieldError(mUsernameField, R.string.SignupXBOXUsernameTooLong);
                        error = "TooLong";
                    } else if(!Pattern.compile("([A-Z]|[a-z]|[0-9]|_)*").matcher(mUsername).matches()) {
                        setFieldError(mUsernameField, R.string.SignupXBOXUsernameInvalidCharacters);
                        error = "InvalidCharacters";
                    } else if( mUsername.charAt(0) == '_' || mUsername.charAt(mUsername.length() - 1) == '_' ) {
                        setFieldError(mUsernameField, R.string.SignupXBOXUsernameInvalidFirstOrLastCharacter);
                        error = "InvalidFirstOrLastCharacter";
                    } else if( mUsername.contains("__") ) {
                        setFieldError(mUsernameField, R.string.SignupXBOXUsernameInvalidDoubleUnderscore);
                        error = "InvalidUsernameDoubleUnderscore";
                    } else {
                        (new ValidationAsyncTask(op)).execute();
                    }
                } else {
                    if (mUsername.isEmpty()) {
                        setFieldError(mUsernameField, R.string.SignupNoUsername);
                        error = "Empty";
                    } else if (mUsername.length() < 3) {
                        //Utils.alertExclusively( R.string.UsernameTooShort );
                        setFieldError(mUsernameField, R.string.SignupUsernameTooShort);
                        error = "TooShort";
                    } else if (mUsername.length() > 20) {
                        //Utils.alertExclusively( R.string.UsernameTooLong );
                        setFieldError(mUsernameField, R.string.SignupUsernameTooLong);
                        error = "TooLong";
                    } else {
                        (new ValidationAsyncTask(op)).execute();
                    }
                }
				break;
			case PASSWORD:
                field = "password";
                if(AndroidAppSettings.EnableXBOXSignupRules()) {
                    if (mPassword.isEmpty()) {
                        setFieldError(mPasswordField, R.string.SignupNoPassword);
                        error = "Empty";
                    } else if (mUsername.equals(mPassword)) {
                        setFieldError(mPasswordField, R.string.SignupXBOXPasswordIsUsername);
                        error = "IsUsername";
                    } else if (mPassword.length() < 8) {
                        setFieldError(mPasswordField, R.string.SignupXBOXPasswordTooShort);
                        error = "TooShort";
                    } else {
                        (new ValidationAsyncTask(op)).execute();
                    }
                } else {
                    if (!mPassword.isEmpty()) {
                        (new ValidationAsyncTask(op)).execute();
                    } else {
                        setFieldError(mPasswordField, R.string.SignupNoPassword);
                        error = "Empty";
                    }
                }
				break;
		}

        if (!error.isEmpty())
            RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
	}

    // -----------------------------------------------------------------------
	public void doPasswordVerifyValidation(boolean warn)
	{
		getStringsFromUi();

        String field = "password";
        String error = "";
        if(AndroidAppSettings.EnableXBOXSignupRules()) {
            if (mPassword.isEmpty()) {
                setFieldError(mPasswordField, R.string.SignupNoPassword);
                error = "Empty";
                mPasswordVerifyValidation = Validation.BLANK;
            } else if (!mPasswordVerify.equals(mPassword)) {
                setFieldError(mPasswordVerifyField, R.string.SignupPasswordsMatch);
                error = "PasswordMismatch";
                mPasswordVerifyValidation = Validation.INVALID;
            } else {
                setFieldSuccess(mPasswordVerifyField, "");
                mPasswordVerifyValidation = Validation.VALID;
            }
        } else {
            if (mPassword.isEmpty()) {
                setFieldError(mPasswordVerifyField, R.string.SignupNoPassword);
                error = "Empty";
            } else if (mPasswordVerify.isEmpty()) {
                setFieldError(mPasswordVerifyField, R.string.SignupNoVerifyPassword);
                mPasswordVerifyValidation = Validation.BLANK;
                error = "Empty";
            } else if (mPassword.equals(mPasswordVerify)) {
                setFieldSuccess(mPasswordField, null);
                setFieldSuccess(mPasswordVerifyField, null);
                mPasswordVerifyValidation = Validation.VALID;
            } else if (mPassword.length() < 4) {
                setFieldError(mPasswordField, R.string.SignupPasswordTooShort);
                mPasswordVerifyValidation = Validation.INVALID;
                error = "TooShort";
            } else if (mPassword.length() > 20) {
                setFieldError(mPasswordField, R.string.SignupPasswordTooLong);
                mPasswordVerifyValidation = Validation.INVALID;
                error = "TooLong";
            } else {
                setFieldError(mPasswordVerifyField, R.string.SignupPasswordsMatch);
                mPasswordVerifyValidation = Validation.INVALID;
                error = "PasswordMismatch";
            }
        }

        if (!error.isEmpty())
            RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
	}

    public void doBirthdayValidation(){

        mBirthdayValidation = (mYear != -1 && mMonth != -1 && mDay != -1) ? Validation.VALID : Validation.INVALID;

        if(mBirthdayValidation != Validation.VALID){
            mBirthdayPicker.setError();

            RbxAnalytics.fireFormFieldValidation(ctx, "birthday", "incomplete", true);
        }
    }

    // -----------------------------------------------------------------------
	public void doEmailValidation()
    {
		getStringsFromUi();
        String field = "email";
		if( mEmail.isEmpty() )
		{
            mEmailVerifyValidation = Validation.BLANK;
			//setEmailValidation( Validation.BLANK );
            setFieldError(mEmailField, R.string.SignupEmailSuggestion);
            RbxAnalytics.fireFormFieldValidation(ctx, field, "Empty", true);
			return;
		}
		
		final Pattern p = Pattern.compile("[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}");
		Matcher m = p.matcher(mEmail);

		boolean matches = m.matches();
		mEmailVerifyValidation = ( matches ? Validation.VALID : Validation.INVALID );
		if( !matches )
		{
            RbxAnalytics.fireFormFieldValidation(ctx, field, "EmailInvalid", true);
			//Utils.alertExclusively( R.string.EmailInvalid );
            setFieldError(mEmailField, R.string.EmailInvalid);
		}
        else {
            mEmailVerifyValidation = Validation.VALID;
            setFieldSuccess(mEmailField, null);
            RbxAnalytics.fireFormFieldValidation(ctx, field, true);
        }
	}

    // -----------------------------------------------------------------------
    class ValidationAsyncTask extends AsyncTask<Void, Void, Void> {

    	ValidationOp mOp;
		String mUsernameLocal = null;
		String mPasswordLocal = null;	
    	String mResponse = null;
    	JSONObject mJson = null;
    	
    	ValidationAsyncTask( ValidationOp op )
    	{
    		mOp = op;
            String field = "";
            String error = "NotUTF8";
    		try {
                field = "username";
                mUsernameLocal = URLEncoder.encode(mUsername, "UTF-8");
            } catch (UnsupportedEncodingException e) {
                Utils.alertUnexpectedError( "Username contains invalid characters." );
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
                return;
			}

            try {
                field = "password";
                mPasswordLocal = URLEncoder.encode(mPassword, "UTF-8");
            } catch (UnsupportedEncodingException e) {
                Utils.alertUnexpectedError( "Password contains invalid characters" );
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
                return;
            }
    	}
 
        @Override
        protected Void doInBackground(Void... arg0) {
            String field = (mOp == ValidationOp.USERNAME ? "username" : "password");
            String error = "";
    		switch( mOp )
    		{
    			case USERNAME:
                    if(AndroidAppSettings.EnableXBOXSignupRules()) {
                        mResponse = HttpAgent.readUrlToString(RobloxSettings.usernameCheckUrlXBOX(mUsernameLocal), null, null);
                    } else {
                        mResponse = HttpAgent.readUrlToString(RobloxSettings.usernameCheckUrl(mUsernameLocal), null, null);
                    }
    				break;
    			case PASSWORD:
                    if(AndroidAppSettings.EnableXBOXSignupRules()) {
                        mResponse = HttpAgent.readUrlToString(RobloxSettings.passwordCheckUrlXBOX(mUsernameLocal, mPasswordLocal), null, null);
                    } else {
                        mResponse = HttpAgent.readUrlToString(RobloxSettings.passwordCheckUrl(), RobloxSettings.passwordCheckUrlArgs(mUsernameLocal, mPasswordLocal), null);
                    }
    				break;
    		}
   		 	if( mResponse != null )
   		 	{
   		 		try {
   		 			mJson = new JSONObject( mResponse );
   		 		} catch (JSONException e) {
                    error = "ValidationJSONException";
   		 		}
   		 	}
            else
                error = "NoResponse";

            if (!error.isEmpty())
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);

 			//Log.i(TAG, "JSON: " + mResponse );
   		 	return null;
        }

		@Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);

            String field = (mOp == ValidationOp.USERNAME ? "username" : "password");
            String error = "";

            if( getActivity() == null )
            {
                error = "WindowClosed";
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true); // for safety, we want to get out ASAP
            	return; // Task still running after Fragment has been removed.
            }
            
            if( mResponse == null )
            {
            	if( !Utils.alertIfNetworkNotConnected() )
            	{
    	            Utils.sendUnexpectedError( "ValidationAsyncTask cannot get response" );
            		Utils.alertExclusively( R.string.ConnectionError );
            	}
                error = "NoResponse";
            } else if( mJson == null )
            {
	            Utils.alertUnexpectedError( "ValidationAsyncTask cannot parse JSON #1" );
                error = "JSONParseFailure";
            }

            if (!error.isEmpty()) {
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
                return;
            }
            
			try {
				switch( mOp )
				{
				case USERNAME:
                    if( AndroidAppSettings.EnableXBOXSignupRules() ) {
                        boolean isUsernameValid = mJson.optBoolean("IsValid", false);
                        String errorMessage = mJson.optString("ErrorMessage", "");
                        if(isUsernameValid) {
                            mUsernameValidation = Validation.VALID;
                            setFieldSuccess(mUsernameField, null);
                        } else {
                            // Use errorMessage
                            if(errorMessage.contains("already in use")) {
                                (new UsernameSuggestionAsyncTask()).execute(); // the error/success message is handled by this AsyncTask
                            }
                            else {
                                // username is not valid and not in use, do not attempt suggestion (likely swear words)
                                mUsernameField.setTextBoxText("");
                                setFieldError(mUsernameField, R.string.UsernameExplicit);
                            }
                            error = "UsernameInvalidWeb";
                        }
                    } else {
                        if (mJson.getInt("data") != 0) {
                            (new UsernameSuggestionAsyncTask()).execute(); // the error/success message is handled by this AsyncTask
                            error = "UsernameInvalidWeb";
                        } else {
                            mUsernameValidation = Validation.VALID;
                            setFieldSuccess(mUsernameField, null);
                        }
                    }
					break;
				case PASSWORD:
                    if( AndroidAppSettings.EnableXBOXSignupRules() ) {
                        boolean isPasswordValid = mJson.optBoolean("IsValid", false);
                        String errorMessage = mJson.optString("ErrorMessage", "");
                        if(isPasswordValid) {
                            mPasswordValidation = Validation.VALID;
                            setFieldSuccess(mPasswordField, null);
                        } else {
                            mPasswordValidation = Validation.INVALID;
                            setFieldError(mPasswordField, errorMessage);
                            error = errorMessage;
                        }
                    } else {
                        if (mJson.getBoolean("success") != true) {
                            //Utils.alertExclusively( mJson.getString("error") );
                            setFieldError(mPasswordField, mJson.getString("error"));
                            mPasswordValidation = Validation.INVALID;
                            error = mJson.getString("error");
                        } else {
                            setFieldSuccess(mPasswordField, null);
                            mPasswordValidation = Validation.VALID;
                        }
                    }
					break;
				}
			} catch (JSONException e) {
	            Utils.alertUnexpectedError( "ValidationAsyncTask cannot parse JSON #2" );
                error = "JSONReadFailure";
    		}

            if (error.isEmpty())
                RbxAnalytics.fireFormFieldValidation(ctx, field, true);
            else
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
		}
    }

    // -----------------------------------------------------------------------
    class UsernameSuggestionAsyncTask extends AsyncTask<Void, Void, Void> {

		String mUsernameLocal = null;
    	String mResponse = null;
    	
    	UsernameSuggestionAsyncTask()
    	{
        	try {
        		mUsernameLocal = URLEncoder.encode(mUsername, "UTF-8");
			} catch (UnsupportedEncodingException e) {
                Utils.alertUnexpectedError( "Username contains illegal characters" );
                RbxAnalytics.fireFormFieldValidation(ctx, "usernameSuggest", "NotUTF8", true);
            	return; // Task still running after Fragment has been removed.
			}
    	}
 
        @Override
        protected Void doInBackground(Void... arg0) {
 	        mResponse = HttpAgent.readUrlToString( RobloxSettings.recommendUsernameUrl(mUsernameLocal), null, null );
   		 	return null;
        }

		@Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            String field = "username";
            String error = "";
            if( getActivity() == null )
            {
                RbxAnalytics.fireFormFieldValidation(ctx, field, "WindowClosed", true);
            	return; // Task still running after Fragment has been removed.
            }
            if( mResponse == null )
            {
            	if( !Utils.alertIfNetworkNotConnected() )
            	{
    	            Utils.sendUnexpectedError( "UsernameSuggestionAsyncTask cannot get response" );
            		Utils.alertExclusively( R.string.ConnectionError );
            	}
                error = "NoResponseSuggestion";
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
            	return;
            }

            // Apparently the website doesn't make up funny suggestions using swear words.
    		if( mUsernameLocal.equals( mResponse ) )
    		{
                setFieldError(mUsernameField, R.string.UsernameExplicit);
                //Utils.alertExclusively( R.string.UsernameExplicit );
                error = "UsernameNotAllowed";
    		}
    		else
    		{
                String format = getResources().getString(R.string.UsernameTaken);
                String s = Utils.format( format, mResponse );
                //Utils.alertExclusively( s );

                mUsernameField.setTextBoxText(mResponse);
                // Can set success because we're guaranteed that the username the web gives back is
                // available
                setFieldSuccess(mUsernameField, R.string.SignupUsernameTakenSuggestion);
                error = "UsernameTaken";
                doValidationTask(ValidationOp.USERNAME);
    		}

            if (error.isEmpty())
                RbxAnalytics.fireFormFieldValidation(ctx, field, true);
            else
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);


		}	
    }

    /**
     * SignUpAsyncTaskListener for SignUpApiTask (api proxy sign-up)
     */
    private com.roblox.client.signup.SignUpAsyncTask.SignUpAsyncTaskListener signUpApiListener = new SignUpResponseListener();

    class SignUpResponseListener implements com.roblox.client.signup.SignUpAsyncTask.SignUpAsyncTaskListener {

        @Override
        public void onSignUpPostExecuteSuccess(SignUpResult result){
            JSONObject json = null;
            try {
                json = new JSONObject(result.message);
            } catch (JSONException e) {}

            SessionManager.getInstance().setUsername(mUsername);
            SessionManager.getInstance().onLoginAfterApiLogin(json);

            RbxReportingManager.fireSignupSuccess(result.code);
        }

        @Override
        public void onSignUpPostExecuteFailed(SignUpResult result){

            if (result == null) {
                // Unknown error. AsyncTask is responsible for returning a non-null status for reporting
                Utils.alertUnexpectedError("Oops! Something went wrong.");
                result = new SignUpResult();
                result.reportingAction = RbxReportingManager.ACTION_F_UNKNOWNERROR;
            }
            else if(result.status.size() == 0 || result.status.get(0) == null){
                // Unknown error
                Utils.alertUnexpectedError("Oops! Something went wrong.");
                result.reportingAction = RbxReportingManager.ACTION_F_UNKNOWNERROR;
            }
            else {
                // Alert the user of the first error we have
                String status = result.status.get(0);
                if (status.equals(SignUpResult.StatusUsernameTaken)) {
                    setFieldError(mUsernameField, R.string.UsernameTaken);
                    result.reportingAction = RbxReportingManager.ACTION_F_ALREADYTAKEN;
                }
                else if (status.equals(SignUpResult.StatusUsernameContainsInvalidCharacters)) {
                    setFieldError(mUsernameField, R.string.InvalidCharactersUsed);
                    result.reportingAction = RbxReportingManager.ACTION_F_INVALIDCHAR;
                }
                else if (status.equals(SignUpResult.StatusUsernameCannotContainSpaces)) {
                    setFieldError(mUsernameField, R.string.UsernameCannotContainSpaces);
                    result.reportingAction = RbxReportingManager.ACTION_F_CONTAINSSPACE;
                }
                else if (status.equals(SignUpResult.StatusUsernameInvalid)) {
                    setFieldError(mUsernameField, R.string.UsernameInvalid);
                    result.reportingAction = RbxReportingManager.ACTION_F_INVALIDUSER;
                }
                else if (status.equals(SignUpResult.StatusBirthdayInvalid)) {
                    // Note: backend only returns BirthdayInvalid response if the date is in the future.
                    mBirthdayPicker.setError();
                    result.reportingAction = RbxReportingManager.ACTION_F_INVALIDBIRTHDAY;
                }
                else if (status.equals(SignUpResult.StatusGenderInvalid)) {
                    mGenderPicker.setError();
                    result.reportingAction = RbxReportingManager.ACTION_F_INVALIDGENDER;
                }
                else if (status.equals(SignUpResult.StatusPasswordInvalid)) {
                    setFieldError(mPasswordField, R.string.PasswordRequirements);
                    result.reportingAction = RbxReportingManager.ACTION_F_INVALIDPASS;
                }
                else if (status.equals(SignUpResult.StatusCaptcha)) {
                    Context context = getActivity();
                    if(context != null) {
                        Intent intent = new Intent(context, ReCaptchaActivity.class);
                        intent.putExtra(ReCaptchaActivity.USERNAME, mUsername);
                        intent.putExtra(ReCaptchaActivity.ACTION, ReCaptchaActivity.ACTION_SIGNUP);
                        startActivityForResult(intent, ReCaptchaActivity.ACTIVITY_REQUEST_CODE);
                    }
                    result.reportingAction = RbxReportingManager.ACTION_F_ACCOUNTCREATEFLOOD;
                }
                else if (status.equals(SignUpResult.StatusJsonError)) {
                    Utils.alertUnexpectedError("Bad response from server.");
                    result.reportingAction = RbxReportingManager.ACTION_F_JSONPARSE;
                }
                else if (status.equals(SignUpResult.StatusServerError)) {
                    Utils.alertUnexpectedError("Server error.");
                    result.reportingAction = RbxReportingManager.ACTION_F_ACCOUNTCREATEFLOOD;
                }
                else {
                    Utils.alertUnexpectedError("There was an error.");
                }
            }

            fireSignupFailure(result);

            unlockFields();
        }

        protected void fireSignupFailure(SignUpResult result) {

            Log.e(com.roblox.client.signup.SignUpAsyncTask.TAG, "FSU.fireSignupFailure()");
            Log.e(com.roblox.client.signup.SignUpAsyncTask.TAG, "     status:" + result.status);
            Log.e(com.roblox.client.signup.SignUpAsyncTask.TAG, "     action:" + result.reportingAction);
            Log.e(com.roblox.client.signup.SignUpAsyncTask.TAG, "     url:" + result.url);
            Log.e(com.roblox.client.signup.SignUpAsyncTask.TAG, "     code:" + result.code);
            Log.e(com.roblox.client.signup.SignUpAsyncTask.TAG, "     message:" + result.message);

            RbxReportingManager.fireSignupFailure(
                    result.reportingAction,
                    result.code,
                    result.url,
                    result.message,
                    mUsername,
                    System.currentTimeMillis() - taskStartTime);
        }
    };

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == ReCaptchaActivity.ACTIVITY_REQUEST_CODE && resultCode == Activity.RESULT_OK){
            // try to sign up again
            onSignUpClicked(false);
        }
    }

    // -----------------------------------------------------------------------
    class SignUpAsyncTask extends AsyncTask<Void, Void, Void> {
		String mUsernameLocal = null;
		String mPasswordLocal = null;	
		int mGenderLocal = 0;
		int mYearLocal = 2000;
		int mMonthLocal = Calendar.JANUARY;
		int mDayLocal = 1;
		String mEmailLocal = null;
		
    	String mResponse = null;
    	JSONObject mJson = null;		
    	String mStatus;
    	boolean mCancel = false;
        long startTime = 0l;
    	
    	SignUpAsyncTask()
    	{
            startTime = System.currentTimeMillis();
    		mUsernameLocal = null;
    		mPasswordLocal = null;	
    		mGenderLocal   = mGender;
    		mYearLocal     = mYear;
    		mMonthLocal    = mMonth;
    		mDayLocal      = mDay;
    		mEmailLocal    = mEmail;

            if (mYearLocal % 4 == 0 && mMonthLocal == 1 && mDayLocal == 29) // yes really
                mDayLocal = 28;

            String field = "";
            String error = "NotUTF8";
        	try {
        		mUsernameLocal = URLEncoder.encode(mUsername, "UTF-8");
			} catch (UnsupportedEncodingException e) {
	            Utils.alertUnexpectedError( "Username contains invalid characters." );
	            mCancel = true;
                field = "username";
			}

            try {
                mPasswordLocal = URLEncoder.encode(mPassword, "UTF-8");
            } catch (UnsupportedEncodingException e) {
                Utils.alertUnexpectedError( "Password contains invalid characters." );
                field = "password";
                mCancel = true;
            }

            if (!field.isEmpty())
                RbxAnalytics.fireFormFieldValidation(ctx, field, error, true);
        }
 
        @Override
        protected Void doInBackground(Void... arg0) {
        	if( mCancel ) return null;

            String gender = "Unknown";
            if (mGenderLocal == 1) gender = "Male";
            else if (mGenderLocal == 2) gender = "Female";

            // convert 0-based month to start from 1
            int month = mMonthLocal + 1;

        	String dateOfBirth = Utils.format( "%d/%d/%d", month, mDayLocal, mYearLocal );
        	HttpHeader[] headerList = new HttpHeader[1];
        	
        	try
        	{
        		String s;
        		if(RobloxSettings.isTestSite())
        			s = s6+s7+s8+s9+s10+mUsernameLocal;
        		else
        			s = s1+s2+s3+s4+s5+mUsernameLocal;
        			
		        String h = computeHash(s);
	        	HttpAgent.HttpHeader header = new HttpAgent.HttpHeader();
	        	header.header = "X-RBXUSER-TOKEN";
	        	header.value = h;
	        	headerList[0] = header;
	        	
        	}
        	catch(Exception e)
        	{ /*Do nothing, continue, let's go with improper hash, it will get rejected response 400 from web */}
        	
        	
        	mResponse = HttpAgent.readUrlToString( RobloxSettings.signUpUrl(),
        			RobloxSettings.signUpUrlArgs(mUsernameLocal, mPasswordLocal, gender, dateOfBirth, mEmailLocal), headerList );
        	if( mResponse != null )
        	{
        		try {
        			mJson = new JSONObject( mResponse );
       				mStatus = mJson.getString("Status");
        		} catch (JSONException e) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_JSONPARSE, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
        		}
        	}

        	//Log.i(TAG, "JSON: " + mResponse );
			return null;
        }

		@Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            
            if( getActivity() == null )
            {
                RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_TASKRUNNING, 0,
                        RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                        System.currentTimeMillis() - startTime);
            	return; // Task still running after Fragment has been removed.
            }
            
            if( mResponse == null )
            {
            	if( !Utils.alertIfNetworkNotConnected() )
            	{
    	            Utils.sendUnexpectedError( "SignUpAsyncTask cannot get response" );
            		Utils.alertExclusively( R.string.ConnectionError );
                    unlockFields();
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_NORESPONSE, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
            	}
            }
            else {
                boolean errorAlreadyReported = false;
                if (mJson == null || mStatus == null) {
                    Utils.alertUnexpectedError("Unable to contact server. Please check your internet connection!");
                }
                else if (mStatus.equals("OK")) {
                    RbxReportingManager.fireSignupSuccess(0);
                    SessionManager.getInstance().doLoginFromStart(mUsername, mPassword, (RobloxActivity) getActivity());
                    return;
                }
                else if (mStatus.equals("Already Taken")) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_ALREADYTAKEN, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
                    setFieldError(mUsernameField, R.string.UsernameTaken);
                    errorAlreadyReported = true;
                }
                else if (mStatus.equals("Invalid Characters Used")) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_INVALIDCHAR, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
                    setFieldError(mUsernameField, R.string.InvalidCharactersUsed);
                    errorAlreadyReported = true;
                }
                else if (mStatus.equals("Username Cannot Contain Spaces")) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_CONTAINSSPACE, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
                    setFieldError(mUsernameField, R.string.UsernameCannotContainSpaces);
                    errorAlreadyReported = true;
                }
                else if (mStatus.equals("AccountCreationFloodcheck")) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_ACCOUNTCREATEFLOOD, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
                    Utils.alertExclusively(R.string.AccountCreationFloodcheck);
                    errorAlreadyReported = true;
                } else if (mStatus.equals("Invalid username")) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_INVALIDUSER, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
                    setFieldError(mUsernameField, R.string.UsernameInvalid);
                    errorAlreadyReported = true;
                }

                // If we get this far, something went wrong
                unlockFields();
                if (!errorAlreadyReported) {
                    RbxReportingManager.fireSignupFailure(RbxReportingManager.ACTION_F_UNKNOWNERROR, 0,
                            RobloxSettings.signUpUrl(), mResponse, mUsernameLocal,
                            System.currentTimeMillis() - startTime);
                }
            }
		}	
    }

    // -----------------------------------------------------------------------
    void closeDialog() {
        Utils.hideKeyboard(mViewRef);

        FragmentTransaction ft = getFragmentManager().beginTransaction();
        ft.setCustomAnimations(R.animator.slide_in_from_bottom, R.animator.slide_out_to_bottom);
        ft.remove(this);
        ft.commit();
    }

    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        if (!SessionManager.getInstance().getIsLoggedIn())
            RbxAnalytics.fireButtonClick(ctx, "close");
    }

    public void showLoginActivity() {
        mSignUpButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_PROGRESS, R.string.LoggingIn);
    }

    public void stopLoginActivity() {
        unlockFields();
        mSignUpButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
    }

    // -----------------------------------------------------------------------
    @Override
    public void handleNotification(int notificationId, Bundle userParams) {
        switch (notificationId) {
            case NotificationManager.EVENT_USER_LOGIN:
                if(SessionManager.getInstance().getIsLoggedIn()) {
                    closeDialog();
                }
                break;

            default:
                break;
        }
    }

    private void lockFields() {
        mPasswordField.lock();
        mUsernameField.lock();
        mPasswordVerifyField.lock();
        mEmailField.lock();
        mGenderPicker.lock();
        mBirthdayPicker.lock();
    }

    private void unlockFields() {
        mSignUpButton.toggleState(RbxProgressButton.STATE_COMMAND.SHOW_BUTTON);
        mUsernameField.unlock();
        mPasswordField.unlock();
        mPasswordVerifyField.unlock();
        mEmailField.unlock();
        mGenderPicker.unlock();
        mBirthdayPicker.unlock();
    }
}


