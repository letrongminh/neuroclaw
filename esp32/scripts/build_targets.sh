#!/usr/bin/env bash
# Build NeuroClaw-ESP for every chip it supports: the Wi-Fi example for each
# target (ESP-IDF v5.4 environment, as in espressif/idf:v5.4), then the app's
# image size per target. QEMU runs only the ESP32-S3; this checks that the
# component compiles and links for the others.
#
#   esp32/scripts/build_targets.sh [target...]   # default: esp32 esp32s3 esp32c3 esp32c6
set -euo pipefail

app=$(cd "$(dirname "$0")/../examples/wifi" && pwd)
targets=("$@")
[ ${#targets[@]} -gt 0 ] || targets=(esp32 esp32s3 esp32c3 esp32c6)
for target in "${targets[@]}"; do
  build="$app/build-$target"
  idf.py -C "$app" -B "$build" -D SDKCONFIG="$build/sdkconfig" set-target "$target" >/dev/null
  idf.py -C "$app" -B "$build" -D SDKCONFIG="$build/sdkconfig" build >/dev/null
  echo "$target: $(wc -c <"$build/neuroclaw-wifi.bin" | tr -d ' ') bytes"
done
