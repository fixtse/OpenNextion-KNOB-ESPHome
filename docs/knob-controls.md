# Native knob controls

These controls use ESPHome's Native API. Enable **Allow the device to perform
Home Assistant actions** for the device in Home Assistant. A connected API alone
does not grant permission to run actions.

| Entity | Rotate | Short press | Hold and rotate |
| --- | --- | --- | --- |
| Climate | Adjust selected temperature, range endpoint, or humidity | Cycle advertised HVAC modes, or confirm a previewed option | Choose temperature, whole range, low/high endpoint, preset, fan, swing, or humidity |
| Fan | Adjust percentage using `percentage_step`, or preview a secondary option | Toggle power, or confirm the preview | Choose speed, preset, oscillation, or direction |
| Number / input number | Adjust on the advertised min/max/step grid | No additional action | — |
| Select / input select | Preview an option | Apply preview | — |
| Scene / button / input button | — | Activate | — |
| Media player | Adjust volume | Play/pause a loaded track; run the configured automation/script when idle | Next track clockwise; previous track counterclockwise |

Secondary settings appear only when Home Assistant advertises the capability
and its required attributes. Release the knob after choosing a setting, then
turn it normally to adjust. Hold without turning to return to the device list.
Scenes and buttons show **Action sent**, rather than claiming device execution
has been confirmed. Their initial `unknown` timestamp does not disable activation.

Climate mode lists and other options accept both the Python-style strings
sent through ESPHome and JSON lists. A thermostat's operating activity (such as
`heating` or `idle`) is distinct from the HVAC mode requested by a short press.

Requested values appear immediately. Rotation is combined into updates at most
five times per second per control, with the final detent retained. Temperature
and HVAC mode requests remain independent, so pressing while a temperature
command is pending does not discard that temperature. Feedback acknowledges the
requested value; intermediate feedback updates the confirmed cache without
replacing the requested display. After five seconds without confirmation, the
confirmed value returns and **No confirmation** appears briefly. Check the HA
action permission if every command times out.

Leaving the detail page, losing the HA connection, or making the entity
unavailable cancels queued commands and previews. Reconnecting waits for a fresh
state subscription before enabling controls; it never replays old input.
The offline overlay distinguishes missing initial state from an unavailable
entity and a disconnected Home Assistant connection.

Light color temperature now uses 100 increments over the advertised Kelvin
range. Existing stored Kelvin values and legacy ten-step values remain readable.

## Design reference

The capability-dependent fan and climate controls follow the approach used by
[OpenHAPanel entity handlers](https://github.com/OpenNextion/OpenHAPanel/tree/main/main/entity_handlers).
The shared confirmed-state/request separation adapts the
[entity-store design](https://github.com/OpenNextion/OpenHAPanel/tree/main/components/ha_entity_store)
to this project's ESPHome Native API and rotary input. No OpenHAPanel board
firmware or touchscreen layout is used.

## Validation

In the project's Codespace, run `c++ -std=c++17 tests/test_control_state.cpp -o
/tmp/test_control_state && /tmp/test_control_state`, the existing C++ and Python
regressions, the configuration browser tests, and ESPHome 2026.10.2 configuration
validation and compilation. Hardware and live HA behavior still require a device.

## Playlist and station menu

On the knob's configuration webpage, choose a media player slot and add entries
under **Playlists & stations**. Give each a name and a `script.*` or `automation.*`
entity ID that starts that playlist or station on the intended player. Up to eight
entries per slot are saved on the knob and included in configuration backups.
Automation conditions are respected. These actions do not receive a player
parameter: choose the player in the Home Assistant script or automation itself.

With entries configured, press and hold in the media player view. A visible bar
fills; at **0.5 seconds**, **Release now / Open playlists** appears. Release while
that cue is visible to open the list. Rotate to highlight an entry and short-press
to run it. **Action sent** acknowledges the request, not confirmed playback.
Medium-press again to return to the player. Continue holding to **1.5 seconds** to
return to the device list. Without entries configured, the usual 688ms back
threshold remains. No double press is needed.

Short presses on the player view still play/pause or run its idle action. Holding
and turning there still skips tracks. In the playlist menu, turning while holding
is ignored and suppresses the release action. Disconnecting, leaving the player,
or entering standby cancels the menu; reconnecting never launches an entry.

## Press feedback

Executable entity presses show a shared 1.5-second on-screen notification.
Queued native controls show **Action requested**, changing to **Action sent**
after the API request is transmitted. This covers climate/fan controls,
select confirmations, scenes/buttons, and media play/pause. Script/automation
bindings, playlist entries, light/switch toggles, sensor press events, and cover
commands also show **Action sent** at dispatch. Local GPIO toggles show **Done**.
This acknowledges dispatch, not successful Home Assistant execution or playback.
Existing pending-state and timeout messages remain available in the detail view.
Navigation, unavailable actions, and press bindings set to **No action** do not
show execution feedback. Leaving the entity or entering standby clears it.
