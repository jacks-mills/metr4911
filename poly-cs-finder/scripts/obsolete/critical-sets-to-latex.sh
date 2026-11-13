#!/usr/bin/env bash

# Usage:
#   critical-sets-to-latex.sh COVER [N] [SEED]
#
# Examples:
#   ./critical-sets-to-latex.sh cover.txt
#   ./critical-sets-to-latex.sh cover.txt 15
#   ./critical-sets-to-latex.sh cover.txt 15 12345
#
# If SEED is supplied, the shuffle is reproducible.
#
# The generated LaTeX is written to stdout, so you can redirect it:
#
#   ./critical-sets-to-latex.sh cover.txt 15 12345 > table.tex

#set -euo pipefail

# ----------------------------------------------------------------------
# Argument checking
# ----------------------------------------------------------------------

if [ "$#" -lt 1 ] || [ "$#" -gt 3 ]; then
    echo "usage: critical-sets-to-latex.sh COVER [N] [SEED]" >&2
    exit 1
fi

COVER="$1"
N="${2:-10}"
SEED="${3:-}"

if [ ! -f "$COVER" ]; then
    echo "error: file not found: $COVER" >&2
    exit 1
fi

if ! [[ "$N" =~ ^[0-9]+$ ]]; then
    echo "error: N must be a non-negative integer" >&2
    exit 1
fi

if [ -n "$SEED" ] && ! [[ "$SEED" =~ ^[0-9]+$ ]]; then
    echo "error: SEED must be an integer" >&2
    exit 1
fi

# ----------------------------------------------------------------------
# Settings
# ----------------------------------------------------------------------

COLUMNS=5
MINIPAGE_WIDTH="0.19"
SCALE="0.35"

# ----------------------------------------------------------------------
# LaTeX preamble
# ----------------------------------------------------------------------

cat <<'EOF'
\documentclass[]{article}
\usepackage[dvipsnames]{xcolor}
\usepackage{tikz}
\usepackage{graphicx}
\usepackage{polyomino}

\pgfkeys{
/polyomino,
p={A}{style={Red!65, draw=black, ultra thick}},
p={B}{style={MidnightBlue!65, draw=black, ultra thick}},
p={C}{style={Green!65, draw=black, ultra thick}},
p={D}{style={Magenta!65, draw=black, ultra thick}},
p={E}{style={Cyan!65, draw=black, ultra thick}},
p={F}{style={Yellow!65, draw=black, ultra thick}},
p={G}{style={Orange!65, draw=black, ultra thick}},
p={H}{style={Gray!65, draw=black, ultra thick}},
p={I}{style={Emerald!65, draw=black, ultra thick}},
p={Z}{style={White!65, draw=black, ultra thick}}
}

\title{Poly Table}
\author{Jack Mills}
\date{\today}

\begin{document}

\begin{center}
EOF

# ----------------------------------------------------------------------
# Filter, shuffle, select and transform polyominoes
# ----------------------------------------------------------------------

if [ -n "$SEED" ]; then

    grep -v "#" "$COVER" |
        grep "\." |
        awk -v seed="$SEED" '
        # Park-Miller (Lehmer) minimal-standard LCG, self-contained so the
        # shuffle does not depend on any awk implementation'"'"'s built-in
        # rand()/srand() (some, e.g. mawk, seed rand() from OS entropy and
        # ignore the seed argument entirely, which is not deterministic).
        function next_rand() {
            state = (MULT * state) % MOD
            return state / MOD
        }
        {
            lines[NR] = $0
        }
        END {
            n = NR
            MOD = 2147483647    # 2^31 - 1 (Mersenne prime)
            MULT = 48271        # Park-Miller multiplier

            state = seed % MOD
            if (state < 0)
                state += MOD
            if (state == 0)
                state = 1

            # Fisher-Yates shuffle using the deterministic LCG above.
            for (i = n; i > 1; i--) {
                j = int(next_rand() * i) + 1
                tmp = lines[i]
                lines[i] = lines[j]
                lines[j] = tmp
            }
            for (i = 1; i <= n; i++)
                print lines[i]
        }
        '

else

    grep -v "#" "$COVER" |
        grep "\."

fi |
    head -n "$N" |
    sed -e 's/\./Z/g' -e 's/ /, /g' |
    awk -v columns="$COLUMNS" -v width="$MINIPAGE_WIDTH" -v scale="$SCALE" '
    {
        # Start a new row.
        if ((NR - 1) % columns == 0) {
            if (NR > 1)
                printf "\\vspace{1.5em}\n\n"
        }

        # Start minipage.
        printf "\\begin{minipage}[t]{%s\\textwidth}\n", width
        printf "\\centering\n"

        # TikZ picture.
        printf "\\begin{tikzpicture}[scale=%s]\n", scale

        # Polyomino.
        printf "    \\polyomino[grid]{\n"
        printf "    %s\n", $0
        printf "    }\n"

        # Close TikZ and minipage.
        printf "\\end{tikzpicture}\n"
        printf "\\end{minipage}"

        # Put \hfill between columns.
        if (NR % columns != 0)
            printf "\\hfill\n"
        else
            printf "\n"
    }

    END {
        printf "\n"
    }
    '

# ----------------------------------------------------------------------
# LaTeX ending
# ----------------------------------------------------------------------

cat <<'EOF'
\end{center}
\end{document}
EOF
