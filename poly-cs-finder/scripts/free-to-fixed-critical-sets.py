#!/usr/bin/env python3
"""
free-to-fixed-critical-sets.py - expand critical sets of free covers to fixed covers.

Usage:
    python3 free-to-fixed-critical-sets.py [-o OUT.cs] CRITICAL.cs [CRITICAL.cs ...]

Definitions
    A .cs file lists one representative cover for each FREE cover (covers
    equal up to rotation and reflection), followed by that cover's critical
    sets. Critical sets are defined with respect to the universe of FIXED
    covers (Havas et al.), and the 8 symmetries of the square (the dihedral
    group D4) map covers to covers and critical sets to critical sets. So
    applying each symmetry to a cover and its critical sets gives the fixed
    covers and their critical sets.

    Orbit: the distinct images of a cover under the 8 symmetries. Its size is
    8 / |stabiliser|: 1, 2, 4 or 8.

    Additional critical sets: fixed critical sets minus the ones listed in
    the .cs file (the listed ones belong to the representative covers).

Output
    A summary per file: free and fixed covers, listed and fixed critical
    sets, the additional ones, a breakdown by critical-set size, and the
    orbit-size distribution. With -o, also writes the full fixed .cs file in
    the same block format (cover line, then its critical sets, blank line),
    so it can be fed to the other scripts. Tile letters are relabelled by
    first appearance in reading order, consistently for a cover and its sets.

Consistency check
    If a cover is symmetric, the images of its critical sets under its
    stabiliser must already be in its own list. Violations are reported and
    give exit status 2.

Requires only the Python 3 standard library.
"""

import argparse
import signal
import sys
from collections import Counter


def die(msg):
    sys.stderr.write("free-to-fixed-critical-sets.py: " + msg + "\n")
    sys.exit(1)


# --------------------------------------------------------------------- parsing

def grid_rows(line):
    rows = line.split()
    n = len(rows)
    if n == 0 or any(len(r) != n for r in rows):
        die("not an n x n grid: %r" % line)
    return rows


def tiles_of(rows):
    """Map each tile label to a bitmask of its cells (cell = r*n + c)."""
    n = len(rows)
    tiles = {}
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch != ".":
                tiles[ch] = tiles.get(ch, 0) | (1 << (r * n + c))
    return tiles


def read_blocks(path):
    """Yield (cover_line, [cs_line, ...]) for each blank-line separated block."""
    cur = []
    with open(path) as f:
        for line in f:
            s = line.strip()
            if s.startswith("#"):
                continue
            if not s:
                if cur:
                    yield cur[0], cur[1:]
                    cur = []
                continue
            cur.append(s)
    if cur:
        yield cur[0], cur[1:]


# ------------------------------------------------------------------ symmetries

def symmetries(n):
    """The 8 symmetries of the n x n square as cell permutations:
    perm[cell] = image cell."""
    def cell(r, c):
        return r * n + c
    maps = [
        lambda r, c: (r, c),                    # identity
        lambda r, c: (c, n - 1 - r),            # rotate 90
        lambda r, c: (n - 1 - r, n - 1 - c),    # rotate 180
        lambda r, c: (n - 1 - c, r),            # rotate 270
        lambda r, c: (r, n - 1 - c),            # reflect left-right
        lambda r, c: (n - 1 - r, c),            # reflect top-bottom
        lambda r, c: (c, r),                    # reflect main diagonal
        lambda r, c: (n - 1 - c, n - 1 - r),    # reflect anti-diagonal
    ]
    return [[cell(*f(r, c)) for r in range(n) for c in range(n)] for f in maps]


class Mapper:
    """Applies a cell permutation to tile bitmasks, with a cache."""

    def __init__(self, perm):
        self.perm = perm
        self.cache = {}

    def __call__(self, mask):
        out = self.cache.get(mask)
        if out is None:
            out = 0
            m = mask
            while m:
                low = m & -m
                out |= 1 << self.perm[low.bit_length() - 1]
                m ^= low
            self.cache[mask] = out
        return out


