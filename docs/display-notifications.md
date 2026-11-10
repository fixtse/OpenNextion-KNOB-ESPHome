# Home Assistant display notifications

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
```

The message appears for 60 seconds. Press the knob to dismiss it, or turn it to
scroll longer text. Dismissing and scrolling do not operate the page underneath.
The underlying page is retained.

For a custom timeout, use `show_message_with_timeout`:

```yaml
action: esphome.onx2424g013_dbbc54_show_message_with_timeout
data:
  title: Next meeting
  message: Team catch-up in ten minutes. Bring the project notes.
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
