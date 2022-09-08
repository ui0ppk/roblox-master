package com.roblox.hybrid;

import org.json.JSONObject;

public class RBHybridEvent {
    private String mModuleName;
    private String mName;
    private JSONObject mParams;

    public String getModuleName() { return mModuleName; }
    public void setModuleName(final String module) { mModuleName = module; }

    public String getName() { return mName; }
    public void setName(final String name) { mName = name; }

    public JSONObject getParams() { return mParams; }
    public void setParams(final JSONObject params) { mParams = params; }

}
