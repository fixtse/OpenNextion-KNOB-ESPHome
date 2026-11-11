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
| Enjoy your music | Change volume, play or pause, skip tracks, and choose your own playlists or radio stations. Choose a classic player or full-screen album art, with a progress ring at the screen edge. Playlist selection follows each wheel step without scrolling animations. |
| Keep an eye on things | See sensor readings, add a second piece of information, or open a 12-hour graph. Browse current weather and forecasts. |
| Keep timers nearby | Active Home Assistant and supported voice/speaker timers pop up automatically. Hide their regular pages to keep your list tidy. |
| Remember what matters | Show reminders, door alerts or calendar messages with an optional image. HTML line breaks become readable text, and longer messages scroll. Failed image downloads fall back to text. Press to dismiss. |
| Create your own controls | Assign clockwise, counterclockwise and held turns to Home Assistant scripts or automations. Choose custom press actions on supported controls and optionally display returned values in a brief popup. |
| Run everyday shortcuts | Use scripts, automations, scenes and buttons, or adjust number and selection helpers. Script and automation pages show clear running/idle or enabled/disabled states, and can display a configured feedback value. |

[Learn the knob controls](docs/knob-controls.md) · [Playlist guide](docs/web-configuration.md#playlists-and-stations) · [Reminders and calendars](docs/display-notifications.md) · [Custom turns and desktop controls](docs/gesture-controls.md)

## Make it yours

- **Up to 20 controls.** Start with the ones you use most, then add more. Search by name or room, rename, reorder and duplicate them.
- **Easy to find your place.** Icons follow the lower curve of the screen, with the current page centered.
- **A quieter display.** Keep hidden timers working in the background. Choose the clock when idle, or show an actively playing media player with a clock fallback.
- **Your look and routine.** Choose display orientation, brightness, dial sensitivity, date and clock formats, media style, progress-ring color, standby weather and scheduled sleep.
- **Set up from your phone or computer.** The knob has its own configuration webpage, with entity icons beside slot names, live previews, help links, light/dark themes and configuration backups.
- **Keep familiar icons.** Use your Home Assistant entity icons, with built-in alternatives when an icon cannot be downloaded.
- **Stay up to date.** Install wireless updates through the knob's **Firmware Update** entity in Home Assistant.

## Get started

1. [Install the firmware and connect to Wi-Fi](docs/setup.md). Bluetooth setup shows when to press the knob to allow pairing.
2. Add the knob through Home Assistant's **ESPHome** integration and enable **Allow the device to perform Home Assistant actions**.
3. Open **Visit** from the device page, or the web address shown on the knob. Sign in to browse your Home Assistant devices, choose a control and **Save slot**.

No Home Assistant access token needs to be pasted into the knob. Computer controls
need an existing Home Assistant connection or action for your computer; the knob
does not install desktop software.

## Send a notification

Use the single **show_message** action for both image and text-only messages:

```yaml
action: esphome.onx2424g013_dbbc54_show_message
data:
  title: Reminder
  message: Take the washing out.<br/>The cycle has finished.
  image_url: ""
  timeout_seconds: 60
```

Replace the device prefix with yours. Set `image_url` to a reachable HTTP or HTTPS
image URL, or leave it empty for text only. Include all four fields; use
`timeout_seconds: 0` to wait for dismissal. **dismiss_message** clears it remotely.

Image notifications reserve space for the picture and at least three text lines.
Downloads have size and time limits, and a newer message cancels the previous
image request. The knob converts HTML breaks and entities into plain text rather
than displaying markup. [Notification examples and image limits](docs/display-notifications.md).

## Show action results

In a slot's **Press action** tab, enable **Show the returned value after pressing**
for a script that returns `value` and optional `unit`. Automations can send a value
back through the knob's feedback action. This setting is separate from gesture
feedback, and the brief popup does not consume wheel input.
[Press and gesture response examples](docs/gesture-controls.md#press-action-results).

## Find your guide

| I want to… | Guide |
| --- | --- |
| Connect Wi-Fi or get started | [Setup and connection help](docs/setup.md) |
| Add controls, change the display or make a backup | [Web configuration](docs/web-configuration.md) |
| Learn turning, pressing and playlist selection | [Using the knob](docs/knob-controls.md) |
| Assign turns, customize a press or show returned values | [Custom actions and feedback](docs/gesture-controls.md) |
| Display messages, images or calendar events | [Display notifications](docs/display-notifications.md) |
| Understand icons and album art | [Icons and media](docs/icons-and-media.md) |

**Starting over?** A separate **10-second hold** in normal mode clears Wi-Fi and
device configuration. Read the [reset instructions](docs/knob-controls.md#reset-wi-fi-and-device-configuration)
before using it. Pressing to dismiss a reminder does not reset your knob.
