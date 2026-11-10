# OpenNextion Knob by fixtSE

**Your home, music and everyday shortcuts—one turn away.**

Turn the **ONX2424G013** into a Home Assistant controller with a round screen and
one simple knob. Dim a lamp, change the temperature, pick a playlist, see a useful
reminder, or adjust your computer's volume through Home Assistant.

Made by [fixtSE](https://fixtse.com/).

[Get started](docs/setup.md) · [User guide](docs/web-configuration.md) · [Firmware downloads](https://github.com/fixtse/esp-ota/releases) · [简体中文](README.zh-CN.md)

## What can it do?

| Make it part of your day | What you can do |
| --- | --- |
| Set the mood | Switch lights on, dim them, change colors and adjust color temperature. |
| Get comfortable | Adjust heating, cooling, fans, switches, plugs and curtains. Available controls follow what your device supports. |
| Enjoy your music | Change volume, play or pause, skip tracks, and choose your own playlists or radio stations. Album art and a progress ring show what's playing. |
| Keep an eye on things | See sensor readings, add a second piece of information, or open a 12-hour graph. Browse current weather and forecasts. |
| Keep timers nearby | Active Home Assistant and supported voice/speaker timers pop up automatically. Hide their regular pages to keep your list tidy. |
| Remember what matters | Let a Home Assistant automation show a washing reminder, door alert or next meeting on the screen. Press to dismiss; turn to read longer messages. |
| Create your own controls | Choose what clockwise and counterclockwise turns do. Run Home Assistant scripts or automations, optionally show the result, and give climate controls a custom press action. |

[Learn the knob controls](docs/knob-controls.md) · [Playlist guide](docs/web-configuration.md#playlists-and-stations) · [Reminders and calendars](docs/display-notifications.md) · [Custom turns and desktop controls](docs/gesture-controls.md)

## Make it yours

- **Up to 20 controls.** Start with the ones you use most, then add more. Search by name or room, rename, reorder and duplicate them.
- **Easy to find your place.** Icons follow the lower curve of the screen, with the current page centered.
- **A quieter display.** Keep hidden timers working in the background. Choose the clock when idle, or show an actively playing media player with a clock fallback.
- **Your look and routine.** Choose brightness, dial sensitivity, date and clock formats, progress-ring color, standby weather and scheduled sleep.
- **Set up from your phone or computer.** The knob has its own configuration webpage, with help links next to the settings, light/dark themes and configuration backups.
- **Keep familiar icons.** Use your Home Assistant entity icons, with built-in alternatives when an icon cannot be downloaded.
- **Stay up to date.** Install wireless updates through the knob's **Firmware Update** entity in Home Assistant.

## Get started

1. [Install the firmware and connect to Wi-Fi](docs/setup.md). Bluetooth setup shows when to press the knob to allow pairing.
2. Add the knob through Home Assistant's **ESPHome** integration and enable **Allow the device to perform Home Assistant actions**.
3. Open **Visit** from the device page, or the web address shown on the knob. Sign in to browse your Home Assistant devices, choose a control and **Save slot**.

No Home Assistant access token needs to be pasted into the knob. Computer controls
need an existing Home Assistant connection or action for your computer; the knob
does not install desktop software.

## Find your guide

| I want to… | Guide |
| --- | --- |
| Connect Wi-Fi or get started | [Setup and connection help](docs/setup.md) |
| Add controls, change the display or make a backup | [Web configuration](docs/web-configuration.md) |
| Learn turning, pressing and playlist selection | [Using the knob](docs/knob-controls.md) |
| Assign turns or show action feedback | [Custom gestures](docs/gesture-controls.md) |
| Display reminders or calendar events | [Display notifications](docs/display-notifications.md) |
| Understand icons and album art | [Icons and media](docs/icons-and-media.md) |

**Starting over?** A separate **10-second hold** in normal mode clears Wi-Fi and
device configuration. Read the [reset instructions](docs/knob-controls.md#reset-wi-fi-and-device-configuration)
before using it. Pressing to dismiss a reminder does not reset your knob.
