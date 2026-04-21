Knapsack — solve the 0/1 knapsack via ZDD
==========================================

Given N items with per-item weight w_i and value v_i and a capacity C,
the 0/1 knapsack problem asks for a subset of items whose total weight
is at most C and whose total value is maximum.

This sample builds the power-set ZDD over N Boolean variables (one per
item), filters it by the capacity constraint using cost_bound_le, then
reports:

  * the exact number of feasible subsets (BigInt),
  * the optimal (max-value) subset via max_weight_set, and
  * the top-5 subsets by value via get_k_heaviest.

Usage:
    knapsack <file.txt>

Input format
------------
The first non-comment line contains two integers: the item count N and
the capacity C. The next N lines give one item per line as two
non-negative integers: the weight and the value of that item.

Tokens may be split across lines; whitespace is flexible. Lines
beginning with 'c', 'C', '#', or '%' are treated as comments.

Example (small.txt, N=5, C=10):
    5 10
    2 3
    3 4
    4 5
    5 6
    9 10

Sample instances (run from the build directory):
    ./knapsack ../app/knapsack/small.txt   # N=5,  C=10
    ./knapsack ../app/knapsack/medium.txt  # N=10, C=50
    ./knapsack ../app/knapsack/tight.txt   # N=8,  C=5 (few feasible sets)

Notes
-----
* Item numbers in the output correspond to the order in the input
  (starting from 1).
* top-5 solutions are re-sorted by descending value before printing.
  ZDD enumeration order reflects the diagram structure, not value.
* Every instance admits the empty set as a feasible solution, so the
  #feasible count is always at least 1.
