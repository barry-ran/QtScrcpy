#!/usr/bin/env bash
SNDCPY_PORT="28200"
SNDCPY_APK="sndcpy.apk"
ADB="./adb"

serial=
if [[ $# -ge 2 ]]
then
    serial="-s $1"
    SNDCPY_PORT="$2"
fi

echo "Waiting for device '$1'..."
"$ADB" "$serial" wait-for-device
echo "Found device '$1'"

sndcpy_installed=$($ADB "$serial" shell pm path com.rom1v.sndcpy)
if [[ $sndcpy_installed == "" ]]; then
    echo "Installing '$SNDCPY_APK'..." 
    "$ADB" "$serial" uninstall com.rom1v.sndcpy || echo "Uninstall failed!"
    "$ADB" "$serial" install -t -r -g "$SNDCPY_APK"
    echo "Successfully installed '$SNDCPY_APK'"
fi

echo "Requesting PROJECT_MEDIA permission..."
"$ADB" "$serial" shell appops set com.rom1v.sndcpy PROJECT_MEDIA allow

echo "Forwarding port $SNDCPY_PORT..."
"$ADB" "$serial" forward "tcp:$SNDCPY_PORT" localabstract:sndcpy

echo "Starting '$SNDCPY_APK'..."
"$ADB" "$serial" shell am start com.rom1v.sndcpy/.MainActivity

while ((1))
do
    echo "Waiting for '$SNDCPY_APK' to start..."
    sleep 0.1
    sndcpy_started=$($ADB shell 'ps | grep com.rom1v.sndcpy')
    if [[ $sndcpy_started != "" ]]; then
        break
    fi
done

echo "Ready playing..."