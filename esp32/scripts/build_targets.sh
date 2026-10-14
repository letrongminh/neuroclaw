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
# Each target's output goes to build-<target>.log; on a failure its tail is shown.
for target in "${targets[@]}"; do
  build="$app/build-$target"
  log="$app/build-$target.log"
  rm -rf "$build" # set-target refuses a half-cleaned directory; ccache keeps this cheap
  if ! { idf.py -C "$app" -B "$build" -D SDKCONFIG="$build/sdkconfig" set-target "$target" &&
    idf.py -C "$app" -B "$build" -D SDKCONFIG="$build/sdkconfig" build; } >"$log" 2>&1; then
    echo "$target: build failed; the last lines of $log:" >&2
    tail -n 60 "$log" >&2
    exit 1
  fi
  echo "$target: $(wc -c <"$build/neuroclaw-wifi.bin" | tr -d ' ') bytes (log: $log)"
done
