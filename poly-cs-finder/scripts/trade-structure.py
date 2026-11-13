#!/usr/bin/env python3
"""
trade-structure.py - checks on the minimal trades of each cover.

Usage:
    python3 scripts/trade-structure.py CRITICAL.cs [CRITICAL.cs ...]

Minimal trades are computed from the critical sets (minimal tile sets meeting
every critical set). For each file, reports:
  - gap m - P: smallest critical set size minus the most pairwise-disjoint
    minimal trades
  - covers whose minimal trades are all 2-tile ("pair-only"), split by whether
    the trade graph is bipartite (Konig: bipartite => gap 0)
  - gap-1 covers by cause (odd cycle in a pair-only graph, or a 3/4-tile trade)
  - covers with a hole in their spectrum and their trade structures, up to
    relabelling the tiles
  - tiles in no minimal trade (= in no critical set): count per cover and
    whether they touch the edge of the square
"""
import sys, itertools
from collections import Counter
def blocks(path):
    for b in open(path).read().split('\n\n'):
        ls=[l.strip() for l in b.splitlines() if l.strip() and not l.startswith('#')]
        if len(ls)>1: yield ls[0], ls[1:]
def letters(s): return frozenset(c for c in s if c.isalpha())
def min_transversals(V, sets):
    found=[]
    for k in range(1,len(V)+1):
        for t in itertools.combinations(V,k):
            t=frozenset(t)
            if all(t&s for s in sets) and not any(f<=t for f in found): found.append(t)
    return found
def packing(trades):
    best=0
    for k in range(len(trades),0,-1):
        for comb in itertools.combinations(trades,k):
            if all(not(a&b) for a,b in itertools.combinations(comb,2)): return k
    return 0
def bipartite(V,edges):
    col={}
    for s in V:
        if s in col: continue
        col[s]=0; st=[s]
        while st:
            u=st.pop()
            for e in edges:
                if u in e:
                    (w,)=e-{u}
                    if w not in col: col[w]=1-col[u]; st.append(w)
                    elif col[w]==col[u]: return False
    return True
def canon(V,trades):
    V=sorted(V); best=None
    for p in itertools.permutations(range(len(V))):
        m={v:p[i] for i,v in enumerate(V)}
        f=tuple(sorted(tuple(sorted(m[x] for x in t)) for t in trades))
        if best is None or f<best: best=f
    return best
def touches_edge(cover,t):
    rows=cover.split(); n=len(rows)
    return any(ch==t and (r in(0,n-1) or c in(0,n-1)) for r,row in enumerate(rows) for c,ch in enumerate(row))
for path in sys.argv[1:]:
    n=0; gap=Counter(); paironly=0; paironly_gap=Counter(); gap1_kinds=Counter()
    holes=0; hole_pair=0; hole_struct=Counter(); hole_spec=Counter(); unused=Counter(); unused_edge=0
    for cover,sets in blocks(path):
        n+=1
        V=sorted(letters(cover)); crit=[letters(s) for s in sets]
        T=min_transversals(V,crit)
        P=packing(T); m=min(len(s) for s in crit)
        g=m-P; gap[g]+=1
        po=all(len(t)==2 for t in T)
        if po:
            paironly+=1
            bp=bipartite(V,T)
            paironly_gap[(g,'bipartite' if bp else 'odd cycle')]+=1
        if g==1:
            gap1_kinds['pairs only, odd cycle' if po and not bipartite(V,T) else ('pairs only, bipartite' if po else 'has 3/4-tile trade')]+=1
        sizes=sorted({len(s) for s in crit})
        if sizes[-1]-sizes[0]+1!=len(sizes):
            holes+=1; hole_spec[tuple(sizes)]+=1; hole_pair+=po
            hole_struct[(tuple(sizes),canon(V,T))]+=1
        u=[v for v in V if not any(v in t for t in T)]
        unused[len(u)]+=1
        unused_edge+=sum(touches_edge(cover,t) for t in u)
    print('==',path,'covers',n)
    print(' gap m-P:',dict(gap))
    print(' pair-only covers:',paironly, ' by (gap, graph):',dict(paironly_gap))
    print(' gap-1 covers by kind:',dict(gap1_kinds))
    print(' covers with holes:',holes,' spectra:',dict(hole_spec),' pair-only among them:',hole_pair)
    print(' hole trade structures (top):')
    for (sp,st),c in hole_struct.most_common(8): print('   ',sp,c,st)
    print(' unused tiles per cover:',dict(unused),' unused tiles touching edge:',unused_edge)
