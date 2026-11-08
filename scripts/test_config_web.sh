#!/usr/bin/env bash
# Run from GitHub Codespaces; pins both the browser and its test driver.
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
docker run --rm -v "$project_dir:/config" -w /config \
  -e "ONX_UI_SCREENSHOT=${ONX_UI_SCREENSHOT:-/tmp/onx-config-mobile.png}" \
  --entrypoint bash mcr.microsoft.com/playwright:v1.56.1-noble -lc \
  'npm install --prefix /tmp/onx-web-tests --no-audit --no-fund playwright@1.56.1 && NODE_PATH=/tmp/onx-web-tests/node_modules node tests/test_config_web.cjs'
