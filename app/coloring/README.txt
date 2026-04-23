Graph Coloring — MVBDD-based proper k-coloring
================================================

Each vertex becomes a multi-valued (MVBDD) variable with domain
{0, 1, ..., k-1}. For every edge (u, v) we AND the constraint
"x_u != x_v", i.e.,

    ~( OR_{c=0..k-1} (x_u == c AND x_v == c) ) ,

into the characteristic MVBDD F. A proper k-coloring is any MVBDD
assignment with F = 1. This sample reports:

  * the MVBDD and the underlying BDD size,
  * the exact #proper k-colorings (BigInt), computed by counting the
    satisfying assignments of F.to_bdd() over n*(k-1) one-hot DD vars,
  * up to 5 sample colorings (first-enumerated via MVZDD::enumerate),
  * the chromatic number obtained by searching k' = 1, 2, ..., k.

Usage:
    coloring <file.txt>

Input format
------------
First non-comment tokens: three integers n m k
  n = vertex count (vertices are 1..n)
  m = edge count
  k = target number of colors (>= 1)

Then m lines of "<u> <v>", one edge per line. Edges are undirected;
self-loops are ignored with a warning. Lines starting with 'c', 'C',
'#', or '%' are comments; blank lines are ignored.

Example (small.txt, triangle + pendant):
    4 4 3
    1 2
    2 3
    1 3
    1 4

Sample instances (run from the build directory):
    ./coloring ../app/coloring/small.txt      # triangle+pendant, chi=3, 12 proper 3-colorings
    ./coloring ../app/coloring/c5.txt         # 5-cycle, chi=3, 30 proper 3-colorings
    ./coloring ../app/coloring/petersen.txt   # Petersen graph, chi=3
    ./coloring ../app/coloring/k4.txt         # K_4, NOT 3-colorable (chi=4)

Notes
-----
* Colors are numbered 0..k-1 in the output. The label "v3=2" means
  vertex 3 is assigned color 2.
* Because MVBDD variables are shared with their internal MVZDD via the
  shared variable table, the enumerate() pass lists assignments in the
  structural order of the ZDD — not uniformly at random.
* The chromatic number search rebuilds the MVBDD from scratch for each
  k' = 1..k; variable pool grows each iteration but memory cost is
  trivial for the small educational instances in this directory.
