#!/usr/bin/env python3
"""
generate-trades.py - derive the minimal trades of each cover from its critical sets.

Usage:
    python3 generate-trades.py [-v] [--top2 K] [--top3 K] CRITICAL.cs [CRITICAL.cs ...]

Definitions
    Trade: a set T of tiles of cover C whose combined region can be re-tiled
    differently while every other tile stays put. Equivalently T = C \\ C'
    for some other cover C' in the universe. A minimal trade contains no
    smaller trade.

    A set D of tiles defines C exactly when D meets (shares a tile with)
    every trade, so the critical sets of C are the minimal sets meeting every
    trade. The relation also runs backwards (blocker duality): the minimal
    trades of C are the minimal sets of tiles meeting every critical set.
    For a set T of tiles of C:
        T meets every critical set
        <=> the tiles outside T contain no critical set
        <=> the tiles outside T do not define C
        <=> some other cover contains every tile outside T
        <=> T contains a trade.
    So this script computes, for each cover, the minimal sets meeting all of
    its critical sets. No universe is needed; only the .cs file is read.

    Packing number: the largest number of pairwise disjoint minimal trades.
    Every critical set meets each of them in a different tile, so the packing
    number is a lower bound on the size of every critical set.

Output (per file):
    - distribution of minimal-trade sizes and of trades per cover
    - catalogue of the regions formed by 2-tile trades (and the most common
      3-tile ones), up to translation, rotation and reflection
    - packing number vs smallest critical set
    - checks of the theorems:
        (T1) every cover with n >= 2 has at least 2 critical sets
        (T2) no minimal trade has a single tile
        (T3) {t} is a critical set  <=>  t is in every minimal trade
        (T4) C \\ {t} is a critical set <=> {t, s} is a trade for every s != t
    -v also lists each cover with its trades, e.g.
        cover 12  AAAA BBBC DBCC DDDC  trades={AB,BC,BD}

Exit status: 0 ok, 1 input error, 2 a theorem check failed.
Requires only the Python 3 standard library.
"""

import argparse
import signal
import sys
from collections import Counter


def die(msg):
    sys.stderr.write("generate-trades.py: " + msg + "\n")
    sys.exit(1)


# --------------------------------------------------------------------- parsing

def grid_rows(line):
    rows = line.split()
    n = len(rows)
    if n == 0 or any(len(r) != n for r in rows):
        die("not an n x n grid: %r" % line)
    return rows


def tile_cells(rows):
    """Map each tile letter to its set of (row, col) cells."""
    cells = {}
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch != ".":
                cells.setdefault(ch, set()).add((r, c))
    return cells


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


# -------------------------------------------------------------------- analysis

def popcount(x):
    return bin(x).count("1")


def minimal_transversals(sets, k):
    """Minimal subsets of {0..k-1} (bitmasks) meeting every set in `sets`."""
    out = []
    for m in sorted(range(1 << k), key=popcount):
        if all(m & s for s in sets) and not any(t & m == t for t in out):
            out.append(m)
    return out


def packing_number(trades):
    """Largest number of pairwise disjoint sets among `trades` (bitmasks)."""
    best = 0

    def go(i, used, count):
        nonlocal best
        if count + (len(trades) - i) <= best:
            return
        if i == len(trades):
            best = max(best, count)
            return
        if not trades[i] & used:
            go(i + 1, used | trades[i], count + 1)
        go(i + 1, used, count)

    go(0, 0, 0)
    return best


def canonical_shape(cells):
    """Canonical form of a cell set up to translation, rotation, reflection."""
    forms = []
    pts = list(cells)
    for _ in range(2):
        for _ in range(4):
            pts = [(c, -r) for r, c in pts]                  # rotate 90
            r0 = min(r for r, _ in pts)
            c0 = min(c for _, c in pts)
            forms.append(tuple(sorted((r - r0, c - c0) for r, c in pts)))
        pts = [(r, -c) for r, c in pts]                      # reflect
    return min(forms)


def shape_grid(shape, indent="    "):
    h = max(r for r, _ in shape) + 1
    w = max(c for _, c in shape) + 1
    s = set(shape)
    return "\n".join(indent + "".join("#" if (r, c) in s else "." for c in range(w))
                     for r in range(h))


def fmt_dist(counter):
    return "  ".join("%s: %d" % (k, counter[k]) for k in sorted(counter))


