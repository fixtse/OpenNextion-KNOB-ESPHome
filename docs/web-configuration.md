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
credentials. JSON backups have schema version 1 and model ONX2424G013. Imports
are limited to 64 KB and validated before replacing the local draft. Importing
never writes to the knob: review, then save slots and device settings separately.

Diagnostics downloads contain firmware, uptime, reset reason, and connection
flags. They exclude entity names, HA addresses, tokens, and Wi-Fi credentials.
