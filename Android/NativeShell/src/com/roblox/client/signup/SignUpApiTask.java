package com.roblox.client.signup;

import android.util.Log;

import com.roblox.client.RobloxSettings;
import com.roblox.client.Utils;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

/**
 * Created on 11/24/15.
 *
 * Sign-up task for signup/v1 endpoint
 */
public class SignUpApiTask extends SignUpAsyncTask {

    // Known error reasons returned by login/v1
    private static final String ReasonUsernameTaken = "UsernameTaken";
    private static final String ReasonUsernameInvalid = "UsernameInvalid";
    private static final String ReasonCaptcha = "Captcha";
    private static final String ReasonBirthdayInvalid = "BirthdayInvalid";
    private static final String ReasonPasswordInvalid = "PasswordInvalid";
    private static final String ReasonGenderInvalid = "GenderInvalid";

    private static HashMap<String, Integer> errorWeightMap;
    static {
        errorWeightMap = new HashMap<String, Integer>();
        errorWeightMap.put(SignUpResult.StatusUsernameTaken, 1);
        errorWeightMap.put(SignUpResult.StatusUsernameInvalid, 2);
        errorWeightMap.put(SignUpResult.StatusPasswordInvalid, 3);
        errorWeightMap.put(SignUpResult.StatusGenderInvalid, 4);
        errorWeightMap.put(SignUpResult.StatusBirthdayInvalid, 5);

        // We want to handle all other errors before captcha
        errorWeightMap.put(SignUpResult.StatusCaptcha, 10000);
    }

    public SignUpApiTask(int mGender, int mYear, int mMonth, int mDay, String mEmail, String mUsername, String mPassword, SignUpAsyncTaskListener listener) {
        super(mGender, mYear, mMonth, mDay, mEmail, mUsername, mPassword, listener);
    }

    @Override
    protected SignUpResult doSignupRequest(String username, String password, HttpAgent.HttpHeader[] headerList){
        String gender = getGenderParamValue();
        String dateOfBirth = getDateOfBirthParamValue();
        return doSignupRequest(username, password, gender, dateOfBirth, headerList);
    }

    protected SignUpResult doSignupRequest(String username, String password, String gender, String dateOfBirth, HttpAgent.HttpHeader[] headerList){

        String params = Utils.format("username=%s&password=%s&gender=%s&birthday=%s", username, password, gender, dateOfBirth);
        Log.i(SignUpAsyncTask.TAG, "SignUpApiTask.doSignupRequest() params:" + params);

        String url = RobloxSettings.signUpApiUrl();
        HttpResponse mResponse = HttpAgent.readUrl(url, params, headerList);
        int code = mResponse.responseCode();
        String body = mResponse.responseBodyAsString();

        SignUpResult result = new SignUpResult();
        result.code = code;
        result.url = url;
        result.message = body;

        Log.i(SignUpAsyncTask.TAG, "SignUpApiTask.doSignupRequest() code:" + code);
        if(code == 200 || code == 403) {
            try {
                JSONObject json = new JSONObject(body);
                int userId = json.optInt("userId", -1);
                if(userId != -1){
                    result.status.add(SignUpResult.StatusOK);
                }
                else {
                    JSONArray jsonArray = json.optJSONArray("reasons");
                    // client currently only handles one error at a time.
                    addToSignUpStatus(result.status, jsonArray);
                }
            } catch (JSONException e) {
                result.status.add(SignUpResult.StatusJsonError);
            }
        }
        else {
            // error probably 500 or 503
            result.status.add(SignUpResult.StatusServerError);
        }

        return result;
    }

    private void addToSignUpStatus(ArrayList<String> errorList, JSONArray jsonArray){

        if(jsonArray != null) {
            for(int i=0; i<jsonArray.length(); i++) {
                String reason = jsonArray.optString(i, null);
                if(reason != null) {
                    errorList.add(getSignUpStatus(reason));
                }
            }

            if(errorList.size() > 1){
                Collections.sort(errorList, new Comparator<String>() {
                    @Override
                    public int compare(String lhs, String rhs) {
                        Integer left = errorWeightMap.get(lhs);
                        if(left == null){
                            left = 1000;
                        }
                        Integer right = errorWeightMap.get(rhs);
                        if(right == null){
                            right = 1000;
                        }
                        return left - right;
                    }
                });
            }
            Log.w(SignUpAsyncTask.TAG, "SignUpApiTask.getErrorReason() errorList:" + errorList);
        }
    }

    public String getSignUpStatus(String value) {

        String status;

        if(value.equals(ReasonUsernameTaken)){
            status = SignUpResult.StatusUsernameTaken;
        }
        else if(value.equals(ReasonUsernameInvalid)){
            status = SignUpResult.StatusUsernameInvalid;
        }
        else if(value.equals(ReasonCaptcha)){
            status = SignUpResult.StatusCaptcha;
        }
        else if(value.equals(ReasonBirthdayInvalid)){
            status = SignUpResult.StatusBirthdayInvalid;
        }
        else if(value.equals(ReasonPasswordInvalid)){
            status = SignUpResult.StatusPasswordInvalid;
        }
        else if(value.equals(ReasonGenderInvalid)){
            status = SignUpResult.StatusGenderInvalid;
        }
        else{
            status = value;
        }
        return status;
    }
}
