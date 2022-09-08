package com.roblox.client.signup;

import com.roblox.client.RobloxSettings;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Created on 11/24/15.
 *
 * Sign-up task for mobileapi/securesignup endpoint
 */
public class SignUpMobileApiTask extends SignUpAsyncTask {

    public SignUpMobileApiTask(int mGender, int mYear, int mMonth, int mDay, String mEmail, String mUsername, String mPassword, SignUpAsyncTaskListener listener) {
        super(mGender, mYear, mMonth, mDay, mEmail, mUsername, mPassword, listener);
    }

    @Override
    protected SignUpResult doSignupRequest(String username, String password, HttpAgent.HttpHeader[] headerList){
        String gender = getGenderParamValue();
        String dateOfBirth = getDateOfBirthParamValue();
        return doSignupRequest(username, password, gender, dateOfBirth, email, headerList);
    }

    protected SignUpResult doSignupRequest(String username, String password, String gender, String dateOfBirth, String email, HttpAgent.HttpHeader[] headerList){

        String url = RobloxSettings.signUpUrl();

        HttpResponse mResponse = HttpAgent.readUrl(
                url,
                RobloxSettings.signUpUrlArgs(username, password, gender, dateOfBirth, email),
                headerList);

        String body = mResponse.responseBodyAsString();

        SignUpResult result = new SignUpResult();
        result.code = mResponse.responseCode();
        result.url = url;
        result.message = body;

        try {
            JSONObject json = new JSONObject(body);
            String statusString = json.getString("Status");
            result.status.add(getSignUpStatus(statusString));
        } catch (JSONException e) {
            result.status.add(SignUpResult.StatusJsonError);
        }

        return result;
    }

    public String getSignUpStatus(String value) {

        String message;

        if(value.equals("OK")){
            message = SignUpResult.StatusOK;
        }
        else if(value.equals("Already Taken")){
            message = SignUpResult.StatusUsernameTaken;
        }
        else if(value.equals("Invalid username")){
            message = SignUpResult.StatusUsernameInvalid;
        }
        else if(value.equals("Invalid Characters Used")){
            message = SignUpResult.StatusUsernameContainsInvalidCharacters;
        }
        else if(value.equals("Username Cannot Contain Spaces")){
            message = SignUpResult.StatusUsernameCannotContainSpaces;
        }
        else if(value.equals("AccountCreationFloodcheck")){
            message = SignUpResult.StatusCaptcha;
        }
        else{
            message = value;
        }
        return message;
    }
}
