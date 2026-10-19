#!/usr/bin/env sh
# Builds apps into dist/<id>.xpa: scripts/build.sh [apps/<id> ...] (default: all).
# Each app is apps/<id>/ with manifest.json, C sources and any data files.
# Needs zig (https://ziglang.org, or `pip install ziglang`) and python3.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
if [ -z "$ZIG" ]; then
  if command -v zig >/dev/null 2>&1; then ZIG=zig; else ZIG="python3 -m ziglang"; fi
fi
[ $# -gt 0 ] || set -- "$ROOT"/apps/*/
mkdir -p "$ROOT/dist"
for dir in "$@"; do
  dir=${dir%/}
  id=$(basename "$dir")
  out="$ROOT/build/$id"
  rm -rf "$out" && mkdir -p "$out"
  $ZIG cc --target=wasm32-wasi -mexec-model=reactor -Os -I"$ROOT/sdk" \
    -Wl,-z,stack-size=4096 -Wl,--initial-memory=65536 -o "$out/app.wasm" "$dir"/*.c
  python3 "$ROOT/scripts/pack.py" "$dir" "$out" "$ROOT/dist/$id.xpa"
done
