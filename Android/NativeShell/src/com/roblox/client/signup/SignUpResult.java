package com.roblox.client.signup;

import java.util.ArrayList;
import java.util.List;

/**
 * Created on 11/24/15.
 */
public class SignUpResult {

    // Status values that are handled by the client
    public static final String StatusOK = "OK";
    public static final String StatusUsernameTaken = "UsernameTaken";
    public static final String StatusUsernameContainsInvalidCharacters = "UsernameContainsInvalidCharacters";
    public static final String StatusUsernameCannotContainSpaces = "UsernameCannotContainSpaces";
    public static final String StatusCaptcha = "Captcha";
    public static final String StatusUsernameInvalid = "UsernameInvalid";
    public static final String StatusBirthdayInvalid = "BirthdayInvalid";
    public static final String StatusGenderInvalid = "GenderInvalid";
    public static final String StatusPasswordInvalid = "PasswordInvalid";
    public static final String StatusJsonError = "StatusJsonError";
    public static final String StatusServerError = "StatusServerError";

    // Holds the status value
    public ArrayList<String> status = new ArrayList<String>();

    // Reporting information
    public String reportingAction;
    public int code;
    public String url;
    public String message;

    public boolean isOk(){
        String value = status.get(0);
        return value != null && value.equals(StatusOK);
    }
}
