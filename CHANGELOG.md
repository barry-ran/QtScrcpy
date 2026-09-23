# QtScrcpy Plus — Changelog

## v4.1.1 — Visual Keymap Studio (TC Gaming Style)

**Release Date:** 2026-09-23

### ✨ New Features

#### 🎮 Visual Keymap Studio
- Added a full **TC Gaming-style Visual Keymap Studio** (KeymapEditor) accessible via the keymap button in the toolbar
- Interactive canvas overlaid on the phone screen — drag and drop key nodes visually
- **Supported node types:**
  - **WASD Joystick** — analog movement mapping to W/A/S/D keys
  - **Aim / Look** — mouse movement area for camera control
  - **Free Look** — right-stick style free camera look
  - **Click Button** — single key/click binding on any screen position
- **Key recording**: Click any node label to record a new key binding from the keyboard directly
- **Zoom-independent** positioning: nodes stay accurate at any display scale
- **Live Apply**: Push the keymap directly to the connected Android device without restarting
- **Save / Load**: Keymap configurations stored as JSON files for easy sharing

### 🛠️ Improvements

- Replaced the old broken KeymapDialog with the new KeymapEditor
- Updated ToolForm so the keymap toolbar button opens the new studio
- Cleaned up CMakeLists.txt: removed old keymapdialog files, registered new keymapeditor sources

### 🔧 CI / Build Fixes

- Fixed GitHub Actions CI pipeline for Windows builds:
  - Updated .gitmodules to use **HTTPS** instead of SSH for QtScrcpyCore submodule (required in CI environment)
  - Updated actions/checkout from v3 to **v4** with submodules: recursive to ensure proper submodule fetching
  - Resolved 'fatal: No names found, cannot describe anything' error from generate-version.py

---

## v4.1.0 — Internal Keymap Manager + BitRate Improvement

- Added internal Keymap Manager with improved UI
- Improved BitRate selection controls

---

## Previous Versions

Based on upstream QtScrcpy by barry-ran.
