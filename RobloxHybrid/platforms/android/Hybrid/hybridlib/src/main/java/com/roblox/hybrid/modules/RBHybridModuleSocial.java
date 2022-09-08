package com.roblox.hybrid.modules;

import android.content.Intent;

import com.roblox.hybrid.RBHybridCommand;
import com.roblox.hybrid.RBHybridModule;

import org.json.JSONObject;

/**
 * Created by roblox on 3/6/15.
 */
public class RBHybridModuleSocial extends RBHybridModule {

    private static final String MODULE_ID = "Social";

    public RBHybridModuleSocial() {
        super(MODULE_ID);

        this.registerFunction("presentShareDialog", new PresentShareDialog());
    }

    private class PresentShareDialog implements ModuleFunction {
        @Override
        public void execute(RBHybridCommand command) {
            JSONObject params = command.getParams();

            String text = params.optString("text", "");
            String link = params.optString("link");
            String imageURL = params.optString("imageURL");

            Intent sendIntent = new Intent();
            sendIntent.setAction(Intent.ACTION_SEND);
            sendIntent.setType("text/plain");
            sendIntent.putExtra(Intent.EXTRA_TEXT, text + " " + link);
            sendIntent.putExtra(Intent.EXTRA_TITLE, text);
            sendIntent.putExtra(Intent.EXTRA_SUBJECT, text);

            Intent chooserIntent = Intent.createChooser(sendIntent, null);
            command.getContext().startActivity(chooserIntent);

            command.executeCallback(true, null);
        }
    };
}
