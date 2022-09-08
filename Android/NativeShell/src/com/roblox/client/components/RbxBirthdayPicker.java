package com.roblox.client.components;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import com.roblox.client.R;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.List;

public class RbxBirthdayPicker extends LinearLayout {

    public final String TAG = "RbxBirthdayPicker";

    public static final int NEUTRAL = -1;

    private Spinner mDaySpinner;
    private Spinner mMonthSpinner;
    private Spinner mYearSpinner;
    private LinearLayout mContainer;

    private OnRbxDateChanged mDateChangeListener;

    private ArrayList<Integer> allDays;
    private ArrayList<Integer> allYears;
    private ArrayList<Integer> allMonths;

    private int mCurYear;
    private int mCurMonth;
    private int mCurDay;

    private int selectedDay = NEUTRAL;
    private int selectedMonth = NEUTRAL;
    private int selectedYear = NEUTRAL;

    private int colorRbxGray2;
    private int colorRbxTextLight;

    private HashMap<Integer, String> monthValues;

    public RbxBirthdayPicker(Context context) {
        super(context);
        init(context, null);
    }

    public RbxBirthdayPicker(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs);
    }

    private void init (Context c, AttributeSet attrs) {
        LayoutInflater.from(c).inflate(R.layout.rbx_birthday_picker, (ViewGroup)(this.getRootView()));
        mDaySpinner = (Spinner)findViewById(R.id.rbxBirthdayDaySpinner);
        mMonthSpinner = (Spinner)findViewById(R.id.rbxBirthdayMonthSpinner);
        mYearSpinner = (Spinner)findViewById(R.id.rbxBirthdayYearSpinner);
        mContainer = (LinearLayout)findViewById(R.id.rbxBirthdayContainer);

        colorRbxGray2 = getResources().getColor(R.color.RbxGray2);
        colorRbxTextLight = getResources().getColor(R.color.RbxTextLight);

        if (attrs != null) {
            RbxFontHelper.setCustomFont((TextView) findViewById(R.id.rbxBirthdayText), c, attrs);
        }

        // init month display values 0-11
        monthValues = new HashMap<Integer, String>();
        String[] monthDisplayValues = getResources().getStringArray(R.array.MonthArray);
        for(int i=0; i<monthDisplayValues.length; i++){
            monthValues.put(i, monthDisplayValues[i]);
        }

        Calendar cal = Calendar.getInstance();
        mCurYear = cal.get(Calendar.YEAR);
        mCurMonth = cal.get(Calendar.MONTH);
        mCurDay = cal.get(Calendar.DAY_OF_MONTH);

        allDays = new ArrayList<Integer>();
        for (int i = 1; i <= 31; i++) {
            allDays.add(i);
        }
        allYears = getYearList();
        allMonths = getMonthList();

        if (!isInEditMode()) { // stops the editor from running this code

            // initial list of options include neutral option
            ArrayList<Integer> initialDayList = getDayList();
            initialDayList.add(0, NEUTRAL);

            ArrayList<Integer> initialYearList = getYearList();
            initialYearList.add(0, NEUTRAL);

            ArrayList<Integer> initialMonthList = getMonthList();
            initialMonthList.add(0, NEUTRAL);

            setDaySpinner(initialDayList);
            setYearSpinner(initialYearList);
            setMonthSpinner(initialMonthList);

            mDaySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                    boolean isChanged = onDaySelected();
                    if (mDateChangeListener != null && isChanged) {
                        //Log.v(TAG, "RBP mDaySpinner.onItemSelected() " + selectedDay);
                        mDateChangeListener.dateChanged(OnRbxDateChanged.DAY, selectedDay);
                    }
                }

                @Override
                public void onNothingSelected(AdapterView<?> adapterView) {}
            });

            mMonthSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                    boolean isChanged = onMonthSelected();
                    if (mDateChangeListener != null && isChanged) {
                        //Log.v(TAG, "RBP mMonthSpinner.onItemSelected() " + selectedMonth);
                        mDateChangeListener.dateChanged(OnRbxDateChanged.MONTH, selectedMonth);
                    }
                }

                @Override
                public void onNothingSelected(AdapterView<?> adapterView) {}
            });

            mYearSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                    boolean isChanged = onYearSelected();
                    if (mDateChangeListener != null && isChanged) {
                        //Log.v(TAG, "RBP mYearSpinner.onItemSelected() " + selectedYear);
                        mDateChangeListener.dateChanged(OnRbxDateChanged.YEAR, selectedYear);
                    }
                }

                @Override
                public void onNothingSelected(AdapterView<?> adapterView) {}
            });
        }
    }

    public int getDay()     { return (int) mDaySpinner.getSelectedItem(); }
    public int getMonth()   { return (int) mMonthSpinner.getSelectedItem(); }
    public int getYear()    { return (int) mYearSpinner.getSelectedItem(); }

    public void setError() {
        mContainer.setBackgroundResource(R.drawable.rbx_bg_gender_full_error);
    }

    public void clearError() {
        mContainer.setBackgroundResource(R.drawable.rbx_bg_gender_full);
    }

    public void setRbxDateChangedListener(OnRbxDateChanged l) {
        mDateChangeListener = l;
    }

    public void lock() {
        AlphaAnimation alpha = RbxAnimHelper.lockAnimation(this);
        this.startAnimation(alpha);

        View.OnTouchListener consumeTouch = new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                return true;
            }
        };

        mDaySpinner.setOnTouchListener(consumeTouch);
        mMonthSpinner.setOnTouchListener(consumeTouch);
        mYearSpinner.setOnTouchListener(consumeTouch);
    }

    public void unlock() {
        AlphaAnimation alpha = RbxAnimHelper.unlockAnimation(this);
        this.startAnimation(alpha);

        mDaySpinner.setOnTouchListener(null);
        mMonthSpinner.setOnTouchListener(null);
        mYearSpinner.setOnTouchListener(null);
    }

    /**
     * @return true if day selected has changed
     */
    private boolean onDaySelected() {
        int prevDay = selectedDay;
        selectedDay = (int) mDaySpinner.getSelectedItem();
        //Log.v(TAG, "RBP onDaySelected() " + selectedDay);

        // remove the neutral view if applicable
        if(mDaySpinner.getItemAtPosition(0) == NEUTRAL && selectedDay != NEUTRAL){
            updateDaySpinner();
        }

        return prevDay != selectedDay;
    }

    /**
     * @return true if month selected has changed
     */
    private boolean onMonthSelected() {
        int prevMonth = selectedMonth;
        selectedMonth = (int) mMonthSpinner.getSelectedItem();
        //Log.v(TAG, "RBP.onMonthSelected() " + selectedMonth);

        updateDaySpinner();

        // remove the neutral view if applicable
        if(mMonthSpinner.getItemAtPosition(0) == NEUTRAL && selectedMonth != NEUTRAL){
            updateMonthSpinner();
        }

        return prevMonth != selectedMonth;
    }

    /**
     * @return true if year selected has changed
     */
    private boolean onYearSelected() {
        int prevYear = selectedYear;
        selectedYear = (int) mYearSpinner.getSelectedItem();
        //Log.v(TAG, "RBP.onYearSelected() " + selectedYear);

        updateMonthSpinner();

        // remove the neutral view if applicable
        if(mYearSpinner.getItemAtPosition(0) == NEUTRAL && selectedYear != NEUTRAL){
            updateYearSpinner();
        }

        return prevYear != selectedYear;
    }

    private void updateDaySpinner(){

        List<Integer> days = getDayList();

        if(selectedDay == NEUTRAL){
            days.add(0, NEUTRAL);
        }

        int lastIdx = days.size() - 1;
        int lastDay = days.get(lastIdx);
        int selectedIdx = days.indexOf(selectedDay);

        setDaySpinner(days);

        if(selectedDay > lastDay) {
            mDaySpinner.setSelection(lastIdx);
        }
        else if(selectedIdx == -1) {
            mDaySpinner.setSelection(0);
        }
        else {
            mDaySpinner.setSelection(selectedIdx);
        }
    }

    private void updateYearSpinner(){

        int selectedIdx = allYears.indexOf(selectedYear);

        setYearSpinner(allYears);

        if(selectedIdx == NEUTRAL) {
            mYearSpinner.setSelection(0);
        }
        else {
            mYearSpinner.setSelection(selectedIdx);
        }
    }

    private void updateMonthSpinner(){

        List<Integer> months = null;

        if(selectedMonth == NEUTRAL){
            if (selectedYear == mCurYear) {
                // Limit available months
                months = new ArrayList<Integer>(allMonths.subList(0, mCurMonth + 1));
            }
            else {
                months = getMonthList();
            }
            // Append neutral selection
            months.add(0, NEUTRAL);
        }
        else {
            if (selectedYear == mCurYear) {
                // Limit available months
                months = allMonths.subList(0, mCurMonth + 1);
            }
            else {
                months = allMonths;
            }
        }

        int lastIdx = months.size() - 1;
        int lastMonth = months.get(lastIdx);
        int selectedIdx = months.indexOf(selectedMonth);

        setMonthSpinner(months);

        if(selectedMonth > lastMonth) {
            mMonthSpinner.setSelection(lastIdx);
        }
        else if(selectedIdx == NEUTRAL) {
            mMonthSpinner.setSelection(0);
        }
        else {
            mMonthSpinner.setSelection(selectedIdx);
        }
    }

    private ArrayList<Integer> getDayList(){
        int numDays = 31;
        if(selectedMonth != NEUTRAL){
            if(mCurYear == selectedYear && mCurMonth == selectedMonth){
                numDays = mCurDay;
            }
            else {
                numDays = numDaysInMonth(selectedMonth);
            }
        }
        ArrayList<Integer> days = new ArrayList(allDays.subList(0, numDays));
        return days;
    }

    private ArrayList<Integer> getYearList(){
        ArrayList<Integer> years = new ArrayList<Integer>();
        for (int i = mCurYear; i >= (mCurYear - 100); i--) {
            years.add(i);
        }
        return years;
    }

    private ArrayList<Integer> getMonthList(){
        ArrayList<Integer> months = new ArrayList<Integer>();
        for (int i = 0; i <= 11; i++) {
            months.add(i);
        }
        return months;
    }

    private void setDaySpinner(List<Integer> list) {
        NeutralDateArrayAdapter dayAdapter = new NeutralDateArrayAdapter(getContext(), R.layout.rbx_spinner_top, list);
        dayAdapter.setDropDownViewResource(R.layout.rbx_spinner_item);
        dayAdapter.setNeutralText("--");
        mDaySpinner.setAdapter(dayAdapter);
    }

    private void setMonthSpinner(List<Integer> list) {
        NeutralDateArrayAdapter monthAdapter = new NeutralDateArrayAdapter(getContext(), R.layout.rbx_spinner_top, list);
        monthAdapter.setDropDownViewResource(R.layout.rbx_spinner_item);
        monthAdapter.setNeutralText("--");
        monthAdapter.setDisplayValues(monthValues);
        mMonthSpinner.setAdapter(monthAdapter);
    }

    private void setYearSpinner(List<Integer> list){
        NeutralDateArrayAdapter yearAdapter = new NeutralDateArrayAdapter(getContext(), R.layout.rbx_spinner_top, list);
        yearAdapter.setDropDownViewResource(R.layout.rbx_spinner_item);
        yearAdapter.setNeutralText("----");
        mYearSpinner.setAdapter(yearAdapter);
    }

    /**
     * Gets the number of days in the given month. Month is 0 based.
     * @param month
     */
    private int numDaysInMonth(int month) {
        if (month == 1) return 28;
        else if (month == 3 || month == 5 || month == 8 || month == 10) return 30;
        else return 31;
    }

    /**
     * Common adapter for month/day/year spinners
     */
    class NeutralDateArrayAdapter<T> extends ArrayAdapter<T> {

        private String neutralText;
        private HashMap<Integer, String> displayValues = null;

        private int resource;
        private int dropResource;

        public NeutralDateArrayAdapter(Context context, int resource, List<T> objects) {
            super(context, resource, objects);
            this.resource = resource;
        }

        public void setNeutralText(String text){
            neutralText = text;
        }

        public void setDisplayValues(HashMap<Integer, String> values){
            displayValues = values;
        }

        @Override
        public void setDropDownViewResource(int resource) {
            super.setDropDownViewResource(resource);
            dropResource = resource;
        }

        @Override
        public View getDropDownView(int position, View convertView, ViewGroup parent) {
            T value = getItem(position);
            if(value.equals(NEUTRAL)){
                return getNeutralView(position, convertView, parent, dropResource);
            }
            else if(displayValues != null) {
                return getDisplayView(position, convertView, parent, dropResource);
            }
            else {
                return super.getDropDownView(position, convertView, parent);
            }
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            T value = getItem(position);
            if(value.equals(NEUTRAL)){
                return getNeutralView(position, convertView, parent, resource);
            }
            else if(displayValues != null) {
                return getDisplayView(position, convertView, parent, resource);
            }
            else {
                return super.getView(position, convertView, parent);
            }
        }

        private View getNeutralView(int position, View convertView, ViewGroup parent, int resource){
            View view;
            if (convertView == null) {
                view = LayoutInflater.from(getContext()).inflate(resource, parent, false);
            } else {
                view = convertView;
            }
            ((TextView) view).setText(neutralText);
            ((TextView) view).setTextColor(colorRbxTextLight);
            return view;
        }

        private View getDisplayView(int position, View convertView, ViewGroup parent, int resource){
            View view;
            if (convertView == null) {
                view = LayoutInflater.from(getContext()).inflate(resource, parent, false);
            } else {
                view = convertView;
            }
            TextView text = (TextView) view;

            T value = getItem(position);

            String displayVal = displayValues.get(value);
            if(value.equals(NEUTRAL)){
                text.setText(neutralText);
                text.setTextColor(colorRbxTextLight);
            }
            else if(displayVal != null){
                text.setText(displayVal);
                text.setTextColor(colorRbxGray2);
            }
            else {
                text.setText(value.toString());
                text.setTextColor(colorRbxGray2);
            }
            return text;
        }
    }
}
