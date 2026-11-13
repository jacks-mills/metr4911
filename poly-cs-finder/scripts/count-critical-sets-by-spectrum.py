#!/usr/bin/env python3
"""Group the covers in a critical-sets file by their critical-set spectrum.

Input format: '#' comment lines are ignored; paragraphs are separated by
blank lines. The first line of a paragraph is the cover, and every following
line is one critical set. The order of a critical set is the number of
distinct polyominoes (letters) it contains. The spectrum of a cover is the
sorted set of orders of its critical sets.

Usage: count-critical-sets-by-spectrum.py [-c] [FILE ...]   (reads stdin if no FILE is given)
"""
import argparse
import sys
from collections import defaultdict


def order(line):
    return len({c for c in line if c.isalpha()})


def paragraphs(lines):
    para = []
    for line in lines:
        line = line.strip()
        if line.startswith("#"):
            continue
        if not line:
            if para:
                yield para
            para = []
        else:
            para.append(line)
    if para:
        yield para


def group_by_spectrum(lines):
    by_spectrum = defaultdict(list)
    for cover, *crit_sets in paragraphs(lines):
        spectrum = tuple(sorted({order(cs) for cs in crit_sets}))
        by_spectrum[spectrum].append(cover)
    return by_spectrum


def fmt_spectrum(spectrum):
    return f"({', '.join(map(str, spectrum))})"


def main():
    parser = argparse.ArgumentParser(
        description="Group covers by their critical-set spectrum.")
    parser.add_argument("files", nargs="*", type=argparse.FileType(),
                        default=[sys.stdin],
                        help="critical-sets files (default: stdin)")
    parser.add_argument("-c", "--covers", action="store_true",
                        help="also print the covers under each spectrum")
    args = parser.parse_args()

    results = []
    for f in args.files:
        with f:
            results.append((f.name, group_by_spectrum(f)))

    # Column widths shared across all files so output lines up.
    all_groups = [g for _, groups in results for g in groups.items()]
    spec_w = max((len(fmt_spectrum(s)) for s, _ in all_groups), default=0)
    count_w = max((len(str(len(c))) for _, c in all_groups), default=0)

    blocks = []
    for name, groups in results:
        lines = [f"{name}:"] if len(results) > 1 else []
        for i, spectrum in enumerate(sorted(groups)):
            covers = groups[spectrum]
            noun = "cover" if len(covers) == 1 else "covers"
            if args.covers and i > 0:
                lines.append("")
            lines.append(f"{fmt_spectrum(spectrum):<{spec_w}}  "
                         f"{len(covers):>{count_w}} {noun}")
            if args.covers:
                lines.extend(f"    {cover}" for cover in covers)
        blocks.append("\n".join(lines))

    print("\n\n".join(blocks))


if __name__ == "__main__":
    main()
