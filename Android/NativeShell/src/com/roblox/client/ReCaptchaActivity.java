package com.roblox.client;

import android.content.Intent;
import android.lib.recaptcha.ReCaptcha;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.design.widget.TextInputLayout;
import android.support.v7.app.AppCompatActivity;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;

/**
 * Created on 12/11/15.
 */
public class ReCaptchaActivity extends AppCompatActivity implements ReCaptcha.OnShowChallengeListener, View.OnClickListener {

    private static final String RECAPTCHA_PUBLIC_KEY  = "6Le88gcTAAAAALG04IFgQDENWlQmc_hy_3tdF1yY";

    public static final String USERNAME = "USERNAME_EXTRA";
    public static final String ACTION = "ACTION_EXTRA";

    public static final int ACTION_NONE = 0;
    public static final int ACTION_LOGIN = 1;
    public static final int ACTION_SIGNUP = 2;

    public static final int ACTIVITY_REQUEST_CODE = 989;

    private ReCaptcha reCaptcha;
    private ProgressBar progress;
    private View container;
    private EditText captchaText;
    private Button verifyButton;
    private Button reloadButton;
    private TextInputLayout captchaTextLayout;

    private String token;
    private String username;
    private int action = ACTION_NONE;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_recaptcha);

        Intent intent = getIntent();
        if(intent != null){
            username = intent.getStringExtra(USERNAME);
            action = intent.getIntExtra(ACTION, ACTION_NONE);
        }

        if(action == ACTION_NONE) {
            // not enough information to complete captcha
            finish();
        }

        reCaptcha = (ReCaptcha) findViewById(R.id.recaptcha);
        progress = (ProgressBar) findViewById(R.id.progress);
        container = findViewById(R.id.container);
        captchaText = (EditText) findViewById(R.id.captcha_input);
        verifyButton = (Button) findViewById(R.id.verify);
        reloadButton = (Button) findViewById(R.id.reload);
        captchaTextLayout = (TextInputLayout) findViewById(R.id.captcha_input_layout);

        // initialize the error field
        captchaTextLayout.setErrorEnabled(true);
        captchaTextLayout.setError(null);

        captchaText.setImeOptions(EditorInfo.IME_ACTION_SEND);
        captchaText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_SEND) {
                    validateCaptcha();
                    return true;
                }
                return false;
            }
        });

        container.setOnClickListener(this);
        verifyButton.setOnClickListener(this);
        reloadButton.setOnClickListener(this);

        verifyButton.setEnabled(false);

        showChallenge();
    }

    @Override
    public void onClick(View v) {
        if(v == container){
            clearInputFocus();
        }
        else if(v == verifyButton){
            validateCaptcha();
        }
        else if(v == reloadButton){
            showChallenge();
        }
    }

    private void showChallenge() {
        // Displays a progress bar while downloading CAPTCHA
        progress.setVisibility(View.VISIBLE);
        reCaptcha.setVisibility(View.GONE);

        // clear field
        captchaText.setText("");
        captchaTextLayout.setError(null);

        reCaptcha.showChallengeAsync(RECAPTCHA_PUBLIC_KEY, ReCaptchaActivity.this);
    }

    @Override
    public void onChallengeShown(final boolean shown, final String token) {
        progress.setVisibility(View.GONE);

        this.token = token;

        if (shown) {
            // If a CAPTCHA is shown successfully, displays it for the user to enter the words
            reCaptcha.setVisibility(View.VISIBLE);
            verifyButton.setEnabled(true);
        }
    }

    private void validateCaptcha(){
        clearInputFocus();
        verifyButton.setEnabled(false);

        progress.setVisibility(View.VISIBLE);
        reCaptcha.setVisibility(View.GONE);

        CaptchaTask task = new CaptchaTask(username, token, captchaText.getText().toString(), action);
        task.execute();
    }

    private void clearInputFocus(){
        if (captchaText.hasFocus()) {
            captchaText.clearFocus();
            Utils.hideKeyboard(captchaText);
        }
    }

    private class CaptchaTask extends AsyncTask<Void, Void, HttpResponse> {

        private String username;
        private String token;
        private String answer;
        private int action;

        public CaptchaTask(String username, String token, String answer, int action){
            this.username = username;
            this.token = token;
            this.answer = answer;
            this.action = action;
        }

        @Override
        protected HttpResponse doInBackground(Void... args) {

            String params = Utils.format("username=%s&recaptcha_challenge_field=%s&recaptcha_response_field=%s", username, token, answer);

            String url = null;

            if(action == ACTION_LOGIN){
                url = RobloxSettings.loginCaptchaValidateUrl();
            }
            else if(action == ACTION_SIGNUP){
                url = RobloxSettings.signupCaptchaValidateUrl();
            }

            HttpResponse mResponse = null;
            if(url != null) {
                mResponse = HttpAgent.readUrl(url, params, null);
            }

            return mResponse;
        }

        @Override
        protected void onPostExecute(HttpResponse httpResponse) {

            if(httpResponse != null && httpResponse.responseCode() == 200){
                // success
                setResult(RESULT_OK);
                finish();
            }
            else{
                Toast.makeText(getApplicationContext(), R.string.captchaFailed, Toast.LENGTH_SHORT).show();
                showChallenge();
            }
        }
    }
}
