package com.roblox.client;

import android.app.DialogFragment;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.roblox.client.components.RbxLinearLayout;

public class FragmentValuePage extends DialogFragment {
    public enum PAGE {HOME, FRIENDS, MESSAGES, MORE, CATALOG, NONE};
    private PAGE mPage;

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        Bundle args = getArguments();
        if (args != null)
            mPage = (PAGE) args.get("page");
        else
            mPage = PAGE.NONE;

        View view = inflater.inflate(R.layout.fragment_value_container, null);

        int cardResource;
        switch (mPage) {
            case HOME:
                cardResource = R.layout.fragment_value_home;
                break;
            case FRIENDS:
                cardResource = R.layout.fragment_value_friends;
                break;
            case MESSAGES:
                cardResource = R.layout.fragment_value_messages;
                break;
            case MORE:
                cardResource = R.layout.fragment_value_more;
                break;
            case CATALOG:
                cardResource = R.layout.fragment_value_catalog;
                break;
            default:
                cardResource = R.layout.fragment_value_error;
                break;
        }
        inflater.inflate(cardResource, (RbxLinearLayout)view.findViewById(R.id.fragment_value_card_container));

        return view;
//        return super.onCreateView(inflater, container, savedInstanceState);
    }
}
