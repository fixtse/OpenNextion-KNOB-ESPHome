#!/usr/bin/env bash
# Run in the repository's GitHub Codespace; retain failures while checking every group.
set -uo pipefail
cd "$(dirname "$0")/.."
output_dir="$(mktemp -d /tmp/onx-controls-tests.XXXXXX)"
trap 'rm -rf "$output_dir"' EXIT
failed=0
check() {
  if "$@"; then return 0; else
    local code=$?
    printf 'FAILED (%s): %s\n' "$code" "$*" >&2
    failed=1
    return 1
  fi
}
for name in control_state media_state icon_state icon_raster timer_state config_rules; do
  if check c++ -std=c++17 "tests/test_${name}.cpp" -o "$output_dir/$name"; then
    check "$output_dir/$name"
  fi
done
if check c++ -std=c++17 -Itests/icon_stubs -I. tests/test_icon_downloader.cpp -o "$output_dir/icon_downloader"; then
  check "$output_dir/icon_downloader"
fi
if check c++ -std=c++17 -pthread tests/test_registry_queue.cpp -o "$output_dir/registry"; then
  check "$output_dir/registry"
fi
if check c++ -std=c++17 -Itests/wifi_stubs tests/test_wifi_lifecycle.cpp \
  my_components/onx_mode_manager/onx_mode_manager.cpp \
  my_components/onx_app_state/onx_app_state.cpp -o "$output_dir/wifi"; then
  check "$output_dir/wifi"
fi
if check c++ -std=c++20 -pthread -Itests/http_stubs tests/test_config_http.cpp \
  my_components/onx_config_web/onx_config_web.cpp -o "$output_dir/http"; then
  check "$output_dir/http"
fi
check python3 -m unittest discover -s tests
check node tests/test_browser_catalogue.cjs
check bash scripts/test_config_web.sh
exit "$failed"
