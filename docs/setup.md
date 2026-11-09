# Set up your knob

Use compatible **ONX2424G013** hardware: ESP32-S3, 16 MB flash, Octal PSRAM,
and a GC9A01A round display. Use a data-capable USB cable for the first installation.

## First installation

Clone the complete repository; the configuration needs its bundled components,
packages, fonts, and assets. Build with **ESPHome 2026.10.2**:

```bash
git clone https://github.com/fixtse/OpenNextion-KNOB-ESPHome.git
cd OpenNextion-KNOB-ESPHome
docker run --rm -v "$PWD":/config -w /config \
  ghcr.io/esphome/esphome:2026.10.2 compile ONX2424G013.yaml
```

Open [ESPHome Web](https://web.esphome.io/) in Chrome or Edge, connect the device,
and install the generated factory image:

```text
.esphome/build/onx2424g013/.pioenvs/onx2424g013/firmware.factory.bin
```

If the device does not enter download mode automatically, hold **BOOT**, tap
**RESET** if available, and release **BOOT** when the upload starts. Restart after
installation. Factory images are for USB installation; `.ota.bin` files are for
wireless updates on an already configured device.

## Connect and customize

1. Follow the device’s Wi-Fi provisioning screen. A captive portal is available
   when no network is configured; compatible Improv clients can also provision
   over USB or Bluetooth.
2. Add the device to Home Assistant through the ESPHome integration.
3. Enable **Allow the device to perform Home Assistant actions** in its integration options.
4. Open **Visit** on the device page, or browse to `http://<knob IP>/`.
5. Sign in to Home Assistant in the configuration page, search for your entities,
   and save up to ten control slots.

For regular updates, use the device’s firmware update control.
[Published firmware](https://github.com/fixtse/esp-ota/releases) includes the OTA image.

## Everyday operation

Turn to browse or adjust, and press to open a control or perform its action.
Hold to return. On climate, fan, and media controls, hold and turn to choose
secondary settings. Available controls depend on the selected entity.

See the [control guide](knob-controls.md) and [configuration guide](web-configuration.md)
for entity behavior, backups, and display settings.
