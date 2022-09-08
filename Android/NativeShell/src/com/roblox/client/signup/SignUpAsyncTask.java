package com.roblox.client.signup;

import android.os.AsyncTask;

import com.roblox.client.RobloxSettings;
import com.roblox.client.Utils;
import com.roblox.client.http.HttpAgent;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * Created on 11/23/15.
 */
public abstract class SignUpAsyncTask extends AsyncTask<Void, Void, SignUpResult> {

    public static final String TAG = "roblox.signup";

    public interface SignUpAsyncTaskListener {
        void onSignUpPostExecuteSuccess(SignUpResult result);
        void onSignUpPostExecuteFailed(SignUpResult result);
    }

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

    private final boolean useRbxUserToken = true;

    protected String username = null;
    protected String password = null;
    protected int gender = 0;
    protected int year;
    protected int month;
    protected int day;
    protected String email = null;

    protected SignUpAsyncTaskListener listener;

    public SignUpAsyncTask(int gender, int year, int month, int day, String email, String username, String password, SignUpAsyncTaskListener listener)
    {
        this.listener = listener;

        this.username = username;
        this.password = password;
        this.gender = gender;
        this.year = year;
        this.month = month;
        this.day = day;
        this.email = email;
    }

    @Override
    protected SignUpResult doInBackground(Void... arg0) {

        String lUsername;
        String lPassword;

        try {
            lUsername = URLEncoder.encode(username, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            // According to the intertubes, this cannot happen because... UTF-8
            SignUpResult result = new SignUpResult();
            result.status.add(SignUpResult.StatusUsernameContainsInvalidCharacters);
            return result;
        }

        try {
            lPassword = URLEncoder.encode(password, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            // According to the intertubes, this cannot happen because... UTF-8
            SignUpResult result = new SignUpResult();
            result.status.add(SignUpResult.StatusPasswordInvalid);
            return result;
        }

        HttpAgent.HttpHeader[] headerList = getHeaderList(lUsername);

        SignUpResult result = doSignupRequest(lUsername, lPassword, headerList);

        return result;
    }

    abstract protected SignUpResult doSignupRequest(String username, String password, HttpAgent.HttpHeader[] headerList);

    @Override
    protected void onPostExecute(SignUpResult result) {
        super.onPostExecute(result);

        if(listener != null){
            if(result.isOk()){
                listener.onSignUpPostExecuteSuccess(result);
            }
            else {
                listener.onSignUpPostExecuteFailed(result);
            }
        }
    }

    protected String getDateOfBirthParamValue(){
        // convert 0-based month to start from 1
        int adjustedMonth = month + 1;
        return Utils.format("%d/%d/%d", adjustedMonth, day, year);
    }

    protected String getGenderParamValue(){
        String gender = "Unknown";
        if (this.gender == 1){
            gender = "Male";
        }
        else if (this.gender == 2){
            gender = "Female";
        }
        return gender;
    }

    protected HttpAgent.HttpHeader[] getHeaderList(String username){

        HttpAgent.HttpHeader[] headerList = null;
        String hash = null;

        if(useRbxUserToken) {
            // NOTE: X-RBXUSER-TOKEN was used for the  mobileapi/securesignup endpoint
            // In testing, it doesn't seem to be required for signup/v1
            try {
                String s;
                if (RobloxSettings.isTestSite()) {
                    s = s6 + s7 + s8 + s9 + s10 + username;
                } else {
                    s = s1 + s2 + s3 + s4 + s5 + username;
                }
                hash = computeHash(s);
            } catch (Exception e) {
                // Do nothing, continue, let's go with improper hash, it will get rejected response 400 from web
            }

            if (hash != null) {
                HttpAgent.HttpHeader header = new HttpAgent.HttpHeader();
                header.header = "X-RBXUSER-TOKEN";
                header.value = hash;

                headerList = new HttpAgent.HttpHeader[1];
                headerList[0] = header;
            }
        }

        return headerList;
    }

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
}
