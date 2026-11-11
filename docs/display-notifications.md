# Reminders and calendar messages on your knob

[All guides](../README.md#find-your-guide) · [Timers](web-configuration.md#timers-without-clutter)

Put a useful message where you can see it: washing finished, a door left open,
or your next meeting. Home Assistant sends the message; you do not need to give
it a display slot. Press the knob to dismiss, or turn to scroll longer text.
The underlying control is not changed by those movements.

## Send your first reminder without writing YAML

1. In Home Assistant, open **Settings → Automations & scenes** and create an automation.
2. Choose when it should run, such as a time of day or a sensor changing state.
3. Add an action and search for **show_message** under your knob's ESPHome device name.
4. Fill in **title** (for example, “Reminder”) and **message** (“Take the washing out”). Set **image_url** to a reachable image URL, or an empty string for text only.
5. Set **timeout_seconds** to 60, save the automation, and try its message action.

For a different duration, change **timeout_seconds**. Use 120 for two minutes, or 0 to keep it until you dismiss it.
Use **dismiss_message** in an automation to clear it remotely. If the actions
are missing, update the knob and let its ESPHome integration reconnect.

Timers take priority over messages. Messages wait while the screen is deliberately
off, during scheduled sleep or during setup. Only the newest message is kept,
and messages do not survive restarting the knob.

## Copy-and-adapt examples

These examples are for Home Assistant's YAML editor. The calendar example below
shows a meeting title automatically, rather than needing to type it each time.

The knob exposes ESPHome actions in Home Assistant for reminders, calendar events,
and messages from other automations. Install the firmware and let Home Assistant
reconnect to discover the new actions. In Developer Tools → Actions, search for
`show_message` under your knob's ESPHome device name.

The examples use `onx2424g013_dbbc54`; replace that prefix if your device has a
different name.

## Show a reminder

```yaml
action: esphome.onx2424g013_dbbc54_show_message
data:
  title: Reminder
  message: Take the washing out.
  image_url: ""
  timeout_seconds: 60
```

The message appears for 60 seconds. Press the knob to dismiss it, or turn it to
scroll longer text. Dismissing and scrolling do not operate the page underneath.
The underlying page is retained.

Holding a notification-dismiss press for 10 seconds does not reset the device or
erase its settings. The entire press is consumed until release, including if the
message times out or is dismissed remotely. A notification appearing during an
existing hold also consumes that press. See [reset Wi-Fi and device configuration](knob-controls.md#reset-wi-fi-and-device-configuration)
for the separate 10-second reset gesture in normal mode.

For a custom timeout, use `show_message`:

```yaml
action: esphome.onx2424g013_dbbc54_show_message
data:
  title: Next meeting
  message: Team catch-up in ten minutes. Bring the project notes.
  image_url: ""
  timeout_seconds: 120
```

Set `timeout_seconds: 0` to keep a message until dismissed. Positive timeouts are
limited to one hour. An empty title displays as “Reminder”. Empty messages are
ignored. Titles are limited to 96 UTF-8 bytes and messages to 768 UTF-8 bytes.
Only the newest message is retained, including while the display is unavailable.
Messages are temporary and do not survive a reboot.

To dismiss remotely:

```yaml
action: esphome.onx2424g013_dbbc54_dismiss_message
```

## Calendar reminder

Replace `calendar.work` with your calendar entity. This automation displays each
meeting's title ten minutes before it starts:

```yaml
alias: Meeting reminder on knob
triggers:
  - trigger: calendar
    entity_id: calendar.work
    event: start
    offset: "-00:10:00"
actions:
  - action: esphome.onx2424g013_dbbc54_show_message
    data:
      title: Meeting in 10 minutes
      message: "{{ trigger.calendar_event.summary }}"
      image_url: ""
      timeout_seconds: 60
mode: queued
```

Use a future calendar event when checking the automation; manually running its
actions does not provide `trigger.calendar_event`.

## Display priority

Messages appear over the current view, including the idle clock or idle media
player. Timer popups have priority and pause a message's timeout. Scheduled sleep,
a manually switched-off screen, and provisioning defer messages until the display
is available. The timeout starts when the message becomes visible and resumes
after a timer popup closes.

References: [ESPHome user-defined actions](https://esphome.io/components/api/#user-defined-actions),
[Home Assistant calendar automations](https://www.home-assistant.io/integrations/calendar/),
and [calendar trigger data](https://www.home-assistant.io/docs/automation/trigger-data/#calendar).

## Include an image

The single `show_message` action accepts `image_url` and `timeout_seconds`. Set the URL to
an empty string (`""`) for a text-only notification. After this update, add that
field and `timeout_seconds` to existing Home Assistant calls. The separate
`show_message_with_timeout` action has been consolidated into `show_message`. The URL must be an absolute HTTP or HTTPS
address reachable directly from the knob; browser sign-in and Home Assistant
session cookies are not forwarded.

```yaml
action: esphome.onx2424g013_dbbc54_show_message
data:
  title: Front door
  message: Someone is at the door.
  image_url: "https://example.com/door-snapshot.jpg"
  timeout_seconds: 60
```

Use a publicly readable or privately hosted image URL that the knob can access
without authentication. JPEG, PNG, and BMP are supported where the decoder can
read them; progressive JPEG is unsupported. Downloads are limited to 512 KiB,
source images to 1,048,576 pixels, and the display image to a 112 × 64 pixel box.
The image attempt, including waiting for downloader memory or other image work,
expires after eight seconds. Text appears immediately, remains scrollable, and
uses the full text-only layout if the image fails, is invalid, or exceeds a limit.

Only the newest notification is retained. A replacement, dismissal, or expiry
cancels the old download, detaches its display buffer before freeing it, and
rejects late results. Media artwork downloads wait while a notification is visible.
