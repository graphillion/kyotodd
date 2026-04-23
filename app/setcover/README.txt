Set Cover — weighted 0/1 set cover via BDD + ZDD
==================================================

Given a universe U = {1..n} and a family F = {S_1, ..., S_m} of subsets,
each with a non-negative cost c_i, find subsets of F whose union covers
U with minimum total cost.

Encoding: one Boolean variable x_i per subset. For each element e in U
add the positive clause OR_{i : e in S_i} x_i ("at least one subset
containing e is chosen"). AND all clauses for the characteristic BDD,
then convert to a ZDD of covers via BDD::to_zdd(m).

Outputs:

  * #feasible covers             (exact BigInt count)
  * #subset-minimal covers       (minimal() then exact_count)
  * the minimum-cost cover       (min_weight_set)
  * the top-5 lightest covers    (get_k_lightest, re-sorted by cost)

Usage:
    setcover <file.txt>

Input format
------------
First non-comment tokens: two integers m (subset count) and n (universe
size; elements are 1..n). Then m lines, one subset per line:

    <cost_i>  <e_{i,1}>  <e_{i,2}>  ...

Costs are non-negative integers. Element numbers must be in 1..n.
Duplicate elements within a line are deduplicated. Whitespace separates
tokens. Lines starting with 'c', 'C', '#', or '%' are comments; blank
lines are ignored.

Example (small.txt):
    4 5
    2 1 2 3      # S_1 = {1,2,3}, cost=2
    3 2 4        # S_2 = {2,4},   cost=3
    4 3 4 5      # S_3 = {3,4,5}, cost=4
    1 1 5        # S_4 = {1,5},   cost=1

Sample instances (run from the build directory):
    ./setcover ../app/setcover/small.txt       # hand-checkable
    ./setcover ../app/setcover/medium.txt      # m=10, n=8
    ./setcover ../app/setcover/infeasible.txt  # element 5 uncoverable

Notes
-----
* Subset indices in the output correspond to the input order (1..m).
* Top-5 is re-sorted by ascending cost (ZDD enumeration order reflects
  the diagram structure, not cost).
* Every instance reports whether the min-cost cover covers all n
  elements (✓/!); a ! would indicate a bug in the encoding.
