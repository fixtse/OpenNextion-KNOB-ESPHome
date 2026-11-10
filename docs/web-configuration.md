# Make the knob yours

[All guides](../README.md#find-your-guide) · [First-time setup](setup.md) · [Using the knob](knob-controls.md)

Open **Visit** on the knob's Home Assistant device page, or open the web address
shown on the knob. Use a phone or computer on a network that can reach it.
Settings stay on the knob after you save them.

## Add and arrange controls

1. Open **Display & actions** and select a slot. A slot is a place for one control.
2. In **Display**, click **Choose entity**. Search by name or room and select the Home Assistant item you want.
3. Give it a shorter **Display name** if you like, then click **Save slot**.
4. Use **Add slot** for more controls, up to **20**. Move controls earlier/later or duplicate one into an empty slot.

Start with a few favorites. Empty slots do not need to be filled. The main screen
shows readings as you browse, and the curved row of icons keeps the selected page
in the center. Press to open a control.

**Save slot** saves the selected control. **Save all** saves all changed slots.
Unsaved changes are marked; **Discard changes** restores the last saved version.
If a save fails, already saved slots stay saved and the others remain marked.
The browser can offer to recover an unsaved draft after a reload—still save your
work before closing it.

The **On your knob** preview shows live readings when available. Switch between
**Main screen** and **Opened entity** to see each layout. A new entity needs to be
saved before its readings can load.

## Press actions

Open a slot's **Press action** tab to choose what a short press does.

- **Default** keeps the control's usual behavior.
- **Choose action** lets supported controls run a Home Assistant script or automation.
- **No action** disables the custom press where supported.

For **climate** controls, Default keeps the normal heating/cooling mode control;
you can choose a script or automation instead. For **media players**, the custom
action runs when the player is idle; a loaded playing or paused track keeps
play/pause. Some controls need their own press behavior and do not allow an override.
Automation conditions still apply.

[Everyday press controls](knob-controls.md) · [Custom climate and turn actions](gesture-controls.md)

## Custom turn actions

In **Gestures**, choose separate actions for **Turn clockwise** and **Turn
counterclockwise**. These are ordinary turns. **Hold + turn** is a separate,
optional pair of actions. Assignments apply only while that control is opened;
browsing the main list still works normally.

For example, show a computer-volume sensor and make turning run the Home
Assistant script that changes your computer's volume. Choose an optional
**Feedback entity** to show the resulting value, and enable the brief popup if
you want a quick confirmation. Leave a gesture at **Default** to keep its usual
behavior. Advanced script responses and event routing are optional.

[Set up gestures and feedback, with desktop examples](gesture-controls.md)

## Playlists and stations

1. Choose a media player slot and open **Playlists**.
2. Click **Add playlist or station** and choose the script or automation that starts it.
3. Edit the friendly name if needed, arrange the entries and **Save slot**.

You can add up to **eight entries per player**. Set the intended player and music
source in the Home Assistant action itself; adding a row does not create a music
playlist in Home Assistant.

On the knob, open the player, hold until **Release now / Open playlists** appears,
then release. Turn through the Settings-style list and press your choice. The
knob sends the action and **returns to the player automatically**. A checkmark
means the request was sent; the player may take a moment to start.

[Media controls and holding gestures](knob-controls.md#music-and-playlists)

## Sensors and graphs

Choose a sensor slot and open **Display**. Keep **Value** for a simple reading,
or choose **12-hour graph when opened** for a numeric sensor. The main screen
still shows its current value. Graphs use Home Assistant's recorded statistics
where available; otherwise they show readings collected since the knob started.
Some sensors do not have recorded statistics. Gaps stay empty, and locally
collected history is lost on restart.

Add a **Secondary entity** for a second piece of information beneath the main
reading. **Secondary text** can turn that reading into a useful sentence:

| Write this | What it shows |
| --- | --- |
| `{value} {unit}` | The secondary reading and its unit |
| `Resets {relative}` | A relative time for a date/time entity |
| `Resets {relative} ({time})` | Relative time plus the clock time |
| `Next: {date}` | A date using your chosen format |
| `{datetime}` | Both the date and time |

For example, show remaining allowance as the main sensor and its reset time as
the secondary entity. Secondary text replaces the usual name below the reading.
Missing data is shown explicitly rather than as a made-up reading.

## Timers without clutter

Add a Home Assistant timer or a supported voice/speaker timer sensor to a slot.
Then select **Hide timer from carousel** in **Display** if you do not want a
regular page for it. It still counts toward the **20 configured slots**, and its
automatic popup stays active.

The knob supports Home Assistant `timer.*`, compatible Voice PE/SoundSense voice
timer sensors and Google Home integration timer sensors. Ordinary text sensors
are not automatically treated as timers.

Ringing timers take priority, followed by the nearest running timer and paused
timers. Active timers appear while browsing, or after ten idle seconds on an
opened control. Turn to return to browsing; an active timer can return after ten
idle seconds. Timer control remains in Home Assistant or the originating speaker.
Settings, scheduled sleep and a deliberately switched-off screen are not interrupted.

## Display and idle screen

Open **Device settings**, make your choices, then **Save device settings**.

| Setting | Choose what suits you |
| --- | --- |
| Screen brightness | 10–100%. Saving brightness turns the screen on. |
| Knob sensitivity | Fast, Balanced or Fine. |
| Standby after | How long to wait before the automatic idle view, from 1–900 seconds. |
| Automatic idle screen | Allow it from the main screen only, from any screen, or never automatically. |
| Idle screen | **Clock** is the default. **Playing media** shows an actively playing configured player from the main carousel, with the clock when stopped. |
| Standby weather | Choose the weather entity used by the standby display. |
| Date and clock | Choose day/month order, named months, 12/24-hour time and optional AM/PM. |
| Media progress ring | Pick a preset or your own color for playback and timer rings. |

The media idle view uses a player already configured on the knob. Timers and
scheduled sleep keep priority. Turn or press to leave the idle view before using
the underlying control.

### Scheduled sleep

Enable the sleep schedule, choose its start/end times, and set how long to wait
before switching off the screen during those hours. The schedule uses Home
Assistant's clock. Matching start/end times mean all day. This is separate from
the idle clock/media view.

## Reminders and calendar messages

Home Assistant automations can show a message over the current view, such as
“Take the washing out” or “Meeting in 10 minutes.” No display slot is needed.
Press to dismiss it, or turn to scroll longer text. A normal reminder lasts
60 seconds; an automation can choose a different duration or keep it until dismissed.

Timer popups take priority. Messages wait during setup, scheduled sleep or when
the screen is deliberately off. Only the latest message is kept.

[Create a reminder or calendar popup](display-notifications.md)

## Home Assistant connection

The **Home Assistant** section lets your browser sign in to search entities and
sync their icons. This is separate from adding the knob's ESPHome integration.
Enable **Allow the device to perform Home Assistant actions** in that integration
so the knob can control devices.

The status strip shows three different connections: reaching the knob, the
knob's connection to Home Assistant, and browser sign-in. Use **Refresh entity
list** after changing an icon or name in Home Assistant. **Retry knob entity
states** asks for fresh readings without restarting or replaying old commands.

[Connection troubleshooting](setup.md#if-something-does-not-work) · [Icons and album art](icons-and-media.md)

## Backups and diagnostics

In **Backup & diagnostics**, click **Download saved configuration** before making
large changes. Backups include saved controls, names, press/turn actions,
playlists, sensor display choices and device settings. They exclude credentials.

**Import configuration** opens a backup as an unsaved draft. Review it, then save
slots and device settings separately to apply it. Importing alone does not change
the knob. Older supported backups receive defaults for newer settings.

**Download diagnostics** provides firmware, uptime and connection information
for troubleshooting, without entity names, addresses or credentials. Wireless
updates are available through the **Firmware Update** entity in Home Assistant.
