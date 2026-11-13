#!/usr/bin/env bash

# usage
if [ "$#" -ne 2 ]; then
    echo "usage: verify-critical-sets.sh COVERS UNIVERSE" >&2
    exit 1
fi

COVERS="$1"
UNIVERSE="$2"

RED="\e[31m"
GREEN="\e[32m"
RESET="\e[0m"

LINE_NUMBER=1
NUM_NON_CRITICAL_SETS=0
while IFS= read -r COVER; do
    /home/jack/dev/c/metr4911/poly-cs-finder/bin/pchecker "$COVER" "$UNIVERSE"
    rc=$?
    case "$rc" in
        0)
            ;;
        1)
            ((NUM_NON_CRITICAL_SETS++))
            echo -e "${RED}NOT CRITICAL${RESET} (${LINE_NUMBER}): \"${COVER}\" is not critical" ;;

        *)
            echo -e "${RED}ERROR${RESET} (${LINE_NUMBER}): error processing cover \"${COVER}\"" ;;
    esac
    
    if [ 0 -eq $(($LINE_NUMBER % 100)) ]; then
        echo "Line number: $LINE_NUMBER"
    fi

    ((LINE_NUMBER++))
done < "$COVERS"

echo "Non-critical sets found: ${NUM_NON_CRITICAL_SETS}"
