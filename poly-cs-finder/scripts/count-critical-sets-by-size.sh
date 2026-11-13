#!/usr/bin/env bash

if (( $# < 1 )); then
    printf 'Usage: %s FILE [FILE...]\n' "${0##*/}" >&2
    exit 1
fi

for file in "$@"; do
    echo "File: $file"
    echo "ORDER COUNT"

    awk '
        /#/ { next }
        !/\./ { next }

        {
            s = $0
            gsub(/[^A-Za-z]/, "", s)

            n = 0
            for (i = 1; i <= length(s); i++) {
                c = substr(s, i, 1)

                if (!(c in seen)) {
                    seen[c] = 1
                    n++
                }
            }

            counts[n]++
            delete seen
        }

        END {
            for (n in counts)
                printf "%-5s %s\n", n, counts[n]
        }
    ' "$file"

    echo
done
