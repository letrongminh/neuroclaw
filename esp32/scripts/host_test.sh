#!/usr/bin/env bash
# Build and run NeuroClaw-ESP's host tests (nc_core.c; nc_workspace.c and nc_cron.c
# on a temporary directory) with AddressSanitizer and
# UBSan. cJSON v1.7.18 — the version ESP-IDF v5.4 ships — is fetched once into
# esp32/.cache/ and checked against its SHA-256 (the same bytes as
# $IDF_PATH/components/json/cJSON/ in espressif/idf:v5.4).
#
#   esp32/scripts/host_test.sh
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
component="$here/components/neuroclaw"
cache="$here/.cache/cjson-1.7.18"
mkdir -p "$cache"
fetch() { # <file> <sha256>
  if [ ! -f "$cache/$1" ]; then
    curl -fsSL "https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.18/$1" -o "$cache/$1.tmp"
    mv "$cache/$1.tmp" "$cache/$1"
  fi
  echo "$2  $cache/$1" | shasum -a 256 -c - >/dev/null
}
fetch cJSON.c 75c51de8fa40ac9d7a99319c6330719bd692eb81c0a869265f3d4c682533f9b9
fetch cJSON.h 0578cc29132912edbc88f83207a8fc76e5db3db0605497e909a9384ef3cc474b

cc=${CC:-cc}
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
san=(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
# cJSON is upstream's: built as is, without our warning flags.
"$cc" -std=c99 -O1 -g -w "${san[@]}" -c "$cache/cJSON.c" -o "$out/cJSON.o"
"$cc" -std=c99 -Wall -Wextra -Wpedantic -Werror -O1 -g "${san[@]}" \
  -I "$component/include" -I "$cache" \
  "$component/src/nc_core.c" "$component/test/test_core_host.c" "$out/cJSON.o" \
  -o "$out/test_core_host"
"$out/test_core_host"

"$cc" -std=c99 -Wall -Wextra -Wpedantic -Werror -O1 -g "${san[@]}" \
  -I "$component/include" -I "$cache" \
  "$component/src/nc_workspace.c" "$component/src/nc_cron.c" \
  "$component/test/test_memory_host.c" "$out/cJSON.o" \
  -o "$out/test_memory_host"
mkdir "$out/ws"
"$out/test_memory_host" "$out/ws"

"$cc" -std=c99 -Wall -Wextra -Wpedantic -Werror -O1 -g "${san[@]}" \
  -I "$component/include" -I "$cache" \
  "$component/src/nc_web.c" "$component/test/test_web_host.c" "$out/cJSON.o" \
  -o "$out/test_web_host"
"$out/test_web_host"
