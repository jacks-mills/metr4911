#!/usr/bin/env bash
# usage
if [ "$#" -ne 2 ]; then
    echo "usage: generate-critical-sets.sh CRITICAL_SETS_DIR TIMES_DIR" >&2
    exit 1
fi
CRITICAL_SETS_DIR="$1"
TIMES_DIR="$2"

COVERS_DIR="/home/jack/dev/c/metr4911/poly-cs-finder/covers/"
UNIVERSE_DIR="/home/jack/dev/c/metr4911/poly-cs-finder/universes/"

mkdir -p "$CRITICAL_SETS_DIR" "$TIMES_DIR"

run_loop() {
    if [ "$#" -ne 2 ]; then
        return 1
    fi
    local COVERS="$1"
    local UNIVERSE="$2"
    while IFS= read -r COVER; do
        echo "$COVER"
        /home/jack/dev/c/metr4911/poly-cs-finder/bin/pcritical "$COVER" "$UNIVERSE"
        echo ""
    done < "$COVERS"
}

generate_covers() {
    if [ "$#" -ne 4 ]; then
        return 1
    fi
    local COVERS="$1"
    local UNIVERSE="$2"
    local TIME="$3"
    local CRITICAL_SET="$4"
    {
        echo "# Generated on $(date)"
        echo ""
        echo "# covers file:   $(realpath "$COVERS")"
        echo "# universe file: $(realpath "$UNIVERSE")"
        echo ""
        echo ""
    } | tee "$TIME" > "$CRITICAL_SET"
    { time run_loop "$COVERS" "$UNIVERSE" >> "$CRITICAL_SET"; } 2>> "$TIME"
}

for n in 1 2 3 4 5 6; do
    echo "Generating covers for n=$n"
    generate_covers \
        "$COVERS_DIR/n${n}.cvr" \
        "$UNIVERSE_DIR/n${n}.uni" \
        "$TIMES_DIR/n${n}.time" \
        "$CRITICAL_SETS_DIR/n${n}.cs" \
        || echo "  -> failed for n=$n" >&2
done

echo "Done"
