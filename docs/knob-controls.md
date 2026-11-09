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
| Media player | Adjust volume, or preview source/mute | Next track and double-press previous-track on primary view; confirm a preview | Choose volume, source, or mute |

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
