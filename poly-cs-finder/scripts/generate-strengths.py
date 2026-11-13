#!/usr/bin/env python3
"""
generate-strengths.py - classify polyomino critical sets by strength, adapting the
classes of Adams, Bean & Khodkar, "A census of critical sets in the Latin
squares of order at most six" (2003).

Usage:
    python3 generate-strengths.py [-v] [-j JOBS] CRITICAL.cs [CRITICAL.cs ...]

Only local information is used: the solver knows the board is N x N and is
tiled by N-ominoes, but it does NOT see the universe of covers.

Definitions
    Placement: any set of N edge-connected cells inside the N x N board,
    i.e. any fixed N-omino at any position.

    Candidate: a placement that overlaps no tile already on the board.

    Forced tile: an empty cell that exactly one candidate covers. That
    candidate must be a tile of the cover. This is the polyomino version of
    the Latin-square forcing rules: in exact-cover terms, a constraint (here a
    cell) that only one remaining option can satisfy.

    Reduction (pigeonhole step, the polyomino version of reducing the
    prospect array): discard every candidate that would leave an
    edge-connected empty region whose size is not a multiple of N, since such
    a region can never be filled with N-ominoes.

    Propagation: repeatedly place every forced tile until the board is full
    or nothing more is forced. Forced tiles are always tiles of the cover, so
    the order of placement does not matter and propagation decides whether
    some sequence of forced steps exists.

Classes (each critical set S of cover C)
    strong              propagation WITHOUT reduction completes C
    near-strong         not strong, but propagation WITH reduction completes C
    neither             not even near-strong
    totally weak        S itself has no forced tile (without reduction)
    BW totally weak     S itself has no forced tile, even with reduction
                        (Bedford-Whitehouse)

Output: one line per critical set (with -v), then a summary per file.
Exit status: 0 ok, 1 input error, 2 a consistency check failed.
"""

import argparse
import os
import sys
from collections import Counter
from multiprocessing import Pool


def die(msg):
    sys.stderr.write("generate-strengths.py: " + msg + "\n")
    sys.exit(1)


# --------------------------------------------------------------------- parsing

def grid_rows(line):
    rows = line.split()
    n = len(rows)
    if n == 0 or any(len(r) != n for r in rows):
        die("not an n x n grid: %r" % line)
    return rows


def tiles_of(rows):
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


# ------------------------------------------------------------------ placements

def all_placements(n):
    """Every edge-connected set of n cells in the n x n board (bitmasks)."""
    def neighbours(cell):
        r, c = divmod(cell, n)
        if r > 0: yield cell - n
        if r < n - 1: yield cell + n
        if c > 0: yield cell - 1
        if c < n - 1: yield cell + 1

    level = {1 << c for c in range(n * n)}
    for _ in range(n - 1):
        nxt = set()
        for s in level:
            m = s
            while m:
                low = m & -m
                cell = low.bit_length() - 1
                m ^= low
                for nb in neighbours(cell):
                    if not s >> nb & 1:
                        nxt.add(s | (1 << nb))
        level = nxt
    return sorted(level)


class Solver:
    def __init__(self, n):
        self.n = n
        self.full = (1 << (n * n)) - 1
        self.placements = all_placements(n)
        self.by_cell = [[p for p in self.placements if p >> c & 1]
                        for c in range(n * n)]
        first_col = sum(1 << (r * n) for r in range(n))
        self.not_first_col = self.full & ~first_col
        self.not_last_col = self.full & ~(first_col << (n - 1))

    def regions_ok(self, empty):
        """Every edge-connected region of `empty` has size divisible by n."""
        n = self.n
        nfc, nlc = self.not_first_col, self.not_last_col
        while empty:
            comp = empty & -empty
            while True:
                grow = (comp | ((comp << 1) & nfc) | ((comp >> 1) & nlc)
                        | (comp << n) | (comp >> n)) & empty
                if grow == comp:
                    break
                comp = grow
            if bin(comp).count("1") % n:
                return False
            empty &= ~comp
        return True

    def forced_step(self, occ, reduce):
        """Return the set of forced placements for board `occ`, or None if
        some empty cell has no candidate at all (dead end)."""
        forced = set()
        covered = occ
        empty = self.full & ~occ
        ok_cache = {}
        for c in range(self.n * self.n):
            if covered >> c & 1:
                continue
            only = None
            count = 0
            for p in self.by_cell[c]:
                if p & occ:
                    continue
                if reduce:
                    v = ok_cache.get(p)
                    if v is None:
                        v = ok_cache[p] = self.regions_ok(empty & ~p)
                    if not v:
                        continue
                count += 1
                if count == 2:
                    break
                only = p
            if count == 0:
                return None
            if count == 1:
                forced.add(only)
                covered |= only
        return forced

    def propagate(self, occ, reduce, cover_tiles):
        """Place forced tiles until stuck. Returns (status, rounds, first)
        where status is 'complete', 'stuck' or 'error', and first is the
        number of tiles forced directly by the starting set."""
        rounds = 0
        first = None
        while occ != self.full:
            step = self.forced_step(occ, reduce)
            if step is None:
                return "error", rounds, first or 0
            if first is None:
                first = len(step)
            if not step:
                return "stuck", rounds, first
            for p in step:
                if p not in cover_tiles or p & occ:
                    return "error", rounds, first
                occ |= p
            rounds += 1
        return "complete", rounds, first or 0