def process(path, args):
    ok = True
    w = sys.stdout.write
    n = None
    n_covers = 0
    trade_sizes = Counter()
    trades_per_cover = Counter()
    shapes2 = Counter()
    shapes3 = Counter()
    pack_vs_min = Counter()
    t1 = t2 = t3 = t4 = 0          # violations
    n_size1 = n_sizen1 = 0         # size-1 / size-(n-1) critical sets seen

    for ci, (cover_line, cs_lines) in enumerate(read_blocks(path), 1):
        rows = grid_rows(cover_line)
        if n is None:
            n = len(rows)
        n_covers += 1
        cells = tile_cells(rows)
        letters = sorted(cells)
        k = len(letters)
        bit = {ch: 1 << i for i, ch in enumerate(letters)}
        full = (1 << k) - 1

        crit = []
        for cs_line in cs_lines:
            m = 0
            for ch, cl in tile_cells(grid_rows(cs_line)).items():
                if ch not in cells or cells[ch] != cl:
                    die("%s: cover %d: critical set %r does not match the "
                        "cover" % (path, ci, cs_line))
                m |= bit[ch]
            crit.append(m)

        if not crit:
            die("%s: cover %d has no critical sets" % (path, ci))

        # The n = 1 cover's only critical set is empty; it has no trades.
        trades = minimal_transversals(crit, k) if all(crit) else []

        # --- statistics ---
        for t in trades:
            trade_sizes[popcount(t)] += 1
        trades_per_cover[len(trades)] += 1
        smallest = min(popcount(c) for c in crit)
        pack = packing_number(trades)
        pack_vs_min[(pack, smallest)] += 1
        if pack > smallest:
            ok = False
            w("ERROR: cover %d: packing %d > smallest critical set %d\n"
              % (ci, pack, smallest))

        for t in trades:
            size = popcount(t)
            if size in (2, 3):
                region = set()
                for ch in letters:
                    if t & bit[ch]:
                        region |= cells[ch]
                (shapes2 if size == 2 else shapes3)[canonical_shape(region)] += 1

        # --- theorem checks ---
        if k >= 2 and len(crit) < 2:                                  # T1
            t1 += 1
        if any(popcount(t) == 1 for t in trades):                     # T2
            t2 += 1
        for ch in letters:                                            # T3
            in_every = bool(trades) and all(t & bit[ch] for t in trades)
            is_crit = bit[ch] in crit
            n_size1 += is_crit
            if in_every != is_crit:
                t3 += 1
        trade_set = set(trades)
        if k >= 2:                                                    # T4
            for ch in letters:
                pairs = all((bit[ch] | bit[o]) in trade_set
                            for o in letters if o != ch)
                is_crit = (full & ~bit[ch]) in crit
                n_sizen1 += is_crit
                if pairs != is_crit:
                    t4 += 1

        if args.verbose:
            names = ["".join(ch for ch in letters if t & bit[ch])
                     for t in trades]
            w("cover %d  %s  trades={%s}\n" % (ci, cover_line, ",".join(names)))

    # --- summary ---
    w("== %s: n=%d, %d covers\n" % (path, n, n_covers))
    w("   minimal trade sizes:   %s\n" % fmt_dist(trade_sizes))
    w("   trades per cover:      %s\n" % fmt_dist(trades_per_cover))
    w("   packing number vs smallest critical set (packing, smallest): count\n")
    for key in sorted(pack_vs_min):
        w("     (%d, %d): %d%s\n" % (key[0], key[1], pack_vs_min[key],
                                     "   <- bound tight" if key[0] == key[1]
                                     else ""))

    for label, shapes, top in (("2-tile", shapes2, args.top2),
                               ("3-tile", shapes3, args.top3)):
        if not shapes:
            continue
        total = sum(shapes.values())
        items = shapes.most_common()
        shown = items if not top else items[:top]
        w("   %s trade regions: %d trades, %d distinct shapes%s\n"
          % (label, total, len(shapes),
             "" if len(shown) == len(items) else " (top %d shown)" % len(shown)))
        for shape, count in shown:
            w("     %d\n%s\n" % (count, shape_grid(shape, "       ")))

    checks = [
        ("T1 every cover with n>=2 has >=2 critical sets", t1),
        ("T2 no minimal trade of size 1", t2),
        ("T3 {t} critical <=> t in every minimal trade "
         "(%d size-1 sets)" % n_size1, t3),
        ("T4 C\\{t} critical <=> {t,s} trade for all s "
         "(%d size-(n-1) sets)" % n_sizen1, t4),
    ]
    for name, bad in checks:
        w("   %-62s %s\n" % (name, "ok" if not bad else "FAILED (%d)" % bad))
        ok &= not bad
    return ok


def main():
    if hasattr(signal, "SIGPIPE"):
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    ap = argparse.ArgumentParser(
        description="Derive minimal trades from critical sets.")
    ap.add_argument("csfiles", nargs="+", metavar="CS")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="list each cover with its minimal trades")
    ap.add_argument("--top2", type=int, default=10,
                    help="2-tile trade shapes to show (default 10, 0 = all)")
    ap.add_argument("--top3", type=int, default=5,
                    help="3-tile trade shapes to show (default 5, 0 = all)")
    args = ap.parse_args()
    ok = True
    for path in args.csfiles:
        ok &= process(path, args)
    sys.exit(0 if ok else 2)


if __name__ == "__main__":
    main()
