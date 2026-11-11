# Using your knob

[All guides](../README.md#find-your-guide) · [Change your settings](web-configuration.md)

Turn to browse your controls, then press to open one. Turn again to adjust it.
Hold without turning to return to the list. The curved icons along the lower
edge help you find your place; the current page stays in the center.

## Everyday controls

| Control | Turn | Short press | Hold and turn |
| --- | --- | --- | --- |
| Lights | Adjust the selected light setting | Switch on/off | Choose another supported light setting |
| Climate | Adjust the selected temperature or setting | Change heating/cooling mode, confirm an option, or run your custom press action | Choose temperature, range endpoints, preset, fan, swing or humidity where supported |
| Fans | Adjust speed or preview an option | Switch on/off or confirm an option | Choose speed, preset, oscillation or direction where supported |
| Numbers | Adjust the value | No extra action needed | — |
| Choices | Browse the options | Confirm your choice | — |
| Scenes and buttons | — | Activate | — |
| Media players | Adjust volume | Play/pause; run your chosen action when idle | Next track clockwise; previous track counterclockwise |
| Weather | Browse readings and daily forecasts | Retry unavailable forecasts | — |

Some devices offer fewer options. Hold and turn to choose a secondary setting,
release, then turn normally to adjust it. Home Assistant must allow the knob to
perform actions: see [connection setup](setup.md#add-it-to-home-assistant).

A brief checkmark means the request was sent, rather than a guarantee that the
other device finished it. If **No confirmation** appears, check the device and
permissions in Home Assistant. Missing or unavailable readings show clearly;
reconnecting does not replay your old turns.

## Music and playlists

Open a media player to see its artwork, track details and progress ring.
Turn for volume; press to play/pause a loaded track. When idle, pressing can
run the script or automation you selected under **Press action**.

[Add playlists or radio stations](web-configuration.md#playlists-and-stations)
in that player's **Playlists** tab first. On the knob:

1. Hold until **Release now / Open playlists** appears (about half a second), then release.
2. Turn through the list to highlight what you want.
3. Press to start it. The screen returns to the media player automatically.

Continue holding past the playlist cue (about 1.5 seconds total) to return to the
device list instead. You can also use the playlist hold-and-release gesture to
close the list. Without playlists configured, the normal shorter return hold applies.
Holding and turning in the player skips tracks where the player supports it.

## Your own turn and press actions

The webpage's **Gestures** tab can replace ordinary clockwise/counterclockwise
turns independently. Optional held turns are configured separately. For example,
a sensor can show computer volume while turning runs the Home Assistant script
that changes it. Custom turns work while that control is opened, so main-list
navigation stays familiar.

Climate controls also accept a custom **Press action**. Choose **Default** to keep
their normal mode control. [Set up custom actions and feedback](gesture-controls.md).

## Timers and reminders

Active supported timers can appear automatically, even when their normal pages
are hidden. Turn to return to browsing; an active timer may return after ten idle
seconds. Start, pause or cancel it in Home Assistant or on the originating speaker.
[Timer setup](web-configuration.md#timers-without-clutter).

A Home Assistant reminder appears over your current page. **Press to dismiss**
or **turn to scroll** longer text. Those movements do not operate the control
underneath. Only the latest message is kept; timer popups have priority.
[Create reminders and calendar messages](display-notifications.md).

## When you leave it idle

Choose **Clock** or **Playing media** in **Device settings → Idle screen**.
Playing media uses an actively playing configured player from the main carousel
and falls back to the clock when stopped. Timers and scheduled sleep keep priority.
The automatic idle view and scheduled screen-off hours are separate settings.
[Display and sleep options](web-configuration.md#display-and-idle-screen).

## Reset Wi-Fi and device configuration

In normal mode, press and hold the knob for **10 seconds** to clear saved Wi-Fi
credentials and device configuration, then reboot. You will need to reconnect
Wi-Fi and configure your devices again, or restore a configuration backup.
The reset is unavailable during sleep, while waking from standby, and while
viewing an offline device. Turning while holding can also suppress the reset
when that gesture adjusts a secondary setting.

A press used to dismiss a [display notification](display-notifications.md)
**cannot trigger this reset**, even if held for 10 seconds. This also applies
when a notification appears while the knob is already held. The press stays
consumed until release, even if the notification times out or Home Assistant
dismisses it first. Release the knob before starting a separate reset hold.

## Media player appearance

In the configuration webpage's **Device settings**, choose **Media player view →
Material · tonal card** and save. Classic remains the default. The selection
applies to media detail views and the playing-media idle screen, and is included
in configuration backups.

Material fills the entire circular display with artwork, preserving proportions
and cropping overflow when needed to avoid gaps. Each text line has its own
compact translucent backing, including the device name. The pause icon is
centered; there is no Playing/Paused text pill. The circular playback progress
indicator retains its selected color and paused behavior. Existing knob controls
and playlist navigation work in both views. Missing artwork falls back to the
entity icon. Press the physical knob to play or pause.

When the media player reports **off**, its detail view closes and returns to the
main carousel in either appearance. **Idle** and **paused** keep the player view
open. Losing the connection or receiving an unavailable state does not trigger
this automatic return.

A short press means press and release, without waiting for a hold cue. Media
play/pause and playlist selection act on release. Button edges and their original
timestamps are buffered during display redraws so a quick click is retained;
contact debounce is 5ms. The 0.5-second playlist gesture and longer back/reset
gestures continue to use the physical hold duration.

## Sensor graph scale

The 12-hour sensor graph shows its maximum value beside the top gridline and
minimum beside the bottom gridline. Compact 12px labels use a narrow left gutter
without increasing the graph height. Values use the same unit as the current
reading above the graph. Precision expands for narrow ranges so different bounds
remain distinguishable; constant history shows the same value at both bounds.
The existing **12h ago / Now** labels describe the horizontal time axis.

To open playlists with a short press when the player is idle, select the media
player slot on the configuration webpage and choose **When idle → Open playlists**,
then save the slot. Add entries in the **Playlists** tab first. Playing or paused
media still uses short press for play/pause; the existing hold gesture can also
open playlists. Selecting an entry runs its configured action and returns to the
player. The idle-press option is saved with configuration backups.

## Numeric value rings

Opening a `number` or `input_number` shows its value and a ring around the screen
edge. The ring uses the minimum and maximum advertised by Home Assistant and
updates immediately with a requested dial adjustment. Percentage sensors, such
as PC audio volume, use a 0–100 ring that follows their live reading or configured
feedback value. Fan percentage controls use the same ring. Sensor graphs retain
their graph presentation. Missing values or invalid ranges hide the ring.

Number, input number, fan, selection, scene and button controls can all be opened
from the main list. Their detail icons and labels stay clear of the value and
confirmation text.
