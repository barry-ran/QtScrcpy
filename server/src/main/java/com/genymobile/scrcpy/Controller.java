package com.genymobile.scrcpy;

import com.genymobile.scrcpy.wrappers.InputManager;

import android.graphics.Point;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.io.IOException;
import java.util.Vector;

public class Controller {

    private final Device device;
    private final DesktopConnection connection;
    private final DeviceMessageSender sender;

    private final KeyCharacterMap charMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);

    private long lastMouseDown;
    private Vector<MotionEvent.PointerProperties> pointerProperties = new Vector<MotionEvent.PointerProperties>();
    private Vector<MotionEvent.PointerCoords> pointerCoords = new Vector<MotionEvent.PointerCoords>();

    public Controller(Device device, DesktopConnection connection) {
        this.device = device;
        this.connection = connection;
        sender = new DeviceMessageSender(connection);
    }

    private int getPointer(int id) {
        for (int i = 0; i < pointerProperties.size(); i++) {
            if (id == pointerProperties.get(i).id) {
                return i;
            }
        }

        MotionEvent.PointerProperties props = new MotionEvent.PointerProperties();
        props.id = id;
        props.toolType = MotionEvent.TOOL_TYPE_FINGER;
        pointerProperties.addElement(props);

        MotionEvent.PointerCoords  coords = new MotionEvent.PointerCoords();
        coords.orientation = 0;
        coords.pressure = 1;
        coords.size = 1;
        pointerCoords.addElement(coords);
        return pointerProperties.size() - 1;
    }

    private void releasePointer(int id) {
        int index = -1;
        for (int i = 0; i < pointerProperties.size(); i++) {
            if (id == pointerProperties.get(i).id) {
                index = i;
                break;
            }
        }

        if ( -1 != index) {
            pointerProperties.remove(index);
            pointerCoords.remove(index);
        }
    }

    private void setPointerCoords(int id, Point point) {
        int index = -1;
        for (int i = 0; i < pointerProperties.size(); i++) {
            if (id == pointerProperties.get(i).id) {
                index = i;
                break;
            }
        }

        if ( -1 != index) {
            MotionEvent.PointerCoords coords = pointerCoords.get(index);
            coords.x = point.x;
            coords.y = point.y;
        }
    }

    private void setScroll(int id, int hScroll, int vScroll) {
        int index = -1;
        for (int i = 0; i < pointerProperties.size(); i++) {
            if (id == pointerProperties.get(i).id) {
                index = i;
                break;
            }
        }

        if ( -1 != index) {
            MotionEvent.PointerCoords coords = pointerCoords.get(index);
            coords.setAxisValue(MotionEvent.AXIS_HSCROLL, hScroll);
            coords.setAxisValue(MotionEvent.AXIS_VSCROLL, vScroll);
        }
    }

    public DeviceMessageSender getSender() {
        return sender;
    }

    @SuppressWarnings("checkstyle:MagicNumber")
    public void control() throws IOException {
        // on start, power on the device
        if (!device.isScreenOn()) {
            injectKeycode(KeyEvent.KEYCODE_POWER);

            // dirty hack
            // After POWER is injected, the device is powered on asynchronously.
            // To turn the device screen off while mirroring, the client will send a message that
            // would be handled before the device is actually powered on, so its effect would
            // be "canceled" once the device is turned back on.
            // Adding this delay prevents to handle the message before the device is actually
            // powered on.
            SystemClock.sleep(500);
        }

        while (true) {
            handleEvent();
        }
    }

    private void handleEvent() throws IOException {
        ControlMessage msg = connection.receiveControlMessage();
        switch (msg.getType()) {
            case ControlMessage.TYPE_INJECT_KEYCODE:
                injectKeycode(msg.getAction(), msg.getKeycode(), msg.getMetaState());
                break;
            case ControlMessage.TYPE_INJECT_TEXT:
                injectText(msg.getText());
                break;
            case ControlMessage.TYPE_INJECT_MOUSE:
                injectMouse(msg.getAction(), msg.getButtons(), msg.getPosition());
                break;
            case ControlMessage.TYPE_INJECT_TOUCH:
                injectTouch(msg.getId(), msg.getAction(), msg.getPosition());
                break;
            case ControlMessage.TYPE_INJECT_SCROLL:
                injectScroll(msg.getPosition(), msg.getHScroll(), msg.getVScroll());
                break;
            case ControlMessage.TYPE_BACK_OR_SCREEN_ON:
                pressBackOrTurnScreenOn();
                break;
            case ControlMessage.TYPE_EXPAND_NOTIFICATION_PANEL:
                device.expandNotificationPanel();
                break;
            case ControlMessage.TYPE_COLLAPSE_NOTIFICATION_PANEL:
                device.collapsePanels();
                break;
            case ControlMessage.TYPE_GET_CLIPBOARD:
                String clipboardText = device.getClipboardText();
                sender.pushClipboardText(clipboardText);
                break;
            case ControlMessage.TYPE_SET_CLIPBOARD:
                device.setClipboardText(msg.getText());
                break;
            case ControlMessage.TYPE_SET_SCREEN_POWER_MODE:
                device.setScreenPowerMode(msg.getAction());
                break;
            default:
                // do nothing
        }
    }

    private boolean injectKeycode(int action, int keycode, int metaState) {
        return injectKeyEvent(action, keycode, 0, metaState);
    }

    private boolean injectChar(char c) {
        String decomposed = KeyComposition.decompose(c);
        char[] chars = decomposed != null ? decomposed.toCharArray() : new char[]{c};
        KeyEvent[] events = charMap.getEvents(chars);
        if (events == null) {
            return false;
        }
        for (KeyEvent event : events) {
            if (!injectEvent(event)) {
                return false;
            }
        }
        return true;
    }

    private int injectText(String text) {
        int successCount = 0;
        for (char c : text.toCharArray()) {
            if (!injectChar(c)) {
                Ln.w("Could not inject char u+" + String.format("%04x", (int) c));
                continue;
            }
            successCount++;
        }
        return successCount;
    }

    private boolean injectTouch(int id, int action, Position position) {
        if (action != MotionEvent.ACTION_DOWN
                && action != MotionEvent.ACTION_UP
                && action != MotionEvent.ACTION_MOVE) {
            Ln.w("Unsupported action: " + action);
            return false;
        }
        if (id < 0 || id > 9) {
            Ln.w("Unsupported id[0-9]: " + id);
            return false;
        }

        int index = getPointer(id);
        int convertAction = action;
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                if (1 != pointerProperties.size()) {
                    convertAction = (index << 8) | MotionEvent.ACTION_POINTER_DOWN;
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (1 != pointerProperties.size()) {
                    convertAction = (index << 8) | convertAction;
                }
                break;
            case MotionEvent.ACTION_UP:
                if (1 != pointerProperties.size()) {
                    convertAction = (index << 8) | MotionEvent.ACTION_POINTER_UP;
                }
                break;
        }

        Point point = device.getPhysicalPoint(position);
        if (point == null) {
            // ignore event
            return false;
        }

        if (pointerProperties.isEmpty()) {
            // ignore event
            return false;
        }
        setPointerCoords(id, point);
        MotionEvent.PointerProperties[] props = pointerProperties.toArray(new MotionEvent.PointerProperties[pointerProperties.size()]);
        MotionEvent.PointerCoords[] coords = pointerCoords.toArray(new MotionEvent.PointerCoords[pointerCoords.size()]);
        MotionEvent event = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), convertAction,
                pointerProperties.size(), props, coords, 0, 0, 1f, 1f, 0, 0,
                InputDevice.SOURCE_TOUCHSCREEN, 0);

        if (action == MotionEvent.ACTION_UP) {
            releasePointer(id);
        }
        return injectEvent(event);
    }

    private boolean injectMouse(int action, int buttons, Position position) {
        long now = SystemClock.uptimeMillis();
        if (action == MotionEvent.ACTION_DOWN) {
            getPointer(0);
            lastMouseDown = now;
        }
        Point point = device.getPhysicalPoint(position);
        if (point == null) {
            // ignore event
            return false;
        }

        if (pointerProperties.isEmpty()) {
            // ignore event
            return false;
        }
        setPointerCoords(0, point);
        MotionEvent.PointerProperties[] props = pointerProperties.toArray(new MotionEvent.PointerProperties[pointerProperties.size()]);
        MotionEvent.PointerCoords[] coords = pointerCoords.toArray(new MotionEvent.PointerCoords[pointerCoords.size()]);
        MotionEvent event = MotionEvent.obtain(lastMouseDown, now, action,
                pointerProperties.size(), props, coords, 0, buttons, 1f, 1f, 0, 0,
                InputDevice.SOURCE_TOUCHSCREEN, 0);

        if (action == MotionEvent.ACTION_UP) {
            releasePointer(0);
        }
        return injectEvent(event);
    }

    private boolean injectScroll(Position position, int hScroll, int vScroll) {
        long now = SystemClock.uptimeMillis();
        Point point = device.getPhysicalPoint(position);
        if (point == null) {
            // ignore event
            return false;
        }

        // init
        MotionEvent.PointerProperties[] props = {new MotionEvent.PointerProperties()};
        props[0].id = 0;
        props[0].toolType = MotionEvent.TOOL_TYPE_FINGER;
        MotionEvent.PointerCoords[] coords = {new MotionEvent.PointerCoords()};
        coords[0].orientation = 0;
        coords[0].pressure = 1;
        coords[0].size = 1;

        // set data
        coords[0].x = point.x;
        coords[0].y = point.y;
        coords[0].setAxisValue(MotionEvent.AXIS_HSCROLL, hScroll);
        coords[0].setAxisValue(MotionEvent.AXIS_VSCROLL, vScroll);

        MotionEvent event = MotionEvent.obtain(lastMouseDown, now, MotionEvent.ACTION_SCROLL, 1, props, coords, 0, 0, 1f, 1f, 0,
                0, InputDevice.SOURCE_MOUSE, 0);
        return injectEvent(event);
    }

    private boolean injectKeyEvent(int action, int keyCode, int repeat, int metaState) {
        long now = SystemClock.uptimeMillis();
        KeyEvent event = new KeyEvent(now, now, action, keyCode, repeat, metaState, KeyCharacterMap.VIRTUAL_KEYBOARD, 0, 0,
                InputDevice.SOURCE_KEYBOARD);
        return injectEvent(event);
    }

    private boolean injectKeycode(int keyCode) {
        return injectKeyEvent(KeyEvent.ACTION_DOWN, keyCode, 0, 0)
                && injectKeyEvent(KeyEvent.ACTION_UP, keyCode, 0, 0);
    }

    private boolean injectEvent(InputEvent event) {
        return device.injectInputEvent(event, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
    }

    private boolean pressBackOrTurnScreenOn() {
        int keycode = device.isScreenOn() ? KeyEvent.KEYCODE_BACK : KeyEvent.KEYCODE_POWER;
        return injectKeycode(keycode);
    }
}
