package com.roblox.client;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.HashMap;
import java.lang.ref.WeakReference;

import android.content.Context;
import android.graphics.Point;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.hardware.input.InputManager;
import android.util.Log;
import android.util.SparseArray;
import android.view.GestureDetector;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnTouchListener;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.view.WindowManager;

import com.roblox.client.RotationGestureDetector.OnRotationGestureListener;
import com.roblox.client.ScaleGestureDetector.OnScaleGestureListener;

public class InputListener implements OnTouchListener,
		OnRotationGestureListener, OnScaleGestureListener, SensorEventListener
{

	class TouchInfo
    {
		private int x;
		private int y;

		private int eventType;

		int lastX;
		int lastY;
		int lastEventType;

		public int getX() {
			return x;
		}

		public void setX(int X)
        {
			this.lastX = this.x;
			this.x = X;
		}

		public int getY() {
			return y;
		}

		public void setY(int Y)
        {
			this.lastY = this.y;
			this.y = Y;
		}

		public int getEventType() {
			return eventType;
		}

		public void setEventType(int newEventType)
        {
			this.lastEventType = this.eventType;
			this.eventType = newEventType;
		}
	}

    private static final int MESSAGE_STOP = 201;
    private static final int MESSAGE_TEST_FOR_DISCONNECT = 101;
    private static final long CHECK_ELAPSED_TIME = 3000L;

	private SurfaceView mSurfaceView = null;
	private float mDisplayDensity = 0;
	private boolean mHasTouchscreen = true;
	private GestureDetector mGestureDetector = null;
	private GestureListener mGestureListener = null;
	private RotationGestureDetector mRotationDetector = null;
	private ScaleGestureDetector mScaleDetector = null;
	private SparseArray<TouchInfo> mActivePointers = new SparseArray<TouchInfo>();
	private SensorManager mSensorManager = null;
	private Sensor mAccelerometer = null;
	private Sensor mGyroscope = null;
	private boolean mUserRequested = false;
	
	private final float[] rotationQuaternion = new float[4];
	private final float[] rotationMatrix = new float[9];
	private final float[] eulerAnglesVector = new float[3];
	
	private final float[] gravity = new float[3];
	private final float[] linear_acceleration = new float[3];

    private final SparseArray mDevices = new SparseArray();
    private final Handler mDefaultHandler = new PollingMessageHandler(this);

    private InputManager mInputManager;
    private ActivityGlView mActivityGlViewRef;

	public InputListener(ActivityGlView glView, SurfaceView surfaceView)
    {
		mSurfaceView = surfaceView;
        mActivityGlViewRef = glView;
		mDisplayDensity = glView.getResources().getDisplayMetrics().density;
		mHasTouchscreen = glView.getPackageManager().hasSystemFeature("android.hardware.touchscreen");

		if (mHasTouchscreen)
        {
			mGestureListener = new GestureListener();
			mGestureDetector = new GestureDetector(glView, mGestureListener);
			mRotationDetector = new RotationGestureDetector(this);
			mScaleDetector = new ScaleGestureDetector(this);
		}

		// controller input handling
		setupControllerInput(surfaceView);

		mSensorManager = (SensorManager) glView.getBaseContext().getSystemService(Context.SENSOR_SERVICE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
        {
            mInputManager = (InputManager) glView.getBaseContext().getSystemService(Context.INPUT_SERVICE);
        }

		mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		mGyroscope = mSensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
		
		if (mGyroscope != null)
        {
			nativeSetGyroscopeEnabled(true);
		}

		if (mAccelerometer != null)
        {
			nativeSetAccelerometerEnabled(true);
		}
	}

	public void stopSensorListening()
    {
		mSensorManager.unregisterListener(this);
	}

    public void stopControllerListening()
    {
        mDefaultHandler.sendEmptyMessageDelayed(MESSAGE_STOP,
                CHECK_ELAPSED_TIME);
    }

    public void startControllerListening()
    {
        mDefaultHandler.sendEmptyMessageDelayed(MESSAGE_TEST_FOR_DISCONNECT,
                CHECK_ELAPSED_TIME);
    }

	public void startSensorListening(boolean userRequested)
    {
		if (!mUserRequested)
        {
			mUserRequested = userRequested;
		}

		if (mUserRequested)
        {
			mSensorManager.registerListener(this, mGyroscope, SensorManager.SENSOR_DELAY_GAME);
			mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_GAME);
		}
	}

    private void checkForControllerConnection(int deviceId)
    {
        // This is what Google told me to do for gamepad connect events
        // yes I am serious. WTF Google you have a lot of money make this better
        long[] timeArray = (long[]) mDevices.get(deviceId);
        if (null == timeArray)
        {
            timeArray = new long[1];
            mDevices.put(deviceId, timeArray);

            setGamepadSupportedKeys(deviceId);
            nativeGamepadConnectEvent(deviceId);
        }
        long time = SystemClock.elapsedRealtime();
        timeArray[0] = time;
    }

    private void setGamepadSupportedKeys(int deviceId)
    {
        boolean[] supportedKeys = getSupportedControllerActions(deviceId);
        List<InputDevice.MotionRange> motionRangesSupported = InputDevice.getDevice(deviceId).getMotionRanges();

        HashMap<Integer, Boolean> supportedThings = new HashMap<Integer, Boolean>();

        for (int i = 0; i < 14; ++i)
        {
            Integer keyCode = 0;
            switch (i)
            {
                case 0: keyCode = KeyEvent.KEYCODE_BUTTON_A; break;
                case 1: keyCode = KeyEvent.KEYCODE_BUTTON_B; break;
                case 2: keyCode = KeyEvent.KEYCODE_BUTTON_X; break;
                case 3: keyCode = KeyEvent.KEYCODE_BUTTON_Y; break;

                case 4: keyCode = KeyEvent.KEYCODE_DPAD_UP; break;
                case 5: keyCode = KeyEvent.KEYCODE_DPAD_DOWN; break;
                case 6: keyCode = KeyEvent.KEYCODE_DPAD_LEFT; break;
                case 7: keyCode = KeyEvent.KEYCODE_DPAD_RIGHT; break;

                case 8: keyCode = KeyEvent.KEYCODE_BUTTON_R1; break;
                case 9: keyCode = KeyEvent.KEYCODE_BUTTON_L1; break;
                case 10: keyCode = KeyEvent.KEYCODE_BUTTON_THUMBL; break;
                case 11: keyCode = KeyEvent.KEYCODE_BUTTON_THUMBR; break;

                case 12: keyCode = KeyEvent.KEYCODE_BUTTON_SELECT; break;
                case 13: keyCode = KeyEvent.KEYCODE_BUTTON_START; break;
            }

            if (i < supportedKeys.length)
            {
                supportedThings.put(keyCode, (Boolean) supportedKeys[i]);
            }
            else
            {
                supportedThings.put(keyCode, false);
            }
        }

        supportedThings.put(MotionEvent.AXIS_X, false);
        supportedThings.put(MotionEvent.AXIS_Y, false);

        supportedThings.put(MotionEvent.AXIS_Z, false);
        supportedThings.put(MotionEvent.AXIS_RZ, false);

        supportedThings.put(MotionEvent.AXIS_BRAKE, false);
        supportedThings.put(MotionEvent.AXIS_GAS, false);

        supportedThings.put(MotionEvent.AXIS_LTRIGGER, false);
        supportedThings.put(MotionEvent.AXIS_RTRIGGER, false);

        supportedThings.put(MotionEvent.AXIS_HAT_X, false);
        supportedThings.put(MotionEvent.AXIS_HAT_Y, false);

        for (InputDevice.MotionRange motionRange : motionRangesSupported)
        {
            supportedThings.put(motionRange.getAxis(), true);
        }

        Iterator it = supportedThings.entrySet().iterator();
        while (it.hasNext())
        {
            HashMap.Entry pairs = (HashMap.Entry)it.next();

            Integer keyCode = (Integer) pairs.getKey();
            Boolean supportsKeyCode = (Boolean) pairs.getValue();

            nativeSetGamepadSupportedKey(deviceId, keyCode, supportsKeyCode);

            it.remove(); // avoids a ConcurrentModificationException
        }
    }

    private boolean[] getSupportedControllerActions(int deviceId)
    {
        boolean[] keySupported = new boolean[14];

        int[] keys = new int[14];

        keys[0] = KeyEvent.KEYCODE_BUTTON_A;
        keys[1] = KeyEvent.KEYCODE_BUTTON_B;
        keys[2] = KeyEvent.KEYCODE_BUTTON_X;
        keys[3] = KeyEvent.KEYCODE_BUTTON_Y;

        keys[4] = KeyEvent.KEYCODE_DPAD_UP;
        keys[5] = KeyEvent.KEYCODE_DPAD_DOWN;
        keys[6] = KeyEvent.KEYCODE_DPAD_LEFT;
        keys[7] = KeyEvent.KEYCODE_DPAD_RIGHT;

        keys[8] = KeyEvent.KEYCODE_BUTTON_R1;
        keys[9] = KeyEvent.KEYCODE_BUTTON_L1;
        keys[10] = KeyEvent.KEYCODE_BUTTON_THUMBL;
        keys[11] = KeyEvent.KEYCODE_BUTTON_THUMBR;

        keys[12] = KeyEvent.KEYCODE_BUTTON_SELECT;
        keys[13] = KeyEvent.KEYCODE_BUTTON_START;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
        {
            final InputDevice device = mInputManager.getInputDevice(deviceId);
            keySupported = device.hasKeys(keys);

            return keySupported;
        }
        else // we will just assume you got the keys (no way to check prior to KitKat)
        {
            for (int i = 0; i < keySupported.length; ++i)
            {
                keySupported[i] = true;
            }
        }

        return keySupported;
    }


	private void setupControllerInput(SurfaceView surfaceView)
    {
        mDefaultHandler.sendEmptyMessageDelayed(MESSAGE_TEST_FOR_DISCONNECT,
                CHECK_ELAPSED_TIME);

		surfaceView.setOnKeyListener(new View.OnKeyListener() {
                                         @Override
                                         public boolean onKey(View v, int keyCode, KeyEvent event) {
                                             boolean handled = false;

                                             switch (keyCode) {
                                                 case KeyEvent.KEYCODE_MENU: {
                                                     InputDevice inputDevice = event.getDevice();
                                                     int hasFlags = InputDevice.SOURCE_GAMEPAD | InputDevice.SOURCE_JOYSTICK;
                                                     boolean isGamepad = (inputDevice.getSources() & hasFlags) == hasFlags;
                                                     if (isGamepad && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
                                                     {
                                                         int[] keys = new int[1];
                                                         keys[0] = KeyEvent.KEYCODE_BUTTON_START;
                                                         boolean[] keySupported = new boolean[1];
                                                         keySupported = inputDevice.hasKeys(keys);
                                                         if (!keySupported[0])
                                                         {
                                                             // this is a giant hack for fire tv gamepad
                                                             // does not have start button, so menu button
                                                             // must be able to open the menu
                                                             keyCode = KeyEvent.KEYCODE_BUTTON_START;
                                                         }
                                                     }

                                                     if(keyCode == KeyEvent.KEYCODE_MENU)
                                                     {
                                                         return handled;
                                                     }
                                                 }
                                                 case KeyEvent.KEYCODE_BUTTON_A:
                                                 case KeyEvent.KEYCODE_BUTTON_B:
                                                 case KeyEvent.KEYCODE_BUTTON_X:
                                                 case KeyEvent.KEYCODE_BUTTON_Y:
                                                 case KeyEvent.KEYCODE_BUTTON_THUMBR:
                                                 case KeyEvent.KEYCODE_BUTTON_THUMBL:
                                                 case KeyEvent.KEYCODE_BUTTON_R1:
                                                 case KeyEvent.KEYCODE_BUTTON_R2:
                                                 case KeyEvent.KEYCODE_BUTTON_L1:
                                                 case KeyEvent.KEYCODE_BUTTON_L2:
                                                 case KeyEvent.KEYCODE_BUTTON_START:
                                                 case KeyEvent.KEYCODE_BUTTON_SELECT:
                                                 case KeyEvent.KEYCODE_DPAD_DOWN:
                                                 case KeyEvent.KEYCODE_DPAD_UP:
                                                 case KeyEvent.KEYCODE_DPAD_LEFT:
                                                 case KeyEvent.KEYCODE_DPAD_RIGHT:
                                                     handled = true;
                                                     int buttonState = 0;
                                                     if (event.getAction() == MotionEvent.ACTION_DOWN) {
                                                         buttonState = 1;
                                                     }

                                                     int deviceId = event.getDeviceId();
                                                     checkForControllerConnection(deviceId);
                                                     nativeGamepadButtonEvent(deviceId, keyCode, buttonState);
                                                     break;
                                             }

                                             return handled;
                                         }
                                     }

        );

            surfaceView.setOnGenericMotionListener(new View.OnGenericMotionListener()

            {
                float[] axisValues = new float[8];

                private float getCenteredAxis(MotionEvent event, InputDevice device, int axis, int historyPos) {
                    final InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());

                    // A joystick at rest does not always report an absolute position of
                    // (0,0). Use the getFlat() method to determine the range of values
                    // bounding the joystick axis center.
                    if (range != null) {
                        final float flat = range.getFlat();
                        final float value = historyPos < 0 ? event.getAxisValue(axis) : event.getHistoricalAxisValue(axis, historyPos);

                        // Ignore axis values that are within the 'flat' region of the
                        // joystick axis center.
                        if (Math.abs(value) > flat) {
                            return value;
                        }
                    }
                    return 0;
                }

                private void processJoystickInput(MotionEvent event, int historyPos) {
                    InputDevice mInputDevice = event.getDevice();

                    // Calculate the horizontal distance to move by
                    // using the input value from one of these physical controls:
                    // the left control stick, hat axis, or the right control stick.
                    axisValues[0] = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_X, historyPos);
                    axisValues[1] = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_Y, historyPos);

                    axisValues[2] = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_Z, historyPos);
                    axisValues[3] = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_RZ, historyPos);

                    // following two values are like so because Android likes to be inconsistent
                    // with keycodes, some gamepads fire GAS/BRAKE, some fire L/RTRIGGER, some do both :(
                    float lTriggerValue = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_LTRIGGER, historyPos);
                    float brakeValue = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_BRAKE, historyPos);
                    axisValues[4] = java.lang.Math.max(lTriggerValue, brakeValue);

                    float rTriggerValue = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_RTRIGGER, historyPos);
                    float gasValue = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_GAS, historyPos);
                    axisValues[5] = java.lang.Math.max(rTriggerValue, gasValue);

                    axisValues[6] = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_HAT_X, historyPos);
                    axisValues[7] = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_HAT_Y, historyPos);

                }

                @Override
                public boolean onGenericMotion(View v, MotionEvent event) {
                    if (event.getSource() != InputDevice.SOURCE_GAMEPAD &&
                            event.getSource() != InputDevice.SOURCE_JOYSTICK) {
                        return false;
                    }

                    // Process all historical movement samples in the batch
                    final int historySize = event.getHistorySize();

                    // Process the movements starting from the
                    // earliest historical position in the batch
                    for (int i = 0; i < historySize; i++) {
                        // Process the event at historical position i
                        processJoystickInput(event, i);
                    }

                    // Process the current movement sample in the batch (position -1)
                    processJoystickInput(event, -1);

                    int deviceId = event.getDeviceId();
                    checkForControllerConnection(deviceId);

                    for (int i = 0; i < axisValues.length; i++) {
                        int actionType = -1;
                        switch (i) {
                            case 0:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_X, axisValues[0], -axisValues[1], 0);
                                break;
                            case 1:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_Y, axisValues[0], -axisValues[1], 0);
                                break;
                            case 2:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_Z, axisValues[2], -axisValues[3], 0);
                                break;
                            case 3:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_RZ, axisValues[2], -axisValues[3], 0);
                                break;
                            case 4:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_LTRIGGER, 0, 0, axisValues[4]);
                                break;
                            case 5:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_RTRIGGER, 0, 0, axisValues[5]);
                                break;
                            case 6:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_HAT_X, 0, 0, axisValues[6]);
                                break;
                            case 7:
                                nativeGamepadAxisEvent(deviceId, MotionEvent.AXIS_HAT_Y, 0, 0, -axisValues[7]);
                                break;
                            default:
                                break;
                        }
                    }

                    return true;
                }
            });
	}

	public boolean onTouch(final View view, final MotionEvent event) {
		if (!mHasTouchscreen)
			return false;

		// get pointer index from the event object
		int pointerIndex = event.getActionIndex();

		// get pointer ID
		int pointerId = event.getPointerId(pointerIndex);

		// get masked (not specific to a pointer) action
		int maskedAction = event.getActionMasked();

		switch (maskedAction) {
		case MotionEvent.ACTION_DOWN:
		case MotionEvent.ACTION_POINTER_DOWN: {
			// We have a new pointer. Lets add it to the list of pointers
			TouchInfo info = new TouchInfo();
			info.setX((int) (event.getX(pointerIndex) / mDisplayDensity));
			info.setY((int) (event.getY(pointerIndex) / mDisplayDensity));
			info.setEventType(0);
			mActivePointers.put(pointerId, info);
			break;
		}
		case MotionEvent.ACTION_MOVE: {
			// a pointer was moved
			for (int j = 0; j < mActivePointers.size(); j++) {
				try {
					int touchId = mActivePointers.keyAt(j);
					int movePointerIndex = event.findPointerIndex(touchId);

					TouchInfo info = mActivePointers.get(touchId);
					info.setX((int) (event.getX(movePointerIndex) / mDisplayDensity));
					info.setY((int) (event.getY(movePointerIndex) / mDisplayDensity));
					info.setEventType(1);

					if (touchId == mGestureListener.longPressEventId) {
						nativePassLongPressGesture(1, info.getX(), info.getY());
					}
				} catch (IllegalArgumentException e) {
					// Rare Analytics crash
				}
			}
			break;
		}
		case MotionEvent.ACTION_UP:
		case MotionEvent.ACTION_POINTER_UP:
		case MotionEvent.ACTION_CANCEL: {
			TouchInfo info = mActivePointers.get(pointerId);

            if(AndroidAppSettings.EnableInputListenerActivePointerNullFix() && info == null) {
                // Fix a possible null pointer TouchInfo seen in Crashlytics
                int ex = (int) (event.getX(pointerIndex) / mDisplayDensity);
                int ey = (int) (event.getY(pointerIndex) / mDisplayDensity);
                if (pointerId == mGestureListener.longPressEventId) {
                    mGestureListener.longPressEventId = -1;
                    nativePassLongPressGesture(2, ex, ey);
                } else if (pointerId == mGestureListener.panEventId) {
                    mGestureListener.endPanGesture();
                }
            } else {
                info.setEventType(2);
                if (pointerId == mGestureListener.longPressEventId) {
                    mGestureListener.longPressEventId = -1;
                    nativePassLongPressGesture(2, info.getX(), info.getY());
                } else if (pointerId == mGestureListener.panEventId) {
                    mGestureListener.endPanGesture();
                }
            }
			break;
		}
		}

		List<Integer> idsToRemove = new ArrayList<Integer>();

		for (int size = mActivePointers.size(), i = 0; i < size; i++) {
			int touchId = mActivePointers.keyAt(i);
			TouchInfo point = mActivePointers.valueAt(i);
			boolean processEvent = true;

			if (point.getEventType() == 2) {
				idsToRemove.add(touchId);
			} else if (point.getEventType() == point.lastEventType) {
				if (point.getX() == point.lastX && point.getY() == point.lastY) {

					processEvent = false;
				}
			} else if (point.getEventType() == 1 && point.lastEventType == 0) {
				if (point.getX() == point.lastX && point.getY() == point.lastY) {
					processEvent = false;
				}
			}

			if (processEvent
					&& ActivityGlView.getSurfaceState() == ActivityGlView.SURFACE_CREATED) {
				int width = (int) (mSurfaceView.getWidth() / mDisplayDensity);
				int height = (int) (mSurfaceView.getHeight() / mDisplayDensity);

				point.lastX = point.getX();
				point.lastY = point.getY();

				nativePassInput(touchId, point.getX(), point.getY(),
						point.getEventType(), width, height);
			} else if (ActivityGlView.getSurfaceState() != ActivityGlView.SURFACE_CREATED) {
				Log.w("InputListener",
						"nativePassInput not ready or already passed event");
			}
		}

		for (Iterator<Integer> iter = idsToRemove.iterator(); iter.hasNext();) {
			Integer key = iter.next();
			mActivePointers.remove(key);
		}

		// send event to gesture detectors
		mScaleDetector.onTouchEvent(event);
		mGestureDetector.onTouchEvent(event);
		mRotationDetector.onTouchEvent(event);

		return true;
	}

	private final class GestureListener implements
			GestureDetector.OnGestureListener {

		private static final int SWIPE_THRESHOLD = 100;
		private static final int SWIPE_VELOCITY_THRESHOLD = 100;
		public int longPressEventId = -1;
		public int panEventId = -1;

		// used for pan gesture
		private int xCurrPanPos = 0;
		private int yCurrPanPos = 0;
		private int xPanTranslation = 0;
		private int yPanTranslation = 0;

		@Override
		public boolean onDown(MotionEvent e) {
			return false;
		}

		@Override
		public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
				float velocityY) {
			try {
				float diffY = e2.getY() - e1.getY();
				float diffX = e2.getX() - e1.getX();
				if (Math.abs(diffX) > Math.abs(diffY)) {
					if (Math.abs(diffX) > SWIPE_THRESHOLD
							&& Math.abs(velocityX) > SWIPE_VELOCITY_THRESHOLD) {
						if (diffX > 0) {
							onSwipeRight();
						} else {
							onSwipeLeft();
						}
					}
				} else {
					if (Math.abs(diffY) > SWIPE_THRESHOLD
							&& Math.abs(velocityY) > SWIPE_VELOCITY_THRESHOLD) {
						if (diffY > 0) {
							onSwipeBottom();
						} else {
							onSwipeTop();
						}
					}
				}
			} catch (Exception exception) {
				exception.printStackTrace();
			}
			return false;
		}

		@Override
		public void onShowPress(MotionEvent e) {
			// need to implement this method, but don't need to use it at the
			// moment
		}

		@Override
		public boolean onSingleTapUp(MotionEvent e) {
			int xPos = (int) (e.getX() / mDisplayDensity);
			int yPos = (int) (e.getY() / mDisplayDensity);
			nativePassTapGesture(xPos, yPos);
			return false;
		}

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2,
				float distanceX, float distanceY) {
			boolean isNewEvent = false;
			if (panEventId == -1)
				isNewEvent = true;

			// get pointer index from the event object
			int pointerIndex = e2.getActionIndex();
			// get pointer ID
			panEventId = e2.getPointerId(pointerIndex);

			xCurrPanPos = (int) (e2.getX() / mDisplayDensity);
			yCurrPanPos = (int) (e2.getY() / mDisplayDensity);

			int xInitPos = (int) (e1.getX() / mDisplayDensity);
			int yInitPos = (int) (e1.getY() / mDisplayDensity);

			xPanTranslation = xCurrPanPos - xInitPos;
			yPanTranslation = yCurrPanPos - yInitPos;

			if (isNewEvent) {
				nativePassPanGesture(0, xCurrPanPos, yCurrPanPos,
						xPanTranslation, yPanTranslation, 0.0f);
			} else {
				nativePassPanGesture(1, xCurrPanPos, yCurrPanPos,
						xPanTranslation, yPanTranslation, 0.0f);
			}
			return false;
		}

		public void endPanGesture() {
			panEventId = -1;
			nativePassPanGesture(2, xCurrPanPos, yCurrPanPos, xPanTranslation,
					yPanTranslation, 0.0f);
		}

		@Override
		public void onLongPress(MotionEvent e) {
			// get pointer index from the event object
			int pointerIndex = e.getActionIndex();
			// get pointer ID
			longPressEventId = e.getPointerId(pointerIndex);

			int xPos = (int) (e.getX() / mDisplayDensity);
			int yPos = (int) (e.getY() / mDisplayDensity);
			nativePassLongPressGesture(0, xPos, yPos);
		}
	}

	public void onSwipeRight() {
		nativePassSwipeGesture(0, 1);
	}

	public void onSwipeBottom() {
		nativePassSwipeGesture(1, 1);
	}

	public void onSwipeLeft() {
		nativePassSwipeGesture(2, 1);
	}

	public void onSwipeTop() {
		nativePassSwipeGesture(3, 1);
	}

	@Override
	public void OnRotation(RotationGestureDetector rotationDetector) {
		float angle = rotationDetector.getAngle();
		int state = rotationDetector.getState();
		Point point1 = rotationDetector.getPoint1();
		Point point2 = rotationDetector.getPoint2();

		nativePassRotateGesture(state, angle, 0.0f, point1.x, point1.y,
				point2.x, point2.y);
	}

	@Override
	public void OnScale(ScaleGestureDetector scaleDetector) {
		Point point1 = scaleDetector.getPoint1();
		Point point2 = scaleDetector.getPoint2();
		float scale = scaleDetector.getScale();
		int state = scaleDetector.getState();
		nativePassPinchGesture(state, scale, 0.0f, (int)(point1.x/mDisplayDensity), (int)(point1.y/mDisplayDensity),
                (int)(point2.x/mDisplayDensity), (int)(point2.y/mDisplayDensity));
	}
    
    public static float[] adjustAccelOrientation(int displayRotation, float[] eventValues)
    {
        float[] adjustedValues = new float[3];
        
        final int axisSwap[][] = {
            {  1,  -1,  0,  1  },     // ROTATION_0
            {-1,  -1,  1,  0  },      // ROTATION_90
            {-1,    1,  0,  1  },     // ROTATION_180
            {  1,    1,  1,  0  }  }; // ROTATION_270
        
        final int[] as = axisSwap[displayRotation];
        adjustedValues[0]  =  (float)as[0] * eventValues[ as[2] ];
        adjustedValues[1]  =  (float)as[1] * eventValues[ as[3] ];
        adjustedValues[2]  =  eventValues[2];
        
        return adjustedValues;
    }

	@Override
	public void onSensorChanged(SensorEvent event)
    {
        android.view.Display display = ((WindowManager) mActivityGlViewRef.getBaseContext().getSystemService(android.content.Context.WINDOW_SERVICE)).getDefaultDisplay();
        int rotationOrientation = display.getRotation();
        
		if (event.sensor.getType() == Sensor.TYPE_ROTATION_VECTOR)
		{
            float[] values_trans = adjustAccelOrientation(rotationOrientation, event.values);
            
			SensorManager.getQuaternionFromVector(rotationQuaternion, values_trans);
			SensorManager.getRotationMatrixFromVector(rotationMatrix, rotationQuaternion);
			SensorManager.getOrientation(rotationMatrix, eulerAnglesVector);
			  
			// this passes the same readout as iOS, which is good
			nativePassGyroscopeChange(eulerAnglesVector[0], eulerAnglesVector[1], eulerAnglesVector[2], rotationQuaternion[0], rotationQuaternion[2], rotationQuaternion[3], rotationQuaternion[1]);
		}
		else if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
		{

			// In this example, alpha is calculated as t / (t + dT),
			// where t is the low-pass filter's time-constant and
			// dT is the event delivery rate.
			
			final float alpha = 0.8f;
			
			// Isolate the force of gravity with the low-pass filter.
			gravity[0] = alpha * gravity[0] + (1 - alpha) * event.values[0];
			gravity[1] = alpha * gravity[1] + (1 - alpha) * event.values[1];
			gravity[2] = alpha * gravity[2] + (1 - alpha) * event.values[2];
			
			// Remove the gravity contribution with the high-pass filter.
			linear_acceleration[0] = event.values[0] - gravity[0];
			linear_acceleration[1] = event.values[1] - gravity[1];
			linear_acceleration[2] = event.values[2] - gravity[2];
            
            float[] linear_acceleration_trans = adjustAccelOrientation(rotationOrientation, linear_acceleration);
            float[] gravity_trans = adjustAccelOrientation(rotationOrientation, gravity);
			
			// this passes the same readout as iOS, which is good
			nativePassAccelerometerChange(-linear_acceleration_trans[1], linear_acceleration_trans[2], linear_acceleration_trans[0]);
			nativePassGravityChange(-gravity_trans[1], -gravity_trans[2], gravity_trans[0]);
		}
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
	}

    private static native void nativeGamepadConnectEvent(int deviceId);
    private static native void nativeGamepadDisconnectEvent(int deviceId);
    private static native void nativeGamepadButtonEvent(int deviceId, int keyCode, int buttonState);
    private static native void nativeGamepadAxisEvent(int deviceId, int actionType, float newValueX, float newValueY, float newValueZ);
    private static native void nativeSetGamepadSupportedKey(int deviceId, int keyCode, boolean supported);
	
	private static native void nativePassInput(int id, int posX, int posY,
			int inputState, int windowWidth, int windowHeight);

	private static native void nativePassPinchGesture(int state,
			float pinchScale, float velocity, int position1X, int position1Y,
			int position2X, int position2Y);
	private static native void nativePassTapGesture(int positionX, int positionY);
	private static native void nativePassSwipeGesture(int swipeDirection,
			int numOfTouches);
	private static native void nativePassLongPressGesture(int state,
			int positionX, int positionY);
	private static native void nativePassPanGesture(int state, int positionX,
			int positionY, float totalTranslationX, float totalTranslationY,
			float velocity);
	private static native void nativePassRotateGesture(int state,
			float rotation, float velocity, int position1X, int position1Y,
			int position2X, int position2Y);

	private static native void nativePassGravityChange(float x, float y,
			float z);
	private static native void nativePassAccelerometerChange(float x, float y,
			float z);
	private static native void nativePassGyroscopeChange(float eulerX, float eulerY,
			float eulerZ, float quaternionX, float quaternionY, float quaternionZ, float quaternionW);
	private static native void nativeSetAccelerometerEnabled(boolean enabled);
	private static native void nativeSetGyroscopeEnabled(boolean enabled);

    // This is what Google told me to do for gamepad disconnect events
    // yes I am serious. WTF Google you have a lot of money make this better
    class PollingMessageHandler extends Handler
    {
        private final WeakReference mInputListener;

        PollingMessageHandler(InputListener im)
        {
            mInputListener = new WeakReference(im);
        }

        @Override
        public void handleMessage(Message msg)
        {
            switch (msg.what)
            {
                case MESSAGE_TEST_FOR_DISCONNECT:
                    InputListener imv = (InputListener) mInputListener.get();
                    if (null != imv)
                    {
                        long time = SystemClock.elapsedRealtime();
                        int size = imv.mDevices.size();
                        for (int i = 0; i < size; i++)
                        {
                            long[] lastContact = (long[]) imv.mDevices.valueAt(i);
                            if (null != lastContact)
                            {
                                if (time - lastContact[0] > CHECK_ELAPSED_TIME)
                                {
                                    // check to see if the device has been
                                    // disconnected
                                    int id = imv.mDevices.keyAt(i);
                                    if (null == InputDevice.getDevice(id))
                                    {
                                        nativeGamepadDisconnectEvent(id);
                                        imv.mDevices.remove(id);
                                    }
                                    else
                                    {
                                        lastContact[0] = time;
                                    }
                                }
                            }
                        }
                        sendEmptyMessageDelayed(MESSAGE_TEST_FOR_DISCONNECT,
                                CHECK_ELAPSED_TIME);
                    }
                    break;
            }
        }
    }
}
