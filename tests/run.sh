#!/usr/bin/env sh
# Host tests: each tests/test_<app>.c includes apps/<app>/main.c and drives it
# through tests/stub.c. Needs a C compiler (cc). Exits non-zero on any failure.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=$ROOT/build/tests
mkdir -p "$OUT"
fails=0
for t in "$ROOT"/tests/test_*.c; do
  name=$(basename "$t" .c)
  ${CC:-cc} -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -g \
    -I"$ROOT/sdk" -o "$OUT/$name" "$t" "$ROOT/tests/stub.c"
  if (cd "$ROOT" && "$OUT/$name"); then echo "PASS $name"; else echo "FAIL $name"; fails=$((fails + 1)); fi
done
[ "$fails" -eq 0 ]
