#!/usr/bin/env python3
"""
forced-tiles.py - find "forced" tiles in polyomino critical sets.

Usage:
    python3 forced-tiles.py [options] UNIVERSE.uni CRITICAL.cs [CRITICAL.cs ...]

Definitions
    Placement universe: every distinct piece (set of cells) that appears in at
    least one tiling listed in the .uni file.

    Valid placement: a piece from the placement universe that
      (a) overlaps no tile already placed, and
      (b) leaves every connected region of empty cells with a size that is
          a multiple of n (otherwise that region could never be tiled by
          n-ominoes). Disable (b) with --no-region-check.

    Forced tile: start from the tiles in a critical set. Look at an empty
    cell. If exactly one valid placement covers that cell, that piece is
    forced. It is necessarily a tile of the full cover.

    Propagation: place every forced tile, then look for forced tiles again,
    and repeat. "complete" means forcing alone rebuilds the whole cover.
    "stuck" means it stops with tiles still missing, so finishing the cover
    needs search/backtracking.

Output (one line per critical set, then a summary per file):
    cover <i> cs <j>  <grid>  set={...}  forced={...}  propagation=...

Exit status: 0 = ok, 1 = input error, 2 = a consistency check failed.

Requires only the Python 3 standard library.
"""

import argparse
import signal
import sys


def die(msg):
    sys.stderr.write("forced-tiles.py: " + msg + "\n")
    sys.exit(1)


# --------------------------------------------------------------------- parsing

def grid_rows(line, where):
    """Split a grid line ('AAB ACB CCB') into its rows and check it is n x n."""
    rows = line.split()
    n = len(rows)
    if n == 0 or any(len(r) != n for r in rows):
        die("%s: not an n x n grid: %r" % (where, line))
    return rows


def tiles_of(rows):
    """Map each tile label to a bitmask of its cells (cell index = r*n + c).
    '.' marks an empty cell."""
    n = len(rows)
    tiles = {}
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch != ".":
                tiles[ch] = tiles.get(ch, 0) | (1 << (r * n + c))
    return tiles


def read_universe(path):
    """Return (n, set of piece bitmasks) from a .uni file."""
    pieces = set()
    n = None
    with open(path) as f:
        for lineno, line in enumerate(f, 1):
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            rows = grid_rows(s, "%s:%d" % (path, lineno))
            if n is None:
                n = len(rows)
            elif len(rows) != n:
                die("%s:%d: grid size changes" % (path, lineno))
            pieces.update(tiles_of(rows).values())
    if n is None:
        die("%s: no tilings found" % path)
    return n, pieces


def read_blocks(path):
    """Yield (lineno, cover_line, [(lineno, cs_line), ...]) for each block of
    a .cs file. Blocks are separated by blank lines; '#' lines are comments."""
    cur = []
    with open(path) as f:
        for lineno, line in enumerate(f, 1):
            s = line.strip()
            if s.startswith("#"):
                continue
            if not s:
                if cur:
                    yield cur[0][0], cur[0][1], cur[1:]
                    cur = []
                continue
            cur.append((lineno, s))
    if cur:
        yield cur[0][0], cur[0][1], cur[1:]


# -------------------------------------------------------------------- analysis

