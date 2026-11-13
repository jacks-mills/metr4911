#!/usr/bin/env python3
"""
show-trades.py - draw covers and their minimal trades in the terminal.

Usage:
    python3 scripts/show-trades.py [-s SIZES] [-l LIMIT] [--plain] CRITICAL.cs

For each cover with a minimal trade of one of the requested sizes, prints the
cover, then the cover again showing only that trade's tiles. Cells are drawn as
coloured blocks whose text is the tile letter in the same colour, so copying
the output gives plain letters ("A A A B ...").

Minimal trades are worked out from the critical sets alone: they are the
minimal sets of tiles that share a tile with every critical set of the cover.

Options:
    -s, --sizes SIZES   comma-separated trade sizes to show (default 3,4)
    -l, --limit N       examples to show per size (default 2)
    --plain             no colour, letters only

Examples:
    python3 scripts/show-trades.py critical-sets/n6.cs
    python3 scripts/show-trades.py -s 2 -l 5 critical-sets/n5.cs
"""

import argparse
import signal
import sys
from itertools import combinations

# 256-colour codes for tiles A-I (same order as the poster's tab colours).
COLOURS = {"A": 203, "B": 31, "C": 35, "D": 205, "E": 39,
           "F": 227, "G": 215, "H": 249, "I": 37}


def blocks(path):
    """Yield (cover, [critical sets]) for each blank-line separated block."""
    cur = []
    with open(path) as f:
        for line in f:
            s = line.strip()
            if s.startswith("#"):
                continue
            if not s:
                if len(cur) > 1:
                    yield cur[0], cur[1:]
                cur = []
                continue
            cur.append(s)
    if len(cur) > 1:
        yield cur[0], cur[1:]


def minimal_trades(cover, sets):
    """Minimal tile sets that share a tile with every critical set."""
    tiles = sorted({c for c in cover if c.isalpha()})
    crit = [{c for c in s if c.isalpha()} for s in sets]
    found = []
    for k in range(1, len(tiles) + 1):
        for t in combinations(tiles, k):
            t = set(t)
            if all(t & s for s in crit) and not any(f <= t for f in found):
                found.append(t)
    return found


def draw(grid, plain):
    """Print a cover string such as 'AAB ACB CCB', one row per line."""
    for row in grid.split():
        out = []
        for c in row:
            if c == "." or plain:
                out.append(c + " ")
            else:
                col = COLOURS.get(c, 250)
                out.append("\033[48;5;%dm\033[38;5;%dm%s \033[0m" % (col, col, c))
        print("".join(out))


def main():
    if hasattr(signal, "SIGPIPE"):
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    ap = argparse.ArgumentParser(
        description="Draw covers and their minimal trades in the terminal.")
    ap.add_argument("csfile", help="critical set file (.cs)")
    ap.add_argument("-s", "--sizes", default="3,4",
                    help="comma-separated trade sizes (default 3,4)")
    ap.add_argument("-l", "--limit", type=int, default=2,
                    help="examples per size (default 2)")
    ap.add_argument("--plain", action="store_true", help="no colour")
    args = ap.parse_args()

    sizes = {int(s) for s in args.sizes.split(",")}
    shown = {s: 0 for s in sizes}
    for cover, sets in blocks(args.csfile):
        if all(n >= args.limit for n in shown.values()):
            break
        trades = minimal_trades(cover, sets)
        for t in trades:
            if len(t) not in sizes or shown[len(t)] >= args.limit:
                continue
            shown[len(t)] += 1
            print("%s   minimal trade: %s   (all: %s)" % (
                cover, "".join(sorted(t)),
                ",".join("".join(sorted(x)) for x in trades)))
            draw(cover, args.plain)
            draw("".join(c if c in t or c == " " else "." for c in cover),
                 args.plain)
            print()
    for s in sorted(sizes):
        if shown[s] < args.limit:
            print("found %d example(s) of %d-tile trades" % (shown[s], s))


if __name__ == "__main__":
    main()
