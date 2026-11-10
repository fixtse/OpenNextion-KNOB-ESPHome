# Set up your knob

[All guides](../README.md#find-your-guide) · [Choose your controls](web-configuration.md)

You need an **ONX2424G013** knob, a Wi-Fi network and Home Assistant.
If your knob already has this firmware, start with Wi-Fi below. For a new or
unflashed device, see [first installation](#first-installation).

## Connect to Wi-Fi

### With Bluetooth

1. Open your compatible Improv Bluetooth setup tool on your phone or computer and choose the knob.
2. When the display says **Press the knob to allow setup**, press it once. Continue in the setup tool.
3. Choose your Wi-Fi network and enter its password. The knob shows that it is connecting.
4. When connected, the setup screen closes. If there are no controls yet, the knob shows its web address and the next steps.

If the screen says **Could not connect**, check the network and password, then
try again. Bluetooth support depends on your browser and setup tool; use the
hotspot below if Bluetooth setup is unavailable.

### With the Wi-Fi hotspot

1. Join the Wi-Fi network named **ONX2424G013-…** from your phone or computer.
2. Open **http://192.168.4.1** and enter your home Wi-Fi details.
3. Rejoin your home Wi-Fi after the knob connects.

A connected knob should show its normal controls or connected onboarding screen,
rather than ask you to join the hotspot again. Saved networks reconnect after
restarting; a temporary Wi-Fi outage does not mean you need to erase your setup.

## Add it to Home Assistant

1. In Home Assistant, open **Settings → Devices & services** and add the discovered **ESPHome** device. If it is not discovered, add ESPHome manually using the knob's IP address.
2. Open the integration's options and enable **Allow the device to perform Home Assistant actions**. Without this, readings may work while controls do not.
3. Open the device's **Visit** link, or type the web address shown on the knob into your browser.
4. On the knob webpage, open **Home Assistant** and sign in to browse your entities. An entity is a Home Assistant item such as a lamp, thermostat, sensor or script.
5. Open **Display & actions**, choose an entity and click **Save slot**. Add up to **20** controls as you need them.

Browser sign-in helps you choose items; adding the ESPHome integration connects
the physical knob. Both steps matter. No long-lived access token needs to be
copied into the device.

## Keep it updated

Use the knob's **Firmware Update** entity in Home Assistant to install available
wireless updates. Keep power connected during installation. The
[published releases](https://github.com/fixtse/esp-ota/releases) contain the wireless
`.ota.bin` image; it is not a first-install USB image.

Before changing a large setup, [download a configuration backup](web-configuration.md#backups-and-diagnostics).

## If something does not work

- **Readings appear but turning does nothing:** check the Home Assistant action permission above.
- **The browser has no entity list:** sign in on the webpage's **Home Assistant** section. This is separate from the knob's connection.
- **A device is unavailable:** check it in Home Assistant first. Use **Retry knob entity states** on the webpage to request fresh readings.
- **Wi-Fi is temporarily down:** allow saved Wi-Fi to reconnect. Use a [configuration reset](knob-controls.md#reset-wi-fi-and-device-configuration) only if you intend to start over.

## First installation

This section is for installing firmware on a new device. If you are unfamiliar
with firmware flashing, ask the person supplying your knob to install it first.
The supported hardware is **ONX2424G013** with ESP32-S3, 16 MB flash, Octal PSRAM
and a GC9A01A round display. Use a USB cable that supports data.

The source configuration needs its bundled components, packages, fonts and assets.
For users building their own factory image with **ESPHome 2026.10.2**:

```bash
git clone https://github.com/fixtse/OpenNextion-KNOB-ESPHome.git
cd OpenNextion-KNOB-ESPHome
docker run --rm -v "$PWD":/config -w /config \
  ghcr.io/esphome/esphome:2026.10.2 compile ONX2424G013.yaml
```

Open [ESPHome Web](https://web.esphome.io/) in Chrome or Edge, connect the knob,
and install the generated factory image:

```text
.esphome/build/onx2424g013/.pioenvs/onx2424g013/firmware.factory.bin
```

If download mode does not start automatically, hold **BOOT**, tap **RESET** if
available, and release **BOOT** when uploading starts. Restart after installation,
then follow the Wi-Fi steps above. Compatible Improv tools can also set Wi-Fi over USB.