class ForcingAnalyser:
    """Works out forced tiles for a given set of occupied cells."""

    def __init__(self, n, pieces, region_check=True):
        self.n = n
        self.pieces = pieces
        self.region_check = region_check
        self.full = (1 << (n * n)) - 1
        first_col = sum(1 << (r * n) for r in range(n))
        self.not_first_col = self.full & ~first_col
        self.not_last_col = self.full & ~(first_col << (n - 1))
        # by_cell[c] = every piece that covers cell c
        self.by_cell = [[p for p in pieces if p >> c & 1]
                        for c in range(n * n)]
        self.cache = {}

    def cell_name(self, c):
        return "r%dc%d" % (c // self.n + 1, c % self.n + 1)

    def regions_ok(self, empty):
        """True if every 4-connected region of the `empty` bitmask has a
        size that is a multiple of n."""
        n = self.n
        nfc, nlc = self.not_first_col, self.not_last_col
        while empty:
            comp = empty & -empty                 # seed: lowest empty cell
            while True:                           # flood fill
                grow = (comp
                        | ((comp << 1) & nfc)     # step right
                        | ((comp >> 1) & nlc)     # step left
                        | (comp << n)             # step down
                        | (comp >> n)) & empty    # step up
                if grow == comp:
                    break
                comp = grow
            if bin(comp).count("1") % n:
                return False
            empty &= ~comp
        return True

    def forced_step(self, occ):
        """Return {piece: forcing cell} for the board state `occ`, or None
        if some empty cell cannot be covered by any valid placement."""
        forced = {}
        covered = occ
        empty = self.full & ~occ
        valid = {}          # piece -> passes the region check (this step)
        check = self.region_check
        c = 0
        while empty >> c:
            # Skip cells already covered by a piece found forced this step:
            # if such a cell had a single candidate, it was that piece.
            if (empty >> c & 1) and not (covered >> c & 1):
                only = None
                count = 0
                for p in self.by_cell[c]:
                    if not (p & occ):
                        if check:
                            v = valid.get(p)
                            if v is None:
                                v = valid[p] = self.regions_ok(empty & ~p)
                            if not v:
                                continue
                        count += 1
                        if count == 2:
                            break
                        only = p
                if count == 0:
                    return None
                if count == 1:
                    forced[only] = c
                    covered |= only
            c += 1
        return forced

    def analyse(self, occ):
        """Return (first, status, rounds, placed):
            first   {piece: cell} forced directly by the critical set
                    (None if the state is already a dead end)
            status  'complete', 'stuck' or 'dead-end'
            rounds  number of forcing rounds performed
            placed  set of every piece placed by propagation
        """
        key = occ
        if key in self.cache:
            return self.cache[key]

        first = self.forced_step(occ)
        if first is None:
            res = (None, "dead-end", 0, set())
            self.cache[key] = res
            return res

        placed = set()
        rounds = 0
        step = first
        status = None
        while step:
            rounds += 1
            for p in step:
                if p & occ:            # two forced pieces overlap: no tiling
                    status = "dead-end"
                    break
                occ |= p
                placed.add(p)
            if status or occ == self.full:
                break
            step = self.forced_step(occ)
            if step is None:
                status = "dead-end"
                break
        if status is None:
            status = "complete" if occ == self.full else "stuck"

        res = (first, status, rounds, placed)
        self.cache[key] = res
        return res


# ------------------------------------------------------------------------ main

def process_file(path, an, args):
    """Analyse one .cs file. Returns True if every consistency check passed."""
    n = an.n
    ok = True
    n_cov = n_cs = n_forced = n_unforced = n_complete = n_stuck = n_dead = 0
    out = sys.stdout.write

    if not args.quiet:
        out("== %s  (universe %s, n=%d, %d distinct pieces, region check %s)\n"
            % (path, args.universe, n, len(an.pieces),
               "on" if an.region_check else "off"))

    for ci, (cl, cover_line, cs_lines) in enumerate(read_blocks(path), 1):
        n_cov += 1
        cover_rows = grid_rows(cover_line, "%s:%d" % (path, cl))
        if len(cover_rows) != n:
            die("%s:%d: cover is %dx%d but universe is %dx%d"
                % (path, cl, len(cover_rows), len(cover_rows), n, n))
        cover = tiles_of(cover_rows)
        label_of = {m: k for k, m in cover.items()}
        for m, k in label_of.items():
            if m not in an.pieces:
                sys.stderr.write("warning: %s:%d: tile %s of cover %d is not "
                                 "in the universe\n" % (path, cl, k, ci))

        for si, (sl, cs_line) in enumerate(cs_lines, 1):
            n_cs += 1
            cs = tiles_of(grid_rows(cs_line, "%s:%d" % (path, sl)))
            occ = 0
            for k, m in cs.items():
                if cover.get(k) != m:
                    die("%s:%d: tile %s does not match cover %d"
                        % (path, sl, k, ci))
                occ |= m

            first, status, rounds, placed = an.analyse(occ)

            # Sanity check: anything forcing places must be a cover tile.
            if any(p not in label_of for p in placed):
                sys.stderr.write(
                    "warning: %s:%d: forcing placed a piece that is not in "
                    "the cover (cover not the unique completion?)\n"
                    % (path, sl))
                ok = False

            has_forced = bool(first)
            n_forced += has_forced
            n_unforced += not has_forced
            n_complete += status == "complete"
            n_stuck += status == "stuck"
            n_dead += status == "dead-end"
            if status == "dead-end":
                ok = False

            if args.quiet or (args.unforced and has_forced):
                continue

            items = sorted((first or {}).items(),
                           key=lambda pc: label_of.get(pc[0], "?"))
            if args.verbose:
                ftxt = ",".join("%s@%s" % (label_of.get(p, "?"),
                                           an.cell_name(c)) for p, c in items)
            else:
                ftxt = ",".join(label_of.get(p, "?") for p, _ in items)

            prop = status
            if status != "dead-end":
                prop += "(%d round%s)" % (rounds, "" if rounds == 1 else "s")
            if status == "stuck":
                missing = sorted(set(cover) - set(cs)
                                 - {label_of.get(p, "?") for p in placed})
                prop += " missing={%s}" % ",".join(missing)

            out("cover %d cs %d  %s  set={%s}  forced={%s}  propagation=%s\n"
                % (ci, si, cs_line, ",".join(sorted(cs)), ftxt, prop))

    out("-- %s: %d covers, %d critical sets\n" % (path, n_cov, n_cs))
    out("   with a forced tile:     %d\n" % n_forced)
    out("   with no forced tile:    %d\n" % n_unforced)
    out("   completed by forcing:   %d\n" % n_complete)
    out("   stuck (needs search):   %d\n" % n_stuck)
    if n_dead:
        out("   dead-end (ERROR):       %d\n" % n_dead)
    return ok


def main():
    # Exit quietly when output is piped into e.g. `head` (not on Windows).
    if hasattr(signal, "SIGPIPE"):
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    ap = argparse.ArgumentParser(
        description="Find forced tiles in polyomino critical sets.",
        epilog="See the module docstring for the definitions used.")
    ap.add_argument("universe", help="universe file (.uni)")
    ap.add_argument("csfiles", nargs="+", metavar="CS",
                    help="critical set file(s) (.cs)")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="print only the per-file summary")
    ap.add_argument("-u", "--unforced", action="store_true",
                    help="list only critical sets with NO forced tile")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="show the cell forcing each tile, e.g. C@r2c4 "
                         "(1-based, top-left is r1c1)")
    ap.add_argument("--no-region-check", action="store_true",
                    help="do not reject placements that leave an empty "
                         "region whose size is not a multiple of n")
    args = ap.parse_args()

    n, pieces = read_universe(args.universe)
    an = ForcingAnalyser(n, pieces, region_check=not args.no_region_check)

    ok = True
    for path in args.csfiles:
        ok &= process_file(path, an, args)
    sys.exit(0 if ok else 2)


if __name__ == "__main__":
    main()
