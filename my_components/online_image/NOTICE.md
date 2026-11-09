This component retains the ESPHome 2026.10.2 online_image Python schema and C++
public API. ESPHome is MIT licensed: https://github.com/esphome/esphome/tree/2026.10.2

The local C++ implementation runs artwork network requests in a worker task.
It bounds responses to 512 KB in PSRAM, limits redirects and request duration,
checks cancellation generations, and hands results to the UI only after HTTP
cleanup. RuntimeImage decoding and all UI work stay on the ESPHome main task.
Images and compressed buffers are RAM-only. The Python schema's buffer_size is
accepted for compatibility; the network worker sizes its bounded PSRAM buffer
from the response length instead. HTTP conditional-cache headers are unused;
the knob retains its completed artwork until the supplied URL changes.
