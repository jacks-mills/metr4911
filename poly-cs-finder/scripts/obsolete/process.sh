#!/usr/bin/env bash

if [ $# -ne 2 ]; then
    echo "usage: process.sh IN OUT" >&2
    exit 1
fi

IN="$1"
OUT="$2"

LC_ALL=C grep '^ [ABCDEFG]' "$IN" \
  | LC_ALL=C tr -d ' ' \
  | LC_ALL=C paste -d' ' - - - - - - - \
  > "$OUT"
