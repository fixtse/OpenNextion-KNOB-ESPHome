# OpenNextion ESPHome Knob Control

[简体中文](README.zh-CN.md)

ESPHome firmware project for the ONX2424G013 round knob display. It uses an ESP32-S3, ESP-IDF, LVGL, and bundled custom components to provide a local Home Assistant control panel.

## Demo

<p align="center">
  <img src="docs/control_ha_light.gif" alt="Control home-assistant light">
</p>

## Features

- Controls Home Assistant switches, lights, plugs, covers, and automations from the display.
- Supports rotary and push-button input, screen brightness, standby settings, and standby weather.
- Provides Wi-Fi captive-portal provisioning when no network has been configured.
- Supports HTTPS manifest-based firmware updates; the current release OTA image is in `firmware/`.

The main configuration is [ONX2424G013.yaml](ONX2424G013.yaml). The target hardware uses an ESP32-S3 with 16 MB flash, Octal PSRAM, and a GC9A01A round display; use this firmware only with compatible ONX2424G013 hardware.

## Requirements

Build this project with **ESPHome 2026.10.2**. Docker is recommended to keep ESPHome, PlatformIO, and ESP-IDF versions consistent.

- Docker, or a local Python environment with `esphome==2026.10.2`
- Network access for the first build to download dependencies
- A data-capable USB cable for serial flashing

## Clone and build

Clone the complete repository first. The configuration depends on the included `packages`, `my_components`, `assets`, and `fonts` directories.

```bash
git clone https://github.com/fixtse/OpenNextion-KNOB-ESPHome.git
cd OpenNextion-KNOB-ESPHome
```

### Docker (recommended)

Linux/macOS:

```bash
docker run --rm \
  -v "$PWD":/config \
  -w /config \
  ghcr.io/esphome/esphome:2026.10.2 \
  compile ONX2424G013.yaml
```

Windows PowerShell:

```powershell
docker run --rm -v "${PWD}:/config" -w /config ghcr.io/esphome/esphome:2026.10.2 compile ONX2424G013.yaml
```

### Local ESPHome

```bash
python -m pip install "esphome==2026.10.2"
esphome version
esphome compile ONX2424G013.yaml
```

After a successful build, the factory image for browser-based first-time installation is `.esphome/build/onx2424g013/.pioenvs/onx2424g013/firmware.factory.bin`. For later serial uploads, use the firmware file reported by the ESPHome build output.

## Flash over USB

Docker is the recommended way to compile and flash this project. You do not need to install ESPHome locally when flashing with Docker or ESPHome Web.

### Docker (recommended, Linux)

Pass the actual serial device through to the container. The following command compiles the project if needed and uploads it:

```bash
docker run --rm \
  --device=/dev/ttyACM0 \
  -v "$PWD":/config \
  -w /config \
  ghcr.io/esphome/esphome:2026.10.2 \
  run --device /dev/ttyACM0 ONX2424G013.yaml
```

Replace `/dev/ttyACM0` with the serial port of your device. If the device does not enter download mode automatically, hold `BOOT` (GPIO0), tap `RESET` if available, then release `BOOT` when upload starts. Restart the device after uploading completes.

### Local ESPHome CLI

If you installed `esphome==2026.10.2` locally as described above, connect the device and run the following command. It compiles the firmware when needed and uploads it. When prompted, select the corresponding serial port, for example `/dev/ttyACM0` or `/dev/ttyUSB0` on Linux, or `COM3` on Windows:

```bash
esphome run ONX2424G013.yaml
```

Specify the serial port if necessary:

```bash
esphome run --device /dev/ttyACM0 ONX2424G013.yaml
```

## Browser installation (first-time flash)

