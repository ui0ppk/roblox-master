package com.roblox.client.components;

import android.view.View;

public interface OnRbxDateChanged {
    public static final int DAY = 0;
    public static final int MONTH = 1;
    public static final int YEAR = 2;

    void dateChanged(int field, int newValue);
}
