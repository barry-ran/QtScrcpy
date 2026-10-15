#!/bin/bash

echo Begin Runing...
SNDCPY_PORT=28200
SNDCPY_APK=sndcpy.apk
ADB=./adb

serial=()
if [[ $# -ge 2 ]]; then
    serial=(-s "$1")
    SNDCPY_PORT=$2
fi

echo "Waiting for device $1..."
if ! $ADB "${serial[@]}" wait-for-device; then
    echo "Failed waiting for device $1"
    exit 1
fi
echo "Find device $1"

sndcpy_installed=$($ADB "${serial[@]}" shell pm path com.rom1v.sndcpy)
if [[ $sndcpy_installed == "" ]]; then
    echo Install $SNDCPY_APK... 
    $ADB "${serial[@]}" uninstall com.rom1v.sndcpy || echo uninstall failed
    if ! $ADB "${serial[@]}" install -t -r -g "$SNDCPY_APK"; then
        echo "Failed to install $SNDCPY_APK"
        exit 1
    fi
    echo Install $SNDCPY_APK success
fi

echo Request PROJECT_MEDIA permission...
if ! $ADB "${serial[@]}" shell appops set com.rom1v.sndcpy PROJECT_MEDIA allow; then
    echo "Failed to grant audio capture permission"
    exit 1
fi

echo Forward port $SNDCPY_PORT...
$ADB "${serial[@]}" forward --remove tcp:$SNDCPY_PORT >/dev/null 2>&1 || true
if ! $ADB "${serial[@]}" forward tcp:$SNDCPY_PORT localabstract:sndcpy; then
    echo "Failed to forward audio port $SNDCPY_PORT"
    exit 1
fi

echo Start $SNDCPY_APK...
if ! $ADB "${serial[@]}" shell am start com.rom1v.sndcpy/.MainActivity; then
    echo "Failed to start $SNDCPY_APK"
    exit 1
fi

attempt=0
while ((1))
do
    attempt=$((attempt + 1))
    if ((attempt > 100)); then
        echo "Timed out waiting for $SNDCPY_APK to start"
        exit 1
    fi
    sleep 0.1
    sndcpy_started=$($ADB "${serial[@]}" shell pidof com.rom1v.sndcpy 2>/dev/null)
    if [[ -n "$sndcpy_started" ]]; then
        break
    fi
done

echo Ready playing...
exit 0