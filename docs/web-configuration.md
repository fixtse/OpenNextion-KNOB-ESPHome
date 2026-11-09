# Knob web configuration

Open the knob's IP address or its ESPHome **Visit** link. The embedded interface
has separate pages for display slots, Home Assistant, device settings, and backups.
All browser assets are served by the knob; no frontend build or external CDN is required.

## Display slots

Choose entities by name, ID, domain, or room. Each slot can have a custom display
name and an automation/script press action. Climate and media players retain their
native press controls and reject custom overrides. Light and cover type choices
use the browser's Home Assistant capability data when available. Manual entity IDs
remain available when the catalogue cannot be loaded.

Move slots earlier or later, or duplicate one into an empty slot. A dot marks
unsaved slots. **Save slot** applies the selected slot; **Save all** saves dirty
slots sequentially. If a save fails, successful slots stay saved and the remaining
changes stay dirty. Discard restores the last successfully saved values.
The round preview describes configuration and controls, not live device state.

Drafts are kept in this browser tab's session storage, with a recovery prompt after
reload or Home Assistant sign-in. Closing with pending changes prompts the user.
A browser that blocks storage cannot guarantee recovery. The theme preference
(system, light, or dark) is stored locally.

## Connections and settings

The status strip distinguishes reachability of the knob, the knob's Home Assistant
Native API connection, and browser sign-in/catalogue loading. A connected Native API
does not prove Home Assistant has granted action permissions; the connection page
explains where to enable them. Status refreshes every 30 seconds while visible.

Browser sign-in loads the entity catalogue directly from Home Assistant and does
not set the knob's HA credentials or register its ESPHome integration.
Only selected entity metadata is persisted on the knob.

Device settings include brightness (10–100%), standby timeout (1–900 seconds),
knob sensitivity, standby weather, and scheduled sleep (1–3600 seconds idle).
Sleep times use Home Assistant's clock; matching start/end times mean all day.
Saving brightness wakes the backlight. Firmware updates continue through Home
Assistant's **Firmware Update** entity.

## Backups

Export downloads the last successfully saved slots and settings, excluding
credentials. JSON backups have schema version 2 (version 1 imports migrate with defaults) and model ONX2424G013. Imports
are limited to 64 KB and validated before replacing the local draft. Importing
never writes to the knob: review, then save slots and device settings separately.

Diagnostics downloads contain firmware, uptime, reset reason, and connection
flags. They exclude entity names, HA addresses, tokens, and Wi-Fi credentials.

## Display feedback and icons

Command confirmations and errors clear after 2.5 seconds, restoring normal entity
metadata such as the media artist. Pending commands remain visible until Home
Assistant confirms them or they time out.

Opening or refreshing the signed-in browser catalogue also synchronizes the icons
for saved slots, including icons changed in Home Assistant. Unsaved slot edits are
preserved. The device downloads MDI icons after its clock has synchronized, retries
failed downloads with backoff, and tries a second HTTPS source if needed. Built-in
entity icons remain visible while downloading or when an icon is unavailable. The
browser connection retry also clears failed icon download delays.

Icon selection also reads Home Assistant's integration icon resources and
device-class definitions, which can supply the dashboard icon even when no
explicit `icon` attribute exists. Native sensor device-class updates provide a
useful default without browser sign-in. Explicit Home Assistant overrides take
priority. **Download diagnostics** includes per-slot configured, runtime and
selected MDI names, download status and the last download error; entity names and
IDs remain excluded. Registry and integration icons sync when the signed-in
browser catalogue loads or refreshes.

SVG parsing, rasterizer scratch space, and cached pixels all use PSRAM. This avoids
large parser allocations failing in the device's constrained internal heap even
when the icon has downloaded successfully.

## Live display previews

The slot preview reads live values from the knob and uses the same presentation
model as the display. Sensor readings are prominent, names have two lines, and
icons are compact. Climate and fan controls include mode symbols; media previews
show the current artwork, artist, volume and playback ring. New entities show a
clear save-to-load state instead of invented readings. Preview refreshes pause
while the page is hidden or a configuration save is in progress.

The configuration page uses an orange/slate palette in light and dark themes,
with Nunito typography when the browser can load it and a local system fallback.

Sensor values are also visible in the main device carousel, without opening the
entity. The preview's screen selector switches between the carousel and opened
entity layouts. Media volume always displays a whole-number percentage.

Choose the media progress ring color in **Device settings**, using a preset or
the custom color picker. The default remains blue; paused playback uses the same
color at reduced opacity. The color is stored on the knob and included in backups.

The weather view's inactivity timer defaults to **From the main screen only**,
so an opened entity or settings page stays visible. Choose **From any screen**
for the previous behavior, or **Never automatically** to disable that transition.
The scheduled sleep timer is configured separately.

Every configured entity now shows a state or value in the main carousel. Lights
show power and brightness, covers show position and movement, climate and fans
show their target/speed and mode, and media players show playback state and track.
Unavailable entities show `--` rather than a stale reading.
