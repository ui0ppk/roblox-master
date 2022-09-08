package com.roblox.client;

import android.app.DialogFragment;
import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

public class FragmentBannedAccount extends DialogFragment {

    private String TAG = "FragmentBannedAccount";

    private Bundle mArgs = null;
    private TextView mTextPunishmentType = null;
    private TextView mTextReviewed = null;
    private TextView mTextModNote = null;
    private TextView mTextBannedLength = null;
    private TextView mTextAppeals = null;
    private TextView mTextGuidelines = null;
    private TextView mTextClose = null;
    private TextView mTextFirstLine = null;

    public static final String FRAGMENT_TAG = "banned_window";

    // -----------------------------------------------------------------------
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.setStyle(DialogFragment.STYLE_NO_TITLE, this.getTheme());
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = null;
        view = inflater.inflate(R.layout.fragment_banned_account, container, false);
        if( RobloxSettings.isPhone() )
        {
            // Set the containging layouts to wrap so we don't extend past the end of the screen
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            LinearLayout topContainer = (LinearLayout)view.findViewById(R.id.fragment_banned_account_background);
            topContainer.setLayoutParams(params);

            LinearLayout.LayoutParams paramsFill = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            LinearLayout headerContainer = (LinearLayout)view.findViewById(R.id.fragment_banned_account_header);
            headerContainer.setLayoutParams(paramsFill);

            LinearLayout tableContainer = (LinearLayout)view.findViewById(R.id.fragment_banned_account_table);
            tableContainer.setLayoutParams(params);
        }

        mTextPunishmentType = (TextView)view.findViewById(R.id.tvPunishmentType);
        mTextModNote = (TextView)view.findViewById(R.id.tvModeratorNote);
        mTextReviewed = (TextView)view.findViewById(R.id.tvReviewed);
        mTextBannedLength = (TextView)view.findViewById(R.id.tvBannedLength);
        mTextClose = (TextView)view.findViewById(R.id.tvCloseButton);
        mTextFirstLine = (TextView)view.findViewById(R.id.tvFirstLine);

        mTextClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                closeDialog();
            }
        });

        // Makes the links in these TextViews clickable
        mTextGuidelines = (TextView)view.findViewById(R.id.tvCommunityGuidelines);
        Utils.makeTextViewHtml(getActivity(), mTextGuidelines, getString(R.string.BannedGuidelines));

        mTextAppeals = (TextView)view.findViewById(R.id.tvAppeals);
        Utils.makeTextViewHtml(getActivity(), mTextAppeals, getString(R.string.BannedAppeals));

        mArgs = getArguments();

        if (mArgs != null)
        {
            boolean isWarned = false;
            boolean isDeleted = false;

            String punishmentType = mArgs.getString("PunishmentType");
            int numDays = 0;
            switch (punishmentType)
            {
                case "Ban 1 Days":
                    mTextPunishmentType.setText(R.string.Banned1Day);
                    numDays = 1;
                    break;
                case "Ban 3 Days":
                    mTextPunishmentType.setText(R.string.Banned3Day);
                    numDays = 3;
                    break;
                case "Ban 7 Days":
                    mTextPunishmentType.setText(R.string.Banned7Day);
                    numDays = 7;
                    break;
                case "Ban 14 Days":
                    mTextPunishmentType.setText(R.string.Banned14Day);
                    numDays = 14;
                    break;
                case "Warn":
                    mTextPunishmentType.setText(R.string.BannedWarning);
                    isWarned = true;
                    break;
                case "Delete":
                    mTextPunishmentType.setText(R.string.BannedDelete);
                    isDeleted = true;
                    break;
                default:
                    mTextPunishmentType.setText(R.string.BannedDefault);
                    break;
            }


            mTextReviewed.setText(String.format(getResources().getString(R.string.BannedReviewed), mArgs.getString("ReviewDate", "Date Unknown")));
            mTextModNote.setText(String.format(getResources().getString(R.string.BannedModeratorNote), mArgs.getString("ModeratorNote", "No message.")));
            if (!isWarned && !isDeleted)
                mTextBannedLength.setText(String.format(getResources().getString(R.string.BannedLengthAndEndDate), numDays, mArgs.getString("EndDate", "then.")));
            else
            {
                if (isWarned)
                {
                    mTextAppeals.setVisibility(View.GONE);
                    Utils.makeTextViewHtml(getActivity(), mTextBannedLength, getResources().getString(R.string.BannedWarningInstructions));
                }
                else if (isDeleted)
                {
                    mTextGuidelines.setVisibility(View.GONE);
                    mTextBannedLength.setText(R.string.BannedDeletedMessage);
                    mTextFirstLine.setText(R.string.BannedDeletedFirstLine);
                }
            }
        }


        return view;
    }

    public void closeDialog()
    {
        getActivity().getFragmentManager().beginTransaction().remove(this).commit();
    }
}