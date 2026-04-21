Sudoku — solve a 4x4 Sudoku puzzle via BDD
============================================

Builds a BDD over 64 variables (x_{r,c,d} = "cell (r,c) holds digit d"),
reports the BDD size and the number of satisfying assignments (= valid
completions), then uses ZDD::sample_k to draw up to 5 random solutions.

Usage:
    sudoku <file.txt>

Input format
------------
The parser is lenient. It accepts either "4 lines of 4 characters" or
"16 characters on a single line". Characters are read left-to-right,
top-to-bottom:
    '1'..'4' = given digit,
    '0', '.' = blank,
    ' ', '\t', '|', '-', '+', ',' and newlines are ignored,
    lines beginning with 'c', 'C', '#', or '%' are treated as comments.

Sample puzzles (run from the build directory):
    ./sudoku ../app/Sudoku/easy.txt   # unique solution
    ./sudoku ../app/Sudoku/multi.txt  # several solutions — sampled
    ./sudoku ../app/Sudoku/unsat.txt  # conflicting clues, no solution

Why 4x4 instead of 9x9?
-----------------------
A standard 9x9 Sudoku over 729 Boolean variables explodes the intermediate
BDD sizes when the 324 "exactly one" constraints are AND-ed naively — the
BDD grows into the millions during construction. Serious BDD/ZDD-based
Sudoku solvers use frontier methods or exact-cover encodings. For a
teaching-sized demo, 4x4 is large enough to illustrate encoding, exact
counting, and random sampling, and small enough that the straightforward
encoding finishes instantly.
