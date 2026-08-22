Coloring Reconfiguration — MVZDD-based shortest recoloring sequence
====================================================================

Given a graph G, a number of colors k, and two proper k-colorings s and
t, this sample decides whether s can be transformed into t by recoloring
one vertex at a time while keeping every intermediate coloring proper,
and if so reports a shortest such sequence.

The vertex set of the reconfiguration graph is the set of all proper
k-colorings, which is exponential in general, so it is never built
explicitly: the whole set is represented as one MVZDD and the
breadth-first search advances by MVZDD operations only. The algorithm
follows ddreconf (https://github.com/junkawahara/ddreconf), adapted from
subsets (token jumping) to multi-valued assignments.

Algorithm
---------
Each vertex v becomes a multi-valued (MVZDD) variable with domain
{0, ..., k-1}. The solution space S is the family of all proper
k-colorings, built as an MVBDD exactly as in app/coloring and then
converted to an MVZDD.

    F_0 = {s}
    F_i = ( S & recolor_one(F_{i-1}) ) - F_{i-1} - F_{i-2}

    t in F_i  ->  reachable, i is the shortest number of steps
    F_i = {}  ->  not reachable

recolor_one(F) is the family of all assignments obtained from a member of
F by changing exactly one vertex's color; it is the multi-valued
counterpart of ddreconf's swap_element. Subtracting only the two previous
layers is enough, because an edge of the reconfiguration graph changes
the BFS distance by at most one, so every neighbor of F_{i-1} lies in
F_{i-2}, F_{i-1} or F_i.

recolor_one is implemented on the one-hot encoded ZDD. An MV variable
with domain size k is backed by k-1 DD variables d_0, ..., d_{k-2}:
color 0 means no d_j is present, color c > 0 means exactly d_{c-1} is
present. So the members with a given color, with that vertex's bits
stripped, come from one onset0 plus offsets, and writing a new color
back is one change.

A shortest sequence is then recovered by walking the layers backwards
from t: at layer i the predecessor is any single-vertex recoloring of the
current coloring that is a member of F_i. The result is re-checked
against the graph before it is printed.

Usage
-----
    coloring_reconf <file.txt>

Input format
------------
This is the app/coloring format plus a start line and a goal line.

First non-comment tokens: three integers n m k
  n = vertex count (vertices are 1..n)
  m = edge count
  k = number of colors (>= 2)

Then m lines of "<u> <v>", one edge per line. Edges are undirected;
self-loops are ignored with a warning. Then two lines

  s <c1> <c2> ... <cn>     the start coloring
  t <c1> <c2> ... <cn>     the goal coloring

giving the color of each vertex 1..n, each in 0..k-1. The 's' and 't'
lines may appear in either order, and both must be proper colorings.
Lines starting with 'c', 'C', '#', or '%' are comments; blank lines are
ignored.

Example (small.txt, triangle + pendant):
    4 4 3
    1 2
    2 3
    1 3
    1 4
    s 0 1 2 1
    t 0 1 2 2

Sample instances (run from the build directory)
-----------------------------------------------
    ./coloring_reconf ../app/coloring_reconf/small.txt
        triangle + pendant, k=3. The triangle is frozen; only the pendant
        moves. REACHABLE in 1 step.

    ./coloring_reconf ../app/coloring_reconf/small-no.txt
        Same graph, but the goal permutes the triangle. NOT REACHABLE.

    ./coloring_reconf ../app/coloring_reconf/path4.txt
        Path on 4 vertices, k=3. REACHABLE in 4 steps.

    ./coloring_reconf ../app/coloring_reconf/c5.txt
        5-cycle, k=3. REACHABLE in 2 steps.

    ./coloring_reconf ../app/coloring_reconf/c5-4colors.txt
        5-cycle, k=4 >= Delta+2, so the reconfiguration graph is
        connected. REACHABLE in 4 steps.

    ./coloring_reconf ../app/coloring_reconf/k4.txt
        K_4 with k=5 >= Delta+2. REACHABLE in 4 steps.

    ./coloring_reconf ../app/coloring_reconf/k4-frozen.txt
        K_4 with k=4: every proper coloring is isolated. NOT REACHABLE.

    ./coloring_reconf ../app/coloring_reconf/petersen.txt
        Petersen graph, k=3: the 120 colorings split into 60 components
        of size 2. NOT REACHABLE.

    ./coloring_reconf ../app/coloring_reconf/petersen-4colors.txt
        Petersen graph, k=4: all 12960 colorings form one component.
        REACHABLE in 13 steps.

Notes
-----
* Colors are numbered 0..k-1 in both the input and the output. The label
  "v3=2" means vertex 3 is assigned color 2.
* k must be at least 2. With k = 1 there is a single candidate coloring
  and no recoloring step is possible.
* build_solution_space() expects a freshly initialized DD manager,
  because the BDD -> ZDD conversion fills every DD level up to the last
  variable of the table.
* The core routines live in the header coloring_reconf.hpp so that
  test/test_coloring_reconf.cpp can exercise them; main.cpp is only the
  command line driver.
* Layer sizes and shortest lengths of every sample instance above were
  checked against an independent brute-force BFS over the explicit
  reconfiguration graph.
