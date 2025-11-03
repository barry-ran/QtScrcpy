#!/bin/bash

echo "Package AppImage"

build_mode="$1"
if [[ $build_mode != "Release" && $build_mode != "Debug" && $build_mode != "MinSizeRel" && $build_mode != "RelWithDebInfo" ]]; then
    echo "error: unknown build mode, exiting......"
    exit 1
fi

# Qt path detection
detected_qt_path=""
if command -v qmake &> /dev/null; then
    qmake_path=$(which qmake)
    if [ -n "$qmake_path" ]; then
        qt_base=$(dirname "$(dirname "$(dirname "$qmake_path")")")
        if [ -d "$qt_base/gcc_64" ]; then
            detected_qt_path="$qt_base"
        fi
    fi
fi

if [ -n "$detected_qt_path" ]; then
    ENV_QT_PATH="$detected_qt_path"
elif [ -n "$ENV_QT_PATH" ]; then
    if [ ! -d "$ENV_QT_PATH/gcc_64" ]; then
        detected_qt_path=""
    fi
fi

if [ -z "$ENV_QT_PATH" ] || [ ! -d "$ENV_QT_PATH/gcc_64" ]; then
    common_qt_paths=(
        "$HOME/Qt"
        "/opt/Qt"
        "/usr/local/Qt"
        "/usr/lib/qt5"
    )
    for base_path in "${common_qt_paths[@]}"; do
        if [ -d "$base_path" ]; then
            latest_version=$(ls -1td "$base_path"/*/gcc_64 2>/dev/null | head -n 1 | sed 's|/gcc_64$||')
            if [ -n "$latest_version" ] && [ -d "$latest_version/gcc_64" ]; then
                ENV_QT_PATH="$latest_version"
                break
            fi
        fi
    done
fi

if [ ! -d "$ENV_QT_PATH/gcc_64" ]; then
    echo "error: Qt installation not found at $ENV_QT_PATH/gcc_64"
    exit 1
fi

echo "Using Qt: $ENV_QT_PATH"

script_dir=$(cd $(dirname "$0") && pwd)
project_root=$(cd "$script_dir/../.." && pwd)
old_cd=$(pwd)
cd "$project_root"

output_path="./output/x64/$build_mode"
appimage_output_path="./output/appimage"
appdir_path="$appimage_output_path/QtScrcpy.AppDir"
app_name="QtScrcpy"
app_version=$(cat QtScrcpy/appversion 2>/dev/null || echo "0.0.0")

echo "Build mode: $build_mode"
echo "App version: $app_version"

if [ ! -f "$output_path/$app_name" ]; then
    echo "error: $app_name executable not found in $output_path"
    exit 1
fi

# Clean previous build
if [ -d "$appimage_output_path" ]; then
    rm -rf "$appimage_output_path"
fi

mkdir -p "$appimage_output_path"
mkdir -p "$appdir_path/usr/bin"
mkdir -p "$appdir_path/usr/lib"
mkdir -p "$appdir_path/usr/share/applications"
mkdir -p "$appdir_path/usr/share/icons/hicolor/"{16x16,24x24,32x32,48x48,64x64,128x128,256x256}"/apps"
mkdir -p "$appdir_path/usr/share/metainfo"

# Copy executable and resources
cp "$output_path/$app_name" "$appdir_path/usr/bin/$app_name"
chmod +x "$appdir_path/usr/bin/$app_name"

if [ -f "$output_path/sndcpy.sh" ]; then
    cp "$output_path/sndcpy.sh" "$appdir_path/usr/bin/"
    chmod +x "$appdir_path/usr/bin/sndcpy.sh"
fi
if [ -f "$output_path/sndcpy.apk" ]; then
    cp "$output_path/sndcpy.apk" "$appdir_path/usr/bin/"
fi

if [ -d "$project_root/keymap" ]; then
    cp -r "$project_root/keymap" "$appdir_path/usr/share/"
fi
if [ -d "$project_root/config" ]; then
    cp -r "$project_root/config" "$appdir_path/usr/share/"
fi

# Copy ADB and scrcpy-server
adb_source="$project_root/QtScrcpy/QtScrcpyCore/src/third_party/adb/linux/adb"
server_source="$project_root/QtScrcpy/QtScrcpyCore/src/third_party/scrcpy-server"
mkdir -p "$appdir_path/usr/lib/qtscrcpy"

if [ -f "$adb_source" ]; then
    cp "$adb_source" "$appdir_path/usr/lib/qtscrcpy/adb"
    chmod +x "$appdir_path/usr/lib/qtscrcpy/adb"
    # Create symlink for sndcpy.sh compatibility
    if [ ! -f "$appdir_path/usr/bin/adb" ]; then
        ln -sf "../lib/qtscrcpy/adb" "$appdir_path/usr/bin/adb"
    fi
fi

if [ -f "$server_source" ]; then
    cp "$server_source" "$appdir_path/usr/lib/qtscrcpy/scrcpy-server"
    chmod +x "$appdir_path/usr/lib/qtscrcpy/scrcpy-server"
fi

# Process icon
icon_file=""
icon_source=""
target_icon_path="$appdir_path/usr/share/icons/hicolor/256x256/apps/$app_name.png"

if [ -f "$project_root/QtScrcpy/res/QtScrcpy.png" ]; then
    icon_source="$project_root/QtScrcpy/res/QtScrcpy.png"
elif [ -f "$project_root/QtScrcpy/res/image/tray/logo.png" ]; then
    icon_source="$project_root/QtScrcpy/res/image/tray/logo.png"
elif [ -f "$project_root/QtScrcpy/res/QtScrcpy.ico" ]; then
    icon_source="$project_root/QtScrcpy/res/QtScrcpy.ico"
fi

if [ -n "$icon_source" ] && [ -f "$icon_source" ]; then
    need_resize=false
    if command -v identify &> /dev/null; then
        icon_size=$(identify -format "%wx%h" "$icon_source" 2>/dev/null)
        [ "$icon_size" = "256x256" ] || need_resize=true
    elif command -v magick &> /dev/null; then
        icon_size=$(magick identify -format "%wx%h" "$icon_source" 2>/dev/null)
        [ "$icon_size" = "256x256" ] || need_resize=true
    elif [[ "$icon_source" == *.png ]]; then
        cp "$icon_source" "$target_icon_path"
        icon_file="$target_icon_path"
    else
        need_resize=true
    fi

    if [ "$need_resize" = true ]; then
        if command -v convert &> /dev/null; then
            convert "$icon_source" -resize 256x256 "$target_icon_path" && icon_file="$target_icon_path"
        elif command -v magick &> /dev/null; then
            magick "$icon_source" -resize 256x256 "$target_icon_path" && icon_file="$target_icon_path"
        elif command -v ffmpeg &> /dev/null; then
            ffmpeg -i "$icon_source" -vf scale=256:256 -y "$target_icon_path" 2>/dev/null && icon_file="$target_icon_path"
        else
            cp "$icon_source" "$target_icon_path"
            icon_file="$target_icon_path"
        fi
    fi
fi

if [ -z "$icon_file" ]; then
    if command -v convert &> /dev/null; then
        convert -size 256x256 xc:transparent "$target_icon_path" 2>/dev/null
    elif command -v magick &> /dev/null; then
        magick -size 256x256 xc:transparent "$target_icon_path" 2>/dev/null
    else
        touch "$target_icon_path"
    fi
    icon_file="$target_icon_path"
fi

if [ -n "$icon_file" ] && [ -f "$icon_file" ]; then
    icon_sizes=(16 24 32 48 64 128 256)
    for size in "${icon_sizes[@]}"; do
        icon_size_path="$appdir_path/usr/share/icons/hicolor/${size}x${size}/apps/$app_name.png"
        if command -v convert &> /dev/null; then
            convert "$icon_file" -resize ${size}x${size} "$icon_size_path" 2>/dev/null || cp "$icon_file" "$icon_size_path"
        elif command -v magick &> /dev/null; then
            magick "$icon_file" -resize ${size}x${size} "$icon_size_path" 2>/dev/null || cp "$icon_file" "$icon_size_path"
        else
            cp "$icon_file" "$icon_size_path" 2>/dev/null || true
        fi
    done
fi

# Create desktop file
cat > "$appdir_path/usr/share/applications/$app_name.desktop" << EOF
[Desktop Entry]
Type=Application
Name=QtScrcpy
Comment=Display and control Android devices via USB or over network
Exec=$app_name
Icon=$app_name
Categories=Utility;
Terminal=false
StartupNotify=true
EOF

# Create metainfo file
app_id="com.github.barry-ran.QtScrcpy"
cat > "$appdir_path/usr/share/metainfo/$app_name.appdata.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>$app_id</id>
  <name>QtScrcpy</name>
  <summary>Display and control Android devices via USB or over network</summary>
  <description>
    <p>QtScrcpy supports displaying and controlling Android devices via USB or over network. It does NOT require root privileges.</p>
  </description>
  <provides>
    <binary>$app_name</binary>
  </provides>
  <launchable type="desktop-id">$app_name.desktop</launchable>
  <url type="homepage">https://github.com/barry-ran/QtScrcpy</url>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>Apache-2.0</project_license>
</component>
EOF

# Create AppRun script
IS_DOCKER_OR_CI=false
if [ -f "/.dockerenv" ] || [ -n "${GITHUB_ACTIONS:-}" ] || [ -n "${CI:-}" ]; then
    IS_DOCKER_OR_CI=true
fi

if [ "$IS_DOCKER_OR_CI" = true ]; then
    cat > "$appdir_path/AppRun" << 'APPRUN_EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
APPIMAGE_LIB_DIRS="$HERE/usr/lib:$HERE/usr/lib/x86_64-linux-gnu"
export LD_LIBRARY_PATH="$APPIMAGE_LIB_DIRS:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:/usr/lib"
if [ -f "$HERE/usr/lib/libQt5XcbQpa.so.5" ]; then
    export LD_PRELOAD="$HERE/usr/lib/libQt5XcbQpa.so.5"
fi
QT_PLUGINS_DIR="$HERE/usr/plugins"
if [ -d "$QT_PLUGINS_DIR" ]; then
    export QT_PLUGIN_PATH="$QT_PLUGINS_DIR"
    export QT_QPA_PLATFORM_PLUGIN_PATH="$QT_PLUGINS_DIR/platforms"
fi
export QTSCRCPY_ADB_PATH="$HERE/usr/lib/qtscrcpy/adb"
export QTSCRCPY_SERVER_PATH="$HERE/usr/lib/qtscrcpy/scrcpy-server"
export QTSCRCPY_KEYMAP_PATH="$HERE/usr/share/keymap"
export QTSCRCPY_CONFIG_PATH="$HERE/usr/share/config"
exec "$HERE/usr/bin/QtScrcpy" "$@"
APPRUN_EOF
else
    cat > "$appdir_path/AppRun" << 'APPRUN_EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
export QTSCRCPY_ADB_PATH="$HERE/usr/lib/qtscrcpy/adb"
export QTSCRCPY_SERVER_PATH="$HERE/usr/lib/qtscrcpy/scrcpy-server"
export QTSCRCPY_KEYMAP_PATH="$HERE/usr/share/keymap"
export QTSCRCPY_CONFIG_PATH="$HERE/usr/share/config"
exec "$HERE/usr/bin/QtScrcpy" "$@"
APPRUN_EOF
fi
chmod +x "$appdir_path/AppRun"

# Download linuxdeploy tools
linuxdeploy_url="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
linuxdeploy_qt_url="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
linuxdeploy_temp_dir="$appimage_output_path/.tools"
linuxdeploy_path="$linuxdeploy_temp_dir/linuxdeploy.AppImage"
linuxdeploy_qt_path="$linuxdeploy_temp_dir/linuxdeploy-plugin-qt.AppImage"

mkdir -p "$linuxdeploy_temp_dir"

if [ ! -f "$linuxdeploy_path" ]; then
    wget "$linuxdeploy_url" -O "$linuxdeploy_path" || exit 1
    chmod +x "$linuxdeploy_path"
fi

if [ ! -f "$linuxdeploy_qt_path" ]; then
    wget "$linuxdeploy_qt_url" -O "$linuxdeploy_qt_path" || exit 1
    chmod +x "$linuxdeploy_qt_path"
fi

linuxdeploy_path_abs=$(cd "$(dirname "$linuxdeploy_path")" && pwd)/$(basename "$linuxdeploy_path")
linuxdeploy_qt_path_abs=$(cd "$(dirname "$linuxdeploy_qt_path")" && pwd)/$(basename "$linuxdeploy_qt_path")

if [ ! -f "$linuxdeploy_path_abs" ] || [ ! -f "$linuxdeploy_qt_path_abs" ]; then
    echo "error: linuxdeploy tools not found"
    exit 1
fi

linuxdeploy_path="$linuxdeploy_path_abs"
linuxdeploy_qt_path="$linuxdeploy_qt_path_abs"

export QMAKE="$ENV_QT_PATH/gcc_64/bin/qmake"
export QML_SOURCES_PATHS="$project_root/QtScrcpy"
export DEPLOY_CMD="$linuxdeploy_path"
export PATH="$ENV_QT_PATH/gcc_64/bin:$PATH"

# Pre-copy Qt plugins and libraries for CI environment
if [ "$IS_DOCKER_OR_CI" = true ]; then
    qt_plugins_source="$ENV_QT_PATH/gcc_64/plugins"
    qt_plugins_target="$appdir_path/usr/plugins"
    qt_libs_source="$ENV_QT_PATH/gcc_64/lib"
    qt_libs_target="$appdir_path/usr/lib"

    if [ -d "$qt_plugins_source/platforms" ]; then
        mkdir -p "$qt_plugins_target/platforms"
        cp -r "$qt_plugins_source/platforms"/* "$qt_plugins_target/platforms/" 2>/dev/null || true
    fi

    if [ -d "$qt_libs_source" ]; then
        mkdir -p "$qt_libs_target"
        for lib in "libQt5XcbQpa.so.5" "libQt5XcbQpa.so"; do
            if [ -f "$qt_libs_source/$lib" ]; then
                cp "$qt_libs_source/$lib" "$qt_libs_target/" 2>/dev/null || true
            fi
        done
    fi
fi

# Run linuxdeploy
cd "$project_root"
export LINUXDEPLOY_PLUGIN_QT_PATH="$linuxdeploy_qt_path"

linuxdeploy_args=(
    --appdir "$appdir_path"
    --plugin qt
    --output appimage
    --executable "$appdir_path/usr/bin/$app_name"
    --desktop-file "$appdir_path/usr/share/applications/$app_name.desktop"
)

if [ -n "$icon_file" ] && [ -f "$icon_file" ]; then
    linuxdeploy_args+=(--icon-file "$icon_file")
fi

"$linuxdeploy_path" "${linuxdeploy_args[@]}" || {
    if [ -f "$appdir_path/usr/share/metainfo/$app_name.appdata.xml" ]; then
        mv "$appdir_path/usr/share/metainfo/$app_name.appdata.xml" "$appdir_path/usr/share/metainfo/$app_name.appdata.xml.bak"
        if "$linuxdeploy_path" "${linuxdeploy_args[@]}"; then
            rm -f "$appdir_path/usr/share/metainfo/$app_name.appdata.xml.bak"
        else
            mv "$appdir_path/usr/share/metainfo/$app_name.appdata.xml.bak" "$appdir_path/usr/share/metainfo/$app_name.appdata.xml"
            linuxdeploy_args_no_plugin=(
                --appdir "$appdir_path"
                --output appimage
                --executable "$appdir_path/usr/bin/$app_name"
                --desktop-file "$appdir_path/usr/share/applications/$app_name.desktop"
            )
            [ -n "$icon_file" ] && [ -f "$icon_file" ] && linuxdeploy_args_no_plugin+=(--icon-file "$icon_file")
            "$linuxdeploy_path" "${linuxdeploy_args_no_plugin[@]}" || exit 1
        fi
    else
        linuxdeploy_args_no_plugin=(
            --appdir "$appdir_path"
            --output appimage
            --executable "$appdir_path/usr/bin/$app_name"
            --desktop-file "$appdir_path/usr/share/applications/$app_name.desktop"
        )
        [ -n "$icon_file" ] && [ -f "$icon_file" ] && linuxdeploy_args_no_plugin+=(--icon-file "$icon_file")
        "$linuxdeploy_path" "${linuxdeploy_args_no_plugin[@]}" || exit 1
    fi
}

# Verify Qt plugins for CI environment
if [ "$IS_DOCKER_OR_CI" = true ]; then
    qt_plugins_dir="$appdir_path/usr/plugins"
    qt_platforms_dir="$qt_plugins_dir/platforms"
    if [ ! -d "$qt_platforms_dir" ] || [ -z "$(find "$qt_platforms_dir" -name "*.so" -type f 2>/dev/null)" ]; then
        if [ -n "$ENV_QT_PATH" ] && [ -d "$ENV_QT_PATH/gcc_64/plugins/platforms" ]; then
            mkdir -p "$qt_platforms_dir"
            cp -r "$ENV_QT_PATH/gcc_64/plugins/platforms"/* "$qt_platforms_dir/" 2>/dev/null || true
        fi
    fi
fi

# Find generated AppImage
appimage_file=""
appimage_file=$(find "$project_root" -maxdepth 1 -name "*.AppImage" -type f 2>/dev/null | grep -v "linuxdeploy" | grep -v "plugin" | head -n 1)

if [ -z "$appimage_file" ] || [ ! -f "$appimage_file" ]; then
    appimage_file=$(find "$(dirname "$appdir_path")" -maxdepth 1 -name "*.AppImage" -type f 2>/dev/null | grep -v "linuxdeploy" | grep -v "plugin" | head -n 1)
fi

if [ -n "$appimage_file" ] && [ -f "$appimage_file" ]; then
    final_appimage="$appimage_output_path/${app_name}-x86_64.AppImage"
    mv "$appimage_file" "$final_appimage"
    echo "AppImage created: $final_appimage ($(du -h "$final_appimage" | cut -f1))"
else
    echo "error: AppImage file not found"
    exit 1
fi

cd "$old_cd"
echo "AppImage packaging completed successfully!"
exit 0
