package com.genymobile.scrcpy.wrappers;

import com.genymobile.scrcpy.Ln;

import android.content.ClipData;
import android.content.IOnPrimaryClipChangedListener;
import android.os.Build;
import android.os.IInterface;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class ClipboardManager {
    private final Object manager;
    private Method getPrimaryClipMethod;
    private Method setPrimaryClipMethod;
    private Method addPrimaryClipChangedListener;

    public ClipboardManager(IInterface manager) {
        this.manager = manager;
    }

    private Method getGetPrimaryClipMethod() throws NoSuchMethodException {
        if (getPrimaryClipMethod == null) {
            for (Method method : manager.getClass().getMethods()) {
                if (method.getName().equals("getPrimaryClip")) {
                    getPrimaryClipMethod = method;
                    break;
                }
            }
        }
        return getPrimaryClipMethod;
    }

    private Method getSetPrimaryClipMethod() throws NoSuchMethodException {
        if (setPrimaryClipMethod == null) {
            for (Method method : manager.getClass().getMethods()) {
                if (method.getName().equals("setPrimaryClip")) {
                    setPrimaryClipMethod = method;
                    break;
                }
            }
        }
        return setPrimaryClipMethod;
    }

    private static ClipData getPrimaryClip(Method method, IInterface manager) throws InvocationTargetException, IllegalAccessException {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            return (ClipData) method.invoke(manager, ServiceManager.PACKAGE_NAME);
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            return (ClipData) method.invoke(manager, ServiceManager.PACKAGE_NAME, ServiceManager.USER_ID);
        }
        return (ClipData) method.invoke(manager, ServiceManager.PACKAGE_NAME, null, ServiceManager.USER_ID);
    }

    private static void setPrimaryClip(Method method, IInterface manager, ClipData clipData) throws InvocationTargetException, IllegalAccessException {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            method.invoke(manager, clipData, ServiceManager.PACKAGE_NAME);
        } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            method.invoke(manager, clipData, ServiceManager.PACKAGE_NAME, ServiceManager.USER_ID);
        } else {
            method.invoke(manager, clipData, ServiceManager.PACKAGE_NAME, null, ServiceManager.USER_ID);
        }
    }

    public CharSequence getText() {
        try {
            Method method = getGetPrimaryClipMethod();
            ClipData clipData = getPrimaryClip(method, manager);
            if (clipData == null || clipData.getItemCount() == 0) {
                return null;
            }
            return clipData.getItemAt(0).getText();
        } catch (InvocationTargetException | IllegalAccessException | NoSuchMethodException e) {
            Ln.e("Could not invoke method", e);
            return null;
        }
    }

    public boolean setText(CharSequence text) {
        try {
            Method method = getSetPrimaryClipMethod();
            ClipData clipData = ClipData.newPlainText(null, text);
            setPrimaryClip(method, manager, clipData);
            return true;
        } catch (InvocationTargetException | IllegalAccessException | NoSuchMethodException e) {
            Ln.e("Could not invoke method", e);
            return false;
        }
    }

    private static void addPrimaryClipChangedListener(Method method, IInterface manager, IOnPrimaryClipChangedListener listener) throws InvocationTargetException, IllegalAccessException {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            method.invoke(manager, listener, ServiceManager.PACKAGE_NAME);
        } else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            method.invoke(manager, listener, ServiceManager.PACKAGE_NAME, ServiceManager.USER_ID);
        } else {
            method.invoke(manager, listener, ServiceManager.PACKAGE_NAME, null, ServiceManager.USER_ID);
        }
    }

    private Method getAddPrimaryClipChangedListener() throws NoSuchMethodException {
        if (addPrimaryClipChangedListener == null) {
            for (Method method : manager.getClass().getMethods()) {
                if (method.getName().equals("addPrimaryClipChangedListener")) {
                    addPrimaryClipChangedListener = method;
                    break;
                }
            }
        }
        return addPrimaryClipChangedListener;
    }

    public boolean addPrimaryClipChangedListener(IOnPrimaryClipChangedListener listener) {
        try {
            Method method = getAddPrimaryClipChangedListener();
            addPrimaryClipChangedListener(method, manager, listener);
            return true;
        } catch (InvocationTargetException | IllegalAccessException | NoSuchMethodException e) {
            Ln.e("Could not invoke method", e);
            return false;
        }
    }
}