# ------------------------------------------------------------------------ main

_solver = None


def _init(n):
    global _solver
    _solver = Solver(n)


def classify_block(block):
    """Classify every critical set of one cover. Returns a list of tuples
    (cs_line, size, cls, tw, bwtw, rounds_plain, rounds_reduced, error)."""
    cover_line, cs_lines = block
    sv = _solver
    cover = tiles_of(grid_rows(cover_line))
    cover_tiles = set(cover.values())
    out = []
    for cs_line in cs_lines:
        cs = tiles_of(grid_rows(cs_line))
        occ = 0
        err = False
        for k, m in cs.items():
            if cover.get(k) != m:
                err = True
            occ |= m
        st_p, r_p, first_p = sv.propagate(occ, False, cover_tiles)
        st_r, r_r, first_r = sv.propagate(occ, True, cover_tiles)
        err = err or st_p == "error" or st_r == "error"
        if st_p == "complete":
            cls = "strong"
        elif st_r == "complete":
            cls = "near-strong"
        else:
            cls = "neither"
        # The full cover minus nothing is never a critical set, so occ is
        # never full and first_* is the count of directly forced tiles.
        tw = first_p == 0
        bwtw = first_r == 0
        out.append((cs_line, len(cs), cls, tw, bwtw, r_p, r_r, err))
    return cover_line, out


def process(path, jobs, verbose):
    blocks = list(read_blocks(path))
    if not blocks:
        die("%s: no covers" % path)
    n = len(grid_rows(blocks[0][0]))

    by_class = Counter()
    by_size_class = Counter()
    flags = Counter()
    errors = 0
    total = 0
    examples = {}

    with Pool(jobs, initializer=_init, initargs=(n,)) as pool:
        for ci, (cover_line, res) in enumerate(
                pool.imap(classify_block, blocks, chunksize=64), 1):
            for si, (cs_line, size, cls, tw, bwtw, rp, rr, err) in \
                    enumerate(res, 1):
                total += 1
                errors += err
                by_class[cls] += 1
                by_size_class[(size, cls)] += 1
                flags[(cls, tw, bwtw)] += 1
                key = (cls, tw, bwtw)
                if key not in examples:
                    examples[key] = (ci, si, cs_line)
                if verbose or err:
                    sys.stdout.write(
                        "cover %d cs %d  %s  size=%d  %s%s%s  rounds=%d/%d%s\n"
                        % (ci, si, cs_line, size, cls,
                           "  totally-weak" if tw else "",
                           "  BW-totally-weak" if bwtw else "",
                           rp, rr, "  ERROR" if err else ""))

    w = sys.stdout.write
    w("== %s: n=%d, %d covers, %d critical sets, %d placements\n"
      % (path, n, len(blocks), total, len(Solver(n).placements)
         if n <= 6 else -1))
    for cls in ("strong", "near-strong", "neither"):
        w("   %-12s %8d\n" % (cls, by_class[cls]))
    w("   by size:\n")
    sizes = sorted({s for s, _ in by_size_class})
    for s in sizes:
        w("     size %d: %s\n" % (s, "  ".join(
            "%s %d" % (cls, by_size_class[(s, cls)])
            for cls in ("strong", "near-strong", "neither"))))
    w("   (class, totally weak, BW totally weak): count  [first example]\n")
    for key in sorted(flags):
        ci, si, line = examples[key]
        w("     %-28s %8d  [cover %d cs %d: %s]\n"
          % ("%s, %s, %s" % key, flags[key], ci, si, line))
    if errors:
        w("   CONSISTENCY ERRORS: %d\n" % errors)
    return errors == 0


def main():
    ap = argparse.ArgumentParser(
        description="Classify polyomino critical sets by strength.")
    ap.add_argument("csfiles", nargs="+", metavar="CS")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1)
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="print one line per critical set")
    args = ap.parse_args()
    ok = True
    for path in args.csfiles:
        ok &= process(path, args.jobs, args.verbose)
    sys.exit(0 if ok else 2)


if __name__ == "__main__":
    main()
