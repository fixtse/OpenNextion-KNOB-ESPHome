This component retains the ESPHome 2026.10.2 online_image Python schema and C++
public API. ESPHome licenses C++/runtime code under GPLv3 and Python/other parts under MIT.
See the retained LICENSE and https://github.com/esphome/esphome/blob/2026.10.2/LICENSE

The local C++ implementation runs artwork network requests in a worker task.
It bounds responses to 512 KB in PSRAM, limits redirects and request duration,
checks cancellation generations, and hands results to the UI only after HTTP
cleanup. RuntimeImage decoding and all UI work stay on the ESPHome main task.
Images and compressed buffers are RAM-only. The Python schema's buffer_size is
accepted for compatibility; the network worker sizes its bounded PSRAM buffer
from the response length instead. HTTP conditional-cache headers are unused;
the knob retains its completed artwork until the supplied URL changes.

Grayscale JPEGs use an explicit RGB565 callback. JPEGDEC 1.8.4's grayscale
writer emits 16-bit pixels even when RGB8888 is requested; reading those as
32-bit pixels corrupts the artwork. The callback respects the block row pitch
and clipped width. Color JPEGs retain ESPHome's normal decoder path.
