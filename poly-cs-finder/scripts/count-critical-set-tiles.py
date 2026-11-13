#!/usr/bin/env python3
"""Count how often each polyomino tile appears across the critical sets.

Input format is the same as count-critical-sets-by-spectrum.py: '#' comment lines are ignored,
paragraphs are separated by blank lines, the first line of a paragraph is the
cover and every following line is one critical set.

A tile is the binary occupancy string of one polyomino over the board cells
(spaces ignored), e.g. in "AAA BBB ..." polyomino A is 111000000 and B is
000111000. The same cells under different letters count as the same tile.

By default tiles are grouped under translation and rotation (90/180/270
degrees): each is shifted to the top-left corner of the board in all four
orientations, so e.g. a horizontal bar in any row and a vertical bar in any
column count as the same tile.

Usage: count-critical-set-tiles.py [-t | -p] [-n] [-l] [--cols K] [--top N] [--scale S] [FILE ...]

Arguments:
  FILE      critical-sets file(s); each gets its own count/table.
            Reads stdin if none given.
  -t        group under translation only (rotations stay separate).
  -p        no grouping: tiles are counted at their exact board position.
            -t and -p can't be combined.
  -n        normalise: divide each tile's count by the number of covers
            containing it, and rank by that. Text output then shows
            count, covers and the ratio.
  -l        print a LaTeX longtable per file instead of plain text.
  --cols K  LaTeX only: tile/count pairs per table row (default 4).
  --top N   only the N most frequent tiles per file (default 0 = all).
            Applies to the LaTeX table only.
  --scale S LaTeX only: tikz scale of each tile picture (default 0.3).
"""
import argparse
import math
import sys
from collections import Counter
from pathlib import Path


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


def tiles(line):
    cells = line.replace(" ", "")
    for letter in sorted({c for c in cells if c.isalpha()}):
        yield "".join("1" if c == letter else "0" for c in cells)


def board_side(tile):
    side = math.isqrt(len(tile))
    if side * side != len(tile):
        sys.exit(f"tile {tile} is not a square board")
    return side


def to_corner(tile):
    """Translate a tile so its bounding box touches the top-left corner.

    Every translate of the same shape maps to the same string, so using this
    as the key groups tiles that are equal under translation.
    """
    side = board_side(tile)
    cells = [divmod(i, side) for i, b in enumerate(tile) if b == "1"]
    top = min(r for r, _ in cells)
    left = min(c for _, c in cells)
    out = ["0"] * len(tile)
    for r, c in cells:
        out[(r - top) * side + (c - left)] = "1"
    return "".join(out)


def rotate(tile):
    """Rotate a tile 90 degrees clockwise about the centre of the board."""
    side = board_side(tile)
    out = ["0"] * len(tile)
    for i, b in enumerate(tile):
        if b == "1":
            r, c = divmod(i, side)
            out[c * side + (side - 1 - r)] = "1"
    return "".join(out)


def canonical(tile):
    """Key that is equal for tiles equal under translation and rotation.

    Takes the corner-translated form of all four rotations and picks the
    largest string, which favours orientations with occupied cells early in
    the top row (e.g. a bar is shown horizontally).
    """
    forms = []
    for _ in range(4):
        forms.append(to_corner(tile))
        tile = rotate(tile)
    return max(forms)


def count_tiles(lines, grouping="rotate"):
    """Count tiles in the critical sets and the covers they come from.

    grouping: 'rotate' (translation + rotation), 'translate' or 'exact'.
    Returns (counts, covers): counts[t] is the number of occurrences of t
    across all critical sets; covers[t] is the number of covers (paragraph
    first lines) that contain t at least once.
    """
    key = {"rotate": canonical, "translate": to_corner,
           "exact": lambda t: t}[grouping]
    counts, covers = Counter(), Counter()
    for cover, *crit_sets in paragraphs(lines):
        covers.update(set(map(key, tiles(cover))))
        for cs in crit_sets:
            counts.update(map(key, tiles(cs)))
    return counts, covers


def scores(counts, covers, normalise):
    """Value each tile is ranked by: raw count, or count per cover."""
    if not normalise:
        return dict(counts)
    return {t: n / covers[t] if covers[t] else float("inf")
            for t, n in counts.items()}


def ranked(values):
    # Highest first; ties broken by tile string for stable output.
    return sorted(values.items(), key=lambda kv: (-kv[1], kv[0]))


