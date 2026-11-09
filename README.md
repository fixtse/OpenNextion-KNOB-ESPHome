# Home Assistant Knob Control

**Your everyday smart home controls, one turn away.**

Turn the ONX2424G013 into a compact Home Assistant controller. Adjust the room,
start a routine, or check what matters—all from a round display and a single knob.

[Get started](docs/setup.md) · [Download firmware](https://github.com/fixtse/esp-ota/releases) · [简体中文](README.zh-CN.md)

## Control your home

| Control | What you can do |
| --- | --- |
| Lights | Switch power, dim brightness, adjust color, and fine-tune color temperature. |
| Climate | Set temperature, change HVAC mode, adjust range endpoints, and choose supported presets, fan, swing, and humidity settings. |
| Fans | Adjust speed, toggle power, and choose supported presets, oscillation, or direction. |
| Media | Adjust volume, skip tracks, select a source, mute supported players, and follow playback on a circular progress ring. |
| Switches & covers | Control switches, plugs, and cover position. |
| Helpers & routines | Adjust numbers, confirm select options, activate scenes, press buttons, and run scripts or automations. |
| At a glance | View sensors, weather, and timer countdowns with an animated progress ring. |

Available settings adapt to the capabilities of each Home Assistant entity.

Display text supports accented Latin characters (for example, Beyoncé) and these monochrome emojis:

❤ ☀ ☁ ☂ ☕ ❄ ⚡ ✅ ❌ ⭐ 🌙 🌞 🌧 🌡 🌈 🎵 🎶 🎧 🎤 🎸 🎹 🎷 🎺 🎻 🥁 📻 💿 📀 🔊 🔇 🔔 🔥 💡 🏠 😀 😊 😂 😍 😎 😢 😭 👍 👏 💙 💚 💛 💜 🎉

## Make it yours

- **Ten configurable slots.** Search by name, entity, or room; reorder and duplicate your controls.
- **Simple rotary input.** Turn to adjust, press to act, and hold to return. Hold and turn to choose secondary settings where available.
- **Your Home Assistant icons.** Download the MDI icons chosen for your entities, with built-in defaults when a download is unavailable.
- **Clear feedback.** See requested values immediately, confirmation from Home Assistant, and connection or timeout messages.
- **No device token to manage.** Home Assistant controls use the ESPHome integration; sign in through your browser to choose entities.
- **Built-in configuration page.** Set up from your phone or computer, with themes, unsaved-change recovery, and configuration backup and restore.
- **A display that fits your day.** Choose brightness, knob sensitivity, standby weather, and a sleep schedule.
- **Easy upkeep.** Wi-Fi provisioning, automatic state recovery, manual retry without restarting, and wireless firmware updates.

## Get started

1. [Install the firmware](docs/setup.md) on a compatible **ONX2424G013** and connect it to Wi-Fi.
2. Add the device through Home Assistant’s ESPHome integration and enable **Allow the device to perform Home Assistant actions**.
3. Open the device’s **Visit** link—or `http://<knob IP>/`—to sign in to Home Assistant, choose your entities, and save your slots.

[Control guide](docs/knob-controls.md) · [Configuration & backups](docs/web-configuration.md)