def to_line(n, cover, tiles):
    """Grid string for `tiles` (a subset of `cover`), lettered by first
    appearance of each cover tile in reading order."""
    order = sorted(cover, key=lambda m: (m & -m).bit_length())
    label = {m: chr(ord("A") + i) for i, m in enumerate(order)}
    cells = ["."] * (n * n)
    for m in tiles:
        ch = label[m]
        while m:
            low = m & -m
            cells[low.bit_length() - 1] = ch
            m ^= low
    s = "".join(cells)
    return " ".join(s[i:i + n] for i in range(0, n * n, n))


# ------------------------------------------------------------------------ main

def process(path, out):
    blocks = list(read_blocks(path))
    if not blocks:
        die("%s: no covers" % path)
    n = len(grid_rows(blocks[0][0]))
    maps = [Mapper(p) for p in symmetries(n)]

    n_free = n_fixed = n_listed = n_cs_fixed = 0
    listed_by_size = Counter()
    fixed_by_size = Counter()
    orbit_sizes = Counter()
    violations = []
    seen_covers = set()
    duplicates = 0

    for ci, (cover_line, cs_lines) in enumerate(blocks, 1):
        n_free += 1
        cover = frozenset(tiles_of(grid_rows(cover_line)).values())
        sets = []
        for line in cs_lines:
            cs = frozenset(tiles_of(grid_rows(line)).values())
            if not cs <= cover:
                die("%s: cover %d: critical set %r is not part of the cover"
                    % (path, ci, line))
            sets.append(cs)
        set_of_sets = set(sets)
        n_listed += len(sets)
        for cs in sets:
            listed_by_size[len(cs)] += 1

        images = {}           # image cover -> its critical sets
        for g in maps:
            img = frozenset(g(m) for m in cover)
            mapped = {frozenset(g(m) for m in cs) for cs in sets}
            if img in images:
                if images[img] != mapped:
                    violations.append(ci)
                    images[img] |= mapped
            else:
                images[img] = mapped

        orbit_sizes[len(images)] += 1
        for img, img_sets in images.items():
            if img in seen_covers:
                duplicates += 1
            seen_covers.add(img)
            n_fixed += 1
            n_cs_fixed += len(img_sets)
            for cs in img_sets:
                fixed_by_size[len(cs)] += 1
            if out:
                out.write(to_line(n, img, img) + "\n")
                for cs in sorted(img_sets, key=lambda s: (len(s), sorted(s))):
                    out.write(to_line(n, img, cs) + "\n")
                out.write("\n")
        # stabiliser closure check
        if images.get(cover, set_of_sets) != set_of_sets and ci not in violations:
            violations.append(ci)

    w = sys.stdout.write
    w("== %s: n=%d\n" % (path, n))
    w("   covers:         %9d free  -> %9d fixed\n" % (n_free, n_fixed))
    w("   critical sets:  %9d listed -> %9d fixed  (%d additional)\n"
      % (n_listed, n_cs_fixed, n_cs_fixed - n_listed))
    w("   by size:        size: listed -> fixed\n")
    for s in sorted(fixed_by_size):
        w("                   %4d: %9d -> %9d\n"
          % (s, listed_by_size[s], fixed_by_size[s]))
    w("   orbit sizes:    %s\n" % "  ".join(
        "%d: %d" % (k, orbit_sizes[k]) for k in sorted(orbit_sizes)))
    if duplicates:
        w("   WARNING: %d fixed covers produced more than once "
          "(two listed covers are equivalent)\n" % duplicates)
    if violations:
        w("   STABILISER VIOLATIONS in covers: %s\n"
          % ", ".join(map(str, violations[:20])))
    return not violations and not duplicates


def main():
    if hasattr(signal, "SIGPIPE"):
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    ap = argparse.ArgumentParser(
        description="Expand critical sets of free covers to fixed covers.")
    ap.add_argument("csfiles", nargs="+", metavar="CS",
                    help="critical set file(s) (.cs)")
    ap.add_argument("-o", "--output", metavar="FILE",
                    help="write the fixed covers and critical sets to FILE "
                         "(only valid with a single input file)")
    args = ap.parse_args()
    if args.output and len(args.csfiles) != 1:
        die("-o needs exactly one input file")

    ok = True
    out = open(args.output, "w") if args.output else None
    try:
        for path in args.csfiles:
            ok &= process(path, out)
    finally:
        if out:
            out.close()
    sys.exit(0 if ok else 2)


if __name__ == "__main__":
    main()