For browser flashing, open [ESPHome Web](https://web.esphome.io/) in Chrome or Edge, connect the device, and select a factory firmware file. This method requires a Chromium-based browser with Web Serial support, but does not require a local ESPHome installation.

- **Build from source:** select `.esphome/build/onx2424g013/.pioenvs/onx2424g013/firmware.factory.bin` after compiling the repository.
- **No local build:** download `onx2424g013.factory_v1.0.1.bin` from the [v1.0.1 release](https://github.com/OpenNextion/OpenNextion-Example-ESPHome_KNOB_Control/releases/tag/v1.0.1), then select that file in ESPHome Web.

## User manual

For device setup, Wi-Fi provisioning, Home Assistant integration, screen operation, and OTA updates, see the [online user manual](https://opennextion.github.io/OpenNextion-Example-ESPHome_KNOB_Control/).

## Project layout

- `ONX2424G013.yaml`: main ESPHome configuration.
- `packages/`: split device, connectivity, display, input, and feature configuration.
- `my_components/`: local ESPHome components.
- `assets/` and `fonts/`: UI assets and fonts.
- `docs/`: README demonstration media.
- `firmware/`: release OTA image and manifest.
- `User Manual/`: bundled web user manual.

## Troubleshooting

- **Serial port is not detected:** Use a data-capable USB cable, confirm that the operating system sees the serial device, and close any application using that port.
- **Assets or components cannot be found during compilation:** Run commands from the root of the cloned repository and retain all project directories.

## Sensors, scripts, and climate

The device entity picker also accepts `sensor`, `binary_sensor`, `script`, and
`climate` entities. Select them in the existing device slots; their device type
is inferred automatically. These additions require building this source;
previously published firmware binaries do not contain them.

- **Sensor / binary sensor:** open the detail page to see the live state and unit.
  A short press while viewing it emits `esphome.onx_sensor_press` with `entity_id`
  and `state`. Configure its behavior in Home Assistant with an event automation.
- **Script:** open its detail page and short press to run it.
- **Climate:** see the HVAC mode, current temperature, and target. Rotate to adjust
  the target using the entity's temperature step and limits. For thermostats with
  a low/high target range, rotation shifts both targets together, preserving the
  range width. Short press cycles through the entity's advertised HVAC modes.
  Temperatures use Home Assistant's configured temperature unit. Unsupported or
  missing temperature targets cannot be adjusted. Hold to return as with other
  device detail pages.

Enable **Allow the device to perform Home Assistant actions** in the device's
ESPHome integration options. Sensor presses can execute any script or automation,
for example this Home Assistant automation (replace the entity IDs):

```yaml
alias: Knob sensor press
triggers:
  - trigger: event
    event_type: esphome.onx_sensor_press
    event_data:
      entity_id: sensor.living_room_temperature
actions:
  - action: script.turn_on
    target:
      entity_id: script.living_room_comfort
mode: single
```

To trigger an existing automation instead, use `action: automation.trigger` and
its `automation.*` target. Add `data: {skip_condition: false}` if that automation's
conditions should be checked. To require an open door/window, select its
`binary_sensor.*` entity and add `state: "on"` under `event_data`. Here “viewing a
sensor” means its detail page is open; pressing in the entity list only opens it.

A reproducible development container is included in `.devcontainer/`. In a
Codespace, run `esphome config ONX2424G013.yaml`, then
`esphome compile ONX2424G013.yaml`. Never commit Wi-Fi or API credentials.

## Weather display

Weather is available both on the standby clock and as a regular device entry.
In Home Assistant, use **Refresh Entity List**, choose a `weather.*` entity under
**9. Weather**, and press **Save Entity Config** to configure standby weather.
The standby clock shows a condition icon, temperature, and humidity after the
**Standby timeout** (300 seconds by default). Sleep turns the screen off.

To view weather at any time, select a `weather.*` entity in a **Device1–Device10**
slot and save. Open it from the knob's entity list to see the current condition,
temperature in the entity's unit, and humidity. Weather detail is read-only;
hold the knob to return. Standby and device entries can use different weather
entities. These views use Home Assistant's current weather state and attributes;
they do not request hourly or daily forecasts.

## Wi-Fi provisioning and timer displays

This fork supports Improv Serial on the existing native USB Serial/JTAG port at
115200 baud, plus Improv BLE. Use a compatible Improv client and press the knob
to authorize BLE provisioning for one minute. BLE starts while disconnected and
is disabled after Wi-Fi connects; the captive portal remains available.

Refresh the Home Assistant entity list, choose a `timer.*` entity in a device
slot, and save. Open it with the knob to see its live countdown and shrinking
progress ring. Paused timers freeze; idle timers show zero. The view is read-only.
Active timers use the HA `finishes_at` timestamp, not its static `remaining`
attribute; the display clock must be synchronized. The ring animates locally at
10 frames per second. Missing/offline data shows a syncing/offline indication.

Voice PE and ReSpeaker timers are internal to the voice device, rather than HA
`timer.*` helpers. The matching configurations in `fixtse/esp-ota` expose a
**Voice timer** sensor using the `onx.timer.v1` snapshot format. Select that sensor
in a device slot to display the next running timer (or next paused timer), its
name, and the same animated ring. Updates stop extrapolating after five seconds
without feedback. A ringing voice timer pulses “Time is up”. Multiple timers
remain managed by the voice device; the display shows the next one.

Automatic updates use the `onx2424g013_manifest.yml` release manifest in
`fixtse/esp-ota`. Source packages come from `fixtse/OpenNextion-KNOB-ESPHome`.
The factory image is for compatible ONX2424G013 hardware only.

## Searchable configuration and press actions

In Home Assistant, open **Settings → Devices & services → ESPHome → this device**,
then click **Visit**. It opens the searchable configuration page directly.
You can open `http://<knob IP>/` or `http://<knob IP>/knob` directly. The page runs on the knob and needs no cloud frontend.
Select a slot, search by entity name, ID, or room, and use the type filters to
find an entity. Local HMI GPIO switches are included. An entity ID can also be
entered directly if it is absent from the cached list. Use **Refresh from Home
Assistant** after configuring the existing HA host/token to update the list.

Each slot has **Show on the display** and **When pressed** settings. Choose an
automation or script for the press action, then **Save slot**. No Home Assistant
event automation is needed. Automation conditions are checked; scripts use
`script.turn_on`. **Device default** keeps native controls, while **No action**
disables a short press. Configured actions replace a short press only; rotation
and holding retain their normal behavior. Light mode changes and held cover
position adjustment remain available.

Actions persist on the device and are bound to the selected entity and slot, so
reordering compact display slots or changing an entity cannot run another
entity's action. The existing HA dropdowns remain compatible, but the webpage
provides search and paired action selection. The page never returns the HA token.

Wi-Fi credentials are restored and saved by ESPHome's native Wi-Fi component;
normal startup and reconnects do not restart it or switch its preference key.
Under the Home Assistant device's configuration controls, **Restart device**
reboots while keeping settings. **Reset device** performs a factory reset,
clearing the knob's saved Wi-Fi, Home Assistant configuration, and press actions
and returning it to provisioning. Use Restart for an ordinary reboot.
