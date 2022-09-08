package com.roblox.models;

import android.os.Parcel;
import android.os.Parcelable;

public class FacebookSignupData implements Parcelable{

    public String gender, birthday, email, gigyaUid, realName, profileUrl, rbxUsername;

    public FacebookSignupData(String g, String b, String e, String gU, String r, String p, String rbU) {
        gender = g;
        birthday = g;
        email = e;
        gigyaUid = gU;
        realName = r;
        profileUrl = p;
        rbxUsername = rbU;
    }

    public FacebookSignupData() {
        gender = "";
        birthday = "";
        email = "";
        gigyaUid = "";
        realName = "";
        profileUrl = "";
        rbxUsername = "";
    }

    protected FacebookSignupData(Parcel in) {
        String[] data = new String[7];

        in.readStringArray(data);
        gender = data[0];
        birthday = data[1];
        email = data[2];
        gigyaUid = data[3];
        realName = data[4];
        profileUrl = data[5];
        rbxUsername = data[6];
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeStringArray(new String[] {
                gender,
                birthday,
                email,
                gigyaUid,
                realName,
                profileUrl
        });
    }

    public static final Creator<FacebookSignupData> CREATOR = new Creator<FacebookSignupData>() {
        @Override
        public FacebookSignupData createFromParcel(Parcel in) {
            return new FacebookSignupData(in);
        }

        @Override
        public FacebookSignupData[] newArray(int size) {
            return new FacebookSignupData[size];
        }
    };
}