def text_report(results, normalise):
    # Column widths shared across all files so output lines up.
    count_w = max((len(str(n)) for _, c, _ in results for n in c.values()),
                  default=0)
    cover_w = max((len(str(n)) for _, _, v in results for n in v.values()),
                  default=0)
    blocks = []
    for name, counts, covers in results:
        lines = [f"{name}:"] if len(results) > 1 else []
        for tile, s in ranked(scores(counts, covers, normalise)):
            line = f"{tile}  {counts[tile]:>{count_w}}"
            if normalise:
                line += f"  {covers[tile]:>{cover_w}}  {s:.3f}"
            lines.append(line)
        blocks.append("\n".join(lines))
    return "\n\n".join(blocks)


def tile_tex(tile, scale):
    """Draw a tile with the polyomino package: A = occupied, J = empty."""
    side = board_side(tile)
    rows = (tile[i:i + side] for i in range(0, len(tile), side))
    body = ", ".join(r.replace("1", "A").replace("0", "J") for r in rows)
    return (f"\\begin{{tikzpicture}}[scale={scale},"
            f" baseline=(current bounding box.center)]"
            f"\\polyomino[grid]{{{body}}}\\end{{tikzpicture}}")


def latex_count(n):
    return f"{n:,}".replace(",", "\\,")


def latex_report(results, cols, top, scale, normalise):
    label = "Per cover" if normalise else "Count"
    header = " & ".join([f"Tile & {label}"] * cols) + r" \\"
    tables = []
    for name, counts, covers in results:
        stem = Path(name).stem
        values = scores(counts, covers, normalise)
        items = ranked(values)[:top] if top else ranked(values)
        if not items:
            tables.append(f"% {name}: no tiles")
            continue
        fmt = (lambda s: f"{s:.2f}") if normalise else latex_count
        cells = [f"{tile_tex(t, scale)} & {fmt(s)}" for t, s in items]
        cells += ["&"] * (-len(cells) % cols)          # pad the last row
        rows = [" & ".join(cells[i:i + cols]) + r" \\[4pt]"
                for i in range(0, len(cells), cols)]
        caption = (f"Tile frequencies across the critical sets of {stem}"
                   + (", per cover containing the tile" if normalise else ""))
        if top and len(counts) > top:
            caption += f" (top {top} of {len(counts)})"
        tables.append("\n".join([
            f"\\begin{{longtable}}{{*{{{cols}}}{{cr}}}}",
            f"  \\caption{{{caption}.}}\\label{{tab:tiles-{stem}}} \\\\",
            r"  \toprule", f"  {header}", r"  \midrule", r"  \endfirsthead",
            r"  \toprule", f"  {header}", r"  \midrule", r"  \endhead",
            r"  \bottomrule", r"  \endlastfoot",
            *(f"  {r}" for r in rows),
            r"\end{longtable}",
        ]))
    preamble = ("% Requires \\usepackage{booktabs, longtable, tikz, polyomino}"
                " and the A/J styles from your \\pgfkeys block.")
    return preamble + "\n\n" + "\n\n".join(tables)


def main():
    parser = argparse.ArgumentParser(
        description="Count occurrences of each tile across all critical sets.")
    parser.add_argument("files", nargs="*", type=argparse.FileType(),
                        default=[sys.stdin],
                        help="critical-sets files (default: stdin)")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-t", "--translate-only", dest="grouping",
                       action="store_const", const="translate",
                       help="group under translation only, not rotation")
    group.add_argument("-p", "--positional", dest="grouping",
                       action="store_const", const="exact",
                       help="no grouping: keep tiles at their board position")
    parser.set_defaults(grouping="rotate")
    parser.add_argument("-n", "--normalise", action="store_true",
                        help="divide each count by the number of covers "
                             "containing the tile, and rank by that")
    parser.add_argument("-l", "--latex", action="store_true",
                        help="print a LaTeX longtable per file instead")
    parser.add_argument("--cols", type=int, default=4,
                        help="tile/count pairs per table row (default: 4)")
    parser.add_argument("--top", type=int, default=0,
                        help="only the N most frequent tiles per file")
    parser.add_argument("--scale", type=float, default=0.3,
                        help="tikz scale of each tile picture (default: 0.3)")
    args = parser.parse_args()

    results = []
    for f in args.files:
        with f:
            results.append((f.name, *count_tiles(f, args.grouping)))

    if args.latex:
        print(latex_report(results, args.cols, args.top, args.scale,
                           args.normalise))
    else:
        print(text_report(results, args.normalise))


if __name__ == "__main__":
    main()
