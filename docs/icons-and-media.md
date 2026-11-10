# Icons, album art and playback progress

[All guides](../README.md#find-your-guide) · [Playlists](web-configuration.md#playlists-and-stations)

The knob uses the icons you chose in Home Assistant and shows album art when
your media player provides it. Around the player, a colored ring follows the
track's progress. Live radio or tracks without a known duration do not show a
made-up progress ring.

## Change the look

- Change an entity's icon in Home Assistant, then open the knob webpage's **Home Assistant** section and click **Refresh entity list** to sync saved controls.
- Pick the progress-ring color in **Device settings → Media progress ring**, then **Save device settings**. Paused playback uses the same color at lower brightness.
- If an icon cannot load, a built-in icon keeps the control recognizable. New downloaded icons need internet access.
- If artwork is missing, check whether that player provides artwork in Home Assistant. **Download diagnostics** can help someone investigate a persistent problem.

[Display settings](web-configuration.md#display-and-idle-screen) · [Media controls](knob-controls.md#music-and-playlists)

## Technical details

The knob displays the MDI icon chosen for each Home Assistant entity. On the
configuration page, refresh the entity list and save the affected slots to copy
registry icon overrides. Native API `icon` attribute changes are also followed
when there is no saved registry override. Icon metadata is included in configuration
exports; older backups without icon fields still import.

Icons download over verified HTTPS from `api.iconify.design` and are rasterized on
the knob in a background task. This needs internet access for an uncached icon.
No Home Assistant token or entity ID is sent to the icon service. Only the MDI name
is requested. Downloads have byte and time limits, with no arbitrary URL support.

A bounded RAM cache avoids repeat downloads while browsing. Cached images remain
available during network outages; after restarting the knob, icons download again.
Failed downloads retry after five minutes. Built-in icons for lights, fans,
climate, media, covers, sensors, helpers, scenes, buttons, scripts, automations,
weather, timers, and switches remain available offline. Unsupported icon namespaces
also use the entity-type fallback. Media album artwork takes priority on the
primary media view. It loads directly from HA's supplied artwork URL, including
its artwork access parameter where provided; no device-stored bearer token is used.

The media detail page has a full circular progress ring at the screen edge. It
uses Home Assistant's `media_position`, `media_position_updated_at`, and
`media_duration`, with smooth local updates every 100ms while playing. Pausing
freezes the ring and dims the selected color; seek feedback and track changes reset
the progress anchor. Idle, unavailable, disconnected, and unknown-duration/live
streams hide the ring instead of displaying invented progress. This indicator
shows progress and does not seek when operated.

# Recovering entity state

Missing and unavailable entities are retried automatically, starting after ten
seconds and backing off to five minutes. Recovery reads refresh the existing
Native API callbacks without adding subscriptions or replaying commands.

For an immediate retry, short-press the knob while an unavailable entity's detail
page is open. You can also open **Home Assistant** on the configuration webpage
and select **Retry knob entity states**. The webpage action renews the Native API
connection so Home Assistant restores all subscriptions. Neither method restarts
the device or changes Wi-Fi credentials. An entity that still does not exist in
Home Assistant remains unavailable until it is created.

Upgrading from older firmware removes its token entry, token-status sensor,
“How to use” sensor, and QR-code menus, and deletes the old saved device token.
Home Assistant may retain old unavailable entity-registry entries after removal
from firmware; those entries can be deleted from the device's entity list.

## Performance and reset diagnostics

Icon bindings reuse unchanged LVGL descriptors and tint styles. After two seconds
without knob input, configured icons prefetch one at a time into the existing
12-entry RAM cache. Icons still download again after reboot: no flash partition
or NVS space is consumed for bitmap storage.

Artwork remains provider-independent: Home Assistant's supplied URL is fetched,
and the final display buffer is resized to fit 100x100. This does not shrink the
original network file; JPEG requires the complete compressed download and still
has decode cost for the source dimensions. Artwork is postponed until its URL
has settled, while icons are downloading, and when internal memory is low.
Artwork uses a 2.5-second HTTP timeout and a 15-second overall deadline; failed
URLs back off for a minute. Compressed input stays in a bounded PSRAM buffer.
Existing OTA request timeouts are kept.

Download completion refreshes on the next interval, after downloader cleanup.
Unavailable players cancel in-progress artwork and discard stale progress.
Unchanged media/playlist labels and image sources avoid repeated redraws.

The configuration webpage and diagnostic download include readable reset reason,
free/minimum/largest internal heap, free PSRAM, and artwork status. An RTC RAM
breadcrumb can preserve the last media action or artwork event across a crash or
software reset. It is the last observed event, not a crash backtrace or proof of
cause; power-on and brownout discard it. No continuous log or flash writes are
required. Hardware crash reproduction remains unverified without a reset log.

Artwork DNS, TCP/TLS, redirects, and body reads now run in a separate task. Slow
servers no longer block knob input during network operations. Downloads are
limited to 512 KB of PSRAM, three redirects, and a 15-second deadline, with
2.5-second socket timeouts. Cancelled requests cannot display stale artwork.
JPEG/PNG/BMP content is identified from its bytes. Final decoding remains on the
main task, so very large source dimensions can still cause a shorter decode
pause; the network limit does not promise a dimension-independent decode time.

HTTP diagnostics also report the most recent Wi-Fi RSSI (sampled every 60 seconds),
Wi-Fi disconnect count since boot, and the last artwork download/decode error.
A connected Wi-Fi flag alone is not a measurement of signal quality or internet
reachability. Artwork error messages omit URLs and access parameters.

The network memory gate requires 48 KB total internal free RAM and a 20 KB
largest block, allowing the observed 28 KB block to start a worker. A low-memory
deferral is reported as `waiting_for_memory` for icons and artwork. These are
allocation headroom checks, not a guarantee against every memory failure.

TLS allocation failures (`mbedtls_ssl_setup -0x7F00`) were observed during the
GitHub OTA manifest check on live firmware `.25`. The ESP-IDF build now allocates
mbedTLS memory from PSRAM. This covers icon/artwork HTTPS and OTA checks/downloads;
UI/worker stacks and DMA retain their normal internal-memory placement.

Artwork uses its own memory gate: 32 KB total internal free RAM and a 14 KB
contiguous block for its 12 KB worker stack. The icon worker retains its separate
48 KB/20 KB check. Sharing the icon gate incorrectly deferred artwork on a live
unit with a 16 KB block. JPEG decoder objects (including MCU/Huffman workspaces)
now use checked PSRAM allocation rather than the fragmented internal heap.
