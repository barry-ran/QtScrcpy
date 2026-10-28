# QtScrcpy Local — Warm Linen

Approved direction: the user selected the warm linen concept (C) and requested a separate projection window. Main workspace is an operational device/settings utility, not a marketing surface.

- Main window: left device identity and selector, right tabs for projection, audio, key mapping, connection and advanced configuration. Persistent primary projection actions and log access.
- Palette: ivory #fcfcfa, linen #efeee9, graphite #303832, muted green #6b886f. SF/PingFang system typography with restrained 12–19 px scale.
- Flat grouped settings with fine separators; no promotional hero, card grid or oversized headings.
- Device portrait is explicitly illustrative, never a fabricated live preview. Connection and device selection use actual ADB state.
- Projection remains a separate window. Both the floating title controls and toolbar hide after 3 seconds idle. Top-edge hover reveals title controls; portrait reveals the toolbar from either edge and places it on a side with available space, landscape uses a horizontal top toolbar. Controls remain visible while hovered, dragging or opening a menu. Ctrl+Alt+T restores controls and releases cursor capture. Cursor capture suppresses edge reveal.
- Main-window close hides to background; explicit menu Quit/Cmd+Q shuts down sessions.
- Source feature semantics remain unchanged: sndcpy forwards audio; it does not guarantee phone-speaker muting.

- App identity: warm linen rounded square with muted sage monitor and graphite phone. Icon source is scripts/render-linen-icon.swift; ICNS, tray and workspace logo share this source.
- Advanced subpages own their scroll containers; the advanced parent does not create a second scroll area.

- Projection title controls sit outside and above captured pixels. Double-click saves/restores explicit window bounds. Normal touch mode uses a stationary 400 ms hold for window dragging; immediate movement remains a phone swipe. Game mode bypasses hold-to-drag.

- Tool strip has no deliberate side gap. During projection moves it hides, then restores only if previously visible once mouse release and 180 ms settling have completed.

- Window zoom fits the source aspect into the available screen rather than stretching to desktop bounds. Zoom adds no content margin; title double-click and blank-area double-click share one state. Dragging a zoomed projection restores its normal size. Both fullscreen and ordinary resize preserve source ratio; explicit flexible virtual displays retain adaptive sizing.

- Double-click now enters/exits actual fullscreen, not a fitted floating-window zoom. Preserve image aspect and restore normal bounds. Native macOS fullscreen transitions queue a second toggle until the animation settles.

- Fullscreen always hides custom title chrome, including edge reveal and recovery shortcuts, and reserves zero layout margin. Exiting restores the external title chrome and its idle-hide policy. Quick phone tools remain independently revealable.

- Primary projection tools, in order: audio toggle, home, back, recent apps, settings panel, notifications, More. Audio uses a headphone outline and active dot; state follows the actual selected serial's AudioOutput starting/running state. Main footer no longer duplicates the audio shortcut.
- Phone viewport hover/interaction keeps tools visible. Outside application clicks or application deactivation hide tools immediately. Title chrome keeps a separate three-second idle timer.
- More menu anchors to the tool strip, never to the opposite edge of the phone; constrained display bounds may flip the menu inward.

- ToolForm has one geometry owner (VideoForm), with no inherited magnetic snapping or independent drag logic. Rebuild its layout only on orientation changes. macOS tool-window lifetime is managed explicitly so native auto-hiding does not fight hover reveal.
- The phone tool strip has no idle timeout; it persists until outside click/deactivation. Title chrome alone uses idle hiding.

## 2026-10-23 projection controls policy (supersedes earlier idle/drag notes)

- Title, side tools and resize handles reveal together on phone interaction, hide together on outside click/application deactivation or after 5 seconds without activity. A stationary pointer does not keep them alive.
- Dragging the title moves the projection with both toolbars continuously visible and positioned by VideoForm; no movement-settle hide/show cycle.
- Title height is 34 logical pixels and its bottom directly adjoins captured pixels, without the former six-pixel gap.
- Top-left resize fixes the opposite corner and preserves source aspect; bottom-right handle remains. Fullscreen retains zero title margin and hides resize handles.

## 2026-10-23 projection window ownership

- Title and strip are ordinary non-activating transient windows owned by the projection, not floating NSPanel tool windows. They do not remain above unrelated applications.
- Only a user click from another window raises the projection group. Background hover cannot resurrect controls or steal focus.
- Hide both controls synchronously before custom minimize; native state changes and hide events also hide them. Suppress delayed visibility callbacks throughout minimization and restore controls on return.

## 2026-10-23 application/session lifetime

- Main red close hides only the workspace. Projection windows, audio, recording and device discovery continue.
- Projection minimize preserves session/audio/recording and hides chrome synchronously. Closing a projection ends only its device session and audio; device teardown finalizes recording.
- Device disconnection cleans the corresponding window/audio without reopening the workspace.
- Menu “完全退出”, Cmd+Q, native Quit and application shutdown share one idempotent shutdown pipeline: stop timers and own startup jobs, stop audio, drain devices/recorders, save settings, hide tray, exit. Do not kill the shared adb server.

## 2026-10-28 single host projection (supersedes external window ownership)

- One native VideoForm hosts the phone image, title and side tools. Chrome is ordinary child content; dragging, stacking and minimization move the whole window together. Native popups belong directly to the host.
- Transparent masked margins reserve external chrome space without covering phone pixels. Hiding controls changes the hit region, not phone position or size. Title directly adjoins the phone.
- Phone interaction reveals both controls; outside clicks/deactivation or five seconds idle hide them. Dragging keeps both visible. Source aspect is preserved during resize and rotation.
- Fullscreen uses no chrome margins or custom title. Restoring fullscreen restores normal bounds. Legacy saved image-only bounds migrate once to host bounds.
- Main close hides workspace; projection close ends that session; explicit Quit accepts window closure after session cleanup.

## 2026-10-28 fullscreen playback

- Qt owns render-widget native geometry. Metal updates only its layer bounds, preserving the centered aspect-fit view across rotation and fullscreen.
- Fullscreen controls use one dark translucent strip, located in black letterboxing whenever it fits. Include a direct Exit Fullscreen action and Esc; do not reserve image space.
- Phone clicks and typing do not reveal fullscreen chrome. Moving to a screen edge reveals it; hovering controls or menus holds it open, otherwise hide after two seconds. Ordinary-window interaction remains five seconds.
