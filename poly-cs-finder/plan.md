# PLAN

Program name: pcritical


* read polyomino covering

0. read polyomino covering from standard in, and validate
1. Find the intersection of the cover with all others, and update the status array



## READING COVERS FROM FILE

* aim to now use dynamic memory if I can


* read cover line
* convert line to cover
* if cover is bad, print error message "cover on line X is bad"

* covers_from_stream(stream, len, covers, numCovers, lineNumber)
* cover_status_update(stream, cover, status)


UNIVERSE FILE BNF
```
empty_line  = [*LWS] CR
comment     = [*LWS] "#" *TEXT CR
cover       = [*LWS] COVER [*LWS] ["#" *TEXT ]CR
```


## HIGH LEVEL PROGRAM STRUCTURE


* Get cover of interest from user (via command line or standard in)
* Validate cover of interest

* Get universe of covers filepath from user
* Validate universe of covers filepathj

* In batches, validate the contents of the cover universe file while updating the status array.
  Update the status array by marking the intersection of the cover of interest with covers in the
  cover universe file as non-critical. If an error is found in the cover universe file, halt
  updating the status array and find any remaining errors in the cover universe file

* Mark subsets of non-critical sets an non-critical

* Mark supersets of unflagged sets an non-critical

* Identify all partial covers flagged as critical and find their group representative

* Output all group representatives





## TRANSVERSING SUBSETS AND SUPERSETS THINKING
* perhaps use a depth-first-search style of algorithm
* say you have a universe of three elements a, b, and c, and you represent sets with binary values
* the set {a} is represented by the binary value 100
* algorithm A:
    * you're given a set
    * starting at the least-significant bit, loop over all the bits in the binary value
    * when a zero if found, recurse the algorithm
    * when the recurse is finished, continue the loop
    * after the loop you're done

* subset path exploration
    1 00   1 000
    2 01   2 001
    3 11   3 011
    4 10   4 111
    5 11   5 101
           6 111
           7 010
           8 011
           9 111
           a 110
           b 111
           c 100
           d 101
           e 111

* superset path exploration
    1 110  1 111
    2 100  2 110
    3 000  3 100
    4 010  4 000
    5 000  5 010
           6 000
           7 ...

* unfortunately algorithm A visits the same set multiple times. it does, however visist each one in
  a different order


* visist all sets with a
* visist all sets with b but not a
* visist all sets with c but not a or b



1: given a set {a, b, c}
2: remove the first element and find all subsets SB
3: for all sets sb in SB print set {a} + sb
4: for all sets sb print sb


sub set enumeration follows the pattern of binary counting

F(00, x) = P(x)
F( x, y) = F(head(x), head(x) | y); F(head(x), y)
                      ^ set with a             ^ set without a
                                      


stack method based execution:
F(11,00)

F(01,10)
F(01,00)

F(01,10)
F(00,01)
F(00,00)
        -> "00"
F(01,10)
F(00,01)
        -> "01"
F(01,10)

F(00,11)
F(00,10)
        -> "10"
F(00,11)
        -> "11"


## THINKING ABOUT HOW TO WRITE `covers_from_strem`
* currently five arguments passed to funciton: stream, lineNum, numCovers, len, covers

