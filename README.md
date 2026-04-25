# METR4911

Currently (2026/04/25), this repo contains two projects: cover and polyomino-printer


## Cover
Cover is a C program meant to solve exact-cover problems. The algorithm uses Knuth's [Dancing
Links](https://doi.org/10.48550/arXiv.cs/0011047) as a base. The program is unfinished, and
currently just initialises the problem discusses in Knuth's paper, and checks if any errors
occurred.

## Polyomino Printer
Polyomino printer is a C program that prints lists of polyomino coverings in the terminal. The
program is complete and includes an example input file. The program reads covers from standard
input, in the following form

```
# A 3x3 3-polyomino cover
AAA BBB CCC
```

One cover per line, and prints a graphical representaiton of the cover. The program performs limited
input validation, thus malformed inputs can have incorrect outputs.
